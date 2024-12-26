#ifndef _config_h
#define _config_h

#include "api_keys.h"

#define SLEEP_INTERVAL_MIN 15


struct Location {
    String name = "";
    float lat = 0.0f;
    float lon = 0.0f;

    explicit Location() { }

    explicit Location(String name, String lat, String lon) { 
        this->name = name;
        this->name.trim();
        this->lat = lat.toFloat();
        this->lon = lon.toFloat();
    }
    
    String to_string() {
        return String("[Location] name: ") + name + ", (lat, lon): (" + lat + ", " + lon +")";
    }
    void print() {
        Serial.println("=== DBG: " + this->to_string());
    }
} ;


struct WifiCredentials {
    String ssid = "";
    String pass = "";

    String to_string() {
        return String("[WifiCredentials] ssid: ") + ssid + ", pass: " + pass;
    }
    void print() {
        Serial.println(this->to_string());
    }
} ;


#endif
