import cv2
import requests
import sys
import select
import tty
import termios

SERVER_URL = "http://120.92.77.233:3999"  # 修改为你的服务端地址

# curl -X GET http://120.92.77.233:3999/face/list
# curl -X DELETE http://127.0.0.1:3999/face/delete?name=zz

def register_face(name, frame):
    _, buf = cv2.imencode(".jpg", frame)
    files = {"file": ("frame.jpg", buf.tobytes(), "image/jpeg")}
    try:
        resp = requests.post(f"{SERVER_URL}/face/register", params={"name": name}, files=files, timeout=10)
        if resp.headers.get("content-type", "").startswith("application/json"):
            print(resp.json())
        else:
            print("响应非 JSON:", resp.text)
    except requests.exceptions.RequestException as e:
        print("请求失败:", e)

def get_key():
    """
    非阻塞读取键盘单个字符
    """
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setcbreak(fd)
        if select.select([sys.stdin], [], [], 0.1)[0]:
            return sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return None

def main():
    name = input("请输入注册人姓名: ").strip()
    if not name:
        print("姓名不能为空!")
        return

    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("无法打开摄像头")
        return

    print(f"摄像头已开启，为 [{name}] 注册人脸")
    print("在终端按 '1' 拍照 & 上传，按 'q' 退出")

    while True:
        ret, frame = cap.read()
        if not ret:
            print("无法读取摄像头画面")
            break

        key = get_key()
        if key == "1":
            print("拍照并上传中...")
            register_face(name, frame)
        elif key == "q":
            print("退出程序")
            break

    cap.release()

if __name__ == "__main__":
    main()