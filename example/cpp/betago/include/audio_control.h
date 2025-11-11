#pragma once

#include "config.h"

// 音频控制函数
int initial_audio_controller();
int close_audio_controller();
void greetings(const std::string& name);
bool get_volume(int& volume);
bool set_volume(int volume);