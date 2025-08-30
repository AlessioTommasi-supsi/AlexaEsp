#ifndef SERIAL_CONTROLLER_H
#define SERIAL_CONTROLLER_H

#include <Arduino.h>
#include "../model/SystemConfig.h"

class SerialController {
private:
    SystemConfig* config;
    
public:
    SerialController();
    
    // System Messages
    void printWelcome();
    void printMainMenu(bool wifiConnected, int deviceCount, bool alexaActive);
    void printSystemStatus(const String& ssid, const String& ip, int rssi, const String& mac, 
                          int deviceCount, bool alexaActive, unsigned long uptime);
    
    // WiFi Messages
    void printWiFiNetworks(int networkCount);
    void printNetworkInfo(int index, const String& ssid, int rssi, bool encrypted);
    void printWiFiConnected(const String& ssid, const String& ip);
    void printWiFiStatus(const String& message);
    
    // Device Messages
    void printDeviceList(int deviceCount);
    void printDevice(int index, const String& name, bool isCustomUrl, int pin, const String& url);
    void printDeviceAction(const String& action, const String& name);
    
    // Alexa Messages
    void printAlexaInitializing();
    void printAlexaReady(int deviceCount);
    void printAlexaShutdown();
    void printAlexaRestart();
    void printAlexaCommand(const String& deviceName, bool state);
    void printAlexaResponse(int pin, bool success, int httpCode);
    void printAlexaCustomResponse(const String& url, bool success, int httpCode, const String& response = "");
    
    // Input Prompts - usando il formato del sistema funzionante
    void promptWiFiSelection(int maxOption);
    void promptPassword();
    void promptDeviceName();
    void promptDeviceType();
    void promptDevicePin(int minPin, int maxPin);
    void promptDeviceURL();
    void promptRemoveDevice();
    void promptResetConfirm();
    void promptMenuOption();
    
    // Utility methods
    void print(const String& message);
    void println(const String& message);
    void printf(const char* format, ...);
};

#endif