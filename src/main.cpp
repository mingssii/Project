#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// กำหนดขา DHT11
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// กำหนดขา Raindrop Sensor
#define RAIN_DO 13   // ขา Digital

// กำหนดขา Liquid Level Sensor
#define LIQUID_SENSOR_PIN 27  // ขา Digital

// กำหนดขา Soil Moisture Sensor
#define SOIL_AO 32  // ขา Analog

void setup() {
  Serial.begin(115200);

  // เริ่มต้นเซ็นเซอร์ DHT
  dht.begin();

  // ตั้งค่าขา Input สำหรับ Digital Sensors
  pinMode(RAIN_DO, INPUT);
  pinMode(LIQUID_SENSOR_PIN, INPUT);
}

void loop() {
  // อ่านค่าจาก DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  }
  else {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%  Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");
  }

  // อ่านค่าจาก Raindrop Sensor
  int rainDigital = digitalRead(RAIN_DO);

  Serial.print("  Rain Digital Status: ");
  if (rainDigital == LOW) {
    Serial.println("Rain detected!");
  }
  else {
    Serial.println("No rain.");
  }

  // อ่านค่าจาก Liquid Level Sensor
  int liquidStatus = digitalRead(LIQUID_SENSOR_PIN);

  Serial.print("Liquid Level Sensor: ");
  if (liquidStatus == HIGH) {
    Serial.println("Liquid detected!");
  }
  else {
    Serial.println("No liquid detected.");
  }

  // อ่านค่าจาก Soil Moisture Sensor
  int soilAnalog = analogRead(SOIL_AO);

  Serial.print("Soil Moisture Analog Value: ");
  Serial.println(soilAnalog);

  delay(10000); // รอ 10 วินาที
}
