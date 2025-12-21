#!/bin/bash

# Script de build para Reloj con Toques

set -e

echo "======================================"
echo "   Reloj con Toques - Build Script"
echo "======================================"

# Verificar que ESP-IDF esté configurado
if [ -z "$IDF_PATH" ]; then
    echo "ERROR: ESP-IDF no está configurado"
    echo "Por favor ejecuta: source \$HOME/esp/esp-idf/export.sh"
    exit 1
fi

# Configurar target
echo "Configurando target ESP32-S3..."
idf.py set-target esp32s3

# Compilar
echo "Compilando proyecto..."
idf.py build

echo ""
echo "======================================"
echo "   ✓ Compilación exitosa"
echo "======================================"
echo ""
echo "Para flashear el dispositivo:"
echo "  idf.py -p /dev/ttyUSB0 flash monitor"
echo ""
