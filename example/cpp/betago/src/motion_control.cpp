#include "motion_control.h"
#include <termios.h>
#include <unistd.h>
#include <csignal>
#include <iostream>
#include <thread>

void signalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received.\n";
  is_running.store(false);
  robot.Shutdown();
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
  termios oldt, newt;
  int ch;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}

void RecoveryStand() {
  auto& controller = robot.GetHighLevelMotionController();
  auto status = controller.SetGait(magic::dog::GaitMode::GAIT_STAND_R);
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Set robot gait failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
  }
}

void BalanceStand() {
  auto& controller = robot.GetHighLevelMotionController();
  auto status = controller.SetGait(magic::dog::GaitMode::GAIT_STAND_B);
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Set robot gait failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    return;
  }
  std::cout << "Robot gait set to GAIT_BALANCE_STAND successfully." << std::endl;
}

void UpClimbStairs() {
  auto& controller = robot.GetHighLevelMotionController();
  target_gait = magic::dog::GaitMode::GAIT_UP_CLIMB_STAIRS;
  auto status = controller.SetGait(magic::dog::GaitMode::GAIT_UP_CLIMB_STAIRS);
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Set robot gait failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    return;
  }
  std::cout << "Robot gait set to GAIT_UP_CLIMB_STAIRS successfully." << std::endl;
}

void DownClimbStairs() {
  auto& controller = robot.GetHighLevelMotionController();
  target_gait = magic::dog::GaitMode::GAIT_DOWN_CLIMB_STAIRS;
  auto status = controller.SetGait(magic::dog::GaitMode::GAIT_DOWN_CLIMB_STAIRS);
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Set robot gait failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    return;
  }
  std::cout << "Robot gait set to GAIT_DOWN_CLIMB_STAIRS successfully." << std::endl;
}

void ExecuteTrickAction(const magic::dog::TrickAction action, const std::string& action_name) {
  auto& controller = robot.GetHighLevelMotionController();
  JoyStickCommand(0.0, 0.0, 0.0, 0.0);

  auto status = controller.ExecuteTrick(action);
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Execute robot trick failed: " << action_name
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    return;
  }
  std::cout << "Robot " << action_name << " executed successfully." << std::endl;
}

void Dancing() {
  auto& sensorController = robot.GetSensorController();
  sensorController.CloseBinocularCamera();

  auto& audioController = robot.GetAudioController();

  magic::dog::TtsCommand tts;
  tts.id = "100000000101";
  tts.content = "我给大家跳个舞吧!";
  tts.priority = magic::dog::TtsPriority::HIGH;
  tts.mode = magic::dog::TtsMode::CLEARBUFFER;
  auto status = audioController.Play(tts);

  usleep(3000 * 1000);

  auto start_time = std::chrono::steady_clock::now();
  auto& controller = robot.GetHighLevelMotionController();
  status = controller.ExecuteTrick(magic::dog::TrickAction::ACTION_DANCE);
  usleep(45 * 1000 * 1000);

  auto end_time = std::chrono::steady_clock::now();
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

  std::cout << "[Dancing] 动作总耗时: " << duration_ms << " ms" << std::endl;

  tts.id = "100000000102";
  tts.content = "谢谢!";
  tts.priority = magic::dog::TtsPriority::HIGH;
  tts.mode = magic::dog::TtsMode::CLEARBUFFER;
  status = audioController.Play(tts);

  sensorController.OpenBinocularCamera();
}

void JoyStickCommand(float left_x_axis, float left_y_axis, float right_x_axis, float right_y_axis) {
  ::left_x_axis.store(left_x_axis);
  ::left_y_axis.store(left_y_axis);
  ::right_x_axis.store(right_x_axis);
  ::right_y_axis.store(right_y_axis);
}

