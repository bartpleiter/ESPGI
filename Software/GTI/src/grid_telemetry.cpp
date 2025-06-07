#include "grid_telemetry.h"
#include <Arduino.h>
#include "config.h"
#include "grid_control.h"
#include "sampling.h"
#include "sine_fitting.h"
#include "sensors.h"
#include "hardware.h"

TelemetryData telemetry_data;

void set_telemetry_data() {
  // DC measurements
  telemetry_data.dc_voltage = dc_voltage;
  telemetry_data.dc_current = dc_current;
  telemetry_data.dc_power = dc_voltage * dc_current;

  // AC/grid measurements
  telemetry_data.grid_voltage_rms = grid_voltage_sine_fit.amplitude / sqrt(2.0);
  telemetry_data.grid_voltage_amplitude = grid_voltage_sine_fit.amplitude;
  telemetry_data.ac_current_rms = inverter_current_sine_fit.amplitude / sqrt(2.0);
  telemetry_data.ac_current_amplitude = inverter_current_sine_fit.amplitude;
  telemetry_data.grid_voltage_sine_fit_phase = grid_voltage_sine_fit.phase;
  telemetry_data.inverter_current_sine_fit_phase = inverter_current_sine_fit.phase;
  
  // AC Power
  telemetry_data.power_factor = power_factor;
  telemetry_data.active_power = telemetry_data.grid_voltage_rms * telemetry_data.ac_current_rms * telemetry_data.power_factor;
  telemetry_data.reactive_power = telemetry_data.grid_voltage_rms * telemetry_data.ac_current_rms * sqrt(1.0 - telemetry_data.power_factor * telemetry_data.power_factor);

  // Control values
  int32_t phase_shift_fp = PHASE_MATCH_OFFSET_FP + phase_shift_control_fp;
  telemetry_data.phase_offset_deg = (phase_shift_fp / (float)FP_ONE) * (180.0 / PI);
  telemetry_data.amplitude_offset = amplitude_offset_control_fp / (float)FP_ONE;

  // State and control
  telemetry_data.grid_state = currentState;
  telemetry_data.shutdown_reason = shutdownReason;
  telemetry_data.cannot_tie_reason = cannotTieReason;
  telemetry_data.power_control_state = power_control_state;

  // Flags
  telemetry_data.safety_shutdown_flag = safety_shutdown_flag;
  telemetry_data.tied_to_grid = tied_to_grid;
}

String telemetry_data_to_json() {
  String json = "{";
  json += "\"dc_voltage\":" + String(telemetry_data.dc_voltage, 4) + ",";
  json += "\"dc_current\":" + String(telemetry_data.dc_current, 4) + ",";
  json += "\"dc_power\":" + String(telemetry_data.dc_power, 4) + ",";

  json += "\"grid_voltage_rms\":" + String(telemetry_data.grid_voltage_rms, 4) + ",";
  json += "\"grid_voltage_amplitude\":" + String(telemetry_data.grid_voltage_amplitude, 4) + ",";
  json += "\"ac_current_rms\":" + String(telemetry_data.ac_current_rms, 4) + ",";
  json += "\"ac_current_amplitude\":" + String(telemetry_data.ac_current_amplitude, 4) + ",";

  json += "\"grid_voltage_sine_fit_phase\":" + String(telemetry_data.grid_voltage_sine_fit_phase, 4) + ",";
  json += "\"inverter_current_sine_fit_phase\":" + String(telemetry_data.inverter_current_sine_fit_phase, 4) + ",";

  json += "\"power_factor\":" + String(telemetry_data.power_factor, 4) + ",";
  json += "\"active_power\":" + String(telemetry_data.active_power, 4) + ",";
  json += "\"reactive_power\":" + String(telemetry_data.reactive_power, 4) + ",";

  json += "\"phase_offset_deg\":" + String(telemetry_data.phase_offset_deg, 4) + ",";
  json += "\"amplitude_offset\":" + String(telemetry_data.amplitude_offset, 4) + ",";
  json += "\"grid_state\":" + String((int)telemetry_data.grid_state) + ",";

  json += "\"shutdown_reason\":" + String((int)telemetry_data.shutdown_reason) + ",";
  json += "\"cannot_tie_reason\":" + String((int)telemetry_data.cannot_tie_reason) + ",";
  json += "\"power_control_state\":" + String((int)telemetry_data.power_control_state) + ",";

  json += "\"safety_shutdown_flag\":" + String(telemetry_data.safety_shutdown_flag ? "true" : "false") + ",";
  json += "\"tied_to_grid\":" + String(telemetry_data.tied_to_grid ? "true" : "false");
  json += "}";
  return json;
}
