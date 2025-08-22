/*
 * ESP32 Bridge per Alexa - Controllo Vocale Locale
 * 
 * Architettura Modulare:
 * - config.h: Configurazione centralizzata
 * - device_manager.h/cpp: Gestione dispositivi
 * - wifi_manager.h/cpp: Gestione WiFi con fix password
 * - alexa_manager.h/cpp: Gestione Alexa
 * - esp-bridge.ino: Orchestratore principale
 */

#include <WiFi.h>
#include <fauxmoESP.h>
#include <Preferences.h>
#include "config.h"
#include "device_manager.h"
#include "wifi_manager.h"
#include "alexa_manager.h"

// ===== DEFINIZIONE VARIABILI GLOBALI EXTERN =====
const char* ESP_ORIGINALE_IP = "192.168.178.164";
const char* DEVICE_NAME = "ESP32-Bridge";
const char* PREFERENCES_NAMESPACE = "esp-bridge";

// ===== STATI SISTEMA =====
enum SerialState {
    IDLE,
    WAITING_WIFI_SELECTION,
    WAITING_WIFI_PASSWORD,
    WAITING_DEVICE_TYPE,
    WAITING_DEVICE_PIN,
    WAITING_DEVICE_URL,
    WAITING_RESET_CONFIRM,
    WAITING_DEVICE_REMOVE
};

// ===== ISTANZE CLASSI =====
Preferences preferences;
fauxmoESP fauxmo;
DeviceManager deviceManager(&preferences);
WiFiManager wifiManager(&preferences);
AlexaManager alexaManager(&fauxmo, &deviceManager);

// ===== GESTIONE INPUT SERIALE =====
SerialState currentState = IDLE;
String inputBuffer = "";
String pendingDeviceName = "";

// ===== SETUP =====
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(SETUP_DELAY);
    
    printWelcome();
    initializeSystem();
    
    if (wifiManager.connectToSaved()) {
        if (deviceManager.getDeviceCount() > 0) {
            alexaManager.initialize();
        }
        showMainMenu();
    } else {
        wifiManager.startConfiguration();
        currentState = WAITING_WIFI_SELECTION;
    }
}

// ===== LOOP PRINCIPALE =====
void loop() {
    // Gestione Alexa
    alexaManager.handle();
    
    // Controllo connessione WiFi
    wifiManager.checkConnection();
    if (!wifiManager.isWiFiConnected()) {
        if (wifiManager.autoReconnect() && deviceManager.getDeviceCount() > 0) {
            alexaManager.restart();
        }
    }
    
    // Gestione input seriale
    handleSerialInput();
    
    delay(LOOP_DELAY);
}

// ===== INIZIALIZZAZIONE =====
void printWelcome() {
    Serial.println("\n🚀 ESP32 Bridge per Alexa - Refactored");
    Serial.println("========================================");
    Serial.printf("🔧 Max dispositivi: %d\n", MAX_DEVICES);
    Serial.printf("📡 Target ESP: %s\n", ESP_ORIGINALE_IP);
    Serial.printf("🏗️  Architettura: Modulare OOP\n");
}

void initializeSystem() {
    preferences.begin(PREFERENCES_NAMESPACE, false);
    deviceManager.loadDevices();
}

// ===== GESTIONE INPUT SERIALE =====
void handleSerialInput() {
    // ✅ NUOVO: Gestione input seriale senza timeout - await pulito
    if (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                processInput(inputBuffer);
                inputBuffer = "";
            }
        } else if (c >= 32 && c <= 126) {
            inputBuffer += c;
            if (inputBuffer.length() > MAX_INPUT_LENGTH) {
                inputBuffer = "";
                Serial.println("❌ Input troppo lungo");
                // Rimani nello stesso stato, non tornare al menu
                if (currentState != IDLE) {
                    Serial.print("👉 ");
                }
            }
        }
    }
}

