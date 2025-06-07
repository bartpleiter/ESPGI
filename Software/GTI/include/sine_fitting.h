#pragma once
#include <Arduino.h>
#include "sampling.h"

// Struct for sine wave fitting parameters
struct SineFit
{
  float amplitude;
  float frequency;
  float phase;
  int64_t last_update_time; // To track when the fit was last updated
};

// Fixed-point version of SineFit for ISR
struct SineFitFP
{
  int32_t amplitude;  // Q16.16
  int32_t frequency;  // Q16.16
  int32_t phase;      // Q16.16
  int64_t last_update_time; // Time in microseconds
};

extern SineFit grid_voltage_sine_fit;
extern SineFit inverter_voltage_sine_fit;
extern SineFit inverter_current_sine_fit;

// Only grid voltage sine fit is used in the ISR
extern SineFitFP grid_voltage_sine_fit_fp;

// Convert regular SineFit to fixed-point SineFitFP
SineFitFP convert_to_fixed_point(const SineFit& fit);

float calculate_sine_amplitude(const ACSample samples[], size_t size, float (*value_getter)(const ACSample &));

SineFit fit_sine_wave_float(const ACSample samples[], size_t size, float (*value_getter)(const ACSample &), int64_t (*time_getter)(const ACSample &));

float predict_sine_value(const SineFit& fit, int64_t time);

// Fixed point version of the sine wave prediction
int32_t predict_sine_value_fp(const SineFitFP& fit, int64_t time_us,
                              int32_t phase_shift_fp, int32_t amplitude_offset_fp);

// Calculate the phase difference between two sine fits at a common reference time
float calculate_phase_difference(const SineFit &fit1, const SineFit &fit2);

// Initialize the fixed point sine wave lookup table
void init_fp_sine_table();
void init_float_sine_table();

void update_grid_voltage_sine_fit();
void update_inverter_voltage_sine_fit();
void update_inverter_current_sine_fit();

