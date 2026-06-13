/**
 * @file EvaraTDS.h
 * @brief Industrial TDS Calibration & Math Engine
 * @version 1.4.0
 * @author EvaraTech Engineering
 *
 * Changelog v1.4.0:
 *  - NEW: Direct Voltage->PPM single-stage quadratic regression
 *         (replaces two-stage ML pipeline from v1.3.x)
 *         STATIC: 11.91*V^2 + 398.26*V + 6.28  (Data: STATIC.csv)
 *         INLINE:  9.36*V^2 + 463.50*V + 9.84  (Data: INLINE.csv)
 *
 * All v1.3.1 quality fixes retained:
 *  - FIX #2: Cold-start buffer seeded from first real reading (no zero-bias)
 *  - FIX #3: BUFFER_SIZE = 15 (matches firmware median window)
 *  - FIX #4: getRawVoltage() / getCompVoltage() split (no misleading getVoltage)
 *  - FIX #5: Input validation -- voltage [0-5V], temp [-10-100C]
 *  - FIX #6: begin() does NOT reset _currentMode
 */

#ifndef EVARATDS_H
#define EVARATDS_H

#include <Arduino.h>

// Calibration Modes
enum TDSMode {
    MODE_STATIC, // Lab/Bottle Measurement (High Sensitivity Model)
    MODE_INLINE  // Pump Loop Measurement (Flow Compensated Model)
};

class EvaraTDS {
  public:
    EvaraTDS();

    /**
     * @brief Initialize the library. Safe to call multiple times.
     * Does NOT reset TDSMode -- mode survives soft-resets.
     */
    void begin();

    // Mode switch -- set AFTER begin()
    void setMode(TDSMode mode);

    /**
     * @brief Main DSP update loop. Call this every sample cycle.
     * Includes median filtering, temp compensation, and v1.4.0 direct regression.
     * @param voltage_volts  Raw ADC voltage [0.0 - 5.0 V]
     * @param temp_c         Water temperature in Celsius [-10 - 100 C]
     */
    void update(float voltage_volts, float temp_c);

    // --- Getters ---
    float getTDS();           // ppm -- K-factor scaled final reading
    float getEC();            // uS/cm
    float getRawVoltage();    // Median-filtered voltage BEFORE temp compensation
    float getCompVoltage();   // Median-filtered voltage AFTER temp compensation

    // --- Fine-Tuning ---
    /**
     * @brief TDS Conversion Factor.
     * 0.5 = USA/NaCl (default) | 0.7 = Europe/Hydroponics (442)
     */
    void setTDSFactor(float factor);

    /**
     * @brief Temperature Compensation Coefficient.
     * Default: 0.02 (2% per degree C)
     */
    void setTempCoefficient(float coeff);

    /**
     * @brief Manual K-factor field calibration multiplier (default 1.0).
     * K = Reference / Reading  (e.g. K = 500/480 = 1.041)
     */
    void setKFactor(float k);

  private:
    TDSMode _currentMode = MODE_STATIC;
    float   _kFactor     = 1.0f;
    float   _tdsFactor   = 0.5f;
    float   _tempCoeff   = 0.02f;

    // DSP buffer -- 15 samples to match firmware median window
    static const int BUFFER_SIZE = 15;
    float _analogBuffer[BUFFER_SIZE];
    int   _bufferIndex  = 0;
    bool  _bufferSeeded = false;

    // Outputs
    float _finalTDS  = 0.0f;
    float _finalEC   = 0.0f;
    float _rawVolts  = 0.0f; // pre  temp-compensation
    float _compVolts = 0.0f; // post temp-compensation

    // Internal Math
    float getMedian(float* array, int size);
    float computeDirectPhysics(float voltage); // v1.4.0 direct V->PPM model
};

#endif
