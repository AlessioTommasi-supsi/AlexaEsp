# 📥 Download Librerie per ESP Bridge

## 🎯 Librerie necessarie
Per compilare `esp-bridge.ino` hai bisogno di:

1. **fauxmoESP** - Libreria principale
2. **AsyncTCP** - Dipendenza per ESP32
3. **ESPAsyncWebServer** - Server web asincrono (opzionale ma consigliata)

## 📋 Download manuale

### 1. FauxmoESP
```bash
# Scarica da GitHub
wget https://github.com/vintlabs/fauxmoESP/archive/refs/heads/master.zip
unzip master.zip
# Copia i file nella cartella del progetto:
# - fauxmoESP.h
# - fauxmoESP.cpp
```

### 2. AsyncTCP (per ESP32)
```bash
# Scarica AsyncTCP
wget https://github.com/me-no-dev/AsyncTCP/archive/refs/heads/master.zip
unzip master.zip
# Copia tutta la cartella AsyncTCP
```

### 3. ESPAsyncWebServer (opzionale)
```bash
# Se necessario
wget https://github.com/me-no-dev/ESPAsyncWebServer/archive/refs/heads/master.zip
```

## 🛠️ Installazione manuale

### Opzione A: Cartella progetto
```
esp-bridge/
├── esp-bridge.ino
├── fauxmoESP.h           # ← Copiato manualmente
├── fauxmoESP.cpp         # ← Copiato manualmente
└── AsyncTCP/             # ← Cartella completa
    ├── src/
    └── library.properties
```

### Opzione B: Cartella libraries Arduino
```
~/Arduino/libraries/
├── fauxmoESP/
│   ├── fauxmoESP.h
│   ├── fauxmoESP.cpp
│   └── library.properties
└── AsyncTCP/
    ├── src/
    └── library.properties
```

## ⚡ Setup automatico (script)

Creo uno script per scaricare tutto automaticamente:

```bash
#!/bin/bash
# setup-libraries.sh

echo "📥 Download librerie per ESP Bridge..."

# Crea cartelle
mkdir -p esp-bridge/libraries

cd esp-bridge

# Scarica fauxmoESP
echo "Scaricando fauxmoESP..."
wget -q https://github.com/vintlabs/fauxmoESP/archive/refs/heads/master.zip -O fauxmo.zip
unzip -q fauxmo.zip
mv fauxmoESP-master/* .
rm -rf fauxmoESP-master fauxmo.zip

# Scarica AsyncTCP
echo "Scaricando AsyncTCP..."
wget -q https://github.com/me-no-dev/AsyncTCP/archive/refs/heads/master.zip -O async.zip
unzip -q async.zip
mv AsyncTCP-master libraries/AsyncTCP
rm async.zip

echo "✅ Librerie scaricate!"
echo "📁 Struttura:"
find . -name "*.h" -o -name "*.cpp" | head -10
```

## 🔧 Compilazione Arduino IDE

1. **Apri esp-bridge.ino** in Arduino IDE
2. **Seleziona board**: ESP32 Dev Module
3. **Compila** - Arduino IDE troverà automaticamente i file .h nella stessa cartella

## ⚠️ Note importanti

- **ESP32 only**: fauxmoESP funziona meglio con ESP32
- **WiFi necessario**: Assicurati che le credenziali WiFi siano corrette
- **Porta seriale**: Usa 115200 baud per il monitor seriale

Vuoi che ti crei lo script di download automatico?