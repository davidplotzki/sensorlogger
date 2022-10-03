#ifndef _SENSORLOGGER_H
#define _SENSORLOGGER_H

#define SENSORLOGGER_VERSION "1.2"
#define SENSORLOGGER_VERSION_DATE "2022-10-03"

// Logbook and measurement defaults:
#define DEFAULT_CYCLETIME            900000  // 15 minutes
#define DEFAULT_MAX_ENTRIES          30
#define DEFAULT_MEASUREMENT_INTERVAL 60000   // 60 seconds
#define DEFAULT_RETRY_TIME           300000  // 5 minutes
#define DEFAULT_HTTP_TIMEOUT         10L
#define DEFAULT_HTTP_MAXFILESIZE     10485760L  // 10 MB
#define DEFAULT_HTTP_MAXREDIRS       10L

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