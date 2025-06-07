#include <Arduino.h>
#include "wifi_manager.h"
#include "hardware.h"
#include "config.h"
#include "grid_telemetry.h"

IPAddress local_IP(192, 168, 0, 236);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(1, 1, 1, 1);

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

AsyncWebServer server(WEB_SERVER_PORT);
int wifiReconnectAttempts = 0;
unsigned long wifiCheckPrev = 0;

void setupWiFiManager() {
    // Configure static IP
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("Failed to configure static IP");
    }

    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Setup web server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hello from ESP32!");
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = telemetry_data_to_json();
        request->send(200, "application/json", json);
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not Found");
    });

    server.begin();
}

void ensureWiFiConnected() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, trying to reconnect...");
        WiFi.disconnect();
        WiFi.begin(ssid, password);

        unsigned long startAttemptTime = millis();

        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
            delay(100);
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Reconnected to WiFi");
            wifiReconnectAttempts = 0;
        } else {
            wifiReconnectAttempts++;
            Serial.printf("Failed to reconnect (attempt %d/%d)\n", wifiReconnectAttempts, MAX_WIFI_RETRIES);

            if (wifiReconnectAttempts >= MAX_WIFI_RETRIES) {
                Serial.println("Too many failed attempts. Restarting...");
                safety_shutdown();
                delay(200);
                ESP.restart();
            }
        }
    }
}
