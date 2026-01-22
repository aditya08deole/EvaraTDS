/**
 * @file EvaraTDS.h
 * @brief Industrial TDS Calibration & Math Engine
 * @version 1.3.0
 * @author EvaraTech Engineering
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
    
    // Initialize the library
    void begin();

    // --- Physics Mode Switch ---
    // Set to MODE_INLINE for pipe assemblies to apply ML Flow Correction.
    void setMode(TDSMode mode);

    /**
     * @brief Main DSP update loop. Call this before getting readings.
     * Incorporates Median Filtering to reject noise.
     * @param voltage_volts Raw voltage from ADS1115 or Analog Pin
     * @param temp_c Current temperature in Celsius
     */
    void update(float voltage_volts, float temp_c);

    // Getters
    float getTDS();      // ppm
    float getEC();       // uS/cm
    float getVoltage();  // Smoothed Volts (Temperature Compensated)

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
    float _kFactor = 1.0;
    float _tdsFactor = 0.5;
    float _tempCoeff = 0.02;

    // DSP Buffers
    static const int BUFFER_SIZE = 10;
    float _analogBuffer[BUFFER_SIZE];
    int _bufferIndex = 0;
    
    // Outputs
    float _finalTDS = 0.0;
    float _finalEC = 0.0;
    float _smoothedVolts = 0.0;
    
    // Internal Math Kernels
    float getMedian(float* array, int size);
    float computePhysics(float voltage); // Renamed from computePoly for clarity
};

#endif