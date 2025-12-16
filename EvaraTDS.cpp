/**
 * @file EvaraTDS.cpp
 * @brief Implementation of EvaraTDS Math Engine
 */

#include "EvaraTDS.h"

// ====================================================
// INTERNAL CALIBRATION TABLE (Generated via Regression)
// Range: 0 - 1000 PPM | Step: 20 PPM | Correlation: 98.8%
// ====================================================
static const CalPoint CAL_TABLE[] = {
    {0.0200f, 0.0f},   {0.0459f, 20.0f},  {0.0913f, 40.0f},  {0.1355f, 60.0f},
    {0.1789f, 80.0f},  {0.2213f, 100.0f}, {0.2629f, 120.0f}, {0.3037f, 140.0f},
    {0.3438f, 160.0f}, {0.3831f, 180.0f}, {0.4218f, 200.0f}, {0.4599f, 220.0f},
    {0.4973f, 240.0f}, {0.5341f, 260.0f}, {0.5704f, 280.0f}, {0.6062f, 300.0f},
    {0.6414f, 320.0f}, {0.6762f, 340.0f}, {0.7105f, 360.0f}, {0.7444f, 380.0f},
    {0.7778f, 400.0f}, {0.8108f, 420.0f}, {0.8434f, 440.0f}, {0.8756f, 460.0f},
    {0.9075f, 480.0f}, {0.9389f, 500.0f}, {0.9701f, 520.0f}, {1.0009f, 540.0f},
    {1.0314f, 560.0f}, {1.0615f, 580.0f}, {1.0914f, 600.0f}, {1.1209f, 620.0f},
    {1.1502f, 640.0f}, {1.1792f, 660.0f}, {1.2079f, 680.0f}, {1.2363f, 700.0f},
    {1.2645f, 720.0f}, {1.2925f, 740.0f}, {1.3202f, 760.0f}, {1.3476f, 780.0f},
    {1.3749f, 800.0f}, {1.4019f, 820.0f}, {1.4286f, 840.0f}, {1.4552f, 860.0f},
    {1.4816f, 880.0f}, {1.5077f, 900.0f}, {1.5337f, 920.0f}, {1.5594f, 940.0f},
    {1.5850f, 960.0f}, {1.6104f, 980.0f}, {1.6355f, 1000.0f}
};
static const size_t CAL_TABLE_SIZE = sizeof(CAL_TABLE) / sizeof(CalPoint);

EvaraTDS::EvaraTDS() {}

void EvaraTDS::begin() {
    // Reserved for future init
}

void EvaraTDS::setKFactor(float k) {
    _kFactor = k;
}

float EvaraTDS::compensateTemperature(float voltage, float temp_c) {
    // Standard compensation: 2.0% per degree C from 25.0C standard
    float comp_factor = 1.0f + 0.02f * (temp_c - 25.0f);
    return voltage / comp_factor;
}

float EvaraTDS::computePoly(float voltage) {
    // 1. Deadzone check
    if (voltage <= CAL_TABLE[0].v) return 0.0f;

    // 2. Extrapolation Check (High Range)
    if (voltage >= CAL_TABLE[CAL_TABLE_SIZE - 1].v) {
        float m = (CAL_TABLE[CAL_TABLE_SIZE - 1].ppm - CAL_TABLE[CAL_TABLE_SIZE - 2].ppm) / 
                  (CAL_TABLE[CAL_TABLE_SIZE - 1].v - CAL_TABLE[CAL_TABLE_SIZE - 2].v);
        return m * (voltage - CAL_TABLE[CAL_TABLE_SIZE - 1].v) + CAL_TABLE[CAL_TABLE_SIZE - 1].ppm;
    }

    // 3. Interpolation Search
    // Since the table is sorted, we scan for the bracket
    for (size_t i = 0; i < CAL_TABLE_SIZE - 1; i++) {
        if (voltage >= CAL_TABLE[i].v && voltage < CAL_TABLE[i+1].v) {
            float slope = (CAL_TABLE[i+1].ppm - CAL_TABLE[i].ppm) / 
                          (CAL_TABLE[i+1].v - CAL_TABLE[i].v);
            float result = CAL_TABLE[i].ppm + (slope * (voltage - CAL_TABLE[i].v));
            return result;
        }
    }
    return 0.0f;
}

float EvaraTDS::getTDS(float voltage_volts, float temp_c) {
    float comp_v = compensateTemperature(voltage_volts, temp_c);
    float tds = computePoly(comp_v);
    return tds * _kFactor; // Apply final tuning factor
}