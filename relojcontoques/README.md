# Reloj con Toques - WiFi Beacon Card Transmitter

Sistema para enviar cartas de poker mediante toques en un reloj ESP32-S3 con acelerómetro QMI8658. Las cartas se transmiten vía WiFi beacons con caracteres Unicode invisibles.

## 🎯 Características

- **Detección de toques** usando acelerómetro QMI8658
- **Decodificación de cartas** mediante secuencia de toques
- **Transmisión WiFi** con beacons usando caracteres invisibles
- **Fácil de usar** sin necesidad de pantalla o botones

## 🃏 Cómo Usar

### Seleccionar una Carta

1. **Paso 1: Seleccionar el PALO** (dar toques cortos)
   - 1 toque = Corazones ♥
   - 2 toques = Picas ♠
   - 3 toques = Tréboles ♣
   - 4 toques = Diamantes ♦

2. **Paso 2: Esperar 2-3 segundos** (pausa para separar palo y número)

3. **Paso 3: Seleccionar el NÚMERO** (dar toques cortos)
   - 1 toque = As (A)
   - 2-10 toques = 2-10
   - 11 toques = Jota (J)
   - 12 toques = Reina (Q)
   - 13 toques = Rey (K)

### Ejemplo: Enviar "3 de Corazones"

```
1. Dar 1 toque corto en el reloj (♥ Corazones)
2. Esperar 2-3 segundos
3. Dar 3 toques cortos (número 3)
4. La carta se enviará automáticamente por WiFi
```

### Ejemplo: Enviar "Rey de Diamantes"

```
1. Dar 4 toques cortos (♦ Diamantes)
2. Esperar 2-3 segundos
3. Dar 13 toques cortos (Rey)
4. La carta se enviará automáticamente por WiFi
```

## 🔧 Compilación y Flash

### Requisitos

- ESP-IDF v5.0 o superior
- ESP32-S3 con acelerómetro QMI8658
- Cable USB para programación

### Comandos

```bash
# Navegar a la carpeta del proyecto
cd relojcontoques

# Configurar el proyecto (primera vez)
idf.py set-target esp32s3
idf.py menuconfig  # Opcional: ajustar configuración

# Compilar
idf.py build

# Flash al dispositivo
idf.py -p /dev/ttyUSB0 flash monitor

# O flash + monitor en un comando
idf.py -p /dev/ttyUSB0 flash monitor
```

## 📡 Formato del Beacon WiFi

Los beacons WiFi usan el siguiente formato:

```
SSID: "CARD_" + caracteres_invisibles
```

### Codificación

Los caracteres invisibles Unicode codifican la carta:

- **Palo** (1 carácter):
  - `U+200B` (ZWSP) = Corazones ♥
  - `U+200C` (ZWNJ) = Picas ♠
  - `U+200D` (ZWJ) = Tréboles ♣
  - `U+200E` (LRM) = Diamantes ♦

- **Separador**: `U+200B` (ZWSP)

- **Número** (4 caracteres, binario):
  - `U+200B` = bit 0
  - `U+200C` = bit 1
  - Ejemplo: número 3 = `0011` = ZWSP+ZWSP+ZWNJ+ZWNJ

### Decodificación en Receptor

Para decodificar en la aplicación receptora (ej. NotesVoice):

```javascript
// Pseudocódigo
function decodeCard(ssid) {
    // Remover prefijo "CARD_"
    let encoded = ssid.substring(5);

    // Extraer primer carácter = palo
    let suitChar = encoded[0];
    let suit = decodeSuit(suitChar);

    // Saltar separador
    // Extraer siguientes 4 caracteres = número en binario
    let numberBits = encoded.substring(2, 6);
    let number = decodeNumber(numberBits);

    return {suit, number};
}
```

## 🔌 Hardware

### Pines I2C del QMI8658

```
SCL: GPIO 8
SDA: GPIO 18
```

### Configuración del Acelerómetro

- **Rango**: ±4g
- **ODR**: 125 Hz
- **Modo**: Solo acelerómetro (sin giroscopio)

## 🐛 Troubleshooting

### El acelerómetro no se detecta

1. Verificar conexiones I2C (SCL=GPIO8, SDA=GPIO18)
2. Verificar pull-ups en las líneas I2C
3. Revisar logs: `idf.py monitor`

### Los toques no se detectan

1. Ajustar `TAP_THRESHOLD_MG` en `tap_detector.c`
2. Dar toques más fuertes
3. Revisar logs para ver valores del acelerómetro

### La pausa no se detecta correctamente

1. Ajustar `PAUSE_DURATION_MS` en `card_decoder.c` (por defecto 2500ms)
2. Esperar más tiempo entre palo y número

### Los beacons WiFi no se ven

1. Verificar que WiFi esté inicializado correctamente
2. Usar un escáner WiFi (ej. WiFi Analyzer en Android)
3. Buscar SSIDs que empiecen con "CARD_"

## 📝 Archivos del Proyecto

```
relojcontoques/
├── CMakeLists.txt              # Configuración CMake principal
├── sdkconfig.defaults          # Configuración por defecto
├── README.md                   # Este archivo
└── main/
    ├── CMakeLists.txt          # Configuración del componente main
    ├── main.c                  # Programa principal
    ├── tap_detector.h/c        # Módulo de detección de toques
    ├── card_decoder.h/c        # Módulo de decodificación de cartas
    └── wifi_beacon.h/c         # Módulo de transmisión WiFi
```

## 🚀 Próximos Pasos

- [ ] Integrar con app NotesVoice para mostrar la carta
- [ ] Añadir feedback táctil (vibración) al completar carta
- [ ] Añadir feedback sonoro
- [ ] Optimizar consumo de energía
- [ ] Añadir pantalla OLED para visualizar estado
- [ ] Soporte para gestos adicionales (shake para reset)

## 📄 Licencia

Este proyecto es parte del repositorio RELOJMAGICO.

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Por favor, abre un issue o pull request.
