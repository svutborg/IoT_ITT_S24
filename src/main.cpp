#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <ArduinoJson.h>

#define CURRENT_VERSION "0.1.0"

const char* ssid = "KP66";
const char* password = "25072015";

const char* version_url = "https://raw.githubusercontent.com/svutborg/IoT_ITT_S24/refs/heads/master/firmware/version.json";

void performOTA(String binUrl) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    https.begin(client, binUrl);
    int httpCode = https.GET();

    if (httpCode == 200) {
        int contentLength = https.getSize();
        bool canBegin = Update.begin(contentLength);

        if (canBegin) {
            WiFiClient* stream = https.getStreamPtr();
            Update.writeStream(*stream);
            if (Update.end()) {
                Serial.println("OTA done. Rebooting...");
                ESP.restart();
            }
        }
    }
    https.end();
}


void setup() {
    Serial.begin(115200);
    Serial.println("Firmware version: " CURRENT_VERSION);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);

    WiFiClientSecure client;
    client.setInsecure(); // For dev only

    HTTPClient https;
    https.begin(client, version_url);
    int httpCode = https.GET();

    if (httpCode == 200) {
        String payload = https.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);

        String latestVersion = doc["version"];
        String binUrl = doc["bin_url"];

        if (latestVersion != CURRENT_VERSION) {
            Serial.println("New version found. Starting OTA...");
            https.end();
            performOTA(binUrl);
        } else {
            Serial.println("Already up to date.");
        }
    }
    https.end();
}

void loop() {
    delay(10);
}