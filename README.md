# 🤖 Next-Gen AI Agricultural Rover (Agri-Rover)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32S3 / ESP8266](https://img.shields.io/badge/Platform-ESP32S3%20%2F%20ESP8266-blue.svg)](https://www.espressif.com/)
[![Edge AI: NVIDIA Nemotron](https://img.shields.io/badge/Edge%20AI-NVIDIA%20Nemotron-76B900.svg)](https://developer.nvidia.com/)

An autonomous, edge-vision robotic platform engineered for precision small-scale farming, real-time crop disease detection, and intelligent micro-irrigation.

---

## 📌 Table of Contents
1. [Project Overview](#project-overview)
2. [NVIDIA & Nebius Integration Architecture](#nvidia--nebius-integration-architecture)
3. [Token Factory Workflow Acceleration](#token-factory-workflow-acceleration)
4. [Hardware & Tech Stack](#hardware--tech-stack)
5. [Setup & Installation Instructions](#setup--installation-instructions)
6. [License](#license)

---

## 🚀 Project Overview
Smallholder farming faces severe yield losses due to blanket irrigation and delayed disease identification. The **Agri-Rover** solves this by combining a mobile robotic chassis, multi-sensor environmental telemetry, and real-time edge computer vision to inspect crops, diagnose health anomalies locally, and deliver targeted micro-irrigation precisely where needed.

---

## 🧠 NVIDIA & Nebius Integration Architecture

### 1. NVIDIA Open-Source Models & Nemotron
* **On-Device & Cloud Synergy:** While the rover executes lightweight vision inferencing locally on the **Seeed Studio XIAO ESP32S3 Sense**, complex agronomic classification, multi-modal anomaly analysis, and predictive crop yield modeling are powered by fine-tuned open-source **NVIDIA Nemotron** variants.
* **Perception & Decision Support:** Nemotron models process telemetry streams to correlate soil moisture thresholds, ambient gas/flame metrics, and camera frames into actionable irrigation and treatment schedules.

### 2. Nebius Tools & Cloud Services
* **Nebius AI Cloud Infrastructure:** Used for high-throughput model training, weight quantization, and continuous integration pipeline testing before deploying vision weights onto the edge microcontrollers.
* **Nebius Distributed Storage:** Manages large-scale agricultural image datasets used for training crop health classification models.

---

## ⚡ Token Factory Workflow Acceleration
During the development of this hackathon project, **Token Factory** drastically accelerated our engineering workflow across three core areas:
* **Rapid Code Generation:** Token Factory's accelerated LLM endpoints generated optimized C++ boilerplate code for dual-controller communication (ESP32S3 to ESP8266) and I2C sensor polling in a fraction of usual development time.
* **Model Prompt Tuning:** Streamlined prompt engineering and dataset labeling scripts for our custom computer vision classifiers.
* **Documentation & Pitch Structuring:** Automated technical README generation, architectural diagram text-mapping, and pitch deck copy formatting.

---

## 🛠️ Hardware & Tech Stack

* **Vision & Edge AI:** Seeed Studio XIAO ESP32S3 Sense (OV2640 Camera Module)
* **Drive & Actuation:** ESP8266 Controller, L298N Motor Driver, MG995 High-Torque Servos, Relay Pump
* **Telemetry Sensors:** Capacitive Soil Moisture Sensor, Gas/Flame Detector, PIR Motion Sensor
* **Display Output:** SSD1306 OLED Display (128x64 I2C)
* **Power Subsystem:** 2S 18650 Li-ion Battery Pack, 2S Battery Management System (BMS), Step-Down Buck Converter (Regulated 5V Rail)

---

## ⚙️ Setup & Installation Instructions

### Prerequisites
* Arduino IDE (v2.0 or higher) installed on your system.
* Required Board Packages: `esp32` by Espressif Systems and `ESP8266` board support.
* Required Libraries: 
  * `Adafruit_GFX` & `Adafruit_SSD1306`
  * `ESP32Camera` / `esp32-camera`

### Step 1: Clone the Repository
```bash
git clone (https://github.com/kgroboop/agri-rover.git)
cd agri-rover

'Step 2: Flash the Upper Module (Vision & Edge AI)
Open the upper module code (/code/upper_module/brain.ino) in Arduino IDE.

Select your board: Seeed Studio XIAO ESP32S3.

Configure Partition Scheme to Huge APP (3MB No OTA/1MB SPIFFS).

Connect the board via USB-C and click Upload.

Step 3: Flash the Lower Module (Actuation & Telemetry)
Open (/code/lower_module/driver.ino) in Arduino IDE.

Select your board: NodeMCU 1.0 / ESP-12E Module (or equivalent ESP8266 board).

Upload the motor and sensor actuation code.'

📜 License
This project is open-source and licensed under the MIT License. See the LICENSE file for details.
