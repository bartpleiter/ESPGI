#include <Arduino.h>
#include "telemetry.h"
#include "webserver.h"
#include "config.h"
#include <WiFi.h>

void telemetryTask(void *parameter) {
  while (true) {
    set_telemetry_data();
    vTaskDelay(TELEMETRY_UPDATE_INTERVAL_MS / portTICK_PERIOD_MS); // 1 second
  }
}

void wifiTask(void *parameter) {
  while (true) {
    ensure_wifi_connected();
    vTaskDelay(WIFI_CHECK_INTERVAL_MS / portTICK_PERIOD_MS);
  }
}

void start_telemetry_task() {
  xTaskCreatePinnedToCore(
    telemetryTask,         // Task function
    "TelemetryTask",      // Name
    8192,                 // Stack size
    NULL,                 // Parameter
    2,                    // Priority
    NULL,                 // Task handle
    0                     // Core 0
  );
}

void start_wifi_monitor_task() {
  xTaskCreatePinnedToCore(
    wifiTask,            // Task function
    "WiFiTask",          // Name
    8192,                // Stack size
    NULL,                // Parameter
    1,                   // Priority
    NULL,                // Task handle
    0                    // Core 0
  );
}
