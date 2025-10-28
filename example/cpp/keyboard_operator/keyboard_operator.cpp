#include "magic_robot.h"

#include <curl/curl.h>
#include <termios.h>
#include <unistd.h>

#include <csignal>
#include <fstream>

#include <iostream>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <thread>

using namespace magic::dog;

magic::dog::MagicRobot robot;
std::atomic<bool> is_running(true);

GaitMode target_gait = GaitMode::GAIT_DOWN_CLIMB_STAIRS;

std::atomic<float> left_x_axis(0.0);
std::atomic<float> left_y_axis(0.0);
std::atomic<float> right_x_axis(0.0);
std::atomic<float> right_y_axis(0.0);

std::atomic<float> left_x_axis_gain(0.0);
std::atomic<float> left_y_axis_gain(0.0);
std::atomic<float> right_x_axis_gain(0.0);
std::atomic<float> right_y_axis_gain(0.0);

// 目标服务器 URL
const std::string SERVER_URL = "http://120.92.77.233:3999/face/frame";

struct member {
  uint64_t command_id;
  std::string greeting;
  std::string department;
};

std::unordered_map<std::string, member> BETAGO_MEMBERS = {
    {"富正鑫", {100000000001, "富哥好！今天又是元气满满的一天吗？", "智算云，数据平台研发部，数据库研发组"}},
    {"卢祚", {100000000002, "卢总好！福气满满的卢总今天有什么好运？", "智算云，数据平台研发部，数据库研发组"}},
    {"肖文然", {100000000003, "文然兄好！文艺气息扑面而来！", "智算云，数据平台研发部，数据库研发组"}},
    {"陈嘉敏", {100000000004, "嘉敏好！聪明伶俐的你今天有什么新发现？", "智算云，数据平台研发部，数据库研发组"}},
    {"任阿伟", {100000000005, "阿伟好！伟大的任务等着你去完成！", "智算云，数据平台研发部，数据库研发组"}},
    {"高名发", {100000000006, "名发哥好！名声和发财两不误！", "智算云，数据平台研发部，数据库研发组"}},
    {"鞠鑫锐", {100000000007, "鑫锐好！三金加持，锐不可当！", "智算云，数据平台研发部，数据库研发组"}},
    {"宋卓著", {100000000008, "卓著好！卓越成就的你又有什么新突破？", "智算云，数据平台研发部，数据库研发组"}},
    {"吴敬超", {100000000009, "敬超好！令人敬佩的超凡能力！", "智算云，数据平台研发部，数据库研发组"}},
    {"崔照斌", {100000000010, "照斌好！照亮全场的文武双全！", "智算云，数据平台研发部，数据库研发组"}},
    {"唐贵乾", {100000000011, "贵乾好！富贵乾坤尽在掌握！", "智算云，数据平台研发部，数据库研发组"}}};

void signalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received.\n";
  is_running.store(false);

  robot.Shutdown();
  // Exit process
  exit(signum);
}

void print_help(const char* prog_name) {
  std::cout << "Key Function Demo Program\n\n";
  std::cout << "Usage: " << prog_name << "\n";
  std::cout << "Key Function Description:\n";
  std::cout << "  ESC      Exit program\n";
  std::cout << "  1        Position control standing\n";
  std::cout << "  2        Force control standing\n";
  std::cout << "  3        down climb stairs\n";
  std::cout << "  4        up climb stairs\n";
  std::cout << "  5        walk\n";
  std::cout << "  g        Execute trick - shake right hand\n";
  std::cout << "  f        Execute trick - front flip\n";
  std::cout << "  r        Execute trick - back flip\n";
  std::cout << "  c        Execute trick - sit down\n";
  std::cout << "  z        Execute trick - lie down\n";
  std::cout << "  space    Execute trick - jump\n";
  std::cout << "  w        Move forward\n";
  std::cout << "  a        Move left\n";
  std::cout << "  s        Move backward\n";
  std::cout << "  d        Move right\n";
  std::cout << "  x        Stop movement\n";
  std::cout << "  q        Turn left\n";
  std::cout << "  e        Turn right\n";
}

int getch() {
  struct termios oldt, newt;
  int ch;
  tcgetattr(STDIN_FILENO, &oldt);  // Get current terminal settings
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);  // Disable buffering and echo
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  ch = getchar();                           // Read key press
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // Restore settings
  return ch;
}

