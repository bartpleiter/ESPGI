#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "webserver.h"
#include "config.h"
#include "telemetry.h"
#include "tasks.h"

#ifndef WIFI_SSID
#error "Wi-Fi SSID not defined! Set WIFI_SSID in environment."
#endif

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

// Network configuration objects
IPAddress local_IP = IPAddress LOCAL_IP;
IPAddress gateway = IPAddress GATEWAY;
IPAddress subnet = IPAddress SUBNET;
IPAddress primaryDNS = IPAddress PRIMARY_DNS;
IPAddress secondaryDNS = IPAddress SECONDARY_DNS;

AsyncWebServer server(WEB_SERVER_PORT);

int wifi_reconnect_attempts = 0;

void setup_wifi() {
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
}

void ensure_wifi_connected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, trying to reconnect...");
    WiFi.disconnect();  // Helps with flaky reconnects
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();

    // Retry for up to 5 seconds
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
      delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Reconnected to WiFi");
      wifi_reconnect_attempts = 0;  // Reset counter on success
    } else {
      wifi_reconnect_attempts++;
      Serial.printf("Failed to reconnect (attempt %d/%d)\n", wifi_reconnect_attempts, MAX_WIFI_RETRIES);

      if (wifi_reconnect_attempts >= MAX_WIFI_RETRIES) {
        Serial.println("Too many failed attempts. Restarting...");
        delay(200);  // Brief pause to flush serial output
        ESP.restart();  // Reset the ESP32
      }
    }
  }
}

void setup_webserver() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "MPPT Controller is running");
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", telemetry_json());
  });
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not Found");
  });
  server.begin();
}
