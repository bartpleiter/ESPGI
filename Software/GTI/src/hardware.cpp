#include "hardware.h"
#include "sampling.h"
#include "sine_fitting.h"
#include <Wire.h>

bool safety_shutdown_flag = false; // True when safety shutdown is triggered
bool tied_to_grid = false;

int32_t amplitude_offset_control_fp = 0;  // Amplitude offset relative to grid in Q16.16 format
int32_t phase_shift_control_fp = 0;       // Phase shift relative to grid phase in Q16.16 format

void setupPWM()
{
  // Configure PWM channels
  ledcSetup(HIN_L_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(NLIN_L_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(HIN_N_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(NLIN_N_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

  // Attach GPIO pins to channels
  ledcAttachPin(PIN_HIN_L, HIN_L_CHANNEL);
  ledcAttachPin(PIN_NLIN_L, NLIN_L_CHANNEL);
  ledcAttachPin(PIN_HIN_N, HIN_N_CHANNEL);
  ledcAttachPin(PIN_NLIN_N, NLIN_N_CHANNEL);
}

void shutdown_hbridge()
{
  ledcWrite(HIN_L_CHANNEL, 0);
  ledcWrite(NLIN_L_CHANNEL, PWM_MAX_DUTY);
  ledcWrite(HIN_N_CHANNEL, 0);
  ledcWrite(NLIN_N_CHANNEL, PWM_MAX_DUTY);
}

void setup_hardware()
{
  // Initialize I2C buses
  Wire.begin(PIN_SDA_1, PIN_SCL_1, I2C_1_FREQ);
  Wire1.begin(PIN_SDA_2, PIN_SCL_2, I2C_2_FREQ);

  // Initialize pins
  pinMode(PIN_RELAY_L, OUTPUT);
  pinMode(PIN_RELAY_N, OUTPUT);
  digitalWrite(PIN_RELAY_L, LOW);
  digitalWrite(PIN_RELAY_N, LOW);

  pinMode(PIN_ALERT_G, INPUT);
  pinMode(PIN_ALERT_H, INPUT);
  pinMode(PIN_ALERT_U, INPUT);
  pinMode(PIN_ALERT_I, INPUT);

  pinMode(PIN_POT1, INPUT);
  pinMode(PIN_POT2, INPUT);

  pinMode(PIN_HIN_L, OUTPUT);
  pinMode(PIN_NLIN_L, OUTPUT);
  pinMode(PIN_HIN_N, OUTPUT);
  pinMode(PIN_NLIN_N, OUTPUT);
  digitalWrite(PIN_HIN_L, LOW);
  digitalWrite(PIN_NLIN_L, HIGH);
  digitalWrite(PIN_HIN_N, LOW);
  digitalWrite(PIN_NLIN_N, HIGH);

  pinMode(PIN_NTC_1, INPUT);
  pinMode(PIN_NTC_2, INPUT);

  // Initialize H-Bridge
  setupPWM();
  shutdown_hbridge();
}

float get_potentiometer_value(int pin) {
  // Read the potentiometer value from the analog pin
  int pot_value = analogRead(pin);
  
  // Map the value to a range of 0.0 to 1.0
  float mapped_value = (float)pot_value / 4095.0; // Assuming 12-bit ADC resolution
  
  return mapped_value;
}


void set_pwm_fp(int32_t voltage) {
  if (safety_shutdown_flag) {
    // If safety shutdown is triggered, do not set PWM
    return;
  }

  // Calculate absolute value for PWM duty cycle
  int32_t absolute_voltage;
  if (voltage < 0) {
    absolute_voltage = -voltage;
  } else {
    absolute_voltage = voltage;
  }
  
  // Map voltage to duty cycle up to PWM_MAX_DUTY
  // Uses battery voltage as the maximum reference
  if (dc_voltage_fp <= 0) {
    // Avoid division by zero or negative voltage
    return;
  }
  // No need for fixed_div because the fixed point shift is cancelled out by the division
  int32_t duty_cycle = ((int64_t)absolute_voltage * PWM_MAX_DUTY) / dc_voltage_fp;
  
  // Ensure duty cycle is within valid range
  if (duty_cycle > PWM_MAX_DUTY) {
    duty_cycle = PWM_MAX_DUTY;
  }
  
  if (voltage < 0) {
    // Negative voltage: activate L channels
    ledcWrite(HIN_L_CHANNEL, duty_cycle);
    ledcWrite(NLIN_L_CHANNEL, duty_cycle);
    ledcWrite(HIN_N_CHANNEL, 0);
    ledcWrite(NLIN_N_CHANNEL, 0);
  } else{
    // Positive voltage: activate N channels
    ledcWrite(HIN_L_CHANNEL, 0);
    ledcWrite(NLIN_L_CHANNEL, 0);
    ledcWrite(HIN_N_CHANNEL, duty_cycle);
    ledcWrite(NLIN_N_CHANNEL, duty_cycle);
  }
}

void tie_to_grid()
{
  if (safety_shutdown_flag) {
    // If safety shutdown is triggered, do not activate relay
    // A manual reset is required in case something is wrong
    return;
  }
  tied_to_grid = true;
  digitalWrite(PIN_RELAY_L, HIGH);
  digitalWrite(PIN_RELAY_N, HIGH);
}

void untie_from_grid()
{
  tied_to_grid = false;
  digitalWrite(PIN_RELAY_L, LOW);
  digitalWrite(PIN_RELAY_N, LOW);
}

void safety_shutdown()
{
  untie_from_grid();
  safety_shutdown_flag = true;
  shutdown_hbridge();
}
