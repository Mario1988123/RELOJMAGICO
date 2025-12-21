# 🃏 Reloj Toques

Sistema completo para transmitir cartas de poker mediante toques en un reloj ESP32-S3 con acelerómetro. Las cartas se envían vía WiFi beacons con caracteres invisibles y se visualizan en una app Android.

## 🎯 Descripción

**Reloj Toques** es un sistema de comunicación ingenioso que permite:
1. Seleccionar una carta mediante toques en un reloj
2. Transmitir la carta por WiFi usando beacons con caracteres invisibles
3. Visualizar la carta en una app Android

### ¿Cómo funciona?

1. **Dar toques en el reloj** para seleccionar el palo (1-4 toques)
2. **Esperar 2-3 segundos** (pausa)
3. **Dar toques** para seleccionar el número (1-13 toques)
4. La carta se **transmite automáticamente** por WiFi
5. La **app Android detecta y muestra** la carta

## 📁 Estructura del Proyecto

```
RELOJTOQUES/
├── esp32_arduino/          # Firmware Arduino para ESP32-S3
│   ├── RELOJTOQUES.ino    # Código principal
│   └── README.md          # Instrucciones ESP32
│
└── android_app/           # App Android receptor
    ├── app/               # Código fuente Android
    └── README.md          # Instrucciones Android
```

## 🔧 Hardware Necesario

### ESP32-S3 (Transmisor)
- **Microcontrolador**: ESP32-S3 (o ESP32)
- **Acelerómetro**: QMI8658 (I2C)
- **Conexiones**:
  - SDA → GPIO 18
  - SCL → GPIO 8

### Android (Receptor)
- Dispositivo Android 7.0+ (API 24+)
- WiFi activado

## 🚀 Inicio Rápido

### 1. Programar el ESP32-S3

```bash
cd esp32_arduino
```

1. Abrir `RELOJTOQUES.ino` en Arduino IDE
2. Seleccionar board "ESP32S3 Dev Module"
3. Cargar el sketch al ESP32

Ver [esp32_arduino/README.md](esp32_arduino/README.md) para más detalles.

### 2. Compilar la App Android

```bash
cd android_app
./gradlew assembleDebug
```

O abrir en Android Studio y compilar.

Ver [android_app/README.md](android_app/README.md) para más detalles.

### 3. ¡Usar el Sistema!

1. Instalar la app en Android
2. Abrir la app y presionar "Iniciar Escaneo"
3. Dar toques en el ESP32 para enviar una carta
4. Ver la carta en la app Android

## 🎯 Ejemplo de Uso

### Enviar "3 de Corazones ♥"

1. **1 toque** corto en el reloj (Corazones)
2. **Esperar 2-3 segundos**
3. **3 toques** cortos (número 3)
4. ✅ ¡La carta aparece en la app!

### Codificación de Palos

| Toques | Palo | Símbolo |
|--------|------|---------|
| 1 | Corazones | ♥ |
| 2 | Picas | ♠ |
| 3 | Tréboles | ♣ |
| 4 | Diamantes | ♦ |

### Codificación de Números

| Toques | Carta |
|--------|-------|
| 1 | As |
| 2-10 | 2-10 |
| 11 | J (Jota) |
| 12 | Q (Reina) |
| 13 | K (Rey) |

## 📡 Protocolo WiFi Beacon

### Formato del SSID

```
"CARD_" + [caracteres invisibles Unicode]
```

### Caracteres Invisibles Usados

- `U+200B` (ZWSP) - Zero Width Space
- `U+200C` (ZWNJ) - Zero Width Non-Joiner
- `U+200D` (ZWJ) - Zero Width Joiner
- `U+200E` (LRM) - Left-to-Right Mark

### Codificación

```
SSID = "CARD_" + [PALO] + [SEPARADOR] + [NÚMERO_BINARIO]
```

**Ejemplo: 3 de Corazones**
```
"CARD_" + ZWSP (♥) + ZWSP (sep) + 0011 (binario)
```

## 🛠️ Configuración Avanzada

### Ajustar Sensibilidad del ESP32

Editar `RELOJTOQUES.ino`:

```cpp
#define TAP_THRESHOLD 1500.0    // ↑ Menos sensible
#define PAUSE_DURATION_MS 2500  // Tiempo de pausa
```

### Ajustar Frecuencia de Escaneo Android

Editar `CardScannerService.kt`:

```kotlin
const val SCAN_INTERVAL_MS = 500L  // Frecuencia de escaneo
```

## 🐛 Troubleshooting

### El ESP32 no detecta toques
- Verificar conexión del acelerómetro
- Ajustar `TAP_THRESHOLD`
- Ver Serial Monitor para diagnóstico

### La app no detecta cartas
- Verificar permisos de ubicación y WiFi
- Verificar que WiFi esté activado
- Usar WiFi Analyzer para ver beacons

### La pausa no funciona
- Esperar 3-4 segundos entre palo y número
- Ajustar `PAUSE_DURATION_MS`

## 📊 Diagrama de Flujo

```
[Toques ESP32] → [Decodificación] → [WiFi Beacon] → [App Android] → [Display]
      ↓               ↓                    ↓              ↓             ↓
  QMI8658      Palo + Número      Caracteres        Escaneo      Mostrar
                                  invisibles          WiFi        carta
```

## 📝 Licencia

Proyecto de código abierto para uso educativo y personal.

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Por favor:
1. Fork el repositorio
2. Crea una rama con tu feature
3. Commit tus cambios
4. Push a la rama
5. Abre un Pull Request

## 📞 Soporte

Si tienes problemas:
1. Revisa la documentación en `/esp32_arduino/README.md` y `/android_app/README.md`
2. Verifica las conexiones de hardware
3. Revisa los logs (Serial Monitor en ESP32, Logcat en Android)

## 🎉 ¡Disfruta!

Este proyecto combina hardware, firmware, comunicación inalámbrica y desarrollo móvil en un sistema completo y funcional.

---

**Autor**: Mario1988123
**Repositorio**: https://github.com/Mario1988123/RELOJTOQUES
