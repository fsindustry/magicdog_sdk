#include "nav_control.h"

#include <iostream>

std::string current_slam_mode = "IDLE";
magic::dog::NavMode current_nav_mode = magic::dog::NavMode::IDLE;

int initial_slam_controller() {
  // Get SLAM navigation controller
  auto& slamNavController = robot.GetSlamNavController();
  if (!slamNavController.Initialize()) {
    std::cerr << "Failed to initialize SLAM navigation controller" << std::endl;
    robot.Disconnect();
    robot.Shutdown();
    return -1;
  }

  std::cout << "Successfully initialized SLAM navigation controller" << std::endl;
  return 0;
}

// 加载地图
int loadMap(const std::string& map_name) {
  try {
    // Get SLAM navigation controller
    auto& controller = robot.GetSlamNavController();

    // Switch to localization mode
    auto status = controller.LoadMap(map_name);
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to load map: " << map_name << ", code: " << status.code
                << ", message: " << status.message << std::endl;
      return -1;
    }

    std::cout << "Successfully load map: " << map_name << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred while load map: " << map_name << ", error: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}

// 切换到定位模式
int switchToLocalizationMode() {
  try {
    // Get SLAM navigation controller
    auto& controller = robot.GetSlamNavController();

    // Switch to localization mode
    auto status = controller.SwitchToLocation();
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to switch to localization mode, code: " << status.code
                << ", message: " << status.message << std::endl;
      return -1;
    }

    current_slam_mode = "LOCALIZATION";
    std::cout << "Successfully switched to localization mode" << std::endl;
    std::cout << "Robot is now in localization mode, ready to localize on existing maps" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred while switching to localization mode: " << e.what() << std::endl;
    return -1;
  }
  return 0;
}

// 初始化位姿
int initializePose(double x, double y, double yaw) {
  try {
    // Get SLAM navigation controller
    auto& controller = robot.GetSlamNavController();

    // Create initial pose (set to origin)
    magic::dog::Pose3DEuler initialPose;
    initialPose.position = {x, y, 0.0};         // x, y, z
    initialPose.orientation = {0.0, 0.0, yaw};  // roll, pitch, yaw

    std::cout << "Initializing robot pose to origin..." << std::endl;

    // Initialize pose
    auto status = controller.InitPose(initialPose);
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to initialize pose, code: " << status.code
                << ", message: " << status.message << std::endl;
      return -1;
    }

    std::cout << "Successfully initialized pose" << std::endl;
    std::cout << "Robot pose has been set to origin (" << x << ", " << y << ", " << yaw << ")" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred while initializing pose: " << e.what() << std::endl;
    return -1;
  }
  return 0;
}

// 获取定位状态
int getCurrentLocalizationInfo(magic::dog::LocalizationInfo& poseInfo) {
  try {
    // Get SLAM navigation controller
    auto& controller = robot.GetSlamNavController();

    // Get current pose information
    auto status = controller.GetCurrentLocalizationInfo(poseInfo);
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to get current pose information, code: " << status.code
                << ", message: " << status.message << std::endl;
      return -1;
    }

    std::cout << "Successfully retrieved current pose information" << std::endl;
    std::cout << "Localization status: " << (poseInfo.is_localization ? "Localized" : "Not localized") << std::endl;
    std::cout << "Position: [" << poseInfo.pose.position[0] << ", "
              << poseInfo.pose.position[1] << ", " << poseInfo.pose.position[2] << "]" << std::endl;
    std::cout << "Orientation: [" << poseInfo.pose.orientation[0] << ", "
              << poseInfo.pose.orientation[1] << ", " << poseInfo.pose.orientation[2] << "]" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred while getting current pose information: " << e.what() << std::endl;
    return -1;
  }
  return 0;
}

