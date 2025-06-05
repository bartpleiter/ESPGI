#include <Arduino.h>

#include "sine_fitting.h"
#include "sampling.h"
#include "config.h"
#include "util.h"
#include "hardware.h"

SineFit grid_voltage_sine_fit = {0.0f, 0.0f, 0.0f, 0};
SineFit inverter_voltage_sine_fit = {0.0f, 0.0f, 0.0f, 0};
SineFit inverter_current_sine_fit = {0.0f, 0.0f, 0.0f, 0};

SineFitFP grid_voltage_sine_fit_fp = {0, 0, 0, 0};

// Sine lookup table - covering 0 to 2*PI
int32_t fp_sine_table[SINE_TABLE_SIZE];

// Floating-point sine and cosine lookup tables - covering 0 to 2*PI
float fp_sine_table_float[SINE_TABLE_SIZE];
float fp_cosine_table_float[SINE_TABLE_SIZE];

float prev_phase = 0.0f;
int64_t prev_update_time = 0;
bool phase_initialized = false;

void init_fp_sine_table()
{
  for (int i = 0; i < SINE_TABLE_SIZE; i++)
  {
    // Calculate angle in radians (0 to 2*PI)
    float angle = (i / (float)SINE_TABLE_SIZE) * TWO_PI;
    // Store sine values in Q16.16 format
    fp_sine_table[i] = (int32_t)(sinf(angle) * FP_ONE);
  }
}

// Lookup sine value from the table
int32_t fp_sin(int32_t angle)
{
  // Normalize angle to 0-FP_TWO_PI range
  angle = angle % FP_TWO_PI;
  if (angle < 0)
    angle += FP_TWO_PI;
  
  // Convert angle to table index (0 to SINE_TABLE_SIZE-1)
  uint16_t index = (uint16_t)(((uint64_t)angle * SINE_TABLE_SIZE) / FP_TWO_PI);
  
  // Make sure we don't access out of bounds
  index = index >= SINE_TABLE_SIZE ? SINE_TABLE_SIZE - 1 : index;
  
  // Return value from sine table
  return fp_sine_table[index];
}

void init_float_sine_table()
{
  for (int i = 0; i < SINE_TABLE_SIZE; i++)
  {
    // Calculate angle in radians (0 to 2*PI)
    float angle = (i / (float)SINE_TABLE_SIZE) * TWO_PI;
    // Store sine and cosine values directly as floats
    fp_sine_table_float[i] = sinf(angle);
    fp_cosine_table_float[i] = cosf(angle);
  }
}

// Lookup sine and cosine values from the floating-point tables
void lookup_sin_cos_float(float angle, float& sin_val, float& cos_val)
{
  // Normalize angle to 0-TWO_PI range
  angle = fmodf(angle, TWO_PI);
  if (angle < 0)
    angle += TWO_PI;
  
  // Convert angle to table index (0 to SINE_TABLE_SIZE-1)
  uint16_t index = (uint16_t)((angle * SINE_TABLE_SIZE) / TWO_PI);
  
  // Make sure we don't access out of bounds
  index = index >= SINE_TABLE_SIZE ? SINE_TABLE_SIZE - 1 : index;
  
  // Return values from tables
  sin_val = fp_sine_table_float[index];
  cos_val = fp_cosine_table_float[index];
}

