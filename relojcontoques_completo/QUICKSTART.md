# 🚀 INICIO RÁPIDO - 3 PASOS

## Paso 1: Push a GitHub (1 comando)

```bash
cd /home/user/RELOJTOQUES
./PUSH_TO_GITHUB.sh
```

O manualmente:
```bash
git push -u origin main
```

## Paso 2: Descargar APK (AUTOMÁTICO)

**GitHub Actions compilará el APK automáticamente en ~5 minutos**

1. Ve a: https://github.com/Mario1988123/RELOJTOQUES/actions
2. Click en el workflow "Build Android APK"
3. Click en el run más reciente
4. Baja hasta "Artifacts"
5. Descarga `app-debug` o `app-release`
6. Descomprime el ZIP → obtienes el APK

### O crea un Release:

1. Ve a: https://github.com/Mario1988123/RELOJTOQUES/actions
2. Click en "Create Release with APK"
3. Click "Run workflow"
4. Elige versión (ej: v1.0.0)
5. El APK estará en: https://github.com/Mario1988123/RELOJTOQUES/releases

## Paso 3: Programar ESP32

1. Abrir Arduino IDE
2. Abrir `esp32_arduino/RELOJTOQUES.ino`
3. Seleccionar board "ESP32S3 Dev Module"
4. Upload

## ✅ ¡LISTO!

- 🤖 GitHub Actions compila el APK automáticamente en cada push
- 📦 Los APKs están disponibles en Artifacts
- 🎉 Los releases se crean con un click

---

## 📱 Instalar APK en Android

1. Descargar APK del artifact/release
2. Copiar a Android
3. Permitir instalación de fuentes desconocidas
4. Instalar

## 🎯 Usar el Sistema

1. Abrir app "Reloj Toques"
2. Dar permisos de ubicación y WiFi
3. Presionar "Iniciar Escaneo"
4. En ESP32: 1 toque (palo) + pausa 2-3s + toques (número)
5. ¡Ver carta en app!

---

**Documentación completa**: Ver `README.md` y `INSTALL.md`
