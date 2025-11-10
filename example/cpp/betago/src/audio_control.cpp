#include "audio_control.h"
#include <cstdio>
#include <iostream>
#include <random>

#include "voice_recognition.h"

int initial_audio_controller() {
  auto& controller = robot.GetAudioController();
  int get_volume = 0;
  auto status = controller.GetVolume(get_volume);
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Get volume failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  std::cout << "Get volume success, volume: " << std::to_string(get_volume) << std::endl;

  status = controller.SetVolume(2);
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Set volume failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  magic::dog::GetSpeechConfig voice_config;
  status = controller.GetVoiceConfig(voice_config);
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Get voice config failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  std::cout << "Get voice config success" << std::endl;
  std::cout << "TTS type: " << static_cast<int>(voice_config.tts_type) << std::endl;
  std::cout << "Region: " << voice_config.speaker_config.selected.region << std::endl;
  std::cout << "Speaker: " << voice_config.speaker_config.selected.speaker_id << std::endl;
  std::cout << "Bot config: " << voice_config.bot_config.selected.bot_id << std::endl;
  std::cout << "Wake word: " << voice_config.wakeup_config.name << std::endl;
  std::cout << "Dialog config - Front DOA: " << voice_config.dialog_config.is_front_doa << std::endl;
  std::cout << "Dialog config - Full duplex: " << voice_config.dialog_config.is_fullduplex_enable << std::endl;
  std::cout << "Dialog config - Voice enable: " << voice_config.dialog_config.is_enable << std::endl;
  std::cout << "Dialog config - DOA enable: " << voice_config.dialog_config.is_doa_enable << std::endl;
  std::cout << "Speaker speed: " << voice_config.speaker_config.speaker_speed << std::endl;
  std::cout << "Custom bot size: " << voice_config.bot_config.custom_data.size() << std::endl;

  for (const auto& [key, value] : voice_config.bot_config.custom_data) {
    std::cout << "Custom bot data: " << key << ", " << value.name << std::endl;
  }

  magic::dog::SetSpeechConfig config;
  // config.speaker_id = voice_config.speaker_config.selected.speaker_id;
  // config.region = voice_config.speaker_config.selected.region;
  // config.bot_id = voice_config.bot_config.selected.bot_id;
  config.is_front_doa = true;
  config.is_fullduplex_enable = true;
  config.is_enable = true;
  config.is_doa_enable = true;
  config.speaker_speed = voice_config.speaker_config.speaker_speed;
  // config.wakeup_name = "小K";
  // config.custom_bot = voice_config.bot_config.custom_data;

  status = controller.SetVoiceConfig(config, 5000);
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Set voice config failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }

  // 订阅声音数据
  controller.SubscribeBfVoiceData(receive_voice());

  return 0;
}

int close_audio_controller() {
  auto& controller = robot.GetAudioController();
  auto status = controller.Stop();
  if (status.code != magic::dog::ErrorCode::OK) {
    std::cerr << "Stop TTS failed"
              << ", code: " << status.code
              << ", message: " << status.message << std::endl;
    robot.Shutdown();
    return -1;
  }
  return 0;
}

void greetings(const std::string& name) {
  try {
    std::string tts_id, tts_content;
    auto it = BETAGO_MEMBERS.find(name);
    if (it != BETAGO_MEMBERS.end()) {
      const member& m = it->second;
      tts_id = std::to_string(m.command_id);
      tts_content = m.greeting;
    } else {
      tts_id = "10000086";
      static std::random_device rd;
      static std::mt19937 gen(rd());
      std::uniform_int_distribution<> dis(0, DEFAULT_GREETING_TEMPLATES.size() - 1);
      const std::string& tmpl = DEFAULT_GREETING_TEMPLATES[dis(gen)];
      char buffer[128];
      std::snprintf(buffer, sizeof(buffer), tmpl.c_str(), name.c_str());
      tts_content = buffer;
    }

    magic::dog::TtsCommand tts;
    tts.id = tts_id;
    tts.content = tts_content;
    tts.priority = magic::dog::TtsPriority::HIGH;
    tts.mode = magic::dog::TtsMode::CLEARBUFFER;
    auto& controller = robot.GetAudioController();
    auto status = controller.Play(tts);
    if (status.code != magic::dog::ErrorCode::OK) {
      std::cerr << "Play TTS failed"
                << ", code: " << status.code
                << ", message: " << status.message << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "Exception in greetings: " << e.what() << std::endl;
  }
}