#pragma once

#include <Arduino.h>
#include "sine_fitting.h"
#include "grid_control.h"

struct TelemetryData {
  float dc_voltage;
  float dc_current;
  float dc_power;

  float grid_voltage_rms;
  float grid_voltage_amplitude;
  float ac_current_rms;
  float ac_current_amplitude;
  float grid_voltage_sine_fit_phase;
  float inverter_current_sine_fit_phase;

  float power_factor;
  float active_power;
  float reactive_power;

  float phase_offset_deg;
  float amplitude_offset;

  GridState grid_state;
  ShutdownReason shutdown_reason;
  CannotTieReason cannot_tie_reason;
  PowerControlState power_control_state;

  bool safety_shutdown_flag;
  bool tied_to_grid;
};

extern TelemetryData telemetry_data;

void set_telemetry_data();
String telemetry_data_to_json();
