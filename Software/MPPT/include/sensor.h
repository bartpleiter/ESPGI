#pragma once
#include <Arduino.h>
#include <Adafruit_INA219.h>
#include <Adafruit_ADS1X15.h>

struct SensorData {
    float vin;
    float iin;
    float vout;
    float iout;
    float iout_ina;
    float iout_acs;
};

extern SensorData sensor_data;

extern Adafruit_INA219 ina_input;
extern Adafruit_INA219 ina_output;
extern Adafruit_ADS1115 acs_output;

void update_sensor_data();
float get_input_voltage();
float get_output_voltage();
float get_input_current();
float get_output_current();
float get_output_current_ina();
float get_output_current_acs();

