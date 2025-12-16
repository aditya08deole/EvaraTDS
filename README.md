# EvaraTDS: Professional TDS Sensor Library

![Version](https://img.shields.io/badge/version-1.1.0-blue.svg) ![License](https://img.shields.io/badge/license-MIT-green.svg)

An industrial-grade Arduino/ESP32 library for calculating Total Dissolved Solids (TDS) and Electrical Conductivity (EC). It features a **Non-Linear Quadratic Regression Model** (R²=0.988) and an integrated **Median Filter DSP engine** for noise rejection.

## ?? Key Features
* **High Precision Math**: Uses a 50-point look-up table with quadratic interpolation for the 0-1000ppm range.
* **Noise Rejection**: Built-in Median Filter eliminates bubbles and electrical spikes.
* **Dual Output**: Calculates both **TDS (ppm)** and **EC (µS/cm)**.
* **Temperature Compensation**: Automatic adjustment based on water temperature (default 2.0%/°C).
* **Flexible Standards**: Supports both USA (0.5) and European/Hydroponic (0.7) conversion factors.

## ?? Installation
1.  Download this repository as a ZIP file.
2.  Open Arduino IDE -> Sketch -> Include Library -> Add .ZIP Library.
3.  Select the downloaded file.

## ?? Usage

```cpp
#include <EvaraTDS.h>

EvaraTDS tds;

void setup() {
  Serial.begin(115200);
  tds.begin();
  tds.setTDSFactor(0.5); // 0.5 for NaCl (USA), 0.7 for 442 (Hydroponics)
}

void loop() {
  // 1. Get Raw Data (e.g., from ADS1115 or Analog Pin)
  float voltage = analogRead(A0) * (3.3 / 4095.0);
  float temperature = 25.0; // Get this from a DS18B20

  // 2. Update the DSP Engine
  tds.update(voltage, temperature);

  // 3. Read Results
  Serial.print("TDS: ");
  Serial.print(tds.getTDS());
  Serial.print(" ppm | EC: ");
  Serial.print(tds.getEC());
  Serial.println(" uS/cm");
  
  delay(1000);
}