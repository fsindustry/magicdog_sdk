#pragma once

#include "config.h"

// 语音识别函数
bool contains_any(const std::string& text, const std::vector<std::string>& keys);
void (*receive_voice())(std::shared_ptr<magic::dog::ByteMultiArray>);