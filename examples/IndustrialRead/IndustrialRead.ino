#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <DallasTemperature.h>
#include <EvaraTDS.h>

/**
 * IndustrialRead.ino -- EvaraTDS v1.4.0 Example
 *
 * v1.4.0 uses direct voltage->PPM quadratic regression:
 *   STATIC: 11.91*V^2 + 398.26*V + 6.28
 *   INLINE:  9.36*V^2 + 463.50*V + 9.84
 */

Adafruit_ADS1115 ads;
OneWire oneWire(27);
DallasTemperature tempSensor(&oneWire);

EvaraTDS tds;

void setup() {
  Serial.begin(115200);

  if (!ads.begin(0x48)) {
    Serial.println("ADS1115 Failed -- check wiring");
    while (1);
  }
  ads.setGain(GAIN_ONE);
  tempSensor.begin();

  tds.begin();

  // Set mode AFTER begin() -- survives soft resets (v1.3.1+ fix)
  tds.setMode(MODE_INLINE);       // Pipe/pump system
  // tds.setMode(MODE_STATIC);    // Bottle/beaker lab test

  tds.setTDSFactor(0.5);          // 0.5 = NaCl/USA | 0.7 = Hydroponics
  // tds.setKFactor(1.0);         // Field trim: K = Reference / Reading

  Serial.println("EvaraTDS v1.4.0 ready.");
}

void loop() {
  tempSensor.requestTemperatures();
  float t = tempSensor.getTempCByIndex(0);
  if (t < -10.0f || t > 100.0f) t = 25.0f; // Sensor fail-safe: default to 25°C

  int16_t adc   = ads.readADC_SingleEnded(0);
  float   volts = adc * 0.000125f; // ADS1115 GAIN_ONE: 0.125 mV per LSB

  // update() always uses actual temperature for full compensation.
  // getTDS()    = compensated ppm   (ThingSpeak field2)
  // getTDSRaw() = uncompensated ppm (ThingSpeak field5, no temp adjustment)
  // getEC()     = compensated EC    (ThingSpeak field6, µS/cm = TDS / TDSFactor)
  tds.update(volts, t);

  // --- Serial output in ThingSpeak field order ---
  Serial.print("[F1] CompV: ");   Serial.print(tds.getCompVoltage(), 4); Serial.print(" V");
  Serial.print(" | [F2] TDS: "); Serial.print(tds.getTDS(), 1);         Serial.print(" ppm");
  Serial.print(" | [F3] Temp: "); Serial.print(t, 1);                   Serial.print(" C");
  Serial.print(" | [F4] RawV: "); Serial.print(tds.getRawVoltage(), 4); Serial.print(" V");
  Serial.print(" | [F5] Raw: ");  Serial.print(tds.getTDSRaw(), 1);     Serial.print(" ppm");
  Serial.print(" | [F6] EC: ");   Serial.print(tds.getEC(), 1);         Serial.println(" uS/cm");

  delay(500);
}
