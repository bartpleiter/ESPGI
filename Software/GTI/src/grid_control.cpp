#include "grid_control.h"
#include "grid_telemetry.h"

// -------------------- STATE VARIABLES --------------------
GridState currentState = GridState::DISCONNECTED;
ShutdownReason shutdownReason = ShutdownReason::NONE;
CannotTieReason cannotTieReason = CannotTieReason::NONE;
PowerControlState power_control_state = PowerControlState::IDLE;

// -------------------- GRID TIE VARIABLES --------------------
int consecutive_valid_conditions = 0;
bool should_try_connect = false; // Whether we should attempt to connect to the grid

// -------------------- POWER CONTROL VARIABLES --------------------
float power_factor = 0.0f;
float phase_diff = 0.0f;
float phase_difference_degrees = 0.0f;
float global_zero_phase = 0.0f;
unsigned long power_state_start_time = 0; // When the current state started
int control_step_counter = 0;  // Counter for control update steps

// Phase adjustment algorithm variables
float previous_current_amplitude = 0.0f;
float previous_phase_adjustment = 0.0f;
int phase_adjustment_direction = -1; // Start with decreasing phase

// Amplitude and phase adjustment values
float amplitude_adjustment = 0.0f;
float phase_adjustment = 0.0f;

// -------------------- INITIALIZATION --------------------
void init_grid_control() {
  consecutive_valid_conditions = 0;
  currentState = GridState::DISCONNECTED;
  shutdownReason = ShutdownReason::NONE;
  cannotTieReason = CannotTieReason::NONE;
  power_control_state = PowerControlState::IDLE;
  global_zero_phase = 0.0f;
  power_state_start_time = millis();
  control_step_counter = 0;
  
  // Reset adjustment variables
  amplitude_adjustment = 0.0f;
  phase_adjustment = 0.0f;
  previous_current_amplitude = 0.0f;
  previous_phase_adjustment = 0.0f;
  phase_adjustment_direction = -1;
}

// -------------------- GRID TIE CONDITIONS --------------------
bool checkGridTieConditions(CannotTieReason* reasonOut) {
  bool voltage_within_upper_limit = dc_voltage < MAX_DC_VOLTAGE_BEFORE_TIE;
  bool voltage_within_lower_limit = dc_voltage > MIN_DC_VOLTAGE_BEFORE_TIE;
  bool current_within_limit = (dc_current < MAX_DC_CURRENT_BEFORE_TIE) && (dc_current > -MAX_DC_CURRENT_BEFORE_TIE);
  bool dc_voltage_adequate = dc_voltage > grid_voltage_sine_fit.amplitude + DC_VOLTAGE_MARGIN_BEFORE_TIE + (amplitude_offset_control_fp / (float)FP_ONE); // Allow a small margin for safety and include offset
  bool grid_voltage_adequate = grid_voltage_sine_fit.amplitude > MIN_GRID_VOLTAGE_AMPLITUDE_BEFORE_TIE;

  bool all_conditions_met = voltage_within_upper_limit && voltage_within_lower_limit && 
                            current_within_limit && dc_voltage_adequate && grid_voltage_adequate;
  
  // If reasonOut is provided and conditions are not met, determine the reason
  if (!all_conditions_met && reasonOut != nullptr) {
    if (!voltage_within_upper_limit) {
      *reasonOut = CannotTieReason::DC_VOLTAGE_HIGH;
    } else if (!voltage_within_lower_limit) {
      *reasonOut = CannotTieReason::DC_VOLTAGE_LOW;
    } else if (!current_within_limit) {
      *reasonOut = CannotTieReason::DC_CURRENT_HIGH;
    } else if (!grid_voltage_adequate) {
      *reasonOut = CannotTieReason::GRID_VOLTAGE_LOW;
    } else if (!dc_voltage_adequate) {
      *reasonOut = CannotTieReason::DC_VOLTAGE_INADEQUATE;
    } else {
      *reasonOut = CannotTieReason::UNKNOWN;
    }
  }

  return all_conditions_met;
}

