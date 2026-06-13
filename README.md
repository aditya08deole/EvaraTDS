# EvaraTDS: Industrial TDS & Conductivity Library

![Version](https://img.shields.io/badge/version-1.3.1-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-ESP32%20%7C%20Arduino-orange.svg)

---

## Overview

The **EvaraTDS** library is a high-precision, industrial-grade firmware module designed for measuring **Total Dissolved Solids (TDS)** and **Electrical Conductivity (EC)**.

### Key Features (v1.3.1)

* **ML-Enhanced Dual-Mode Calibration**:
    * `MODE_STATIC`: Updated Quadratic Model (R²=0.9987) for high-precision lab/bottle testing.
    * `MODE_INLINE`: Updated Flow Compensation Model (R²=0.9993) that corrects signal attenuation in active pipe loops.
* **DSP Noise Rejection**: 15-sample Median Filter to reject micro-bubble spikes and pump transients.
* **Cold-Start Seeding**: Buffer pre-filled on first reading — no false low TDS during warm-up.
* **Input Validation**: Voltage clamped to [0–5V], temperature clamped to [-10–100°C].
* **Dual Voltage Getters**: `getRawVoltage()` (pre-compensation) and `getCompVoltage()` (post-compensation).
* **Dual Output**: Calculates both **TDS (ppm)** and **EC (uS/cm)**.
* **Temperature Compensation**: Automatic normalization to 25°C reference standard.
* **Mode-Preserving `begin()`**: Calling `begin()` after a soft reset does NOT wipe your `setMode()` setting.

---

## Usage

### 1. Basic Setup

```cpp
#include <EvaraTDS.h>

EvaraTDS tds;

void setup() {
  tds.begin();

  // IMPORTANT: Set your mode AFTER begin() -- it will survive re-init calls
  tds.setMode(MODE_INLINE);  // For pump/pipe systems
  // tds.setMode(MODE_STATIC); // For cup/beaker lab testing

  tds.setTDSFactor(0.5); // 0.5 for NaCl (USA), 0.7 for Hydroponics (442)
}

void loop() {
  tds.update(voltage, temperature); // Pass actual temp for full compensation
  Serial.println(tds.getTDS());     // ppm
  Serial.println(tds.getEC());      // uS/cm
  Serial.println(tds.getRawVoltage());   // Median-filtered, pre-compensation
  Serial.println(tds.getCompVoltage()); // Post temp-compensation
}
```

---

## Advanced Calibration: The K-Factor

While the **EvaraTDS** library includes highly accurate ML models (R² > 0.99), real-world hardware conditions can introduce small offsets.

### Why do I need it?
1. **Probe Aging**: Electrodes oxidize over time, changing resistance.
2. **Cable Resistance**: Long wires (>2m) add impedance.
3. **Manufacturing Tolerance**: Variations in resistors on the PCB.

### How to Calculate K

If your EvaraTDS reading differs from a reference pen by a consistent percentage:

```
K = Reference Value / EvaraTDS Reading
```

**Example**: Reference = 500 ppm, Reading = 480 ppm → K = 500/480 = **1.041**

### Implementation

```cpp
void setup() {
    tds.begin();
    tds.setMode(MODE_INLINE);
    tds.setKFactor(1.041); // Apply field calibration trim
}
```

---

## Hardware Wiring (ESP32 + ADS1115 + DS18B20)

A **4.7 kΩ pull-up resistor** between the DS18B20 Data line and VCC is mandatory.

| DS18B20 Wire | ESP32 Pin | Function | Notes |
|-------------|----------|----------|------|
| **Red (VCC)** | 3.3V / 5V | Power | |
| **Black (GND)** | GND | Ground | |
| **Yellow (Data)** | GPIO 27 | 1-Wire Data | Requires 4.7kΩ pull-up |

```text
       [ESP32]                       [ADS1115]
      +-------+                     +---------+
      |    3V3|------------------->| VCC     |
      |    GND|------------------->| GND     |
      |     22|------------------->| SCL     |
      |     21|------------------->| SDA     |
      |       |                     | ADDR -> GND
      |     27|<-----+              | A0 <--------- [TDS Signal]
      +-------+      |              +---------+
                     |
        [DS18B20]    |
       (Data Pin)----+
                     |
                    [R] 4.7k Pull-up
                     |
                   [VCC]
```

---

## Changelog

### v1.3.1
- FIX: Resolved all git merge conflicts (kept v1.3.0 ML logic throughout)
- FIX: Buffer cold-start zero-bias eliminated via pre-seeding on first update()
- FIX: BUFFER_SIZE increased 10 → 15 to match firmware median window
- FIX: `getVoltage()` split into `getRawVoltage()` and `getCompVoltage()` for clarity
- FIX: Input validation added — voltage [0–5V], temp [-10–100°C] clamped
- FIX: `begin()` no longer resets `_currentMode` — mode survives soft resets

### v1.3.0
- Two-stage ML regression (sensorPPM → realPPM) for Static and Inline modes
- Improved R² from 0.988 (v1.2.0) to 0.9987/0.9993

### v1.2.0
- Initial dual-mode physics engine (direct voltage quadratic)
