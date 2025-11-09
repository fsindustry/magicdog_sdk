#include "http_client.h"

#include <curl/curl.h>

#include <iostream>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
  size_t total_size = size * nmemb;
  output->append(static_cast<char*>(contents), total_size);
  return total_size;
}

bool upload_image(const std::shared_ptr<magic::dog::CompressedImage>& msg, std::string& response) {
  thread_local CURL* curl = nullptr;
  if (!curl) {
    curl = curl_easy_init();
    if (!curl) {
      std::cerr << "人脸识别初始化 libcurl 失败" << std::endl;
      return false;
    }
    curl_easy_setopt(curl, CURLOPT_URL, IMAGE_SERVER_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
  }

  curl_mime* mime = curl_mime_init(curl);
  curl_mimepart* part = curl_mime_addpart(mime);
  curl_mime_name(part, "file");
  curl_mime_filename(part, "frame.jpg");
  curl_mime_data(part, reinterpret_cast<const char*>(msg->data.data()), msg->data.size());
  curl_mime_type(part, "image/jpeg");

  response.clear();
  curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(curl);
  curl_mime_free(mime);

  if (res != CURLE_OK) {
    std::cerr << "人脸识别请求失败: " << curl_easy_strerror(res) << std::endl;
    return false;
  }

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  if (http_code != 200) {
    std::cerr << "人脸识别请求错误: " << http_code << "\n返回内容: " << response << std::endl;
    return false;
  }
  return true;
}

bool upload_audio(const std::shared_ptr<magic::dog::ByteMultiArray>& msg, std::string& response) {
  thread_local CURL* voice_curl = nullptr;
  if (!voice_curl) {
    voice_curl = curl_easy_init();
    if (!voice_curl) {
      std::cerr << "语音识别初始化 libcurl 失败" << std::endl;
      return false;
    }
    curl_easy_setopt(voice_curl, CURLOPT_URL, VOICE_SERVER_URL.c_str());
    curl_easy_setopt(voice_curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(voice_curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                auto* resp = reinterpret_cast<std::string*>(userdata);
                resp->append(ptr, size * nmemb);
                return size * nmemb; });
  }

  curl_mime* mime = curl_mime_init(voice_curl);
  curl_mimepart* part = curl_mime_addpart(mime);
  curl_mime_name(part, "file");
  curl_mime_filename(part, "voice.wav");
  curl_mime_data(part, reinterpret_cast<const char*>(msg->data.data()), msg->data.size());
  curl_mime_type(part, "audio/wav");

  response.clear();
  curl_easy_setopt(voice_curl, CURLOPT_MIMEPOST, mime);
  curl_easy_setopt(voice_curl, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(voice_curl);
  curl_mime_free(mime);

  if (res != CURLE_OK) {
    std::cerr << "语音识别请求失败: " << curl_easy_strerror(res) << std::endl;
    return false;
  }

  long http_code = 0;
  curl_easy_getinfo(voice_curl, CURLINFO_RESPONSE_CODE, &http_code);
  if (http_code != 200) {
    std::cerr << "语音识别请求错误: " << http_code << "\n返回内容: " << response << std::endl;
    return false;
  }
  return true;
}