// ArduCAM Mini demo (C)2017 Lee
// Web: http://www.ArduCAM.com
// This program is a demo of how to use most of the functions
// of the library with ArduCAM Mini camera, and can run on any Arduino platform.
// This demo was made for ArduCAM_Mini_5MP_Plus.
// It needs to be used in combination with PC software.
// It can take photo continuously as video streaming.
//
// The demo sketch will do the following tasks:
// 1. Set the camera to JPEG output mode.
// 2. Read data from Serial port and deal with it
// 3. If receive 0x00-0x08,the resolution will be changed.
// 4. If receive 0x10,camera will capture a JPEG photo and buffer the image to FIFO.Then write datas to Serial port.
// 5. If receive 0x20,camera will capture JPEG photo and write datas continuously.Stop when receive 0x21.
// 6. If receive 0x30,camera will capture a BMP  photo and buffer the image to FIFO.Then write datas to Serial port.
// 7. If receive 0x11 ,set camera to JPEG output mode.
// 8. If receive 0x31 ,set camera to BMP  output mode.
// This program requires the ArduCAM V4.0.0 (or later) library and ArduCAM_Mini_5MP_Plus
// and use Arduino IDE 1.6.8 compiler or above
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"
//This demo can only work on OV2640_MINI_2MP or OV5642_MINI_5MP or OV5642_MINI_5MP_BIT_ROTATION_FIXED platform.
#if !(defined OV5642_MINI_5MP || defined OV5642_MINI_5MP_BIT_ROTATION_FIXED || defined OV2640_MINI_2MP || defined OV3640_MINI_3MP)
  #error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif
#define BMPIMAGEOFFSET 66
const char bmp_header[BMPIMAGEOFFSET] PROGMEM =
{
  0x42, 0x4D, 0x36, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x28, 0x00,
  0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00,
  0x00, 0x00
};
// set pin 7 as the slave select for the digital pot:
const int CS = 7;
bool is_header = false;
int mode = 2;
uint8_t start_capture = 2;

ArduCAM myCAM( OV2640, CS );

uint8_t read_fifo_burst(ArduCAM myCAM);

void setup() {
  // put your setup code here, to run once:
  uint8_t vid, pid;
  uint8_t temp;

  Wire.begin();
  Serial.begin(921600);

  Serial.println(F("ACK CMD ArduCAM Start! END"));

  // set the CS as an output:
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  // initialize SPI:
  SPI.begin();
  //Reset the CPLD
  myCAM.write_reg(0x07, 0x80);
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);
  while(1){
    //Check if the ArduCAM SPI bus is OK
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);
    if (temp != 0x55){
      Serial.println(F("ACK CMD SPI interface Error! END"));
      delay(1000);continue;
    }else{
      Serial.println(F("ACK CMD SPI interface OK. END"));break;
    }
  }

  while(1){
    //Check if the camera module type is OV2640
    myCAM.wrSensorReg8_8(0xff, 0x01);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
    if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))){
      Serial.println(F("ACK CMD Can't find OV2640 module! END"));
      delay(1000);continue;
    }
    else{
      Serial.println(F("ACK CMD OV2640 detected. END"));break;
    } 
  }

  //Change to JPEG capture mode and initialize the OV5642 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();

  myCAM.OV2640_set_JPEG_size(OV2640_320x240);

  delay(1000);
  myCAM.clear_fifo_flag();

  #if !(defined (OV2640_MINI_2MP))
  myCAM.write_reg(ARDUCHIP_FRAMES,0x00);
  #endif
}

void loop() {
  // put your main code here, to run repeatedly:
  uint8_t temp = 0xff, temp_last = 0;
  bool is_header = false;

  if (mode == 2) {
    while (1) {
      if (start_capture == 2) {
        myCAM.flush_fifo();
        myCAM.clear_fifo_flag();
        //Start capture
        myCAM.start_capture();
        start_capture = 0;
      }

      if (myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
        uint32_t length = 0;
        length = myCAM.read_fifo_length();
        if ((length >= MAX_FIFO_SIZE) | (length == 0)) {
          myCAM.clear_fifo_flag();
          start_capture = 2;
          continue;
        }

        myCAM.CS_LOW();
        myCAM.set_fifo_burst();//Set fifo burst mode
        temp =  SPI.transfer(0x00);
        length--;
        while (length--) {
          temp_last = temp;
          temp =  SPI.transfer(0x00);
          if (is_header == true) {
            Serial.write(temp);
          }
          else if ((temp == 0xD8) & (temp_last == 0xFF)) {
            is_header = true;
            // Serial.println(F("ACK IMG END"));
            Serial.write(temp_last);
            Serial.write(temp);
          }
          if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
          break;
          delayMicroseconds(15);
        }
        myCAM.CS_HIGH();
        myCAM.clear_fifo_flag();
        start_capture = 2;
        is_header = false;
      }
    }
  } 
}

uint8_t read_fifo_burst(ArduCAM myCAM){
  uint8_t temp = 0, temp_last = 0;
  uint32_t length = 0;
  length = myCAM.read_fifo_length();
  Serial.println(length, DEC);
  // 512 kb
  if (length >= MAX_FIFO_SIZE) {  
    Serial.println(F("ACK CMD Over size. END"));
    return 0;
  }
  //0 kb
  if (length == 0 ) { 
    Serial.println(F("ACK CMD Size is 0. END"));
    return 0;
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst(); // Set fifo burst mode
  temp =  SPI.transfer(0x00);
  length --;
  while ( length-- ) {
    temp_last = temp;
    temp =  SPI.transfer(0x00);
    if (is_header == true) {
      Serial.write(temp);
    }
    else if ((temp == 0xD8) & (temp_last == 0xFF)) {
      is_header = true;
      // Serial.println(F("ACK IMG END"));
      Serial.write(temp_last);
      Serial.write(temp);
    }
    if ( (temp == 0xD9) && (temp_last == 0xFF) ) // If find the end ,break while,
    break;
    delayMicroseconds(15);
  }
  myCAM.CS_HIGH();
  is_header = false;
  return 1;
}