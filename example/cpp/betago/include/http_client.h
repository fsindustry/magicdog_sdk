#pragma once

#include "config.h"

// HTTP客户端函数
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);
bool upload_image(const std::shared_ptr<magic::dog::CompressedImage>& msg, std::string& response);
bool upload_audio(const std::shared_ptr<magic::dog::ByteMultiArray>& msg, std::string& response);