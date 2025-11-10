#pragma once

#include "config.h"

// 初始化slam controller
int initial_slam_controller();
// 切换到定位模式
int switchToLocalizationMode();
// 初始化位姿
int initializePose(double x, double y, double yaw);
// 获取定位状态
int getCurrentLocalizationInfo(magic::dog::LocalizationInfo& poseInfo);
// 激活导航模式
int switchToNavigationMode();
// 设置目标位姿(开始导航任务)
int setNavigationTarget(double x, double y, double yaw);
// 暂停导航任务
int pauseNavigation();
// 继续导航任务
int resumeNavigation();
// 取消导航任务
int cancelNavigation();
// 获取导航状态
int getNavigationStatus(magic::dog::NavStatus& navStatus);
// 关闭导航状态
int closeNavigation();
// 关闭slam controller
int close_slam_controller();
// 导航到指定位置
int nav_to_target();
// 从指定位置返回
int back_from_target();