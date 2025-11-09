#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <mutex>
#include "magic_robot.h"

// 全局变量声明
extern magic::dog::MagicRobot robot;
extern std::atomic<bool> is_running;
extern magic::dog::GaitMode target_gait;

// 摇杆控制变量
extern std::atomic<float> left_x_axis;
extern std::atomic<float> left_y_axis;
extern std::atomic<float> right_x_axis;
extern std::atomic<float> right_y_axis;
extern std::atomic<float> left_x_axis_gain;
extern std::atomic<float> left_y_axis_gain;
extern std::atomic<float> right_x_axis_gain;
extern std::atomic<float> right_y_axis_gain;

// 服务器配置
extern const std::string IMAGE_SERVER_URL;
extern const std::string VOICE_SERVER_URL;
extern const int SAME_PERSON_DELAY_MS;
extern const int REQUEST_COOLDOWN_MS;

// 成员信息结构
struct member {
  uint64_t command_id;
  std::string greeting;
  std::string department;
};

// 贝塔狗成员数据库
extern std::unordered_map<std::string, member> BETAGO_MEMBERS;
extern const std::vector<std::string> DEFAULT_GREETING_TEMPLATES;

// 状态结构
struct GreetingState {
  std::string last_name;
  std::chrono::steady_clock::time_point last_greeted_time;
  std::chrono::steady_clock::time_point last_request_time;
  std::mutex mutex;
};

struct VoiceState {
  std::chrono::steady_clock::time_point last_request_time;
  std::mutex mutex;
};

extern GreetingState g_state;
extern VoiceState v_state;