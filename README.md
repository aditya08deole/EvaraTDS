# EvaraTDS: Industrial TDS & Conductivity Library

![Version](https://img.shields.io/badge/version-1.3.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-ESP32%20%7C%20Arduino-orange.svg)

---

## Overview

The **EvaraTDS** library is a high-precision, industrial-grade firmware module designed for measuring **Total Dissolved Solids (TDS)** and **Electrical Conductivity (EC)**.

### 🚀 Key Features (v1.3.0)
* **ML-Enhanced Dual-Mode Calibration**:
    * `MODE_STATIC`: Updated Quadratic Model ($R^2=0.9987$) for high-precision lab testing.
    * `MODE_INLINE`: Updated Flow Compensation Model ($R^2=0.9993$) that corrects signal attenuation in active pipe loops.
* **DSP Noise Rejection**: Integrated Median Filter to ignore bubble spikes.
* **Dual Output**: Calculates both **TDS (ppm)** and **EC (µS/cm)**.
* **Temperature Compensation**: Automatic normalization to 25°C.

---

## 🔧 Usage

### 1. Basic Setup

```cpp
#include <EvaraTDS.h>

EvaraTDS tds;

void setup() {
  tds.begin();
  
  // IMPORTANT: Select your environment
  tds.setMode(MODE_INLINE); // Uses new v1.3.0 Inline Calibration Model
  // tds.setMode(MODE_STATIC); // Uses new v1.3.0 Static Calibration Model
  
  tds.setTDSFactor(0.5); // 0.5 for NaCl (USA), 0.7 for Hydroponics
}
## 🎛️ Advanced Calibration: The K-Factor

While the **EvaraTDS** library includes highly accurate ML models ($R^2 > 0.99$), real-world hardware conditions can vary. The **K-Factor** is a linear multiplier used to fine-tune the final output to match a reference standard.

### Why do I need it?
Even with a perfect software model, physical factors can introduce small offsets:
1.  **Probe Aging:** As electrodes oxidize over months, resistance changes.
2.  **Cable Resistance:** Long wires (>2m) add impedance.
3.  **Manufacturing Tolerance:** Slight variations in the 100Ω or 1kΩ resistors on your PCB.

### How to Calculate K
If you notice a consistent percentage error between your EvaraTDS reading and a commercial reference pen, calculate K as follows:

$$K = \frac{\text{Reference Value}}{\text{EvaraTDS Reading}}$$

### Example Scenario
* **Situation:** You are measuring a calibration solution known to be **500 ppm**.
* **Reading:** Your EvaraTDS system reads **480 ppm**.
* **Calculation:** $K = 500 / 480 = \mathbf{1.041}$

### Implementation
Apply this factor in your `setup()`:

```cpp
void setup() {
    tds.begin();
    tds.setMode(MODE_INLINE);
    
    // Apply the correction calculated above
    tds.setKFactor(1.041); 
}