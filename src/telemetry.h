#pragma once
#include <Arduino.h>
#include "config.h"

struct SimTelemetry
{
    int speed = 0;
    char gear[4] = "N";
    int rpm = 0;
    int maxRpm = 8000;
    int redlinePct = 95;
    int rpmPct = 0;
    int absLevel = -1;
    int tcLevel = -1;
    bool absActive = false;
    bool tcActive = false;
    float tyreTemp[4] = {0, 0, 0, 0};
    float tyrePressure[4] = {0, 0, 0, 0};
    char pressureUnit[5] = "psi";
    char currentLapTime[16] = "--:--.---";
    char lastLapTime[16] = "--:--.---";
    char bestLapTime[16] = "--:--.---";
    int deltaMs = 0;
    bool deltaValid = false;
    int currentLap = 0;
    int totalLaps = 0;
    int position = 0;
    char trackName[32] = "NO TRACK";
    bool updated = false;
    uint32_t lastUpdateMs = 0;

    bool isLive() const
    {
        return lastUpdateMs != 0 && millis() - lastUpdateMs < SIM_LIVE_MS;
    }
};

extern SimTelemetry g_sim;
