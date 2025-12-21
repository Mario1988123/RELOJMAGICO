# Reloj Toques - App Android

Aplicación Android para recibir cartas enviadas por el reloj ESP32-S3 vía WiFi beacons.

## 📋 Requisitos

- Android Studio Hedgehog | 2023.1.1 o superior
- Android SDK 24+ (Android 7.0 Nougat)
- Dispositivo Android con WiFi

## 🔧 Compilar el APK

### Opción 1: Android Studio (Recomendado)

1. Abrir Android Studio
2. `File → Open` y seleccionar la carpeta `android_app`
3. Esperar a que Gradle sincronice
4. `Build → Build Bundle(s) / APK(s) → Build APK(s)`
5. El APK estará en `app/build/outputs/apk/debug/app-debug.apk`

### Opción 2: Línea de Comandos

```bash
cd android_app
./gradlew assembleDebug
```

El APK estará en `app/build/outputs/apk/debug/app-debug.apk`

## 📱 Instalar en Android

### Método 1: Android Studio

1. Conectar dispositivo Android por USB
2. Habilitar "Depuración USB" en el dispositivo
3. Click en el botón ▶️ Run en Android Studio

### Método 2: APK Manual

1. Copiar `app-debug.apk` al dispositivo
2. Abrir el APK en el dispositivo
3. Permitir instalación de fuentes desconocidas
4. Instalar

### Método 3: ADB

```bash
adb install app/build/outputs/apk/debug/app-debug.apk
```

## 🎯 Uso de la App

1. **Abrir la app** "Reloj Toques"
2. **Conceder permisos** de ubicación y WiFi cuando se soliciten
3. **Pulsar "Iniciar Escaneo"**
4. **Enviar una carta** desde el reloj ESP32-S3
5. **Ver la carta** aparecer en pantalla

## ⚙️ Permisos Necesarios

La app necesita los siguientes permisos:

- `ACCESS_FINE_LOCATION` - Para escanear WiFi
- `ACCESS_COARSE_LOCATION` - Para escanear WiFi
- `ACCESS_WIFI_STATE` - Para leer estado WiFi
- `CHANGE_WIFI_STATE` - Para iniciar escaneos WiFi

**Nota:** Android requiere permisos de ubicación para escanear redes WiFi (incluso si no se usa GPS).

## 🐛 Troubleshooting

### No se detectan cartas

1. Verificar que los permisos estén concedidos
2. Verificar que WiFi esté activado
3. Verificar que el ESP32 esté enviando beacons
4. Revisar logs en Logcat (filtrar por "CardDecoder")

### Error de compilación

```bash
cd android_app
./gradlew clean
./gradlew assembleDebug
```

### La app se cierra al iniciar

- Verificar que todos los permisos estén concedidos
- Revisar logs en Android Studio Logcat

## 📊 Arquitectura

```
MainActivity
├── CardScannerService (escaneo WiFi)
│   └── WifiManager
├── CardDecoder (decodificación)
└── UI (visualización)
```

## 🔍 Cómo funciona

1. `CardScannerService` escanea redes WiFi cada 500ms
2. Filtra SSIDs que empiecen con "CARD_"
3. `CardDecoder` decodifica los caracteres invisibles
4. Se envía broadcast a `MainActivity`
5. La carta se muestra en pantalla

## 📝 Estructura de Archivos

```
android_app/
├── app/
│   ├── src/main/
│   │   ├── java/com/relojtoques/
│   │   │   ├── MainActivity.kt
│   │   │   ├── CardScannerService.kt
│   │   │   ├── CardDecoder.kt
│   │   │   └── Card.kt
│   │   ├── res/
│   │   │   ├── layout/
│   │   │   │   └── activity_main.xml
│   │   │   └── values/
│   │   │       ├── strings.xml
│   │   │       ├── colors.xml
│   │   │       └── themes.xml
│   │   └── AndroidManifest.xml
│   └── build.gradle
├── build.gradle
└── settings.gradle
```

## 🚀 Próximas Mejoras

- [ ] Imágenes de cartas reales
- [ ] Sonidos al detectar carta
- [ ] Vibración al detectar carta
- [ ] Exportar historial
- [ ] Estadísticas de cartas recibidas
- [ ] Modo oscuro

## 📄 Licencia

Proyecto de código abierto para uso educativo y personal.
