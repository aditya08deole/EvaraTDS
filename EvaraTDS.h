/**
 * @file EvaraTDS.h
 * @brief Industrial TDS Calibration & Math Engine
 * @version 1.0.0
 * @author EvaraTech Engineering
 */

#ifndef EVARATDS_H
#define EVARATDS_H

#include <Arduino.h>

// Calibration Point Structure
struct CalPoint {
    float v;   // Voltage
    float ppm; // TDS Value
};

class EvaraTDS {
  public:
    /**
     * @brief Constructor
     * @param vRef Reference voltage of the ADC (usually 3.3V or 5.0V, or 1.0 for normalized)
     * @param adcResolution ADC Resolution (e.g., 4096 for 12-bit) - Optional if passing Volts directly
     */
    EvaraTDS();

    /**
     * @brief Initialize the library
     */
    void begin();

    /**
     * @brief Calculate TDS from raw voltage and temperature
     * Uses a Non-Linear Quadratic Interpolation Engine (R2=0.988)
     * * @param voltage_volts The voltage read from the analog pin (or ADS1115)
     * @param temp_c The current temperature in Celsius
     * @return float TDS value in PPM
     */
    float getTDS(float voltage_volts, float temp_c);

    /**
     * @brief Set a custom K-Factor (default 1.0) if probe degrades over time
     */
    void setKFactor(float k);

  private:
    float _kFactor = 1.0;
    
    // Internal helper for temperature compensation
    float compensateTemperature(float voltage, float temp_c);
    
    // Internal helper for mathematical interpolation
    float computePoly(float voltage);
};

#endif