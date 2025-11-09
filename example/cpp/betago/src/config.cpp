#include "config.h"

// 全局变量定义
magic::dog::MagicRobot robot;
std::atomic<bool> is_running(true);
magic::dog::GaitMode target_gait = magic::dog::GaitMode::GAIT_DOWN_CLIMB_STAIRS;

// 摇杆控制变量
std::atomic<float> left_x_axis(0.0);
std::atomic<float> left_y_axis(0.0);
std::atomic<float> right_x_axis(0.0);
std::atomic<float> right_y_axis(0.0);
std::atomic<float> left_x_axis_gain(0.0);
std::atomic<float> left_y_axis_gain(0.0);
std::atomic<float> right_x_axis_gain(0.0);
std::atomic<float> right_y_axis_gain(0.0);

// 服务器配置
const std::string IMAGE_SERVER_URL = "http://120.92.77.233:3999/face/frame";
const std::string VOICE_SERVER_URL = "http://120.92.77.233:3999/speech/once";
const int SAME_PERSON_DELAY_MS = 10000;
const int REQUEST_COOLDOWN_MS = 2000;

// 贝塔狗成员数据库
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

const std::vector<std::string> DEFAULT_GREETING_TEMPLATES = {
    "你好呀！%s，今天过得怎么样？",
    "嗨！%s，希望你今天心情愉快！",
    "Hello! %s， 愿你今天充满能量！",
    "Hi！%s，认识你很高兴！"};

// 状态变量
GreetingState g_state;
VoiceState v_state;