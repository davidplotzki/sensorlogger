# Changelog

## 1.1 (2021-05-22)
+ Supports multiple MQTT brokers

## 1.0.1 (2021-02-08)
+ Changed HomeMatic value request URL from `sysvar.cgi?ise_id` to `state.cgi?datapoint_id` such that `homematic_subscribe` can fetch any datapoint values, not just system variables.

## 1.0 (2021-01-31)
+ Initial release