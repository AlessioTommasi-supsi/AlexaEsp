/*
 * ESP32 Bridge per Alexa - Architettura MVC Completa
 * 
 * Refactor completato:
 * - Rimossi tutti i file obsoleti (device_manager, wifi_manager, alexa_manager, config.h)
 * - Implementata architettura MVC pura con logica interna nei controller
 * - Pattern: MVC, Singleton, Dependency Injection, Observer, Strategy
 * 
 * Struttura finale:
 * - Model: SystemConfig (Singleton per configurazioni)
 * - View: SerialController (tutte le stampe centralizzate)
 * - Controller: DeviceController, WiFiController, AlexaController (logica completa interna)
 */

#include <WiFi.h>
#include <fauxmoESP.h>
#include <Preferences.h>

// Include dell'architettura MVC completa
#include "src/model/SystemConfig.h"
#include "src/view/SerialController.h"
#include "src/controller/DeviceController.h"
#include "src/controller/WiFiController.h"
#include "src/controller/AlexaController.h"

// ===== SYSTEM COMPONENTS =====
Preferences preferences;
fauxmoESP fauxmo;

// ===== MVC COMPONENTS =====
SerialController* serialController;
DeviceController* deviceController;
WiFiController* wifiController;
AlexaController* alexaController;

// ===== SERIAL INPUT STATE MACHINE =====
enum SerialState {
    IDLE,
    WAITING_WIFI_SELECTION,
    WAITING_WIFI_PASSWORD,
    WAITING_DEVICE_NAME,
    WAITING_DEVICE_TYPE,
    WAITING_DEVICE_PIN,
    WAITING_DEVICE_URL,
    WAITING_DEVICE_REMOVE,
    WAITING_RESET_CONFIRM
};

SerialState currentState = IDLE;
String inputBuffer = "";
String pendingDeviceName = "";

// ===== FORWARD DECLARATIONS =====
void handleSerialInput();
void processInput(const String& input);
void showMainMenu();
void handleMainCommand(const String& input);
void handleDeviceAdd();
void handleDeviceRemove();
void handleReset();

// ===== SYSTEM SETUP =====
void setup() {
    SystemConfig* config = SystemConfig::getInstance();
    
    Serial.begin(config->SERIAL_BAUD_RATE);
    delay(config->SETUP_DELAY);
    
    // Initialize preferences
    preferences.begin(config->PREFERENCES_NAMESPACE, false);
    
    // Create MVC components with dependency injection
    serialController = new SerialController();
    deviceController = new DeviceController(&preferences, serialController);
    wifiController = new WiFiController(&preferences, serialController);
    alexaController = new AlexaController(&fauxmo, deviceController, serialController);
    
    // Initialize system
    serialController->printWelcome();
    
    deviceController->initialize();
    wifiController->initialize();
    
    // Auto-connect and start services
    if (wifiController->connectToSaved()) {
        if (deviceController->getDeviceCount() > 0) {
            alexaController->initialize();
        }
        showMainMenu();
    } else {
        wifiController->startConfiguration();
        currentState = WAITING_WIFI_SELECTION;
    }
}

// ===== MAIN LOOP =====
void loop() {
    // Handle all subsystems
    alexaController->handle();
    wifiController->checkConnection();
    handleSerialInput();
    
    // Auto-reconnect logic
    if (!wifiController->isWiFiConnected()) {
        if (wifiController->autoReconnect() && deviceController->getDeviceCount() > 0) {
            alexaController->restart();
        }
    }
    
    delay(SystemConfig::getInstance()->LOOP_DELAY);
}

// ===== MAIN MENU =====
void showMainMenu() {
    currentState = IDLE;
    bool wifiConnected = wifiController->isWiFiConnected();
    int deviceCount = deviceController->getDeviceCount();
    bool alexaActive = alexaController->isAlexaInitialized();
    
    serialController->printMainMenu(wifiConnected, deviceCount, alexaActive);
    serialController->promptMenuOption();
}

// ===== SERIAL INPUT HANDLING =====
void handleSerialInput() {
    if (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                processInput(inputBuffer);
                inputBuffer = "";
            }
        } else if (c >= 32 && c <= 126) {
            inputBuffer += c;
            if (inputBuffer.length() > SystemConfig::getInstance()->MAX_INPUT_LENGTH) {
                inputBuffer = "";
                serialController->println("❌ Input troppo lungo");
                if (currentState != IDLE) {
                    Serial.print("👉 ");
                }
            }
        }
    }
}

