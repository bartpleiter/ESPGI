#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_ADS1X15.h>

// I2C 1
#define ADDR_VOLTAGE_GRID_AC 0x4B
#define ADDR_ACS_INV_AC 0x4A

// I2C 2
#define ADDR_INA_INPUT 0x44
#define ADDR_VOLTAGE_INV_AC 0x49
#define ADDR_ACS_INV_DC 0x48

#define ADS1015_VOLTAGE_SCALE 8.08f // Scale factor for voltage readings
#define GRID_VOLTAGE_SCALING_FACTOR 1.01f // Scaling factor for grid voltage due to hardware differences

// Function declarations
void init_sensors(TwoWire& wire1, TwoWire& wire2);
void init_ina_input();
void init_voltage_grid_ac();
void init_voltage_inv_ac();
void init_acs_inv_ac();
void init_acs_inv_dc();

float get_dc_voltage_inverter();
float get_dc_current_inverter_ina();
float get_dc_current_inverter_acs();
float get_ac_current_inverter();
float get_ac_voltage_grid();
float get_ac_voltage_inverter();

// Global sensor objects
extern Adafruit_INA219 ina_input;
extern Adafruit_ADS1115 acs_inv_ac;
extern Adafruit_ADS1115 acs_inv_dc;
extern Adafruit_ADS1015 voltage_grid_ac;
extern Adafruit_ADS1015 voltage_inv_ac;
