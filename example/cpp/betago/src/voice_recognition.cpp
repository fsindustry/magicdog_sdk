#include "voice_recognition.h"
#include <functional>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include "http_client.h"
#include "motion_control.h"
// #include "nav_control.h"

bool contains_any(const std::string& text, const std::vector<std::string>& keys) {
  for (const auto& key : keys) {
    if (text.find(key) != std::string::npos)
      return true;
  }
  return false;
}

const std::map<std::vector<std::string>, std::function<void()>> actions = {
    {{"跳舞", "跳个舞", "跳支舞"}, []() {
       Dancing();
     }},
    {{"握手", "握个手", "握握手"}, []() {
       JoyStickCommand(0.0, 0.0, 0.0, 0.0);
       ExecuteTrickAction(magic::dog::TrickAction::ACTION_SHAKE_RIGHT_HAND, "ACTION_SHAKE_RIGHT_HAND");
     }},
    // {{"导航", "导航到", "导航至"}, []() {
    //
    //  }},
    // {{"返程", "返回", "返航"}, []() {
    //
    //  }},
    // {{"停止", "停止导航", "取消", "取消导航"}, []() {
    //    cancelNavigation();
    //  }},
    // {{"暂停", "暂停导航"}, []() {
    //    pauseNavigation();
    //  }},
    // {{"继续", "继续导航", "恢复", "恢复导航"}, []() {
    //    resumeNavigation();
    //  }},
    // {{"关闭", "关闭导航"}, []() {
    //    closeNavigation();
    //  }},
};

void (*receive_voice())(std::shared_ptr<magic::dog::ByteMultiArray>) {
  return [](const std::shared_ptr<magic::dog::ByteMultiArray> data) {
    auto now = std::chrono::steady_clock::now();

    {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_state.last_request_time).count() < REQUEST_COOLDOWN_MS)
        return;
      g_state.last_request_time = now;
    }

    std::cout << "Received BF voice data, size: " << data->data.size() << std::endl;

    std::string response;
    if (!upload_audio(data, response)) {
      return;
    }

    std::string txt = "";
    try {
      nlohmann::json j = nlohmann::json::parse(response);
      if (j.contains("data")) {
        txt = j.value("data", "");
      }
    } catch (const std::exception& e) {
      std::cerr << "Exception parsing voice response: " << e.what() << std::endl;
    }

    bool matched = false;
    for (const auto& kv : actions) {
      if (contains_any(txt, kv.first)) {
        kv.second();
        matched = true;
        break;
      }
    }

    if (!matched)
      std::cout << "未匹配到任何动作" << std::endl;
  };
}