bool checkGridStayConditions(ShutdownReason* reasonOut) {
  bool voltage_within_upper_limit = dc_voltage < MAX_DC_VOLTAGE_AFTER_TIE;
  bool voltage_within_lower_limit = dc_voltage > MIN_DC_VOLTAGE_AFTER_TIE;
  bool current_within_limit = (dc_current < DC_CURRENT_THRESHOLD_AFTER_TIE) && (dc_current > -DC_CURRENT_THRESHOLD_AFTER_TIE);
  bool dc_voltage_adequate = dc_voltage > grid_voltage_sine_fit.amplitude + DC_VOLTAGE_MARGIN_AFTER_TIE + (amplitude_offset_control_fp / (float)FP_ONE); // Allow a small margin for safety and include offset
  bool grid_voltage_adequate = grid_voltage_sine_fit.amplitude > MIN_GRID_VOLTAGE_AMPLITUDE_AFTER_TIE;

  bool all_conditions_met = voltage_within_upper_limit && voltage_within_lower_limit && 
                            current_within_limit && dc_voltage_adequate && grid_voltage_adequate;
  
  // If reasonOut is provided and conditions are not met, determine the reason
  if (!all_conditions_met && reasonOut != nullptr) {
    if (!voltage_within_upper_limit) {
      *reasonOut = ShutdownReason::DC_VOLTAGE_HIGH;
    } else if (!voltage_within_lower_limit) {
      *reasonOut = ShutdownReason::DC_VOLTAGE_LOW;
    } else if (!current_within_limit) {
      *reasonOut = ShutdownReason::DC_CURRENT_HIGH;
    } else if (!grid_voltage_adequate) {
      *reasonOut = ShutdownReason::GRID_VOLTAGE_LOW;
    } else if (!dc_voltage_adequate) {
      Serial.println("DC voltage lower than grid amplitude");
      Serial.printf("DC Voltage: %.2f, Grid Amplitude: %.2f, Offset: %.2f\n",
                     dc_voltage, grid_voltage_sine_fit.amplitude, amplitude_offset_control_fp / (float)FP_ONE);
      Serial.printf("Calculated Grid Voltage + Offset + margin: %.2f\n",
                     grid_voltage_sine_fit.amplitude + DC_VOLTAGE_MARGIN_AFTER_TIE + (amplitude_offset_control_fp / (float)FP_ONE));

      *reasonOut = ShutdownReason::DC_VOLTAGE_INADEQUATE;
    } else {
      *reasonOut = ShutdownReason::UNKNOWN;
    }
  }

  return all_conditions_met;
}

// -------------------- STATE HANDLING --------------------
void handle_disconnected_state() {
  CannotTieReason reason = CannotTieReason::NONE;
  
  // Only try to connect if battery voltage is outside the medium range
  should_try_connect = (dc_voltage > BATTERY_HIGH_VOLTAGE + CONNECT_VOLTAGE_TRESHOLD) || (dc_voltage < BATTERY_LOW_VOLTAGE - CONNECT_VOLTAGE_TRESHOLD);
  
  if (should_try_connect && checkGridTieConditions(&reason)) {
    consecutive_valid_conditions++;
    if (consecutive_valid_conditions >= REQUIRED_CONSEC_CHECKS_BEFORE_TIE) {
      consecutive_valid_conditions = 0;
      tie_to_grid();
      power_state_start_time = millis(); // Reset the timer when we tie to the grid
      power_control_state = PowerControlState::IDLE; // Always start in IDLE state
      currentState = GridState::CONNECTED;
    } else {
      // Conditions are met but we need more consecutive checks
      reason = CannotTieReason::CONSECUTIVE_CHECKS_NOT_MET;
    }
  } else {
    consecutive_valid_conditions = 0;
    // Set reason if we're not trying to connect due to battery voltage
    if (!should_try_connect) {
      reason = CannotTieReason::NONE; // Not trying to connect is not an error
    }
  }
  // Update the global reason
  cannotTieReason = reason;
}

void handle_connected_state() {
  ShutdownReason reason = ShutdownReason::UNKNOWN;
  if (!checkGridStayConditions(&reason)) {
    shutdownReason = reason;  // Update the global reason
    safety_shutdown();
    currentState = GridState::SHUTDOWN;
  }

  power_control_update();
}

void control_logic() {
  switch (currentState) {
    case GridState::DISCONNECTED:
      handle_disconnected_state();
      break;
    case GridState::CONNECTED:
      handle_connected_state();
      break;
    case GridState::SHUTDOWN:
      // Nothing to do in shutdown state for now
      break;
  }
}

