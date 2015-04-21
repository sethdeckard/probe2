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

const int maxRows = 10000;
const int chipSelect = 10;
const int buttonPin = 2;
const int ledPin =  5;

int buttonState = 0;
int lastButtonState = LOW;

bool logging = false;

long lastDebounceTime = 0;
long debounceDelay = 40;  // the debounce time; increase if the output flickers

File dataFile;
int rowCount = 0;

void setup() {
  Serial.begin(9600);

  Serial.print("Initializing SD card...");
  
  pinMode(10, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  
  digitalWrite(ledPin, LOW);

  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present!");
    return;
  }
  Serial.println("card initialized.");
  
  Wire.begin();
  
  rtc.begin();
  
  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    return;
  }
  Serial.println("RTC is running.");
  
  if(!bmp.begin())
  {
    Serial.print("No BMP180 detected!");
    return;
  }
  Serial.println("BMP180 detected.");
  
  if (!htu.begin()) {
    Serial.println("No HTU21D-F detected!");
    return;
  }
  Serial.println("HTU21D-F detected.");
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
        Serial.println("button pressed");
        logging = !logging;
      }
    }
  }
  
  lastButtonState = reading;
  
  if (logging) {
    logData();
    rowCount++;
  } else {
    stopLogging();
    rowCount = 0;
  }
  
  if (rowCount == maxRows)
  {
    stopLogging();
    rowCount = 0;
    logData();
    rowCount++;
  }
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
    Serial.print("Pressure:    ");
    Serial.print(event.pressure);
    Serial.println(" hPa");
    
    float temperature;
    bmp.getTemperature(&temperature);
    Serial.print("Temperature (BMP180): ");
    Serial.print(temperature);
    Serial.println(" C");
  }
  else
  {
    Serial.println("Sensor error");
  }
}

void readHumidity(void) {
  Serial.print("Temperature (HTU21D-F): ");
  Serial.print(htu.readTemperature());
  Serial.println(" C");
  
  Serial.print("Humidity: ");
  Serial.print(htu.readHumidity());
  Serial.println(" %");
}

void logData(void) {
  if (!dataFile) {
   createFile(); 
  }
  
  digitalWrite(ledPin, HIGH);
    
  DateTime now = rtc.now();
  String time = String(now.unixtime(), DEC);
  
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
  
  String row = time + "," + temp1 + "," + pressure+ "," + temp2+ "," + humidity;
  
  Serial.println(row);
  
  if (dataFile) 
  {
    dataFile.println(row);
  }
  else 
  {
    Serial.println("Can not write to data file");
  }  
}

void stopLogging(void) {

  digitalWrite(ledPin, LOW);
  
  if (dataFile) {
    dataFile.close();
  }
}

