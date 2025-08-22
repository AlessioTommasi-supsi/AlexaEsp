#include "alexa_manager.h"

// 🔒 SAFETY: Variabili statiche per protezione callback
volatile bool AlexaManager::callbackSafe = false;
AlexaManager* AlexaManager::safeInstance = nullptr;

AlexaManager::AlexaManager(fauxmoESP* fauxmoInstance, DeviceManager* devManager) 
    : fauxmo(fauxmoInstance), deviceManager(devManager), isInitialized(false) {
    // 🛡️ SAFETY: Registrazione sicura dell'istanza
    safeInstance = this;
    callbackSafe = false;
}

AlexaManager::~AlexaManager() {
    // 🛡️ SAFETY: Pulizia sicura
    disableCallbacks();
    shutdown();
    safeInstance = nullptr;
}

bool AlexaManager::initialize() {
    Serial.println("\n🎤 Inizializzazione Alexa...");
    
    // Pulizia completa per evitare duplicati
    if (isInitialized) {
        shutdown();
    }
    
    // 🛡️ SAFETY: Disabilita callback durante inizializzazione
    disableCallbacks();
    
    fauxmo->createServer(true);
    fauxmo->setPort(FAUXMO_PORT);
    fauxmo->enable(true);
    
    // Aggiungi dispositivi
    addDevices();
    
    // 🛡️ SAFETY: Imposta callback DOPO aver configurato tutto
    fauxmo->onSetState(onDeviceStateChanged);
    
    isInitialized = true;
    
    // 🛡️ SAFETY: Abilita callback SOLO alla fine
    enableCallbacks();
    
    printStatus();
    
    return true;
}

void AlexaManager::shutdown() {
    if (isInitialized) {
        // 🛡️ SAFETY: Disabilita callback PRIMA dello shutdown
        disableCallbacks();
        
        Serial.println("🔄 Shutdown Alexa...");
        fauxmo->enable(false);
        delay(FAUXMO_DISABLE_DELAY);
        
        isInitialized = false;
    }
}

bool AlexaManager::restart() {
    Serial.println("🔄 Riavvio sistema Alexa...");
    shutdown();
    delay(ALEXA_RESTART_DELAY);
    return initialize();
}

// 🛡️ SAFETY: Metodi di protezione callback
void AlexaManager::enableCallbacks() {
    callbackSafe = true;
    Serial.println("🔒 Callback Alexa abilitati");
}

void AlexaManager::disableCallbacks() {
    callbackSafe = false;
    delay(50); // Aspetta che eventuali callback in corso terminino
    Serial.println("🔒 Callback Alexa disabilitati");
}

bool AlexaManager::isCallbackSafe() const {
    return callbackSafe && safeInstance != nullptr && isInitialized;
}

void AlexaManager::addDevices() {
    for (int i = 0; i < deviceManager->getDeviceCount(); i++) {
        const Device& device = deviceManager->getDevice(i);
        
        // Aggiungi dispositivo a FauxmoESP
        unsigned char deviceId = fauxmo->addDevice(device.name.c_str());
        Serial.printf("   ➕ %s", device.name.c_str());
        
        // 🎯 GESTIONE UUID PERSISTENTE
        if (device.uuid.length() > 0) {
            // UUID già salvato - lo ripristiniamo
            fauxmo->setDeviceUniqueId(deviceId, device.uuid.c_str());
            Serial.printf(" (UUID: %s)\n", device.uuid.substring(0, 8).c_str());
        } else {
            // Nessun UUID salvato - generiamo e salviamo quello nuovo
            saveGeneratedUUID(i, deviceId);
            Serial.println(" (UUID: Nuovo generato)");
        }
    }
}

void AlexaManager::saveGeneratedUUID(int deviceIndex, unsigned char fauxmoDeviceId) {
    // Generiamo l'UUID come fa FauxmoESP internamente
    String mac = WiFi.macAddress();
    char generatedUUID[28];  // FAUXMO_DEVICE_UNIQUE_ID_LENGTH = 27 + 1
    snprintf(generatedUUID, sizeof(generatedUUID), "%02X:%s:%s", fauxmoDeviceId, mac.c_str(), "00:00");
    
    // Salviamo l'UUID nel DeviceManager
    Device* device = deviceManager->findDevice(deviceManager->getDevice(deviceIndex).name);
    if (device) {
        device->uuid = String(generatedUUID);
        deviceManager->saveDevices();  // Salva subito in EEPROM
        Serial.printf("💾 UUID salvato: %s\n", generatedUUID);
    }
}

