# EvaraTDS — Industrial TDS & Conductivity Library

![Version](https://img.shields.io/badge/version-1.3.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-ESP32%20%7C%20Arduino-orange.svg)

---

## Overview

**EvaraTDS** is a high-precision, industrial-grade firmware library for measuring **Total Dissolved Solids (TDS)** and **Electrical Conductivity (EC)** with ESP32 / Arduino platforms. It combines a physics-based approach with machine-learning-enhanced calibration models and DSP filtering to provide robust readings for both static lab measurements and inline pipe systems under flow conditions.

Key design goals:
- High accuracy (ML-calibrated models; R² > 0.99)
- Flow compensation for inline systems (bubble and velocity effects)
- Temperature compensation to 25°C
- Easy hardware interface via ADS1115 ADC and DS18B20 temperature sensor
- On-device fine-tuning using a K-Factor for per-unit calibration

---

## Version 1.3.0 — Highlights

- ML-Enhanced Dual-Mode Calibration
  - `MODE_STATIC` — Quadratic model for lab/beaker measurements (R² = 0.9987)
  - `MODE_INLINE` — Flow compensation quadratic model for pump/pipe systems (R² = 0.9993)
- DSP Noise Rejection — Median filter to reject transient bubble spikes
- Dual Output — Reports both TDS (ppm) and EC (µS/cm)
- Temperature Compensation — Normalizes readings to 25°C automatically

---

## Table of Contents

- Overview
- Features
- Hardware & Wiring
- Quick Start (example)
- API Reference (common functions)
- Calibration & The K‑Factor (why it matters and how to compute)
- Accuracy & Benchmarks
- Troubleshooting & FAQ
- Changelog
- License & Contribution

---

## Features

- Dual-mode calibration (static / inline)
- ML-trained quadratic models to map ADC → EC/TDS
- Per-device K‑Factor (linear multiplier) for hardware-specific tuning
- Median filter + configurable sampling for robust measurements
- ADS1115 support (16-bit ADC over I²C)
- DS18B20 1‑Wire temperature sensor support
- Works with both ESP32 and Arduino-compatible boards

---

## Hardware Architecture

Recommended components:
1. ESP32 Development Board (e.g., DOIT DEVKIT V1)
2. ADS1115 (16-bit Sigma-Delta ADC, I²C)
3. DS18B20 waterproof temperature sensor (1‑Wire)
4. Analog TDS probe (Gravity-style analog TDS sensor)
5. Passive parts:
   - 1 × 4.7 kΩ resistor (DS18B20 pull-up)
   - 2 × 4.7 kΩ resistors (I²C pull-ups if ADS1115 board doesn't include them)

Notes:
- ADS1115 ADDR tied to GND sets the I²C address to 0x48.
- Use common ground between ESP32, ADS1115, and probe.
- Power ADS1115 at 3.3V for logic compatibility; 5V can increase ADC dynamic range but requires proper level handling for ESP32 pins.

---

## Connection / Pinout

ADS1115 (I²C) wiring:

| ADS1115 Pin | ESP32 Pin | Function | Notes |
|-------------|-----------|----------|-------|
| VCC         | 3.3V / 5V | Power    | 5V recommended for dynamic range (use care) |
| GND         | GND       | Ground   | Common ground required |
| SCL         | GPIO 22   | I²C SCL  | ESP32 default |
| SDA         | GPIO 21   | I²C SDA  | ESP32 default |
| ADDR        | GND       | Address  | Sets address to 0x48 |
| A0          | GPIO 27*  | TDS analog input via ADS1115 channel | Connect probe analog output to A0 of ADS1115 |

DS18B20 wiring:

| Wire | ESP32 Pin | Notes |
|------|-----------|-------|
| Red  | 3.3V / 5V | VCC |
| Black| GND       | Ground |
| Yellow| GPIO 27  | 1‑Wire Data (example) — needs 4.7 kΩ pull-up to VCC |

(You can use a different pin for DS18B20; this README uses GPIO 27 as an example and assumes ADS1115 analog input connected accordingly.)

ASCII wiring:

```text
       [ESP32]                       [ADS1115]
      +-------+                     +---------+
      |    3V3|-------------------->| VCC     |
      |    GND|-------------------->| GND     |
      |     22|-------------------->| SCL     |
      |     21|-------------------->| SDA     |
      |     27|<-----+              | A0 <--------- [TDS Signal]
      +-------+      |              +---------+
                     |
        [DS18B20]    |
       (Data Pin)----+
                     |
                    [R] 4.7kΩ Pull-up
                     |
                   [VCC]
```

---

## Quick Start (Example)

Install the library into Arduino IDE / PlatformIO as usual, then use:

```cpp
#include <EvaraTDS.h>

EvaraTDS tds;

void setup() {
  Serial.begin(115200);
  tds.begin();                 // initialize sensors and ADC
  tds.setMode(MODE_INLINE);    // MODE_INLINE or MODE_STATIC
  tds.setTDSFactor(0.5);       // 0.5 for NaCl, 0.7 for some fertilizers/other solutions
  tds.setKFactor(1.0);         // default (no scaling). Compute and set per Calibration section
}

void loop() {
  tds.update();                // sample & process (may be blocking briefly)
  float ec_uS_per_cm = tds.getEC();  // microSiemens per cm
  float tds_ppm = tds.getTDS();      // ppm (TDS)
  float temp_c = tds.getTemperature(); // °C

  Serial.printf("T=%.2f °C, EC=%.1f µS/cm, TDS=%.1f ppm\n", temp_c, ec_uS_per_cm, tds_ppm);
  delay(1000);
}
```

Notes:
- `update()` triggers a new measurement cycle and applies filtering, compensation and K-factor.
- `getEC()` and `getTDS()` return compensated and calibrated values.

---

## API Reference (common functions)

- `tds.begin()` — Initialize ADC, temperature sensor, and internal state.
- `tds.update()` — Acquire samples, run filtering and model, update cached values.
- `tds.getEC()` — Returns EC in µS/cm (temperature compensated and scaled).
- `tds.getTDS()` — Returns TDS in ppm (EC × TDS factor × K-factor).
- `tds.getTemperature()` — Returns measured temperature in °C (from DS18B20).
- `tds.setMode(mode)` — `MODE_STATIC` or `MODE_INLINE`. Select calibration model.
- `tds.setTDSFactor(factor)` — Sets conversion factor from EC → TDS (typical 0.5 for NaCl).
- `tds.setKFactor(k)` — Applies linear multiplier to final reported values for per-unit calibration.
- `tds.setMedianFilter(window)` — Configure median filter window length (if exposed).
- `tds.setSampleInterval(ms)` — Set sampling interval (if exposed).

(Exact function names may vary slightly depending on library version; check the library header for exact signatures.)

---

## Calibration & The K‑Factor

Even with ML-calibrated models and careful electronics, every physical installation can have small systematic differences. The K‑Factor is a simple linear multiplier applied to the final TDS/EC result to align your unit with a traceable reference.

Why the K-Factor is important:
- Probe aging / electrode surface changes alter sensitivity over time.
- Cable length and connector resistance add series impedance.
- Resistors and PCB tolerances (100 Ω / 1 kΩ etc.) create small systematic offsets.
- Manufacturing variance from probe or ADC front-end.
- Environmental differences: flow turbulence, air bubbles, partial immersion, fouling.

K‑Factor definition:

K = ReferenceValue / EvaraTDSReading

Where:
- ReferenceValue: reading from a calibrated commercial TDS/EC meter or a known calibration solution (ppm or µS/cm).
- EvaraTDSReading: current value reported by the EvaraTDS unit (same units).

Example:
- Calibration solution: 500 ppm
- EvaraTDS reads: 480 ppm
- K = 500 / 480 = 1.0417
- Set `tds.setKFactor(1.0417);` — future readings scaled up by ~4.17%

Recommended calibration procedure:
1. Prepare a stable calibration solution with known conductivity/TDS at the expected measurement temperature (or note the solution temperature).
2. Let the probe stabilize in the solution; ensure no bubbles and gentle stirring if static.
3. Take multiple readings from EvaraTDS (average of N samples recommended).
4. Read the same solution with a reference calibrated meter (commercial pen).
5. Compute K = Reference / EvaraReading.
6. Set the library K-factor: `tds.setKFactor(K)`.
7. Re-check across 2–3 known solutions (if available) to ensure linear scaling is valid across your expected range.

Important notes:
- K-factor is a single linear multiplier; it corrects multiplicative errors but not complex nonlinear behavior.
- If discrepancies vary with range, you may need to re-evaluate probe conditioning, wiring, or consider custom calibration curves.

---

## Temperature Compensation

EvaraTDS normalizes EC to 25°C by applying a temperature coefficient (α). Typical default α ≈ 0.02 (2% per °C) for many aqueous solutions; library applies:

EC25 = ECraw / (1 + α * (T - 25))

Final TDS = EC25 × TDSFactor × K

You can tune the coefficient if you know your solution's specific temperature coefficient.

---

## Accuracy & Benchmarks (v1.3.0)

Measured vs. ground truth (representative results):

| Mode      | RMSE (ppm) | R²     | Model Type         |
|-----------|------------|--------|--------------------|
| INLINE    | 5.61       | 0.9993 | Quadratic (Poly-2) |
| STATIC    | 7.36       | 0.9987 | Quadratic (Poly-2) |

- Inline model explicitly compensates for signal loss due to flow and micro-bubble formation in closed-loop pipe systems.
- Benchmarks collected using ADS1115 (configured for best dynamic range), DS18B20 for temperature, and traceable calibration solutions.

---

## Troubleshooting & FAQ

Q: Reading jumps or shows spikes
- Ensure median filter is enabled. Reduce physical bubble formation; orientation and probe depths matter.
- Increase median filter window or sampling rate.
- Check wiring for loose connections and shared noisy power rails.

Q: Readings are off by a constant percentage
- Compute and apply a K-factor as described in Calibration section.

Q: Readings change with cable length
- Use shielded cable and keep wiring short. Account for additional series resistance with K-factor calibration.

Q: Temperature compensation seems incorrect
- Confirm DS18B20 wiring and that measured temperature matches ambient/reference. Adjust temperature coefficient if required.

---

## Best Practices

- Rinse and condition new probes per the manufacturer's instructions prior to calibration.
- Perform calibration periodically (e.g., monthly) or after probe cleaning/maintenance.
- For inline installations, flush lines before measuring and reduce air entrainment to minimize bubble artifacts.
- Use an appropriate TDSFactor for your expected solution chemistry (typical: 0.5 for NaCl; 0.67 for certain mineral mixes; 0.7 for many hydroponic nutrient mixes).

---

## Changelog (selected)

- v1.3.0 — ML-Enhanced Dual-Mode Calibration, improved inline flow compensation (R²=0.9993), median filter improvements, documentation updates.
- v1.2.0 — Initial ML inline model, DSP noise filtering, temperature compensation.

(See repository tags for a full changelog.)

---

## Contribution

Contributions, issues and feature requests are welcome. Please open an issue or a pull request in this repository. If contributing code, follow the existing style and add tests where appropriate.

---

## License

MIT License — see LICENSE file.

---

If you need a trimmed quick-start README, wiring diagram image files, or a script to compute K-factor and log calibration sessions (CSV exporter), tell me which one and I will add it.
```