// 激活导航模式
int switchToNavigationMode() {
  try {
    // Get SLAM navigation controller
    auto& controller = robot.GetSlamNavController();

    // Switch to navigation mode
    auto status = controller.ActivateNavMode(magic::dog::NavMode::GRID_MAP);
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to switch to navigation mode, code: " << status.code
                << ", message: " << status.message << std::endl;
      return -1;
    }

    current_nav_mode = magic::dog::NavMode::GRID_MAP;
    std::cout << "Successfully switched to navigation mode" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred while switching to navigation mode: " << e.what() << std::endl;
    return -1;
  }
  return 0;
}

// 设置目标位姿(开始导航任务)
int setNavigationTarget(double x, double y, double yaw) {
  try {
    // Get SLAM navigation controller
    auto& controller = robot.GetSlamNavController();
    auto& highController = robot.GetHighLevelMotionController();

    // Disable joy stick
    highController.DisableJoyStick();
    std::cout << "Successfully disabled joy stick" << std::endl;

    // change gait to slow
    // To use Navigation, you must switch to the down stairs gait
    auto status = highController.SetGait(magic::dog::GaitMode::GAIT_DOWN_CLIMB_STAIRS);
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to set gait to slow: " << status.message << std::endl;
      return -1;
    }
    std::cout << "Successfully set gait to slow" << std::endl;

    // Create target goal
    magic::dog::NavTarget targetGoal;
    targetGoal.id = 1;
    targetGoal.frame_id = "map";

    // 设置导航坐标点和朝向
    targetGoal.goal.position = {x, y, 0.0};
    targetGoal.goal.orientation = {0.0, 0.0, yaw};

    // 设置目标坐标
    status = controller.SetNavTarget(targetGoal);
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to set navigation target, code: " << status.code
                << ", message: " << status.message << std::endl;
      return -1;
    }

    std::cout << "Successfully set navigation target: position=("
              << targetGoal.goal.position[0] << ", " << targetGoal.goal.position[1] << ", "
              << targetGoal.goal.position[2] << "), orientation=("
              << targetGoal.goal.orientation[0] << ", " << targetGoal.goal.orientation[1] << ", "
              << targetGoal.goal.orientation[2] << ")" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred while setting navigation target: " << e.what() << std::endl;
    return -1;
  }
  return 0;
}

// 暂停导航任务
int pauseNavigation() {
  try {
    // Get SLAM navigation controller
    auto& controller = robot.GetSlamNavController();

    // Pause navigation
    auto status = controller.PauseNavTask();
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to pause navigation, code: " << status.code
                << ", message: " << status.message << std::endl;
      return -1;
    }

    std::cout << "Successfully paused navigation" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred while pausing navigation: " << e.what() << std::endl;
    return -1;
  }
  return 0;
}

// 继续导航任务
int resumeNavigation() {
  try {
    // Get SLAM navigation controller
    auto& controller = robot.GetSlamNavController();

    // Resume navigation
    auto status = controller.ResumeNavTask();
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to resume navigation, code: " << status.code
                << ", message: " << status.message << std::endl;
      return -1;
    }

    std::cout << "Successfully resumed navigation" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred while resuming navigation: " << e.what() << std::endl;
    return -1;
  }
  return 0;
}

// 取消导航任务
int cancelNavigation() {
  try {
    // Get SLAM navigation controller
    auto& controller = robot.GetSlamNavController();

    // Cancel navigation
    auto status = controller.CancelNavTask();
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to cancel navigation, code: " << status.code
                << ", message: " << status.message << std::endl;
      return -1;
    }

    std::cout << "Successfully cancelled navigation" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred while cancelling navigation: " << e.what() << std::endl;
    return -1;
  }
  return 0;
}

