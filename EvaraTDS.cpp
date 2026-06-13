/**
 * @file EvaraTDS.cpp
 * @brief Implementation of EvaraTDS Math Engine v1.4.0
 * @details Direct Voltage->PPM Quadratic Regression.
 *          STATIC: 11.91*V^2 + 398.26*V + 6.28
 *          INLINE:  9.36*V^2 + 463.50*V + 9.84
 *          Coefficients from Least Squares fit on STATIC.csv / INLINE.csv datasets.
 *
 * All v1.3.1 quality fixes retained (see EvaraTDS.h changelog).
 */

#include "EvaraTDS.h"

// ---------------------------------------------------------------------------
// Constructor
// FIX #2: Buffer NOT zeroed here -- cold-start seeding done on first update()
// ---------------------------------------------------------------------------
EvaraTDS::EvaraTDS() {
    _bufferSeeded = false;
    _bufferIndex  = 0;
}

// ---------------------------------------------------------------------------
// begin()
// FIX #6: Does NOT touch _currentMode -- mode survives soft-resets / re-init.
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
    if (voltage < 0.0f)  voltage = 0.0f;
    if (voltage > 5.0f)  voltage = 5.0f;
    if (temp   < -10.0f) temp    = -10.0f;
    if (temp   > 100.0f) temp    = 100.0f;

    // --- FIX #2: Cold-Start Buffer Seeding ---
    // First real voltage pre-fills all 15 slots -- no zero-bias on boot.
    if (!_bufferSeeded) {
        for (int i = 0; i < BUFFER_SIZE; i++) _analogBuffer[i] = voltage;
        _bufferSeeded = true;
        _bufferIndex  = 0;
    }

    // --- STAGE 1: Data Ingestion ---
    _analogBuffer[_bufferIndex] = voltage;
    _bufferIndex++;
    if (_bufferIndex >= BUFFER_SIZE) _bufferIndex = 0;

    // --- STAGE 2: DSP -- Noise Rejection ---
    // 15-sample median filter rejects micro-bubble spikes and pump transients
    float cleanVoltage = getMedian(_analogBuffer, BUFFER_SIZE);

    // --- FIX #4: Store raw (pre-compensation) voltage ---
    _rawVolts = cleanVoltage;

    // --- STAGE 3: Temperature Compensation to 25.0 C standard ---
    // Pass actual temp for compensated reading.
    // Pass 25.0 to get uncompensated reference reading.
    float compFactor  = 1.0f + _tempCoeff * (temp - 25.0f);
    float compVoltage = cleanVoltage / compFactor;

    // Store compensated voltage (FIX #4)
    _compVolts = compVoltage;

    // --- STAGE 4: v1.4.0 Direct ML Model Inference ---
    float calculatedTDS = computeDirectPhysics(compVoltage);

    // --- STAGE 5: Final Output Scaling ---
    _finalTDS = calculatedTDS * _kFactor;

    if (_tdsFactor > 0) _finalEC = _finalTDS / _tdsFactor;
    else                _finalEC = 0.0f;
}

// ---------------------------------------------------------------------------
// computeDirectPhysics() -- v1.4.0 Direct Voltage -> PPM Model
//
// Single-stage quadratic regression fitted directly on calibration datasets.
// Coefficients derived via Least Squares Regression on STATIC.csv / INLINE.csv.
// Input: temp-compensated voltage (V)
// ---------------------------------------------------------------------------
float EvaraTDS::computeDirectPhysics(float v) {
    // Deadzone: air / dry probe check
    if (v < 0.02f) return 0.0f;

    float realPPM = 0.0f;

    if (_currentMode == MODE_STATIC) {
        // [MODEL A] Static Calibration -- v1.4.0
        // Data Source: STATIC.csv
        // Formula: 11.91*V^2 + 398.26*V + 6.28
        realPPM = (11.91f * v * v) + (398.26f * v) + 6.28f;
    } else {
        // [MODEL B] Inline Calibration -- v1.4.0
        // Data Source: INLINE.csv
        // Formula: 9.36*V^2 + 463.50*V + 9.84
        realPPM = (9.36f * v * v) + (463.50f * v) + 9.84f;
    }

    // Safety clamp
    return (realPPM < 0.0f) ? 0.0f : realPPM;
}

// ---------------------------------------------------------------------------
// getMedian() -- Bubble Sort Median Filter
// Works on a defensive copy -- original ring buffer is never modified.
// ---------------------------------------------------------------------------
float EvaraTDS::getMedian(float* array, int size) {
    float bCopy[BUFFER_SIZE];
    for (int i = 0; i < size; i++) bCopy[i] = array[i];

    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (bCopy[j] > bCopy[j + 1]) {
                float tmp    = bCopy[j];
                bCopy[j]     = bCopy[j + 1];
                bCopy[j + 1] = tmp;
            }
        }
    }

    if (size % 2 == 0) return (bCopy[size / 2 - 1] + bCopy[size / 2]) / 2.0f;
    else               return bCopy[size / 2];
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------
float EvaraTDS::getTDS()         { return _finalTDS;  }
float EvaraTDS::getEC()          { return _finalEC;   }
float EvaraTDS::getRawVoltage()  { return _rawVolts;  } // pre  temp-compensation
float EvaraTDS::getCompVoltage() { return _compVolts; } // post temp-compensation
