# Marine engine monitoring

This repo implements an engine monitoring system based on the [SH-ESP32 development board](https://docs.hatlabs.fi/sh-esp32/).

## TODO

- Documentation
- Data to collect:

  - [ ] Engine coolant temperature: analog input from current sensor.
  - [ ] Engine oil pressure: analog input from current sensor.
  - [ ] Engine revolutions: digital input from W-terminal output.
  - [ ] Engine exhaust temperature: 1-wire sensor attached to exhaust elbow.
  - [ ] Engine room temperature: 1-wire sensor suspended.
  - [ ] Alternator temperature: 1-wire sensor attached to alternator terminal.
  - [ ] Engine hours counter: inferred from engine revs.
  - [ ] Water tank level: analog input from tank level sender.
  - [ ] Diesel tank level: TBD
