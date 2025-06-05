#include "sensors.h"

// Define the global sensor objects
Adafruit_INA219 ina_input(ADDR_INA_INPUT);
Adafruit_ADS1115 acs_inv_ac;
Adafruit_ADS1115 acs_inv_dc;
Adafruit_ADS1015 voltage_grid_ac;
Adafruit_ADS1015 voltage_inv_ac;

void init_sensors(TwoWire& wire1, TwoWire& wire2) {
  // Initialize I2C devices
  voltage_grid_ac.begin(ADDR_VOLTAGE_GRID_AC, &wire1);
  acs_inv_ac.begin(ADDR_ACS_INV_AC, &wire1);

  ina_input.begin(&wire2);
  voltage_inv_ac.begin(ADDR_VOLTAGE_INV_AC, &wire2);
  acs_inv_dc.begin(ADDR_ACS_INV_DC, &wire2);

  // Initialize each sensor with specific settings
  init_ina_input();
  init_voltage_grid_ac();
  init_voltage_inv_ac();
  init_acs_inv_ac();
  init_acs_inv_dc();
}

// Initialize the INA219 sensor
void init_ina_input() {
  ina_input.setCalibration_01ohm();
}

// Initialize the grid AC voltage sensor
void init_voltage_grid_ac() {
  voltage_grid_ac.setDataRate(0x00C0); // 3.3k SPS
  voltage_grid_ac.setGain(GAIN_ONE);
  voltage_grid_ac.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, /*continuous=*/true);
}

// Initialize the inverter AC voltage sensor
void init_voltage_inv_ac() {
  voltage_inv_ac.setDataRate(0x00C0); // 3.3k SPS
  voltage_inv_ac.setGain(GAIN_ONE);
  voltage_inv_ac.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, /*continuous=*/true);
}

// Initialize the inverter AC current sensor
void init_acs_inv_ac() {
  acs_inv_ac.setDataRate(0x00E0); // 860 SPS
  acs_inv_ac.setGain(GAIN_ONE);
  acs_inv_ac.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, /*continuous=*/true);
}

// Initialize the inverter DC current sensor
void init_acs_inv_dc() {
  acs_inv_dc.setDataRate(0x00E0); // 860 SPS
  acs_inv_dc.setGain(GAIN_ONE);
  acs_inv_dc.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, /*continuous=*/true);
}

// Get DC voltage from inverter
float get_dc_voltage_inverter() {
  float voltage = ina_input.getBusVoltage_V();
  return voltage;
}

// Get DC current from inverter using INA219
float get_dc_current_inverter_ina() {
  float current = ina_input.getCurrent_mA() / 1000.0; // Fix for 0.01 Ohm shunt config
  return current;
}

// Get DC current from inverter using ACS sensor
float get_dc_current_inverter_acs() {
  int16_t raw = acs_inv_dc.getLastConversionResults();
  float voltage = acs_inv_dc.computeVolts(raw);
  float current = voltage / 0.2; // 200mV/A
  return current;
}

// Get AC current from inverter
float get_ac_current_inverter() {
  int16_t raw = acs_inv_ac.getLastConversionResults();
  float voltage = acs_inv_ac.computeVolts(raw);
  float current = voltage / 0.2; // 200mV/A
  return current;
}

// Get AC voltage from grid
float get_ac_voltage_grid() {
  int16_t raw = voltage_grid_ac.getLastConversionResults();
  float voltage = voltage_grid_ac.computeVolts(raw) * ADS1015_VOLTAGE_SCALE;
  return voltage * GRID_VOLTAGE_SCALING_FACTOR; // Apply scaling factor for grid voltage because of slight differences in hardware
}

// Get AC voltage from inverter
float get_ac_voltage_inverter() {
  int16_t raw = voltage_inv_ac.getLastConversionResults();
  float voltage = voltage_inv_ac.computeVolts(raw) * ADS1015_VOLTAGE_SCALE;
  return voltage;
}
