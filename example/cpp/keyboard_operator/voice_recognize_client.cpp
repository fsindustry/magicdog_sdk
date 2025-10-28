#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <curl/curl.h>
#include <alsa/asoundlib.h>

constexpr const char* SERVER_URL = "http://120.92.77.233:3999/speech/once";
constexpr int SAMPLE_RATE = 16000;
constexpr int CHANNELS = 1;
constexpr snd_pcm_format_t FORMAT = SND_PCM_FORMAT_S16_LE;
constexpr int CHUNK = 1024;
constexpr int SILENCE_THRESHOLD = 500;   // 小于此 RMS 视为静音
constexpr double SILENCE_DURATION = 5.0; // 静音持续秒数
constexpr const char* TEMP_FILE = "temp.wav";

// 计算音量 RMS
double compute_rms(const std::vector<char>& buffer) {
    const int16_t* samples = reinterpret_cast<const int16_t*>(buffer.data());
    size_t count = buffer.size() / 2;
    double sum = 0;
    for (size_t i = 0; i < count; ++i) {
        sum += samples[i] * samples[i];
    }
    return std::sqrt(sum / count);
}

// 保存为 WAV 文件
void save_wav(const std::string& filename, const std::vector<char>& data) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "无法写入文件: " << filename << std::endl;
        return;
    }

    uint32_t data_size = data.size();
    uint32_t file_size = 36 + data_size;
    uint16_t audio_format = 1; // PCM
    uint16_t bits_per_sample = 16;
    uint32_t byte_rate = SAMPLE_RATE * CHANNELS * bits_per_sample / 8;
    uint16_t block_align = CHANNELS * bits_per_sample / 8;

    out.write("RIFF", 4);
    out.write(reinterpret_cast<char*>(&file_size), 4);
    out.write("WAVE", 4);
    out.write("fmt ", 4);

    uint32_t subchunk1_size = 16;
    out.write(reinterpret_cast<char*>(&subchunk1_size), 4);
    out.write(reinterpret_cast<char*>(&audio_format), 2);
    out.write(reinterpret_cast<const std::ostream::char_type*>(&CHANNELS), 2);
    out.write(reinterpret_cast<const std::ostream::char_type*>(&SAMPLE_RATE), 4);
    out.write(reinterpret_cast<char*>(&byte_rate), 4);
    out.write(reinterpret_cast<char*>(&block_align), 2);
    out.write(reinterpret_cast<char*>(&bits_per_sample), 2);

    out.write("data", 4);
    out.write(reinterpret_cast<char*>(&data_size), 4);
    out.write(data.data(), data.size());
}

// 录音直到静音
void record_until_silence(std::vector<char>& recorded) {
    snd_pcm_t* pcm_handle;
    snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    snd_pcm_set_params(pcm_handle, FORMAT, SND_PCM_ACCESS_RW_INTERLEAVED,
                       CHANNELS, SAMPLE_RATE, 1, 500000); // 0.5s 延迟

    std::vector<char> buffer(CHUNK * 2);
    std::cout << "请开始说话...（静音自动结束）" << std::endl;
    auto silence_start = std::chrono::steady_clock::time_point();
    bool in_silence = false;

    while (true) {
        snd_pcm_readi(pcm_handle, buffer.data(), CHUNK);
        recorded.insert(recorded.end(), buffer.begin(), buffer.end());

        double rms = compute_rms(buffer);
        // std::cout << "rms=" << rms << std::endl;

        if (rms < SILENCE_THRESHOLD) {
            if (!in_silence) {
                in_silence = true;
                silence_start = std::chrono::steady_clock::now();
            } else {
                auto elapsed = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - silence_start).count();
                if (elapsed > SILENCE_DURATION)
                    break;
            }
        } else {
            in_silence = false;
        }
    }

    snd_pcm_close(pcm_handle);
    std::cout << "录音结束" << std::endl;
}

// HTTP 发送音频文件
void send_audio(const std::string& filename) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "初始化 libcurl 失败" << std::endl;
        return;
    }

    CURLcode res;
    std::string response;
    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    curl_mime_filedata(part, filename.c_str());
    curl_mime_type(part, "audio/wav");

    curl_easy_setopt(curl, CURLOPT_URL, SERVER_URL);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

    // 回调保存响应
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            auto* resp = reinterpret_cast<std::string*>(userdata);
            resp->append(ptr, size * nmemb);
            return size * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "HTTP 请求失败: " << curl_easy_strerror(res) << std::endl;
    } else {
        std::cout << "识别结果: " << response << std::endl;
    }

    curl_mime_free(mime);
    curl_easy_cleanup(curl);
}

int main() {
    std::vector<char> recorded;
    record_until_silence(recorded);
    save_wav(TEMP_FILE, recorded);
    send_audio(TEMP_FILE);
    std::remove(TEMP_FILE);
    return 0;
}
