# ECE332 Stopwatch

## Description
The purpose of the stopwatch is to track the time passed after the press of a button displayed i n
“HH:MM:SS.hh” format. The stopwatch i s precise to 1/100th of a second, accurate to ±1 second
over a 1 hour period. A piezo buzzer generates a 1kHz tone for 100ms at the start of each
second.

## Operating Conditions
Operating voltage range of the device: 4V to 5.5V
Operating temperature: +20ºC to +80ºC
Storage temperature: -65ºC to +150ºC
Maximum DC Current per I/O pin: 40.0mA

## Setup Instructions
To operate the device, connect VCC (Pin 7) to a voltage source that i s within the operating
voltage range, GND (Pin 8, 22) to ground. Refer to KiCAD schematic.

## Usage Instructions
Press the button to start the stopwatch. Pressing the button for a short time while it’s running
pauses the stopwatch. A short press while it’s paused will resume it, and a long press while
paused resets the stopwatch.
