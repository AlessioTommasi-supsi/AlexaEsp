#include "SerialController.h"
#include <stdarg.h>

SerialController::SerialController() {
    config = SystemConfig::getInstance();
}

void SerialController::printWelcome() {
    Serial.println("\n🚀 ESP32 Bridge per Alexa - Architettura MVC");
    Serial.println("=============================================");
    Serial.printf("🔧 Max dispositivi: %d\n", config->MAX_DEVICES);
    Serial.printf("📡 Target ESP: %s\n", config->ESP_ORIGINALE_IP);
    Serial.printf("🏗️ Architettura: Modulare MVC\n");
}

void SerialController::printMainMenu(bool wifiConnected, int deviceCount, bool alexaActive) {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║      🏠 ESP32 BRIDGE ALEXA v3.0       ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ WiFi: %-31s ║\n", wifiConnected ? "✅ Connesso" : "❌ Disconnesso");
    Serial.printf("║ Dispositivi: %2d/%-3d                  ║\n", deviceCount, config->MAX_DEVICES);
    Serial.printf("║ Alexa: %-30s ║\n", alexaActive ? "✅ Attivo" : "❌ Non attivo");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.println("║  1. 📱 Mostra dispositivi             ║");
    Serial.println("║  2. ➕ Aggiungi dispositivo           ║");
    Serial.println("║  3. 🗑️ Rimuovi dispositivo             ║");
    Serial.println("║  4. 📡 Configura WiFi                 ║");
    Serial.println("║  5. 📊 Status sistema                 ║");
    Serial.println("║  6. 🔄 Reset configurazione           ║");
    Serial.println("║  7. 🎤 Riavvia Alexa                  ║");
    Serial.println("║  0. 👋 Modalità silenziosa            ║");
    Serial.println("╚═══════════════════════════════════════╝");
}

void SerialController::printAlexaInitializing() {
    Serial.println("\n🎤 Inizializzazione Alexa...");
}

void SerialController::printAlexaReady(int deviceCount) {
    Serial.println("\n🎉 ESP Bridge pronto!");
    Serial.printf("📱 %d dispositivi configurati\n", deviceCount);
    if (deviceCount > 0) {
        Serial.println("\n💡 'Alexa, scopri dispositivi' se necessario");
    }
}

void SerialController::printAlexaCommand(const String& deviceName, bool state) {
    Serial.printf("🗣️ Alexa: '%s' -> %s\n", deviceName.c_str(), state ? "ON" : "OFF");
}

void SerialController::printAlexaResponse(int pin, bool success, int httpCode) {
    if (success) {
        Serial.printf("✅ Pin %d attivato\n", pin);
    } else {
        Serial.printf("❌ Errore ESP (codice: %d)\n", httpCode);
    }
}

void SerialController::printAlexaCustomResponse(const String& url, bool success, int httpCode, const String& response) {
    if (success) {
        Serial.println("✅ Richiesta custom eseguita");
        if (response.length() > 0 && response.length() < config->HTTP_RESPONSE_MAX_LENGTH) {
            Serial.println("📥 Risposta: " + response);
        }
    } else {
        Serial.printf("❌ Errore custom (codice: %d)\n", httpCode);
    }
}

void SerialController::printWiFiNetworks(int networkCount) {
    Serial.println("\n📋 Reti WiFi disponibili:");
    Serial.println("==========================");
}

void SerialController::printNetworkInfo(int index, const String& ssid, int rssi, bool encrypted) {
    Serial.printf("%2d. %-25s", index + 1, ssid.c_str());
    
    if (rssi > -50) Serial.print(" 📶📶📶📶");
    else if (rssi > -60) Serial.print(" 📶📶📶");
    else if (rssi > -70) Serial.print(" 📶📶");
    else Serial.print(" 📶");
    
    Serial.print(encrypted ? " 🔒" : " 🔓");
    Serial.printf(" (%d dBm)\n", rssi);
}

void SerialController::printDeviceList(int deviceCount) {
    Serial.println("\n📱 Dispositivi configurati:");
    Serial.println("============================");
    if (deviceCount == 0) {
        Serial.println("   Nessun dispositivo configurato");
    }
}

