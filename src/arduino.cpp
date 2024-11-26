#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Soil Moisture Sensor
#define SOIL_AO 4

void setup() {
    Serial.begin(112500); // UART
    dht.begin();
}

void loop() {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int soilMoisture = analogRead(SOIL_AO);
    // ส่งข้อมูลในรูปแบบ JSON
    Serial.print("{");
    Serial.print("\"humidity\":");
    Serial.print(humidity);
    Serial.print(",\"temperature\":");
    Serial.print(temperature);
    Serial.println("}");

    delay(2000); // ส่งข้อมูลทุก 2 วินาที
}
