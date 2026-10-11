#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>

// ---------------- PIN DEFINITIONS (LOWER XIAO ESP32-S3) ----------------
// Drive Motors (First L298N)
#define ENA_DRIVE   1  // GPIO 1 / D0
#define IN1_DRIVE   2  // GPIO 2 / D1
#define IN2_DRIVE   3  // GPIO 3 / D2
#define IN3_DRIVE   4  // GPIO 4 / D3
#define IN4_DRIVE   5  // GPIO 5 / D4
#define ENB_DRIVE   6  // GPIO 6 / D5

// Servo Pump Actuator Pin
#define SERVO_PIN   7  // GPIO 7 / D6

// ---------------- SERVO ANGLES ----------------
// Adjust these two angles based on how your physical switch is mounted
const int PUMP_OFF_ANGLE = 0;    // Angle where servo arm is away from switch
const int PUMP_ON_ANGLE  = 90;   // Angle where servo arm depresses/flips switch

Servo pumpServo;

// ---------------- ESP-NOW DATA STRUCTURE ----------------
typedef struct struct_message {
  uint8_t driveCommand; // 0=STOP, 1=FWD, 2=REV, 3=LEFT, 4=RIGHT
  uint8_t motorSpeed;   // 0 - 255 PWM
  bool pumpState;       // true = PUMP ON, false = PUMP OFF
} struct_message;

struct_message incomingData;

// ---------------- PUMP CONTROL VIA SERVO ----------------
void setPumpState(bool turnOn) {
  if (turnOn) {
    pumpServo.write(PUMP_ON_ANGLE);
    Serial.println(">>> SERVO: DEPRESSING SWITCH (PUMP ON)");
  } else {
    pumpServo.write(PUMP_OFF_ANGLE);
    Serial.println(">>> SERVO: RELEASING SWITCH (PUMP OFF)");
  }
}

// ---------------- ESP-NOW RECEIVE CALLBACK ----------------
void onDataRecv(const esp_now_recv_info *info, const uint8_t *incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  
  // 1. Actuate Mechanical Pump Switch
  setPumpState(incomingData.pumpState);

  // 2. Control Rover Drive Motors
  uint8_t speed = incomingData.motorSpeed;

  switch (incomingData.driveCommand) {
    case 1: // FORWARD
      digitalWrite(IN1_DRIVE, HIGH);
      digitalWrite(IN2_DRIVE, LOW);
      digitalWrite(IN3_DRIVE, HIGH);
      digitalWrite(IN4_DRIVE, LOW);
      analogWrite(ENA_DRIVE, speed);
      analogWrite(ENB_DRIVE, speed);
      break;

    case 2: // REVERSE
      digitalWrite(IN1_DRIVE, LOW);
      digitalWrite(IN2_DRIVE, HIGH);
      digitalWrite(IN3_DRIVE, LOW);
      digitalWrite(IN4_DRIVE, HIGH);
      analogWrite(ENA_DRIVE, speed);
      analogWrite(ENB_DRIVE, speed);
      break;

    case 3: // TURN LEFT
      digitalWrite(IN1_DRIVE, LOW);
      digitalWrite(IN2_DRIVE, HIGH);
      digitalWrite(IN3_DRIVE, HIGH);
      digitalWrite(IN4_DRIVE, LOW);
      analogWrite(ENA_DRIVE, speed);
      analogWrite(ENB_DRIVE, speed);
      break;

    case 4: // TURN RIGHT
      digitalWrite(IN1_DRIVE, HIGH);
      digitalWrite(IN2_DRIVE, LOW);
      digitalWrite(IN3_DRIVE, LOW);
      digitalWrite(IN4_DRIVE, HIGH);
      analogWrite(ENA_DRIVE, speed);
      analogWrite(ENB_DRIVE, speed);
      break;

    default: // STOP
      digitalWrite(IN1_DRIVE, LOW);
      digitalWrite(IN2_DRIVE, LOW);
      digitalWrite(IN3_DRIVE, LOW);
      digitalWrite(IN4_DRIVE, LOW);
      analogWrite(ENA_DRIVE, 0);
      analogWrite(ENB_DRIVE, 0);
      break;
  }
}

void setup() {
  Serial.begin(115200);

  // Allow allocation of timer for Servo on ESP32
  ESP32PWM::allocateTimer(0);
  pumpServo.setPeriodHertz(50);             // Standard 50Hz servo
  pumpServo.attach(SERVO_PIN, 500, 2400);   // Standard 500-2400us pulse width

  // Force initial state to OFF on boot
  pumpServo.write(PUMP_OFF_ANGLE);

  // Initialize Drive Motor Pins
  pinMode(ENA_DRIVE, OUTPUT);
  pinMode(IN1_DRIVE, OUTPUT);
  pinMode(IN2_DRIVE, OUTPUT);
  pinMode(IN3_DRIVE, OUTPUT);
  pinMode(IN4_DRIVE, OUTPUT);
  pinMode(ENB_DRIVE, OUTPUT);

  digitalWrite(IN1_DRIVE, LOW);
  digitalWrite(IN2_DRIVE, LOW);
  digitalWrite(IN3_DRIVE, LOW);
  digitalWrite(IN4_DRIVE, LOW);

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed!");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);
  Serial.println("\n--- LOWER BOARD READY (MECHANICAL SERVO PUMP ACTUATOR ACTIVE) ---");
}

void loop() {
  // All processing is handled in the ESP-NOW receive callback
}