void processInput(const String& input) {
    String trimmedInput = input;
    trimmedInput.trim();
    
    switch (currentState) {
        case IDLE:
            handleMainCommand(trimmedInput);
            break;
        case WAITING_WIFI_SELECTION:
            // Fix: Gestione corretta degli stati WiFi
            if (wifiController->handleNetworkSelection(trimmedInput)) {
                // Configurazione completata (connesso o annullato)
                if (wifiController->isWiFiConnected() && deviceController->getDeviceCount() > 0) {
                    alexaController->initialize();
                }
                showMainMenu();
            } else {
                // Se non è completato, verifica se è in attesa di password
                if (wifiController->isConfiguring()) {
                    currentState = WAITING_WIFI_PASSWORD;
                } else {
                    // Errore nella selezione, continua a chiedere
                    // Lo stato rimane WAITING_WIFI_SELECTION
                }
            }
            break;
        case WAITING_WIFI_PASSWORD:
            if (wifiController->handlePasswordInput(trimmedInput)) {
                if (wifiController->isWiFiConnected() && deviceController->getDeviceCount() > 0) {
                    alexaController->initialize();
                }
                showMainMenu();
            }
            // Se fallisce, rimane in WAITING_WIFI_PASSWORD per riprovare
            break;
        case WAITING_DEVICE_NAME:
            handleDeviceNameInput(trimmedInput);
            break;
        case WAITING_DEVICE_TYPE:
            handleDeviceTypeInput(trimmedInput);
            break;
        case WAITING_DEVICE_PIN:
            handleDevicePinInput(trimmedInput);
            break;
        case WAITING_DEVICE_URL:
            handleDeviceURLInput(trimmedInput);
            break;
        case WAITING_DEVICE_REMOVE:
            handleDeviceRemoveInput(trimmedInput);
            break;
        case WAITING_RESET_CONFIRM:
            handleResetConfirmInput(trimmedInput);
            break;
    }
}

void handleMainCommand(const String& input) {
    int choice = input.toInt();
    
    switch (choice) {
        case 0: // Modalità silenziosa
            serialController->println("ℹ️ Modalità silenziosa. Digita qualsiasi numero per menu.");
            return;
        case 1: // Mostra dispositivi
            deviceController->printDevices();
            break;
        case 2: // Aggiungi dispositivo
            handleDeviceAdd();
            return;
        case 3: // Rimuovi dispositivo
            handleDeviceRemove();
            return;
        case 4: // Configura WiFi
            wifiController->startConfiguration();
            currentState = WAITING_WIFI_SELECTION;
            return;
        case 5: // Status sistema
            showSystemStatus();
            break;
        case 6: // Reset configurazione
            handleReset();
            return;
        case 7: // Riavvia Alexa
            if (wifiController->isWiFiConnected() && deviceController->getDeviceCount() > 0) {
                alexaController->restart();
            } else {
                serialController->println("❌ WiFi non connesso o nessun dispositivo");
            }
            break;
        default:
            serialController->println("❌ Opzione non valida! Scegli 0-7");
            break;
    }
    
    delay(SystemConfig::getInstance()->MENU_RETURN_DELAY);
    showMainMenu();
}

// ===== DEVICE MANAGEMENT =====
void handleDeviceAdd() {
    if (deviceController->isFull()) {
        serialController->printf("❌ Limite massimo dispositivi raggiunto (%d)\n", SystemConfig::getInstance()->MAX_DEVICES);
        delay(SystemConfig::getInstance()->MENU_RETURN_DELAY);
        showMainMenu();
        return;
    }
    
    serialController->promptDeviceName();
    pendingDeviceName = "";
    currentState = WAITING_DEVICE_NAME;
}

void handleDeviceNameInput(const String& input) {
    if (input == "0") {
        serialController->println("ℹ️ Aggiunta annullata");
        showMainMenu();
        return;
    }
    
    if (input.length() == 0) {
        serialController->println("❌ Nome vuoto! Riprova:");
        serialController->promptDeviceName();
        return;
    }
    
    if (deviceController->deviceExists(input)) {
        serialController->println("❌ Dispositivo esistente! Riprova:");
        serialController->promptDeviceName();
        return;
    }
    
    pendingDeviceName = input;
    serialController->printf("✅ Nome: %s\n", input.c_str());
    serialController->promptDeviceType();
    currentState = WAITING_DEVICE_TYPE;
}

