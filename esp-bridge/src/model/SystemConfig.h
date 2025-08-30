#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <Arduino.h>

class SystemConfig {
private:
    static SystemConfig* instance;
    SystemConfig() = default;

public:
    static SystemConfig* getInstance() {
        if (instance == nullptr) {
            instance = new SystemConfig();
        }
        return instance;
    }
    
    // Configurazioni dal config.h funzionante
    static const int MAX_DEVICES = 100;
    static const int MAX_DEVICE_NAME_LENGTH = 50;
    static const int MAX_URL_LENGTH = 200;
    static const int MAX_INPUT_LENGTH = 200;
    static const int MAX_UNIQUE_ID_LENGTH = 26;
    
    // Serial Configuration
    static const int SERIAL_BAUD_RATE = 115200;
    
    // WiFi Configuration  
    static const int WIFI_CONNECT_TIMEOUT = 20;
    static const int WIFI_RETRY_TIMEOUT = 15;
    static const int WIFI_CHECK_INTERVAL = 30000;
    static const int WIFI_SCAN_DELAY = 100;
    static const int WIFI_RECONNECT_ATTEMPTS = 10;
    
    // HTTP Configuration
    static const int HTTP_TIMEOUT = 5000;
    static const int HTTP_RESPONSE_MAX_LENGTH = 200;
    
    // Alexa Configuration
    static const int FAUXMO_PORT = 80;
    static const int ALEXA_RESTART_DELAY = 500;
    static const int FAUXMO_DISABLE_DELAY = 200;
    
    // System Timing
    static const int SETUP_DELAY = 1000;
    static const int LOOP_DELAY = 10;
    static const int MENU_RETURN_DELAY = 1500;
    
    // ESP32 Pin Configuration
    static const int ESP32_MIN_PIN = 1;
    static const int ESP32_MAX_PIN = 39;
    
    // Preferences namespace
    static const char* PREFERENCES_NAMESPACE;
    static const char* ESP_ORIGINALE_IP;
    static const char* DEVICE_NAME;
};

#endif