# Fall Detection Application

[![IMAGE ALT TEXT](https://github.com/chaloemchai-beer/Image/blob/main/Screenshot%202024-06-12%20180857.png?raw=true)](https://www.youtube.com/embed/37Hv5lHzm2g?feature=oembed "Video Title")

This is a Flask web application that uses the YOLOv8 object detection model to detect falls in real-time video streams. When a fall is detected, the application sends an alert notification to a LINE Notify channel along with the frame where the fall was detected.

## Prerequisites

- Python 3.x
- Flask
- OpenCV
- NumPy
- Ultralytics (for YOLOv8)
- Requests

## Installation

1. Clone the repository or download the source code.
2. Install the required Python packages by running `pip install -r requirements.txt`.
3. Obtain a LINE Notify token by following the instructions [here](https://notify-bot.line.me/en/). Replace `line_token = ""` with your actual LINE Notify token.

## Usage

1. Run the Flask application with `python app.py`.
2. Open your web browser and navigate to `http://localhost:5000`.
3. The application will start streaming video from the IP camera at `http://192.168.251.4/cam-video`. Make sure this IP address is correct for your camera.
4. When a fall is detected, the application will send an alert notification to your LINE Notify channel with the frame containing the detected fall.

## How it Works

1. The `gen_frames()` function is a generator that continuously fetches frames from the video stream.
2. Each frame is passed through the YOLOv8 model for pose estimation and fall detection.
3. The model detects human poses and calculates the aspect ratio (width/height) of the bounding box around each detected person.
4. If the aspect ratio exceeds a certain threshold (1.4 in this case), a fall is assumed to have occurred.
5. When a fall is detected, the frame is annotated with a red bounding box and the text "Fall".
6. The `send_alert_to_line()` function is called in a separate thread to send the annotated frame and a message to the LINE Notify channel.

## Customization

- To use a different object detection model, replace `model = YOLO('yolov8s-pose.pt', task="pose")` with the appropriate model path and task.
- Adjust the fall detection threshold by modifying the aspect ratio condition (`w / h > 1.4`).
- Customize the video stream URL by changing `fetch_latest_frame('http://192.168.251.4/cam-video')`.