// 获取导航状态
int getNavigationStatus(magic::dog::NavStatus& navStatus) {
  try {
    // Get SLAM navigation controller
    auto& controller = robot.GetSlamNavController();

    // Get navigation status
    magic::dog::NavStatus navStatus;
    auto status = controller.GetNavTaskStatus(navStatus);
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to get navigation status, code: " << status.code
                << ", message: " << status.message << std::endl;
      return -1;
    }

    // Display navigation status information
    std::cout << "=== Navigation Status ===" << std::endl;
    std::cout << "Target ID: " << navStatus.id << std::endl;
    std::cout << "Status: " << static_cast<int>(navStatus.status) << std::endl;
    std::cout << "Message: " << navStatus.message << std::endl;

    // Provide status interpretation
    std::string statusMeaning;
    switch (navStatus.status) {
      case magic::dog::NavStatusType::NONE:
        statusMeaning = "No navigation target set";
        break;
      case magic::dog::NavStatusType::RUNNING:
        statusMeaning = "Navigation is running";
        break;
      case magic::dog::NavStatusType::END_SUCCESS:
        statusMeaning = "Navigation completed successfully";
        break;
      case magic::dog::NavStatusType::END_FAILED:
        statusMeaning = "Navigation failed";
        break;
      case magic::dog::NavStatusType::PAUSE:
        statusMeaning = "Navigation is paused";
        break;
      default:
        statusMeaning = "Unknown status value";
        break;
    }

    std::cout << "Status meaning: " << statusMeaning << std::endl;
    std::cout << "========================" << std::endl;

  } catch (const std::exception& e) {
    std::cerr << "Exception occurred while getting navigation status: " << e.what() << std::endl;
    return -1;
  }
  return 0;
}

int closeNavigation() {
  try {
    // Get SLAM navigation controller
    auto& controller = robot.GetSlamNavController();

    // Switch to idle mode to close navigation
    auto status = controller.ActivateNavMode(magic::dog::NavMode::IDLE);
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to close navigation, code: " << status.code
                << ", message: " << status.message << std::endl;
      return -1;
    }

    current_nav_mode = magic::dog::NavMode::IDLE;
    std::cout << "Successfully closed navigation system" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred while closing navigation: " << e.what() << std::endl;
    return -1;
  }
  return 0;
}

int close_slam_controller() {
  try {
    // Get SLAM navigation controller
    auto& controller = robot.GetSlamNavController();

    // Switch to idle mode to close SLAM
    auto status = controller.SwitchToIdle();
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Failed to close SLAM, code: " << status.code
                << ", message: " << status.message << std::endl;
      return -1;
    }

    current_slam_mode = "IDLE";
    std::cout << "Successfully closed SLAM system" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred while closing SLAM: " << e.what() << std::endl;
    return -1;
  }
  return 0;
}

int nav_to_target() {
  // 加载地图 → 切换到定位模式 → 初始化位姿 → 获取定位状态
  if (loadMap("testmap")) {
    return -1;
  }

  if (switchToLocalizationMode()) {
    return -1;
  }

  if (initializePose(0.0, 0.0, 0.0)) {
    return -1;
  }

  magic::dog::LocalizationInfo poseInfo;
  if (getCurrentLocalizationInfo(poseInfo)) {
    return -1;
  }

  if (!poseInfo.is_localization) {
    return -1;
  }

  // 定位成功 → 激活导航模式 → 设置目标位姿(开始导航任务) → 获取导航状态
  if (switchToNavigationMode()) {
    return -1;
  }

  // 开始导航
  if (setNavigationTarget(0.0, 10.0, 0.0)) {
    return -1;
  }

  return 0;
}

int back_from_target() {
  // 加载地图 → 切换到定位模式 → 初始化位姿 → 获取定位状态
  if (loadMap("testmap")) {
    return -1;
  }

  if (switchToLocalizationMode()) {
    return -1;
  }

  if (initializePose(0.0, 10.0, 0.0)) {
    return -1;
  }

  magic::dog::LocalizationInfo poseInfo;
  if (getCurrentLocalizationInfo(poseInfo)) {
    return -1;
  }

  if (!poseInfo.is_localization) {
    return -1;
  }

  // 定位成功 → 激活导航模式 → 设置目标位姿(开始导航任务) → 获取导航状态
  if (switchToNavigationMode()) {
    return -1;
  }

  // 开始导航
  if (setNavigationTarget(0.0, 10.0, 3.14)) {
    return -1;
  }

  return 0;
}