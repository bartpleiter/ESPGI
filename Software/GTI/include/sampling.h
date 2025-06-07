#pragma once

#include <Arduino.h>
#include "config.h"

// Struct for one complete sample
struct ACSample
{
  float grid_voltage;
  float inverter_voltage;
  float inverter_current;
  int64_t grid_time;
  int64_t inverter_voltage_time;
  int64_t inverter_current_time;
};

// Global arrays to store the AC samples
extern ACSample ac_samples[AC_SAMPLE_SIZE];
extern int sample_index;
extern bool buffer_filled;

extern float dc_voltage;
extern int32_t dc_voltage_fp; // Fixed-point voltage for ISR use
extern float dc_current;
extern float dc_current_ina;
extern float dc_current_acs;

template <typename T>
void update_sample_array(T array[], int size, T new_value);

float get_grid_voltage(const ACSample &sample);
float get_inverter_voltage(const ACSample &sample);
float get_inverter_current(const ACSample &sample);

int64_t get_grid_time(const ACSample &sample);
int64_t get_inverter_voltage_time(const ACSample &sample);
int64_t get_inverter_current_time(const ACSample &sample);

void sample_ac_signals();
void sample_dc_signals();
