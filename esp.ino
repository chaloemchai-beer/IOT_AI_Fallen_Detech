#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>
#include <HTTPClient.h>

const char* WIFI_SSID = "SSID";
const char* WIFI_PASS = "Password";

WebServer server(80);
HTTPClient httpClient;

// Define available resolutions
static auto loRes = esp32cam::Resolution::find(320, 240);
static auto midRes = esp32cam::Resolution::find(640, 480);
static auto hiRes = esp32cam::Resolution::find(800, 600);

const int pirPin = 13; // GPIO pin used for the PIR sensor's output
volatile bool motionDetected = false;

void IRAM_ATTR detectsMovement() {
  motionDetected = true;
}

void serveJpg() {
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("CAPTURE FAIL");
    server.send(503, "", "");
    return;
  }
  Serial.printf("CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(), static_cast<int>(frame->size()));
  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}

void serveVideo() {
  WiFiClient client = server.client();

  // Send the video header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Cache-Control: no-cache, no-store, must-revalidate");
  client.println("Pragma: no-cache");
  client.println("Expires: 0");
  client.println();

  while (client.connected()) {
    auto frame = esp32cam::capture();
    if (frame == nullptr) {
      Serial.println("CAPTURE FAIL");
      break;
    }

    client.print("--frame\r\n");
    client.print("Content-Type: image/jpeg\r\n\r\n");
    frame->writeTo(client);
    client.print("\r\n");

    delay(100); // Adjust the delay based on your desired frame rate
  }
}

void handleJpgLo() {
  if (!esp32cam::Camera.changeResolution(loRes)) {
    Serial.println("SET-LO-RES FAIL");
  }
  serveJpg();
}

void handleJpgMid() {
  if (!esp32cam::Camera.changeResolution(midRes)) {
    Serial.println("SET-MID-RES FAIL");
  }
  serveJpg();
}

void handleJpgHi() {
  if (!esp32cam::Camera.changeResolution(hiRes)) {
    Serial.println("SET-HI-RES FAIL");
  }
  serveJpg();
}

void handleVideo() {
  if (!esp32cam::Camera.changeResolution(hiRes)) {
    Serial.println("SET-HI-RES FAIL");
  }
  serveVideo();
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(pirPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(pirPin), detectsMovement, RISING);

  using namespace esp32cam;
  Config cfg;
  cfg.setPins(pins::AiThinker);
  cfg.setResolution(hiRes);
  cfg.setBufferCount(2);
  cfg.setJpeg(80);

  bool ok = Camera.begin(cfg);
  Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("  /cam-lo.jpg");
  Serial.println("  /cam-hi.jpg");
  Serial.println("  /cam-mid.jpg");
  Serial.println("  /cam-video");

  server.on("/cam-lo.jpg", handleJpgLo);
  server.on("/cam-mid.jpg", handleJpgMid);
  server.on("/cam-hi.jpg", handleJpgHi);
  server.on("/cam-video", handleVideo);

  server.begin();
}

void loop() {
  server.handleClient();

  if (motionDetected) {
    Serial.println("Motion detected!");
    handleVideo();
    motionDetected = false; // Reset the motion flag
  }
}
