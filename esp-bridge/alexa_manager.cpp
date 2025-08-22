#include "alexa_manager.h"

// Riferimenti globali per callback static
AlexaManager* globalAlexaManager = nullptr;
DeviceManager* globalDeviceManager = nullptr;

AlexaManager::AlexaManager(fauxmoESP* fauxmoInstance, DeviceManager* devManager) 
    : fauxmo(fauxmoInstance), deviceManager(devManager), isInitialized(false) {
    globalAlexaManager = this;
    globalDeviceManager = devManager;
}

bool AlexaManager::initialize() {
    Serial.println("\n🎤 Inizializzazione Alexa...");
    
    // Pulizia completa per evitare duplicati
    if (isInitialized) {
        shutdown();
    }
    
    fauxmo->createServer(true);
    fauxmo->setPort(FAUXMO_PORT);
    fauxmo->enable(true);
    
    // Aggiungi dispositivi
    addDevices();
    
    // Imposta callback
    fauxmo->onSetState(onDeviceStateChanged);
    
    isInitialized = true;
    printStatus();
    
    return true;
}

void AlexaManager::shutdown() {
    if (isInitialized) {
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

void AlexaManager::addDevices() {
    for (int i = 0; i < deviceManager->getDeviceCount(); i++) {
        const Device& device = deviceManager->getDevice(i);
        fauxmo->addDevice(device.name.c_str());
        Serial.printf("   ➕ %s\n", device.name.c_str());
    }
}

void AlexaManager::handleDeviceCommand(const char* device_name, bool state) {
    if (!state) return; // Solo comandi "accendi"
    
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

void AlexaManager::onDeviceStateChanged(unsigned char device_id, const char* device_name, bool state, unsigned char value) {
    Serial.printf("🗣️ Alexa: '%s' -> %s\n", device_name, state ? "ON" : "OFF");
    
    if (globalAlexaManager) {
        globalAlexaManager->handleDeviceCommand(device_name, state);
    }
}

void AlexaManager::handle() {
    if (isInitialized) {
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
        }
        if (deviceManager->getDeviceCount() > 3) {
            Serial.println("   - ... e altri dispositivi");
        }
        Serial.println("\n💡 'Alexa, scopri dispositivi' se necessario");
    }
}