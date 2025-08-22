# 🏠 Alexa-ESP Bridge - Controllo Vocale Locale

Soluzione **100% locale** per controllare ESP con Alexa senza PC o cloud.

## 🎯 Architettura
```
Alexa (rete locale) → ESP Bridge (FauxmoESP) → ESP Originale (API)
```

## 📋 Hardware Necessario
- **ESP32 Bridge** (nuovo, dedicato ad Alexa)
- **ESP Originale** (quello che hai già con le API)
- **Alexa** (nella stessa rete WiFi)

## 🚀 Setup Veloce

### 1. **ESP Bridge** - Programma nuovo ESP32
- Usa il file `esp-bridge.ino`
- Modifica IP del tuo ESP originale
- Carica il codice

### 2. **Alexa Discovery**
```
"Alexa, scopri dispositivi"
```

### 3. **Comandi Vocali Diretti**
- **"Alexa, accendi luce studio"** → Chiama `192.168.178.164/pulsePin?pin=33`
- **"Alexa, accendi porta studio"** → Chiama `192.168.178.164/pulsePin?pin=32`
- **"Alexa, accendi luce presepe"** → Chiama `192.168.178.164/pulsePin?pin=26`

## ✅ Vantaggi
- **Zero configurazione cloud** - Tutto locale
- **ESP originale intatto** - Continua a funzionare come ora
- **Nessun PC necessario** - Solo ESP + Alexa
- **Comandi naturali** - Nessuna skill personalizzata

## 🔧 Dispositivi Supportati

| Comando Alexa | API ESP Originale | Pin |
|---------------|------------------|-----|
| "accendi porta studio" | `/pulsePin?pin=32` | 32 |
| "accendi luce studio" | `/pulsePin?pin=33` | 33 |
| "accendi luce studio esterno" | `/pulsePin?pin=25` | 25 |
| "accendi luce presepe" | `/pulsePin?pin=26` | 26 |
| "accendi luce cantina" | `/pulsePin?pin=27` | 27 |
| "accendi luce camino" | `/pulsePin?pin=14` | 14 |

## ⚡ Funzionamento
1. Alexa riconosce il comando vocale
2. ESP Bridge riceve il comando via FauxmoESP 
3. ESP Bridge chiama API dell'ESP originale
4. ESP originale attiva il pin fisico
5. Alexa risponde "OK"

---
*Soluzione by GitHub Copilot - Controllo vocale 100% locale*