// -------------------- POWER CONTROL FUNCTIONS --------------------
void handle_idle_state(float& amplitude_adjustment, float& phase_adjustment) {
  // In idle state, we only adjust phase to find zero current point
  amplitude_adjustment = 0.0f;  // No amplitude adjustment in idle state
  
  // Perturb and observe algorithm for phase adjustment
  if (control_step_counter % 2 == 0) {
    // If this isn't the first iteration, compare with previous values
    if (previous_phase_adjustment != 0.0f) {
      // Check if current amplitude decreased or increased
      if (inverter_current_sine_fit.amplitude < previous_current_amplitude) {
        // Current amplitude decreased - we're going in the right direction
        // Keep the same direction
      } else {
        // Current amplitude increased or stayed the same - change direction
        phase_adjustment_direction *= -1;
      }
    }
    
    // Calculate adjustment step size based on current amplitude
    // Smaller steps when close to zero amplitude for finer control
    float adjustment_factor;
    
    if (inverter_current_sine_fit.amplitude < CURRENT_THRESHOLD_VERY_LOW) {
      adjustment_factor = PHASE_ADJ_FACTOR_VERY_SMALL; // Very small steps when very close to zero
    } else if (inverter_current_sine_fit.amplitude < CURRENT_THRESHOLD_LOW) {
      adjustment_factor = PHASE_ADJ_FACTOR_SMALL; // Small steps when close to zero
    } else if (inverter_current_sine_fit.amplitude < CURRENT_THRESHOLD_MEDIUM) {
      adjustment_factor = PHASE_ADJ_FACTOR_MEDIUM; // Medium steps when getting closer
    } else {
      adjustment_factor = PHASE_ADJ_FACTOR_LARGE; // Full steps when far from zero
    }
    
    // Apply phase adjustment in the determined direction with adaptive step size
    phase_adjustment += phase_adjustment_direction * PHASE_ADJ_BASE_STEP * adjustment_factor;
    
    // Store current values for next iteration
    previous_current_amplitude = inverter_current_sine_fit.amplitude;
    previous_phase_adjustment = phase_adjustment;
  }

  global_zero_phase = phase_adjustment; // Record the zero-crossing phase

  
  // Check if we should transition to another state
  // Must stay in idle state for at least the minimum time
  unsigned long current_time = millis();
  if ((current_time - power_state_start_time) > POWER_STATE_MIN_TIME_MS) {
    if (dc_voltage > BATTERY_HIGH_VOLTAGE) {
      // Battery voltage high, transition to discharge
      power_control_state = PowerControlState::DISCHARGE;
      power_state_start_time = current_time;
      control_step_counter = 0;
      // Initialize with no amplitude adjustment
      amplitude_adjustment = 0.0f;
      phase_adjustment = global_zero_phase;
    } 
    else if (dc_voltage < BATTERY_LOW_VOLTAGE) {
      // Battery voltage low, transition to charge
      power_control_state = PowerControlState::CHARGE;
      power_state_start_time = current_time;
      control_step_counter = 0;
      // Initialize with no amplitude adjustment
      amplitude_adjustment = 0.0f;
      phase_adjustment = global_zero_phase;
    }
    else if (dc_voltage >= BATTERY_LOW_VOLTAGE && dc_voltage <= BATTERY_HIGH_VOLTAGE) {
      // Battery voltage is in the medium range, disconnect from grid
      untie_from_grid();
      currentState = GridState::DISCONNECTED;
      power_state_start_time = current_time;
      control_step_counter = 0;
    }
  }
}

void handle_discharge_state(float& amplitude_adjustment, float& phase_adjustment) {
  // Adjust phase every 10 steps based on DC current
  if (control_step_counter % 10 == 0) {
    if (dc_current < DC_CURRENT_TARGET) {
      // Increase phase adjustment to increase current
      phase_adjustment += PHASE_ADJ_STEP;
    } else if (dc_current > DC_CURRENT_TARGET) {
      // Decrease phase adjustment to reduce current
      phase_adjustment -= PHASE_ADJ_STEP;
    }
  }
  
  // Adjust amplitude every step based on phase difference
  if (phase_difference_degrees > 0) {
    // Increase voltage to get closer to unity power factor
    amplitude_adjustment += AMPLITUDE_ADJ_STEP;
  } else if (phase_difference_degrees < 0) {
    // Decrease voltage
    amplitude_adjustment -= AMPLITUDE_ADJ_STEP;
  }
  
  // Ensure phase adjustment stays within allowed range
  phase_adjustment = constrain(phase_adjustment, global_zero_phase, global_zero_phase + MAX_PHASE_ADJ);
  
  // Check if we should go back to idle
  if (dc_voltage < BATTERY_MEDIUM_VOLTAGE) {
    power_control_state = PowerControlState::IDLE;
    power_state_start_time = millis();
    control_step_counter = 0;
  }
}

