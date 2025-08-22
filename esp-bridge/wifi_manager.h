#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>
#include "config.h"

class WiFiManager {
private:
    Preferences* preferences;
    bool isConnected;
    unsigned long lastCheck;
    
    // Stato configurazione WiFi
    int selectedNetworkIndex;
    String selectedSSID;
    bool configuringMode;  // Rinominato per evitare conflitto
    int cachedNetworkCount;  // NUOVO: Cache del numero di reti trovate
    
public:
    WiFiManager(Preferences* prefs);
    
    // Connessione WiFi
    bool connectToSaved();
    bool attemptConnection(const String& ssid, const String& password, int maxAttempts = WIFI_CONNECT_TIMEOUT);
    void saveCredentials(const String& ssid, const String& password);
    void disconnect();
    
    // Gestione stato
    bool isWiFiConnected() const { return isConnected; }
    void checkConnection();
    bool autoReconnect();
    
    // Configurazione interattiva
    void startConfiguration();
    bool handleNetworkSelection(const String& input);
    bool handlePasswordInput(const String& input);
    void cancelConfiguration();
    
    // Scansione reti
    int scanNetworks();
    void printNetworks(int networkCount);
    void cleanupScan();
    
    // Info WiFi
    String getSSID() const;
    String getIP() const;
    int getRSSI() const;
    String getMAC() const;
    
    // Stato configurazione
    bool isConfiguring() const { return configuringMode; }  // Usa la variabile rinominata
    String getSelectedSSID() const { return selectedSSID; }
};

#endif