void processInput(String input) {
    input.trim();
    
    switch (currentState) {
        case IDLE:
            handleMainCommand(input);
            break;
        case WAITING_WIFI_SELECTION:
            if (wifiManager.handleNetworkSelection(input)) {
                // Configurazione completata o annullata
                resetSerialState();
                showMainMenu();
            } else {
                // Aspetta password
                currentState = WAITING_WIFI_PASSWORD;
            }
            break;
        case WAITING_WIFI_PASSWORD:
            if (wifiManager.handlePasswordInput(input)) {
                // Configurazione completata
                if (wifiManager.isWiFiConnected() && deviceManager.getDeviceCount() > 0) {
                    alexaManager.initialize();
                }
                resetSerialState();
                showMainMenu();
            }
            // Altrimenti continua ad aspettare password
            break;
        case WAITING_DEVICE_TYPE:
            handleDeviceType(input);
            break;
        case WAITING_DEVICE_PIN:
            handleDevicePin(input);
            break;
        case WAITING_DEVICE_URL:
            handleDeviceURL(input);
            break;
        case WAITING_RESET_CONFIRM:
            handleResetConfirm(input);
            break;
        case WAITING_DEVICE_REMOVE:
            handleDeviceRemove(input);
            break;
    }
}

void resetSerialState() {
    currentState = IDLE;
    pendingDeviceName = "";
}

void startSerialInput(SerialState newState) {
    currentState = newState;
}

// ===== MENU PRINCIPALE =====
void showMainMenu() {
    resetSerialState();
    
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║      🏠 ESP32 BRIDGE ALEXA v2.0       ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ WiFi: %-31s ║\n", wifiManager.isWiFiConnected() ? "✅ Connesso" : "❌ Disconnesso");
    Serial.printf("║ Dispositivi: %2d/%-3d                  ║\n", deviceManager.getDeviceCount(), MAX_DEVICES);
    Serial.printf("║ Alexa: %-30s ║\n", alexaManager.isAlexaInitialized() ? "✅ Attivo" : "❌ Non attivo");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.println("║  1. 📱 Mostra dispositivi             ║");
    Serial.println("║  2. ➕ Aggiungi dispositivo           ║");
    Serial.println("║  3. 🗑️  Rimuovi dispositivo            ║");
    Serial.println("║  4. 📡 Configura WiFi                 ║");
    Serial.println("║  5. 📊 Status sistema                 ║");
    Serial.println("║  6. 🔄 Reset configurazione           ║");
    Serial.println("║  7. 🎤 Riavvia Alexa                  ║");
    Serial.println("║  0. 👋 Modalità silenziosa            ║");
    Serial.println("╚═══════════════════════════════════════╝");
    Serial.print("👉 Scegli opzione (0-7): ");
}

void handleMainCommand(const String& input) {
    int choice = input.toInt();
    
    if (choice < MENU_MIN_OPTION || choice > MENU_MAX_OPTION - 1) {
        Serial.println("❌ Opzione non valida! Scegli 0-7");
        delay(MENU_RETURN_DELAY);
        showMainMenu();
        return;
    }
    
    switch (choice) {
        case 1:
            deviceManager.printDevices();
            break;
        case 2:
            startAddDevice();
            return;
        case 3:
            startRemoveDevice();
            return;
        case 4:
            wifiManager.startConfiguration();
            currentState = WAITING_WIFI_SELECTION;
            return;
        case 5:
            showSystemStatus();
            break;
        case 6:
            startResetConfiguration();
            return;
        case 7:
            restartAlexa();
            break;
        case 0:
            Serial.println("👋 Modalità silenziosa. Digita qualsiasi numero per menu.");
            return;
    }
    
    delay(MENU_RETURN_DELAY);
    showMainMenu();
}

// ===== GESTIONE DISPOSITIVI =====
void startAddDevice() {
    if (deviceManager.isFull()) {
        Serial.printf("❌ Limite massimo dispositivi raggiunto (%d)\n", MAX_DEVICES);
        delay(MENU_RETURN_DELAY);
        showMainMenu();
        return;
    }
    
    Serial.println("\n➕ Inserisci nome del nuovo dispositivo:");
    Serial.print("👉 ");
    pendingDeviceName = "";
    startSerialInput(WAITING_DEVICE_TYPE);
}

