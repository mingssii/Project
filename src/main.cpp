#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <SoftwareSerial.h>
#include <ArduinoJson.h> 

// UART
#define MYPORT_TX 18
#define MYPORT_RX 19
EspSoftwareSerial::UART arduinoPort;


// WiFi
const char* ssid = "Mix";
const char* password = "wifi1234";

// Firebase
#define API_KEY "AIzaSyDxjY-g-_C4hG0mi5uqpr8uWqn1SaY6j2E"
#define DATABASE_URL "https://smartfarm-85477-default-rtdb.asia-southeast1.firebasedatabase.app/"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// Blynk
#define BLYNK_TEMPLATE_ID "TMPL64wgnVBDg"
#define BLYNK_TEMPLATE_NAME "EmbededProject"
#define BLYNK_AUTH_TOKEN "AZ9oKspUFAR7B8A7_t4brXV8Iainwpap"
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp32.h>

// DHT Sensor
#define DHTPIN 21
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Soil Moisture Sensor
#define SOIL_AO 32

// Pump Control
#define PUMP_PIN 23
bool autoMode = true; // Automatic watering enabled

// sensor values
float humidity = -1;
float temperature = -1;
int soilMoisture = -1;
int rainStatus = -1;
int liquidStatus = -1;


//UART
void readFromUART() {
    static String inputString = "";
    while (arduinoPort.available()) {
        char c = arduinoPort.read();
        inputString += c;

        // ตรวจจับ JSON ข้อมูล (ถ้าจบด้วย "}")
        if (c == '}') {
            // แปลงข้อมูลเป็น JSON
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, inputString);
            if (!error) {
                // ดึงค่าจาก JSON
                int rrainStatus = doc["rainStatus"];
                int rliquidStatus = doc["liquidStatus"];
                // พิมพ์ค่าที่รับมาเพื่อเช็ค
                Serial.print("RainStatus from UART: ");
                Serial.println(rrainStatus);
                Serial.print("LiquidStatus from UART: ");
                Serial.println(rliquidStatus);
                // ใช้ค่าที่รับมา
                rainStatus = rrainStatus;
                liquidStatus = rliquidStatus;
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
    //UART
    arduinoPort.begin(115200, SWSERIAL_8N1, MYPORT_RX, MYPORT_TX, false);
    if (!arduinoPort) { // If the object did not initialize, then its configuration is invalid
        Serial.println("Invalid EspSoftwareSerial pin configuration, check config");
        while (1) { // Don't continue with invalid configuration
            delay(1000);
        }
    }
    // Initialize sensors
    dht.begin();
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW); // Turn off pump initially

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
    readFromUART();
    // Read sensor values
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int soilMoisture = analogRead(SOIL_AO);

    // Auto watering logic
    if (autoMode && soilMoisture < 500) {
        controlPump(true);
        delay(5000); // Watering duration
        controlPump(false);
    }

    // Send to Firebase if values changed
    FirebaseJson json;
    json.set("humidity", humidity);
    json.set("temperature", temperature);
    json.set("rainStatus", rainStatus);
    json.set("liquidStatus", liquidStatus);
    json.set("soilMoisture", soilMoisture);
    Serial.print(soilMoisture);
    if (Firebase.RTDB.setJSON(&fbdo, "/sensors", &json)) {
        Serial.println("Firebase update successful");
    }
    else {
        Serial.print("Firebase update failed: ");
        Serial.println(fbdo.errorReason());
        Serial.println(humidity);
        Serial.println(temperature);
        Serial.println(soilMoisture);
        Serial.println(rainStatus);
        Serial.println(liquidStatus);
    }

    // Send sensor data to Blynk
    Blynk.virtualWrite(V3, humidity);
    Blynk.virtualWrite(V4, temperature);
    Blynk.virtualWrite(V5, rainStatus);
    Blynk.virtualWrite(V6, liquidStatus);
    Blynk.virtualWrite(V7, soilMoisture);

    delay(5000); // Update every 10 seconds
}
