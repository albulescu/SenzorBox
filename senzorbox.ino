#include <OneWire.h>

#include <DallasTemperature.h>

#include <Timer.h>
#include <Event.h>

int FREQUENCY = 1;

unsigned long SEND_INTERVAL    = 300000;//5min

int MOVING_STOP_TIMEOUT  = 3000;

// Senzors zone
int ZONE             = 4;

// Senzors pins
int EMIT_PIN        = 0;
int TEMPERATURE_PIN = 1;
int LIGHT_PIN       = A1;
int HUMIDITY_PIN    = A3;
int PIR_PIN         = A2;
int LED_PIN         = A0;

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
boolean isMoving = false;

unsigned long delayVal( unsigned long value ) {
  return value;
}

int read_temperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

int read_humidity() {
  return analogRead(HUMIDITY_PIN)  * (99 / 1023.0);
}

int read_luminosity() {
  return analogRead(LIGHT_PIN)  * (99 / 1023.0);
}

boolean read_moving() {
  int voltage = analogRead(PIR_PIN)  * (5.0 / 1023.0);
  return voltage > 2;
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


void sendData() {

  boolean moving  = read_moving();
  int temperature = read_temperature();
  int humidity    = read_humidity();
  int luminosity  = read_luminosity();

  if ( temperature == sent_temperature &&
       humidity == sent_humidity &&
       moving == sent_moving &&
       luminosity == sent_luminosity ) {
    return;
  }

  sent_temperature = temperature;
  sent_humidity = humidity;
  sent_luminosity = luminosity;
  sent_moving = moving;

  digitalWrite(LED_PIN, HIGH);
 
  sendSync();
  sendBytes( dec2bin(ZONE, 4) );
  sendBytes( dec2bin(moving ? 1:0, 1) );
  sendBytes( dec2bin(temperature, 8) );
  sendBytes( dec2bin(humidity, 8) );
  sendBytes( dec2bin(luminosity, 8) );

  digitalWrite(LED_PIN, LOW);

}

void setup() {

  pinMode(EMIT_PIN, OUTPUT);
  pinMode(TEMPERATURE_PIN, INPUT);
  pinMode(LIGHT_PIN, INPUT);
  pinMode(HUMIDITY_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  sensors.begin();

  timer.every(delayVal(SEND_INTERVAL), sendData);
}

void stopMoving() {
  isMoving = false;
}

void loop() {

  boolean moving = read_moving();

  if ( moving && moving != isMoving ) {
    // if moving detected
    // send data immediately
    sendData();

    isMoving = moving;

    timer.after(delayVal(MOVING_STOP_TIMEOUT), stopMoving);
  }

  timer.update();

  delay(delayVal(50));
}
