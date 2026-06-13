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
  if (t < -10.0f || t > 100.0f) t = 25.0f; // Sensor fail-safe

  int16_t adc   = ads.readADC_SingleEnded(0);
  float   volts = adc * 0.000125f; // ADS1115 GAIN_ONE: 0.125mV/LSB

  // Pass actual temp -> full compensation | Pass 25.0 -> uncompensated reference
  tds.update(volts, t);

  Serial.print("Raw V: ");         Serial.print(tds.getRawVoltage(), 4);
  Serial.print(" | Comp V: ");     Serial.print(tds.getCompVoltage(), 4);
  Serial.print(" | TDS: ");        Serial.print(tds.getTDS(), 1);
  Serial.print(" ppm | EC: ");     Serial.print(tds.getEC(), 1);
  Serial.print(" uS/cm | Temp: "); Serial.print(t, 1);
  Serial.println(" C");

  delay(500);
}
