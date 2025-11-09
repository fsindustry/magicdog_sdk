#include "face_recognition.h"
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
        if (similarity > 0.85) {
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

void (*receive_img())(std::shared_ptr<magic::dog::CompressedImage>) {
  return [](const std::shared_ptr<magic::dog::CompressedImage> msg) {
    auto now = std::chrono::steady_clock::now();

    {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_state.last_request_time).count() < REQUEST_COOLDOWN_MS)
        return;
      g_state.last_request_time = now;
    }

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