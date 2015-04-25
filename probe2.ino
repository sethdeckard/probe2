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

int buttonState = 0;
int lastButtonState = LOW;

bool logging = false;

long lastDebounceTime = 0;
long debounceDelay = 10;  // the debounce time; increase if the output flickers

File dataFile;

void setup() {
  Serial.begin(9600);
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
  
  if ((millis() - lastDebounceTime) > debounceDelay) {

    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == HIGH) {
        Serial.println(F("button pressed"));
        logging = !logging;
      }
    }
  }
  
  lastButtonState = reading;
  
  if (logging) {
    logData();
  } else {
    stopLogging();
  }
}

void initializeSdCard(void) {
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(chipSelect)) {
    Serial.println(F("Card failed, or not present!"));
    return;
  }
  Serial.println(F("card initialized."));  
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
  if(!bmp.begin())
  {
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
  String month = padZero(String(now.month(), DEC));
  String day = padZero(String(now.day(), DEC));
  String hour = padZero(String(now.hour(), DEC));
  String minute = padZero(String(now.minute(), DEC));
  String fileName = month + day + hour + minute + ".csv";
  
  int len = fileName.length() + 1;
  char buf[len];
  fileName.toCharArray(buf, len);
  
  Serial.println(buf);
  
  dataFile = SD.open(buf, O_CREAT | O_APPEND | O_WRITE);
}

String padZero(String string) 
{
  if (string.length() == 1) {
    return "0" + string;
  } else {
    return string;
  }  
}

void readPressure(void) {
  sensors_event_t event;
  bmp.getEvent(&event);

  if (event.pressure)
  {
    Serial.print(F("Pressure:    "));
    Serial.print(event.pressure);
    Serial.println(F(" hPa"));
    
    float temperature;
    bmp.getTemperature(&temperature);
    Serial.print(F("Temperature (BMP180): "));
    Serial.print(temperature);
    Serial.println(F(" C"));
  }
  else
  {
    Serial.println(F("Sensor error"));
  }
}

void readHumidity(void) {
  Serial.print(F("Temperature (HTU21D-F): "));
  Serial.print(htu.readTemperature());
  Serial.println(F(" C"));
  
  Serial.print(F("Humidity: "));
  Serial.print(htu.readHumidity());
  Serial.println(F(" %"));
}

void logData(void) {
  if (!dataFile) {
   createFile(); 
  }
  
  digitalWrite(ledPin, HIGH);
  
  float temp1;
  float pressure;
  
  sensors_event_t event;
  bmp.getEvent(&event);
  
  if (event.pressure)
  {
    pressure = event.pressure;
    bmp.getTemperature(&temp1);
  }
  
  float temp2 = htu.readTemperature();
  float humidity = htu.readHumidity();
  
  char temp1String[6];
  dtostrf(temp1, 3, 2, temp1String);
  
  char pressureString[8];
  dtostrf(pressure, 4, 2, pressureString);
  
  char temp2String[6];
  dtostrf(temp2, 3, 2, temp2String);
  
  char humidityString[6];
  dtostrf(humidity, 3, 2, humidityString);
  
  DateTime now = rtc.now();
  char dateTimeString[24];
  sprintf(dateTimeString, "%04d-%02d-%02d %02d:%02d:%02d",
   now.year(), now.month(), now.day(),
   now.hour(), now.minute(), now.second()
  );
  
  char row[64];
  sprintf(row, "%s,%s,%s,%s,%s", dateTimeString, temp1String, pressureString, temp2String, humidityString);
  
  Serial.println(row);
  
  if (dataFile) 
  {
    dataFile.println(row);
  }
  else 
  {
    Serial.println(F("Can not write to data file"));
  }  
}

void stopLogging(void) {

  digitalWrite(ledPin, LOW);
  
  if (dataFile) {
    dataFile.close();
  }
}

