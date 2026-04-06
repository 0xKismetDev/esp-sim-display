# SimHub Dash — JC3248W535C

SimHub racing telemetry dashboard for the **JC3248W535C** ESP32-S3 3.5" capacitive touch display (480x320 AXS15231B QSPI).

Receives live data from SimHub's Custom Serial Devices plugin over USB and renders a gear indicator, RPM bar, shift lights, speed, and tachometer on the LCD.

## Features

- Large centered gear indicator (N / R / 1–8)
- Progressive RPM bar (green → yellow → orange → red)
- 9 shift lights that activate in the last 20% before redline
- Full-screen red flash + shift light blink at redline
- Speed (km/h) and actual RPM displayed at the bottom
- Auto-resyncing serial parser (no line terminator required)

## Hardware

**Board:** JC3248W535C — ESP32-S3 N16R8 (16MB flash, 8MB PSRAM)
**Display:** 3.5" IPS 480x320, AXS15231B driver over QSPI (Integrated)

Can be found on sites like Aliexpress for cheap (~$20-30).

## Building

Requires [PlatformIO](https://platformio.org/).

```bash
pio run              # build
pio run -t upload    # flash (put board in bootloader mode: hold BOOT, press RESET)
pio device monitor   # serial monitor
```

## SimHub Setup

1. In SimHub, open **Custom Serial Devices** and add a new device
2. Select the ESP32's COM port and click **Open**
3. Go to the **Update messages** tab and add a new message
4. Set the formula format to **NCalc** and paste:

```
format([DataCorePlugin.GameData.NewData.SpeedKmh],'0') + ';' + isnull([DataCorePlugin.GameData.NewData.Gear],'N') + ';' + format([DataCorePlugin.GameData.NewData.Rpms],'0') + ';' + format([DataCorePlugin.GameData.NewData.MaxRpm],'0') + ';' + format([CarSettings_RPMRedLineSetting],'0')
```

5. Set update frequency to **"When changes occur"** (or 30–60 Hz)
6. Start a game — the dashboard updates live

## Protocol

5 semicolon-separated fields per message, no line terminator:

```
speed;gear;rpm;maxRpm;redlinePercent
```

Example: `120;3;7500;9000;95`

The parser auto-syncs on the gear field (always a single character: N, R, or 1–8) so it recovers automatically from any desync.

## Credits

BSP, display driver, and LVGL port based on [NorthernMan54/JC3248W535EN](https://github.com/NorthernMan54/JC3248W535EN).

