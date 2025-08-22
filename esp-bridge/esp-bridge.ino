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
 * 4. Seleziona rete WiFi dalla lista
 * 5. "Alexa, scopri dispositivi"
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <fauxmoESP.h>

// Configurazione
const char* ESP_ORIGINALE_IP = "192.168.178.164"; // IP del tuo ESP originale

fauxmoESP fauxmo;
bool wifiConnected = false;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n🚀 ESP32 Bridge per Alexa - Avvio...");
    Serial.println("=======================================");
    
    // Configurazione WiFi interattiva
    configuraWiFi();
    
    if (wifiConnected) {
        inizializzaAlexa();
    }
}

void loop() {
    if (wifiConnected) {
        fauxmo.handle();
    }
    
    // Controlla se c'è input dal serial per riconfigurare WiFi
    if (Serial.available()) {
        String input = Serial.readString();
        input.trim();
        if (input.equalsIgnoreCase("wifi")) {
            Serial.println("\n🔄 Riconfigurazione WiFi...");
            WiFi.disconnect();
            wifiConnected = false;
            delay(1000);
            configuraWiFi();
            if (wifiConnected) {
                inizializzaAlexa();
            }
        }
    }
    
    delay(10);
}

void configuraWiFi() {
    Serial.println("\n📡 Scansione reti WiFi disponibili...");
    
    // Scansiona reti WiFi
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    int reti = WiFi.scanNetworks();
    
    if (reti == 0) {
        Serial.println("❌ Nessuna rete WiFi trovata!");
        return;
    }
    
    Serial.println("\n📋 Reti WiFi disponibili:");
    Serial.println("==========================");
    
    // Mostra liste reti con numerazione
    for (int i = 0; i < reti; i++) {
        Serial.printf("%2d. %-25s", i + 1, WiFi.SSID(i).c_str());
        
        // Mostra intensità segnale
        int rssi = WiFi.RSSI(i);
        if (rssi > -50) Serial.print(" 📶📶📶📶");
        else if (rssi > -60) Serial.print(" 📶📶📶");
        else if (rssi > -70) Serial.print(" 📶📶");
        else Serial.print(" 📶");
        
        // Mostra se è protetta
        if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) {
            Serial.print(" 🔒");
        } else {
            Serial.print(" 🔓");
        }
        
        Serial.printf(" (%d dBm)\n", rssi);
    }
    
    Serial.println("\n🔢 Inserisci il numero della rete (1-" + String(reti) + "):");
    Serial.print("👉 ");
    
    // Aspetta input utente
    while (!Serial.available()) {
        delay(100);
    }
    
    String scelta = Serial.readString();
    scelta.trim();
    int numeroRete = scelta.toInt();
    
    if (numeroRete < 1 || numeroRete > reti) {
        Serial.println("❌ Numero non valido!");
        return;
    }
    
    String ssid = WiFi.SSID(numeroRete - 1);
    Serial.println("✅ Rete selezionata: " + ssid);
    
    // Chiedi password se necessaria
    String password = "";
    if (WiFi.encryptionType(numeroRete - 1) != WIFI_AUTH_OPEN) {
        Serial.println("\n🔐 Inserisci la password:");
        Serial.print("👉 ");
        
        while (!Serial.available()) {
            delay(100);
        }
        
        password = Serial.readString();
        password.trim();
        Serial.println("✅ Password inserita");
    }
    
    // Connetti alla rete
    Serial.println("\n🔄 Connessione in corso...");
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int tentativi = 0;
    while (WiFi.status() != WL_CONNECTED && tentativi < 20) {
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
    } else {
        Serial.println("\n❌ Connessione fallita!");
        Serial.println("💡 Verifica password e riprova");
        wifiConnected = false;
    }
    
    // Pulisce la scansione
    WiFi.scanDelete();
}

void inizializzaAlexa() {
    Serial.println("\n🎤 Inizializzazione Alexa...");
    
    // Configurazione FauxmoESP per Alexa
    fauxmo.createServer(true);
    fauxmo.setPort(80);
    fauxmo.enable(true);
    
    // ✨ Dispositivi virtuali che Alexa vedrà
    fauxmo.addDevice("porta studio");
    fauxmo.addDevice("luce studio");         // Luce interna (pin 33)
    fauxmo.addDevice("luce studio esterno"); // Luce esterna (pin 25)
    fauxmo.addDevice("luce presepe");
    fauxmo.addDevice("luce cantina");
    fauxmo.addDevice("luce camino");
    
    // Gestione comandi Alexa
    fauxmo.onSetState([](unsigned char device_id, const char* device_name, bool state, unsigned char value) {
        Serial.printf("🗣️ Alexa: '%s' -> %s\n", device_name, state ? "ON" : "OFF");
        
        if (state) { // Solo quando Alexa dice "accendi"
            int pin = 0;
            
            // Mappa dispositivo → pin
            if (strcmp(device_name, "porta studio") == 0)          pin = 32;
            else if (strcmp(device_name, "luce studio") == 0)      pin = 33;
            else if (strcmp(device_name, "luce studio esterno") == 0) pin = 25;
            else if (strcmp(device_name, "luce presepe") == 0)     pin = 26;
            else if (strcmp(device_name, "luce cantina") == 0)     pin = 27;
            else if (strcmp(device_name, "luce camino") == 0)      pin = 14;
            
            if (pin > 0) {
                chiamaESPOriginale(pin);
            }
        }
    });
    
    Serial.println("\n🎉 ESP Bridge pronto!");
    Serial.println("=====================");
    Serial.println("📱 Comandi disponibili:");
    Serial.println("   - 'Alexa, scopri dispositivi'");
    Serial.println("   - 'Alexa, accendi luce studio'");
    Serial.println("   - 'Alexa, accendi porta studio'");
    Serial.println("\n⚙️  Comandi Serial:");
    Serial.println("   - Scrivi 'wifi' per riconfigurare rete");
    Serial.println("\n🌐 ESP Bridge online su: " + WiFi.localIP().toString());
}

// Funzione per chiamare l'API del tuo ESP originale
void chiamaESPOriginale(int pin) {
    HTTPClient http;
    String url = "http://" + String(ESP_ORIGINALE_IP) + "/pulsePin?pin=" + String(pin);
    
    Serial.printf("📡 Chiamata ESP: %s\n", url.c_str());
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        Serial.printf("✅ Pin %d attivato con successo\n", pin);
    } else {
        Serial.printf("❌ Errore chiamata ESP (codice: %d)\n", httpCode);
    }
    
    http.end();
}