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
  std::cout << "  g        Execute trick - shake right hand\n";
  std::cout << "  f        Execute trick - front flip\n";
  std::cout << "  r        Execute trick - back flip\n";
  std::cout << "  c        Execute trick - sit down\n";
  std::cout << "  z        Execute trick - lie down\n";
  std::cout << "  space    Execute trick - jump\n";
  std::cout << "  h        Execute trick - dance\n";
  std::cout << "  w        Move forward\n";
  std::cout << "  a        Move left\n";
  std::cout << "  s        Move backward\n";
  std::cout << "  d        Move right\n";
  std::cout << "  q        Turn left\n";
  std::cout << "  e        Turn right\n";
  std::cout << "  x        Stop movement\n";
  std::cout << "  W        Jump forward\n";
  std::cout << "  A        Jump left\n";
  std::cout << "  S        Stretch\n";
  std::cout << "  D        Jump right\n";
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

void Dancing() {
  // 暂停图像识别
  auto& sensorController = robot.GetSensorController();
  sensorController.CloseBinocularCamera();

  auto& audioController = robot.GetAudioController();
  // Play voice
  TtsCommand tts;
  tts.id = "100000000101";
  tts.content = "我给大家跳个舞吧!";
  tts.priority = TtsPriority::HIGH;
  tts.mode = TtsMode::CLEARBUFFER;
  Status status = audioController.Play(tts);

  usleep(3000 * 1000);

  // // 启动音乐播放线程
  // std::atomic<bool> stop_music{false};
  // std::thread music_thread([&audioController, &stop_music]() {
  //   std::cout << "Play music in background thread" << std::endl;
  //
  //   std::string music_file_path =
  //       "local_music:/opt/eame/dreame_manager/share/dreame_manager/configures/music_files/dance02.mp3";
  //
  //   TtsCommand music_tts;
  //   music_tts.id = "100000000200";
  //   music_tts.content = music_file_path;
  //   music_tts.priority = TtsPriority::HIGH;
  //   music_tts.mode = TtsMode::CLEARBUFFER;
  //
  //   Status status = audioController.Play(music_tts);
  //   if (status.code != ErrorCode::OK) {
  //     std::cerr << "Music play failed!" << std::endl;
  //     return;
  //   }
  //
  //   // 模拟播放检测（防止线程空转）
  //   while (!stop_music.load()) {
  //     usleep(100000);  // 每100ms检测一次是否要退出
  //   }
  //
  //   audioController.Stop();
  //   std::cout << "Music thread exiting..." << std::endl;
  // });

  // start dancing
  auto& controller = robot.GetHighLevelMotionController();

  // 记录开始时间
  auto start_time = std::chrono::steady_clock::now();

  status = controller.ExecuteTrick(TrickAction::ACTION_DANCE);
  usleep(30 * 1000 * 1000);

  // 记录结束时间
  auto end_time = std::chrono::steady_clock::now();

  // 计算耗时（毫秒）
  auto duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

  // 打印耗时
  std::cout << "[Dancing] 动作总耗时: " << duration_ms << " ms" << std::endl;

  // // 通知音乐线程结束并等待退出
  // stop_music.store(true);
  // if (music_thread.joinable()) {
  //   music_thread.join();
  // }

  tts.id = "100000000102";
  tts.content = "谢谢!";
  tts.priority = TtsPriority::HIGH;
  tts.mode = TtsMode::CLEARBUFFER;
  status = audioController.Play(tts);

  sensorController.OpenBinocularCamera();
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
        DownClimbStairs();
        break;
      }
      case '4': {
        UpClimbStairs();
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
      case 'h': {
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        Dancing();
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
      case 'W': {
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(TrickAction::ACTION_JUMP_FRONT, "ACTION_JUMP_FRONT");
        break;
      }
      case 'A': {
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(TrickAction::ACTION_SPIN_JUMP_LEFT, "ACTION_SPIN_JUMP_LEFT");
        break;
      }
      case 'S': {
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(TrickAction::ACTION_STRETCH, "ACTION_STRETCH");
        break;
      }
      case 'D': {
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(TrickAction::ACTION_SPIN_JUMP_RIGHT, "ACTION_SPIN_JUMP_RIGHT");
        break;
      }
      default:
        std::cout << "Unknown key: " << key << std::endl;
        break;
    }

    usleep(10000);
  }
}

