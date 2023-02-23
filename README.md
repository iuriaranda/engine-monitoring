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
  - [ ] Alternator output (Amps): analog input from hall effect sensor + burden resistor
  - [ ] Engine hours counter: inferred from engine revs.
  - [ ] Water tank level: analog input from tank level sender.
  - [ ] Diesel tank level: TBD

- Change alt. wire
- Fuse alt. wire
- Enable / disable Victron Orion DC-DC chargers via bluetooth


On the DS1603L: 
- https://forum.arduino.cc/t/how-to-read-serial-data-from-non-contact-ultrasonic-liquid-level-sensor/507781/51
- https://forum.arduino.cc/t/is-this-ds1603lv1-0-sensor-bad-or-am-i-bad/911205
- https://forum.arduino.cc/t/contact-less-boat-tank-monitor-help-with-libraries-for-newbie/993770
- https://open-boat-projects.org/en/diy-ultraschall-fuellstandsmessung/
