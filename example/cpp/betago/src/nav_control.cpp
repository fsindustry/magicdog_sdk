#include "nav_control.h"

int initial_nav_controller() {

  // 加载地图 → 切换到定位模式 → 初始化位姿 → 获取定位状态

  // 定位成功 → 激活导航模式 → 设置目标位姿(开始导航任务) → 获取导航状态

  return 0;
}

int close_nav_controller() {
  return 0;
}

void nav2target() {
  // 设置目标姿态(开始导航任务) → 暂停导航 → 恢复导航 → 取消导航

}