void RecoveryStand() {
  // Get high level motion controller
  auto& controller = robot.GetHighLevelMotionController();

  // Set gait
  auto status = controller.SetGait(GaitMode::GAIT_STAND_R);
  if (status.code != ErrorCode::OK) {
    std::cerr << "Set robot gait failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    return;
  }
}

void BalanceStand() {
  // Get high level motion controller
  auto& controller = robot.GetHighLevelMotionController();

  // Set posture display gait
  auto status = controller.SetGait(GaitMode::GAIT_STAND_B);
  if (status.code != ErrorCode::OK) {
    std::cerr << "Set robot gait failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    return;
  }
  std::cout << "Robot gait set to GAIT_BALANCE_STAND successfully." << std::endl;
}

void DownClimbStairs() {
  // Get high level motion controller
  auto& controller = robot.GetHighLevelMotionController();

  // Set posture display gait
  target_gait = GaitMode::GAIT_DOWN_CLIMB_STAIRS;
  auto status = controller.SetGait(GaitMode::GAIT_DOWN_CLIMB_STAIRS);
  if (status.code != ErrorCode::OK) {
    std::cerr << "Set robot gait failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    return;
  }
  std::cout << "Robot gait set to GAIT_DOWN_CLIMB_STAIRS successfully." << std::endl;
}

void UpClimbStairs() {
  // Get high level motion controller
  auto& controller = robot.GetHighLevelMotionController();

  // Set posture display gait
  target_gait = GaitMode::GAIT_UP_CLIMB_STAIRS;
  auto status = controller.SetGait(GaitMode::GAIT_UP_CLIMB_STAIRS);
  if (status.code != ErrorCode::OK) {
    std::cerr << "Set robot gait failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    return;
  }
  std::cout << "Robot gait set to GAIT_UP_CLIMB_STAIRS successfully." << std::endl;
}

void Walk() {
  // Get high level motion controller
  auto& controller = robot.GetHighLevelMotionController();

  // Set posture display gait
  target_gait = GaitMode::GAIT_WALK;
  auto status = controller.SetGait(GaitMode::GAIT_WALK);
  if (status.code != ErrorCode::OK) {
    std::cerr << "Set robot gait failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    return;
  }
  std::cout << "Robot gait set to GAIT_WALK successfully." << std::endl;
}

void ExecuteTrickAction(const TrickAction action, const std::string& action_name) {
  auto& controller = robot.GetHighLevelMotionController();

  auto status = controller.ExecuteTrick(action);
  if (status.code != ErrorCode::OK) {
    std::cerr << "Execute robot trick failed: " << action_name
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    return;
  }

  std::cout << "Robot " << action_name << " executed successfully." << std::endl;
}


void JoyStickCommand(float left_x_axis,
                     float left_y_axis,
                     float right_x_axis,
                     float right_y_axis) {
  ::left_x_axis.store(left_x_axis);
  ::left_y_axis.store(left_y_axis);
  ::right_x_axis.store(right_x_axis);
  ::right_y_axis.store(right_y_axis);
}

void send_motion_cmd() {
  auto& controller = robot.GetHighLevelMotionController();
  while (is_running.load()) {
    JoystickCommand joy_command;
    joy_command.left_x_axis = left_x_axis;
    joy_command.left_y_axis = left_y_axis;
    joy_command.right_x_axis = right_x_axis;
    joy_command.right_y_axis = right_y_axis;
    auto status = controller.SendJoyStickCommand(joy_command);
    if (status.code != ErrorCode::OK) {
      std::cerr << "Send joystick command failed"
                << ", code: " << status.code
                << ", message: " << status.message << std::endl;
    }

    static double last_left_x_axis_v = -1;
    static double last_left_y_axis_v = -1;
    static double last_right_x_axis_v = -1;
    static double last_right_y_axis_v = -1;

    double left_x_axis_v_current = left_x_axis * left_x_axis_gain;
    double left_y_axis_v_current = left_y_axis * left_y_axis_gain;
    double right_x_axis_v_current = right_x_axis * right_x_axis_gain;
    double right_y_axis_v_current = right_y_axis * right_y_axis_gain;

    if (std::abs(left_x_axis_v_current - last_left_x_axis_v) > 0.00001 ||
        std::abs(left_y_axis_v_current - last_left_y_axis_v) > 0.00001 ||
        std::abs(right_x_axis_v_current - last_right_x_axis_v) > 0.00001 ||
        std::abs(right_y_axis_v_current - last_right_y_axis_v) > 0.00001) {
      std::cout << "left_x_v: " << left_x_axis_v_current << ", left_y_v: " << left_y_axis_v_current << ", right_x_v: " << right_x_axis_v_current << ", right_y_v: " << right_y_axis_v_current << std::endl;
      last_left_x_axis_v = left_x_axis_v_current;
      last_left_y_axis_v = left_y_axis_v_current;
      last_right_x_axis_v = right_x_axis_v_current;
      last_right_y_axis_v = right_y_axis_v_current;
    }

    usleep(10000);
  }
}

