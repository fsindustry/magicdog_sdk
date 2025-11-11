# 查找 OpenCV
find_package(OpenCV REQUIRED)
# 查找 libcurl
find_package(CURL REQUIRED)
# 查找 json
find_package(nlohmann_json REQUIRED)
# 查找 ALSA
find_package(ALSA REQUIRED)

# 设置 MagicDog SDK 路径
set(MAGICDOG_SDK_DIR "/opt/magic_robotics/magicdog_sdk")

# 查找 MagicDog 库
find_library(MAGICDOG_SDK_LIB
        NAMES magicdog_sdk magicdog magicdog::sdk
        PATHS ${MAGICDOG_SDK_DIR}/lib
        NO_DEFAULT_PATH
        REQUIRED
)

message(STATUS "Found MagicDog library: ${MAGICDOG_SDK_LIB}")

# 包含目录
include_directories(include)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${MAGICDOG_SDK_DIR}/include)

# 源文件
set(SOURCES
        src/config.cpp
        src/motion_control.cpp
        src/audio_control.cpp
        src/sensor_control.cpp
        src/http_client.cpp
        src/face_recognition.cpp
        src/voice_recognition.cpp
#        src/nav_control.cpp
        src/main.cpp
)

# 创建可执行文件
add_executable(betago ${SOURCES})

target_include_directories(betago PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${MAGICDOG_SDK_DIR}/include
)

target_link_libraries(betago PRIVATE
        ${OpenCV_LIBS}
        ${CURL_LIBRARIES}
        ${MAGICDOG_SDK_LIB}
        nlohmann_json::nlohmann_json
)