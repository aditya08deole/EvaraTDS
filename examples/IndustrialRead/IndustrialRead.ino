#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <DallasTemperature.h>
#include <EvaraTDS.h> 

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
  
  // OPTIONAL SETTINGS:
  tds.setTDSFactor(0.5); // 0.5 for USA (NaCl), 0.7 for Hydroponics
  // tds.setKFactor(1.0); // Use if you need to manually calibrate drift
}

void loop() {
  // 1. Get Physical Readings
  tempSensor.requestTemperatures();
  float t = tempSensor.getTempCByIndex(0);
  
  // Fail-safe for temp sensor disconnection
  if (t < -50 || t > 125) t = 25.0;

  int16_t adc = ads.readADC_SingleEnded(0);
  float volts = adc * 0.000125f; 

  // 2. Feed the Engine (Handles Filtering & Math)
  tds.update(volts, t);

  // 3. Print Results
  Serial.print("Volts: "); Serial.print(tds.getVoltage(), 3);
  Serial.print(" V | TDS: "); Serial.print(tds.getTDS(), 1);
  Serial.print(" ppm | EC: "); Serial.print(tds.getEC(), 1);
  Serial.println(" uS/cm");

  delay(500);
}