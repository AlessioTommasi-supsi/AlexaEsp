#include "WiFiController.h"

WiFiController::WiFiController(Preferences* prefs, SerialController* serial) 
    : preferences(prefs), serialController(serial), isConnected(false), lastCheck(0),
      selectedNetworkIndex(-1), configuringMode(false), cachedNetworkCount(0) {
    config = SystemConfig::getInstance();
}

void WiFiController::initialize() {
    updateConnectionState();
    serialController->println("ℹ️ WiFiController inizializzato");
}

bool WiFiController::connectToSaved() {
    String ssid = preferences->getString("wifi_ssid", "");
    String password = preferences->getString("wifi_pass", "");
    
    if (ssid.length() == 0) {
        serialController->println("ℹ️ Nessun WiFi salvato");
        return false;
    }
    
    serialController->printf("📡 Connessione WiFi salvato: %s\n", ssid.c_str());
    return attemptConnection(ssid, password, config->WIFI_RETRY_TIMEOUT);
}

bool WiFiController::attemptConnection(const String& ssid, const String& password, int maxAttempts) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    serialController->println("📡 Connessione in corso...");
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    updateConnectionState();
    
    if (isConnected) {
        serialController->printWiFiConnected(WiFi.SSID(), WiFi.localIP().toString());
        return true;
    } else {
        serialController->printWiFiStatus("❌ Connessione fallita!");
        return false;
    }
}

void WiFiController::saveCredentials(const String& ssid, const String& password) {
    preferences->putString("wifi_ssid", ssid);
    preferences->putString("wifi_pass", password);
    serialController->println("💾 Credenziali WiFi salvate");
}

void WiFiController::disconnect() {
    WiFi.disconnect();
    isConnected = false;
}

void WiFiController::checkConnection() {
    if (millis() - lastCheck < config->WIFI_CHECK_INTERVAL) return;
    lastCheck = millis();
    
    updateConnectionState();
    if (!isConnected && WiFi.status() != WL_CONNECTED) {
        serialController->printWiFiStatus("⚠️ WiFi disconnesso!");
    }
}

bool WiFiController::autoReconnect() {
    if (isConnected) return true;
    
    String ssid = preferences->getString("wifi_ssid", "");
    String password = preferences->getString("wifi_pass", "");
    
    if (ssid.length() > 0) {
        serialController->println("🔄 Tentativo riconnessione automatica...");
        return attemptConnection(ssid, password, config->WIFI_RECONNECT_ATTEMPTS);
    }
    
    return false;
}

void WiFiController::startConfiguration() {
    configuringMode = true;
    selectedNetworkIndex = -1;
    selectedSSID = "";
    cachedNetworkCount = 0;
    
    serialController->println("🔍 Scansione reti WiFi...");
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(config->WIFI_SCAN_DELAY);
    
    cachedNetworkCount = scanNetworks();
    
    if (cachedNetworkCount <= 0) {
        if (cachedNetworkCount < 0) {
            serialController->printWiFiStatus("❌ Errore durante la scansione WiFi!");
        } else {
            serialController->printWiFiStatus("❌ Nessuna rete WiFi trovata!");
        }
        serialController->println("ℹ️ Verifica che il WiFi sia abilitato e riprova.");
        cancelConfiguration();
        return;
    }
    
    printNetworks(cachedNetworkCount);
    serialController->promptWiFiSelection(cachedNetworkCount);
}

int WiFiController::scanNetworks() {
    return WiFi.scanNetworks();
}

void WiFiController::printNetworks(int networkCount) {
    serialController->printWiFiNetworks(networkCount);
    
    for (int i = 0; i < networkCount; i++) {
        bool encrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        serialController->printNetworkInfo(i, WiFi.SSID(i), WiFi.RSSI(i), encrypted);
    }
}

bool WiFiController::handleNetworkSelection(const String& input) {
    serialController->printf("🔍 DEBUG: handleNetworkSelection input='%s', cachedNetworkCount=%d\n", input.c_str(), cachedNetworkCount);
    
    if (input == "0") {
        serialController->println("ℹ️ Configurazione WiFi annullata");
        cancelConfiguration();
        return true; // Configurazione completata (annullata)
    }
    
    int selection = input.toInt();
    serialController->printf("🔍 DEBUG: selection=%d, range: 1-%d\n", selection, cachedNetworkCount);
    
    if (selection < 1 || selection > cachedNetworkCount) {
        serialController->printf("❌ Selezione non valida! Riprova (1-%d):\n", cachedNetworkCount);
        serialController->promptWiFiSelection(cachedNetworkCount);
        return false; // Continua a chiedere selezione
    }
    
    selectedNetworkIndex = selection - 1;
    selectedSSID = WiFi.SSID(selectedNetworkIndex);
    
    serialController->printf("✅ Rete selezionata: %s\n", selectedSSID.c_str());
    
    // Controlla se la rete ha una password
    if (WiFi.encryptionType(selectedNetworkIndex) == WIFI_AUTH_OPEN) {
        serialController->println("🔍 DEBUG: Rete aperta, connessione diretta");
        // Rete aperta, connetti direttamente
        if (attemptConnection(selectedSSID, "", config->WIFI_CONNECT_TIMEOUT)) {
            saveCredentials(selectedSSID, "");
        }
        cancelConfiguration();
        return true; // Configurazione completata
    } else {
        serialController->println("🔍 DEBUG: Rete protetta, richiedo password");
        // Rete protetta, chiedi password
        serialController->promptPassword();
        return false; // Aspetta password
    }
}

bool WiFiController::handlePasswordInput(const String& input) {
    if (input == "0") {
        serialController->println("ℹ️ Configurazione WiFi annullata");
        cancelConfiguration();
        return true; // Configurazione completata (annullata)
    }
    
    String password = input;
    password.trim();
    
    if (password.length() == 0) {
        serialController->println("❌ Password vuota! Riprova:");
        serialController->promptPassword();
        return false; // Continua a chiedere password
    }
    
    if (attemptConnection(selectedSSID, password, config->WIFI_CONNECT_TIMEOUT)) {
        saveCredentials(selectedSSID, password);
        cancelConfiguration();
        return true; // Configurazione completata con successo
    } else {
        serialController->println("❌ Password errata! Riprova (0=annulla):");
        serialController->promptPassword();
        return false; // Continua a chiedere password
    }
}

void WiFiController::cancelConfiguration() {
    configuringMode = false;
    selectedNetworkIndex = -1;
    selectedSSID = "";
    cachedNetworkCount = 0;
    cleanupScan();
}

void WiFiController::cleanupScan() {
    WiFi.scanDelete();
}

void WiFiController::updateConnectionState() {
    isConnected = (WiFi.status() == WL_CONNECTED);
}

String WiFiController::getSSID() const {
    return isConnected ? WiFi.SSID() : "";
}

String WiFiController::getIP() const {
    return isConnected ? WiFi.localIP().toString() : "";
}

int WiFiController::getRSSI() const {
    return isConnected ? WiFi.RSSI() : 0;
}

String WiFiController::getMAC() const {
    return WiFi.macAddress();
}
