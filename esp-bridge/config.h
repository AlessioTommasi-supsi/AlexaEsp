#ifndef CONFIG_H
#define CONFIG_H

// ===== CONFIGURAZIONE SISTEMA =====
extern const char* ESP_ORIGINALE_IP;            // IP del tuo ESP originale
extern const char* DEVICE_NAME;                 // Nome dispositivo
extern const char* PREFERENCES_NAMESPACE;       // Namespace EEPROM

// ===== LIMITI SISTEMA =====
const int MAX_DEVICES = 100;                        // Massimo dispositivi supportati
const int MAX_DEVICE_NAME_LENGTH = 50;              // Lunghezza max nome dispositivo
const int MAX_URL_LENGTH = 200;                     // Lunghezza max URL custom
const int MAX_INPUT_LENGTH = 200;                   // Lunghezza max input seriale
const int MAX_UNIQUE_ID_LENGTH = 26;                // Lunghezza max UID FauxmoESP

// ===== CONFIGURAZIONE SERIALE =====
const int SERIAL_BAUD_RATE = 115200;                // Velocità comunicazione seriale

// 🚫 RIMOSSI I TIMEOUT - Ora usiamo await seriale pulito
// const int SERIAL_INPUT_TIMEOUT = 120000;         // RIMOSSO
// const int WIFI_SELECTION_TIMEOUT = 180000;       // RIMOSSO  
// const int DEVICE_CONFIG_TIMEOUT = 60000;         // RIMOSSO

// ===== CONFIGURAZIONE WIFI =====
const int WIFI_CONNECT_TIMEOUT = 20;                // Timeout connessione WiFi (secondi)
const int WIFI_RETRY_TIMEOUT = 15;                  // Timeout retry WiFi salvato (secondi)
const int WIFI_CHECK_INTERVAL = 30000;              // Controllo connessione WiFi (30 sec)
const int WIFI_SCAN_DELAY = 100;                    // Delay dopo scan WiFi
const int WIFI_RECONNECT_ATTEMPTS = 10;             // Tentativi riconnessione

// ===== CONFIGURAZIONE HTTP =====
const int HTTP_TIMEOUT = 5000;                      // Timeout richieste HTTP (ms)
const int HTTP_RESPONSE_MAX_LENGTH = 200;           // Max lunghezza risposta HTTP da mostrare

// ===== CONFIGURAZIONE ALEXA =====
const int FAUXMO_PORT = 80;                         // Porta per FauxmoESP
const int ALEXA_RESTART_DELAY = 500;                // Delay prima restart Alexa

// ===== TIMING SISTEMA =====
const int SETUP_DELAY = 1000;                       // Delay iniziale setup
const int LOOP_DELAY = 10;                          // Delay loop principale
const int MENU_RETURN_DELAY = 1500;                 // Delay prima ritorno menu
const int FAUXMO_DISABLE_DELAY = 200;               // Delay per disable FauxmoESP

// ===== CONFIGURAZIONE PIN ESP32 =====
const int ESP32_MIN_PIN = 1;                        // Pin minimo ESP32
const int ESP32_MAX_PIN = 39;                       // Pin massimo ESP32

// ===== CONFIGURAZIONE MENU =====
const int MENU_MIN_OPTION = 0;                      // Opzione menu minima
const int MENU_MAX_OPTION = 8;                      // Opzione menu massima

#endif