int initial_robot() {
  std::string local_ip = "192.168.54.10";
  // Configure local IP address for direct network connection to machine and initialize SDK
  if (!robot.Initialize(local_ip)) {
    std::cerr << "Robot SDK initialization failed." << std::endl;
    robot.Shutdown();
    return -1;
  }

  Status status = robot.Connect();
  if (status.code != ErrorCode::OK) {
    std::cerr << "Connect robot failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }
  return 0;
}

// 用于接收 HTTP 响应
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
  size_t total_size = size * nmemb;
  output->append((char*)contents, total_size);
  return total_size;
}

constexpr int GREETING_INTERVAL_MS = 5000;  // 5秒
std::chrono::time_point<std::chrono::steady_clock> last_time = std::chrono::steady_clock::now();

void greetings(const std::string& response) {
  // 使用静态变量记录上一次执行时间
  last_time = std::chrono::steady_clock::now() - std::chrono::milliseconds(GREETING_INTERVAL_MS);

  auto now = std::chrono::steady_clock::now();
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count();

  // 如果距离上次执行未超过指定时间，直接返回
  if (duration_ms < GREETING_INTERVAL_MS) {
    return;
  }

  // 更新 last_time
  last_time = now;

  // 解析 JSON
  nlohmann::json j = nlohmann::json::parse(response);

  // 检查 data 是否存在
  if (j.contains("data")) {
    auto data = j["data"];

    // 获取 data.status
    std::string data_status = data.value("status", "");
    if (data_status == "success") {
      // 获取 data.name
      std::string name = data.value("name", "");
      std::cout << "data.name: " << name << std::endl;
      auto it = BETAGO_MEMBERS.find(name);
      if (it != BETAGO_MEMBERS.end()) {
        const member& m = it->second;

        auto& controller = robot.GetAudioController();
        // Play voice
        TtsCommand tts;
        tts.id = std::to_string(m.command_id);
        tts.content = m.greeting;
        tts.priority = TtsPriority::HIGH;
        tts.mode = TtsMode::CLEARTOP;
        Status status = controller.Play(tts);
        if (status.code != ErrorCode::OK) {
          std::cerr << "Play TTS failed"
                    << ", code: " << status.code
                    << ", message: " << status.message << std::endl;
        }
      }
    }
  } else {
    std::cerr << "JSON does not contain 'data'" << std::endl;
  }
}

int initial_audio_controller() {
  auto& controller = robot.GetAudioController();
  int get_volume = 0;
  Status status = controller.GetVolume(get_volume);
  if (status.code != ErrorCode::OK) {
    std::cerr << "Get volume failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  std::cout << "Get volume success, volume: " << std::to_string(get_volume) << std::endl;

  // Set robot volume
  status = controller.SetVolume(5);
  if (status.code != ErrorCode::OK) {
    std::cerr << "Set volume failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  return 0;
}

int initial_sensor_controller() {
  auto& controller = robot.GetSensorController();
  Status status = controller.OpenChannelSwith();
  if (status.code != ErrorCode::OK) {
    std::cerr << "Open channel failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  controller.SubscribeLeftBinocularHighImg([](const std::shared_ptr<CompressedImage> msg) {
    std::cout << "Received rgbd color image, format: " << msg->format << std::endl;

    try {
      // // 构造保存文件名（可以用时间戳）
      // std::string filename = "rgbd_image_" + std::to_string(msg->header.stamp) + ".jpg";
      // // 保存到本地
      // std::ofstream out(filename, std::ios::binary);
      // if (!out) {
      //   std::cerr << "Failed to open file for writing: " << filename << std::endl;
      //   return;
      // }
      // out.write(reinterpret_cast<const char*>(msg->data.data()), msg->data.size());
      // out.close();

      // 初始化 libcurl
      CURL* curl = curl_easy_init();
      if (!curl) {
        std::cerr << "初始化 libcurl 失败" << std::endl;
        return;
      }

      CURLcode res;
      std::string response;
      struct curl_slist* headers = nullptr;

      // 构建 multipart/form-data 请求体
      curl_mime* mime = curl_mime_init(curl);
      curl_mimepart* part = curl_mime_addpart(mime);
      curl_mime_name(part, "file");
      curl_mime_filename(part, "frame.jpg");
      curl_mime_data(part, reinterpret_cast<const char*>(msg->data.data()), msg->data.size());
      curl_mime_type(part, "image/jpeg");

      // 设置 curl 选项
      curl_easy_setopt(curl, CURLOPT_URL, SERVER_URL.c_str());
      curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

      // 发送请求
      res = curl_easy_perform(curl);

      if (res != CURLE_OK) {
        std::cerr << "请求失败: " << curl_easy_strerror(res) << std::endl;
      } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code == 200) {
          std::cout << "识别结果: " << response << std::endl;
          greetings(response);
        } else {
          std::cerr << "HTTP 错误: " << http_code << "\n返回内容: " << response << std::endl;
        }
      }

      // 释放资源
      curl_mime_free(mime);
      curl_easy_cleanup(curl);
      curl_slist_free_all(headers);

    } catch (const std::exception& e) {
      std::cerr << "Exception in image callback: " << e.what() << std::endl;
    }
  });

  status = controller.OpenBinocularCamera();
  if (status.code != ErrorCode::OK) {
    std::cerr << "Open rgbd camera failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  return 0;
}

