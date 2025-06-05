/* TODO for refactor:
- Consistent camelCase vs snake_case for function names
*/

#include <Arduino.h>
#include <esp_timer.h>
#include <Wire.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "sensors.h"
#include "config.h"
#include "hardware.h"
#include "sampling.h"
#include "sine_fitting.h"
#include "util.h"
#include "grid_control.h"
#include "pwm_control.h"
#include "tasks.h"

// Timing variables
unsigned long last_ac_sample_time = 0;
unsigned long last_dc_sample_time = 0;
unsigned long last_grid_sine_fit_time = 0;

// Core 1 (critical) main loop
void loop()
{
  unsigned long current_time = esp_timer_get_time();

  // Sample the AC signals
  if (current_time - last_ac_sample_time >= INTERVAL_AC_SAMPLE_MICROS)
  {
    last_ac_sample_time = current_time;
    unsigned long start_time = esp_timer_get_time();
    sample_ac_signals();
    exec_time_ac_sample = esp_timer_get_time() - start_time;
  }

  // Sample the DC signals
  if (current_time - last_dc_sample_time >= INTERVAL_DC_SAMPLE_MICROS)
  {
    last_dc_sample_time = current_time;
    unsigned long start_time = esp_timer_get_time();
    sample_dc_signals();
    exec_time_dc_sample = esp_timer_get_time() - start_time;
  }

  // Update the grid voltage sine fit
  if (current_time - last_grid_sine_fit_time >= INTERVAL_GRID_SINE_FIT_MICROS)
  {
    last_grid_sine_fit_time = current_time;
    unsigned long start_time = esp_timer_get_time();
    update_grid_voltage_sine_fit();
    exec_time_grid_sine_fit = esp_timer_get_time() - start_time;
  }
}

void setup()
{
  Serial.begin(115200);
  setup_hardware();
  init_sensors(Wire, Wire1);
  init_pwm_control();
  init_grid_control();
  init_tasks();
}
