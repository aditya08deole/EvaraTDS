/**
 * @file EvaraTDS.cpp
 * @brief Implementation of EvaraTDS Math Engine v1.4.0
 * @details Implements Direct Voltage->PPM Quadratic Regression.
 */

#include "EvaraTDS.h"

EvaraTDS::EvaraTDS() {
    // Clear buffer on boot
    for(int i=0; i<BUFFER_SIZE; i++) _analogBuffer[i] = 0.0;
}

void EvaraTDS::begin() {
    _bufferIndex = 0;
    _currentMode = MODE_STATIC; // Default to standard lab mode
}

void EvaraTDS::setMode(TDSMode mode) {
    _currentMode = mode;
}

void EvaraTDS::setTDSFactor(float factor) { _tdsFactor = factor; }
void EvaraTDS::setKFactor(float k) { _kFactor = k; }
void EvaraTDS::setTempCoefficient(float coeff) { _tempCoeff = coeff; }

void EvaraTDS::update(float voltage, float temp) {
    // --- STAGE 1: Data Ingestion ---
    // Add new reading to circular buffer
    _analogBuffer[_bufferIndex] = voltage;
    _bufferIndex++;
    if(_bufferIndex >= BUFFER_SIZE) _bufferIndex = 0;

    // --- STAGE 2: DSP (Noise Rejection) ---
    // Apply Median Filter to reject micro-bubbles and pump noise
    float cleanVoltage = getMedian(_analogBuffer, BUFFER_SIZE);
    
    // --- STAGE 3: Physics Normalization ---
    // Temperature Compensation to standard 25.0°C
    // We normalize voltage first, as the ML model is trained on standard temp behavior.
    float compFactor = 1.0f + _tempCoeff * (temp - 25.0f);
    float compVoltage = cleanVoltage / compFactor;
    
    _smoothedVolts = compVoltage; 

    // --- STAGE 4: Direct ML Model Inference (v1.4.0) ---
    float calculatedTDS = computeDirectPhysics(compVoltage);
    
    // --- STAGE 5: Final Output Scaling ---
    _finalTDS = calculatedTDS * _kFactor;
    
    // Calculate Electrical Conductivity (EC)
    if (_tdsFactor > 0) _finalEC = _finalTDS / _tdsFactor;
    else _finalEC = 0;
}

float EvaraTDS::computeDirectPhysics(float v) {
    // Deadzone (Air/Dry Probe Check)
    if (v < 0.02) return 0.0;

    float realPPM = 0.0;

    // Direct Voltage Mapping derived from INLINE.csv and STATIC.csv
    // Coefficients calculated via Least Squares Regression on provided datasets.
    // Models adjusted for Temp-Compensated Voltage (25C).

    if (_currentMode == MODE_STATIC) {
        // [MODEL A] Static Calibration (v1.4.0)
        // Data Source: STATIC.csv
        // Trend: Slightly steeper curve in lower voltages
        // Formula: 11.91*V^2 + 398.26*V + 6.28
        realPPM = (11.91f * v * v) + (398.26f * v) + 6.28f;
    } 
    else {
        // [MODEL B] Inline Calibration (v1.4.0)
        // Data Source: INLINE.csv
        // Trend: Compensates for flow dynamics where sensitivity shifts
        // Formula: 9.36*V^2 + 463.50*V + 9.84
        realPPM = (9.36f * v * v) + (463.50f * v) + 9.84f;
    }

    // Safety Clamp
    return (realPPM < 0.0f) ? 0.0f : realPPM;
}

// Robust Bubble Sort Algorithm for Median Filter
float EvaraTDS::getMedian(float* array, int size) {
    float bCopy[BUFFER_SIZE];
    // Copy array to protect original buffer
    for (int i = 0; i < size; i++) bCopy[i] = array[i];
    
    // Sort
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (bCopy[j] > bCopy[j + 1]) {
                float temp = bCopy[j];
                bCopy[j] = bCopy[j + 1];
                bCopy[j + 1] = temp;
            }
        }
    }
    
    // Return median
    if (size % 2 == 0) return (bCopy[size / 2 - 1] + bCopy[size / 2]) / 2.0;
    else return bCopy[size / 2];
}

float EvaraTDS::getTDS() { return _finalTDS; }
float EvaraTDS::getEC() { return _finalEC; }
float EvaraTDS::getVoltage() { return _smoothedVolts; }
