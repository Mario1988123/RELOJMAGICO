#!/bin/bash

echo "=========================================="
echo "  RELOJ TOQUES - Push automático a GitHub"
echo "=========================================="
echo ""

# Verificar que estamos en el directorio correcto
if [ ! -f "README.md" ] || [ ! -d "esp32_arduino" ]; then
    echo "❌ Error: Ejecuta este script desde el directorio RELOJTOQUES"
    exit 1
fi

# Mostrar estado
echo "📊 Estado actual:"
git status
echo ""

# Preguntar al usuario
read -p "¿Hacer push a GitHub? (s/n): " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Ss]$ ]]; then
    echo "Cancelado."
    exit 0
fi

echo ""
echo "🚀 Haciendo push a GitHub..."
echo ""

# Push con reintentos
git push -u origin main || \
(sleep 2 && git push -u origin main) || \
(sleep 4 && git push -u origin main) || \
(sleep 8 && git push -u origin main)

if [ $? -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo "  ✅ PUSH EXITOSO!"
    echo "=========================================="
    echo ""
    echo "🎉 El código está ahora en:"
    echo "   https://github.com/Mario1988123/RELOJTOQUES"
    echo ""
    echo "📱 GitHub Actions compilará el APK automáticamente"
    echo "   Ve a: https://github.com/Mario1988123/RELOJTOQUES/actions"
    echo ""
    echo "📥 El APK estará disponible en 'Artifacts' después de ~5 min"
    echo ""
else
    echo ""
    echo "=========================================="
    echo "  ❌ ERROR AL HACER PUSH"
    echo "=========================================="
    echo ""
    echo "Intenta manualmente:"
    echo "  git push -u origin main"
    echo ""
    exit 1
fi