// 用于接收 HTTP 响应
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
  size_t total_size = size * nmemb;
  output->append(static_cast<char*>(contents), total_size);
  return total_size;
}

std::string get_face_name(std::string& response) {
  nlohmann::json j = nlohmann::json::parse(response);

  if (j.contains("data")) {
    auto data = j["data"];
    std::string data_status = data.value("status", "");
    if (data_status == "success") {
      return data.value("name", "");
    }
  } else {
    std::cerr << "JSON does not contain 'data'" << std::endl;
  }
  return "";
}

void greetings(const std::string& name) {
  try {
    auto it = BETAGO_MEMBERS.find(name);
    if (it != BETAGO_MEMBERS.end()) {
      const member& m = it->second;
      auto& controller = robot.GetAudioController();

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
  } catch (const std::exception& e) {
    std::cerr << "Exception in greetings: " << e.what() << std::endl;
  }
}

static bool upload_image(const std::shared_ptr<CompressedImage>& msg, std::string& response) {
  thread_local CURL* curl = nullptr;  // 每个线程一个 curl 对象
  if (!curl) {
    curl = curl_easy_init();
    if (!curl) {
      std::cerr << "初始化 libcurl 失败" << std::endl;
      return false;
    }
    curl_easy_setopt(curl, CURLOPT_URL, SERVER_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
  }

  curl_mime* mime = curl_mime_init(curl);
  curl_mimepart* part = curl_mime_addpart(mime);
  curl_mime_name(part, "file");
  curl_mime_filename(part, "frame.jpg");
  curl_mime_data(part, reinterpret_cast<const char*>(msg->data.data()), msg->data.size());
  curl_mime_type(part, "image/jpeg");

  response.clear();
  curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(curl);
  curl_mime_free(mime);

  if (res != CURLE_OK) {
    std::cerr << "请求失败: " << curl_easy_strerror(res) << std::endl;
    return false;
  }

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  if (http_code != 200) {
    std::cerr << "HTTP 错误: " << http_code << "\n返回内容: " << response << std::endl;
    return false;
  }

  return true;
}

static const int SAME_PERSON_DELAY_MS = 10000;  // 10 秒
static const int REQUEST_COOLDOWN_MS = 2000;    // 全局最小请求间隔

struct GreetingState {
  std::string last_name;
  std::chrono::steady_clock::time_point last_greeted_time;
  std::chrono::steady_clock::time_point last_request_time;
  std::mutex mutex;
};

static GreetingState g_state;

auto receive_img() -> void (*)(std::shared_ptr<CompressedImage>) {
  return [](const std::shared_ptr<CompressedImage> msg) {
    auto now = std::chrono::steady_clock::now();

    // 限制全局请求频率
    {
      std::lock_guard<std::mutex> lock(g_state.mutex);
      if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_state.last_request_time).count() < REQUEST_COOLDOWN_MS)
        return;
      g_state.last_request_time = now;
    }

    std::string response;
    if (!upload_image(msg, response))
      return;

    // 假设 get_face_name() 从 response 提取识别结果
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

    std::cout << "识别结果: " << name << std::endl;
    greetings(name);
  };
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
  status = controller.SetVolume(2);
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

  controller.SubscribeLeftBinocularHighImg(receive_img());

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

  std::cout << "left_x_axis_gain: " << left_x_axis_gain.load() << ", left_y_axis_gain: " << left_y_axis_gain.load() << ", right_x_axis_gain: " << right_x_axis_gain.load() << ", right_y_axis_gain: " << right_y_axis_gain.load() << std::endl;
  return 0;
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
  if (const int return_code = initial_audio_controller()) {
    return return_code;
  }

  // initial sensor controller
  if (const int return_code = initial_sensor_controller()) {
    return return_code;
  }

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
