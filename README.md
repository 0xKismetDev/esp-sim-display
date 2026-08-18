# ESP Sim Display

SimHub dashboard firmware for the JC3248W535C ESP32-S3 480x320 display.

## Screens

- Race: gear, speed, RPM, ABS and TC
- GT: compact tires, pressure, temperature and lap time
- Tires: four-corner tire view and lap time
- Timing: current, last and best laps with live delta

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
