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

//-----------------------------------------------------------------
//  Define I/O Pin
//-----------------------------------------------------------------
#define   pinRXD          0
#define   pinTXD          1
#define   pinD2           2

#define   pinSTATUS       5
#define   pinSPK          6
#define   pinPWREN        8

#define   pinSDA          A4
#define   pinSCL          A5

int address = 5;
int count_bt = 0;
float cali_value = 0;
float cali_value_ = 0;
float cali_value__ = 0;
float cali_value_fine = 0;
int close_ = 0;
int startgo = 0;
int selt = 0;
float temp_ = 0;
unsigned long sleep_time = 0;
unsigned long show_time = 0;
struct config_type {
  float average;
};

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
  pinMode(pinPWREN, OUTPUT);
  digitalWrite(pinPWREN, 0);

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
  pinMode(pinD2, INPUT_PULLUP);

  // UART
  pinMode(pinRXD, INPUT);
  pinMode(pinTXD, OUTPUT);
  digitalWrite(pinTXD, 1);

  // STATUS and SPK
  pinMode(pinSTATUS, OUTPUT);
  digitalWrite(pinSTATUS, 1);
  pinMode(pinSPK, OUTPUT);
  digitalWrite(pinSPK, 0);

  // I2C
  pinMode(pinSDA, INPUT_PULLUP);
  pinMode(pinSCL, INPUT_PULLUP);

}

