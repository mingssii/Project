#include <Arduino.h>

//Rain Sensor
#define RAIN_AO A0   // ขา Analog

// กำหนดขา Liquid Level Sensor
#define LIQUID_SENSOR_PIN 3  // ขา Digital

const int sensorMin = 0;     // sensor minimum
const int sensorMax = 1024;  // sensor maximum

void setup() {
  Serial.begin(9600);
}

void loop() {
  // อ่านค่าจาก Raindrop Sensor
  int sensorReading = analogRead(RAIN_AO);
  // map the sensor range (four options):
  // ex: 'long int map(long int, long int, long int, long int, long int)'
  int rainStatus = map(sensorReading, sensorMin, sensorMax, 0, 3);
  // Serial.print("Rain Analog Status: ");
  // Serial.print(range);
  // Serial.print(" - ");

  // // range value:
  // switch (range) {
  // case 0:    // Sensor getting wet
  //   Serial.println("Rain Warning");
  //   break;
  // default:
  //   Serial.println("Not Raining");
  // }

  // อ่านค่าจาก Liquid Level Sensor
  int liquidStatus = digitalRead(LIQUID_SENSOR_PIN);

  // ส่งข้อมูลในรูปแบบ JSON
  Serial.print("{");
  Serial.print(rainStatus);
  Serial.print(",");
  Serial.print(liquidStatus);
  Serial.println("}");
  delay(3000); // ส่งข้อมูลทุก 5 วินาที


}