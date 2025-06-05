#include "sampling.h"
#include "sensors.h"

ACSample ac_samples[AC_SAMPLE_SIZE] = {};
int sample_index = 0;;
bool buffer_filled = false;

float dc_voltage_samples[DC_SAMPLE_WINDOW_SIZE] = {0};
float dc_current_ina_samples[DC_SAMPLE_WINDOW_SIZE] = {0};
float dc_current_acs_samples[DC_SAMPLE_WINDOW_SIZE] = {0};
int dc_sample_index = 0;
bool dc_buffer_filled = false;

float dc_voltage = 0.0f;
int32_t dc_voltage_fp = 0; // Fixed-point voltage for ISR use
float dc_current = 0.0f;
float dc_current_ina = 0.0f;
float dc_current_acs = 0.0f;

float get_grid_voltage(const ACSample &sample) { return sample.grid_voltage; }
float get_inverter_voltage(const ACSample &sample) { return sample.inverter_voltage; }
float get_inverter_current(const ACSample &sample) { return sample.inverter_current; }

int64_t get_grid_time(const ACSample &sample) { return sample.grid_time; }
int64_t get_inverter_voltage_time(const ACSample &sample) { return sample.inverter_voltage_time; }
int64_t get_inverter_current_time(const ACSample &sample) { return sample.inverter_current_time; }

template <typename T>
void update_sample_array(T array[], int size, T new_value)
{
  array[sample_index] = new_value;
  sample_index = (sample_index + 1) % size;
  if (sample_index == 0)
  {
    buffer_filled = true;
  }
}

// Calculate average of a sample buffer
float calculate_average(float buffer[], int size, bool buffer_filled)
{
  float sum = 0.0f;
  int count = buffer_filled ? size : dc_sample_index;
  
  if (count == 0) return 0.0f;
  
  for (int i = 0; i < count; i++) {
    sum += buffer[i];
  }
  
  return sum / count;
}

void sample_dc_signals()
{
  float new_dc_voltage = get_dc_voltage_inverter();
  float new_dc_current_acs = get_dc_current_inverter_acs();

  // Update the sample buffers
  dc_voltage_samples[dc_sample_index] = new_dc_voltage;
  dc_current_acs_samples[dc_sample_index] = new_dc_current_acs;
  
  // Update the buffer index
  dc_sample_index = (dc_sample_index + 1) % DC_SAMPLE_WINDOW_SIZE;
  if (dc_sample_index == 0) {
    dc_buffer_filled = true;
  }

  // Calculate averages and update the global variables
  dc_voltage = calculate_average(dc_voltage_samples, DC_SAMPLE_WINDOW_SIZE, dc_buffer_filled);
  //dc_current_ina = calculate_average(dc_current_ina_samples, DC_SAMPLE_WINDOW_SIZE, dc_buffer_filled);
  dc_current_acs = calculate_average(dc_current_acs_samples, DC_SAMPLE_WINDOW_SIZE, dc_buffer_filled);

  // For ISR use, convert voltage to fixed-point representation
  dc_voltage_fp = (int32_t)(dc_voltage * FP_ONE);

  dc_current = dc_current_acs;
}

void sample_ac_signals()
{
  float grid_voltage = get_ac_voltage_grid();
  int64_t grid_voltage_time = esp_timer_get_time();
  // float inverter_voltage = get_ac_voltage_inverter();
  // int64_t inverter_voltage_time = esp_timer_get_time();
  float inverter_current = get_ac_current_inverter();
  int64_t inverter_current_time = esp_timer_get_time();

  // Create a new sample from the measurements
  ACSample new_sample;
  new_sample.grid_voltage = grid_voltage;
  new_sample.inverter_voltage = 0;//inverter_voltage;
  new_sample.inverter_current = inverter_current;
  new_sample.grid_time = grid_voltage_time;
  new_sample.inverter_voltage_time = 0;//inverter_voltage_time;
  new_sample.inverter_current_time = inverter_current_time;

  update_sample_array(ac_samples, AC_SAMPLE_SIZE, new_sample);
}
