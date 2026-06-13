/**
 * @file EvaraTDS.h
 * @brief Industrial TDS Calibration & Math Engine
 * @version 1.3.1
 * @author EvaraTech Engineering
 *
 * Changelog v1.3.1:
 *  - FIX #1: Resolved all git merge conflicts (kept v1.3.0 logic throughout)
 *  - FIX #2: Buffer pre-filled with first real reading to eliminate cold-start zero bias
 *  - FIX #3: BUFFER_SIZE increased from 10 to 15 to match firmware window
 *  - FIX #4: getVoltage() renamed to getCompVoltage(); getRawVoltage() added for true raw ADC value
 *  - FIX #5: Input validation added in update() — voltage and temp clamped to safe ranges
 *  - FIX #6: begin() no longer resets _currentMode — mode survives soft resets
 */

#ifndef EVARATDS_H
#define EVARATDS_H

#include <Arduino.h>

// Professional Calibration Modes
enum TDSMode {
    MODE_STATIC, // Lab/Bottle Measurement (High Sensitivity Model)
    MODE_INLINE  // Pump Loop Measurement (Flow Compensated Model)
};

class EvaraTDS {
  public:
    EvaraTDS();

    /**
     * @brief Initialize the library. Safe to call multiple times.
     * NOTE: Does NOT reset TDSMode — set your mode after begin() and it will survive re-init.
     */
    void begin();

    // --- Physics Mode Switch ---
    // Set to MODE_INLINE for pipe assemblies to apply ML Flow Correction.
    void setMode(TDSMode mode);

    /**
     * @brief Main DSP update loop. Call this before getting readings.
     * Incorporates Median Filtering to reject noise.
     * Input validation: voltage clamped to [0.0, 5.0]V; temp clamped to [-10, 100]C.
     * @param voltage_volts Raw voltage from ADS1115 or Analog Pin
     * @param temp_c Current temperature in Celsius
     */
    void update(float voltage_volts, float temp_c);

    // --- Getters ---
    float getTDS();           // ppm — final K-factor scaled reading
    float getEC();            // uS/cm
    float getCompVoltage();   // Smoothed, Temperature-Compensated voltage (for diagnostics)
    float getRawVoltage();    // Raw median-filtered voltage BEFORE temperature compensation

    // Fine-Tuning Settings
    /**
     * @brief Set the TDS Conversion Factor.
     * 0.5 = USA/NaCl (Default)
     * 0.7 = Europe/Hydroponics (442)
     */
    void setTDSFactor(float factor);

    /**
     * @brief Set Temperature Compensation Coefficient.
     * Default: 0.02 (2.0% per degree C)
     */
    void setTempCoefficient(float coeff);

    /**
     * @brief Set a manual K-factor tuning multiplier (Default 1.0)
     */
    void setKFactor(float k);

  private:
    TDSMode _currentMode = MODE_STATIC;
    float _kFactor       = 1.0f;
    float _tdsFactor     = 0.5f;
    float _tempCoeff     = 0.02f;

    // DSP Buffers — size 15 to match firmware median window
    static const int BUFFER_SIZE = 15;
    float _analogBuffer[BUFFER_SIZE];
    int   _bufferIndex  = 0;
    bool  _bufferSeeded = false; // tracks cold-start state

    // Outputs
    float _finalTDS     = 0.0f;
    float _finalEC      = 0.0f;
    float _compVolts    = 0.0f; // post-compensation (for getCompVoltage)
    float _rawVolts     = 0.0f; // pre-compensation  (for getRawVoltage)

    // Internal Math Kernels
    float getMedian(float* array, int size);
    float computePhysics(float voltage); // v1.3.0 two-stage ML regression
};

#endif