//--------------------------------------------------------
//
//--------------------------------------------------------
void Goto_Sleep_Power_Down() {
  close_ = 0;
  //OFF ACD
  ACSR &= ~_BV(ACIE);
  ACSR |= _BV(ACD);
  //OFF ADC
  ADCSRA |= _BV(ADIF);
  ADCSRA &= ~_BV(ADIE);
  ADCSRA &= ~_BV(ADEN);

  cli();
  attachInterrupt(digitalPinToInterrupt(pinD2), ISR_pinD2, RISING);
  sei();

  // PWREN= 1 (3V3-A OFF)
  digitalWrite(pinPWREN, 1);

  // Buzzer and LED = 0
  digitalWrite(pinSPK, 0);
  digitalWrite(pinSTATUS, 0);

  // TXD/RXD/SDA/SCL set to INPUT without PULLUP
  pinMode(pinRXD, INPUT);
  pinMode(pinTXD, INPUT);
  pinMode(pinSDA, INPUT);
  pinMode(pinSCL, INPUT);
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
  detachInterrupt(digitalPinToInterrupt(pinD2));
  Serial.println("Wakeup");
  sei();
  Init_IO_Pins();
  Serial.begin(9600);
  mlx.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//  ISR and Program Entry
///////////////////////////////////////////////////////////////////////////////////////////////////////
void ISR_pinD2() {  // Do nothing
  
  
}

void setup() {
  Init_IO_Pins();
  Serial.begin(9600);
  tone(pinSPK,1046); 
  delay(100);
  tone(pinSPK,1175);
  delay(100);
  tone(pinSPK,1318);
  delay(100);
  noTone(pinSPK);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.drawBitmap(0, 0,  myBitmap8, 128, 64, 1);
  display.display();
  mlx.begin();
  delay(1000);
  display.clearDisplay();
  display.drawBitmap(0, 0,  myBitmap9, 128, 64, 1);
  display.display();
}

void loop() {
  if (digitalRead(pinD2) == 0) {
    close_ = 0;
    startgo = 1;
  }
  if (startgo == 1) {
    //chack button push time
    count_bt = 0;
    while (digitalRead(pinD2) == 0) {
      if (count_bt == 50) {
        Serial.println("calibrate mode, please dropout button!");
        tone(pinSPK,1000);
        delay(500);
        noTone(pinSPK);
        display.clearDisplay();
        display.drawBitmap(0, 0,  myBitmap3, 128, 64, 1);
        display.display();
        while (digitalRead(pinD2) == 0) {}
        break;
      }
      count_bt += 1;
      delay(100);
    }
    //read mode
    if (count_bt < 50) {
      tone(pinSPK,1000);
      delay(100);
      noTone(pinSPK);
      Serial.print("Read value: ");
      config_type config_readback;
      EEPROM_readAnything(address, config_readback);
      Serial.println(config_readback.average);
      temp_ = 0;
      for (int i=0; i<5; i++) {
        display.clearDisplay();
        display.drawBitmap(0, 0,  myBitmap1, 128, 64, 1);
        display.setTextSize(2);
        display.setCursor(58,40);
        if (String(config_readback.average) == " NAN") {
          display.println(String(mlx.get_object_temp(), 1));
        }
        else {
          display.println(String(mlx.get_object_temp()-config_readback.average, 1));
        }
        
        display.display();
        temp_ += mlx.get_object_temp();
        delay(100);
      }
      for(int i=0;i<3;i++) {
        tone(pinSPK,1000);
        delay(100);
        noTone(pinSPK);
        delay(50);
      }
      noTone(pinSPK);
      display.clearDisplay();
      display.drawBitmap(0, 0,  myBitmap2, 128, 64, 1);
      display.setTextSize(2);
      display.setCursor(58,40);
      if (String(config_readback.average) == " NAN") {
         display.println(String(temp_/5, 1));
      }
      else {
        display.println(String(temp_/5 - config_readback.average, 1));
      }
      
      display.display();
      if (String(config_readback.average) == " NAN") {
         if ((temp_/5) > 37.2) {
          for (int i=0; i<3; i++) {
            tone(pinSPK, 1046);
            delay(600);
            tone(pinSPK, 1318);
            delay(500);
          }
          noTone(pinSPK);
        }
      }
      else {
        if ((temp_/5 - config_readback.average) > 37.2) {
          for (int i=0; i<3; i++) {
            tone(pinSPK, 1046);
            delay(600);
            tone(pinSPK, 1318);
            delay(500);
          }
          noTone(pinSPK);
        }
      }
      
      Serial.println("read down");
    }

    //calibrate mode
    else {
      Serial.println("calibrate mode");
      display.clearDisplay();
      display.drawBitmap(0, 0,  myBitmap4, 128, 64, 1);
      display.display();
      cali_value = 0;
      cali_value_ = 0;
      cali_value__ = 0;
      show_time = millis();
      while (count_bt < 99) {
        count_bt = 0;
        if (digitalRead(pinD2) == 0) {
          close_ = 0;
          show_time = millis();
          while (digitalRead(pinD2) == 0) {
            if (count_bt == 20) {
              tone(pinSPK,1000);
              delay(100);
              noTone(pinSPK);
              if (selt != 2) {
                selt++;
                Serial.println("OK");
                display.clearDisplay();
                display.drawBitmap(0, 0,  myBitmap4, 128, 64, 1);
                display.setTextSize(2);
                display.setCursor(58,40);
                display.println("OK");
                display.display();
                while (digitalRead(pinD2) == 0) {}
              }
              else {
                Serial.println("OK");
                display.clearDisplay();
                display.drawBitmap(0, 0,  myBitmap4, 128, 64, 1);
                display.setTextSize(2);
                display.setCursor(58,40);
                display.println("OK");
                display.display();
                sleep_time = 0;
                count_bt = 100;
                selt = 0;
                while (digitalRead(pinD2) == 0) {}
              }
            }
            if ((millis() - sleep_time) > 100) {
              count_bt++;
              sleep_time = millis();
            }
          }

          if (count_bt < 20 && selt == 0) {
            cali_value += 10;
            if (cali_value > 90) cali_value = 0;
          }
          if (count_bt < 20 && selt == 1) {
            cali_value_ += 1;
            if (cali_value_ > 9) cali_value_ = 0;
          }
          if (count_bt < 20 && selt == 2) {
            cali_value__ += 1;
            if (cali_value__ > 9) cali_value__ = 0;
          }
          cali_value_fine = cali_value + cali_value_ + cali_value__/10;
        }
        if (millis() - show_time > 200) {
          Serial.println(cali_value_fine);
          display.clearDisplay();
          display.drawBitmap(0, 0,  myBitmap4, 128, 64, 1);
          display.setTextSize(2);
          display.setCursor(58,40);
          display.println(String(cali_value_fine, 1));
          display.display();
        }
        if ((millis() - sleep_time) > 1000) {
          close_++;
          sleep_time = millis();
          Serial.println(close_);
        }
        if (close_ == 10) {
          Serial.println("Sleep!");
          display.clearDisplay();
          display.drawBitmap(0, 0,  myBitmap8, 128, 64, 1);
          display.display();
          tone(pinSPK,262);
          delay(100);
          noTone(pinSPK);
          delay(1000);
          Goto_Sleep_Power_Down();
        }
      }
      tone(pinSPK,1000);
      delay(100);
      noTone(pinSPK);
      Serial.println("please push button, read value.");
      display.clearDisplay();
      display.drawBitmap(0, 0,  myBitmap5, 128, 64, 1);
      display.display();
      while (digitalRead(pinD2) == 1) {
        if ((millis() - sleep_time) > 1000) {
          close_++;
          sleep_time = millis();
          Serial.println(close_);
        }
        if (close_ == 10) {
          Serial.println("Sleep!");
          display.clearDisplay();
          display.drawBitmap(0, 0,  myBitmap8, 128, 64, 1);
          display.display();
          tone(pinSPK,262);
          delay(100);
          noTone(pinSPK);
          delay(1000);
          Goto_Sleep_Power_Down();
        }
      }
      config_type config;
      config.average = mlx.get_object_temp() - cali_value_fine;
      EEPROM_writeAnything(address, config);
      config_type config_readback;
      EEPROM_readAnything(address, config_readback);
      for(int i=0;i<3;i++) {
        tone(pinSPK,1000);
        delay(100);
        noTone(pinSPK);
        delay(50);
      }
      Serial.println(config_readback.average);
      Serial.println("please dropout button!");
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(62,40);
      display.println(String(config.average, 1)+"*C");
      display.display();
      delay(2000);
      while (digitalRead(pinD2) == 0) {}
    }
    startgo = 0;
  }

  if ((millis() - sleep_time) > 1000) {
    close_++;
    sleep_time = millis();
    Serial.println(close_);
  }
  if (close_ == 10) {
    Serial.println("Sleep!");
    display.clearDisplay();
    display.drawBitmap(0, 0,  myBitmap8, 128, 64, 1);
    display.display();
    tone(pinSPK,262);
    delay(100);
    noTone(pinSPK);
    delay(1000);
    Goto_Sleep_Power_Down();
  }
}
