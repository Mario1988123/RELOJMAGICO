# RELOJ TOQUES - Arduino ESP32-S3

Firmware para ESP32-S3 que detecta toques y envía cartas por WiFi beacon.

## 📋 Requisitos

### Hardware
- ESP32-S3 (o ESP32)
- Acelerómetro QMI8658 conectado por I2C:
  - SDA → GPIO 18
  - SCL → GPIO 8

### Software
- Arduino IDE 2.0+
- Board Manager: ESP32 by Espressif (versión 2.0.0+)

## 🔧 Instalación

### 1. Configurar Arduino IDE

1. Abrir Arduino IDE
2. Ir a `File → Preferences`
3. En "Additional Board Manager URLs" agregar:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Ir a `Tools → Board → Boards Manager`
5. Buscar "ESP32" e instalar "esp32 by Espressif Systems"

### 2. Seleccionar Board

1. `Tools → Board → ESP32 Arduino`
2. Seleccionar **"ESP32S3 Dev Module"**
3. Configurar:
   - USB CDC On Boot: **Enabled**
   - Flash Mode: **QIO 80MHz**
   - Flash Size: **8MB** (o según tu módulo)
   - Partition Scheme: **Default 4MB with spiffs**
   - Upload Speed: **921600**

### 3. Cargar el Sketch

1. Abrir `RELOJTOQUES.ino`
2. Conectar ESP32-S3 por USB
3. Seleccionar el puerto correcto en `Tools → Port`
4. Click en **Upload** (→)

## 🎯 Uso

Una vez cargado el firmware:

### Enviar una Carta

**Ejemplo: 3 de Corazones**
1. Da **1 toque** corto en el reloj (♥ Corazones)
2. **Espera 2-3 segundos**
3. Da **3 toques** cortos (número 3)
4. La carta se enviará automáticamente por WiFi

### Palos (1-4 toques)
- 1 toque = ♥ Corazones
- 2 toques = ♠ Picas
- 3 toques = ♣ Tréboles
- 4 toques = ♦ Diamantes

### Números (1-13 toques)
- 1 = As
- 2-10 = 2-10
- 11 = J (Jota)
- 12 = Q (Reina)
- 13 = K (Rey)

## 🐛 Troubleshooting

### El acelerómetro no se detecta
- Verificar conexiones I2C (SDA=18, SCL=8)
- Verificar dirección I2C del QMI8658 (0x6B por defecto)
- Abrir Serial Monitor (115200 baud) para ver logs

### Los toques no se detectan
- Ajustar `TAP_THRESHOLD` en línea 32 del código
- Dar toques más fuertes
- Ver valores del acelerómetro en Serial Monitor

### La pausa no funciona
- Ajustar `PAUSE_DURATION_MS` en línea 33
- Esperar más tiempo entre palo y número (3-4 segundos)

### Error al compilar
- Verificar que ESP32 board manager esté instalado
- Actualizar a la última versión
- Limpiar y recompilar: `Sketch → Clean Build Folder`

## 📊 Serial Monitor

El Serial Monitor (115200 baud) muestra:
- Estado de inicialización
- Detección de toques en tiempo real
- Cuenta de toques para palo y número
- Carta decodificada
- Estado de transmisión WiFi

## ⚙️ Configuración Avanzada

Si necesitas ajustar:

```cpp
// Sensibilidad de toques (línea 32)
#define TAP_THRESHOLD 1500.0  // Aumentar = menos sensible

// Tiempo mínimo entre toques (línea 33)
#define TAP_COOLDOWN_MS 200   // Aumentar = toques más lentos

// Duración de la pausa (línea 34)
#define PAUSE_DURATION_MS 2500  // Ajustar según preferencia
```

## 📡 Formato WiFi Beacon

El firmware envía 50 beacons con formato:
```
SSID: "CARD_" + caracteres_invisibles
```

Los caracteres invisibles codifican:
- Palo (1 carácter Unicode)
- Separador
- Número en binario (4 caracteres)

Ver `INTEGRACION_ANDROID.md` para decodificar en la app.
