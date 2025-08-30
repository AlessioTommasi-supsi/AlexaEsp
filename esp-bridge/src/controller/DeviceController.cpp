#include "DeviceController.h"

DeviceController::DeviceController(Preferences* prefs, SerialController* serial) 
    : deviceCount(0), preferences(prefs), serialController(serial) {
    config = SystemConfig::getInstance();
}

void DeviceController::initialize() {
    loadDevices();
    serialController->printf("ℹ️ DeviceController inizializzato con %d dispositivi\n", deviceCount);
}

bool DeviceController::addDevice(const String& name, int pin) {
    if (isFull() || deviceExists(name)) return false;
    
    Device newDevice(name, pin);
    newDevice.uuid = generateUUID(name);
    
    devices[deviceCount++] = newDevice;
    saveDevices();
    serialController->printDeviceAction("✅ Dispositivo aggiunto", name);
    return true;
}

bool DeviceController::addDevice(const String& name, const String& customUrl) {
    if (isFull() || deviceExists(name)) return false;
    
    Device newDevice(name, customUrl);
    newDevice.uuid = generateUUID(name);
    
    devices[deviceCount++] = newDevice;
    saveDevices();
    serialController->printDeviceAction("✅ Dispositivo aggiunto", name);
    return true;
}

bool DeviceController::removeDevice(const String& name) {
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].name.equalsIgnoreCase(name)) {
            // Sposta elementi indietro
            for (int j = i; j < deviceCount - 1; j++) {
                devices[j] = devices[j + 1];
            }
            deviceCount--;
            saveDevices();
            serialController->printDeviceAction("✅ Dispositivo rimosso", name);
            return true;
        }
    }
    serialController->printf("❌ Dispositivo '%s' non trovato\n", name.c_str());
    return false;
}

bool DeviceController::deviceExists(const String& name) {
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].name.equalsIgnoreCase(name)) {
            return true;
        }
    }
    return false;
}

Device* DeviceController::findDevice(const String& name) {
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].name.equalsIgnoreCase(name)) {
            return &devices[i];
        }
    }
    return nullptr;
}

void DeviceController::printDevices() {
    serialController->printDeviceList(deviceCount);
    
    for (int i = 0; i < deviceCount; i++) {
        serialController->printDevice(i, devices[i].name, devices[i].useCustomUrl, 
                                    devices[i].pin, devices[i].customUrl);
    }
    
    if (deviceCount > 0) {
        serialController->printf("Totale: %d/%d dispositivi\n", deviceCount, config->MAX_DEVICES);
    }
}

void DeviceController::saveDevices() {
    preferences->putInt("device_count", deviceCount);
    
    for (int i = 0; i < deviceCount; i++) {
        String prefix = "dev" + String(i) + "_";
        
        preferences->putString((prefix + "name").c_str(), devices[i].name);
        preferences->putInt((prefix + "pin").c_str(), devices[i].pin);
        preferences->putBool((prefix + "custom").c_str(), devices[i].useCustomUrl);
        preferences->putString((prefix + "url").c_str(), devices[i].customUrl);
        preferences->putString((prefix + "uuid").c_str(), devices[i].uuid);
    }
}

void DeviceController::loadDevices() {
    deviceCount = preferences->getInt("device_count", 0);
    
    for (int i = 0; i < deviceCount && i < config->MAX_DEVICES; i++) {
        String prefix = "dev" + String(i) + "_";
        
        devices[i].name = preferences->getString((prefix + "name").c_str(), "");
        devices[i].pin = preferences->getInt((prefix + "pin").c_str(), -1);
        devices[i].useCustomUrl = preferences->getBool((prefix + "custom").c_str(), false);
        devices[i].customUrl = preferences->getString((prefix + "url").c_str(), "");
        devices[i].uuid = preferences->getString((prefix + "uuid").c_str(), "");
        
        if (devices[i].uuid.length() == 0) {
            devices[i].uuid = generateUUID(devices[i].name);
        }
    }
}

String DeviceController::generateUUID(const String& deviceName) {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();
    
    // Genera UUID basato su MAC + nome dispositivo
    String uuid = "2f402f80-da50-11e1-9b23-" + mac.substring(0, 12);
    return uuid;
}

void DeviceController::clear() {
    deviceCount = 0;
    saveDevices();
    serialController->println("✅ Tutti i dispositivi rimossi");
}

bool DeviceController::isFull() const {
    return deviceCount >= config->MAX_DEVICES;
}