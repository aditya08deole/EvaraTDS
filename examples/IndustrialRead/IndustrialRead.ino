#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <DallasTemperature.h>
#include <EvaraTDS.h> // <--- YOUR NEW LIBRARY

// Objects
Adafruit_ADS1115 ads;
OneWire oneWire(27);
DallasTemperature tempSensor(&oneWire);
EvaraTDS tds; // The Math Engine

void setup() {
  Serial.begin(115200);
  
  ads.begin();
  ads.setGain(GAIN_ONE);
  tempSensor.begin();
  tds.begin();

  Serial.println("System Ready.");
}

void loop() {
  // 1. Get Temperature
  tempSensor.requestTemperatures();
  float t = tempSensor.getTempCByIndex(0);
  if (t < -50 || t > 120) t = 25.0; // Fail-safe

  // 2. Get Voltage
  int16_t adc = ads.readADC_SingleEnded(0);
  float volts = adc * 0.000125f; // ADS1115 Gain 1 multiplier

  // 3. Get TDS (The magic happens here!)
  float ppm = tds.getTDS(volts, t);

  Serial.printf("Temp: %.2f C | Volts: %.4f V | TDS: %.2f PPM\n", t, volts, ppm);
  delay(1000);
}