int32_t predict_sine_value_fp(const SineFitFP &fit, int64_t time_us, int32_t phase_shift_fp, int32_t amplitude_offset_fp)
{
  // Calculate time difference in microseconds
  int64_t time_diff_us = time_us - fit.last_update_time;
  
  // Convert to seconds in fixed point (Q16.16) with higher precision
  // Multiply by FP_ONE first, then divide by 1000000 to avoid losing precision
  int64_t time_diff_sec_64 = (time_diff_us * FP_ONE) / 1000000;
  int32_t time_diff_sec = (int32_t)time_diff_sec_64;

  // Calculate omega (2*PI*f) in Q16.16
  int64_t omega_fp = fixed_mul(FP_TWO_PI, fit.frequency);

  // Calculate angle = omega * time + phase + phase_shift with higher precision
  int64_t angle_product = (omega_fp * time_diff_sec);
  int32_t angle = fit.phase + phase_shift_fp + (int32_t)(angle_product >> FP_SHIFT);

  // Calculate amplitude * sin(angle) using lookup table
  int32_t sine_value = fp_sin(angle);
  int32_t amplitude_fp = fit.amplitude + amplitude_offset_fp;
  if (amplitude_fp < 0)
  {
    amplitude_fp = 0; // Ensure amplitude is non-negative
  }

  return fixed_mul(amplitude_fp, sine_value);
}

float calculate_sine_amplitude(const ACSample samples[], size_t size, float (*value_getter)(const ACSample &))
{
  if (size == 0 || !buffer_filled)
    return 0.0f;
  
  float min_value = 1000.0f;  // Start with extreme values
  float max_value = -1000.0f;
  
  // Pre-calculate buffer indices to avoid repetitive modulo operations
  int actual_indices[AC_SAMPLE_SIZE];
  for (int i = 0; i < size; i++)
  {
    actual_indices[i] = (sample_index - size + i + AC_SAMPLE_SIZE) % AC_SAMPLE_SIZE;
  }
  
  // Find min and max values in a single pass
  for (int i = 0; i < size; i++)
  {
    int actual_idx = actual_indices[i];
    float value = value_getter(samples[actual_idx]);
    
    // Update min/max
    min_value = min(min_value, value);
    max_value = max(max_value, value);
  }
  
  // Calculate amplitude as half the peak-to-peak value
  return (max_value - min_value) / 2.0f;
}

SineFit fit_sine_wave_float(const ACSample samples[], size_t size, float (*value_getter)(const ACSample &), int64_t (*time_getter)(const ACSample &))
{
  // Initialize the result with the default frequency (50Hz)
  SineFit result = {0.0f, FREQ_GRID_HZ, 0.0f, 0};
  
  if (size == 0 || !buffer_filled)
    return result;

  // Calculate last update time once (most recent sample)
  int last_actual_index = (sample_index - 1 + AC_SAMPLE_SIZE) % AC_SAMPLE_SIZE;
  result.last_update_time = time_getter(samples[last_actual_index]);
  
  // Pre-calculate constants used in the loop
  const float omega = OMEGA;
  const float time_micros_to_sec = TIME_MICROS_TO_SEC;
  const int64_t base_time = result.last_update_time;
  
  // Floating-point accumulators for the calculation
  float sum_sin2 = 0.0f;
  float sum_cos2 = 0.0f;
  float sum_sin_cos = 0.0f;
  float sum_v_sin = 0.0f;
  float sum_v_cos = 0.0f;
  
  float min_value = 100.0f;
  float max_value = -100.0f;
  
  // Pre-calculate all buffer indices at once
  int actual_indices[AC_SAMPLE_SIZE];
  const int wrapped_sample_index = sample_index + AC_SAMPLE_SIZE;
  for (int i = 0; i < size; i++)
  {
    actual_indices[i] = (wrapped_sample_index - size + i) % AC_SAMPLE_SIZE;
  }

  // Process each sample in the buffer
  for (int idx = 0; idx < size; idx++)
  {
    int actual_idx = actual_indices[idx];
    
    // Get the voltage value - direct access
    float value = value_getter(samples[actual_idx]);
    
    // Update min/max
    min_value = value < min_value ? value : min_value;
    max_value = value > max_value ? value : max_value;
    
    // Get the time and calculate angle
    int64_t time_us = time_getter(samples[actual_idx]);
    float angle = omega * ((time_us - base_time) * time_micros_to_sec);
    
    // Get sine and cosine values from lookup table
    float sin_term, cos_term;
    lookup_sin_cos_float(angle, sin_term, cos_term);
    
    // Square terms
    float sin2 = sin_term * sin_term;
    float cos2 = cos_term * cos_term;
    float sin_cos = sin_term * cos_term;
    
    // Accumulate sums for least squares fitting
    sum_sin2 += sin2;
    sum_cos2 += cos2;
    sum_sin_cos += sin_cos;
    sum_v_sin += value * sin_term;
    sum_v_cos += value * cos_term;
  }
  
  // Solve the system of equations
  float denominator = sum_sin2 * sum_cos2 - sum_sin_cos * sum_sin_cos;
  
  // Avoid division by zero
  if (fabsf(denominator) < 0.000001f)
  {
    return result;  // Return default result
  }
  
  // Calculate coefficients
  float a = (sum_v_sin * sum_cos2 - sum_v_cos * sum_sin_cos) / denominator;
  float b = (sum_v_cos * sum_sin2 - sum_v_sin * sum_sin_cos) / denominator;
  
  // Calculate amplitude using the square root of the sum of squares
  result.amplitude = sqrtf(a * a + b * b);
  
  // Calculate phase
  result.phase = atan2f(b, a);
  
  return result;
}

