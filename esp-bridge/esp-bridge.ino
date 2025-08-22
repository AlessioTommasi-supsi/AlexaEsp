/*
 * ESP32 Bridge per Alexa - Controllo Vocale Locale
 * 
 * Questo ESP32 fa da ponte tra Alexa e il tuo ESP originale
 * Utilizza FauxmoESP per emulare dispositivi Philips Hue
 * 
 * Setup:
 * 1. Installa libreria "fauxmoESP" in Arduino IDE
 * 2. Carica su ESP32
 * 3. Apri Serial Monitor (115200 baud)
 * 4. WiFi salvato automaticamente, comandi per gestire dispositivi
 * 5. "Alexa, scopri dispositivi"
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <fauxmoESP.h>
#include <Preferences.h>

// ===== CONFIGURAZIONE COSTANTI =====
const char* ESP_ORIGINALE_IP = "192.168.178.164"; // IP del tuo ESP originale
const int MAX_DEVICES = 100;                      // Massimo dispositivi supportati
const int SERIAL_BAUD_RATE = 115200;             // Velocità comunicazione seriale
const int WIFI_CONNECT_TIMEOUT = 20;             // Timeout connessione WiFi (secondi)
const int WIFI_RETRY_TIMEOUT = 15;               // Timeout retry WiFi salvato (secondi)
const int HTTP_TIMEOUT = 5000;                   // Timeout richieste HTTP (ms)
const int FAUXMO_PORT = 80;                      // Porta per FauxmoESP
const int SETUP_DELAY = 1000;                    // Delay iniziale setup
const int LOOP_DELAY = 10;                       // Delay loop principale
const int WIFI_SCAN_DELAY = 100;                 // Delay dopo scan WiFi
const int SERIAL_INPUT_TIMEOUT = 30000;          // Timeout input seriale (30 sec)

// Stati per macchina a stati
enum SerialState {
    IDLE,
    WAITING_WIFI_SELECTION,
    WAITING_WIFI_PASSWORD,
    WAITING_DEVICE_TYPE,
    WAITING_DEVICE_PIN,
    WAITING_DEVICE_URL,
    WAITING_RESET_CONFIRM
};

// Struttura per dispositivi dinamici
struct Device {
    String name;
    int pin;
    String customUrl;  // Se vuoto, usa pulsePin standard
    bool useCustomUrl;
};

// Variabili globali
fauxmoESP fauxmo;
Preferences preferences;
bool wifiConnected = false;
Device devices[MAX_DEVICES];
int deviceCount = 0;

// Gestione input seriale non-blocking
SerialState currentState = IDLE;
String inputBuffer = "";
unsigned long inputStartTime = 0;
String pendingDeviceName = "";
int selectedWifiIndex = -1;
String selectedWifiSSID = "";

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(SETUP_DELAY);
    Serial.println("\n🚀 ESP32 Bridge per Alexa - Avvio...");
    Serial.println("=======================================");
    
    // Inizializza preferences
    preferences.begin("esp-bridge", false);
    
    // Carica dispositivi salvati
    caricaDispositivi();
    
    // Tenta connessione WiFi salvato
    if (tentaConnessioneSalvata()) {
        wifiConnected = true;
        inizializzaAlexa();
        mostraMenuPrincipale();
    } else {
        // Se fallisce, configurazione interattiva
        avviaConfigurazioneWiFi();
    }
}

void loop() {
    if (wifiConnected) {
        fauxmo.handle();
    }
    
    // Gestisce input seriale non-blocking
    gestisciInputSeriale();
    
    delay(LOOP_DELAY);
}

void gestisciInputSeriale() {
    // Controlla timeout
    if (currentState != IDLE && (millis() - inputStartTime) > SERIAL_INPUT_TIMEOUT) {
        Serial.println("\n⏰ Timeout input. Ritorno al menu principale.");
        resetSerialState();
        mostraMenuPrincipale();
        return;
    }
    
    // Leggi input se disponibile
    if (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                processaInput(inputBuffer);
                inputBuffer = "";
            }
        } else if (c >= 32 && c <= 126) { // Caratteri stampabili
            inputBuffer += c;
            if (inputBuffer.length() > 200) { // Limita lunghezza input
                inputBuffer = "";
                Serial.println("❌ Input troppo lungo");
            }
        }
    }
}

void processaInput(String input) {
    input.trim();
    
    switch (currentState) {
        case IDLE:
            gestisciComandoPrincipale(input);
            break;
            
        case WAITING_WIFI_SELECTION:
            gestisciSelezioneWiFi(input);
            break;
            
        case WAITING_WIFI_PASSWORD:
            gestisciPasswordWiFi(input);
            break;
            
        case WAITING_DEVICE_TYPE:
            gestisciTipoDispositivo(input);
            break;
            
        case WAITING_DEVICE_PIN:
            gestisciPinDispositivo(input);
            break;
            
        case WAITING_DEVICE_URL:
            gestisciUrlDispositivo(input);
            break;
            
        case WAITING_RESET_CONFIRM:
            gestisciConfermaReset(input);
            break;
    }
}

void resetSerialState() {
    currentState = IDLE;
    pendingDeviceName = "";
    selectedWifiIndex = -1;
    selectedWifiSSID = "";
    inputStartTime = 0;
}

void startSerialInput(SerialState newState) {
    currentState = newState;
    inputStartTime = millis();
}

bool tentaConnessioneSalvata() {
    String ssid = preferences.getString("wifi_ssid", "");
    String password = preferences.getString("wifi_pass", "");
    
    if (ssid.length() == 0) {
        Serial.println("💾 Nessun WiFi salvato");
        return false;
    }
    
    Serial.println("💾 Tentativo connessione WiFi salvato: " + ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int tentativi = 0;
    while (WiFi.status() != WL_CONNECTED && tentativi < WIFI_RETRY_TIMEOUT) {
        delay(500);
        Serial.print(".");
        tentativi++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi salvato connesso!");
        Serial.printf("📍 IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        Serial.println("\n❌ Connessione WiFi salvato fallita");
        return false;
    }
}

void salvaCredentialiWiFi(const String& ssid, const String& password) {
    preferences.putString("wifi_ssid", ssid);
    preferences.putString("wifi_pass", password);
    Serial.println("💾 Credenziali WiFi salvate");
}

void caricaDispositivi() {
    deviceCount = preferences.getInt("device_count", 0);
    if (deviceCount > MAX_DEVICES) deviceCount = MAX_DEVICES; // Sicurezza
    
    Serial.printf("💾 Caricati %d dispositivi salvati\n", deviceCount);
    
    for (int i = 0; i < deviceCount; i++) {
        String nameKey = "dev_name_" + String(i);
        String pinKey = "dev_pin_" + String(i);
        String urlKey = "dev_url_" + String(i);
        String customKey = "dev_custom_" + String(i);
        
        devices[i].name = preferences.getString(nameKey.c_str(), "");
        devices[i].pin = preferences.getInt(pinKey.c_str(), 0);
        devices[i].customUrl = preferences.getString(urlKey.c_str(), "");
        devices[i].useCustomUrl = preferences.getBool(customKey.c_str(), false);
        
        if (devices[i].name.length() > 0) {
            Serial.printf("   📱 %s -> %s\n", devices[i].name.c_str(), 
                         devices[i].useCustomUrl ? devices[i].customUrl.c_str() : ("Pin " + String(devices[i].pin)).c_str());
        }
    }
}

void salvaDispositivi() {
    preferences.putInt("device_count", deviceCount);
    
    for (int i = 0; i < deviceCount; i++) {
        String nameKey = "dev_name_" + String(i);
        String pinKey = "dev_pin_" + String(i);
        String urlKey = "dev_url_" + String(i);
        String customKey = "dev_custom_" + String(i);
        
        preferences.putString(nameKey.c_str(), devices[i].name);
        preferences.putInt(pinKey.c_str(), devices[i].pin);
        preferences.putString(urlKey.c_str(), devices[i].customUrl);
        preferences.putBool(customKey.c_str(), devices[i].useCustomUrl);
    }
    Serial.println("💾 Dispositivi salvati");
}

void gestisciComandoPrincipale(const String& input) {
    String cmd = input;
    cmd.toLowerCase();
    
    if (cmd == "wifi") {
        avviaConfigurazioneWiFi();
    }
    else if (cmd.startsWith("add ")) {
        String nome = input.substring(4);
        avviaAggiuntaDispositivo(nome);
    }
    else if (cmd == "list") {
        mostraDispositivi();
    }
    else if (cmd.startsWith("remove ")) {
        String nome = input.substring(7);
        rimuoviDispositivo(nome);
    }
    else if (cmd == "help") {
        mostraHelp();
    }
    else if (cmd == "reset") {
        avviaResetConfigurazione();
    }
    else if (cmd == "status") {
        mostraStatus();
    }
    else {
        Serial.println("❌ Comando non riconosciuto. Scrivi 'help' per aiuto");
    }
}

void avviaConfigurazioneWiFi() {
    Serial.println("\n📡 Scansione reti WiFi disponibili...");
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(WIFI_SCAN_DELAY);
    
    int reti = WiFi.scanNetworks();
    
    if (reti == 0) {
        Serial.println("❌ Nessuna rete WiFi trovata!");
        resetSerialState();
        return;
    }
    
    Serial.println("\n📋 Reti WiFi disponibili:");
    Serial.println("==========================");
    
    for (int i = 0; i < reti; i++) {
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
    
    Serial.println("\n🔢 Inserisci il numero della rete (1-" + String(reti) + ") o 'exit' per annullare:");
    Serial.print("👉 ");
    
    startSerialInput(WAITING_WIFI_SELECTION);
}

void gestisciSelezioneWiFi(const String& input) {
    if (input.equalsIgnoreCase("exit")) {
        Serial.println("❌ Configurazione WiFi annullata");
        resetSerialState();
        mostraMenuPrincipale();
        return;
    }
    
    int numeroRete = input.toInt();
    int reti = WiFi.scanNetworks(false, true); // Non async, mostra hidden
    
    if (numeroRete < 1 || numeroRete > reti) {
        Serial.println("❌ Numero non valido! Riprova:");
        Serial.print("👉 ");
        return;
    }
    
    selectedWifiIndex = numeroRete - 1;
    selectedWifiSSID = WiFi.SSID(selectedWifiIndex);
    Serial.println("✅ Rete selezionata: " + selectedWifiSSID);
    
    if (WiFi.encryptionType(selectedWifiIndex) != WIFI_AUTH_OPEN) {
        Serial.println("\n🔐 Inserisci la password o 'exit' per annullare:");
        Serial.print("👉 ");
        startSerialInput(WAITING_WIFI_PASSWORD);
    } else {
        tentaConnessione(""); // Rete aperta
    }
    
    WiFi.scanDelete();
}

void gestisciPasswordWiFi(const String& input) {
    if (input.equalsIgnoreCase("exit")) {
        Serial.println("❌ Configurazione WiFi annullata");
        resetSerialState();
        mostraMenuPrincipale();
        return;
    }
    
    Serial.println("✅ Password inserita");
    tentaConnessione(input);
}

void tentaConnessione(const String& password) {
    Serial.println("\n🔄 Connessione in corso...");
    WiFi.begin(selectedWifiSSID.c_str(), password.c_str());
    
    int tentativi = 0;
    while (WiFi.status() != WL_CONNECTED && tentativi < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        Serial.print(".");
        tentativi++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi connesso!");
        Serial.println("📍 SSID: " + WiFi.SSID());
        Serial.printf("📍 IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("📍 Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("📶 Intensità: %d dBm\n", WiFi.RSSI());
        
        wifiConnected = true;
        salvaCredentialiWiFi(selectedWifiSSID, password);
        
        if (deviceCount > 0) {
            inizializzaAlexa();
        }
        
        resetSerialState();
        mostraMenuPrincipale();
    } else {
        Serial.println("\n❌ Connessione fallita!");
        Serial.println("💡 Verifica password e riprova");
        resetSerialState();
        mostraMenuPrincipale();
    }
}

void avviaAggiuntaDispositivo(const String& nome) {
    String nomeClean = nome;
    nomeClean.trim();
    
    if (nomeClean.length() == 0) {
        Serial.println("❌ Nome dispositivo vuoto");
        return;
    }
    
    if (deviceCount >= MAX_DEVICES) {
        Serial.println("❌ Limite massimo dispositivi raggiunto (" + String(MAX_DEVICES) + ")");
        return;
    }
    
    // Controlla se esiste già
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].name.equalsIgnoreCase(nomeClean)) {
            Serial.println("❌ Dispositivo già esistente");
            return;
        }
    }
    
    pendingDeviceName = nomeClean;
    Serial.println("\n➕ Aggiunta dispositivo: " + nomeClean);
    Serial.println("🔧 Scegli tipo di controllo:");
    Serial.println("   1. Standard (pulsePin con numero pin)");
    Serial.println("   2. Custom (URL personalizzato)");
    Serial.println("   exit. Annulla");
    Serial.print("👉 Scegli (1/2/exit): ");
    
    startSerialInput(WAITING_DEVICE_TYPE);
}

void gestisciTipoDispositivo(const String& input) {
    if (input.equalsIgnoreCase("exit")) {
        Serial.println("❌ Aggiunta dispositivo annullata");
        resetSerialState();
        mostraMenuPrincipale();
        return;
    }
    
    if (input == "1") {
        Serial.print("📍 Inserisci numero pin (es. 32) o 'exit': ");
        startSerialInput(WAITING_DEVICE_PIN);
    }
    else if (input == "2") {
        Serial.print("🌐 Inserisci URL completo (es. http://192.168.1.100/action?cmd=open) o 'exit': ");
        startSerialInput(WAITING_DEVICE_URL);
    }
    else {
        Serial.println("❌ Scelta non valida! Riprova (1/2/exit):");
        Serial.print("👉 ");
    }
}

void gestisciPinDispositivo(const String& input) {
    if (input.equalsIgnoreCase("exit")) {
        Serial.println("❌ Aggiunta dispositivo annullata");
        resetSerialState();
        mostraMenuPrincipale();
        return;
    }
    
    int pin = input.toInt();
    if (pin <= 0 || pin > 39) { // ESP32 ha pin 0-39
        Serial.println("❌ Pin non valido (1-39)! Riprova:");
        Serial.print("👉 ");
        return;
    }
    
    Device newDevice;
    newDevice.name = pendingDeviceName;
    newDevice.pin = pin;
    newDevice.useCustomUrl = false;
    newDevice.customUrl = "";
    
    devices[deviceCount] = newDevice;
    deviceCount++;
    
    Serial.printf("✅ Dispositivo '%s' configurato per pin %d\n", pendingDeviceName.c_str(), pin);
    
    salvaDispositivi();
    if (wifiConnected) {
        Serial.println("🔄 Riavvio sistema Alexa...");
        inizializzaAlexa();
    }
    
    resetSerialState();
    mostraMenuPrincipale();
}

void gestisciUrlDispositivo(const String& input) {
    if (input.equalsIgnoreCase("exit")) {
        Serial.println("❌ Aggiunta dispositivo annullata");
        resetSerialState();
        mostraMenuPrincipale();
        return;
    }
    
    String url = input;
    url.trim();
    
    if (url.length() == 0 || !url.startsWith("http")) {
        Serial.println("❌ URL non valido (deve iniziare con http)! Riprova:");
        Serial.print("👉 ");
        return;
    }
    
    Device newDevice;
    newDevice.name = pendingDeviceName;
    newDevice.customUrl = url;
    newDevice.useCustomUrl = true;
    newDevice.pin = 0;
    
    devices[deviceCount] = newDevice;
    deviceCount++;
    
    Serial.printf("✅ Dispositivo '%s' configurato per URL: %s\n", pendingDeviceName.c_str(), url.c_str());
    
    salvaDispositivi();
    if (wifiConnected) {
        Serial.println("🔄 Riavvio sistema Alexa...");
        inizializzaAlexa();
    }
    
    resetSerialState();
    mostraMenuPrincipale();
}

void rimuoviDispositivo(const String& nome) {
    String nomeClean = nome;
    nomeClean.trim();
    bool trovato = false;
    
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].name.equalsIgnoreCase(nomeClean)) {
            // Sposta tutti gli elementi successivi indietro
            for (int j = i; j < deviceCount - 1; j++) {
                devices[j] = devices[j + 1];
            }
            deviceCount--;
            trovato = true;
            Serial.println("✅ Dispositivo '" + nomeClean + "' rimosso");
            break;
        }
    }
    
    if (!trovato) {
        Serial.println("❌ Dispositivo non trovato");
        return;
    }
    
    salvaDispositivi();
    if (wifiConnected) {
        Serial.println("🔄 Riavvio sistema Alexa...");
        inizializzaAlexa();
    }
}

void mostraDispositivi() {
    Serial.println("\n📱 Dispositivi configurati:");
    Serial.println("============================");
    
    if (deviceCount == 0) {
        Serial.println("   Nessun dispositivo configurato");
        Serial.println("   Usa 'add nome_dispositivo' per aggiungere");
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

void mostraStatus() {
    Serial.println("\n📊 Status Sistema:");
    Serial.println("==================");
    Serial.printf("WiFi: %s\n", wifiConnected ? "✅ Connesso" : "❌ Disconnesso");
    if (wifiConnected) {
        Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    }
    Serial.printf("Dispositivi: %d/%d\n", deviceCount, MAX_DEVICES);
    Serial.printf("Memoria libera: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("Uptime: %lu secondi\n", millis() / 1000);
}

void mostraHelp() {
    Serial.println("\n📖 Comandi disponibili:");
    Serial.println("========================");
    Serial.println("wifi                 - Riconfigura WiFi");
    Serial.println("add nome_dispositivo - Aggiungi nuovo dispositivo");
    Serial.println("remove nome          - Rimuovi dispositivo");
    Serial.println("list                 - Mostra tutti i dispositivi");
    Serial.println("status               - Mostra status sistema");
    Serial.println("reset                - Reset completo configurazione");
    Serial.println("help                 - Mostra questo aiuto");
    Serial.println("\n📱 Comandi Alexa:");
    Serial.println("'Alexa, scopri dispositivi'");
    Serial.println("'Alexa, accendi [nome_dispositivo]'");
    Serial.println("\n💡 Nota: timeout input 30 secondi, 'exit' per annullare");
}

void mostraMenuPrincipale() {
    Serial.println("\n🏠 ESP32 Bridge - Menu Principale");
    Serial.println("=================================");
    Serial.println("Scrivi un comando o 'help' per aiuto");
    Serial.print("👉 ");
}

void avviaResetConfigurazione() {
    Serial.println("⚠️  RESET completo configurazione!");
    Serial.println("Questo cancellerà WiFi e tutti i dispositivi.");
    Serial.print("Confermi? (y/N/exit): ");
    startSerialInput(WAITING_RESET_CONFIRM);
}

void gestisciConfermaReset(const String& input) {
    if (input.equalsIgnoreCase("y")) {
        preferences.clear();
        deviceCount = 0;
        WiFi.disconnect();
        wifiConnected = false;
        Serial.println("✅ Reset completato");
        Serial.println("🔄 Riavvia ESP per riconfigurare");
        resetSerialState();
    } else {
        Serial.println("❌ Reset annullato");
        resetSerialState();
        mostraMenuPrincipale();
    }
}

void inizializzaAlexa() {
    Serial.println("\n🎤 Inizializzazione Alexa...");
    
    // Rimuovi tutti i dispositivi esistenti
    fauxmo.enable(false);
    delay(100);
    
    // Configurazione FauxmoESP per Alexa
    fauxmo.createServer(true);
    fauxmo.setPort(FAUXMO_PORT);
    fauxmo.enable(true);
    
    // Aggiungi dispositivi dinamici
    for (int i = 0; i < deviceCount; i++) {
        fauxmo.addDevice(devices[i].name.c_str());
        Serial.printf("   ➕ %s\n", devices[i].name.c_str());
    }
    
    // Gestione comandi Alexa
    fauxmo.onSetState([](unsigned char device_id, const char* device_name, bool state, unsigned char value) {
        Serial.printf("🗣️ Alexa: '%s' -> %s\n", device_name, state ? "ON" : "OFF");
        
        if (state) { // Solo quando Alexa dice "accendi"
            // Trova il dispositivo
            for (int i = 0; i < deviceCount; i++) {
                if (devices[i].name.equalsIgnoreCase(device_name)) {
                    if (devices[i].useCustomUrl) {
                        chiamaURLCustom(devices[i].customUrl);
                    } else {
                        chiamaESPOriginale(devices[i].pin);
                    }
                    break;
                }
            }
        }
    });
    
    Serial.println("\n🎉 ESP Bridge pronto!");
    Serial.println("=====================");
    Serial.printf("📱 %d dispositivi configurati\n", deviceCount);
    Serial.println("📱 Comandi disponibili:");
    Serial.println("   - 'Alexa, scopri dispositivi'");
    for (int i = 0; i < deviceCount && i < 3; i++) {
        Serial.println("   - 'Alexa, accendi " + devices[i].name + "'");
    }
    if (deviceCount > 3) {
        Serial.println("   - ... e altri " + String(deviceCount - 3) + " dispositivi");
    }
    Serial.println("\n🌐 ESP Bridge online su: " + WiFi.localIP().toString());
}

// Funzione per chiamare l'API del tuo ESP originale (standard)
void chiamaESPOriginale(int pin) {
    HTTPClient http;
    String url = "http://" + String(ESP_ORIGINALE_IP) + "/pulsePin?pin=" + String(pin);
    
    Serial.printf("📡 Chiamata ESP: %s\n", url.c_str());
    
    http.begin(url);
    http.setTimeout(HTTP_TIMEOUT);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        Serial.printf("✅ Pin %d attivato con successo\n", pin);
    } else {
        Serial.printf("❌ Errore chiamata ESP (codice: %d)\n", httpCode);
    }
    
    http.end();
}

// Funzione per chiamare URL custom
void chiamaURLCustom(const String& url) {
    HTTPClient http;
    
    Serial.printf("📡 Chiamata Custom: %s\n", url.c_str());
    
    http.begin(url);
    http.setTimeout(HTTP_TIMEOUT);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String response = http.getString();
        Serial.println("✅ Richiesta custom eseguita");
        if (response.length() > 0 && response.length() < 200) {
            Serial.println("📥 Risposta: " + response);
        }
    } else {
        Serial.printf("❌ Errore chiamata custom (codice: %d)\n", httpCode);
    }
    
    http.end();
}