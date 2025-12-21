# 🚀 Guía de Instalación - Reloj Toques

Todo el código está listo y commiteado localmente. Sigue estos pasos para completar la instalación.

## ✅ Estado Actual

- ✅ Código ESP32 Arduino creado (`RELOJTOQUES.ino`)
- ✅ App Android completa creada (Kotlin)
- ✅ Documentación completa
- ✅ Commit local realizado
- ⏳ **Falta: Push al repositorio remoto**

## 📤 Paso 1: Push al Repositorio

Desde tu terminal local (no el contenedor):

```bash
# Navegar al directorio
cd /ruta/donde/clonaste/RELOJTOQUES

# Verificar que tienes los archivos
git status

# Push al repo
git push -u origin main
```

Si te pide credenciales:
- Usuario: `Mario1988123`
- Password: Tu token de GitHub

## 📱 Paso 2: Compilar la App Android

### Opción A: Android Studio (Recomendado)

1. Abrir Android Studio
2. `File → Open → Seleccionar carpeta android_app`
3. Esperar sincronización de Gradle
4. `Build → Build Bundle(s) / APK(s) → Build APK(s)`
5. APK en: `android_app/app/build/outputs/apk/debug/app-debug.apk`

### Opción B: Línea de Comandos

```bash
cd android_app
./gradlew assembleDebug
```

APK estará en: `app/build/outputs/apk/debug/app-debug.apk`

### Instalación en Android

**Método 1: USB**
```bash
adb install app/build/outputs/apk/debug/app-debug.apk
```

**Método 2: Manual**
1. Copiar `app-debug.apk` al celular
2. Abrir archivo en el celular
3. Permitir instalación de fuentes desconocidas
4. Instalar

## 🔧 Paso 3: Programar el ESP32-S3

### Requisitos
- Arduino IDE 2.0+
- ESP32 Board Support instalado

### Instalación

1. **Configurar Arduino IDE**
   - `File → Preferences`
   - Additional Board Manager URLs:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - `Tools → Board → Boards Manager`
   - Instalar "esp32 by Espressif Systems"

2. **Abrir Sketch**
   - Abrir `esp32_arduino/RELOJTOQUES.ino`

3. **Configurar Board**
   - `Tools → Board → ESP32 Arduino → ESP32S3 Dev Module`
   - `Tools → USB CDC On Boot → Enabled`
   - `Tools → Port → [Seleccionar puerto COM]`

4. **Cargar**
   - Click en botón Upload (→)
   - Esperar compilación y carga

## 🎯 Paso 4: Probar el Sistema

### En el ESP32
1. Abrir Serial Monitor (115200 baud)
2. Ver instrucciones en pantalla
3. Dar toques para probar

### En Android
1. Abrir app "Reloj Toques"
2. Conceder permisos de ubicación y WiFi
3. Presionar "Iniciar Escaneo"

### Prueba Completa
1. En ESP32: Dar **1 toque** (Corazones ♥)
2. **Esperar 2-3 segundos**
3. En ESP32: Dar **3 toques** (número 3)
4. En Android: Debería aparecer "3 de Corazones ♥"

## 🐛 Troubleshooting

### ESP32 no compila
- Verificar que ESP32 board support esté instalado
- Reiniciar Arduino IDE
- Verificar selección de board

### Android no compila
- Verificar que Java JDK esté instalado
- Ejecutar `./gradlew clean`
- Sincronizar Gradle en Android Studio

### No se detectan cartas
- Verificar permisos en Android
- Verificar WiFi activado
- Ver Serial Monitor del ESP32
- Usar WiFi Analyzer para ver beacons

## 📁 Estructura de Archivos Creados

```
/home/user/RELOJTOQUES/
├── README.md                   # Documentación principal
├── INSTALL.md                  # Este archivo
├── .gitignore                  # Git ignore
│
├── esp32_arduino/
│   ├── RELOJTOQUES.ino        # ← Cargar esto en Arduino IDE
│   └── README.md
│
└── android_app/
    ├── app/
    │   ├── src/main/
    │   │   ├── java/com/relojtoques/
    │   │   │   ├── MainActivity.kt
    │   │   │   ├── CardScannerService.kt
    │   │   │   ├── CardDecoder.kt
    │   │   │   └── Card.kt
    │   │   ├── res/
    │   │   └── AndroidManifest.xml
    │   └── build.gradle
    ├── build.gradle
    ├── settings.gradle
    └── README.md
```

## 📝 Notas Importantes

1. **Permisos Android**: La app necesita permisos de ubicación para escanear WiFi
2. **Hardware ESP32**: Verificar que el acelerómetro QMI8658 esté conectado:
   - SDA → GPIO 18
   - SCL → GPIO 8
3. **Rango WiFi**: El ESP32 y el Android deben estar cerca (< 10m)

## ✨ Siguiente Paso

Una vez hecho el push, todo estará en:
`https://github.com/Mario1988123/RELOJTOQUES`

Y podrás:
- Clonar desde cualquier lugar
- Compartir el repositorio
- Compilar en diferentes máquinas

## 🎉 ¡Listo!

Si seguiste todos los pasos, deberías tener:
- ✅ Código en GitHub
- ✅ ESP32 programado
- ✅ App Android instalada
- ✅ Sistema funcionando

---

**¿Problemas?** Revisa:
1. `esp32_arduino/README.md` para detalles del ESP32
2. `android_app/README.md` para detalles de Android
3. Serial Monitor del ESP32 para diagnóstico
4. Logcat de Android para debugging
