"""
从摄像头采集视频帧，上传到 go2ai 服务端的 /face/frame 接口
返回 JSON （姓名、相似度、置信度等）
"""

import cv2
import time
import requests

SERVER_URL = "http://120.92.77.233:3999/face/frame"

def main():
    cap = cv2.VideoCapture(0)  # 0 表示默认摄像头
    if not cap.isOpened():
        print("无法打开摄像头")
        return

    print(f"开始人脸识别视频流，按 'q' 退出...")
    while True:
        ret, frame = cap.read()
        if not ret:
            print("无法读取摄像头画面")
            break

        # 编码为jpg
        _, img_encoded = cv2.imencode(".jpg", frame)

        try:
            files = {"file": ("frame.jpg", img_encoded.tobytes(), "image/jpeg")}
            r = requests.post(SERVER_URL, files=files, timeout=10)
            if r.ok:
                print(r.json())
            else:
                print(f"请求失败: {r.status_code}, {r.text}")
        except requests.exceptions.RequestException as e:
            print(f"请求错误: {e}")

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
        time.sleep(1)

    cap.release()

if __name__ == "__main__":
    main()