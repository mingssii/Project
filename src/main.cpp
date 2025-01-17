#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <SoftwareSerial.h>
#include <ArduinoJson.h> 
#include <DHT.h>

// UART
#define MYPORT_TX 18
#define MYPORT_RX 19
EspSoftwareSerial::UART arduinoPort;


// WiFi
const char* ssid = "ssii";
const char* password = "ming123459sa";

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

//DHT
DHT dht(21, DHT11);

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
bool pumpState = false;
int soilMoistureCheck = 500;
int h_error = 0;
int f_error = 0;
//UART
void readFromUART() {
    static String inputString = "";
    while (arduinoPort.available()) {
        char c = arduinoPort.read();
        inputString += c;

        // ตรวจจับข้อความจบด้วย '}'
        if (c == '}') {
            // ตัด '{' และ '}' ออก
            inputString.remove(0, 1); // ลบ '{'
            while (inputString.length() > 4) {
                if (inputString[0] < '0' || inputString[0] >'2') {
                    inputString.remove(0, 1);
                }
                else {
                    break;
                }
            }
            inputString.remove(inputString.length() - 1); // ลบ '}'
            // แยกข้อมูลด้วยเครื่องหมายจุลภาค
            int comma1 = inputString.indexOf(',');

            if (comma1 != -1) {
                // แยกค่า

                String rainStatusStr = inputString.substring(0, comma1);
                String liquidStatusStr = inputString.substring(comma1 + 1);
                // แปลงเป็นตัวเลข
                int rrainStatus = rainStatusStr.toInt();
                int rliquidStatus = liquidStatusStr.toInt();

                // พิมพ์ค่าที่รับมาเพื่อตรวจสอบ
                Serial.print("RainStatus from UART: ");
                Serial.println(rrainStatus);
                Serial.print("LiquidStatus from UART: ");
                Serial.println(rliquidStatus);
                // อัปเดตค่าที่รับมา
                if (rainStatus >= 2 && rrainStatus <= 1) {
                    Blynk.logEvent("rain", "OMG It's Raining");
                }
                if (liquidStatus == 1 && rliquidStatus == 0) {
                    Blynk.logEvent("liquid", "OMG It's out of water");
                    Serial.println("liquid error");
                }
                rainStatus = rrainStatus;
                liquidStatus = rliquidStatus;
            }
            else {
                Serial.println("Error: Invalid data format");
            }

            // ล้างข้อมูลที่อ่าน
            inputString = "";
        }
    }
}

void setup() {
    Serial.begin(115200);
    dht.begin();
    //UART
    arduinoPort.begin(9600, SWSERIAL_8N1, MYPORT_RX, MYPORT_TX, false);
    if (!arduinoPort) { // If the object did not initialize, then its configuration is invalid
        Serial.println("Invalid EspSoftwareSerial pin configuration, check config");
        while (1) { // Don't continue with invalid configuration
            delay(1000);
        }
    }
    // Initialize sensors
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, HIGH); // Turn off pump initially

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
    digitalWrite(PUMP_PIN, state ? LOW : HIGH);
    Serial.println(state ? "Pump ON" : "Pump OFF");
}

// Blynk Virtual Pin for manual control of the pump
BLYNK_WRITE(V1) {
    int pumpControl = param.asInt();
    pumpState = pumpControl;
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
    int soilMoisture = analogRead(SOIL_AO);
    float nhumidity = dht.readHumidity();
    float ntemperature = dht.readTemperature();
    if (!isnan(nhumidity) && !isnan(ntemperature)) {
        humidity = nhumidity;
        temperature = ntemperature;
    }
    else {
        h_error += 1;
        if (h_error >= 20) {
            Blynk.logEvent("humidity", "sensor humidity need to be checked");
            h_error = 0;
        }
    }
    // Auto watering logic
    if (autoMode) {
        if (liquidStatus == 1 && soilMoisture < soilMoistureCheck && temperature > 5 && humidity < 70 && rainStatus > 0) {
            controlPump(true);
        }
        else {
            controlPump(false);
        }
    }
    else {
        controlPump(pumpState);
    }


    // Send to Firebase if values changed
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
        f_error += 1;
        Serial.print("Firebase update failed: ");
        if (f_error > 20) {
            Blynk.logEvent("firebase", "Some sensor might error");
            f_error = 0;
        }
        Serial.println(fbdo.errorReason());
        Serial.println(humidity);
        Serial.println(temperature);
        Serial.println(soilMoisture);
        Serial.println(rainStatus);
        Serial.println(liquidStatus);
    }
    Serial.println(humidity);
    Serial.println(temperature);
    Serial.println(soilMoisture);
    Serial.println(rainStatus);
    Serial.println(liquidStatus);

    // Send sensor data to Blynk
    Blynk.virtualWrite(V3, humidity);
    Blynk.virtualWrite(V4, temperature);
    Blynk.virtualWrite(V5, rainStatus);
    Blynk.virtualWrite(V6, liquidStatus);
    Blynk.virtualWrite(V7, soilMoisture);

    delay(3000); // Update every 10 seconds
}
