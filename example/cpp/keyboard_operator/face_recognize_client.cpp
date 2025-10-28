//
// Created by fsindustry on 2025/10/27.
//
#include <opencv2/opencv.hpp>
#include <curl/curl.h>
#include <iostream>
#include <vector>
#include <string>

// 目标服务器 URL
const std::string SERVER_URL = "http://120.92.77.233:3999/face/frame";

// 本地保存文件名
const std::string LOCAL_IMAGE_PATH = "capture.jpg";

// 用于接收 HTTP 响应
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

int main() {
    // 打开默认摄像头
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "无法打开摄像头" << std::endl;
        return 1;
    }

    std::cout << "拍摄一帧画面进行人脸识别..." << std::endl;

    cv::Mat frame;
    cap >> frame;  // 读取一帧
    cap.release();

    if (frame.empty()) {
        std::cerr << "无法读取摄像头画面" << std::endl;
        return 1;
    }

    // ✅ 在发送前保存图像到本地
    if (!cv::imwrite(LOCAL_IMAGE_PATH, frame)) {
        std::cerr << "保存图像失败: " << LOCAL_IMAGE_PATH << std::endl;
        return 1;
    }
    std::cout << "图像已保存到本地: " << LOCAL_IMAGE_PATH << std::endl;

    // 将图像编码为 JPEG 格式（用于发送）
    std::vector<uchar> buf;
    if (!cv::imencode(".jpg", frame, buf)) {
        std::cerr << "图像编码失败" << std::endl;
        return 1;
    }

    // 初始化 libcurl
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "初始化 libcurl 失败" << std::endl;
        return 1;
    }

    CURLcode res;
    std::string response;
    struct curl_slist* headers = nullptr;

    // 构建 multipart/form-data 请求体
    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    curl_mime_filename(part, "frame.jpg");
    curl_mime_data(part, reinterpret_cast<const char*>(buf.data()), buf.size());
    curl_mime_type(part, "image/jpeg");

    // 设置 curl 选项
    curl_easy_setopt(curl, CURLOPT_URL, SERVER_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    // 发送请求
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "请求失败: " << curl_easy_strerror(res) << std::endl;
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code == 200) {
            std::cout << "识别结果: " << response << std::endl;
        } else {
            std::cerr << "HTTP 错误: " << http_code << "\n返回内容: " << response << std::endl;
        }
    }

    // 释放资源
    curl_mime_free(mime);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return 0;
}
