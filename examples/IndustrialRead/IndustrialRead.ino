#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <DallasTemperature.h>
#include <EvaraTDS.h> 

/**
 * IndustrialRead.ino - EvaraTDS v1.3.0 Example
 * Demonstrates high-precision Inline/Static measurement.
 */

// Hardware Objects
Adafruit_ADS1115 ads;
OneWire oneWire(27);
DallasTemperature tempSensor(&oneWire);

// Library Object
EvaraTDS tds;

void setup() {
  Serial.begin(115200);
  
  // Init Sensors
  if(!ads.begin(0x48)) {
    Serial.println("ADS1115 Failed");
  }
  ads.setGain(GAIN_ONE);
  tempSensor.begin();

  // Init Library
  tds.begin();
  
  // *** CRITICAL SETTING FOR YOUR PIPE SYSTEM ***
  // v1.3.0 Update: 
  // MODE_INLINE now uses Poly-2 Regression (R2=0.9993) for flow compensation
  tds.setMode(MODE_INLINE); 
  
  // Optional: Set Factor (0.5 for USA, 0.7 for Hydroponics)
  tds.setTDSFactor(0.5);

  // Field Calibration (K-Factor)
  // Use this if your probe reads slightly off due to aging/dirt.
  // tds.setKFactor(1.0); 
}

void loop() {
  // 1. Get Physical Readings
  tempSensor.requestTemperatures();
  float t = tempSensor.getTempCByIndex(0);

  // Fail-safe for temp sensor disconnection
  if (t < -50 || t > 125) t = 25.0;

  int16_t adc = ads.readADC_SingleEnded(0);
  float volts = adc * 0.000125f;

  // 2. Feed the Engine (Handles v1.3.0 ML Models)
  tds.update(volts, t);

  // 3. Print Results
  Serial.print("Volts: ");
  Serial.print(tds.getVoltage(), 3);
  Serial.print(" V | TDS: "); Serial.print(tds.getTDS(), 1);
  Serial.print(" ppm | EC: "); Serial.print(tds.getEC(), 1);
  Serial.println(" uS/cm");

  delay(500);
}
