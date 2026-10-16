#ifndef TURBIDITY_EMULATOR_H
#define TURBIDITY_EMULATOR_H

#include <Arduino.h>

class TurbidityEmulator {
private:
    float fltCurrentTurbidityNtu;
    const float k_fltMaxNtu = 1000.0f; // Rango SEN0710[cite: 10]
    const float k_fltMinNtu = 0.0f;

public:
    TurbidityEmulator();
    void vUpdateSimulation();
    uint16_t u16GetModbusRegister();
};

#endif // TURBIDITY_EMULATOR_H