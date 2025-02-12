#ifndef _config_h
#define _config_h

#include "api_keys.h"
#include <ArduinoJson.h>

extern String dbgPrintln(String _str);

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
        return "[Location] name: " + name + ", (lat, lon): (" + String(lat) + ", " + String(lon) +")";
    }
    void print() {
        dbgPrintln(this->to_string());
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
