#!/bin/bash

echo "========================================"
echo "  ESP32 S3 AMOLED - Build Script"
echo "========================================"

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Verificar si ESP-IDF está configurado
if ! command -v idf.py &> /dev/null; then
    echo -e "${RED}ERROR: idf.py no encontrado${NC}"
    echo ""
    echo "Ejecuta primero:"
    echo "  cd ~/esp/esp-idf"
    echo "  . ./export.sh"
    echo "  cd -"
    exit 1
fi

echo -e "${GREEN}✓ ESP-IDF encontrado${NC}"
echo ""

# Mostrar archivos nuevos
echo "=== Archivos implementados ==="
echo "Componente BLE HID Keyboard:"
find components/ble_hid_keyboard -type f | sed 's/^/  /'
echo ""
echo "Pantalla Magic Trick:"
find components/gui -name "*magic_trick*" | sed 's/^/  /'
echo ""

# Preguntar acción
echo "¿Qué deseas hacer?"
echo "  1) Compilar (build)"
echo "  2) Compilar y flashear"
echo "  3) Compilar, flashear y monitorear"
echo "  4) Solo monitorear"
echo "  5) Limpiar build"
read -p "Opción (1-5): " option

case $option in
    1)
        echo -e "${YELLOW}Compilando...${NC}"
        idf.py build
        ;;
    2)
        echo -e "${YELLOW}Compilando y flasheando...${NC}"
        read -p "Puerto USB (ej: /dev/ttyUSB0): " port
        idf.py -p "$port" build flash
        ;;
    3)
        echo -e "${YELLOW}Compilando, flasheando y monitoreando...${NC}"
        read -p "Puerto USB (ej: /dev/ttyUSB0): " port
        idf.py -p "$port" build flash monitor
        ;;
    4)
        echo -e "${YELLOW}Monitoreando...${NC}"
        read -p "Puerto USB (ej: /dev/ttyUSB0): " port
        idf.py -p "$port" monitor
        ;;
    5)
        echo -e "${YELLOW}Limpiando build...${NC}"
        rm -rf build
        echo -e "${GREEN}✓ Build limpiado${NC}"
        ;;
    *)
        echo -e "${RED}Opción inválida${NC}"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}=== Listo! ===${NC}"
