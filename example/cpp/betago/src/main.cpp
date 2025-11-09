#include <csignal>
#include <iostream>
#include <thread>
#include "audio_control.h"
#include "config.h"
#include "motion_control.h"
#include "sensor_control.h"

int initial_robot() {
  std::string local_ip = "192.168.54.10";
  if (!robot.Initialize(local_ip)) {
    std::cerr << "Robot SDK initialization failed." << std::endl;
    robot.Shutdown();
    return -1;
  }

  auto status = robot.Connect();
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Connect robot failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }
  return 0;
}

int initial_motion_controller() {
  auto status = robot.SetMotionControlLevel(magic::dog::ControllerLevel::HighLevel);
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Switch robot motion control level failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  std::cout << "left_x_axis_gain: " << left_x_axis_gain.load()
            << ", left_y_axis_gain: " << left_y_axis_gain.load()
            << ", right_x_axis_gain: " << right_x_axis_gain.load()
            << ", right_y_axis_gain: " << right_y_axis_gain.load() << std::endl;
  return 0;
}

int main(int argc, char* argv[]) {
  signal(SIGINT, signalHandler);

  if (const int return_code = initial_robot()) {
    return return_code;
  }

  if (const int return_code = initial_audio_controller()) {
    return return_code;
  }

  if (const int return_code = initial_sensor_controller()) {
    return return_code;
  }

  if (const int return_code = initial_motion_controller()) {
    return return_code;
  }

  std::thread send_motion_cmd_thread(send_motion_cmd);
  std::thread motion_control_thread(motion_control);

  print_help(argv[0]);
  std::cout << "Press any key to continue (ESC to exit)..." << std::endl;

  send_motion_cmd_thread.join();
  motion_control_thread.join();

  if (const int return_code = close_sensor_controller()) {
    return return_code;
  }

  if (const int return_code = close_audio_controller()) {
    return return_code;
  }

  auto status = robot.Disconnect();
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Disconnect robot failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  robot.Shutdown();
  return 0;
}