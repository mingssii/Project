#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <BlynkSimpleEsp32.h>

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

// DHT Sensor
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Rain Sensor
#define RAIN_DO 13

// Liquid Level Sensor
#define LIQUID_SENSOR_PIN 27

// Soil Moisture Sensor
#define SOIL_AO 32

// Pump Control
#define PUMP_PIN 26
bool autoMode = true; // Automatic watering enabled

// Previous sensor values
float prevHumidity = -1;
float prevTemperature = -1;
int prevRainStatus = -1;
int prevLiquidStatus = -1;
int prevSoilMoisture = -1;

void setup() {
    Serial.begin(115200);

    // Initialize sensors
    dht.begin();
    pinMode(RAIN_DO, INPUT);
    pinMode(LIQUID_SENSOR_PIN, INPUT);
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

    // Read sensor values
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int rainStatus = digitalRead(RAIN_DO);
    int liquidStatus = digitalRead(LIQUID_SENSOR_PIN);
    int soilMoisture = analogRead(SOIL_AO);

    // Auto watering logic
    if (autoMode && soilMoisture < 500 && rainStatus == 0) {
        controlPump(true);
        delay(5000); // Watering duration
        controlPump(false);
    }

    // Send to Firebase if values changed
    if (humidity != prevHumidity || temperature != prevTemperature ||
        rainStatus != prevRainStatus || liquidStatus != prevLiquidStatus ||
        soilMoisture != prevSoilMoisture) {

        FirebaseJson json;
        json.set("humidity", humidity);
        json.set("temperature", temperature);
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

        // Update previous values
        prevHumidity = humidity;
        prevTemperature = temperature;
        prevRainStatus = rainStatus;
        prevLiquidStatus = liquidStatus;
        prevSoilMoisture = soilMoisture;
    }

    // Send sensor data to Blynk
    Blynk.virtualWrite(V3, humidity);
    Blynk.virtualWrite(V4, temperature);
    Blynk.virtualWrite(V5, rainStatus);
    Blynk.virtualWrite(V6, liquidStatus);
    Blynk.virtualWrite(V7, soilMoisture);

    delay(10000); // Update every 10 seconds
}
