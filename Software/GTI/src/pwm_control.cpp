#include "pwm_control.h"
#include "hardware.h"
#include <esp_timer.h>
#include "config.h"
#include "sine_fitting.h"
#include "grid_control.h"

// Timer interrupt handle
hw_timer_t *spwm_timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// ISR for the PLL PWM frequency
void IRAM_ATTR onSPWMtimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  if (!should_try_connect && currentState != GridState::CONNECTED) {
    // If not trying to connect and not connected, skip PWM update
    shutdown_hbridge();

    portEXIT_CRITICAL_ISR(&timerMux);
    return;
  }

  int64_t current_time = esp_timer_get_time();
  int32_t phase_shift_fp = PHASE_MATCH_OFFSET_FP + phase_shift_control_fp;
  int32_t predicted_voltage = predict_sine_value_fp(grid_voltage_sine_fit_fp, current_time, phase_shift_fp, amplitude_offset_control_fp);
  set_pwm_fp(predicted_voltage);
  portEXIT_CRITICAL_ISR(&timerMux);
}

void init_pwm_control() {
  init_fp_sine_table();
  init_float_sine_table();

  // Setup timer interrupt SPWM update
  spwm_timer = timerBegin(0, 80, true);                 // Timer 0, prescaler 80 (1 us tick), count up
  timerAttachInterrupt(spwm_timer, &onSPWMtimer, true); // Attach onSPWMtimer function, edge triggered
  timerAlarmWrite(spwm_timer,                           // Set interval in uS, auto-reload
                 INTERVAL_SPWM_UPDATE_MICROS, true);
  timerAlarmEnable(spwm_timer);                         // Enable the timer
}
