/**
 * @file EvaraTDS.cpp
 * @brief Implementation of EvaraTDS Math Engine v1.3.1
 * @details Implements ML-based Quadratic Regression for Static/Inline compensation.
 *
 * Changelog v1.3.1 Fixes Applied:
 *  FIX #1 -- Resolved all git merge conflicts (v1.3.0 logic retained throughout)
 *  FIX #2 -- Buffer cold-start: first valid voltage pre-seeds all slots to eliminate zero-bias
 *  FIX #3 -- BUFFER_SIZE: 10 -> 15 (defined in header, matches firmware window)
 *  FIX #4 -- _rawVolts captured BEFORE temp compensation; getCompVoltage()/getRawVoltage() split
 *  FIX #5 -- Input validation: voltage clamped [0.0, 5.0]V; temp clamped [-10.0, 100.0]C
 *  FIX #6 -- begin() no longer resets _currentMode; only resets buffer state
 */

#include "EvaraTDS.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
EvaraTDS::EvaraTDS() {
    // Buffer left uninitialised intentionally -- _bufferSeeded flag handles
    // cold-start seeding on first real update() call (FIX #2)
    _bufferSeeded = false;
    _bufferIndex  = 0;
}

// ---------------------------------------------------------------------------
// begin()
// FIX #6: Does NOT touch _currentMode. Mode survives soft-resets / re-init.
// ---------------------------------------------------------------------------
void EvaraTDS::begin() {
    _bufferIndex  = 0;
    _bufferSeeded = false;
    // _currentMode intentionally NOT reset here
}

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------
void EvaraTDS::setMode(TDSMode mode)           { _currentMode = mode; }
void EvaraTDS::setTDSFactor(float factor)      { _tdsFactor   = factor; }
void EvaraTDS::setKFactor(float k)             { _kFactor     = k; }
void EvaraTDS::setTempCoefficient(float coeff) { _tempCoeff   = coeff; }

// ---------------------------------------------------------------------------
// update()
// ---------------------------------------------------------------------------
void EvaraTDS::update(float voltage, float temp) {

    // --- FIX #5: Input Validation ---
    // Clamp physically impossible inputs rather than silently compute garbage.
    if (voltage < 0.0f)  voltage = 0.0f;
    if (voltage > 5.0f)  voltage = 5.0f;
    if (temp   < -10.0f) temp    = -10.0f;
    if (temp   > 100.0f) temp    = 100.0f;

    // --- FIX #2: Cold-Start Buffer Seeding ---
    // On the very first call, pre-fill every slot with this voltage so the
    // median is representative from sample #1 (no zero-pulling bias).
    if (!_bufferSeeded) {
        for (int i = 0; i < BUFFER_SIZE; i++) _analogBuffer[i] = voltage;
        _bufferSeeded = true;
        _bufferIndex  = 0;
    }

    // --- STAGE 1: Data Ingestion ---
    // Insert new reading into circular ring buffer (BUFFER_SIZE = 15, FIX #3)
    _analogBuffer[_bufferIndex] = voltage;
    _bufferIndex++;
    if (_bufferIndex >= BUFFER_SIZE) _bufferIndex = 0;

    // --- STAGE 2: DSP -- Noise Rejection ---
    // Median filter rejects micro-bubble spikes and pump transients
    float cleanVoltage = getMedian(_analogBuffer, BUFFER_SIZE);

    // --- FIX #4: Capture raw (pre-compensation) voltage ---
    _rawVolts = cleanVoltage;

    // --- STAGE 3: Physics Normalization ---
    // Temperature compensation to 25.0 C reference standard.
    // When temp == 25.0, compFactor == 1.0 -> no change (uncompensated path).
    float compFactor  = 1.0f + _tempCoeff * (temp - 25.0f);
    float compVoltage = cleanVoltage / compFactor;

    // Store compensated voltage for getCompVoltage() (FIX #4)
    _compVolts = compVoltage;

    // --- STAGE 4: ML Model Inference ---
    // Two-stage regression: voltage -> sensorPPM -> corrected realPPM
    float rawTDS = computePhysics(compVoltage);

    // --- STAGE 5: Final Output Scaling ---
    _finalTDS = rawTDS * _kFactor;

    // Electrical Conductivity
    if (_tdsFactor > 0) _finalEC = _finalTDS / _tdsFactor;
    else                _finalEC = 0.0f;
}

// ---------------------------------------------------------------------------
// computePhysics() -- v1.3.0 Two-Stage ML Regression
// FIX #1: computePoly() (v1.2.0) removed. computePhysics() is the sole model.
// ---------------------------------------------------------------------------
float EvaraTDS::computePhysics(float v) {
    // Deadzone: air / dry probe check
    if (v < 0.02f) return 0.0f;

    // STEP 1: Base Sensor PPM (uncorrected standard curve from calibration data)
    // Formula: (113.4 * V^2) + (425.8 * V) + 0.2
    float sensorPPM = (113.4f * v * v) + (425.8f * v) + 0.2f;

    float realPPM = 0.0f;

    // STEP 2: Mode-specific ML correction applied in the PPM domain
    if (_currentMode == MODE_STATIC) {
        // [MODEL A] Static Calibration -- v1.3.0
        // R2 = 0.9987 | RMSE = 7.36 ppm
        // Formula: -12.4258 + (1.1965 * S) + (-0.0001 * S^2)
        realPPM = -12.4258f
                + ( 1.1965f * sensorPPM)
                + (-0.0001f * sensorPPM * sensorPPM);
    } else {
        // [MODEL B] Inline Calibration -- v1.3.0
        // R2 = 0.9993 | RMSE = 5.61 ppm
        // Formula: -3.7242 + (1.3053 * S) + (0.0001 * S^2)
        realPPM = -3.7242f
                + (1.3053f * sensorPPM)
                + (0.0001f * sensorPPM * sensorPPM);
    }

    // Safety clamp: prevent negative readings from regression intercept
    return (realPPM < 0.0f) ? 0.0f : realPPM;
}

// ---------------------------------------------------------------------------
// getMedian() -- Bubble Sort Median Filter
// Operates on a defensive copy to protect the live ring buffer.
// ---------------------------------------------------------------------------
float EvaraTDS::getMedian(float* array, int size) {
    float bCopy[BUFFER_SIZE];
    for (int i = 0; i < size; i++) bCopy[i] = array[i];

    // Bubble sort ascending
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (bCopy[j] > bCopy[j + 1]) {
                float tmp    = bCopy[j];
                bCopy[j]     = bCopy[j + 1];
                bCopy[j + 1] = tmp;
            }
        }
    }

    // Return true median (average two middle values for even-sized buffer)
    if (size % 2 == 0) return (bCopy[size / 2 - 1] + bCopy[size / 2]) / 2.0f;
    else               return bCopy[size / 2];
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------
float EvaraTDS::getTDS()         { return _finalTDS;  }
float EvaraTDS::getEC()          { return _finalEC;   }
float EvaraTDS::getCompVoltage() { return _compVolts; } // post temp-compensation (FIX #4)
float EvaraTDS::getRawVoltage()  { return _rawVolts;  } // pre  temp-compensation (FIX #4)
