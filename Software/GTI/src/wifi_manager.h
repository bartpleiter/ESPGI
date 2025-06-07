#pragma once

#define WEB_SERVER_PORT 80
#define WIFI_CHECK_INTERVAL_MS 10000 // 10s
#define MAX_WIFI_RETRIES 10

#include <WiFi.h>
#include <ESPAsyncWebServer.h>

void setupWiFiManager();
void ensureWiFiConnected();
void wifiTask(void *parameter);

