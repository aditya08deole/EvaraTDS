/**
 * @file EvaraTDS.cpp
 * @brief Implementation of EvaraTDS Math Engine v1.3.0
 * @details Implements ML-based Quadratic Regression for Static/Inline compensation.
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
    float compFactor = 1.0f + _tempCoeff * (temp - 25.0f);
    float compVoltage = cleanVoltage / compFactor;
    
    _smoothedVolts = compVoltage; 

    // --- STAGE 4: ML Model Inference ---
    // Calculate final TDS using the specific regression model
    float rawTDS = computePhysics(compVoltage);
    
    // --- STAGE 5: Final Output Scaling ---
    _finalTDS = rawTDS * _kFactor;
    
    // Calculate Electrical Conductivity (EC)
    if (_tdsFactor > 0) _finalEC = _finalTDS / _tdsFactor;
    else _finalEC = 0;
}

float EvaraTDS::computePhysics(float v) {
    // Deadzone (Air/Dry Probe Check)
    if (v < 0.02) return 0.0;

    // STEP 1: Calculate "Sensor PPM" (Base Reading)
    // This represents the uncorrected standard curve used during data collection.
    // Base Formula: (113.4*v^2) + (425.8*v) + 0.2
    float sensorPPM = (113.4f * v * v) + (425.8f * v) + 0.2f;

    float realPPM = 0.0;

    // STEP 2: Apply ML Correction (v1.3.0 Calibration)
    if (_currentMode == MODE_STATIC) {
        // [MODEL A] Static Calibration (v1.3.0)
        // R2 = 0.9987 | RMSE = 7.36 ppm
        // Formula: -12.4258 + (1.1965 * Sensor) + (-0.0001 * Sensor^2)
        realPPM = -12.4258f + (1.1965f * sensorPPM) + (-0.0001f * sensorPPM * sensorPPM);
    } 
    else {
        // [MODEL B] Inline Calibration (v1.3.0)
        // R2 = 0.9993 | RMSE = 5.61 ppm
        // Formula: -3.7242 + (1.3053 * Sensor) + (0.0001 * Sensor^2)
        realPPM = -3.7242f + (1.3053f * sensorPPM) + (0.0001f * sensorPPM * sensorPPM);
    }

    // Safety Clamp: Prevent negative readings from regression intercept
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
