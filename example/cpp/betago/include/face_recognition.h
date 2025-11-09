#pragma once

#include "config.h"

// 人脸识别函数
std::string get_face_name(std::string& response);
void (*receive_img())(std::shared_ptr<magic::dog::CompressedImage>);