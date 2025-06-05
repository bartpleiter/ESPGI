#pragma once
#include <Arduino.h>
#include "config.h"
#include "grid_control.h"

// Pin definitions
#define PIN_SDA_1 21
#define PIN_SCL_1 22
#define PIN_SDA_2 19
#define PIN_SCL_2 18

#define PIN_ALERT_G 23
#define PIN_ALERT_H 4
#define PIN_ALERT_U 34
#define PIN_ALERT_I 35

#define PIN_HIN_L 32
#define PIN_NLIN_L 33
#define PIN_HIN_N 25
#define PIN_NLIN_N 26

#define PIN_RELAY_L 27
#define PIN_RELAY_N 13

#define PIN_NTC_1 36
#define PIN_NTC_2 39

#define PIN_UART_2_TX 17
#define PIN_UART_2_RX 16

#define PIN_POT1 15
#define PIN_POT2 2

// PWM definitions
#define HIN_L_CHANNEL 0
#define NLIN_L_CHANNEL 1
#define HIN_N_CHANNEL 2
#define NLIN_N_CHANNEL 3

#define PWM_FREQ 20000
#define PWM_RESOLUTION 8
#define PWM_MAX_DUTY 255

// I2C
// Sadly Arduino Wire library cannot go above ~1MHz
#define I2C_1_FREQ 2000000
#define I2C_2_FREQ 2000000

extern bool safety_shutdown_flag;           // True when safety shutdown is triggered
extern bool tied_to_grid;                   // True when inverter is tied to the grid
extern int32_t amplitude_offset_control_fp; // Amplitude offset relative to grid in Q16.16 format
extern int32_t phase_shift_control_fp;      // Phase shift relative to grid phase in Q16.16 format

void setupPWM();
void shutdown_hbridge();
void setup_hardware();  // Initialize all pins and hardware
void set_pwm_fp(int32_t voltage);
void tie_to_grid();
void untie_from_grid();
void safety_shutdown();

float get_potentiometer_value(int pin);
