#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <BlynkSimpleEsp32.h>
#include <ArduinoJson.h> 
#include <SoftwareSerial.h>

// UART
#define MYPORT_TX 22
#define MYPORT_RX 23
EspSoftwareSerial::UART myPort;

// WiFi
const char* ssid = "ssii";
const char* password = "ming123459sa";

// Firebase
#define API_KEY "YOUR_API_KEY"
#define DATABASE_URL "YOUR_DATABASE_URL"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// Blynk
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"
#define BLYNK_PRINT Serial

// Rain Sensor
#define RAIN_DO 13

// Liquid Level Sensor
#define LIQUID_SENSOR_PIN 27

// Pump Control
#define PUMP_PIN 26
bool autoMode = true; // Automatic watering enabled

// sensor values
float humidity = -1;
float temperature = -1;
int rainStatus = -1;
int liquidStatus = -1;
int soilMoisture = -1;

//UART
void readFromUART() {
    static String inputString = "";
    while (myPort.available()) {
        char c = myPort.read();
        inputString += c;

        // ตรวจจับ JSON ข้อมูล (ถ้าจบด้วย "}")
        if (c == '}') {
            // แปลงข้อมูลเป็น JSON
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, inputString);
            if (!error) {
                // ดึงค่าจาก JSON
                float rhumidity = doc["humidity"];
                float rtemperature = doc["temperature"];
                int rsoilMoisture = doc["soilMoisture"];
                // พิมพ์ค่าที่รับมาเพื่อเช็ค
                Serial.print("Humidity from UART: ");
                Serial.println(rhumidity);
                Serial.print("Temperature from UART: ");
                Serial.println(rtemperature);
                Serial.print("soilMoisture from UART: ");
                Serial.println(rsoilMoisture);
                // ใช้ค่าที่รับมา
                humidity = rhumidity;
                temperature = rtemperature;
                soilMoisture = rsoilMoisture;
            }
            else {
                Serial.println("Failed to parse JSON from UART");
            }
            inputString = ""; // ล้างข้อความหลังแปลงเสร็จ
        }
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(RAIN_DO, INPUT);
    pinMode(LIQUID_SENSOR_PIN, INPUT);
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW); // Turn off pump initially

    //UART
    myPort.begin(115200, SWSERIAL_8N1, MYPORT_RX, MYPORT_TX, false);
    if (!myPort) { // If the object did not initialize, then its configuration is invalid
        Serial.println("Invalid EspSoftwareSerial pin configuration, check config");
        while (1) { // Don't continue with invalid configuration
            delay(1000);
        }
    }

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println("\nConnected to WiFi");
    Serial.println(WiFi.localIP());

    // Firebase setup
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    if (Firebase.signUp(&config, &auth, "", "")) {
        signupOK = true;
    }
    else {
        Serial.printf("Firebase Sign-Up Failed: %s\n", config.signer.signupError.message.c_str());
    }
    config.token_status_callback = tokenStatusCallback;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    // Blynk setup
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
}

// Function to control the water pump
void controlPump(bool state) {
    digitalWrite(PUMP_PIN, state ? HIGH : LOW);
    Serial.println(state ? "Pump ON" : "Pump OFF");
}

// Blynk Virtual Pin for manual control of the pump
BLYNK_WRITE(V1) {
    int pumpControl = param.asInt();
    controlPump(pumpControl);
}

// Blynk Virtual Pin for toggling automatic mode
BLYNK_WRITE(V2) {
    autoMode = param.asInt();
    Serial.println(autoMode ? "Auto Mode Enabled" : "Auto Mode Disabled");
}

void loop() {
    Blynk.run();

    // อ่านข้อมูลจาก Arduino Uno ผ่าน UART
    readFromUART();

    // อ่านค่าจากเซ็นเซอร์ที่อยู่บน ESP32
    int rainStatus = digitalRead(RAIN_DO);
    int liquidStatus = digitalRead(LIQUID_SENSOR_PIN);
    

    // Auto watering logic
    if (autoMode && soilMoisture < 500 && rainStatus == 0) {
        controlPump(true);
        delay(5000); // ระยะเวลาเปิดปั๊ม
        controlPump(false);
    }

    // ส่งข้อมูลไป Firebase และ Blynk
    FirebaseJson json;
    json.set("humidity", humidity);          // จาก UART
    json.set("temperature", temperature);    // จาก UART
    json.set("rainStatus", rainStatus);
    json.set("liquidStatus", liquidStatus);
    json.set("soilMoisture", soilMoisture);

    if (Firebase.RTDB.setJSON(&fbdo, "/sensors", &json)) {
        Serial.println("Firebase update successful");
    }
    else {
        Serial.print("Firebase update failed: ");
        Serial.println(fbdo.errorReason());
    }

    // ส่งข้อมูลไปที่ Blynk
    Blynk.virtualWrite(V3, humidity);       // ความชื้นจาก UART
    Blynk.virtualWrite(V4, temperature);    // อุณหภูมิจาก UART
    Blynk.virtualWrite(V5, rainStatus);
    Blynk.virtualWrite(V6, liquidStatus);
    Blynk.virtualWrite(V7, soilMoisture);

    delay(10000); // อัปเดตทุก 10 วินาที
}