void SerialController::printDevice(int index, const String& name, bool isCustomUrl, int pin, const String& url) {
    Serial.printf("%2d. %-20s -> ", index + 1, name.c_str());
    if (isCustomUrl) {
        Serial.println("🌐 " + url);
    } else {
        Serial.printf("📍 Pin %d (pulsePin)\n", pin);
    }
}

void SerialController::promptWiFiSelection(int maxOption) {
    Serial.printf("\n🔢 Scegli rete (1-%d) o 0 per annullare:\n", maxOption);
    Serial.print("👉 ");
}

void SerialController::promptPassword() {
    Serial.print("🔑 Password: ");
}

void SerialController::promptMenuOption() {
    Serial.print("👉 Scegli opzione (0-7): ");
}

void SerialController::promptDeviceName() {
    Serial.print("📝 Nome dispositivo (0=annulla): ");
}

void SerialController::promptDeviceType() {
    Serial.println("🔧 Tipo controllo:");
    Serial.println("   1. Standard (Pin ESP)");
    Serial.println("   2. Custom (URL)");
    Serial.println("   0. Annulla");
    Serial.print("👉 Scegli (1/2/0): ");
}

void SerialController::promptDevicePin(int minPin, int maxPin) {
    Serial.printf("📍 Pin ESP32 (%d-%d) o 0 per annullare: ", minPin, maxPin);
}

void SerialController::promptDeviceURL() {
    Serial.print("🌐 URL completo o 0 per annullare: ");
}

void SerialController::promptRemoveDevice() {
    Serial.print("👉 Nome dispositivo da rimuovere (0=annulla): ");
}

void SerialController::promptResetConfirm() {
    Serial.println("⚠️  RESET COMPLETO CONFIGURAZIONE!");
    Serial.println("Verranno cancellati WiFi e tutti i dispositivi.");
    Serial.println("Confermi? (y/N/0=annulla):");
    Serial.print("👉 ");
}

void SerialController::printSystemStatus(const String& ssid, const String& ip, int rssi, const String& mac, 
                                       int deviceCount, bool alexaActive, unsigned long uptime) {
    Serial.println("\n📊 Status Sistema MVC:");
    Serial.println("==============================");
    
    // WiFi Status
    if (ssid.length() > 0) {
        Serial.printf("🌐 WiFi: ✅ Connesso\n");
        Serial.printf("   SSID: %s\n", ssid.c_str());
        Serial.printf("   IP: %s\n", ip.c_str());
        Serial.printf("   RSSI: %d dBm\n", rssi);
        Serial.printf("   MAC: %s\n", mac.c_str());
    } else {
        Serial.println("🌐 WiFi: ❌ Disconnesso");
    }
    
    // Alexa Status  
    Serial.printf("🎤 Alexa: %s\n", alexaActive ? "✅ Attivo" : "❌ Non attivo");
    
    // Device Status
    Serial.printf("📱 Dispositivi: %d/%d\n", deviceCount, config->MAX_DEVICES);
    
    // System Status
    Serial.printf("💾 Memoria libera: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("⏱️  Uptime: %lu secondi\n", uptime / 1000);
    Serial.printf("📡 Target ESP: %s\n", config->ESP_ORIGINALE_IP);
    Serial.printf("🏗️  Architettura: MVC Completa\n");
}

void SerialController::printAlexaShutdown() {
    Serial.println("🎤 Alexa spento");
}

void SerialController::printAlexaRestart() {
    Serial.println("🔄 Alexa riavviato");
}

void SerialController::printWiFiStatus(const String& message) {
    Serial.println(message);
}

void SerialController::printWiFiConnected(const String& ssid, const String& ip) {
    Serial.printf("✅ WiFi connesso: %s (%s)\n", ssid.c_str(), ip.c_str());
}

void SerialController::printDeviceAction(const String& action, const String& name) {
    Serial.printf("%s: %s\n", action.c_str(), name.c_str());
}

// Utility methods
void SerialController::print(const String& message) {
    Serial.print(message);
}

void SerialController::println(const String& message) {
    Serial.println(message);
}

void SerialController::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.print(buffer);
}