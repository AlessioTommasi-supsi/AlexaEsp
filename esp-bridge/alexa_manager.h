#ifndef ALEXA_MANAGER_H
#define ALEXA_MANAGER_H

#include <fauxmoESP.h>
#include <HTTPClient.h>
#include "config.h"
#include "device_manager.h"

class AlexaManager {
private:
    fauxmoESP* fauxmo;
    DeviceManager* deviceManager;
    bool isInitialized;
    
    // Callback functions
    static void onDeviceStateChanged(unsigned char device_id, const char* device_name, bool state, unsigned char value);
    
public:
    AlexaManager(fauxmoESP* fauxmoInstance, DeviceManager* devManager);
    
    // Inizializzazione Alexa
    bool initialize();
    void shutdown();
    bool restart();
    
    // Gestione dispositivi Alexa
    void addDevices();
    void handleDeviceCommand(const char* device_name, bool state);
    
    // Stato
    bool isAlexaInitialized() const { return isInitialized; }
    void handle();
    
    // Chiamate HTTP
    static void callESP(int pin);
    static void callCustomURL(const String& url);
    
    // Info
    void printStatus();
};

// Riferimenti globali per callback static
extern AlexaManager* globalAlexaManager;
extern DeviceManager* globalDeviceManager;

#endif