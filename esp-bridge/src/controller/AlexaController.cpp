#include "AlexaController.h"

// Static variables initialization
volatile bool AlexaController::callbackSafe = false;
AlexaController* AlexaController::safeInstance = nullptr;

AlexaController::AlexaController(fauxmoESP* fauxmoInstance, DeviceController* devController, SerialController* serial)
    : fauxmo(fauxmoInstance), deviceController(devController), serialController(serial), isInitialized(false) {
    config = SystemConfig::getInstance();
    safeInstance = this;
    callbackSafe = false;
}

AlexaController::~AlexaController() {
    disableCallbacks();
    shutdown();
    safeInstance = nullptr;
}

bool AlexaController::initialize() {
    serialController->printAlexaInitializing();
    
    if (isInitialized) {
        shutdown();
    }
    
    disableCallbacks();
    
    fauxmo->createServer(true);
    fauxmo->setPort(config->FAUXMO_PORT);
    fauxmo->enable(true);
    
    addDevices();
    
    fauxmo->onSetState(onDeviceStateChanged);
    
    isInitialized = true;
    enableCallbacks();
    
    serialController->printAlexaReady(deviceController->getDeviceCount());
    return true;
}

void AlexaController::shutdown() {
    if (!isInitialized) return;
    
    serialController->println("🎤 Spegnimento Alexa...");
    disableCallbacks();
    
    fauxmo->enable(false);
    delay(config->ALEXA_RESTART_DELAY);
    
    isInitialized = false;
}

bool AlexaController::restart() {
    serialController->println("🔄 Riavvio sistema Alexa...");
    shutdown();
    delay(config->ALEXA_RESTART_DELAY);
    return initialize();
}

void AlexaController::handle() {
    if (isInitialized && isCallbackSafe()) {
        fauxmo->handle();
    }
}

void AlexaController::enableCallbacks() {
    delay(100);
    callbackSafe = true;
}

void AlexaController::disableCallbacks() {
    callbackSafe = false;
    delay(100);
}

bool AlexaController::isCallbackSafe() const {
    return callbackSafe && safeInstance != nullptr;
}

void AlexaController::addDevices() {
    for (int i = 0; i < deviceController->getDeviceCount(); i++) {
        const Device& device = deviceController->getDevice(i);
        
        unsigned char deviceId = fauxmo->addDevice(device.name.c_str());
        serialController->printf("🎤 Aggiunto dispositivo Alexa: %s\n", device.name.c_str());
        
        if (device.uuid.length() > 0) {
            fauxmo->setDeviceUniqueId(deviceId, device.uuid.c_str());
        }
    }
}

void AlexaController::onDeviceStateChanged(unsigned char device_id, const char* device_name, bool state, unsigned char value) {
    if (!callbackSafe || !safeInstance) {
        return;
    }
    
    safeInstance->handleDeviceCommand(device_name, state);
}

void AlexaController::handleDeviceCommand(const char* device_name, bool state) {
    if (!isCallbackSafe()) {
        serialController->println("⚠️ Callback ignorato - sistema non sicuro");
        return;
    }
    
    serialController->printAlexaCommand(device_name, state);
    
    Device* device = nullptr;
    for (int i = 0; i < deviceController->getDeviceCount(); i++) {
        if (deviceController->getDevice(i).name.equalsIgnoreCase(device_name)) {
            device = deviceController->getDevicePtr(i);
            break;
        }
    }
    
    if (device) {
        if (device->useCustomUrl) {
            callCustomURL(device->customUrl);
        } else {
            callESP(device->pin);
        }
    } else {
        serialController->printf("❌ Dispositivo '%s' non trovato!\n", device_name);
    }
}

void AlexaController::callESP(int pin) {
    HTTPClient http;
    String url = "http://" + String(config->ESP_ORIGINALE_IP) + "/pulsePin?pin=" + String(pin);
    
    serialController->printf("📡 Chiamata ESP: Pin %d\n", pin);
    
    http.begin(url);
    http.setTimeout(config->HTTP_TIMEOUT);
    int httpCode = http.GET();
    
    bool success = (httpCode == 200);
    serialController->printAlexaResponse(pin, success, httpCode);
    
    http.end();
}

void AlexaController::callCustomURL(const String& url) {
    HTTPClient http;
    
    serialController->printf("🌐 Chiamata URL: %s\n", url.c_str());
    
    http.begin(url);
    http.setTimeout(config->HTTP_TIMEOUT);
    int httpCode = http.GET();
    
    String response = "";
    if (httpCode == 200) {
        response = http.getString();
    }
    
    bool success = (httpCode == 200);
    serialController->printAlexaCustomResponse(url, success, httpCode, response);
    
    http.end();
}

void AlexaController::printStatus() {
    serialController->printf("🎤 Alexa: %s\n", isInitialized ? "✅ Attivo" : "❌ Non attivo");
    if (isInitialized && deviceController->getDeviceCount() > 0) {
        serialController->println("💡 'Alexa, scopri dispositivi' se necessario");
    }
}

void AlexaController::printAlexaCommands() {
    int deviceCount = deviceController->getDeviceCount();
    if (deviceCount > 0) {
        serialController->println("📱 Comandi Alexa:");
        for (int i = 0; i < deviceCount && i < 3; i++) {
            const Device& device = deviceController->getDevice(i);
            serialController->printf("   - 'Alexa, accendi %s'\n", device.name.c_str());
            serialController->printf("   - 'Alexa, spegni %s'\n", device.name.c_str());
        }
        if (deviceCount > 3) {
            serialController->println("   - ... e altri dispositivi");
        }
    }
}