#include <Timer.h>
#include <Event.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


// Senzors zone
int ZONE             = 4;

// Senzors pins
int PIR_PIN         = A0;
int LIGHT_PIN       = A1;
int TEMPERATURE_PIN = A2;

int EMIT_PIN         = 10;

int PULSE_LENGTH     = 100;

Timer timer;


boolean sent_moving;
int sent_temperature;
int sent_luminosity;
int sent_humidity;
int sent_time;



int read_temperature() {
  int value = analogRead( TEMPERATURE_PIN );
  
  if( value < 30 ) {
    value = 0;  
  }
  
  return value;
}  

int read_humidity() {
  return 50;
}

int read_luminosity() {
  return 90;
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
  delayMicroseconds(PULSE_LENGTH * high);
  digitalWrite(EMIT_PIN, LOW);
  delayMicroseconds(PULSE_LENGTH * low);
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

void sendData() {
  
  boolean moving      = read_moving();
  int temperature = read_temperature();
  int humidity    = read_humidity();
  int luminosity  = read_luminosity();
  
  if( temperature == sent_temperature &&
      humidity == sent_humidity && 
      moving == sent_moving &&
      luminosity == sent_luminosity ) {
      Serial.println("Data is the same. Skip sending!");
      return;
  }
  
  sent_temperature = temperature;
  sent_humidity = humidity;
  sent_luminosity = luminosity;
  sent_moving = moving;
  
  sendBytes( dec2bin(ZONE, 4) );
  sendBytes( dec2bin(ZONE, 4) );
  sendBytes( dec2bin(moving ? 1:0, 1) );
  sendBytes( dec2bin(temperature, 8) );
  sendBytes( dec2bin(humidity, 8) );
  sendBytes( dec2bin(luminosity, 8) );
  sendSync();
   
  digitalWrite(13, HIGH);
  delay(50);
  digitalWrite(13, LOW);
  delay(50);
  
  sent_time = millis();
  
  Serial.println("Sent");
}

void setup() {
  
  timer.every(2000, sendData);  
  
  pinMode(EMIT_PIN, OUTPUT);
  pinMode(13, OUTPUT);
  
  Serial.begin(9600);  
  
}

void loop() {
  
  if(read_moving()) {
    sendData();  
  }
  
  timer.update();
  
  delay(500);
}
