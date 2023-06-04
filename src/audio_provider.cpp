/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
  ==============================================================================*/

#include "audio_provider.h"
#include "micro_model_settings.h"
#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_NUM           I2S_NUM_0           // 0 or 1
#define I2S_SAMPLE_RATE   16000

#define I2S_PIN_CLK       26
#define I2S_PIN_WS        32
#define I2S_PIN_DOUT      I2S_PIN_NO_CHANGE
#define I2S_PIN_DIN       33

#define BUFFER_SIZE       512

void CaptureSamples();
extern QueueHandle_t xQueueAudioWave;

namespace {
bool g_is_audio_initialized = false;
// An internal buffer able to fit 16x our sample size
// 能够容纳16倍样本大小的内部缓冲区
constexpr int kAudioCaptureBufferSize = BUFFER_SIZE * 16;

// 1s的采样数据
int16_t g_audio_capture_buffer[kAudioCaptureBufferSize];

// A buffer that holds our output
// 保存输出的缓冲区
int16_t g_audio_output_buffer[kMaxAudioSampleSize];

// Mark as volatile so we can check in a while loop to see if
// any samples have arrived yet.
// 标记为易失性，这样我们就可以在while循环中检查是否有样品到达。
volatile int32_t g_latest_audio_timestamp = 0;

// Our callback buffer for collecting a chunk of data
// 用于收集数据块的回调缓冲区
volatile int16_t recording_buffer[BUFFER_SIZE];

}  // namespace

