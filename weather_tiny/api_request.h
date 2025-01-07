#ifndef _api_request_h
#define _api_request_h

#include <WiFi.h>
#include <time.h>
#include "fmt.h"
#include "config.h"
#include "ca_cert.h"

extern String dbgPrintln(String _str);

struct Request;
typedef bool (*ResponseHandler) (WiFiClient& resp_stream, Request request);

struct Request {
    String server = "";
    String api_key = "";
    String path = "";
    ResponseHandler handler;
    char const *ROOT_CA = nullptr; 

    void make_path() {}
    
    String get_server_path() {
        return this->server + this->path;
    }
} ;


struct TimeZoneDbResponse {
    time_t dt;
    int gmt_offset;
    int dst;

    void print() {
        struct tm* ti;  // timeinfo
        ti = localtime(&dt);

        char buffer[100];
        sprintf(buffer, ": %d:%d %d, %d-%d-%d \n", 
            ti->tm_hour, ti->tm_min, ti->tm_wday, 
            ti->tm_mday, ti->tm_mon+1, 1900+ti->tm_year
        );

        dbgPrintln("Date and time: " + String(buffer));

        /*
        Serial.printf(
            "Date and time:  %d:%d %d, %d-%d-%d \n", 
            ti->tm_hour, ti->tm_min, ti->tm_wday, 
            ti->tm_mday, ti->tm_mon+1, 1900+ti->tm_year
        );
        */
    }
} ;


struct TimeZoneDbRequest: Request {

    explicit TimeZoneDbRequest(): Request() {
        this->server = "api.timezonedb.com";
        this->ROOT_CA = ROOT_CA_TIMEZONEDB;
    }

    explicit TimeZoneDbRequest(String server, String api_key) {
        this->server = server;
        this->api_key = api_key;
    }
    
    void make_path(Location& location) {
        this->path = "/v2.1/get-time-zone?key=" + api_key + "&format=json&by=position&lat=" + String(location.lat) + "&lng=" + String(location.lon);
    }

    explicit TimeZoneDbRequest(const Request& request): Request(request) { }

    
    TimeZoneDbResponse response;
} ;


struct AirQualityResponse {
    int pm25;
    int pm10;
    float no2; 
    float o3;
    float so2;
    float co;
    
    void print() {
        dbgPrintln("Air quality (PM2.5): " + String(pm25) + ", PM10: " + String(pm10) + ", NO2: " + String(no2) + ", O3: " + String(o3) + ", SO2: " + String(so2) + ", CO: " + String(co));
        dbgPrintln("");
    }
} ;


struct AirQualityRequest: Request {
    
    explicit AirQualityRequest(): Request() {
        this->server = "api.waqi.info";
        this->ROOT_CA = ROOT_CA_WAQI;
    } 

    explicit AirQualityRequest(String server, String api_key) {
        this->server = server;
        this->api_key = api_key;
    }
    
    explicit AirQualityRequest(const Request& request): Request(request) { }

    void make_path(Location& location) {
        this->path = "/feed/geo:" + String(location.lat) + ";" + String(location.lon) + "/?token=" + api_key;
    }
    
    AirQualityResponse response;
} ;


struct GeocodingNominatimResponse {
    float lat = 0.0f;
    float lon = 0.0f;
    String label = "";
    
    void print() {
        dbgPrintln("");
        dbgPrintln("City: (" + String(lat) + ", " + String(lon) + ") " + label);
        dbgPrintln("");
    }
} ;


struct GeocodingNominatimRequest: Request {
    String name = "";

    explicit GeocodingNominatimRequest(): Request() {
        this->server = "api.positionstack.com";
        this->ROOT_CA = ROOT_CA_POSITIONSTACK;
    }

    explicit GeocodingNominatimRequest(String server, String name) {
        this->server = server;
        this->name = name;
        make_path();
    }
    
    explicit GeocodingNominatimRequest(const Request& request): Request(request) { }

    void make_path() {
        this->path = "/v1/forward?access_key=" + api_key + "&query=" + name;
    }
    
    GeocodingNominatimResponse response;
} ;


