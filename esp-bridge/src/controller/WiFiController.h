#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "../model/SystemConfig.h"
#include "../view/SerialController.h"

class WiFiController {
private:
    Preferences* preferences;
    SerialController* serialController;
    SystemConfig* config;
    
    bool isConnected;
    unsigned long lastCheck;
    int selectedNetworkIndex;
    String selectedSSID;
    bool configuringMode;
    int cachedNetworkCount;
    
    // Internal methods
    bool attemptConnection(const String& ssid, const String& password, int maxAttempts);
    void saveCredentials(const String& ssid, const String& password);
    void updateConnectionState();
    
public:
    WiFiController(Preferences* prefs, SerialController* serial);
    
    // Connection Management
    bool connectToSaved();
    bool autoReconnect();
    void disconnect();
    void checkConnection();
    
    // Configuration mode
    void startConfiguration();
    bool handleNetworkSelection(const String& input);
    bool handlePasswordInput(const String& input);
    void cancelConfiguration();
    
    // Network Scanning
    int scanNetworks();
    void printNetworks(int networkCount);
    void cleanupScan();
    
    // Status methods
    bool isWiFiConnected() const { return isConnected; }
    bool isConfiguring() const { return configuringMode; }
    
    // Info methods
    String getSSID() const;
    String getIP() const;
    int getRSSI() const;
    String getMAC() const;
    String getSelectedSSID() const { return selectedSSID; }
    
    // System operations
    void initialize();
};

#endif