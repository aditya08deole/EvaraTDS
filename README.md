# EvaraTDS: Industrial TDS & Conductivity Library

![Version](https://img.shields.io/badge/version-1.1.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-ESP32%20%7C%20Arduino-orange.svg)

---

## Overview

The **EvaraTDS** library is a high-precision, industrial-grade firmware module designed for measuring **Total Dissolved Solids (TDS)** and **Electrical Conductivity (EC)** in aqueous solutions.

Unlike conventional implementations that rely on simple linear approximations, **EvaraTDS** employs a **2nd-Order Polynomial Regression Model** derived from laboratory calibration data (**R² = 0.988**), providing superior accuracy across the **0–1000 PPM** range.

The library integrates a **Digital Signal Processing (DSP)** engine using **median filtering**, enabling robust noise rejection in electrically noisy environments such as industrial panels containing pumps, relays, solenoids, and motors.

---

## Hardware Architecture

The system is optimized for the **ESP32** platform while remaining compatible with standard Arduino environments. High-resolution analog acquisition is achieved using the **ADS1115 (16-bit ADC)**, and temperature compensation is handled by the **DS18B20** digital temperature sensor.

### Component List

1. **Microcontroller:** ESP32 Development Board (e.g., DOIT DEVKIT V1)
2. **ADC Module:** ADS1115 (16-bit Sigma-Delta ADC, I²C)
3. **Temperature Sensor:** DS18B20 (Waterproof, 1-Wire)
4. **TDS Probe:** Analog TDS Sensor (Gravity Interface, K = 1.0)
5. **Passive Components**
   - 1 × 4.7 k? resistor (DS18B20 pull-up)
   - 2 × 4.7 k? resistors (I²C pull-ups if not present on ADS1115 module)

---

## Connection Diagram

### 1. ADS1115 (ADC) Interfacing

The ADS1115 communicates over the I²C bus. The `ADDR` pin must be tied to **GND** to set the device address to `0x48`.

| ADS1115 Pin | ESP32 Pin | Function | Notes |
|------------|----------|----------|------|
| **VCC** | 3.3V / 5V | Power | 5V recommended for improved dynamic range |
| **GND** | GND | Ground | Common ground |
| **SCL** | GPIO 22 | I²C Clock | ESP32 default |
| **SDA** | GPIO 21 | I²C Data | ESP32 default |
| **ADDR** | GND | Address Select | Sets address to `0x48` |
| **A0** | TDS Signal | Analog Input | Connect TDS analog output |

---

### 2. DS18B20 (Temperature) Interfacing

A **4.7 k? pull-up resistor** between the Data line and VCC is mandatory.

| DS18B20 Wire | ESP32 Pin | Function | Notes |
|-------------|----------|----------|------|
| **Red (VCC)** | 3.3V / 5V | Power | |
| **Black (GND)** | GND | Ground | |
| **Yellow (Data)** | GPIO 27 | 1-Wire Data | Requires pull-up |

---

## Wiring Schematic

```text
       [ESP32]                       [ADS1115]
      +-------+                     +---------+
      |    3V3|-------------------->| VCC     |
      |    GND|-------------------->| GND     |
      |     22|-------------------->| SCL     |
      |     21|-------------------->| SDA     |
      |       |                     | ADDR -> GND
      |     27|<-----+              | A0 <--------- [TDS Signal]
      +-------+      |              +---------+
                     |
        [DS18B20]    |
       (Data Pin)----+
                     |
                    [R] 4.7k? Pull-up
                     |
                   [VCC]