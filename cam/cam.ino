#include "esp_camera.h"
#include "fd_forward.h"
#include <WiFi.h>
#include <WebServer.h>

// WiFi
const char* ssid = "ssii";
const char* password = "ming123459sa";
int previousDetections = -1; 
int currentDetections = -1; 
// Blynk
#define BLYNK_TEMPLATE_ID "TMPL64wgnVBDg"
#define BLYNK_TEMPLATE_NAME "EmbededProject"
#define BLYNK_AUTH_TOKEN "AZ9oKspUFAR7B8A7_t4brXV8Iainwpap"
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp32.h>

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
 
bool initCamera() {
 
  camera_config_t config;
 
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;
 
  esp_err_t result = esp_camera_init(&config);
 
  if (result != ESP_OK) {
    return false;
  }
 
  return true;
}
 
mtmn_config_t mtmn_config = {0};
int detections = 0;
 
void setup() {
  Serial.begin(115200);

  // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println("\nConnected to WiFi");
    Serial.println(WiFi.localIP());
 
  if (!initCamera()) {
 
    Serial.printf("Failed to initialize camera...");
    return;
  }

  // Blynk setup
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
 
  mtmn_config = mtmn_init_config();
}
 
void loop() {
  Blynk.run();
  camera_fb_t * frame;
  frame = esp_camera_fb_get();
 
  dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, frame->width, frame->height, 3);
  fmt2rgb888(frame->buf, frame->len, frame->format, image_matrix->item);
 
  esp_camera_fb_return(frame);
 
  box_array_t *boxes = face_detect(image_matrix, &mtmn_config);
 
  if (boxes != NULL) {
    currentDetections = 1; 
    free(boxes->score);
    free(boxes->box);
    free(boxes->landmark);
    free(boxes);
  }
  else {
    currentDetections = 0;
  }
  if (currentDetections != previousDetections) {
    if (currentDetections > 0) {
      Blynk.virtualWrite(V8, 1);
      Serial.println("Face detected, Blynk updated: 1");
      Blynk.logEvent("face", "There is person in your farm");
    } else {
      Blynk.virtualWrite(V8, 0);
      Serial.println("No face detected, Blynk updated: 0");
    }
    previousDetections = currentDetections;
  }
  dl_matrix3du_free(image_matrix);
 
}