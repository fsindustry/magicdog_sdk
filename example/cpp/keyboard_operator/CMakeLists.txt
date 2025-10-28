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

#### face recognition demo ####
add_executable(face_recognize_client face_recognize_client.cpp)
target_include_directories(face_recognize_client PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
)
target_link_libraries(face_recognize_client
        ${OpenCV_LIBS}
        ${CURL_LIBRARIES}
)


#### voice recognition demo ####
add_executable(voice_recognize_client voice_recognize_client.cpp)
target_include_directories(voice_recognize_client PRIVATE
        ${ALSA_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
)
target_link_libraries(voice_recognize_client
        ${ALSA_LIBRARIES}
        ${CURL_LIBRARIES}
)


#### keyboard recognition demo ####
add_executable(keyboard_operator keyboard_operator.cpp)

target_include_directories(keyboard_operator PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${MAGICDOG_SDK_DIR}/include
)

target_link_libraries(keyboard_operator PRIVATE
        ${OpenCV_LIBS}
        ${CURL_LIBRARIES}
        ${MAGICDOG_SDK_LIB}
        nlohmann_json::nlohmann_json
)