#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MagicDog SDK Python Usage Example

This file demonstrates how to use MagicDog SDK Python bindings to control the robot,
and how to receive and save RGBD color images to local files.
"""

import sys
import time
import logging
import os
import cv2
import numpy as np
from typing import Optional

logging.basicConfig(
    level=logging.INFO,  # Minimum log level
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)

try:
    import magicdog_python as magicdog
    from magicdog_python import TtsCommand, TtsPriority, TtsMode, GetSpeechConfig, ErrorCode
    logging.info("Successfully imported MagicDog Python module!")
except ImportError as e:
    logging.error(f"Import failed: {e}")
    sys.exit(1)

# 全局计数器，用于保存图片时命名
image_counter = 0

# def save_color_image(color_image):
#     try:
#         logging.info(f"color_image fields: width={color_image.width}, height={color_image.height}, step={color_image.step}")
#         logging.info(f"data type={type(color_image.data)}, len={len(color_image.data)}")
#
#         # 打印前 16 个字节
#         head_bytes = bytes(color_image.data)[:16]
#         logging.info(f"First 16 bytes: {list(head_bytes)}")
#
#         # 原样保存到文件方便分析
#         with open("raw_color_image.bin", "wb") as f:
#             f.write(bytes(color_image.data))
#
#     except Exception as e:
#         logging.error(f"Error while inspecting color image: {e}")

def save_color_image(color_image):
    """
    回调函数：接收 RGBD 彩色图像并保存到本地
    """
    global image_counter
    try:
        # 将 Uint8Vector 转为 numpy 数组
        img_data = np.array(color_image.data, dtype=np.uint8)

        # reshape 成图像矩阵（高度, 宽度, 通道数）
        img = img_data.reshape((color_image.height, color_image.width, 3))

        # 如果颜色顺序不对，可以尝试下面一行转换（BGR<->RGB）
        # img = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)

        output_dir = "rgbd_color_images"
        os.makedirs(output_dir, exist_ok=True)

        image_counter += 1
        filename = os.path.join(output_dir, f"color_image_{image_counter:03d}.jpg")

        # 保存图片
        cv2.imwrite(filename, img)
        logging.info(f"Saved RGBD color image #{image_counter} -> {filename}")

    except Exception as e:
        logging.error(f"Error while saving color image: {e}")

def main():
    """Main function"""
    logging.info("MagicDog SDK Audio Python Example Program")

    local_ip = "192.168.55.10"
    robot = magicdog.MagicRobot()
    if not robot.initialize(local_ip):
        logging.error("Robot initialization failed")
        return

    if not robot.connect():
        logging.error("Robot connection failed")
        robot.shutdown()
        return

    logging.info("Robot connected successfully")

    sensor_controller = robot.get_sensor_controller()

    # 设置彩色图像订阅回调
    sensor_controller.subscribe_rgbd_color_image(save_color_image)

    # 打开相关通道
    status = sensor_controller.open_channel_switch()
    if status.code != ErrorCode.OK:
        logging.error(f"Failed to open channel: {status.message}")
        robot.shutdown()
        return

    status = sensor_controller.open_rgbd_camera()
    if status.code != ErrorCode.OK:
        logging.error(f"Failed to open RGBD camera: {status.message}")
        robot.shutdown()
        return

    # 等待一段时间以接收图像
    logging.info("Receiving RGBD color images for 10 seconds...")
    time.sleep(10)

    # 关闭通道
    sensor_controller.close_rgbd_camera()
    sensor_controller.close_channel_switch()

    # 等待缓冲区数据清空
    time.sleep(2)

    robot.disconnect()
    robot.shutdown()
    logging.info("\nExample program execution completed!")

if __name__ == "__main__":
    main()