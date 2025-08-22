#include "wifi_manager.h"

WiFiManager::WiFiManager(Preferences* prefs) : preferences(prefs), isConnected(false), lastCheck(0), selectedNetworkIndex(-1), configuringMode(false), cachedNetworkCount(0) {}

bool WiFiManager::connectToSaved() {
    String ssid = preferences->getString("wifi_ssid", "");
    String password = preferences->getString("wifi_pass", "");
    
    if (ssid.length() == 0) {
        Serial.println("💾 Nessun WiFi salvato");
        return false;
    }
    
    Serial.println("💾 Connessione WiFi salvato: " + ssid);
    return attemptConnection(ssid, password, WIFI_RETRY_TIMEOUT);
}

bool WiFiManager::attemptConnection(const String& ssid, const String& password, int maxAttempts) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    Serial.println("🔄 Connessione in corso...");
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi connesso!");
        Serial.printf("📍 IP: %s\n", WiFi.localIP().toString().c_str());
        isConnected = true;
        return true;
    } else {
        Serial.println("\n❌ Connessione fallita!");
        isConnected = false;
        return false;
    }
}

void WiFiManager::saveCredentials(const String& ssid, const String& password) {
    preferences->putString("wifi_ssid", ssid);
    preferences->putString("wifi_pass", password);
    Serial.println("💾 Credenziali WiFi salvate");
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    isConnected = false;
}

void WiFiManager::checkConnection() {
    if (millis() - lastCheck < WIFI_CHECK_INTERVAL) return;
    lastCheck = millis();
    
    if (isConnected && WiFi.status() != WL_CONNECTED) {
        Serial.println("\n⚠️ WiFi disconnesso!");
        isConnected = false;
    }
}

bool WiFiManager::autoReconnect() {
    if (isConnected) return true;
    
    String ssid = preferences->getString("wifi_ssid", "");
    String password = preferences->getString("wifi_pass", "");
    
    if (ssid.length() > 0) {
        Serial.println("🔄 Tentativo riconnessione automatica...");
        WiFi.begin(ssid.c_str(), password.c_str());
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < WIFI_RECONNECT_ATTEMPTS) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✅ WiFi riconnesso!");
            isConnected = true;
            return true;
        }
    }
    
    return false;
}

void WiFiManager::startConfiguration() {
    configuringMode = true;
    selectedNetworkIndex = -1;
    selectedSSID = "";
    cachedNetworkCount = 0;  // Reset cache
    
    Serial.println("\n📡 Scansione reti WiFi...");
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(WIFI_SCAN_DELAY);
    
    cachedNetworkCount = scanNetworks();  // SALVA il risultato nella cache
    
    // BUG FIX: Gestione corretta di errori e reti vuote
    if (cachedNetworkCount <= 0) {
        if (cachedNetworkCount < 0) {
            Serial.println("❌ Errore durante la scansione WiFi!");
        } else {
            Serial.println("❌ Nessuna rete WiFi trovata!");
        }
        Serial.println("💡 Verifica che il WiFi sia abilitato e riprova.");
        cancelConfiguration();
        return;
    }
    
    printNetworks(cachedNetworkCount);
    Serial.printf("\n🔢 Scegli rete (1-%d) o 0 per annullare:\n", cachedNetworkCount);
    Serial.print("👉 ");
}

int WiFiManager::scanNetworks() {
    return WiFi.scanNetworks();
}

void WiFiManager::printNetworks(int networkCount) {
    Serial.println("\n📋 Reti WiFi disponibili:");
    Serial.println("==========================");
    
    for (int i = 0; i < networkCount; i++) {
        Serial.printf("%2d. %-25s", i + 1, WiFi.SSID(i).c_str());
        
        int rssi = WiFi.RSSI(i);
        if (rssi > -50) Serial.print(" 📶📶📶📶");
        else if (rssi > -60) Serial.print(" 📶📶📶");
        else if (rssi > -70) Serial.print(" 📶📶");
        else Serial.print(" 📶");
        
        if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) {
            Serial.print(" 🔒");
        } else {
            Serial.print(" 🔓");
        }
        
        Serial.printf(" (%d dBm)\n", rssi);
    }
}

bool WiFiManager::handleNetworkSelection(const String& input) {
    if (!configuringMode) return false;
    
    int choice = input.toInt();
    
    if (choice == 0) {
        Serial.println("❌ Configurazione WiFi annullata");
        cancelConfiguration();
        return true; // Configurazione completata (annullata)
    }
    
    // BUG FIX: USA LA CACHE invece di ri-scansionare!
    if (choice < 1 || choice > cachedNetworkCount) {
        Serial.printf("❌ Numero non valido (1-%d)! Riprova:\n", cachedNetworkCount);
        Serial.print("👉 ");
        return false; // Continua configurazione
    }
    
    selectedNetworkIndex = choice - 1;
    selectedSSID = WiFi.SSID(selectedNetworkIndex);
    Serial.println("✅ Rete: " + selectedSSID);
    
    if (WiFi.encryptionType(selectedNetworkIndex) != WIFI_AUTH_OPEN) {
        Serial.println("🔐 Inserisci password (o 0=annulla, r=riprova selezione):");
        Serial.print("👉 ");
        return false; // Aspetta password
    } else {
        // Rete aperta - tenta connessione
        if (attemptConnection(selectedSSID, "")) {
            saveCredentials(selectedSSID, "");
            cancelConfiguration();
            return true; // Successo
        } else {
            Serial.println("💡 Vuoi riprovare? (r=riprova selezione, 0=annulla, altro=riprova connessione):");
            Serial.print("👉 ");
            return false; // Permette di riprovare
        }
    }
}

bool WiFiManager::handlePasswordInput(const String& input) {
    if (!configuringMode || selectedSSID.length() == 0) return false;
    
    String choice = input;
    choice.trim();
    
    if (choice == "0") {
        Serial.println("❌ Configurazione WiFi annullata");
        cancelConfiguration();
        return true;
    }
    
    if (choice.equalsIgnoreCase("r")) {
        Serial.println("🔄 Torno alla selezione rete...");
        startConfiguration();
        return false;
    }
    
    // BUG FIX: Permetti di riprovare la password
    Serial.println("✅ Tentativo connessione...");
    if (attemptConnection(selectedSSID, choice)) {
        saveCredentials(selectedSSID, choice);
        cancelConfiguration();
        return true; // Successo
    } else {
        Serial.println("💡 Password errata. Opzioni:");
        Serial.println("   - Reinserisci password");
        Serial.println("   - 'r' per cambiare rete");
        Serial.println("   - '0' per annullare");
        Serial.print("👉 ");
        return false; // Permette di riprovare
    }
}

void WiFiManager::cancelConfiguration() {
    configuringMode = false;
    selectedNetworkIndex = -1;
    selectedSSID = "";
    cachedNetworkCount = 0;  // Reset cache
    cleanupScan();
}

void WiFiManager::cleanupScan() {
    WiFi.scanDelete();
}

String WiFiManager::getSSID() const {
    return isConnected ? WiFi.SSID() : "";
}

String WiFiManager::getIP() const {
    return isConnected ? WiFi.localIP().toString() : "";
}

int WiFiManager::getRSSI() const {
    return isConnected ? WiFi.RSSI() : 0;
}

String WiFiManager::getMAC() const {
    return WiFi.macAddress();
}