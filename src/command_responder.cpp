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
    // COMMAND_go,
    // COMMAND_happy,
    // COMMAND_house,
    // COMMAND_learn,
    COMMAND_left,
    COMMAND_right,
    // COMMAND_marvin,
    // COMMAND_yes,
    // COMMAND_no,
    // COMMAND_follow,
    // COMMAND_forward,
    // COMMAND_backward,
    // COMMAND_bed,
    // COMMAND_tree,
    // COMMAND_bird,
    // COMMAND_cat,
    // COMMAND_dog,
    // COMMAND_up,
    // COMMAND_down,
    // COMMAND_sheila,
    // COMMAND_visual,
    // COMMAND_wow,
    // COMMAND_zero,
    // COMMAND_one,
    // COMMAND_two,
    // COMMAND_three,
    // COMMAND_four,
    // COMMAND_five,
    // COMMAND_six,
    // COMMAND_seven,
    // COMMAND_eight,
    // COMMAND_nine,
    // COMMAND_on,
    // COMMAND_off,
    // COMMAND_stop,

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
  // else if (strcmp(found_command, "go") == 0)
  // {
  //   command = COMMAND_go;
  // }
  // else if (strcmp(found_command, "happy") == 0)
  // {
  //   command = COMMAND_happy;
  // }
  // else if (strcmp(found_command, "house") == 0)
  // {
  //   command = COMMAND_house;
  // }
  // else if (strcmp(found_command, "learn") == 0)
  // {
  //   command = COMMAND_learn;
  // }
  else if (strcmp(found_command, "left") == 0)
  {
    command = COMMAND_left;
  }
  else if (strcmp(found_command, "right") == 0)
  {
    command = COMMAND_right;
  }
  // else if (strcmp(found_command, "marvin") == 0)
  // {
  //   command = COMMAND_marvin;
  // }
  // else if (strcmp(found_command, "yes") == 0)
  // {
  //   command = COMMAND_yes;
  // }
  // else if (strcmp(found_command, "no") == 0)
  // {
  //   command = COMMAND_no;
  // }
  // else if (strcmp(found_command, "follow") == 0)
  // {
  //   command = COMMAND_follow;
  // }
  // else if (strcmp(found_command, "forward") == 0)
  // {
  //   command = COMMAND_forward;
  // }
  // else if (strcmp(found_command, "backward") == 0)
  // {
  //   command = COMMAND_backward;
  // }
  // else if (strcmp(found_command, "bed") == 0)
  // {
  //   command = COMMAND_bed;
  // }
  // else if (strcmp(found_command, "tree") == 0)
  // {
  //   command = COMMAND_tree;
  // }
  // else if (strcmp(found_command, "bird") == 0)
  // {
  //   command = COMMAND_bird;
  // }
  // else if (strcmp(found_command, "cat") == 0)
  // {
  //   command = COMMAND_cat;
  // }
  // else if (strcmp(found_command, "dog") == 0)
  // {
  //   command = COMMAND_dog;
  // }
  // else if (strcmp(found_command, "up") == 0)
  // {
  //   command = COMMAND_up;
  // }
  // else if (strcmp(found_command, "down") == 0)
  // {
  //   command = COMMAND_down;
  // }
  // else if (strcmp(found_command, "sheila") == 0)
  // {
  //   command = COMMAND_sheila;
  // }
  // else if (strcmp(found_command, "visual") == 0)
  // {
  //   command = COMMAND_visual;
  // }
  // else if (strcmp(found_command, "wow") == 0)
  // {
  //   command = COMMAND_wow;
  // }
  // else if (strcmp(found_command, "zero") == 0)
  // {
  //   command = COMMAND_zero;
  // }
  // else if (strcmp(found_command, "one") == 0)
  // {
  //   command = COMMAND_one;
  // }
  // else if (strcmp(found_command, "two") == 0)
  // {
  //   command = COMMAND_two;
  // }
  // else if (strcmp(found_command, "three") == 0)
  // {
  //   command = COMMAND_three;
  // }
  // else if (strcmp(found_command, "four") == 0)
  // {
  //   command = COMMAND_four;
  // }
  // else if (strcmp(found_command, "five") == 0)
  // {
  //   command = COMMAND_five;
  // }
  // else if (strcmp(found_command, "six") == 0)
  // {
  //   command = COMMAND_six;
  // }
  // else if (strcmp(found_command, "seven") == 0)
  // {
  //   command = COMMAND_seven;
  // }
  // else if (strcmp(found_command, "eight") == 0)
  // {
  //   command = COMMAND_eight;
  // }
  // else if (strcmp(found_command, "nine") == 0)
  // {
  //   command = COMMAND_nine;
  // }
  // else if (strcmp(found_command, "on") == 0)
  // {
  //   command = COMMAND_on;
  // }
  // else if (strcmp(found_command, "off") == 0)
  // {
  //   command = COMMAND_off;
  // }
  // else if (strcmp(found_command, "stop") == 0)
  // {
  //   command = COMMAND_stop;
  // }
  
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
