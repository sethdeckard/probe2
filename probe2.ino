#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_HTU21DF.h>

/*
  SD card datalogger:
 * analog sensors on analog ins 0, 1, and 2
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
 */

RTC_DS1307 rtc;
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
Adafruit_HTU21DF htu = Adafruit_HTU21DF();

const int chipSelect = 10;
const int buttonPin = 2;
const int ledPin =  5;
const int debounceDelay = 50;
const int blinkDelay = 2000;
const int syncDelay = 30000;

int buttonState = 0;
int lastButtonState = LOW;
long lastDebounceTime = 0;
long lastBlinkTime = 0; 
long lastSyncTime = 0;
bool logging = false;
File dataFile;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  pinMode(10, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  
  digitalWrite(ledPin, LOW);
  
  initializeSdCard();
  initializeRtc();
  initializePressureSensor();
  initializeHumiditySensor();
}

void loop() {
  int reading = digitalRead(buttonPin);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastBlinkTime) > blinkDelay) {
     digitalWrite(ledPin, LOW);
     lastBlinkTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == HIGH) {
        logging = !logging;
      }
    }
  }
  
  lastButtonState = reading;
  
  if (logging) {
    logMeasurements();
    syncDataFile();
  } else {
    stopLogging();
  }
}

void initializeSdCard(void) {
  if (!SD.begin(chipSelect)) {
    Serial.println(F("Card failed, or not present!"));
    return;
  }
  Serial.println(F("Card initialized."));  
}

void initializeRtc(void) {
  rtc.begin();
  if (!rtc.isrunning()) {
    Serial.println(F("RTC is NOT running!"));
    return;
  }
  Serial.println(F("RTC is running."));
}

void initializePressureSensor(void) {
  if(!bmp.begin()) {
    Serial.print(F("No BMP180 detected!"));
    return;
  }
  Serial.println(F("BMP180 detected.")); 
}

void initializeHumiditySensor(void) {
  if (!htu.begin()) {
    Serial.println(F("No HTU21D-F detected!"));
    return;
  }
  Serial.println(F("HTU21D-F detected."));
}

void createFile(void) {
  DateTime now = rtc.now();
  char fileName[13];
  sprintf(fileName, "%02d%02d%02d%02d.csv",
   now.month(), now.day(),
   now.hour(), now.minute()
  );

  Serial.println(fileName);
  
  dataFile = SD.open(fileName, O_CREAT | O_APPEND | O_WRITE);
}

void logMeasurements() {
  if (!dataFile) {
   createFile(); 
  }
  
  char tempPressureText[8];
  char pressureText[10];
  
  sensors_event_t event;
  bmp.getEvent(&event);
  
  if (event.pressure) {
    float pressure = event.pressure;
    dtostrf(pressure, 4, 2, pressureText);
    
    float tempPressure;
    bmp.getTemperature(&tempPressure);
    dtostrf(tempPressure, 3, 2, tempPressureText);
  }
  
  float humidity = htu.readHumidity();
  char humidityText[6];
  dtostrf(humidity, 3, 2, humidityText);
  
  float tempHumidity = htu.readTemperature();
  char tempHumidityText[8];
  dtostrf(tempHumidity, 3, 2, tempHumidityText);
  
  DateTime now = rtc.now();
  char dateTimeText[24];
  sprintf(dateTimeText, "%04d-%02d-%02d %02d:%02d:%02d",
   now.year(), now.month(), now.day(),
   now.hour(), now.minute(), now.second()
  );
  
  char row[64];
  sprintf(row, "%s,%s,%s,%s,%s",
   dateTimeText, tempPressureText, pressureText, 
   tempHumidityText, humidityText
  );
  
  Serial.println(row);
  
  if (dataFile) {
    dataFile.println(row);
    digitalWrite(ledPin, HIGH);
  } else {
    Serial.println(F("Can not write to data file"));
    digitalWrite(ledPin, LOW);
  }
}

void stopLogging(void) {
  digitalWrite(ledPin, LOW);
  
  if (dataFile) {
    dataFile.close();
  }
  
  lastSyncTime = 0;
}

void syncDataFile(void) {
  if ((millis() - lastSyncTime) > syncDelay) {
    Serial.println(F("syncing written data"));
    dataFile.flush();
    lastSyncTime = millis();
  }  
}

