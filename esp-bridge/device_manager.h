#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// Struttura per dispositivi
struct Device {
    String name;
    int pin;
    String customUrl;
    bool useCustomUrl;
    
    Device() : pin(0), useCustomUrl(false) {}
};

class DeviceManager {
private:
    Device devices[MAX_DEVICES];
    int deviceCount;
    Preferences* preferences;
    
public:
    DeviceManager(Preferences* prefs);
    
    // Gestione dispositivi
    bool addDevice(const String& name, int pin);
    bool addDevice(const String& name, const String& customUrl);
    bool removeDevice(const String& name);
    bool deviceExists(const String& name);
    
    // Accesso dati
    int getDeviceCount() const { return deviceCount; }
    const Device& getDevice(int index) const { return devices[index]; }
    Device* findDevice(const String& name);
    
    // Persistenza
    void loadDevices();
    void saveDevices();
    
    // Utility
    void printDevices();
    bool isFull() const { return deviceCount >= MAX_DEVICES; }
    void clear();
};

#endif