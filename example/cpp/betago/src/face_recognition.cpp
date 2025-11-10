#include "face_recognition.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "audio_control.h"
#include "http_client.h"

std::string get_face_name(std::string& response) {
  try {
    nlohmann::json j = nlohmann::json::parse(response);

    if (j.contains("data")) {
      auto data = j["data"];
      std::string data_status = data.value("status", "");
      if (data_status == "success") {
        double similarity = data.value("similarity", 0.0);
        if (similarity > 0.4) {
          return data.value("name", "");
        }
      }
    } else {
      std::cerr << "JSON does not contain 'data'" << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "Exception in get_face_name: " << e.what() << std::endl;
  }
  return "";
}

// 辅助函数：生成带时间戳的文件名
std::string generate_timestamp_filename(const std::string& prefix, const std::string& extension) {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()) % 1000;

  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
  ss << "_" << std::setfill('0') << std::setw(3) << ms.count();
  return prefix + "_" + ss.str() + extension;
}

// 保存图像到本地文件
bool save_image_to_local(const std::shared_ptr<magic::dog::CompressedImage>& image,
                        const std::string& filename) {
  try {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
      std::cerr << "无法创建文件: " << filename << std::endl;
      return false;
    }

    file.write(reinterpret_cast<const char*>(image->data.data()), image->data.size());
    file.close();

    if (!file.good()) {
      std::cerr << "写入文件失败: " << filename << std::endl;
      return false;
    }

    std::cout << "图像已保存到: " << filename << " (大小: " << image->data.size() << " 字节)" << std::endl;
    return true;
  } catch (const std::exception& e) {
    std::cerr << "保存图像异常: " << e.what() << std::endl;
    return false;
  }
}

void (*receive_img())(std::shared_ptr<magic::dog::CompressedImage>) {
  return [](const std::shared_ptr<magic::dog::CompressedImage> msg) {
    auto now = std::chrono::steady_clock::now();

    {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_state.last_request_time).count() < REQUEST_COOLDOWN_MS)
        return;
      g_state.last_request_time = now;
    }
    
    // // 保存图像到本地
    // std::string local_filename = generate_timestamp_filename("face_capture", ".jpg");
    // if (!save_image_to_local(msg, local_filename)) {
    //   std::cerr << "保存本地图像失败，但仍继续处理人脸识别" << std::endl;
    // }

    std::string response;
    if (!upload_image(msg, response)) {
      return;
    }

    const std::string name = get_face_name(response);
    if (name.empty()) {
      g_state.last_name = "";
      return;
    }

    {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_state.last_greeted_time).count();
      if (name == g_state.last_name && diff < SAME_PERSON_DELAY_MS) {
        std::cout << "Detected same person (" << name << ") within "
                  << SAME_PERSON_DELAY_MS << "ms, skip greeting." << std::endl;
        return;
      }

      g_state.last_name = name;
      g_state.last_greeted_time = now;
    }

    std::cout << "人脸识别结果: " << name << std::endl;
    greetings(name);
  };
}