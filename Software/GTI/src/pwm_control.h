#pragma once

#include <Arduino.h>

void init_pwm_control();
void IRAM_ATTR onSPWMtimer();

extern hw_timer_t *spwm_timer;
extern portMUX_TYPE timerMux;
