#include "device_manager.h"
#include <WiFi.h>

DeviceManager::DeviceManager(Preferences* prefs) : deviceCount(0), preferences(prefs) {}

bool DeviceManager::addDevice(const String& name, int pin) {
    if (isFull() || deviceExists(name)) return false;
    
    Device newDevice;
    newDevice.name = name;
    newDevice.pin = pin;
    newDevice.useCustomUrl = false;
    
    devices[deviceCount++] = newDevice;
    saveDevices();
    return true;
}

bool DeviceManager::addDevice(const String& name, const String& customUrl) {
    if (isFull() || deviceExists(name)) return false;
    
    Device newDevice;
    newDevice.name = name;
    newDevice.customUrl = customUrl;
    newDevice.useCustomUrl = true;
    
    devices[deviceCount++] = newDevice;
    saveDevices();
    return true;
}

bool DeviceManager::removeDevice(const String& name) {
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].name.equalsIgnoreCase(name)) {
            // Sposta elementi indietro
            for (int j = i; j < deviceCount - 1; j++) {
                devices[j] = devices[j + 1];
            }
            deviceCount--;
            saveDevices();
            return true;
        }
    }
    return false;
}

bool DeviceManager::deviceExists(const String& name) {
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].name.equalsIgnoreCase(name)) {
            return true;
        }
    }
    return false;
}

Device* DeviceManager::findDevice(const String& name) {
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].name.equalsIgnoreCase(name)) {
            return &devices[i];
        }
    }
    return nullptr;
}

void DeviceManager::loadDevices() {
    deviceCount = preferences->getInt("device_count", 0);
    if (deviceCount > MAX_DEVICES) deviceCount = MAX_DEVICES;
    
    Serial.printf("💾 Caricati %d dispositivi salvati\n", deviceCount);
    
    for (int i = 0; i < deviceCount; i++) {
        String nameKey = "dev_name_" + String(i);
        String pinKey = "dev_pin_" + String(i);
        String urlKey = "dev_url_" + String(i);
        String customKey = "dev_custom_" + String(i);
        
        devices[i].name = preferences->getString(nameKey.c_str(), "");
        devices[i].pin = preferences->getInt(pinKey.c_str(), 0);
        devices[i].customUrl = preferences->getString(urlKey.c_str(), "");
        devices[i].useCustomUrl = preferences->getBool(customKey.c_str(), false);
        
        if (devices[i].name.length() > 0) {
            Serial.printf("   📱 %s -> %s\n", devices[i].name.c_str(), 
                         devices[i].useCustomUrl ? devices[i].customUrl.c_str() : ("Pin " + String(devices[i].pin)).c_str());
        }
    }
}

void DeviceManager::saveDevices() {
    preferences->putInt("device_count", deviceCount);
    
    for (int i = 0; i < deviceCount; i++) {
        String nameKey = "dev_name_" + String(i);
        String pinKey = "dev_pin_" + String(i);
        String urlKey = "dev_url_" + String(i);
        String customKey = "dev_custom_" + String(i);
        
        preferences->putString(nameKey.c_str(), devices[i].name);
        preferences->putInt(pinKey.c_str(), devices[i].pin);
        preferences->putString(urlKey.c_str(), devices[i].customUrl);
        preferences->putBool(customKey.c_str(), devices[i].useCustomUrl);
    }
    Serial.println("💾 Dispositivi salvati");
}

void DeviceManager::printDevices() {
    Serial.println("\n📱 Dispositivi configurati:");
    Serial.println("============================");
    
    if (deviceCount == 0) {
        Serial.println("   Nessun dispositivo configurato");
        return;
    }
    
    for (int i = 0; i < deviceCount; i++) {
        Serial.printf("%2d. %-20s -> ", i + 1, devices[i].name.c_str());
        if (devices[i].useCustomUrl) {
            Serial.println("🌐 " + devices[i].customUrl);
        } else {
            Serial.printf("📍 Pin %d (pulsePin)\n", devices[i].pin);
        }
    }
    
    Serial.printf("\nTotale: %d/%d dispositivi\n", deviceCount, MAX_DEVICES);
}

void DeviceManager::clear() {
    deviceCount = 0;
}