#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <ArduinoJson.h>

#define CHECK_INTERVAL_MS 60000 // One minute in ms
#define LED_PIN 2
const char* ssid = "KP66";
const char* password = "25072015";
unsigned long lastCheck = 0;

String current_version = "0.1.9";
String version_url = "https://raw.githubusercontent.com/svutborg/IoT_ITT_S24/master/firmware/version.json";

DynamicJsonDocument retrieve_version_information(String URL) {
    WiFiClientSecure client; // Creating WiFi client
    client.setInsecure(); // TODO: For dev only, change before production

    HTTPClient https; // Creating HTTP client
    https.begin(client, URL);
    int httpCode = https.GET(); // Send get request
    DynamicJsonDocument doc(1024);

    if (httpCode == 200) { // Status code OK, GET request succeeded
        String payload = https.getString();
        deserializeJson(doc, payload);
    } else {
        doc["version"] = current_version; // Set current version
        Serial.println("Failed to connect.");
    }
    https.end();
    return doc; // Returning playload, if available

}

void performOTA(String binUrl) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    https.begin(client, binUrl);
    int httpCode = https.GET(); // Send get request

    if (httpCode == 200) { // Status code OK, GET request succeeded
        int contentLength = https.getSize();
        bool canBegin = Update.begin(contentLength);

        if (canBegin) {
            WiFiClient* stream = https.getStreamPtr();
            Update.writeStream(*stream);
            if (Update.end()) {
                Serial.println("OTA done. Rebooting...");
                delay(500); // Allow message to be transmitted before rebooting
                ESP.restart();
            }
        }
    } else {
        Serial.println("Failed to retrieve firmware file...");
    }
    https.end();
}

void check_for_update() {
    unsigned int connection_timeout_counter = 0;
    Serial.println("Checking for updates...");
    Serial.println("Current firmware version: " + current_version);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi: " + String(ssid) + " ");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500); // Stuck in loop until connection to network sucessful
        Serial.print(".");
        connection_timeout_counter++;
        if (connection_timeout_counter >= 20) {
            Serial.println("\nCould not connect to WiFi");
            Serial.println("Aborting update check");
            return;
        }
    }
    Serial.println(""); // Adding newline when connection established

    /*
     * Retrieving the latest version from online repository
     * If newer version exists, fetch it
     */
    DynamicJsonDocument version_information = retrieve_version_information(version_url + String("?t=") + String(millis()));
    String latestVersion = version_information["version"];
    if (latestVersion != current_version) {
        Serial.println("New version found: " + latestVersion + ".\nStarting OTA...");
        performOTA(version_information["bin_url"]); // Passing url to the new firmware version
    } else {
        Serial.println("Already up to date.");
    }

}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);

    check_for_update();

    Serial.println("Running main code.");
}

void loop() {
    if (millis() > lastCheck + CHECK_INTERVAL_MS) {
        lastCheck = millis();
        check_for_update();
    }

    digitalWrite(LED_PIN, HIGH);
    delay(900);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    for (int i = 0; i < 4; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }

}