void InitI2S()
{
  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX), // 主机/接收模式
    .sample_rate          = I2S_SAMPLE_RATE, // 采样频率 16kHz
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT, // 样本量化等级 2字节
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT, // 仅左声道；FMT是音频文件格式的缩写，它是一种基于WAV文件格式的音频数据压缩技术
    .communication_format = I2S_COMM_FORMAT_I2S, // 通讯格式：I2S
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,

    .dma_buf_count        = 4, // 接收/传输数据的DMA缓冲区的总数。
                               // 描述符包括一些信息，如缓冲区地址、下一个描述符的地址和缓冲区长度。
                               // 由于一个描述符指向一个缓冲区，因此，'dma_desc_num'可以解释为D的总数。
                               // 注意，这些缓冲区位于'i2s_read'的内部，并且描述符是在I2S内部自动创建的
                               // 用户只需要设置缓冲区编号，而长度是从下面描述的参数派生的。

    .dma_buf_len          = 256, // DMA缓冲区中的帧数。
                                 // 一个帧是指一个WS周期中所有通道的数据。
                                 // real_dma_buf_size = dma_buf_len * chan_num * bits_per_chan / 8。
                                 // 例如，如果两个声道在立体声模式(即，'channel_format'被设置为'I2S_CHANNEL_FMT_RIGHT_LEFT
                                 // 每个通道传输32位(即'bits_per_sample'设置为'I2S_BITS_PER_CHAN_32BIT')，
                                 // 那么一个帧的总字节数是'channel_format' * 'bits_per_sample' = 2 * 32 / 8 =
                                 // 我们假设当前的'dma_buf_len'是100，那么DMA缓冲区的实际长度是8 * 10
                                 // 注意，内部实际DMA缓冲区的长度不应该大于4092。

    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };
  i2s_pin_config_t pin_config = {
    .bck_io_num           = I2S_PIN_CLK, // 26
    .ws_io_num            = I2S_PIN_WS, // 32
    .data_out_num         = I2S_PIN_DOUT,
    .data_in_num          = I2S_PIN_DIN, // 33
  };

  // 配置i2s
  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  // 配置引脚
  i2s_set_pin(I2S_NUM, &pin_config);
  // 配置采样频率，量化等级，单声道
  i2s_set_clk(I2S_NUM, I2S_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

/**
 * 录音任务
*/
void AudioRecordingTask(void *pvParameters) {
  static uint16_t audio_idx = 0;
  size_t bytes_read;
  int16_t i2s_data;
  int16_t sample;

  while (1) {

    if (audio_idx >= BUFFER_SIZE) {
      /**
       * 这是一个调用xQueueGenericSend()的宏。
       * 包含它是为了向后兼容不包含xQueueSendToFront()和xQueueSendToBack()宏的FreeRTOS.org版本。
       * 它相当于xQueueSendToBack()。
       * 
       * 在队列上张贴一个项目。
       * 项按拷贝而不是按引用排队。
       * 这个函数不能从中断服务例程中调用。
       * 请参阅xQueueSendFromISR()以获取可在ISR中使用的替代方法。
       * 
       * @param xQueue 队列的句柄，项目将在该队列上发布。
       * @param pvItemToQueue
       * 指向要放在队列上的项的指针。
       * 时定义队列将保存的项的大小
       * 队列被创建，所以这些字节将从pvItemToQueue复制
       * 到队列存储区域。
       * 
       * @param xTicksToWait
       * 如果队列已经满了，任务应该阻塞等待队列上可用空间的最大时间。
       * 如果该值设置为0并且队列已满，则调用将立即返回。
       * 时间以刻度周期定义，因此如果需要，应该使用常量portTICK_PERIOD_MS来转换为实时时间。
       * 
       * @return pdTRUE 如果条目已成功发布，则返回errQUEUE_FULL。
      */
      xQueueSend(xQueueAudioWave, &sample, 0);

      // 循环512次，经历32ms，抓取一次样本，一次抓取512个样本，1024字节
      CaptureSamples();

      audio_idx = 0;
    }

    /**
      * 从I2S DMA接收缓冲区读取数据
      * 【每次读取1个样本，2字节数据】
      *
      * @param i2s_num I2S端口号
      * @param dest 要读入的目的地址
      * @param size 以字节为单位的数据大小
      * @param[out] bytes_read 读取的字节数，如果超时，读取的字节数将小于传入的大小。
      * @param ticks_to_wait   RX缓冲区等待超时在RTOS滴答。
      *                        如果经过这么多的时间，而DMA接收缓冲区中没有可用的字节，则该函数将返回。
      *                        (注意，如果从DMA缓冲区中分批读取数据，则整体操作可能仍需要比此超时更长时间)
      *                        不超时地传递portMAX_DELAY。
      * 
      * @note 如果启用了内置ADC模式，我们应该在整个读取过程中调用i2s_adc_enable和i2s_adc_disable，以防止数据损坏。
      * @return
      *     - ESP_OK               Success
      *     - ESP_ERR_INVALID_ARG  Parameter error
      */
    i2s_read(I2S_NUM_0, &i2s_data, 2, &bytes_read, portMAX_DELAY );

    if (bytes_read > 0) {
      //sample = (0xfff - (i2s_data & 0xfff)) - 0x800;
      sample = i2s_data;
      // 样本数据存储到样本缓冲区
      recording_buffer[audio_idx] = sample;
      audio_idx++;
    }

  }
}

/**
 * 每调用一次，g_latest_audio_timestamp 增加32ms, 512个样本，1024字节
*/
void CaptureSamples() {
  // This is how many bytes of new data we have each time this is called
  // 这是每次调用这个函数时我们有多少新数据
  const int number_of_samples = BUFFER_SIZE;

  // Calculate what timestamp the last audio sample represents
  // 计算最后一个音频样本所代表的时间戳
  const int32_t time_in_ms = // 32ms, 64ms, 96ms, 128ms, ...
    g_latest_audio_timestamp +
    (number_of_samples / (kAudioSampleFrequency / 1000)); // 每次 + 32ms

  // Determine the index, in the history of all samples, of the last sample
  // 确定所有样本历史中最后一个样本的索引
  const int32_t start_sample_offset =
    g_latest_audio_timestamp * (kAudioSampleFrequency / 1000); // 0, 512, 1024, 1536, 2048, ... , 7680, 8192, 8704, ...

  // Determine the index of this sample in our ring buffer
  // 确定此示例在环形缓冲区中的索引
  const int capture_index = start_sample_offset % kAudioCaptureBufferSize; // 0, 512, 1024, 1536, 2048, ... , 7680, 0, 512, ...

  // Read the data to the correct place in our buffer, note 2 bytes per buffer entry
  // 将数据读入缓冲区中的正确位置，注意每个缓冲区条目2字节
  memcpy(g_audio_capture_buffer + capture_index, (void *)recording_buffer, BUFFER_SIZE * 2);

  // This is how we let the outside world know that new audio data has arrived.
  // 这就是我们让外界知道新的音频数据已经到来的方式。
  g_latest_audio_timestamp = time_in_ms;

  //int peak = (max_audio - min_audio);
  //Serial.printf("peak-to-peak:  %6d\n", peak);
}

// 初始化一次
TfLiteStatus InitAudioRecording(tflite::ErrorReporter* error_reporter) {
  delay(10);

  // 初始化配置
  InitI2S();

  /**
   * 创建具有指定关联的新任务。
   * 该函数类似于xTaskCreate，但允许设置任务关联在SMP系统中。
   * 
   * @param pvTaskCode 任务输入函数的指针。任务必须实现永远不会返回(即连续循环)，还是应该使用vTaskDelete函数终止。
   * @param pcName 任务描述性名称。这主要是用来方便调试。由configMAX_TASK_NAME_LEN定义的最大长度- default是16。
   * @param usStackDepth 任务栈的大小，指定为number字节。注意，这与普通的FreeRTOS不同。
   * @param pvParameters 将被用作任务参数的指针被创建。
   * @param uxPriority 任务运行的优先级。系统, 包括MPU支持可以选择性地在特权系统中创建任务 设置优先级参数的portPRIVILEGE_BIT位。为
   * 例如，使用uxPriority参数创建优先级为2的特权任务 应该设置为(2 | portPRIVILEGE_BIT)。
   * @param pvCreatedTask 用于返回一个句柄，通过该句柄创建的任务 可参考。
   * @param xCoreID 如果值为tskNO_AFFINITY，表示创建的任务不是固定在任何CPU上，调度器可以在任何可用的核心上运行它。0或1表示任务应该使用的CPU索引号
   * 被钉住。指定大于(portNUM_PROCESSORS - 1)的值将会导致函数失败。
   * @返回pdPASS，如果任务已成功创建并添加到ready
   * 列表，否则是文件projdefs.h中定义的错误代码
  */
  xTaskCreatePinnedToCore(
    AudioRecordingTask, 
    "AudioRecordingTask", 
    2048, 
    NULL, 
    10, 
    NULL, 
    0);

  // 直到我们得到第一个音频样本
  while (!g_latest_audio_timestamp) {
    delay(1);
  }

  return kTfLiteOk;
}

/**
 * 一次回调 30ms 512个样本 1024字节
*/
TfLiteStatus GetAudioSamples(tflite::ErrorReporter* error_reporter,
                             int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) {
  // Set everything up to start receiving audio
  // 设置好一切，开始接收音频
  if (!g_is_audio_initialized) {
    TfLiteStatus init_status = InitAudioRecording(error_reporter);
    if (init_status != kTfLiteOk) {
      return init_status;
    }
    g_is_audio_initialized = true;
  }
  // This next part should only be called when the main thread notices that the
  // latest audio sample data timestamp has changed, so that there's new data
  // in the capture ring buffer. The ring buffer will eventually wrap around and
  // overwrite the data, but the assumption is that the main thread is checking
  // often enough and the buffer is large enough that this call will be made
  // before that happens.

  // 下一部分应该只在主线程注意到最近的音频样本数据时间戳发生变化时调用，这样在捕获环缓冲区中就有了新数据。
  // 环形缓冲区最终将环绕并覆盖数据，但假设主线程经常检查并且缓冲区足够大，可以在此发生之前进行此调用。

  // Determine the index, in the history of all samples, of the first
  // sample we want
  // 在所有样本的历史中，确定我们想要的第一个样本的索引
  const int start_offset = start_ms * (kAudioSampleFrequency / 1000);
  // Determine how many samples we want in total
  // 确定我们总共需要多少个样本
  const int duration_sample_count =
    duration_ms * (kAudioSampleFrequency / 1000);
  for (int i = 0; i < duration_sample_count; ++i) {

    // For each sample, transform its index in the history of all samples into
    // its index in g_audio_capture_buffer

    // 对于每个示例，将其在所有示例的历史记录中的索引转换为其在g_audio_capture_buffer中的索引
    const int capture_index = (start_offset + i) % kAudioCaptureBufferSize;

    // Write the sample to the output buffer
    // 将示例写入输出缓冲区
    g_audio_output_buffer[i] = g_audio_capture_buffer[capture_index];
  }

  // Set pointers to provide access to the audio
  // 设置指针以提供对音频的访问
  *audio_samples_size = kMaxAudioSampleSize;
  *audio_samples = g_audio_output_buffer;

  return kTfLiteOk;
}

int32_t LatestAudioTimestamp() {
  return g_latest_audio_timestamp;
}
