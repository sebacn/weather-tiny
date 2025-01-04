#ifndef _config_h
#define _config_h

#include "api_keys.h"
#include <ArduinoJson.h>

struct BootInfo {
    int BootCount;
    long Timestamp;
    int BatteryPct;
    bool ConfigOk;
    int Mode;
    bool Time_fetched_ok; 
    bool Weather_fetched_ok; 
    bool Aq_fetched_ok; 

    JsonDocument doc;
    
    String to_string() {

        doc["BootCount"] = BootCount;
        doc["Timestamp"] = Timestamp;
        doc["BatteryPct"] = BatteryPct;
        doc["ConfigOk"] = ConfigOk;
        doc["Mode"] = Mode;
        doc["Time_fetched_ok"] = Time_fetched_ok;
        doc["Weather_fetched_ok"] = Weather_fetched_ok;
        doc["Aq_fetched_ok"] = Aq_fetched_ok;

        String ret;

        serializeJson(doc, ret);

        return ret;
    }
    
    void print() {
        Serial.println("=== DBG: " + this->to_string() + "\n");
    }
} ;

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
