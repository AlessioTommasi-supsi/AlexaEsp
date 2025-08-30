#ifndef ALEXA_CONTROLLER_H
#define ALEXA_CONTROLLER_H

#include <Arduino.h>
#include <fauxmoESP.h>
#include <HTTPClient.h>
#include "../model/SystemConfig.h"
#include "../view/SerialController.h"
#include "DeviceController.h"

class AlexaController {
private:
    fauxmoESP* fauxmo;
    DeviceController* deviceController;
    SerialController* serialController;
    SystemConfig* config;
    bool isInitialized;
    
    // Variabili per protezione callback
    static volatile bool callbackSafe;
    static AlexaController* safeInstance;
    
    // Callback methods
    static void onDeviceStateChanged(unsigned char device_id, const char* device_name, bool state, unsigned char value);
    void handleDeviceCommand(const char* device_name, bool state);
    
    // Internal methods
    void addDevices();
    void callESP(int pin);
    void callCustomURL(const String& url);
    void enableCallbacks();
    void disableCallbacks();
    bool isCallbackSafe() const;
    
public:
    AlexaController(fauxmoESP* fauxmoInstance, DeviceController* devController, SerialController* serial);
    ~AlexaController();
    
    // Main operations
    bool initialize();
    void shutdown();
    bool restart();
    void handle();
    
    // Status methods
    bool isAlexaInitialized() const { return isInitialized; }
    void printStatus();
    void printAlexaCommands();
};

#endif