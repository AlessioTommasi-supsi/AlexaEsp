#ifndef DEVICE_CONTROLLER_H
#define DEVICE_CONTROLLER_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "../model/SystemConfig.h"
#include "../view/SerialController.h"

// Struttura Device integrata nel controller
struct Device {
    String name;
    int pin;
    bool useCustomUrl;
    String customUrl;
    String uuid;
    
    Device() : pin(-1), useCustomUrl(false) {}
    Device(const String& n, int p) : name(n), pin(p), useCustomUrl(false) {}
    Device(const String& n, const String& url) : name(n), pin(-1), useCustomUrl(true), customUrl(url) {}
};

class DeviceController {
private:
    Device devices[100]; // MAX_DEVICES from SystemConfig
    int deviceCount;
    Preferences* preferences;
    SerialController* serialController;
    SystemConfig* config;
    
    // Internal methods
    void saveDevices();
    void loadDevices();
    Device* findDevice(const String& name);
    String generateUUID(const String& deviceName);
    
public:
    DeviceController(Preferences* prefs, SerialController* serial);
    
    // Device Management
    bool addDevice(const String& name, int pin);
    bool addDevice(const String& name, const String& customUrl);
    bool removeDevice(const String& name);
    bool deviceExists(const String& name);
    
    // Device access
    int getDeviceCount() const { return deviceCount; }
    const Device& getDevice(int index) const { return devices[index]; }
    Device* getDevicePtr(int index) { return &devices[index]; }
    
    // System operations
    void printDevices();
    void clear();
    void initialize();
    
    // Getters
    bool isFull() const;
};

#endif