int initial_motion_controller() {
  Status status = robot.SetMotionControlLevel(ControllerLevel::HighLevel);
  if (status.code != ErrorCode::OK) {
    std::cerr << "Switch robot motion control level failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  // // Set gait speed ratio
  // status = robot.GetHighLevelMotionController().SetGaitSpeedRatio(GaitMode::GAIT_DOWN_CLIMB_STAIRS, GaitSpeedRatio{0.25, 0.2, 0.4});
  // if (status.code != ErrorCode::OK) {
  //   std::cerr << "Set gait speed ratio failed"
  //             << ", code: " << status.code
  //             << ", message: " << status.message << std::endl;
  //   robot.Shutdown();
  //   return -1;
  // }

  // // Get all gait speed ratio
  // AllGaitSpeedRatio gait_speed_ratios;
  // status = robot.GetHighLevelMotionController().GetAllGaitSpeedRatio(gait_speed_ratios);
  // if (status.code != ErrorCode::OK) {
  //   std::cerr << "Get all gait speed ratio failed"
  //             << ", code: " << status.code
  //             << ", message: " << status.message << std::endl;
  //   robot.Shutdown();
  //   return -1;
  // }

  // left_x_axis_gain.store(gait_speed_ratios.gait_speed_ratios[GaitMode::GAIT_DOWN_CLIMB_STAIRS].lateral_ratio);
  // left_y_axis_gain.store(gait_speed_ratios.gait_speed_ratios[GaitMode::GAIT_DOWN_CLIMB_STAIRS].straight_ratio);
  // right_x_axis_gain.store(gait_speed_ratios.gait_speed_ratios[GaitMode::GAIT_DOWN_CLIMB_STAIRS].turn_ratio);
  // right_y_axis_gain.store(0.0);

  std::cout << "left_x_axis_gain: " << left_x_axis_gain.load() << ", left_y_axis_gain: " << left_y_axis_gain.load() << ", right_x_axis_gain: " << right_x_axis_gain.load() << ", right_y_axis_gain: " << right_y_axis_gain.load() << std::endl;
  return 0;
}

void motion_control() {
  auto change_gait_to_down_climb_stairs = [](auto& robot) -> bool {
    GaitMode current_gait = GaitMode::GAIT_PASSIVE;
    auto status = robot.GetHighLevelMotionController().GetGait(current_gait);
    if (status.code != ErrorCode::OK) {
      std::cerr << "Get robot gait failed"
                << ", code: " << status.code
                << ", message: " << status.message << std::endl;
      return false;
    }
    if (current_gait != target_gait) {
      status = robot.GetHighLevelMotionController().SetGait(target_gait);
      if (status.code != ErrorCode::OK) {
        std::cerr << "Set robot gait failed"
                  << ", code: " << status.code
                  << ", message: " << status.message << std::endl;
        return false;
      }
    } else {
      return true;
    }

    status = robot.GetHighLevelMotionController().GetGait(current_gait);
    if (status.code != ErrorCode::OK) {
      std::cerr << "Get robot gait failed"
                << ", code: " << status.code
                << ", message: " << status.message << std::endl;
      return false;
    }
    while (current_gait != target_gait) {
      usleep(10000);
      status = robot.GetHighLevelMotionController().GetGait(current_gait);
      if (status.code != ErrorCode::OK) {
        std::cerr << "Get robot gait failed"
                  << ", code: " << status.code
                  << ", message: " << status.message << std::endl;
        return false;
      }
    }
    return true;
  };
  // Wait for user input
  while (is_running.load()) {
    int key = getch();
    if (key == 27) {  // ESC
      is_running.store(false);
      break;
    }

    std::cout << "Key ASCII: " << key << ", Character: " << static_cast<char>(key) << std::endl;
    switch (key) {
      case '1': {
        RecoveryStand();
        break;
      }
      case '2': {
        BalanceStand();
        break;
      }
      case '3': {
        UpClimbStairs();
        break;
      }
      case '4': {
        DownClimbStairs();
        break;
      }
      case '5': {
        Walk();
        break;
      }
      case 'g': {
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(TrickAction::ACTION_SHAKE_RIGHT_HAND, "ACTION_SHAKE_RIGHT_HAND");
        break;
      }
      case 'f': {
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(TrickAction::ACTION_FRONT_FLIP, "ACTION_FRONT_FLIP");
        break;
      }
      case 'r': {
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(TrickAction::ACTION_BACK_FLIP, "ACTION_BACK_FLIP");
        break;
      }
      case 'c': {
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(TrickAction::ACTION_SIT_DOWN, "ACTION_SIT_DOWN");
        break;
      }
      case 'z': {
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(TrickAction::ACTION_LIE_DOWN, "ACTION_LIE_DOWN");
        break;
      }
      case 32: {  // space
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(TrickAction::ACTION_HIGH_JUMP, "ACTION_HIGH_JUMP");
        break;
      }
      case 'w': {
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(0.0, 1.0, 0.0, 0.0);  // Forward
        break;
      }
      case 'a': {
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(-1.0, 0.0, 0.0, 0.0);  // Left
        break;
      }
      case 's': {
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(0.0, -1.0, 0.0, 0.0);  // Backward
        break;
      }
      case 'd': {
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(1.0, 0.0, 0.0, 0.0);  // Right
        break;
      }
      case 'q': {
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(0.0, 0.0, -1.0, 0.0);  // Turn left
        break;
      }
      case 'e': {
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(0.0, 0.0, 1.0, 0.0);  // Turn right
        break;
      }
      case 'x': {
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);  // Stop
        break;
      }
      default:
        std::cout << "Unknown key: " << key << std::endl;
        break;
    }

    usleep(10000);
  }
}