SineFitFP convert_to_fixed_point(const SineFit &fit)
{
  SineFitFP result;
  result.amplitude = (int32_t)(fit.amplitude * FP_ONE);
  result.frequency = (int32_t)(fit.frequency * FP_ONE);
  result.phase = (int32_t)(fit.phase * FP_ONE);
  result.last_update_time = fit.last_update_time;
  return result;
}

float calculate_phase_difference(const SineFit &fit1, const SineFit &fit2)
{
  // Check if fits have valid data (non-zero amplitude)
  if (fit1.amplitude < 0.2f || fit2.amplitude < 0.2f) {
    return 0.0f; // Return zero if either signal is too weak to have reliable phase
  }
  
  // Calculate phase difference at a common reference time
  // We'll use the most recent time as our reference
  int64_t reference_time = max(fit1.last_update_time, fit2.last_update_time);
  
  // Calculate how the phase of each signal evolves from its last update time to the reference time
  float time_diff1_sec = (reference_time - fit1.last_update_time) / 1000000.0f; // Convert μs to seconds
  float time_diff2_sec = (reference_time - fit2.last_update_time) / 1000000.0f;
  
  // Calculate the phase evolution (phase = ω * t, where ω = 2π * frequency)
  float phase_evolution1 = TWO_PI * fit1.frequency * time_diff1_sec;
  float phase_evolution2 = TWO_PI * fit2.frequency * time_diff2_sec;
  
  // Calculate the total phase at the reference time
  float phase1_at_reference = fit1.phase + phase_evolution1;
  float phase2_at_reference = fit2.phase + phase_evolution2;
  
  // Calculate the phase difference
  float phase_diff = phase1_at_reference - phase2_at_reference;
  
  // Normalize the phase difference to be between -π and π
  while (phase_diff > PI)
    phase_diff -= TWO_PI;
  while (phase_diff <= -PI)
    phase_diff += TWO_PI;
  
  return phase_diff;
}

void update_grid_voltage_sine_fit()
{
  if (!buffer_filled)
    return;

  grid_voltage_sine_fit = fit_sine_wave_float(ac_samples, AC_SAMPLE_SIZE, get_grid_voltage, get_grid_time);
  grid_voltage_sine_fit_fp = convert_to_fixed_point(grid_voltage_sine_fit); // Convert to fixed-point representation for ISR use
}

void update_inverter_voltage_sine_fit()
{
  if (!buffer_filled)
    return;

  inverter_voltage_sine_fit = fit_sine_wave_float(ac_samples, AC_SAMPLE_SIZE, get_inverter_voltage, get_inverter_voltage_time);
}

void update_inverter_current_sine_fit()
{
  if (!buffer_filled)
    return;

  inverter_current_sine_fit = fit_sine_wave_float(ac_samples, AC_SAMPLE_SIZE, get_inverter_current, get_inverter_current_time);
}
