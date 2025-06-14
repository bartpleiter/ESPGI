#pragma once

// Base time unit constants
#define TIME_MICROS_PER_SEC 1000000  // 1 second in microseconds
#define TIME_MILLIS_PER_SEC 1000     // 1 second in milliseconds

// Frequency definitions (in Hz)
#define FREQ_GRID_HZ 50.0f             // Grid frequency (50Hz)
#define FREQ_AC_SAMPLE_HZ 1333         // AC sampling frequency
#define FREQ_SPWM_UPDATE_HZ 20000      // SPWM update frequency (20kHz)
#define FREQ_DC_SAMPLE_HZ 123          // DC sampling frequency
#define FREQ_GRID_SINE_FIT_HZ 27       // Grid sine fit update frequency. Odd number to prevent cutting off same part of the sine wave
#define FREQ_DEBUG_PRINT_HZ 3          // Debug print frequency (3Hz)
#define FREQ_CONTROL_LOGIC_HZ 10       // Control logic frequency (10Hz)

// Derived time intervals (in microseconds)
#define INTERVAL_AC_SAMPLE_MICROS (TIME_MICROS_PER_SEC / FREQ_AC_SAMPLE_HZ)
#define INTERVAL_SPWM_UPDATE_MICROS (TIME_MICROS_PER_SEC / FREQ_SPWM_UPDATE_HZ)
#define INTERVAL_DC_SAMPLE_MICROS (TIME_MICROS_PER_SEC / FREQ_DC_SAMPLE_HZ)
#define INTERVAL_GRID_SINE_FIT_MICROS (TIME_MICROS_PER_SEC / FREQ_GRID_SINE_FIT_HZ)

// Derived time intervals (in milliseconds)
#define INTERVAL_DEBUG_PRINT_MS (TIME_MILLIS_PER_SEC / FREQ_DEBUG_PRINT_HZ)
#define INTERVAL_CONTROL_LOGIC_MS (TIME_MILLIS_PER_SEC / FREQ_CONTROL_LOGIC_HZ)
#define INTERVAL_TELEMETRY_MS 1000

#define AC_SAMPLE_SIZE 500 // AC sample buffer size, with sampling taking ~600us, this gives us ~250ms of data
#define DC_SAMPLE_WINDOW_SIZE 100 // DC sample window size for averaging, with sampling every 10ms, this gives us 1 second of data

#define PHASE_MATCH_OFFSET 0.08f // Offset in radians to exactly match the inverter phase to the grid (LC filter componsation)
#define PHASE_MATCH_OFFSET_FP (int32_t)(PHASE_MATCH_OFFSET * FP_ONE)

// Constants
#define OMEGA (2.0f * PI * FREQ_GRID_HZ)
#define TIME_MICROS_TO_SEC 1.0e-6f
#define PERIOD_GRID_MICROS (TIME_MICROS_PER_SEC / FREQ_GRID_HZ) // Period in microseconds (1/50Hz = 20ms = 20,000 microseconds for 50Hz grid)

// Q16.16 fixed-point constants
#define FP_SHIFT 16
#define FP_ONE (1 << FP_SHIFT)
#define FP_TWO_PI 411774 // 2*PI in Q16.16 (6.28318 * 65536)
#define FP_OMEGA (int32_t)(OMEGA * (1 << FP_SHIFT)) // OMEGA in Q16.16 (2*PI*50Hz * 65536)
#define SINE_TABLE_SIZE 4096

// Start grid tie thresholds
#define REQUIRED_CONSEC_CHECKS_BEFORE_TIE 60 // Number of consecutive checks that need to pass before tying to grid
#define MIN_GRID_VOLTAGE_AMPLITUDE_BEFORE_TIE 5.0f // Minimum grid voltage amplitude to consider grid tie
#define MAX_DC_CURRENT_BEFORE_TIE 0.5f // Maximum DC current before considering grid tie
#define MIN_DC_VOLTAGE_BEFORE_TIE 12.0f // Minimum DC voltage before considering grid tie
#define MAX_DC_VOLTAGE_BEFORE_TIE 13.55f // Maximum DC voltage before considering grid tie

// Safety thresholds after grid tie
#define MIN_DC_VOLTAGE_AFTER_TIE 10.0f // Minimum battery voltage after grid tie
#define MAX_DC_VOLTAGE_AFTER_TIE 13.6f // Max battery voltage (<=14.0 because AC voltage sensor amplitude)
#define DC_CURRENT_THRESHOLD_AFTER_TIE 3.5f // Max DC current
#define MIN_GRID_VOLTAGE_AMPLITUDE_AFTER_TIE 5.0f // Minimum grid voltage amplitude after grid tie

// Grid tie safety margins
#define DC_VOLTAGE_MARGIN_BEFORE_TIE 0.2f // Margin for DC voltage before grid tie
#define DC_VOLTAGE_MARGIN_AFTER_TIE 0.1f  // Margin for DC voltage after grid tie

// Power control constants
#define POWER_STATE_MIN_TIME_MS 10000 // Minimum time (ms) to stay in a power state before transitioning
#define BATTERY_HIGH_VOLTAGE 13.3f    // Battery voltage above which to discharge
#define BATTERY_LOW_VOLTAGE 12.5f     // Battery voltage below which to charge
#define BATTERY_MEDIUM_VOLTAGE 12.8f  // Medium battery voltage threshold for state transitions
#define CONNECT_VOLTAGE_TRESHOLD 0.08f // Voltage threshold for connecting to the grid to prevent rapid toggling

// Phase adjustment constants
#define PHASE_ADJ_BASE_STEP 0.007f       // Base step size for phase adjustment in idle state
#define PHASE_ADJ_STEP 0.005f            // Step size for phase adjustment in charge/discharge states
#define AMPLITUDE_ADJ_STEP 0.01f         // Step size for amplitude adjustment
#define MAX_AMPLITUDE_ADJ 0.75f          // Maximum amplitude adjustment in either direction
#define MAX_PHASE_ADJ 0.3f               // Maximum phase adjustment in either direction
#define DC_CURRENT_TARGET 2.0f           // Target DC current in amperes

// Current amplitude thresholds for adjustment factors
#define CURRENT_THRESHOLD_VERY_LOW 0.1f  // Current amplitude below which to use very small adjustment steps
#define CURRENT_THRESHOLD_LOW 0.25f      // Current amplitude below which to use small adjustment steps
#define CURRENT_THRESHOLD_MEDIUM 0.5f    // Current amplitude below which to use medium adjustment steps

// Adjustment factors for phase
#define PHASE_ADJ_FACTOR_VERY_SMALL 0.25f  // Adjustment factor for very small steps
#define PHASE_ADJ_FACTOR_SMALL 0.5f        // Adjustment factor for small steps
#define PHASE_ADJ_FACTOR_MEDIUM 0.75f      // Adjustment factor for medium steps
#define PHASE_ADJ_FACTOR_LARGE 1.0f        // Adjustment factor for large steps