void startRemoveDevice() {
    if (deviceManager.getDeviceCount() == 0) {
        Serial.println("❌ Nessun dispositivo da rimuovere");
        delay(MENU_RETURN_DELAY);
        showMainMenu();
        return;
    }
    
    Serial.println("\n🗑️ Dispositivi disponibili:");
    deviceManager.printDevices();
    Serial.print("👉 Nome dispositivo da rimuovere (0=annulla): ");
    
    // ✅ NUOVO: Usa stato seriale invece di waitForSerialInput()
    startSerialInput(WAITING_DEVICE_REMOVE);
}

void handleDeviceRemove(const String& input) {
    String deviceName = input;
    deviceName.trim();
    
    if (deviceName == "0") {
        Serial.println("❌ Rimozione annullata");
        resetSerialState();
        showMainMenu();
        return;
    }
    
    if (deviceName.length() > 0) {
        if (deviceManager.removeDevice(deviceName)) {
            Serial.println("✅ Dispositivo '" + deviceName + "' rimosso");
            if (wifiManager.isWiFiConnected()) {
                alexaManager.restart();
            }
        } else {
            Serial.println("❌ Dispositivo non trovato");
        }
    }
    
    delay(MENU_RETURN_DELAY);
    showMainMenu();
}

// ===== STATUS SISTEMA =====
void showSystemStatus() {
    Serial.println("\n📊 Status Sistema Modulare:");
    Serial.println("==============================");
    
    // WiFi Status
    Serial.printf("🌐 WiFi: %s\n", wifiManager.isWiFiConnected() ? "✅ Connesso" : "❌ Disconnesso");
    if (wifiManager.isWiFiConnected()) {
        Serial.printf("   SSID: %s\n", wifiManager.getSSID().c_str());
        Serial.printf("   IP: %s\n", wifiManager.getIP().c_str());
        Serial.printf("   RSSI: %d dBm\n", wifiManager.getRSSI());
        Serial.printf("   MAC: %s\n", wifiManager.getMAC().c_str());
    }
    
    // Alexa Status  
    Serial.printf("🎤 Alexa: %s\n", alexaManager.isAlexaInitialized() ? "✅ Attivo" : "❌ Non attivo");
    
    // Device Status
    Serial.printf("📱 Dispositivi: %d/%d\n", deviceManager.getDeviceCount(), MAX_DEVICES);
    
    // System Status
    Serial.printf("💾 Memoria libera: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("⏱️  Uptime: %lu secondi\n", millis() / 1000);
    Serial.printf("📡 Target ESP: %s\n", ESP_ORIGINALE_IP);
    Serial.printf("🏗️  Architettura: Modulare OOP\n");
}

// ===== GESTIONE AGGIUNTA DISPOSITIVI =====
void handleDeviceType(const String& input) {
    // Se pendingDeviceName è vuoto, questo è il nome del dispositivo
    if (pendingDeviceName.length() == 0) {
        String name = input;
        name.trim();
        
        if (name.length() == 0) {
            Serial.println("❌ Nome vuoto! Riprova:");
            Serial.print("👉 ");
            return;
        }
        
        if (name == "0") {
            Serial.println("❌ Aggiunta annullata");
            resetSerialState();
            showMainMenu();
            return;
        }
        
        if (deviceManager.deviceExists(name)) {
            Serial.println("❌ Dispositivo esistente! Riprova:");
            Serial.print("👉 ");
            return;
        }
        
        pendingDeviceName = name;
        Serial.println("✅ Nome: " + name);
        Serial.println("🔧 Tipo controllo:");
        Serial.println("   1. Standard (Pin ESP)");
        Serial.println("   2. Custom (URL)");
        Serial.println("   0. Annulla");
        Serial.print("👉 Scegli (1/2/0): ");
        return;
    }
    
    // Gestione tipo dispositivo
    int choice = input.toInt();
    
    switch (choice) {
        case 0:
            Serial.println("❌ Aggiunta annullata");
            resetSerialState();
            showMainMenu();
            break;
        case 1:
            Serial.printf("📍 Pin ESP32 (%d-%d) o 0 per annullare: ", ESP32_MIN_PIN, ESP32_MAX_PIN);
            startSerialInput(WAITING_DEVICE_PIN);
            break;
        case 2:
            Serial.print("🌐 URL completo o 0 per annullare: ");
            startSerialInput(WAITING_DEVICE_URL);
            break;
        default:
            Serial.println("❌ Scelta non valida! Riprova (1/2/0):");
            Serial.print("👉 ");
            break;
    }
}

void handleDevicePin(const String& input) {
    if (input == "0") {
        Serial.println("❌ Aggiunta annullata");
        resetSerialState();
        showMainMenu();
        return;
    }
    
    int pin = input.toInt();
    if (pin < ESP32_MIN_PIN || pin > ESP32_MAX_PIN) {
        Serial.printf("❌ Pin non valido (%d-%d)! Riprova:\n", ESP32_MIN_PIN, ESP32_MAX_PIN);
        Serial.print("👉 ");
        return;
    }
    
    if (deviceManager.addDevice(pendingDeviceName, pin)) {
        Serial.printf("✅ '%s' configurato per pin %d\n", pendingDeviceName.c_str(), pin);
        
        if (wifiManager.isWiFiConnected()) {
            alexaManager.restart();
        }
    } else {
        Serial.println("❌ Errore aggiunta dispositivo");
    }
    
    resetSerialState();
    delay(MENU_RETURN_DELAY);
    showMainMenu();
}

void handleDeviceURL(const String& input) {
    if (input == "0") {
        Serial.println("❌ Aggiunta annullata");
        resetSerialState();
        showMainMenu();
        return;
    }
    
    String url = input;
    url.trim();
    
    if (url.length() == 0 || !url.startsWith("http")) {
        Serial.println("❌ URL non valido (deve iniziare con http)! Riprova:");
        Serial.print("👉 ");
        return;
    }
    
    if (deviceManager.addDevice(pendingDeviceName, url)) {
        Serial.printf("✅ '%s' configurato per URL: %s\n", pendingDeviceName.c_str(), url.c_str());
        
        if (wifiManager.isWiFiConnected()) {
            alexaManager.restart();
        }
    } else {
        Serial.println("❌ Errore aggiunta dispositivo");
    }
    
    resetSerialState();
    delay(MENU_RETURN_DELAY);
    showMainMenu();
}

// ===== RESET CONFIGURAZIONE =====
void startResetConfiguration() {
    Serial.println("⚠️  RESET COMPLETO CONFIGURAZIONE!");
    Serial.println("Verranno cancellati WiFi e tutti i dispositivi.");
    Serial.println("Confermi? (y/N/0=annulla):");
    Serial.print("👉 ");
    startSerialInput(WAITING_RESET_CONFIRM);
}

void handleResetConfirm(const String& input) {
    String choice = input;
    choice.toLowerCase();
    
    if (choice == "y" || choice == "yes") {
        Serial.println("🔄 Reset in corso...");
        
        // Reset completo modularizzato
        alexaManager.shutdown();
        wifiManager.disconnect();
        preferences.clear();
        deviceManager.clear();
        
        Serial.println("✅ Reset completato!");
        Serial.println("🔄 Riavvia ESP per riconfigurare");
        
        resetSerialState();
        Serial.println("👋 Sistema in modalità silenziosa.");
        return;
    } else {
        Serial.println("❌ Reset annullato");
        resetSerialState();
        delay(MENU_RETURN_DELAY);
        showMainMenu();
    }
}

void restartAlexa() {
    if (wifiManager.isWiFiConnected() && deviceManager.getDeviceCount() > 0) {
        alexaManager.restart();
        Serial.println("✅ Alexa riavviato");
    } else {
        Serial.println("❌ WiFi non connesso o nessun dispositivo");
    }
}