void send_motion_cmd() {
  auto& controller = robot.GetHighLevelMotionController();
  while (is_running.load()) {
    magic::dog::JoystickCommand joy_command;
    joy_command.left_x_axis = left_x_axis;
    joy_command.left_y_axis = left_y_axis;
    joy_command.right_x_axis = right_x_axis;
    joy_command.right_y_axis = right_y_axis;

    auto status = controller.SendJoyStickCommand(joy_command);
    if (status.code != magic::dog::ErrorCode::OK) {
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
      std::cout << "left_x_v: " << left_x_axis_v_current << ", left_y_v: " << left_y_axis_v_current
                << ", right_x_v: " << right_x_axis_v_current << ", right_y_v: " << right_y_axis_v_current << std::endl;
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
    magic::dog::GaitMode current_gait = magic::dog::GaitMode::GAIT_PASSIVE;
    auto status = robot.GetHighLevelMotionController().GetGait(current_gait);
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Get robot gait failed"
                << ", code: " << status.code
                << ", message: " << status.message << std::endl;
      return false;
    }
    if (current_gait != target_gait) {
      status = robot.GetHighLevelMotionController().SetGait(target_gait);
      if (status.code != magic::dog::ErrorCode::OK) {
        std::cerr << "Set robot gait failed"
                  << ", code: " << status.code
                  << ", message: " << status.message << std::endl;
        return false;
      }
    } else {
      return true;
    }

    status = robot.GetHighLevelMotionController().GetGait(current_gait);
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Get robot gait failed"
                << ", code: " << status.code
                << ", message: " << status.message << std::endl;
      return false;
    }
    while (current_gait != target_gait) {
      usleep(10000);
      status = robot.GetHighLevelMotionController().GetGait(current_gait);
      if (status.code != magic::dog::ErrorCode::OK) {
        std::cerr << "Get robot gait failed"
                  << ", code: " << status.code
                  << ", message: " << status.message << std::endl;
        return false;
      }
    }
    return true;
  };

  while (is_running.load()) {
    int key = getch();
    if (key == 27) {
      is_running.store(false);
      break;
    }

    switch (key) {
      case '1':
        RecoveryStand();
        break;
      case '2':
        BalanceStand();
        break;
      case '3':
        DownClimbStairs();
        break;
      case '4':
        UpClimbStairs();
        break;
      case 'g':
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(magic::dog::TrickAction::ACTION_SHAKE_RIGHT_HAND, "ACTION_SHAKE_RIGHT_HAND");
        break;
      case 'f':
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(magic::dog::TrickAction::ACTION_FRONT_FLIP, "ACTION_FRONT_FLIP");
        break;
      case 'r':
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(magic::dog::TrickAction::ACTION_BACK_FLIP, "ACTION_BACK_FLIP");
        break;
      case 'c':
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(magic::dog::TrickAction::ACTION_SIT_DOWN, "ACTION_SIT_DOWN");
        break;
      case 'z':
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(magic::dog::TrickAction::ACTION_LIE_DOWN, "ACTION_LIE_DOWN");
        break;
      case 32:
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(magic::dog::TrickAction::ACTION_HIGH_JUMP, "ACTION_HIGH_JUMP");
        break;
      case 'h':
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        Dancing();
        break;
      case 'w':
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(0.0, 1.0, 0.0, 0.0);
        break;
      case 'a':
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(-1.0, 0.0, 0.0, 0.0);
        break;
      case 's':
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(0.0, -1.0, 0.0, 0.0);
        break;
      case 'd':
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(1.0, 0.0, 0.0, 0.0);
        break;
      case 'q':
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(0.0, 0.0, -1.0, 0.0);
        break;
      case 'e':
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(0.0, 0.0, 1.0, 0.0);
        break;
      case 'x':
        if (!change_gait_to_down_climb_stairs(robot)) {
          std::cerr << "Change robot gait to down climb stairs failed" << std::endl;
          break;
        }
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        break;
      case 'W':
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(magic::dog::TrickAction::ACTION_JUMP_FRONT, "ACTION_JUMP_FRONT");
        break;
      case 'A':
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(magic::dog::TrickAction::ACTION_SPIN_JUMP_LEFT, "ACTION_SPIN_JUMP_LEFT");
        break;
      case 'S':
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(magic::dog::TrickAction::ACTION_STRETCH, "ACTION_STRETCH");
        break;
      case 'D':
        JoyStickCommand(0.0, 0.0, 0.0, 0.0);
        ExecuteTrickAction(magic::dog::TrickAction::ACTION_SPIN_JUMP_RIGHT, "ACTION_SPIN_JUMP_RIGHT");
        break;
      default:
        std::cout << "Unknown key: " << key << std::endl;
        break;
    }
    usleep(10000);
  }
}