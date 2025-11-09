#include "sensor_control.h"
#include <iostream>

int initial_sensor_controller() {
  auto& controller = robot.GetSensorController();
  auto status = controller.OpenChannelSwith();
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Open channel failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  // 注意：这里需要包含face_recognition.h来获取receive_img函数
  // controller.SubscribeLeftBinocularHighImg(receive_img());

  status = controller.OpenBinocularCamera();
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Open rgbd camera failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  return 0;
}

int close_sensor_controller() {
  auto status = robot.GetSensorController().CloseBinocularCamera();
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Close binocular camera failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
  }

  status = robot.GetSensorController().CloseChannelSwith();
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Close channel failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  return 0;
}