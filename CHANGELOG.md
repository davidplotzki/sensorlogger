# Changelog

## 1.2 (2022-10-03)
+ Added sensor retry time (`retry_time` and `default_retry_time`): time for another attempt after a failed sensor reading.
+ New config options for `general`: `http_timeout`, `default_rest_period`, `default_retry_time`, `loglevel`.
+ Compile options in `makefile` to disable support for Libcurl, MQTT and/or Tinkerforge.
+ Support for Tinkerforge Bricklets: Hall Effect 2.0, GPS, GPS 2.0, GPS 3.0.
+ Bug fix: Added HTTP timeout to avoid libcurl blocking.
+ Bug fixes.

## 1.1 (2021-05-22)
+ Supports multiple MQTT brokers.

## 1.0.1 (2021-02-08)
+ Changed HomeMatic value request URL from `sysvar.cgi?ise_id` to `state.cgi?datapoint_id` such that `homematic_subscribe` can fetch any datapoint values, not just system variables.

## 1.0 (2021-01-31)
+ Initial release.