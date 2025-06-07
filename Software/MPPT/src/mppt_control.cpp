#include "mppt_control.h"
#include "config.h"
#include "sensor.h"

float prev_avg_power = 0;
int duty_cycle = 120;
int direction = 1;

void set_duty_cycle(uint8_t duty) {
  if (duty == 0) {
    // Set NLIN to 1 and HIN to 0 to disable both MOSFETs
    ledcWrite(HIN_CHANNEL, 0);
    ledcWrite(NLIN_CHANNEL, 255);
  }
  ledcWrite(HIN_CHANNEL, duty);
  ledcWrite(NLIN_CHANNEL, duty);
}

void mppt_loop() {
  // Main loop of the MPPT algorithm
  float vin = sensor_data.vin;
  float iin = sensor_data.iin;
  float vout = sensor_data.vout;

  // --- Stop conditions ---
  if (vout >= MAX_VOUT || vout > vin || vin < MIN_VIN) {
    duty_cycle = 0;  // Turn off
    set_duty_cycle(duty_cycle);
    return;
  }

  // --- Increase duty cycle if input current is low ---
  if (iin < MPPT_LOW_CURRENT_THRESHOLD) {
    if (duty_cycle < MIN_DUTY_CYCLE) duty_cycle = MIN_DUTY_CYCLE;
    duty_cycle += DUTY_STEP;
    if (duty_cycle > MAX_DUTY_CYCLE) duty_cycle = MAX_DUTY_CYCLE;
    set_duty_cycle(duty_cycle);
    return;
  }

  // --- Measure and filter input power ---
  float power = vin * iin;
  float avg_power = power;
  float delta_power = avg_power - prev_avg_power;

  // --- Perturb & Observe ---
  if (delta_power > 0) {
  // Power increased, continue in same direction
  duty_cycle += direction * DUTY_STEP;
  } else {
  // Power decreased, reverse direction
  direction *= -1;
  duty_cycle += direction * DUTY_STEP;
  }

  // Clamp duty cycle within valid range
  if (duty_cycle > MAX_DUTY_CYCLE) duty_cycle = MAX_DUTY_CYCLE;
  if (duty_cycle < MIN_DUTY_CYCLE) duty_cycle = MIN_DUTY_CYCLE;

  set_duty_cycle(duty_cycle);
  prev_avg_power = avg_power;
}