void AlexaManager::handleDeviceCommand(const char* device_name, bool state) {
    // 🛡️ SAFETY: Controlla se è sicuro procedere
    if (!isCallbackSafe()) {
        Serial.println("⚠️ Callback ignorato - sistema non sicuro");
        return;
    }
    
    // 🔧 FIX: Gestisci ENTRAMBI i comandi (accendi E spegni)
    Serial.printf("🎯 Comando: '%s' -> %s\n", device_name, state ? "ACCENDI" : "SPEGNI");
    
    for (int i = 0; i < deviceManager->getDeviceCount(); i++) {
        const Device& device = deviceManager->getDevice(i);
        if (device.name.equalsIgnoreCase(device_name)) {
            if (device.useCustomUrl) {
                callCustomURL(device.customUrl);
            } else {
                callESP(device.pin);
            }
            return;
        }
    }
    
    Serial.printf("❌ Dispositivo '%s' non trovato!\n", device_name);
}

// 🛡️ SAFETY: Callback statico completamente protetto
void AlexaManager::onDeviceStateChanged(unsigned char device_id, const char* device_name, bool state, unsigned char value) {
    // 🔒 CONTROLLI DI SICUREZZA MULTIPLI
    if (!callbackSafe) {
        return; // Callback disabilitato
    }
    
    if (safeInstance == nullptr) {
        return; // Istanza non valida
    }
    
    if (!safeInstance->isInitialized) {
        return; // Sistema non inizializzato
    }
    
    // 🛡️ SAFETY: Controllo validità parametri
    if (device_name == nullptr || strlen(device_name) == 0) {
        return; // Nome dispositivo non valido
    }
    
    Serial.printf("🗣️ Alexa: '%s' -> %s\n", device_name, state ? "ON" : "OFF");
    
    // 🔒 SAFE: Chiamata protetta al metodo dell'istanza
    safeInstance->handleDeviceCommand(device_name, state);
}

void AlexaManager::handle() {
    if (isInitialized && isCallbackSafe()) {
        fauxmo->handle();
    }
}

void AlexaManager::callESP(int pin) {
    HTTPClient http;
    String url = "http://" + String(ESP_ORIGINALE_IP) + "/pulsePin?pin=" + String(pin);
    
    Serial.printf("📡 Chiamata ESP: Pin %d\n", pin);
    
    http.begin(url);
    http.setTimeout(HTTP_TIMEOUT);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        Serial.printf("✅ Pin %d attivato\n", pin);
    } else {
        Serial.printf("❌ Errore ESP (codice: %d)\n", httpCode);
    }
    
    http.end();
}

void AlexaManager::callCustomURL(const String& url) {
    HTTPClient http;
    
    Serial.printf("📡 Chiamata Custom: %s\n", url.c_str());
    
    http.begin(url);
    http.setTimeout(HTTP_TIMEOUT);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String response = http.getString();
        Serial.println("✅ Richiesta custom eseguita");
        if (response.length() > 0 && response.length() < HTTP_RESPONSE_MAX_LENGTH) {
            Serial.println("📥 Risposta: " + response);
        }
    } else {
        Serial.printf("❌ Errore custom (codice: %d)\n", httpCode);
    }
    
    http.end();
}

void AlexaManager::printStatus() {
    Serial.println("\n🎉 ESP Bridge pronto!");
    Serial.printf("📱 %d dispositivi configurati\n", deviceManager->getDeviceCount());
    
    if (deviceManager->getDeviceCount() > 0) {
        Serial.println("📱 Comandi Alexa:");
        for (int i = 0; i < deviceManager->getDeviceCount() && i < 3; i++) {
            Serial.println("   - 'Alexa, accendi " + deviceManager->getDevice(i).name + "'");
            Serial.println("   - 'Alexa, spegni " + deviceManager->getDevice(i).name + "'");
        }
        if (deviceManager->getDeviceCount() > 3) {
            Serial.println("   - ... e altri dispositivi");
        }
        Serial.println("\n💡 'Alexa, scopri dispositivi' se necessario");
    }
}