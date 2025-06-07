#pragma once

#include <Arduino.h>
#include "sensor.h"

extern int duty_cycle;
extern float prev_power;
extern int direction; 

void set_duty_cycle(uint8_t duty);
void mppt_loop();

