/**
 * @file EvaraTDS.cpp
 * @brief Implementation of EvaraTDS Math Engine v1.2.0
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
    // Temperature Compensation to standard 25.0�C
    float compFactor = 1.0f + _tempCoeff * (temp - 25.0f);
    float compVoltage = cleanVoltage / compFactor;
    
    _smoothedVolts = compVoltage; // Store for debugging

    // --- STAGE 4: Model Inference ---
    // Select the correct physics model based on the mode
    float rawTDS = computePoly(compVoltage);
    
    // --- STAGE 5: Final Output Scaling ---
    _finalTDS = rawTDS * _kFactor;
    
    // Calculate Electrical Conductivity (EC)
    if (_tdsFactor > 0) _finalEC = _finalTDS / _tdsFactor;
    else _finalEC = 0;
}

float EvaraTDS::computePoly(float v) {
    // Deadzone (Air/Dry Probe Check)
    if (v < 0.02) return 0.0;

    if (_currentMode == MODE_STATIC) {
        // [MODEL A] Static Water (Bottle/Beaker)
        // Original Calibration | R2 = 0.988
        // Optimized for high sensitivity in still water
        return (113.4f * v * v) + (425.8f * v) + 0.2f;
    } 
    else {
        // [MODEL B] Dynamic Flow (Inline Pipe Loop)
        // New ML Calibration | R2 = 0.9999
        // Derived from 55 data points (0-1000ppm)
        // Compensates for hydrodynamic signal attenuation (~38% loss)
        // Formula: 18.05*V^2 + 589.31*V - 3.55
        float tds = (18.05f * v * v) + (589.31f * v) - 3.55f;
        
        // Clamp negative noise near zero
        return (tds < 0.0f) ? 0.0f : tds;
    }
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
