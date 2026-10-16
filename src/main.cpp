#include <Arduino.h>
#include <HardwareSerial.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "TurbidityEmulator.h"

// --- Configuracion de Pines ESP32-CAM ---
// Pines UART libres en el ESP32-CAM para conectar al modulo MAX485
const uint8_t k_u8RxPin = 14; 
const uint8_t k_u8TxPin = 15;

// Instancia de HardwareSerial para Modbus
HardwareSerial SerialModbus(1);

// --- Variables Globales ---
TurbidityEmulator objTurbiditySensor;
SemaphoreHandle_t xMutexSerial;

TurbidityEmulator::TurbidityEmulator() {
    fltCurrentTurbidityNtu = 50.0f;
    randomSeed(analogRead(0));
}

void TurbidityEmulator::vUpdateSimulation() {
    float fltDelta = ((float)random(-50, 51) / 10.0f); // Fluctuaciones de ±5 NTU
    fltCurrentTurbidityNtu += fltDelta;

    if (fltCurrentTurbidityNtu > k_fltMaxNtu) fltCurrentTurbidityNtu = k_fltMaxNtu;
    if (fltCurrentTurbidityNtu < k_fltMinNtu) fltCurrentTurbidityNtu = k_fltMinNtu;
}

uint16_t TurbidityEmulator::u16GetModbusRegister() {
    // El SEN0710 suele enviar el valor NTU sin decimales o escalado (*10)[cite: 10]
    return (uint16_t)fltCurrentTurbidityNtu; 
}

// --- Prototipos de Tareas ---
void vTaskModbusSlave(void *pvParameters);
void vTaskUpdateSensor(void *pvParameters);

void setup() {
    Serial.begin(115200); // Consola de depuracion (USB)
    SerialModbus.begin(9600, SERIAL_8N1, k_u8RxPin, k_u8TxPin); // Interfaz RS485

    xMutexSerial = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(
        vTaskUpdateSensor,
        "TaskSensUpdate",
        2048,
        NULL,
        1,
        NULL,
        0 // Core 0 para simulacion matematica
    );

    xTaskCreatePinnedToCore(
        vTaskModbusSlave,
        "TaskModbus",
        4096,
        NULL,
        2, // Mayor prioridad para no perder paquetes UART
        NULL,
        1 // Core 1 para comunicaciones
    );

    vTaskDelete(NULL);
}

void loop() {}

void vTaskUpdateSensor(void *pvParameters) {
    const TickType_t xDelayTicks = pdMS_TO_TICKS(5000); 
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xDelayTicks);
        objTurbiditySensor.vUpdateSimulation();
    }
}

void vTaskModbusSlave(void *pvParameters) {
    uint8_t arr_u8Buffer[8]; // Buffer para trama Modbus estandar

    for (;;) {
        if (SerialModbus.available() >= 8) {
            SerialModbus.readBytes(arr_u8Buffer, 8);

            // Verificar si es un request de lectura (Function Code 0x03) para ID 1
            if (arr_u8Buffer[0] == 0x01 && arr_u8Buffer[1] == 0x03) {
                uint16_t u16Data = objTurbiditySensor.u16GetModbusRegister();

                // Construir respuesta Modbus basica (ID, FC, Bytes, DataH, DataL, CRC_H, CRC_L)
                uint8_t arr_u8Response[7];
                arr_u8Response[0] = 0x01;
                arr_u8Response[1] = 0x03;
                arr_u8Response[2] = 0x02; // Numero de bytes de datos
                arr_u8Response[3] = (u16Data >> 8) & 0xFF; // Byte alto
                arr_u8Response[4] = u16Data & 0xFF;        // Byte bajo
                
                // Nota: Un emulador estricto calcularia el CRC16 Modbus real aqui. 
                // Para efectos de prototipado, colocamos un CRC dummy o el calculo real.
                arr_u8Response[5] = 0x00; 
                arr_u8Response[6] = 0x00; 

                SerialModbus.write(arr_u8Response, 7);

                if (xSemaphoreTake(xMutexSerial, portMAX_DELAY) == pdTRUE) {
                    Serial.print(F("[MODBUS] Peticion respondida. Turbidez: "));
                    Serial.print(u16Data);
                    Serial.println(F(" NTU"));
                    xSemaphoreGive(xMutexSerial);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Ceder procesador (Eficiencia energetica)
    }
}