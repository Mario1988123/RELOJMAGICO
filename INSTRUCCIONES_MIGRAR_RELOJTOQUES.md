# 🚀 MIGRAR TODO A RELOJTOQUES - Instrucciones

## ⚡ OPCIÓN 1: Script Automático (RECOMENDADO)

Ejecuta desde TU máquina local (fuera del contenedor):

```bash
cd /ruta/a/RELOJMAGICO
./MIGRAR_A_RELOJTOQUES.sh
```

Este script hará TODO automáticamente:
1. ✅ Clona RELOJTOQUES
2. ✅ Copia todos los archivos
3. ✅ Hace commit
4. ✅ Push a GitHub

**¡Y listo!** En 5 minutos tendrás el APK compilado en GitHub Actions.

---

## 📋 OPCIÓN 2: Manual (Paso a Paso)

Si prefieres hacerlo manualmente:

### Paso 1: Clonar RELOJTOQUES

```bash
git clone https://github.com/Mario1988123/RELOJTOQUES.git
cd RELOJTOQUES
```

### Paso 2: Copiar archivos

Copia estos archivos/carpetas desde `/home/user/RELOJTOQUES/`:

```bash
cp -r /home/user/RELOJTOQUES/.github .
cp -r /home/user/RELOJTOQUES/android_app .
cp -r /home/user/RELOJTOQUES/esp32_arduino .
cp /home/user/RELOJTOQUES/.gitignore .
cp /home/user/RELOJTOQUES/README.md .
cp /home/user/RELOJTOQUES/INSTALL.md .
cp /home/user/RELOJTOQUES/QUICKSTART.md .
cp /home/user/RELOJTOQUES/PUSH_TO_GITHUB.sh .
cp /home/user/RELOJTOQUES/INSTRUCCIONES_PUSH.sh .
cp /home/user/RELOJTOQUES/README_PUSH.txt .
```

### Paso 3: Commit

```bash
git add -A
git commit -m "Initial commit: Sistema completo con GitHub Actions"
```

### Paso 4: Push

```bash
git push -u origin main
```

---

## 📦 OPCIÓN 3: Usar código desde RELOJMAGICO

El código también está en RELOJMAGICO (como backup):

```
https://github.com/Mario1988123/RELOJMAGICO/tree/claude/clock-tap-wifi-messaging-e1YfW/relojcontoques_completo
```

Puedes copiarlo desde ahí si prefieres.

---

## ✅ VERIFICAR QUE TODO FUNCIONÓ

Después del push, verifica:

1. **Código en GitHub:**
   ```
   https://github.com/Mario1988123/RELOJTOQUES
   ```

2. **GitHub Actions corriendo:**
   ```
   https://github.com/Mario1988123/RELOJTOQUES/actions
   ```

3. **APK disponible en ~5 min** en Artifacts

---

## 📱 DESCARGAR EL APK

### Opción A: Desde Actions (automático)

1. Ve a: https://github.com/Mario1988123/RELOJTOQUES/actions
2. Click en "Build Android APK"
3. Click en el run más reciente (círculo verde ✅)
4. Scroll down a "Artifacts"
5. Descarga `app-debug` o `app-release`
6. Descomprime el ZIP → tendrás el APK

### Opción B: Crear Release

1. Ve a: https://github.com/Mario1988123/RELOJTOQUES/actions
2. Click en "Create Release with APK"
3. Click "Run workflow"
4. Ingresa versión: `v1.0.0`
5. Click "Run workflow"
6. Espera ~5 min
7. APK estará en: https://github.com/Mario1988123/RELOJTOQUES/releases

---

## 📝 ESTRUCTURA DE ARCHIVOS

Todo lo que se copiará:

```
RELOJTOQUES/
├── .github/workflows/
│   ├── build-apk.yml      ← Compila APK en cada push
│   └── release.yml        ← Crea releases con APKs
│
├── esp32_arduino/
│   ├── RELOJTOQUES.ino    ← Código Arduino para ESP32
│   └── README.md
│
├── android_app/
│   ├── app/
│   │   └── src/main/
│   │       ├── java/com/relojtoques/
│   │       │   ├── MainActivity.kt
│   │       │   ├── CardScannerService.kt
│   │       │   ├── CardDecoder.kt
│   │       │   └── Card.kt
│   │       ├── res/
│   │       └── AndroidManifest.xml
│   ├── build.gradle
│   ├── gradlew
│   └── README.md
│
├── README.md              ← Documentación principal
├── QUICKSTART.md          ← Inicio rápido (3 pasos)
├── INSTALL.md             ← Guía detallada
└── .gitignore
```

---

## 🎯 SIGUIENTE PASO

Después de migrar:

1. **Programar ESP32:**
   - Abrir `esp32_arduino/RELOJTOQUES.ino` en Arduino IDE
   - Upload al ESP32-S3

2. **Instalar APK:**
   - Descargar desde Actions/Releases
   - Instalar en Android

3. **¡Usar!**
   - Dar toques en el reloj
   - Ver cartas en la app

---

## ❓ PROBLEMAS

### El script no funciona
- Verifica que tengas Git instalado
- Asegúrate de tener permisos para push
- Intenta la opción manual

### No puedo hacer push
- Verifica autenticación: `git config --global user.name`
- Usa un token de GitHub si es necesario
- Verifica que el repo existe: https://github.com/Mario1988123/RELOJTOQUES

### GitHub Actions falla
- Revisa los logs en Actions
- Verifica que los archivos se copiaron correctamente
- Los workflows están en `.github/workflows/`

---

**¿Más ayuda?** Lee `README.md`, `QUICKSTART.md` e `INSTALL.md` en el repo.