void handleDeviceTypeInput(const String& input) {
    int choice = input.toInt();
    
    switch (choice) {
        case 0:
            serialController->println("ℹ️ Aggiunta annullata");
            showMainMenu();
            break;
        case 1:
            serialController->promptDevicePin(SystemConfig::getInstance()->ESP32_MIN_PIN, SystemConfig::getInstance()->ESP32_MAX_PIN);
            currentState = WAITING_DEVICE_PIN;
            break;
        case 2:
            serialController->promptDeviceURL();
            currentState = WAITING_DEVICE_URL;
            break;
        default:
            serialController->println("❌ Scelta non valida! Riprova (1/2/0):");
            serialController->promptDeviceType();
            break;
    }
}

void handleDevicePinInput(const String& input) {
    if (input == "0") {
        serialController->println("ℹ️ Aggiunta annullata");
        showMainMenu();
        return;
    }
    
    int pin = input.toInt();
    SystemConfig* config = SystemConfig::getInstance();
    
    if (pin < config->ESP32_MIN_PIN || pin > config->ESP32_MAX_PIN) {
        serialController->printf("❌ Pin non valido (%d-%d)! Riprova:\n", config->ESP32_MIN_PIN, config->ESP32_MAX_PIN);
        serialController->promptDevicePin(config->ESP32_MIN_PIN, config->ESP32_MAX_PIN);
        return;
    }
    
    if (deviceController->addDevice(pendingDeviceName, pin)) {
        serialController->printf("✅ '%s' configurato per pin %d\n", pendingDeviceName.c_str(), pin);
        if (wifiController->isWiFiConnected()) {
            alexaController->restart();
        }
    }
    
    delay(config->MENU_RETURN_DELAY);
    showMainMenu();
}

void handleDeviceURLInput(const String& input) {
    if (input == "0") {
        serialController->println("ℹ️ Aggiunta annullata");
        showMainMenu();
        return;
    }
    
    if (input.length() == 0 || !input.startsWith("http")) {
        serialController->println("❌ URL non valido (deve iniziare con http)! Riprova:");
        serialController->promptDeviceURL();
        return;
    }
    
    if (deviceController->addDevice(pendingDeviceName, input)) {
        serialController->printf("✅ '%s' configurato per URL: %s\n", pendingDeviceName.c_str(), input.c_str());
        if (wifiController->isWiFiConnected()) {
            alexaController->restart();
        }
    }
    
    delay(SystemConfig::getInstance()->MENU_RETURN_DELAY);
    showMainMenu();
}

void handleDeviceRemove() {
    if (deviceController->getDeviceCount() == 0) {
        serialController->println("❌ Nessun dispositivo da rimuovere");
        delay(SystemConfig::getInstance()->MENU_RETURN_DELAY);
        showMainMenu();
        return;
    }
    
    deviceController->printDevices();
    serialController->promptRemoveDevice();
    currentState = WAITING_DEVICE_REMOVE;
}

void handleDeviceRemoveInput(const String& input) {
    if (input == "0") {
        serialController->println("ℹ️ Rimozione annullata");
        showMainMenu();
        return;
    }
    
    if (deviceController->removeDevice(input)) {
        if (wifiController->isWiFiConnected()) {
            alexaController->restart();
        }
    }
    
    delay(SystemConfig::getInstance()->MENU_RETURN_DELAY);
    showMainMenu();
}

// ===== SYSTEM STATUS =====
void showSystemStatus() {
    String ssid = wifiController->getSSID();
    String ip = wifiController->getIP();
    int rssi = wifiController->getRSSI();
    String mac = wifiController->getMAC();
    int deviceCount = deviceController->getDeviceCount();
    bool alexaActive = alexaController->isAlexaInitialized();
    
    serialController->printSystemStatus(ssid, ip, rssi, mac, deviceCount, alexaActive, millis());
}

// ===== RESET SYSTEM =====
void handleReset() {
    serialController->promptResetConfirm();
    currentState = WAITING_RESET_CONFIRM;
}

void handleResetConfirmInput(const String& input) {
    String choice = input;
    choice.toLowerCase();
    
    if (choice == "y" || choice == "yes") {
        serialController->println("🔄 Reset in corso...");
        
        alexaController->shutdown();
        wifiController->disconnect();
        preferences.clear();
        deviceController->clear();
        
        serialController->println("✅ Reset completato!");
        serialController->println("🔄 Riavvia ESP per riconfigurare");
        serialController->println("👋 Sistema in modalità silenziosa.");
        return;
    } else {
        serialController->println("ℹ️ Reset annullato");
        delay(SystemConfig::getInstance()->MENU_RETURN_DELAY);
        showMainMenu();
    }
}