#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MagicDog SDK Python Usage Example

This file demonstrates how to use MagicDog SDK Python bindings to control the robot.
"""

import sys
import time
import threading
import tty
import termios
import os
import cv2
import numpy as np
import logging
from typing import Optional

import requests

logging.basicConfig(
    level=logging.INFO,  # Minimum log level
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)

try:
    import magicdog_python as magicdog
    from magicdog_python import TtsCommand, TtsPriority, TtsMode, GetSpeechConfig, ErrorCode, GaitSpeedRatio

    logging.info("Successfully imported MagicDog Python module!")
    # Print the path of imported magicdog_python
    logging.info(f"Imported magicdog_python path: {sys.path}")
except ImportError as e:
    logging.error(f"Import failed: {e}")
    sys.exit(1)

left_x = 0.0
left_y = 0.0
right_x = 0.0
right_y = 0.0
exit_flag = False
robot = None
high_controller = None
sensor_controller = None
audio_controller = None


# Get single character input (no echo)
def getch():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
        # logging.info(f"Received character: {ch}")
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch


# Position control standing
def recovery_stand():
    global high_controller
    try:
        # Set gait to position control standing
        status = high_controller.set_gait(magicdog.GaitMode.GAIT_STAND_R)
        if status.code != magicdog.ErrorCode.OK:
            logging.error(f"Failed to set position control standing: {status.message}")
        else:
            logging.info("Robot set to position control standing")
    except Exception as e:
        logging.error(f"Error executing position control standing: {e}")


# Force control standing
def balance_stand():
    global high_controller
    try:
        # Set gait to force control standing
        status = high_controller.set_gait(magicdog.GaitMode.GAIT_STAND_B)
        if status.code != magicdog.ErrorCode.OK:
            logging.error(f"Failed to set force control standing: {status.message}")
        else:
            logging.info("Robot set to force control standing")
    except Exception as e:
        logging.error(f"Error executing force control standing: {e}")


# Execute trick - lie down
def execute_trick():
    global high_controller
    try:
        # Execute lie down trick
        status = high_controller.execute_trick(magicdog.TrickAction.ACTION_LIE_DOWN)
        if status.code != magicdog.ErrorCode.OK:
            logging.error(f"Failed to execute trick: {status.message}")
        else:
            logging.info("Trick executed successfully")
    except Exception as e:
        logging.error(f"Error executing trick: {e}")


# Execute tricm - jump
def execute_jump():
    global high_controller
    try:
        # Execute lie down trick
        status = high_controller.execute_trick(magicdog.TrickAction.ACTION_HIGH_JUMP)
        if status.code != magicdog.ErrorCode.OK:
            logging.error(f"Failed to execute trick: {status.message}")
        else:
            logging.info("Trick executed successfully")
    except Exception as e:
        logging.error(f"Error executing trick: {e}")


# Switch gait to down climb stairs mode
def change_gait_to_down_climb_stairs():
    global high_controller
    try:
        gait_mode = magicdog.GaitMode.GAIT_DOWN_CLIMB_STAIRS
        # status = high_controller.set_gait_speed_ratio(gait_mode, 0.5, 5000)
        # gait_mode = magicdog.GaitMode.GAIT_WALK
        current_gait = high_controller.get_gait()
        if current_gait != gait_mode:
            status = high_controller.set_gait(gait_mode)
            if status.code != magicdog.ErrorCode.OK:
                logging.error(f"Failed to set down climb stairs gait: {status.message}")
                return False

            # Wait for gait switch to complete
            while high_controller.get_gait() != gait_mode:
                time.sleep(0.01)
        return True
    except Exception as e:
        logging.error(f"Error changing gait: {e}")
        return False


# Define a function to simulate joystick input based on key presses
def motion_control_thread():
    global left_x, left_y, right_x, right_y, exit_flag
    logging.info("Key function description:")
    logging.info("  1        Function 1: Position control standing")
    logging.info("  2        Function 2: Force control standing")
    logging.info("  3        Function 3: Execute trick - lie down")
    logging.info("  w        Move forward")
    logging.info("  a        Move left")
    logging.info("  s        Move backward")
    logging.info("  d        Move right")
    logging.info("  q        Turn left")
    logging.info("  e        Turn right")
    logging.info("  space    Jump")
    logging.info("  x        Stop movement")
    logging.info("  ESC      Exit program")

    while not exit_flag:
        key = getch()
        if key == '\x1b':  # ESC key
            exit_flag = True
            break
        elif key == '1':
            # Position control standing
            recovery_stand()
        elif key == '2':
            # Force control standing
            balance_stand()
        elif key == '3':
            # Execute trick - lie down
            execute_trick()
        elif key == ' ':
            # Execute trick - jump
            if change_gait_to_down_climb_stairs():
                left_y = 0.0
                left_x = 0.0
                right_x = 0.0
                right_y = 0.0
            execute_jump()

        elif key == 'w':
            if change_gait_to_down_climb_stairs():
                left_y = 1.0
                left_x = 0.0
                right_x = 0.0
                right_y = 0.0
        elif key == 's':
            if change_gait_to_down_climb_stairs():
                left_y = -1.0
                left_x = 0.0
                right_x = 0.0
                right_y = 0.0
        elif key == 'a':
            if change_gait_to_down_climb_stairs():
                left_x = -1.0
                left_y = 0.0
                right_x = 0.0
                right_y = 0.0
        elif key == 'd':
            if change_gait_to_down_climb_stairs():
                left_x = 1.0
                left_y = 0.0
                right_x = 0.0
                right_y = 0.0
        elif key == 'q':
            if change_gait_to_down_climb_stairs():
                left_x = 0.0
                left_y = 0.0
                right_x = -1.0
                right_y = 0.0
        elif key == 'e':
            if change_gait_to_down_climb_stairs():
                left_x = 0.0
                left_y = 0.0
                right_x = 1.0
                right_y = 0.0
        elif key == 'x':
            if change_gait_to_down_climb_stairs():
                left_x = 0.0
                left_y = 0.0
                right_x = 0.0
                right_y = 0.0


# Create motion controlling command sending thread
def motion_send_thread(high_controller):
    global left_x, left_y, right_x, right_y, exit_flag
    while not exit_flag:
        joy_command = magicdog.JoystickCommand()
        joy_command.left_x_axis = left_x
        joy_command.left_y_axis = left_y
        joy_command.right_x_axis = right_x
        joy_command.right_y_axis = right_y

        status = high_controller.send_joystick_command(joy_command)
        if status.code != magicdog.ErrorCode.OK:
            logging.error(f"Failed to send joystick command: {status.message}")
            break
        time.sleep(0.01)  # 10ms interval, consistent with C++ code

SERVER_URL = "http://120.92.77.233:3999/face/frame"
greeting_dict = {
    "富正鑫": {
        "command_id": 100000000001,
        "greeting": "富哥好！今天又是财富自由的一天吗？"
    },
    "卢祚": {
        "command_id": 100000000002,
        "greeting": "卢总好！福气满满的卢总今天有什么好运？"
    },
    "肖文然": {
        "command_id": 100000000003,
        "greeting": "文然兄好！文艺气息扑面而来！"
    },
    "陈嘉敏": {
        "command_id": 100000000004,
        "greeting": "嘉敏好！聪明伶俐的你今天有什么新发现？"
    },
    "任阿伟": {
        "command_id": 100000000005,
        "greeting": "阿伟好！伟大的任务等着你去完成！"
    },
    "高名发": {
        "command_id": 100000000006,
        "greeting": "名发哥好！名声和发财两不误！"
    },
    "鞠鑫锐": {
        "command_id": 100000000007,
        "greeting": "鑫锐好！三金加持，锐不可当！"
    },
    "宋卓著": {
        "command_id": 100000000008,
        "greeting": "卓著好！卓越成就的你又有什么新突破？"
    },
    "吴敬超": {
        "command_id": 100000000009,
        "greeting": "敬超好！令人敬佩的超凡能力！"
    },
    "崔照斌": {
        "command_id": 100000000010,
        "greeting": "照斌好！照亮全场的文武双全！"
    },
    "唐贵乾": {
        "command_id": 100000000011,
        "greeting": "贵乾好！富贵乾坤尽在掌握！"
    }
}

def get_personalized_greeting(name):
    """获取个性化问候"""
    if name in greeting_dict:
        return greeting_dict[name]["greeting"]
    else:
        return f"{name}你好！今天过得怎么样？"

def face_recognize(color_image):
    try:
        # convert Uint8Vector to numpy array
        img_data = np.array(color_image.data, dtype=np.uint8)

        # reshape image data to matrix（height, width, channels）
        img = img_data.reshape((color_image.height, color_image.width, 3))

        # convert color（BGR<->RGB）
        # img = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)

        try:
            _, img_encoded = cv2.imencode(".jpg", img)
            files = {"file": ("frame.jpg", img_encoded.tobytes(), "image/jpeg")}
            resp = requests.post(SERVER_URL, files=files, timeout=10)
            if resp.ok:
                response_dict = resp.json()
                if response_dict.get('status') == 'success':
                    data = response_dict.get("data", {})
                    data_status = data.get("status", "error")
                    if data_status == "success":
                        logging.info(f"Face recognize success: {response_dict}")
                        # get name of the
                        name = response_dict['data']['name']
                        logging.info(f"Name is : {name}")

                        tts_command = TtsCommand()
                        tts_command.id = greeting_dict[name]["command_id"]
                        tts_command.content = greeting_dict[name]["greeting"]
                        tts_command.priority = TtsPriority.HIGH
                        tts_command.mode = TtsMode.CLEARTOP
                        status = audio_controller.play(tts_command)
                        if status.code != magicdog.ErrorCode.OK:
                            logging.error(f"Failed to play voice: {status.message}")
                            robot.shutdown()
                            return
                        high_controller.execute_trick(magicdog.TrickAction.ACTION_CHEER_UP)

        except requests.exceptions.RequestException as e:
            logging.error(f"Failed to request face recognition service: {e}")


    except Exception as e:
        logging.error(f"Error while saving color image: {e}")



def main():
    """Main function"""
    global robot, high_controller, sensor_controller

    logging.info("MagicDog SDK Python Example Program")

    ## initialize robot
    robot = magicdog.MagicRobot()
    if not robot.initialize("192.168.55.10"):
        logging.error("Initialization failed")
        return

    if not robot.connect():
        logging.error("Connection failed")
        robot.shutdown()
        return

    ## initialize high level motion controller
    # Set motion control level to high level
    status = robot.set_motion_control_level(magicdog.ControllerLevel.HIGH_LEVEL)
    if status.code != magicdog.ErrorCode.OK:
        logging.error(f"Failed to set motion control level: {status.message}")
        robot.shutdown()
        return

    # Get high level motion controller
    high_controller = robot.get_high_level_motion_controller()

    # ## initialize image sensor controller
    # sensor_controller = robot.get_sensor_controller()
    #
    # # 设置彩色图像订阅回调
    # sensor_controller.subscribe_rgbd_color_image(face_recognize)
    #
    # # 打开相关通道
    # status = sensor_controller.open_channel_switch()
    # if status.code != ErrorCode.OK:
    #     logging.error(f"Failed to open channel: {status.message}")
    #     robot.shutdown()
    #     return
    #
    # status = sensor_controller.open_rgbd_camera()
    # if status.code != ErrorCode.OK:
    #     logging.error(f"Failed to open RGBD camera: {status.message}")
    #     robot.shutdown()
    #     return
    #
    # audio_controller = robot.get_audio_controller()
    # # Get current volume
    # volume = audio_controller.get_volume()
    # if volume <= 0:
    #     status = audio_controller.set_volume(30)
    #     if status.code != magicdog.ErrorCode.OK:
    #         logging.error(f"Failed to set volume: {status.message}")
    #         robot.shutdown()
    #         return

    # Create a thread for receiving keyboard input
    control_thread = threading.Thread(target=motion_control_thread)
    control_thread.daemon = True  # Set as daemon thread
    control_thread.start()

    # Create movement controlling command sending thread
    send_thread = threading.Thread(target=motion_send_thread, args=(high_controller,))
    send_thread.daemon = True  # Set as daemon thread
    send_thread.start()

    logging.info("Program started, please use keys to control robot...")


    # Main thread waits for exit signal
    global exit_flag
    try:
        while not exit_flag:
            time.sleep(0.1)
    except KeyboardInterrupt:
        logging.info("\nReceived interrupt signal, exiting...")
        exit_flag = True

    logging.info("Closing robot connection...")
    robot.shutdown()
    robot.disconnect()

    logging.info("\nExample program execution completed!")


if __name__ == "__main__":
    main()
