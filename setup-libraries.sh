#!/bin/bash
# Script per scaricare automaticamente le librerie necessarie per ESP Bridge

echo "🚀 ESP Bridge - Setup Librerie Automatico"
echo "========================================"

# Verifica che wget e unzip siano installati
if ! command -v wget &> /dev/null; then
    echo "❌ wget non trovato. Installalo con: sudo apt install wget"
    exit 1
fi

if ! command -v unzip &> /dev/null; then
    echo "❌ unzip non trovato. Installalo con: sudo apt install unzip"
    exit 1
fi

# Crea cartella progetto se non esiste
PROJECT_DIR="esp-bridge"
if [ ! -d "$PROJECT_DIR" ]; then
    mkdir "$PROJECT_DIR"
    echo "📁 Creata cartella: $PROJECT_DIR"
fi

cd "$PROJECT_DIR"

# Copia il file .ino se non esiste
if [ ! -f "esp-bridge.ino" ]; then
    if [ -f "../esp-bridge.ino" ]; then
        cp "../esp-bridge.ino" .
        echo "📄 Copiato esp-bridge.ino"
    else
        echo "⚠️  File esp-bridge.ino non trovato"
    fi
fi

echo ""
echo "📥 Download fauxmoESP..."
wget -q --show-progress https://github.com/vintlabs/fauxmoESP/archive/refs/heads/master.zip -O fauxmo.zip
if [ $? -eq 0 ]; then
    unzip -q fauxmo.zip
    cp fauxmoESP-master/src/* . 2>/dev/null || cp fauxmoESP-master/* . 2>/dev/null
    rm -rf fauxmoESP-master fauxmo.zip
    echo "✅ fauxmoESP scaricato"
else
    echo "❌ Errore download fauxmoESP"
fi

echo ""
echo "📥 Download AsyncTCP..."
wget -q --show-progress https://github.com/me-no-dev/AsyncTCP/archive/refs/heads/master.zip -O async.zip
if [ $? -eq 0 ]; then
    unzip -q async.zip
    mkdir -p libraries
    mv AsyncTCP-master libraries/AsyncTCP
    rm async.zip
    echo "✅ AsyncTCP scaricato"
else
    echo "❌ Errore download AsyncTCP"
fi

echo ""
echo "🎯 Struttura finale del progetto:"
echo "================================"
ls -la
echo ""
echo "📁 File .h trovati:"
find . -name "*.h" | head -5
echo ""
echo "🔧 Prossimi passi:"
echo "1. Apri esp-bridge.ino in Arduino IDE"
echo "2. Seleziona board: ESP32 Dev Module"
echo "3. Modifica WiFi e IP dell'ESP originale"
echo "4. Compila e carica!"
echo ""
echo "✅ Setup completato!"