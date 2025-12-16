/**
 * @file EvaraTDS.cpp
 * @brief Implementation of EvaraTDS Math Engine v1.1.0
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

EvaraTDS::EvaraTDS() {
    for(int i=0; i<BUFFER_SIZE; i++) _analogBuffer[i] = 0.0;
}

void EvaraTDS::begin() {
    _bufferIndex = 0;
}

void EvaraTDS::setTDSFactor(float factor) { _tdsFactor = factor; }
void EvaraTDS::setTempCoefficient(float coeff) { _tempCoeff = coeff; }
void EvaraTDS::setKFactor(float k) { _kFactor = k; }

void EvaraTDS::update(float voltage, float temp) {
    // 1. Fill Buffer (Circular)
    _analogBuffer[_bufferIndex] = voltage;
    _bufferIndex++;
    if(_bufferIndex >= BUFFER_SIZE) _bufferIndex = 0;

    // 2. Median Filtering (Noise Rejection)
    float cleanVoltage = getMedian(_analogBuffer, BUFFER_SIZE);
    
    // 3. Temp Compensation
    float compFactor = 1.0f + _tempCoeff * (temp - 25.0f);
    float compVoltage = cleanVoltage / compFactor;
    
    _smoothedVolts = compVoltage; // For debugging

    // 4. Calculate TDS using Physics Model
    float rawTDS = computePoly(compVoltage);
    
    // 5. Final Outputs
    _finalTDS = rawTDS * _kFactor;
    
    // EC is derived: TDS = EC * Factor  ->  EC = TDS / Factor
    if (_tdsFactor > 0) {
        _finalEC = _finalTDS / _tdsFactor;
    } else {
        _finalEC = 0;
    }
}

float EvaraTDS::getMedian(float* array, int size) {
    // Create a temporary copy to sort
    float bCopy[BUFFER_SIZE];
    for (int i = 0; i < size; i++) bCopy[i] = array[i];

    // Simple Bubble Sort
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (bCopy[j] > bCopy[j + 1]) {
                float temp = bCopy[j];
                bCopy[j] = bCopy[j + 1];
                bCopy[j + 1] = temp;
            }
        }
    }
    // Return median (middle element)
    if (size % 2 == 0)
        return (bCopy[size / 2 - 1] + bCopy[size / 2]) / 2.0;
    else
        return bCopy[size / 2];
}

float EvaraTDS::computePoly(float voltage) {
    // Deadzone
    if (voltage <= CAL_TABLE[0].v) return 0.0f;

    // Extrapolation (High End)
    if (voltage >= CAL_TABLE[CAL_TABLE_SIZE - 1].v) {
        float m = (CAL_TABLE[CAL_TABLE_SIZE - 1].ppm - CAL_TABLE[CAL_TABLE_SIZE - 2].ppm) / 
                  (CAL_TABLE[CAL_TABLE_SIZE - 1].v - CAL_TABLE[CAL_TABLE_SIZE - 2].v);
        return m * (voltage - CAL_TABLE[CAL_TABLE_SIZE - 1].v) + CAL_TABLE[CAL_TABLE_SIZE - 1].ppm;
    }

    // Interpolation (Standard Range)
    for (size_t i = 0; i < CAL_TABLE_SIZE - 1; i++) {
        if (voltage >= CAL_TABLE[i].v && voltage < CAL_TABLE[i+1].v) {
            float slope = (CAL_TABLE[i+1].ppm - CAL_TABLE[i].ppm) / 
                          (CAL_TABLE[i+1].v - CAL_TABLE[i].v);
            return CAL_TABLE[i].ppm + (slope * (voltage - CAL_TABLE[i].v));
        }
    }
    return 0.0f;
}

float EvaraTDS::getTDS() { return _finalTDS; }
float EvaraTDS::getEC() { return _finalEC; }
float EvaraTDS::getVoltage() { return _smoothedVolts; }