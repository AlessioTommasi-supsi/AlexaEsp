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
    
    for (int i = 0; i < deviceCount && i < MAX_DEVICES; i++) {
        String prefix = "dev" + String(i) + "_";
        
        devices[i].name = preferences->getString((prefix + "name").c_str(), "");
        devices[i].pin = preferences->getInt((prefix + "pin").c_str(), -1);
        devices[i].useCustomUrl = preferences->getBool((prefix + "custom").c_str(), false);
        devices[i].customUrl = preferences->getString((prefix + "url").c_str(), "");
        devices[i].uuid = preferences->getString((prefix + "uuid").c_str(), "");  // NUOVO: Carica UUID salvato
    }
    
    Serial.printf("💾 Caricati %d dispositivi\n", deviceCount);
}

void DeviceManager::saveDevices() {
    preferences->putInt("device_count", deviceCount);
    
    for (int i = 0; i < deviceCount; i++) {
        String prefix = "dev" + String(i) + "_";
        
        preferences->putString((prefix + "name").c_str(), devices[i].name);
        preferences->putInt((prefix + "pin").c_str(), devices[i].pin);
        preferences->putBool((prefix + "custom").c_str(), devices[i].useCustomUrl);
        preferences->putString((prefix + "url").c_str(), devices[i].customUrl);
        preferences->putString((prefix + "uuid").c_str(), devices[i].uuid);  // NUOVO: Salva UUID
    }
    
    Serial.printf("💾 Salvati %d dispositivi con UUID\n", deviceCount);
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