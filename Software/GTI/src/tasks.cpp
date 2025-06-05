#include "tasks.h"
#include "sampling.h"
#include "sine_fitting.h"
#include "grid_control.h"
#include "hardware.h"

unsigned long exec_time_ac_sample = 0;
unsigned long exec_time_dc_sample = 0;
unsigned long exec_time_grid_sine_fit = 0;

void debug_print() {
  if (safety_shutdown_flag) {
    Serial.println("Safety shutdown is active!");
  }

  Serial.print("DC: ");
  Serial.print(dc_voltage);
  Serial.print(" V, ");
  Serial.print(dc_current);
  Serial.println(" A");

  Serial.print("Grid ampl: ");
  Serial.print(grid_voltage_sine_fit.amplitude);
  Serial.println(" V");

  // Print execution timing information
  // Serial.print("Exec time (µs) - AC: ");
  // Serial.print(exec_time_ac_sample);
  // Serial.print(", DC: ");
  // Serial.print(exec_time_dc_sample);
  // Serial.print(", Sine fit: ");
  // Serial.println(exec_time_grid_sine_fit);

  // Print reason for not tying to grid and shutdown
  if (currentState == GridState::DISCONNECTED) {
    Serial.print("Cannot tie reason: ");
    Serial.println(getCannotTieReasonString(cannotTieReason));
  } else if (currentState == GridState::SHUTDOWN) {
    Serial.print("Shutdown reason: ");
    Serial.println(getShutdownReasonString(shutdownReason));
  }

  // If in connected state, print power factor and phase difference
  if (currentState == GridState::CONNECTED) {
    Serial.print("PF: ");
    Serial.println(power_factor, 3);
    Serial.print("Phase diff (deg): ");
    Serial.println(phase_difference_degrees, 3);
  }

  // Print control values
  Serial.print("Phase shift control: ");
  Serial.println(phase_shift_control_fp / (float)FP_ONE, 3);
  Serial.print("Amplitude offset control: ");
  Serial.println(amplitude_offset_control_fp / (float)FP_ONE, 3);

  // Print current power_control_state
  Serial.print("Power control state: ");
  switch (power_control_state) {
    case PowerControlState::IDLE:
      Serial.println("IDLE");
      break;
    case PowerControlState::CHARGE:
      Serial.println("CHARGE");
      break;
    case PowerControlState::DISCHARGE:
      Serial.println("DISCHARGE");
      break;
    default:
      Serial.println("UNKNOWN");
  }
  

  Serial.println("----");
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
      2,                  // Task priority (higher is more important)
      NULL,               // Task handle
      0                   // Core to run on (0)
  );
}
