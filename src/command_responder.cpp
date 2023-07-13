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

#include "command_responder.h"

#include <Arduino.h>

int dispMode = 0;

void InitResponder()
{
  Serial.begin(115200);
}

namespace
{
  enum
  {
    COMMAND_SILENCE,
    COMMAND_UNKNOWN,
    COMMAND_stop,

    COMMAND_MAX
  };
  uint8_t scoreList[COMMAND_MAX];
  uint8_t lastCommand;
  int8_t lastCommandTime;
}

void RespondToCommand(tflite::ErrorReporter *error_reporter,
                      int32_t current_time, const char *found_command,
                      uint8_t score, bool is_new_command)
{
  static int32_t last_timestamp = 0;
  if (score < 150)
  {
    return;
  }
  // Score List Update
  uint8_t command = COMMAND_SILENCE;
  memset(scoreList, 0, sizeof(scoreList));
  if (strcmp(found_command, "silence") == 0)
  {
    command = COMMAND_SILENCE;
    return;
  }
  else if (strcmp(found_command, "unknown") == 0)
  {
    command = COMMAND_UNKNOWN;
    return;
  }
  else if (strcmp(found_command, "stop") == 0)
  {
    command = COMMAND_stop;
  }
  
  scoreList[command] = score;

  // New Command
  if (is_new_command)
  {
    lastCommand = command;
    lastCommandTime = 3;
  }

  Serial.printf("current_time(%d) found_command(%s) score(%d) is_new_command(%d)\n", current_time, found_command, score, is_new_command);
}

int drawWaveX = 160;
int drawWaveMin = 1000;
int drawWaveMax = -1000;
void drawWave(int16_t value)
{
}

void drawInput(uint8_t *uint8)
{
}
