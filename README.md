EvaraTDS: Industrial TDS & Conductivity Library

Overview

The EvaraTDS library is a high-precision, industrial-grade firmware module designed for measuring Total Dissolved Solids (TDS) and Electrical Conductivity (EC).

It features a Dual-Mode Physics Engine that solves the common problem of signal attenuation in flowing water.

🚀 Key Features (v1.3.0)

ML-Enhanced Dual-Mode Calibration:

MODE_STATIC: Updated Quadratic Model ($R^2=0.9987$) for high-precision lab testing (Beaker/Bottle).

MODE_INLINE: Updated Flow Compensation Model ($R^2=0.9993$) that corrects signal attenuation caused by flow velocity and micro-bubbles in pipe loops.

DSP Noise Rejection: Integrated Median Filter to ignore bubble spikes.

Dual Output: Calculates both TDS (ppm) and EC (µS/cm).

Temperature Compensation: Automatic normalization to 25°C.

🔧 Usage

1. Basic Setup

#include <EvaraTDS.h>

EvaraTDS tds;

void setup() {
  tds.begin();
  
  // IMPORTANT: Select your environment
  tds.setMode(MODE_INLINE); // Uses new v1.3.0 Inline Calibration Model
  // tds.setMode(MODE_STATIC); // Uses new v1.3.0 Static Calibration Model
  
  tds.setTDSFactor(0.5); // 0.5 for NaCl (USA), 0.7 for Hydroponics
}


2. Accuracy Data (v1.3.0)

Mode

RMSE (ppm)

R² Score

Formula Type

Inline

5.61

0.9993

Quadratic (Poly-2)

Static

7.36

0.9987

Quadratic (Poly-2)

🎛️ Advanced Calibration: The K-Factor

While the EvaraTDS library includes highly accurate ML models ($R^2 > 0.99$), real-world hardware conditions can vary. The K-Factor is a linear multiplier used to fine-tune the final output to match a reference standard.

Why do I need it?

Even with a perfect software model, physical factors can introduce small offsets:

Probe Aging: As electrodes oxidize over months, resistance changes.

Cable Resistance: Long wires (>2m) add impedance.

Manufacturing Tolerance: Slight variations in the 100Ω or 1kΩ resistors on your PCB.

How to Calculate K

If you notice a consistent percentage error between your EvaraTDS reading and a commercial reference pen, calculate K as follows:

$$K = \frac{\text{Reference Value}}{\text{EvaraTDS Reading}}$$

Example Scenario

Situation: You are measuring a calibration solution known to be 500 ppm.

Reading: Your EvaraTDS system reads 480 ppm.

Calculation: $K = 500 / 480 = \mathbf{1.041}$

Implementation

Apply this factor in your setup():

void setup() {
    tds.begin();
    tds.setMode(MODE_INLINE);
    
    // Apply the correction calculated above
    tds.setKFactor(1.041); 
}


This will automatically scale all future readings by 4.1%, aligning your sensor perfectly with the reference.

Hardware Architecture

The system is optimized for the ESP32 platform while remaining compatible with standard Arduino environments. High-resolution analog acquisition is achieved using the ADS1115 (16-bit ADC), and temperature compensation is handled by the DS18B20 digital temperature sensor.

Component List

Microcontroller: ESP32 Development Board (e.g., DOIT DEVKIT V1)

ADC Module: ADS1115 (16-bit Sigma-Delta ADC, I²C)

Temperature Sensor: DS18B20 (Waterproof, 1-Wire)

TDS Probe: Analog TDS Sensor (Gravity Interface, K = 1.0)

Passive Components

1 × 4.7 kΩ resistor (DS18B20 pull-up)

2 × 4.7 kΩ resistors (I²C pull-ups if not present on ADS1115 module)

Connection Diagram

1. ADS1115 (ADC) Interfacing

The ADS1115 communicates over the I²C bus. The ADDR pin must be tied to GND to set the device address to 0x48.

ADS1115 Pin

ESP32 Pin

Function

Notes

VCC

3.3V / 5V

Power

5V recommended for improved dynamic range

GND

GND

Ground

Common ground

SCL

GPIO 22

I²C Clock

ESP32 default

SDA

GPIO 21

I²C Data

ESP32 default

ADDR

GND

Address Select

Sets address to 0x48

A0

TDS Signal

Analog Input

Connect TDS analog output

2. DS18B20 (Temperature) Interfacing

A 4.7 kΩ pull-up resistor between the Data line and VCC is mandatory.

DS18B20 Wire

ESP32 Pin

Function

Notes

Red (VCC)

3.3V / 5V

Power



Black (GND)

GND

Ground



Yellow (Data)

GPIO 27

1-Wire Data

Requires pull-up

Wiring Schematic

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
                    [R] 4.7kΩ Pull-up
                     |
                   [VCC]
