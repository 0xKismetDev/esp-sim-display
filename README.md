# ESP Sim Display

SimHub dashboard firmware for the JC3248W535C ESP32-S3 480x320 display.

## Screens

- Race: gear, speed, RPM, ABS and TC
- GT: compact tires, pressure, temperature and lap time
- Tires: four-corner tire view and lap time
- Timing: current, last and best laps with live delta
- Controls: touch controls for TC, ABS, brake bias and engine map

Swipe left or right to change screens. A long press advances to the next screen. All screens keep the bottom shift strip and red limiter flash.

## Build

```powershell
pio run
pio run -t upload
```

## SimHub

Create a Custom Serial Device at 115200 baud. Add an NCalc update message using [simhub_formula.txt](simhub_formula.txt) and set its maximum frequency to 60 Hz.

The framed protocol carries speed, RPM, assists, tire data and lap timing. Missing game telemetry is shown as `--`.

Rendering is paced to leave touch-sampling time between the panel's full-frame transfers.

## In-game controls

The display also enumerates as a USB game controller. Bind its buttons in the
simulator as follows:

| Gamepad button | Action |
| ---: | --- |
| 1 / 2 | TC decrease / increase |
| 3 / 4 | ABS decrease / increase |
| 5 / 6 | Brake bias decrease / increase |
| 7 / 8 | Engine map decrease / increase |

TC and ABS values on the Controls page remain telemetry-driven, so they show
the setting confirmed by the game rather than predicting the result of a tap.
Touch actions are debounced and queued. Each HID press and release is retried
until accepted by USB, with a 100 ms press and a 45 ms neutral gap between
commands so games polling at different rates see every action cleanly.
Control buttons bubble gestures to page navigation, and page changes avoid
full-screen animations so touch sampling remains responsive.
Navigation has a 400 ms gesture cooldown so one continuous swipe cannot advance
through multiple pages after an immediate screen change.

The USB mode change may give the device a new COM port after flashing. If so,
select that new port in the SimHub Custom Serial Device settings.
