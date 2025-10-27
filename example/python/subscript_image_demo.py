#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MagicDog SDK Python Usage Example

This file demonstrates how to use MagicDog SDK Python bindings to control the robot.
"""

import sys
import time
import logging
import os
import cv2
import numpy as np
from typing import Optional
from datetime import datetime

logging.basicConfig(
    level=logging.INFO,  # Minimum log level
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)

try:
    import magicdog_python as magicdog
    from magicdog_python import TtsCommand, TtsPriority, TtsMode, GetSpeechConfig, ErrorCode

    logging.info("Successfully imported MagicDog Python module!")
    logging.info(f"Imported magicdog_python path: {sys.path}")
except ImportError as e:
    logging.error(f"Import failed: {e}")
    logging.error("Please run build_python.sh to build Python bindings first")
    sys.exit(1)


class ColorImageSaver:
    """彩色图像保存器"""

    def __init__(self, save_dir="color_images"):
        """初始化保存目录"""
        self.save_dir = save_dir
        self.image_count = 0
        self.create_save_directory()

    def create_save_directory(self):
        """创建保存目录"""
        if not os.path.exists(self.save_dir):
            os.makedirs(self.save_dir)
            logging.info(f"Created save directory: {self.save_dir}")

    def save_color_image(self, color_image):
        """保存彩色图像"""
        try:
            # 将图像数据转换为numpy数组
            img_data = np.frombuffer(color_image.data, dtype=np.uint8)

            # 使用OpenCV解码图像
            img = cv2.imdecode(img_data, cv2.IMREAD_COLOR)

            if img is not None:
                # 生成文件名
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
                filename = f"color_image_{timestamp}_{self.image_count:04d}.jpg"
                filepath = os.path.join(self.save_dir, filename)

                # 保存图像
                success = cv2.imwrite(filepath, img)
                if success:
                    logging.info(f"Saved color image: {filename} (Size: {img.shape})")
                    self.image_count += 1
                else:
                    logging.error(f"Failed to save image: {filename}")
            else:
                logging.error("Failed to decode color image")

        except Exception as e:
            logging.error(f"Error saving color image: {e}")


def main():
    """Main function"""
    logging.info("MagicDog SDK Audio Python Example Program")

    # 初始化图像保存器
    image_saver = ColorImageSaver()

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

    # 订阅RGBD彩色图像并保存
    sensor_controller.subscribe_rgbd_color_image(lambda color_image:
                                                 image_saver.save_color_image(color_image))

    # 如果需要订阅双目相机的彩色图像，可以添加以下订阅
    # sensor_controller.subscribe_binocular_color_image(lambda color_image:
    #                                                   image_saver.save_color_image(color_image))

    # 打开RGBD相机（彩色相机）
    status = sensor_controller.open_rgbd_camera()
    if status.code != ErrorCode.OK:
        logging.error(f"Failed to open RGBD camera: {status.message}")
        robot.shutdown()
        return

    logging.info("Starting to capture color images for 30 seconds...")
    start_time = time.time()

    # 运行30秒，持续捕获图像
    while time.time() - start_time < 30:
        time.sleep(0.1)  # 短暂休眠，避免占用太多CPU

        # 可以在这里添加其他逻辑
        if image_saver.image_count % 10 == 0 and image_saver.image_count > 0:
            logging.info(f"Captured {image_saver.image_count} images so far...")

    logging.info(f"Image capture completed. Total images saved: {image_saver.image_count}")

    # 关闭传感器
    status = sensor_controller.close_rgbd_camera()
    if status.code != ErrorCode.OK:
        logging.error(f"Failed to close RGBD camera: {status.message}")
        robot.shutdown()
        return

    status = sensor_controller.close_binocular_camera()
    if status.code != ErrorCode.OK:
        logging.error(f"Failed to close binocular camera: {status.message}")
        robot.shutdown()
        return

    time.sleep(2)

    status = sensor_controller.close_channel_switch()
    if status.code != ErrorCode.OK:
        logging.error(f"Failed to close channel: {status.message}")
        robot.shutdown()
        return

    # Avoid unprocessed buffered data in lcm
    time.sleep(2)

    robot.disconnect()
    robot.shutdown()
    logging.info(f"\nExample program execution completed! Total images saved: {image_saver.image_count}")


if __name__ == "__main__":
    main()