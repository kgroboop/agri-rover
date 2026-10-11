# 🤖 Next-Gen AI Agricultural Rover
Autonomous edge-vision, soil-sensing, and smart irrigation robot built for precision small-scale farming.

## 📌 Overview
The **Agri-Rover** is a low-cost, dual-module robotic platform designed to automate crop health monitoring and localized micro-irrigation. By running computer vision inferencing directly on-device using edge hardware, it detects plant diseases and stress without cloud latency.

## 🛠️ Hardware & Tech Stack
* **Vision & AI:** Seeed Studio XIAO ESP32S3 Sense (On-board camera & edge inferencing)
* **Drive & Control:** ESP8266 + L298N driver / MG995 high-torque metal-gear servos
* **Telemetry:** Capacitive Soil Moisture Sensors, Gas/Flame/Motion sensors
* **Power Subsystem:** 2S 18650 Li-ion Battery + 2S BMS + Step-down Buck Converter (Regulated 5V rail)

## 📂 Repository Structure
* `/code` - Contains the Arduino/C++ source code for upper and lower microcontrollers.
* `/schematics` - Includes system wiring diagrams and pin mapping details.

## 📐 Circuit & Wiring Diagram
View the complete wiring schematic inside the `/schematics` folder or reference the pinout mapping in the documentation.
