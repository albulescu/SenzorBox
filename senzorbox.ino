#include <OneWire.h>

#include <DallasTemperature.h>

#include <Timer.h>
#include <Event.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

int FREQUENCY = 20;

// Send data to master interval
int SEND_INTERVAL    = 1000 * 60;

// set isMoving flag to false after this interval
int MOVING_STOP_TIMEOUT  = 2000;

// Senzors zone
int ZONE             = 4;

// Senzors pins
int EMIT_PIN        = 0;
int TEMPERATURE_PIN = 1;
int LIGHT_PIN       = A1;
int HUMIDITY_PIN    = A3;
int PIR_PIN         = A2;


int PULSE_LENGTH     = 100;

Timer timer;

OneWire oneWire(TEMPERATURE_PIN);
 
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Keep last sent data
// to check if send is 
// required
boolean sent_moving;
int sent_temperature;
int sent_luminosity;
int sent_humidity;
int sent_time;

// PIR senzor is on
boolean isMoving;

int delayVal( int value ) {
   return value / FREQUENCY;
}

int read_temperature() {
  
  sensors.requestTemperatures();
  
  int temperature = sensors.getTempCByIndex(0);
  
  
  
  return temperature;
}  

int read_humidity() {
  return 50;
}

int read_luminosity() {
  return analogRead(LIGHT_PIN);
}

boolean read_moving() {
  int voltage = analogRead(PIR_PIN)  * (5.0 / 1023.0);
  return voltage > 1;
}

char* dec2bin(unsigned long Dec, unsigned int bitLength){
  static char bin[64];
  unsigned int i=0;
  char fill = '0';
  
  while (Dec > 0) {
    bin[32+i++] = ((Dec & 1) > 0) ? '1' : fill;
    Dec = Dec >> 1;
  }

  for (unsigned int j = 0; j< bitLength; j++) {
    if (j >= bitLength - i) {
      bin[j] = bin[ 31 + i - (j - (bitLength - i)) ];
    }else {
      bin[j] = fill;
    }
  }
  bin[bitLength] = '\0';
  
  return bin;
}

void sendBit(int high, int low) {
  digitalWrite(EMIT_PIN, HIGH);
  delayMicroseconds(delayVal(PULSE_LENGTH * high));
  digitalWrite(EMIT_PIN, LOW);
  delayMicroseconds(delayVal(PULSE_LENGTH * low));
}

void sendZero() {
  sendBit(4, 11);
}

void sendOne() {
  sendBit(9, 6);
}

void sendSync() {
  sendBit(1, 71); 
}

void sendBytes( char * sCodeWord ) {
  
  int i = 0;
  while (sCodeWord[i] != '\0') {
    switch(sCodeWord[i]) {
      case '0':
        sendZero();
      break;
      case '1':
        sendOne();
      break;
    }
    i++;
  }
}

void blinkLed() {
  
  // Blink arduino led
  digitalWrite(0, HIGH);
  delay(delayVal(50));
  digitalWrite(0, LOW);
  delay(delayVal(50));
  
}

void sendData() {
  
  boolean moving  = read_moving();
  int temperature = read_temperature();
  int humidity    = read_humidity();
  int luminosity  = read_luminosity();
  
  if( temperature == sent_temperature &&
      humidity == sent_humidity && 
      moving == sent_moving &&
      luminosity == sent_luminosity ) {
      //return;
  }
  
  sent_temperature = temperature;
  sent_humidity = humidity;
  sent_luminosity = luminosity;
  sent_moving = moving;
  
  sendBytes( dec2bin(ZONE, 4) );
  sendBytes( dec2bin(moving ? 1:0, 1) );
  sendBytes( dec2bin(temperature, 8) );
  sendBytes( dec2bin(humidity, 8) );
  sendBytes( dec2bin(luminosity, 8) );
  sendSync();
  
  blinkLed();
  
  sent_time = millis();

}

void setup() {
  
  timer.every(delayVal(SEND_INTERVAL), sendData);  
  
  pinMode(EMIT_PIN, OUTPUT);
  pinMode(TEMPERATURE_PIN, INPUT); 
  pinMode(LIGHT_PIN, INPUT);
  pinMode(HUMIDITY_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  
  sensors.begin();
}

void stopMoving() {
   isMoving = false; 
   blinkLed();
}

void loop() {

  boolean moving = read_moving();
    
  if( moving && moving != isMoving ) {
    // if moving detected 
    // send data immediately
    sendData();
    
    isMoving = moving;
    
    timer.after(delayVal(MOVING_STOP_TIMEOUT), stopMoving);
  }
  
  timer.update();
  
  delay(delayVal(100));
}
