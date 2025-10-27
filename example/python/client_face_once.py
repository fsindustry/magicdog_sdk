"""
人脸识别客户端（单次）
启动 → 从摄像头获取一帧 → 上传到服务端 /face/frame → 打印识别结果 → 退出
"""

import cv2
import requests

SERVER_URL = "http://120.92.77.233:3999/face/frame"

def main():
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("无法打开摄像头")
        return

    print("拍摄一帧画面进行人脸识别...")

    ret, frame = cap.read()
    cap.release()

    if not ret:
        print("无法读取摄像头画面")
        return

    # 编码为jpeg
    _, img_encoded = cv2.imencode(".jpg", frame)

    # 发送到服务端
    try:
        files = {"file": ("frame.jpg", img_encoded.tobytes(), "image/jpeg")}
        resp = requests.post(SERVER_URL, files=files, timeout=10)
        if resp.ok:
            print("识别结果:", resp.json())
        else:
            print(f"请求失败: {resp.status_code} {resp.text}")
    except requests.exceptions.RequestException as e:
        print(f"请求异常: {e}")

if __name__ == "__main__":
    main()