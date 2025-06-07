#include "tasks.h"
#include "sampling.h"
#include "sine_fitting.h"
#include "grid_control.h"
#include "hardware.h"
#include "wifi_manager.h"
#include "config.h"
#include "grid_telemetry.h"

unsigned long exec_time_ac_sample = 0;
unsigned long exec_time_dc_sample = 0;
unsigned long exec_time_grid_sine_fit = 0;

void debug_print() {
  return; // Disable debug print for now
}

void debugTask(void *parameter) {
  while (true) {
    debug_print();
    vTaskDelay(INTERVAL_DEBUG_PRINT_MS / portTICK_PERIOD_MS);
  }
}

void control_logic_task(void *parameter) {
  while (true) {
    control_logic();
    vTaskDelay(INTERVAL_CONTROL_LOGIC_MS / portTICK_PERIOD_MS);
  }
}

void wifiTask(void *parameter) {
  while (true) {
    ensureWiFiConnected();
    vTaskDelay(WIFI_CHECK_INTERVAL_MS / portTICK_PERIOD_MS);
  }
}

void telemetryTask(void *parameter) {
  while (true) {
    set_telemetry_data();
    vTaskDelay(INTERVAL_TELEMETRY_MS / portTICK_PERIOD_MS);
  }
}

void init_tasks() {
  // Create the debug task on Core 0
  xTaskCreatePinnedToCore(
      debugTask,   // Task function
      "DebugTask", // Name of task
      2048,        // Stack size (bytes)
      NULL,        // Parameter to pass
      1,           // Task priority (higher is more important)
      NULL,        // Task handle
      0            // Core to run on (0)
  );

  // Create the control logic task on Core 0
  xTaskCreatePinnedToCore(
      control_logic_task, // Task function
      "ControlLogicTask", // Name of task
      16384,              // Stack size (bytes)
      NULL,               // Parameter to pass
      3,                  // Task priority (higher is more important)
      NULL,               // Task handle
      0                   // Core to run on (0)
  );

  // Create the WiFi task on Core 0
  xTaskCreatePinnedToCore(
      wifiTask,       // Task function
      "WiFiTask",     // Name of the task
      8192,           // Stack size (in words)
      NULL,           // Task input parameter
      2,              // Priority of the task
      NULL,           // Task handle
      0               // Core to run the task on
    );

  // Create the Telemetry task on Core 0
  xTaskCreatePinnedToCore(
      telemetryTask,      // Task function
      "TelemetryTask",    // Name of the task
      8192,               // Stack size (bytes)
      NULL,               // Parameter to pass
      1,                  // Task priority
      NULL,               // Task handle
      0                   // Core to run on (0)
  );
}
