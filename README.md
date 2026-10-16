## Sensor de Turbidez

- **Función:** Cuantifica el nivel de partículas suspendidas en el cuerpo de agua.
- **Sensor base:** Replica al sensor industrial SEN0710, cuyo principio de funcionamiento es la dispersión de luz a 90°.
- **Rango de operación:** Actúa en el rango de 0 a 1000 NTU.
- **Hardware y pines:** Se implementa en el ESP32-CAM OV2640. Debido a que esta placa tiene sus pines analógicos comprometidos por la cámara y la MicroSD, utiliza los pines digitales GPIO 14 (Rx) y GPIO 15 (Tx), los cuales se conectan físicamente a un módulo transceptor MAX485.
- **Tipo de salida:** Proporciona una salida completamente digital basada en el protocolo industrial RS485 (ModBus-RTU). El ESP32-CAM actúa como un esclavo que escucha las peticiones en la red y responde empaquetando el valor numérico exacto en una trama estándar cuando el nodo maestro se lo solicita.
- **Dinámica de cambio:** Comienza con un nivel base de suciedad (ej. 50 NTU) y aplica variaciones aleatorias continuas de ± 5 NTU. Esto simula el arrastre constante e irregular de sedimentos en el lecho del río, asegurando mediante condicionales en el código que el valor jamás baje de 0 NTU ni supere el techo de los 1000 NTU.