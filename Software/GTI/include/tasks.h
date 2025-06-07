#pragma once

#include <Arduino.h>
#include "config.h"

// Task function declarations
void debugTask(void *parameter);
void control_logic_task(void *parameter);
void debug_print();
void init_tasks();

// External variables for debug timing
extern unsigned long exec_time_ac_sample;
extern unsigned long exec_time_dc_sample;
extern unsigned long exec_time_grid_sine_fit;
