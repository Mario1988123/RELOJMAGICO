#!/bin/bash

# Script para migrar automáticamente el código a RELOJTOQUES
# Ejecuta esto desde TU máquina LOCAL (fuera del contenedor)

set -e

echo "══════════════════════════════════════════════════════════════"
echo "  MIGRACIÓN AUTOMÁTICA A RELOJTOQUES"
echo "══════════════════════════════════════════════════════════════"
echo ""

# Variables
RELOJTOQUES_REPO="https://github.com/Mario1988123/RELOJTOQUES.git"
TEMP_DIR="/tmp/relojtoques_migration_$$"
SOURCE_DIR="/home/user/RELOJTOQUES"

echo "📍 Este script hará lo siguiente:"
echo "   1. Clonar el repo RELOJTOQUES"
echo "   2. Copiar todo el código"
echo "   3. Hacer commit"
echo "   4. Push a GitHub"
echo ""
read -p "¿Continuar? (s/n): " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Ss]$ ]]; then
    echo "Cancelado."
    exit 0
fi

echo ""
echo "🔄 Paso 1/4: Clonando repositorio RELOJTOQUES..."
git clone $RELOJTOQUES_REPO $TEMP_DIR
cd $TEMP_DIR

echo ""
echo "📦 Paso 2/4: Copiando archivos desde $SOURCE_DIR..."

# Copiar todo excepto .git
cp -r $SOURCE_DIR/.github .
cp -r $SOURCE_DIR/android_app .
cp -r $SOURCE_DIR/esp32_arduino .
cp $SOURCE_DIR/.gitignore .
cp $SOURCE_DIR/README.md .
cp $SOURCE_DIR/INSTALL.md .
cp $SOURCE_DIR/QUICKSTART.md .
cp $SOURCE_DIR/PUSH_TO_GITHUB.sh .
cp $SOURCE_DIR/INSTRUCCIONES_PUSH.sh .
cp $SOURCE_DIR/README_PUSH.txt .

echo ""
echo "✅ Archivos copiados:"
ls -la

echo ""
echo "📝 Paso 3/4: Haciendo commit..."
git add -A
git commit -m "Initial commit: Sistema completo Reloj Toques con GitHub Actions

Sistema para transmitir cartas de poker mediante toques en ESP32-S3
con visualización en app Android.

Componentes:
- ESP32-S3 Arduino (.ino): Detección de toques + WiFi beacons
- App Android (Kotlin): Escaneo WiFi + decodificación + UI
- GitHub Actions: Compilación automática de APK
- Documentación completa

Características:
- Sistema de toques: palo (1-4) + pausa + número (1-13)
- Protocolo WiFi con caracteres Unicode invisibles
- CI/CD automático para generar APKs

Listo para usar."

echo ""
echo "🚀 Paso 4/4: Haciendo push a GitHub..."
git push -u origin main

if [ $? -eq 0 ]; then
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  ✅ ¡MIGRACIÓN EXITOSA!"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    echo "🎉 El código está ahora en:"
    echo "   https://github.com/Mario1988123/RELOJTOQUES"
    echo ""
    echo "🤖 GitHub Actions está compilando el APK..."
    echo "   https://github.com/Mario1988123/RELOJTOQUES/actions"
    echo ""
    echo "⏱️  Espera ~5 minutos para descargar el APK"
    echo ""
    echo "📱 Descarga el APK de 'Artifacts' o crea un Release"
    echo ""

    # Limpiar
    echo "🧹 Limpiando archivos temporales..."
    cd /
    rm -rf $TEMP_DIR

else
    echo ""
    echo "══════════════════════════════════════════════════════════════"
    echo "  ❌ ERROR AL HACER PUSH"
    echo "══════════════════════════════════════════════════════════════"
    echo ""
    echo "El repositorio temporal está en: $TEMP_DIR"
    echo "Puedes intentar push manual desde ahí:"
    echo "  cd $TEMP_DIR"
    echo "  git push -u origin main"
    echo ""
    exit 1
fi
