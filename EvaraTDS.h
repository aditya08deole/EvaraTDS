/**
 * @file EvaraTDS.h
 * @brief Industrial TDS Calibration & Math Engine
 * @version 1.1.0
 * @author EvaraTech Engineering
 */

#ifndef EVARATDS_H
#define EVARATDS_H

#include <Arduino.h>

struct CalPoint {
    float v;   
    float ppm; 
};

class EvaraTDS {
  public:
    EvaraTDS();

    /**
     * @brief Initialize the library internals
     */
    void begin();

    /**
     * @brief Main DSP update loop. Call this before getting readings.
     * Incorporates Median Filtering to reject noise.
     * @param voltage_volts Raw voltage from ADS1115 or Analog Pin
     * @param temp_c Current temperature in Celsius
     */
    void update(float voltage_volts, float temp_c);

    /**
     * @brief Get the calculated TDS value (Parts Per Million)
     */
    float getTDS();

    /**
     * @brief Get the Electrical Conductivity (µS/cm)
     */
    float getEC();

    /**
     * @brief Get the smoothed, temperature-compensated voltage (Useful for debugging)
     */
    float getVoltage();

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
     * Use this to calibrate the probe if it drifts over time.
     */
    void setKFactor(float k);

  private:
    float _kFactor = 1.0;
    float _tdsFactor = 0.5;
    float _tempCoeff = 0.02;

    // DSP Buffers
    static const int BUFFER_SIZE = 10;
    float _analogBuffer[BUFFER_SIZE];
    int _bufferIndex = 0;
    
    // Processed Values
    float _finalTDS = 0.0;
    float _finalEC = 0.0;
    float _smoothedVolts = 0.0;
    
    // Internals
    float getMedian(float* array, int size);
    float computePoly(float voltage);
};

#endif