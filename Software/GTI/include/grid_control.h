#pragma once

#include <Arduino.h>
#include "config.h"
#include "hardware.h"
#include "sampling.h"
#include "sine_fitting.h"

// -------------------- ENUMS --------------------
enum class GridState {
  DISCONNECTED,
  CONNECTED,
  SHUTDOWN
};

enum class ShutdownReason {
  NONE,
  DC_VOLTAGE_HIGH,
  DC_VOLTAGE_LOW,
  DC_CURRENT_HIGH,
  GRID_VOLTAGE_LOW,
  DC_VOLTAGE_INADEQUATE,
  MANUAL_SHUTDOWN,
  UNKNOWN
};

enum class CannotTieReason {
  NONE,
  DC_VOLTAGE_HIGH,
  DC_VOLTAGE_LOW,
  DC_CURRENT_HIGH,
  GRID_VOLTAGE_LOW,
  DC_VOLTAGE_INADEQUATE,
  CONSECUTIVE_CHECKS_NOT_MET,
  UNKNOWN
};

enum class PowerControlState {
  IDLE,
  CHARGE,
  DISCHARGE
};

// -------------------- FUNCTION DECLARATIONS --------------------
// Initialization
void init_grid_control();

// Grid tie conditions
bool checkGridTieConditions(CannotTieReason* reasonOut = nullptr);
bool checkGridStayConditions(ShutdownReason* reasonOut = nullptr);

// State handlers
void handle_disconnected_state();
void handle_connected_state();
void control_logic();

// Power control functions
void handle_idle_state(float& amplitude_adjustment, float& phase_adjustment);
void handle_discharge_state(float& amplitude_adjustment, float& phase_adjustment);
void handle_charge_state(float& amplitude_adjustment, float& phase_adjustment);
void power_control_update();

// Utility functions
const char* getShutdownReasonString(ShutdownReason reason);
const char* getCannotTieReasonString(CannotTieReason reason);

// -------------------- EXTERNAL VARIABLES --------------------
// State variables
extern GridState currentState;
extern ShutdownReason shutdownReason;
extern CannotTieReason cannotTieReason;
extern PowerControlState power_control_state;

// Grid tie variables
extern int consecutive_valid_conditions;
extern bool should_try_connect;

// Power control variables
extern float power_factor;
extern float phase_diff;
extern float phase_difference_degrees;
extern float global_zero_phase;
extern unsigned long power_state_start_time;
extern int control_step_counter;
