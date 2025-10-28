import pyaudio
import wave
import audioop
import requests
import os

import time

SERVER_URL = "http://120.92.77.233:3999/speech/once"

FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 16000
CHUNK = 1024
TEMP_FILE = "temp.wav"

# 声音检测参数
SILENCE_THRESHOLD = 500  # 小于这个 RMS 值视为静音（数值可调整）
SILENCE_DURATION = 5.0   # 静音持续秒数触发停止

def record_until_silence():
    p = pyaudio.PyAudio()
    stream = p.open(format=FORMAT, channels=CHANNELS,
                    rate=RATE, input=True,
                    frames_per_buffer=CHUNK)

    print("请开始说话...（静音自动结束）")
    frames = []
    silence_start = None

    while True:
        data = stream.read(CHUNK, exception_on_overflow=False)
        frames.append(data)

        # 计算当前块的能量（音量大小）
        rms = audioop.rms(data, 2)

        if rms < SILENCE_THRESHOLD:
            if silence_start is None:
                silence_start = time.time()
            elif time.time() - silence_start > SILENCE_DURATION:
                # 静音持续足够长时间，结束
                break
        else:
            silence_start = None

    print("录音结束")
    stream.stop_stream()
    stream.close()
    p.terminate()

    wf = wave.open(TEMP_FILE, 'wb')
    wf.setnchannels(CHANNELS)
    wf.setsampwidth(p.get_sample_size(FORMAT))
    wf.setframerate(RATE)
    wf.writeframes(b''.join(frames))
    wf.close()

def send_audio():
    with open(TEMP_FILE, 'rb') as f:
        files = {"file": ("audio.wav", f, "audio/wav")}
        r = requests.post(SERVER_URL, files=files)
        print("识别结果：", r.json())

if __name__ == "__main__":
    record_until_silence()
    send_audio()
    os.remove(TEMP_FILE)
