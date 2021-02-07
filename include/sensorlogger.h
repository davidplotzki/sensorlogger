#ifndef _SENSORLOGGER_H
#define _SENSORLOGGER H

#define SENSORLOGGER_VERSION "1.0"
#define SENSORLOGGER_VERSION_DATE "2021-01-29"

// Logbook and measurement defaults:
#define DEFAULT_CYCLETIME            900000  // 15 minutes
#define DEFAULT_MAX_ENTRIES          30
#define DEFAULT_MEASUREMENT_INTERVAL 60000   // 60 seconds

// Tinerforge Defaults:
#define DEFAULT_MAX_BRICKLET_READ_FAILURES   7
#define DEFAULT_MAX_BRICKD_RESTART_ATTEMPTS  3
#define DEFAULT_DEBOUNCE_TIME                7  // ms
#define DEFAULT_TINKERFORGE_TIMEOUT       1000  // ms

// Storage limit: max. number of measurements per _values vector.
#define MAX_MEASUREMENTS     20000   

// MQTT Defaults
#define MQTT_KEEPALIVE_INTERVAL 20  // seconds

#endif