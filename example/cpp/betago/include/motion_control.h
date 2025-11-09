#pragma once

#include "config.h"

// 函数声明
void signalHandler(int signum);
void print_help(const char* prog_name);
int getch();

// 姿态控制
void RecoveryStand();
void BalanceStand();
void UpClimbStairs();
void DownClimbStairs();

// 特技动作
void ExecuteTrickAction(magic::dog::TrickAction action, const std::string& action_name);
void Dancing();

// 摇杆控制
void JoyStickCommand(float left_x_axis, float left_y_axis, float right_x_axis, float right_y_axis);

// 线程函数
void send_motion_cmd();
void motion_control();