int close_audio_controller() {
  auto& controller = robot.GetAudioController();
  Status status = controller.Stop();
  if (status.code != ErrorCode::OK) {
    std::cerr << "Stop TTS failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  return 0;
}

int close_sensor_controller() {
  // Close binocular camera
  Status status = robot.GetSensorController().CloseBinocularCamera();
  if (status.code != ErrorCode::OK) {
    std::cerr << "Close binocular camera failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
  }

  status = robot.GetSensorController().CloseChannelSwith();
  if (status.code != ErrorCode::OK) {
    std::cerr << "Close channel failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  return 0;
}

int main(int argc, char* argv[]) {
  // Bind SIGINT（Ctrl+C）
  signal(SIGINT, signalHandler);

  // initial robot
  if (const int return_code = initial_robot()) {
    return return_code;
  }

  // initial audio controller
  // if (const int return_code = initial_audio_controller()) {
  //   return return_code;
  // }
  //
  // // initial sensor controller
  // if (const int return_code = initial_sensor_controller()) {
  //   return return_code;
  // }

  // initial motion controller
  if (const int return_code = initial_motion_controller()) {
    return return_code;
  }

  // start motion cmd sending thread
  std::thread send_motion_cmd_thread(send_motion_cmd);

  // start motion control thread
  std::thread motion_control_thread(motion_control);

  print_help(argv[0]);

  std::cout << "Press any key to continue (ESC to exit)..." << std::endl;

  // wait for thread to close
  send_motion_cmd_thread.join();
  motion_control_thread.join();

  // close sensor controller
  if (const int return_code = close_sensor_controller()) {
    return return_code;
  }

  // close sensor controller
  if (const int return_code = close_audio_controller()) {
    return return_code;
  }

  // Disconnect from robot
  const Status status = robot.Disconnect();
  if (status.code != ErrorCode::OK) {
    std::cerr << "Disconnect robot failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  robot.Shutdown();

  return 0;
}