void handle_charge_state(float& amplitude_adjustment, float& phase_adjustment) {
  // Adjust phase every 10 steps based on DC current
  if (control_step_counter % 10 == 0) {
    if (dc_current < -DC_CURRENT_TARGET) {
      // Increase phase adjustment to reduce charging current magnitude
      phase_adjustment += PHASE_ADJ_STEP;
    } else if (dc_current > -DC_CURRENT_TARGET) {
      // Decrease phase adjustment to increase charging current magnitude
      phase_adjustment -= PHASE_ADJ_STEP;
    }
  }
  
  // Adjust amplitude every step based on phase difference
  if (phase_difference_degrees < 0) {
    // Increase voltage magnitude to get closer to unity power factor
    amplitude_adjustment -= AMPLITUDE_ADJ_STEP;  // More negative = higher magnitude
  } else if (phase_difference_degrees > 0) {
    // Decrease voltage magnitude
    amplitude_adjustment += AMPLITUDE_ADJ_STEP;  // Less negative = lower magnitude
  }
  
  // Ensure phase adjustment stays within allowed range
  phase_adjustment = constrain(phase_adjustment, global_zero_phase - MAX_PHASE_ADJ, global_zero_phase);
  
  // Check if we should go back to idle
  if (dc_voltage > BATTERY_MEDIUM_VOLTAGE) {
    power_control_state = PowerControlState::IDLE;
    power_state_start_time = millis();
    control_step_counter = 0;
  }
}

void power_control_update() {
  if (currentState != GridState::CONNECTED) {
    return; // No power control if not connected to the grid
  }

  // Fit current sine wave
  update_inverter_current_sine_fit();

  // Calculate phase shift and power factor
  phase_diff = calculate_phase_difference(grid_voltage_sine_fit, inverter_current_sine_fit);
  power_factor = cosf(phase_diff);
  phase_difference_degrees = phase_diff * (180.0f / PI);

  // Increment control step counter (used for slowing down some adjustments)
  control_step_counter++;
  
  // State machine for power control
  switch (power_control_state) {
    case PowerControlState::IDLE:
      handle_idle_state(amplitude_adjustment, phase_adjustment);
      break;
    
    case PowerControlState::DISCHARGE:
      handle_discharge_state(amplitude_adjustment, phase_adjustment);
      break;
    
    case PowerControlState::CHARGE:
      handle_charge_state(amplitude_adjustment, phase_adjustment);
      break;
  }

  // Constrain the adjustments to prevent excessive control signals that could blow the fuses
  amplitude_adjustment = constrain(amplitude_adjustment, -MAX_AMPLITUDE_ADJ, MAX_AMPLITUDE_ADJ);
  phase_adjustment = constrain(phase_adjustment, -MAX_PHASE_ADJ, MAX_PHASE_ADJ);
  
  // Convert to fixed-point and update the control variables
  phase_shift_control_fp = (int32_t)(phase_adjustment * FP_ONE);
  amplitude_offset_control_fp = (int32_t)(amplitude_adjustment * FP_ONE);
}

// -------------------- UTILITY FUNCTIONS --------------------
const char* getShutdownReasonString(ShutdownReason reason) {
  switch (reason) {
    case ShutdownReason::NONE:
      return "None";
    case ShutdownReason::DC_VOLTAGE_HIGH:
      return "DC Voltage High";
    case ShutdownReason::DC_VOLTAGE_LOW:
      return "DC Voltage Low";
    case ShutdownReason::DC_CURRENT_HIGH:
      return "DC Current High";
    case ShutdownReason::GRID_VOLTAGE_LOW:
      return "Grid Voltage Low";
    case ShutdownReason::DC_VOLTAGE_INADEQUATE:
      return "DC Voltage + Offset Lower Than Grid Amplitude";
    case ShutdownReason::MANUAL_SHUTDOWN:
      return "Manual Shutdown";
    case ShutdownReason::UNKNOWN:
    default:
      return "Unknown";
  }
}

const char* getCannotTieReasonString(CannotTieReason reason) {
  switch (reason) {
    case CannotTieReason::NONE:
      return "None";
    case CannotTieReason::DC_VOLTAGE_HIGH:
      return "DC Voltage Too High";
    case CannotTieReason::DC_VOLTAGE_LOW:
      return "DC Voltage Too Low";
    case CannotTieReason::DC_CURRENT_HIGH:
      return "DC Current Out of Range";
    case CannotTieReason::GRID_VOLTAGE_LOW:
      return "Grid Voltage Too Low";
    case CannotTieReason::DC_VOLTAGE_INADEQUATE:
      return "DC Voltage + Offset Lower Than Grid Amplitude";
    case CannotTieReason::CONSECUTIVE_CHECKS_NOT_MET:
      return "Waiting for Consecutive Valid Checks";
    case CannotTieReason::UNKNOWN:
    default:
      return "Unknown";
  }
}
