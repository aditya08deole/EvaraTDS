#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <DallasTemperature.h>
#include <EvaraTDS.h>

/**
 * IndustrialRead.ino -- EvaraTDS v1.3.1 Example
 *
 * Demonstrates high-precision Inline/Static measurement with:
 *  - Cold-start buffer seeding (no false low readings on boot)
 *  - Full temperature compensation via actual sensor temp
 *  - Dual voltage getters: getRawVoltage() and getCompVoltage()
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
  if (!ads.begin(0x48)) {
    Serial.println("ADS1115 Failed -- check wiring");
    while (1);
  }
  ads.setGain(GAIN_ONE);
  tempSensor.begin();

  // Init Library
  tds.begin();

  // Set mode AFTER begin() -- mode is preserved across begin() calls (v1.3.1 FIX #6)
  // Use MODE_INLINE for pipe assemblies (applies ML Flow Correction, R2=0.9993)
  // Use MODE_STATIC for bottle/beaker measurements (R2=0.9987)
  tds.setMode(MODE_INLINE);

  // TDS Conversion Factor: 0.5 = USA/NaCl (default), 0.7 = Europe/Hydroponics
  tds.setTDSFactor(0.5);

  // Optional: Field K-factor trim. Adjust if probe reads slightly off due to aging.
  // tds.setKFactor(1.0);

  Serial.println("EvaraTDS v1.3.1 ready.");
}

void loop() {
  // 1. Read Temperature
  tempSensor.requestTemperatures();
  float t = tempSensor.getTempCByIndex(0);

  // Fail-safe: if sensor disconnects, default to 25C (no compensation applied)
  if (t < -10.0f || t > 100.0f) t = 25.0f;

  // 2. Read Voltage from ADS1115 (Channel 0)
  int16_t adc   = ads.readADC_SingleEnded(0);
  float   volts = adc * 0.000125f; // ADS1115 at GAIN_ONE: 0.125mV per LSB

  // 3. Feed the Engine
  // Passing actual temp -> library applies full temperature compensation
  // To get uncompensated reading (25C reference), pass 25.0f instead
  tds.update(volts, t);

  // 4. Print Results
  Serial.print("Raw V: ");        Serial.print(tds.getRawVoltage(), 4);
  Serial.print(" V | Comp V: ");  Serial.print(tds.getCompVoltage(), 4);
  Serial.print(" V | TDS: ");     Serial.print(tds.getTDS(), 1);
  Serial.print(" ppm | EC: ");    Serial.print(tds.getEC(), 1);
  Serial.print(" uS/cm | Temp: ");Serial.print(t, 1);
  Serial.println(" C");

  delay(500);
}
