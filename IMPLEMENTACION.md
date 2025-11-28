# 📱 Guía de Implementación - ESP32 S3 AMOLED

## 🎯 Funcionalidades Implementadas

### 1️⃣ Ratón Bluetooth HID
- Panel DRAW puede mover cursor del móvil
- Modo MOUSE / Modo DRAW (botón "Mode")
- Dibujar en app de Notas del móvil

### 2️⃣ Truco de Magia (Cartas)
- Selector de PALO (01-04)
- Selector de VALOR (01-13)
- Envía texto de carta al móvil vía teclado HID

### 3️⃣ Icono Bluetooth Funcional
- Muestra estado real de conexión HID
- Se actualiza automáticamente

---

## 🚀 Cómo Implementar

### Opción A: Usar el Script (Recomendado)

```bash
# 1. Activar ESP-IDF
cd ~/esp/esp-idf
. ./export.sh
cd -

# 2. Ejecutar script de build
./build.sh
```

El script te preguntará qué hacer:
- Opción 1: Solo compilar
- Opción 2: Compilar y flashear
- Opción 3: Compilar, flashear y ver logs
- Opción 4: Solo ver logs
- Opción 5: Limpiar build

---

### Opción B: Comandos Manuales

```bash
# 1. Activar ESP-IDF
cd ~/esp/esp-idf
. ./export.sh
cd /home/user/RELOJMAGICO

# 2. Compilar
idf.py build

# 3. Flashear (conecta ESP32 por USB)
idf.py -p /dev/ttyUSB0 flash

# 4. Ver logs (opcional)
idf.py -p /dev/ttyUSB0 monitor
```

**Puertos comunes:**
- Linux: `/dev/ttyUSB0` o `/dev/ttyACM0`
- macOS: `/dev/cu.usbserial-*`
- Windows: `COM3`, `COM4`, etc.

---

## 📱 Emparejamiento Bluetooth

Después de flashear, desde tu móvil:

### 1. Emparejar Mouse
1. Ajustes → Bluetooth
2. Buscar "**S3Watch Mouse**"
3. Emparejar (sin PIN)

### 2. Emparejar Teclado
1. Ajustes → Bluetooth
2. Buscar "**S3Watch Keyboard**"
3. Emparejar (sin PIN)

---

## 🎮 Cómo Usar

### En el Reloj (Watchface):

| Gesto | Acción |
|-------|--------|
| **Toque corto** | Abre DRAW/MOUSE |
| **Toque largo (2s)** | Abre Magic Trick |
| Swipe ← | También abre DRAW |
| Swipe → | Steps |
| Swipe ↑ | Control Panel |
| Swipe ↓ | Notificaciones |

### Panel DRAW/MOUSE:

1. **Botón "Mode"** (azul): Cambia entre MOUSE ↔ DRAW
2. **Botón "Clear"** (rojo): Limpia pantalla
3. **Estado**: Muestra si está conectado
4. **Touch y arrastra**:
   - Modo MOUSE: mueve cursor en móvil
   - Modo DRAW: dibuja en pantalla del reloj

### Magic Trick (Truco de Cartas):

1. **Botones +/- PALO**: Cambia palo (1-4)
   - 01 = ♥ Corazones
   - 02 = ♠ Picas
   - 03 = ♣ Tréboles
   - 04 = ♦ Diamantes

2. **Botones +/- VALOR**: Cambia valor (1-13)
   - 01 = As
   - 02-10 = Números
   - 11 = J
   - 12 = Q
   - 13 = K

3. **"ENVIAR CARTA"**: Envía al móvil (ej: "AS de CORAZONES")

---

## 🧪 Pruebas

### Probar Mouse HID:
1. Empareja "S3Watch Mouse"
2. Abre app de Notas en móvil
3. En reloj: Toque corto → Mode → MOUSE
4. Arrastra dedo → cursor se mueve
5. ¡Dibuja desde el reloj!

### Probar Truco de Magia:
1. Empareja "S3Watch Keyboard"
2. Abre app de Notas en móvil
3. En reloj: Toque largo → Magic Trick
4. Selecciona: Palo=1, Valor=1
5. Presiona "ENVIAR CARTA"
6. En móvil aparece: "AS de CORAZONES"

---

## 📦 Archivos Creados

### Componente BLE HID Keyboard:
```
components/ble_hid_keyboard/
├── CMakeLists.txt
├── include/
│   └── ble_hid_keyboard.h
└── src/
    └── ble_hid_keyboard.c
```

### Pantalla Magic Trick:
```
components/gui/
├── include/
│   └── magic_trick_screen.h
└── src/
    └── magic_trick_screen.c
```

### Archivos Modificados:
- `main/main.cpp` - Inicializa HID Mouse + Keyboard
- `components/gui/src/draw_screen.c` - Integración Mouse
- `components/gui/src/watchface.c` - Estado BT + acceso Magic Trick
- `components/gui/CMakeLists.txt` - Dependencias

---

## 🐛 Solución de Problemas

### Error: "idf.py: command not found"
```bash
cd ~/esp/esp-idf
. ./export.sh
```

### Error: "Port already in use"
Otro programa está usando el puerto serial:
```bash
# Cerrar monitor anterior (Ctrl+])
# O matar proceso:
sudo killall screen
```

### No aparece el dispositivo BLE
- Verifica que Bluetooth del móvil esté ON
- Reinicia el ESP32 (botón RESET)
- Mira los logs: `idf.py monitor`
- Busca: "BLE HID Mouse READY" y "BLE HID Keyboard READY"

### El mouse no mueve el cursor
- Asegúrate de emparejar "S3Watch Mouse" primero
- Verifica que estás en modo MOUSE (botón "Mode")
- Revisa que dice "[Connected]" en pantalla

### El truco de magia no escribe
- Asegúrate de emparejar "S3Watch Keyboard"
- Abre una app donde se pueda escribir (Notas, WhatsApp, etc.)
- Verifica que dice "[Connected]" en pantalla

---

## 📝 Logs Útiles

Para ver qué está pasando:
```bash
idf.py -p /dev/ttyUSB0 monitor

# Busca estos mensajes:
# "BLE HID Mouse READY"
# "BLE HID Keyboard READY"
# "HID CONNECT"
# "Mouse move: dx=X, dy=Y"
# "Enviando carta: AS de CORAZONES"
```

---

## ✅ Checklist Final

- [ ] ESP-IDF instalado y configurado
- [ ] Proyecto compilado sin errores
- [ ] Firmware flasheado al ESP32
- [ ] "S3Watch Mouse" emparejado
- [ ] "S3Watch Keyboard" emparejado
- [ ] Probado dibujar en app de Notas
- [ ] Probado enviar carta desde Magic Trick

---

## 🎉 ¡Listo!

Si todo funciona:
- ✅ Puedes dibujar en el móvil desde el reloj
- ✅ Puedes hacer trucos de magia enviando cartas
- ✅ El icono Bluetooth muestra el estado real

**Disfruta tu smartwatch mejorado!** 🚀
