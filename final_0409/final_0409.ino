#include <avr/sleep.h>
#include "eeprom_anything.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <mlx90615.h>
#include "album.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MLX90615 mlx = MLX90615();

byte sleep_ = 0;
unsigned long sleep_time = 0;
unsigned long count_time = 0;
struct config_type {
  float average;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
//  Init IO
///////////////////////////////////////////////////////////////////////////////////////////////////////
void Init_IO_Pins() {
  delay(10);
  //ON ACD
  //ACSR &= ~_BV(ACIE);
  //ACSR &= ~_BV(ACD);
  //ACSR |= _BV(ACIE);
  //ON ADC
  //ADCSRA |= _BV(ADEN);
  //ADCSRA |= _BV(ADIF);

  // PWREN (Set 0 to turn on the 3V3-A)
  pinMode(8, OUTPUT);
  digitalWrite(8, 0);

  // Unused pins
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);

  // Switch: D2
  pinMode(2, INPUT_PULLUP);

  // UART
  pinMode(0, INPUT);
  pinMode(1, OUTPUT);
  digitalWrite(1, 1);

  // STATUS and SPK
  pinMode(5, OUTPUT);
  digitalWrite(5, 1);
  pinMode(6, OUTPUT);
  digitalWrite(6, 0);

  // I2C
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);

  // USER
  //Serial.begin(9600);

  //module
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.drawBitmap(0, 0,  myBitmap00, 128, 64, 1);
  display.display();
  mlx.begin();
  delay(1000);
  display.clearDisplay();
  display.drawBitmap(0, 0,  myBitmap01, 128, 64, 1);
  display.display();

}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//  Go to sleep and wakeup
///////////////////////////////////////////////////////////////////////////////////////////////////////
void Goto_Sleep_Power_Down() {
  sleep_ = 0;
  //OFF ACD
  ACSR &= ~_BV(ACIE);
  ACSR |= _BV(ACD);
  //OFF ADC
  ADCSRA |= _BV(ADIF);
  ADCSRA &= ~_BV(ADIE);
  ADCSRA &= ~_BV(ADEN);

  cli();
  attachInterrupt(digitalPinToInterrupt(2), ISR_2, RISING);
  sei();

  // PWREN= 1 (3V3-A OFF)
  digitalWrite(8, 1);

  // Buzzer and LED = 0
  digitalWrite(6, 0);
  digitalWrite(5, 0);

  // TXD/RXD/SDA/SCL set to INPUT without PULLUP
  pinMode(0, INPUT);
  pinMode(1, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  // Close all protocol
  Wire.end();
  Serial.end();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_bod_disable();
  sleep_cpu();

  // Wakeup
  cli();
  sleep_disable();
  detachInterrupt(digitalPinToInterrupt(2));
  sei();
  Init_IO_Pins();
  //Serial.println("Wakeup");
  tone(6,1046);
  delay(100);
  tone(6,1175);
  delay(100);
  tone(6,1318);
  delay(100);
  noTone(6);
  while(digitalRead(2) == 0){}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//  Read MLX90615
///////////////////////////////////////////////////////////////////////////////////////////////////////
void Read_MLX90615() {
  if (digitalRead(2) == 0) {
    tone(6,1000);
    delay(500);
    noTone(6);
    delay(50);
    byte close_ = 0;
    display.clearDisplay();
    display.drawBitmap(0, 0,  myBitmap02, 128, 64, 1);
    display.display();
    while(digitalRead(2) == 0){
      if ((millis() - sleep_time) > 1000) {
        close_++;
        sleep_time = millis();
        tone(6,1318);
        delay(100);
        noTone(6);
        delay(50);
      }
      if (close_ == 5) {
        calibration_mode();
      }
    }
    float object_temp = mlx.get_object_temp() + read_eeprom();
    //Serial.println(String(object_temp) + " *C");
    display.clearDisplay();
    display.drawBitmap(0, 0,  myBitmap03, 128, 64, 1);
    display.setTextSize(2);
    display.setCursor(58,40);
    display.println(String(object_temp, 1));
    display.display();
    for (int i=0; i<3; i++) {
      tone(6,1000);
      delay(100);
      noTone(6);
      delay(50);
    }
    sleep_ = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//  Calibration Mode
///////////////////////////////////////////////////////////////////////////////////////////////////////
void calibration_mode() {
  byte next = 0;
  float sign = -1;
  byte digits = 0;
  float decimal = 0;
  //Serial.println("dropout");
  display.clearDisplay();
  display.drawBitmap(0, 0,  myBitmap04, 128, 64, 1);
  display.display();
  while(digitalRead(2) == 0){}

  //set sign
  while(!next) {
    display.clearDisplay();
    display.drawBitmap(0, 0,  myBitmap05, 128, 64, 1);
    display.setTextSize(2);
    display.setCursor(58,40);
    if (sign == -1) {
      display.println("-0.0");
    }
    else {
      display.println("+0.0");
    }
    display.display();
    if (digitalRead(2) == 0) {
      byte close_ = 0;
      sleep_ = 0;
      while(digitalRead(2) == 0){
        if ((millis() - count_time) > 1000) {
          close_++;
          count_time = millis();
          tone(6,1318);
          delay(100);
          noTone(6);
          delay(50);
        }
        if (close_ == 3) {
          //Serial.println("dropout");
          display.clearDisplay();
          display.drawBitmap(0, 0,  myBitmap06, 128, 64, 1);
          display.display();
          while(digitalRead(2) == 0){}
          next = 1;
          break;
        }
      }
      if (close_ != 3){
        sign *= -1;
        //Serial.println("sign:" + String(sign));
      }
      delay(300);
    }
    count_sleep();
  }

  //set digits
  while(next) {
    display.clearDisplay();
    display.drawBitmap(0, 0,  myBitmap05, 128, 64, 1);
    display.setTextSize(2);
    display.setCursor(58,40);
    if (sign == -1) {
      display.println("-"+String(digits)+".0");
    }
    else {
      display.println("+"+String(digits)+".0");
    }
    display.display();
    if (digitalRead(2) == 0) {
      byte close_ = 0;
      sleep_ = 0;
      while(digitalRead(2) == 0){
        if ((millis() - count_time) > 1000) {
          close_++;
          count_time = millis();
          tone(6,1318);
          delay(100);
          noTone(6);
          delay(50);
        }
        if (close_ == 3) {
          //Serial.println("dropout");
          display.clearDisplay();
          display.drawBitmap(0, 0,  myBitmap06, 128, 64, 1);
          display.display();
          while(digitalRead(2) == 0){}
          next = 0;
          break;
        }
      }
      if (close_ != 3){
        digits += 1;
        if (digits == 10)digits=0;
        //Serial.println("digits:" + String(digits));
      }
      delay(300);
    }
    count_sleep();
  }
  //set decimal
  while(!next) {
    display.clearDisplay();
    display.drawBitmap(0, 0,  myBitmap05, 128, 64, 1);
    display.setTextSize(2);
    display.setCursor(58,40);
    if (sign == -1) {
      display.println("-"+String(digits+decimal, 1));
    }
    else {
      display.println("+"+String(digits+decimal, 1));
    }
    display.display();
    if (digitalRead(2) == 0) {
      byte close_ = 0;
      sleep_ = 0;
      while(digitalRead(2) == 0){
        if ((millis() - count_time) > 1000) {
          close_++;
          count_time = millis();
          tone(6,1318);
          delay(100);
          noTone(6);
          delay(50);
        }
        if (close_ == 3) {
          //Serial.println("dropout");
          //Serial.println("final: "+ String(sign*(digits+decimal), 1));
          display.clearDisplay();
          display.drawBitmap(0, 0,  myBitmap07, 128, 64, 1);
          display.setTextSize(2);
          display.setCursor(58,40);
          display.println(String(sign*(digits+decimal), 1));
          display.display();
          config_type config;
          config.average = sign*(digits+decimal);
          EEPROM_writeAnything(5, config);
          while(digitalRead(2) == 0){}
          next = 1;
          break;
        }
      }
      if (close_ != 3){
        decimal += 0.1;
        if (decimal >= 1.0) {
          decimal=0;
        }
        //Serial.println("decimal:" + String(decimal, 1));
      }
      delay(300);
    }
    count_sleep();
  }
}


void count_sleep() {
  if ((millis() - sleep_time) > 1000) {
    sleep_++;
    sleep_time = millis();
    //Serial.println(sleep_);
  }

  if (sleep_ == 10) {
    //Serial.println("Sleep!");
    display.clearDisplay();
    display.drawBitmap(0, 0,  myBitmap00, 128, 64, 1);
    display.display();
    tone(6,262);
    delay(100);
    noTone(6);
    delay(1000);
    Goto_Sleep_Power_Down();
  }
}

float read_eeprom() {
  float read_eeprom = 0;
  config_type config_readback;
  EEPROM_readAnything(5, config_readback);
  read_eeprom = config_readback.average;
  if (String(read_eeprom) == " NAN") {
    read_eeprom = 0;
  }
  return read_eeprom;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//  ISR and Program Entry
///////////////////////////////////////////////////////////////////////////////////////////////////////
void ISR_2() {  // Do nothing

}

void setup() {
  Init_IO_Pins();
  //Serial.println("Start");
  tone(6,1046);
  delay(100);
  tone(6,1175);
  delay(100);
  tone(6,1318);
  delay(100);
  noTone(6);
}

void loop() {
  Read_MLX90615();
  count_sleep();
}
