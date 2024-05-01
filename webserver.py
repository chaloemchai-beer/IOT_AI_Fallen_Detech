from flask import Flask, render_template, Response
import cv2
import requests
import numpy as np
import threading
from ultralytics import YOLO

app = Flask(__name__)

# Load a model
model = YOLO('yolov8s-pose.pt', task="pose")  # pretrained YOLOv8n model

def send_alert_to_line(message, frame):
    line_token = ""  # Replace with your actual LINE Notify token
    headers = {
        "Authorization": "Bearer " + line_token
    }
    # Encode frame to JPEG format
    _, buffer = cv2.imencode('.jpg', frame)
    files = {
        'imageFile': ('image.jpg', buffer.tobytes(), 'image/jpeg'),
        'message': (None, message)
    }
    response = requests.post("https://notify-api.line.me/api/notify", headers=headers, files=files)
    print("LINE Notify response:", response.text)  # Logging the response for debugging

def async_send_alert_to_line(message, frame):
    thread = threading.Thread(target=send_alert_to_line, args=(message, frame))
    thread.start()

def fetch_latest_frame(url):
    """Fetch the latest frame from a video stream."""
    cap = cv2.VideoCapture(url)
    ret, frame = cap.read()
    cap.release()
    if ret:
        return frame
    else:
        print("Failed to retrieve frame")
        return None

def gen_frames():
    frame_number = 1
    fall = 0
    while True:
        frame = fetch_latest_frame('http://192.168.251.4/cam-video')
        if frame is None:
            continue

        results = model(frame)
        try:
            boxes = results[0].boxes  # Boxes object for bbox outputs
            for box in boxes:
                x, y, w, h = box.xywh[0]
                kpts = results[0].keypoints
                nk = kpts.shape[1]
                points = []
                for i in range(nk):
                    keypoint = kpts.xy[0, i]
                    x_kpt, y_kpt = int(keypoint[0].item()), int(keypoint[1].item())
                    points.append((x_kpt, y_kpt))
                # Draw skeleton on frame
                for i in range(len(points) - 1):
                    cv2.line(frame, points[i], points[i + 1], (0, 255, 0), 2)
                if w / h > 1.4:
                    fall += 1
                    print("Fall detected at frame {}".format(frame_number))
                    # Draw a red rectangle around the detected fall
                    cv2.rectangle(frame, (int(x - w/2), int(y - h/2)), (int(x + w/2), int(y + h/2)), (0, 0, 255), 2)
                    # Print "Fall" on top of person's head
                    cv2.putText(frame, "Fall", (int(x), int(y - h/2 - 10)), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)
                    # Send alert to LINE Notify
                    async_send_alert_to_line("Fall detected!", frame)
                else:
                    # Print "Stable" on top of person's head
                    cv2.putText(frame, "Stable", (int(x), int(y - h/2 - 10)), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        except:
            pass

        _, buffer = cv2.imencode('.jpg', frame)
        frame_bytes = buffer.tobytes()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')

        frame_number += 1

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/video_feed')
def video_feed():
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)