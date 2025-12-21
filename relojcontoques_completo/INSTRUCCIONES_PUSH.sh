#!/bin/bash

# Script para hacer push desde TU máquina local
# Ejecuta esto FUERA del contenedor Docker

echo "=========================================="
echo "  PUSH AUTOMÁTICO A GITHUB - RELOJTOQUES"
echo "=========================================="
echo ""
echo "📍 Asegúrate de estar en el directorio RELOJTOQUES"
echo ""

# Verificar que estamos en el lugar correcto
if [ ! -f "README.md" ] || [ ! -d "esp32_arduino" ]; then
    echo "❌ Error: Debes estar en el directorio RELOJTOQUES"
    echo ""
    echo "Ejecuta primero:"
    echo "  cd /ruta/donde/clonaste/RELOJTOQUES"
    echo ""
    exit 1
fi

echo "✅ Directorio correcto detectado"
echo ""
echo "📊 Verificando estado de Git..."
echo ""

git status

echo ""
echo "📝 Commits locales:"
git log --oneline

echo ""
echo "🌐 Remote configurado:"
git remote -v

echo ""
echo "════════════════════════════════════════"
echo "  ¿Todo se ve bien? Vamos a hacer PUSH"
echo "════════════════════════════════════════"
echo ""
read -p "Presiona ENTER para continuar o Ctrl+C para cancelar..."
echo ""

echo "🚀 Haciendo push a GitHub..."
echo ""

# Push con reintentos
git push -u origin main

if [ $? -eq 0 ]; then
    echo ""
    echo "════════════════════════════════════════"
    echo "  ✅ ¡PUSH EXITOSO!"
    echo "════════════════════════════════════════"
    echo ""
    echo "🎉 El código está ahora en GitHub:"
    echo "   https://github.com/Mario1988123/RELOJTOQUES"
    echo ""
    echo "🤖 GitHub Actions está compilando el APK..."
    echo "   https://github.com/Mario1988123/RELOJTOQUES/actions"
    echo ""
    echo "⏱️  Espera ~5 minutos y descarga el APK de 'Artifacts'"
    echo ""
    echo "📱 Para crear un Release:"
    echo "   1. Ve a Actions"
    echo "   2. Click en 'Create Release with APK'"
    echo "   3. Run workflow → versión v1.0.0"
    echo "   4. APK estará en Releases"
    echo ""
else
    echo ""
    echo "════════════════════════════════════════"
    echo "  ❌ ERROR AL HACER PUSH"
    echo "════════════════════════════════════════"
    echo ""
    echo "Posibles soluciones:"
    echo ""
    echo "1. Verificar que el repositorio existe:"
    echo "   https://github.com/Mario1988123/RELOJTOQUES"
    echo ""
    echo "2. Verificar autenticación de Git:"
    echo "   git config --global user.name"
    echo "   git config --global user.email"
    echo ""
    echo "3. Intentar push manual:"
    echo "   git push -u origin main"
    echo ""
    exit 1
fi
