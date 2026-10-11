/*
 * Agri-Rover Lightweight Blue Color Tracker (NO ML / NO EDGE IMPULSE)
 * Board: Seeed Studio XIAO ESP32S3 Sense
 */

#include "esp_camera.h"
#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include "DHT.h"

/* Camera Pin Configuration (XIAO ESP32S3) -------------------------------- */
#define CAMERA_MODEL_XIAO_ESP32S3
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

/* Sensor & Peripheral Pins ----------------------------------------------- */
#define SOIL_PIN        1   // GPIO 1 / A0 - Soil Moisture Sensor (Analog)
#define DHT_PIN         2   // GPIO 2 / A1 - DHT22 Sensor
#define SERVO_PROBE_PIN 3   // GPIO 3 / A2 - Probe Servo
#define SERVO_PAN_PIN   4   // GPIO 4 / A3 - Camera Pan Servo
#define DHTTYPE         DHT22
#define SOIL_DRY_THRES  70  // Moisture threshold (%)

/* State Machine ----------------------------------------------------------- */
enum State { SEARCHING, DRIVING, SAMPLING };
State currentState = SEARCHING;

unsigned long driveStartTime = 0;
unsigned long lastTrackerPrint = 0;

/* ESP-NOW Data Package ---------------------------------------------------- */
typedef struct struct_message {
  uint8_t driveCommand; // 0=STOP, 1=FWD, 2=REV, 3=LEFT, 4=RIGHT
  uint8_t motorSpeed;   // 0 - 255 PWM
  bool pumpState;       // true = ON, false = OFF
} struct_message;

struct_message controlData;

/* Target Receiver MAC Address -------------------------------------------- */
uint8_t receiverAddress[] = {0xAC, 0x27, 0x6E, 0xA7, 0x42, 0x40};
esp_now_peer_info_t peerInfo;

/* Global Objects ---------------------------------------------------------- */
DHT dht(DHT_PIN, DHTTYPE);
Servo probeServo;
Servo panServo;

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_RGB565, // Direct RGB scanning without compression
    .frame_size = FRAMESIZE_QQVGA,    // 160x120 resolution for maximum stability
    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

/* ESP-NOW Callback -------------------------------------------------------- */
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {}

/* Algorithmic Color Detection (Blue Pixel Thresholding) -------------------- */
bool detectBlueColor() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed!");
    return false;
  }

  uint32_t bluePixelCount = 0;
  uint32_t totalPixels = fb->width * fb->height;

  // Scan frame pixels (RGB565 format)
  for (size_t i = 0; i < fb->len; i += 2) {
    uint16_t pixel = (fb->buf[i] << 8) | fb->buf[i + 1];

    // Extract 8-bit RGB components from RGB565
    uint8_t r = (pixel >> 11) & 0x1F; r = (r * 527 + 23) >> 6;
    uint8_t g = (pixel >> 5) & 0x3F;  g = (g * 259 + 33) >> 6;
    uint8_t b = pixel & 0x1F;         b = (b * 527 + 23) >> 6;

    // Check if blue component strongly dominates red and green
    if (b > 80 && (b - r) > 30 && (b - g) > 20) {
      bluePixelCount++;
    }
  }

  esp_camera_fb_return(fb);

  // Trigger true if at least 8% of total frame area matches blue
  float blueRatio = (float)bluePixelCount / totalPixels;
  if (blueRatio > 0.08) {
    Serial.printf(">>> BLUE CUBE DETECTED! Pixel Coverage: %.2f%%\n", blueRatio * 100);
    return true;
  }

  return false;
}

/* Manual Serial Overrides ------------------------------------------------ */
bool isHandTriggered() {
  if (Serial.available() > 0) {
    char c = Serial.read();
    while (Serial.available() > 0) Serial.read();
    if (c == 't' || c == 'T') {
      Serial.println("\n[MANUAL OVERRIDE TRIGGER DETECTED]");
      return true;
    }
  }
  return false;
}

/* Setup ------------------------------------------------------------------- */
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=================================");
  Serial.println(" LIGHTWEIGHT COLOR BRAIN BOOT    ");
  Serial.println("=================================");

  dht.begin();

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  probeServo.setPeriodHertz(50);
  panServo.setPeriodHertz(50);
  probeServo.attach(SERVO_PROBE_PIN, 600, 2400);
  panServo.attach(SERVO_PAN_PIN, 600, 2400);

  probeServo.write(0);   
  panServo.write(90);    

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW Init Failed!");
    return;
  }

  esp_now_register_send_cb(onDataSent);
  
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ERROR: Failed to add peer!");
  }

  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
  } else {
    Serial.println("Camera initialized without ML engine!");
  }

  Serial.println("\n--- NO-ML COLOR ROVER READY ---");
  Serial.println("Show the blue face of the Rubik's cube to trigger movement!");
}

/* Loop State Machine ------------------------------------------------------ */
void loop() {
  switch (currentState) {

    case SEARCHING: {
      controlData.driveCommand = 0; // STOP
      controlData.motorSpeed = 0;
      controlData.pumpState = false;
      esp_now_send(receiverAddress, (uint8_t *) &controlData, sizeof(controlData));

      if (millis() - lastTrackerPrint > 1000) {
        Serial.println("[SEARCHING] Scanning frame for blue color...");
        lastTrackerPrint = millis();
      }

      bool blueDetected = detectBlueColor();
      bool manualOverride = isHandTriggered();

      if (blueDetected || manualOverride) {
        Serial.println(">>> Target Acquired! Firing ESP-NOW drive command for 10s...");
        driveStartTime = millis();
        currentState = DRIVING;
      }
      break;
    }

    case DRIVING: {
      if (millis() - driveStartTime < 10000) { 
        controlData.driveCommand = 1; // FORWARD
        controlData.motorSpeed = 160;  
        controlData.pumpState = false;
        esp_now_send(receiverAddress, (uint8_t *) &controlData, sizeof(controlData));
      } else {
        Serial.println(">>> Drive run completed. Stopping to sample...");
        controlData.driveCommand = 0; 
        controlData.motorSpeed = 0;
        controlData.pumpState = false;
        esp_now_send(receiverAddress, (uint8_t *) &controlData, sizeof(controlData));
        currentState = SAMPLING;
      }
      break;
    }

    case SAMPLING: {
      delay(500); 

      Serial.println("Lowering Probe Servo...");
      probeServo.write(90); 
      delay(1200);

      float temp = dht.readTemperature();
      float hum = dht.readHumidity();
      int rawSoil = analogRead(SOIL_PIN);
      int soilPct = constrain(map(rawSoil, 4095, 1500, 0, 100), 0, 100);

      bool needWater = (soilPct < SOIL_DRY_THRES);

      Serial.printf("Temp: %.1fC | Hum: %.1f%% | Soil Moisture: %d%%\n", temp, hum, soilPct);

      Serial.println("Retracting Probe Servo...");
      probeServo.write(0);
      delay(500);

      if (needWater) {
        Serial.println("Soil Dry! Triggering Pump for 3s...");
        controlData.pumpState = true;
        controlData.driveCommand = 0;
        esp_now_send(receiverAddress, (uint8_t *) &controlData, sizeof(controlData));
        delay(3000);
        controlData.pumpState = false;
        esp_now_send(receiverAddress, (uint8_t *) &controlData, sizeof(controlData));
      }

      Serial.println("\nSampling Complete. Returning to SEARCHING...\n");
      currentState = SEARCHING;
      break;
    }
  }

  delay(30);
}