struct WeatherResponseHourly {  // current and hourly
    int date_ts;
    int sunr_ts; // sunrise
    int suns_ts; // sunset
    int temp;  // round from float
    int feel_t;  // round from float
    int max_t; // [daily] round from float
    int min_t; // [daily] round from float
    int pressure; 
    int clouds;
    int wind_bft; // round from float to bft int
    int wind_deg; // round from float
    String icon;
    String descr;
    float snow;
    float rain;
    int pop; // [hourly] probability of percipitation hourly round to int percent
    
    void print() {
        char buffer[150];
        dbgPrintln("Weather currently: " + descr);        
        // 15 * 8 char strings
        sprintf(
            buffer, 
            "%8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s", 
            "date_ts", "sunr_ts", "suns_ts", "temp", "feel_t", 
            "max_t", "min_t", "pressure", "clouds", "wind_bft", 
            "wind_deg", "icon", "snow", "rain", "pop"
        );
        dbgPrintln(String(buffer));
        sprintf(
            buffer, 
            "%8s %8s %8s %8d %8d %8d %8d %8d %8d %8d %8d %8s %8.1f %8.1f %8d",
            ts2HM(date_ts), ts2HM(sunr_ts), ts2HM(suns_ts), 
            temp, feel_t, max_t, min_t,
            pressure, clouds, wind_bft, wind_deg,
            icon, snow, rain, pop
        );
        dbgPrintln(String(buffer));
    }
} ;


struct WeatherResponseDaily {
    int date_ts;
    int max_t;
    int min_t;
    int wind_bft;
    int wind_deg;
    int pop;
    float snow;
    float rain;

    void print() {
        char buffer[100];
        dbgPrintln("Forecast: " + ts2dm(date_ts));
        sprintf(
            buffer, 
            "%8s %8s %8s %8s %8s %8s %8s",
            "max_t", "min_t", "wind_bft", 
            "wind_deg", "pop", "snow", "rain"
        );
        dbgPrintln(String(buffer));
        //Serial.printf("%15s", "");
        dbgPrintln("");
        sprintf(
            buffer, 
            "%8d %8d %8d %8d %8d %8s %8s",
            max_t, min_t, wind_bft, wind_deg, pop,
            snow? "yes" : "no", rain? "yes" : "no"
        );
        dbgPrintln(String(buffer));
    }

} ;


struct WeatherResponseRainHourly {
    int date_ts;
    int pop;
    float feel_t;
    float snow;
    float rain;
    String icon;

    void print() {
        char buffer[60];
        dbgPrintln("Rain: " + ts2date(date_ts));
        sprintf(
            buffer, 
            "%8s %8s %8s %8s %8s",
            "pop", "snow", "rain", "feel", "icon"
        );
        dbgPrintln(String(buffer));
        //Serial.printf("%25s", "");
        dbgPrintln("");
        sprintf(
            buffer, 
            "%8d %8.1f %8.1f %8.1f %8s",
            pop, snow, rain, feel_t, icon
        );
        dbgPrintln(String(buffer));
    }
} ;


struct WeatherRequest: Request {
   
    explicit WeatherRequest(): Request() {
        this->server = "api.openweathermap.org";
        this->ROOT_CA = ROOT_CA_OWM;
    }
    
    explicit WeatherRequest(String server, String api_key) {
        this->server = server;
        this->api_key = api_key;
    }
    
    explicit WeatherRequest(const Request& request): Request(request) { }

    void make_path(Location& location) {
        this->path = "/data/2.5/onecall?lat="+String(location.lat)
            +"&lon="+String(location.lon)
            +"&exclude=minutely,alerts"
            +"&appid="+api_key
            +"&lang="+LANGS[LANG];
    }
    
    WeatherResponseHourly hourly[1];
    WeatherResponseDaily daily[2];
    WeatherResponseRainHourly rain[5];
} ;


String openweather_icons[9] = {
    "01",   // 0 clear sky
    "02",   // 1 few clouds    
    "03",   // 2 scattered clouds
    "04",   // 3 broken clouds
    "09",   // 4 shower rain
    "10",   // 5 rain
    "11",   // 6 thunderstorm
    "13",   // 7 snow
    "50"    // 8 mist
};


#endif
