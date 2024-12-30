# RF Power Meter QRP

A microcontroller-based RF power meter designed for QRP (low power) amateur radio applications. Features automatic frequency band selection, peak power hold, and efficient power management.

## Features

- Power measurements in dBm and Watts
- Support for multiple frequency bands (3.7-50MHz)
- Peak power hold mode
- Auto power-off after 5 minutes
- Battery voltage monitoring
- 4-digit LED display
- Polynomial curve fitting for accurate measurements
- Non-volatile storage of settings

## Hardware Requirements

- Arduino-compatible microcontroller
- 4-digit LED display
- Two control buttons (Menu and OK)
- RF detector circuit
- Voltage divider network (11kΩ/2.5kΩ)
- Power latch circuit
- Buzzer for audio feedback
- External voltage reference (4.762V)

## Dependencies

- TimerOne library
- OneButton library
- EEPROM library (built-in)
- FmDisplay library (custom)

## Installation

1. Clone this repository
2. Install required libraries via Arduino Library Manager
3. Connect hardware according to pin definitions:
   - CS Pin: 11
   - Clock Pin: 12
   - Data Pin: 10
   - Menu Button: Pin 8
   - OK Button: Pin 9
   - Power Latch: Pin 7
   - RF Peak Input: A2
   - Battery Monitor: A0

## Usage

- **Power On**: Press and hold the OK button (lower) to power on. Device starts with battery voltage display followed by selected frequency band. 
- **Menu Button**:
  - Single click: Toggle between dBm/Watts
  - Double click: Enter measurement mode selection - continious (default) vs hold peak mode. Use the same button to switch, use OK button to confirm.
  - Long press: Enter frequency band selection. Use the same button to switch, use OK button to confirm.
- **OK Button**:
  - Single click: In main screen - zero calibration, in menu screens confirm selectionand exit to main screen.
  - Long press (2s): Power off
- Auto shutdown triggers after 5 minutes of inactivity

## Technical Specifications

- Frequency Bands: 3.7MHz, 7.2MHz, 14.2MHz, 18MHz, 21MHz, 24MHz, 28MHz, 50MHz 
- Reference Voltage: 4.762V
- Diode Drop Compensation: 0.42V
- ADC Correction Factor: 0.1988/0.1930
- Measurement display updates every 400ms

## License

[MIT License](LICENSE)