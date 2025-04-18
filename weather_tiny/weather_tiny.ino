// ----------------------------------
// DISPLAY --------------------------
// ----------------------------------
#include <SPI.h>
#include <GxEPD.h>

// #include <GxGDE0213B72B/GxGDE0213B72B.h> // 2.13" b/w
#include <GxDEPG0213BN/GxDEPG0213BN.h>  //V2.3.1 2.13" b/w newer panel

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

// ----------------------------------
// LIBS -----------------------------
// ----------------------------------
#define ARDUINOJSON_USE_DOUBLE 1

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <DNSServer.h>
#include <rom/rtc.h> 
#include <Preferences.h>
#include <WiFiManager.h>
#include <RTClib.h>

#define ADC_PIN 35
#define WAKE_BTN_PIN 39

// ----------------------------------
// FONTS ----------------------------
// ----------------------------------

#include "fonts/Monofonto10pt.h"
#include "fonts/Monofonto12pt.h"
#include "fonts/Monofonto18pt.h"
#include "fonts/MeteoCons8pt.h"
#include "fonts/MeteoCons10pt.h"
#include "fonts/Cousine6pt.h"

// ----------------------------------
// LOCAL FILES AND DECLARATIONS -----
// ----------------------------------

#include "fmt.h"
#include "i18n.h"
#include "config.h"
#include "units.h"
#include "api_keys.h"
ApiKeys apiKeys;
#include "api_request.h"
#include "display.h"
#include "view.h"
#include "loginfo.h"

/*
Replace in the sdkconfig.h

From:
#define CONFIG_ARDUINO_LOOP_STACK_SIZE 8192

To:
#if defined(CONFIG_ARDUINO_LOOP_STACK_SIZE_OVERRIDE)
    #define CONFIG_ARDUINO_LOOP_STACK_SIZE CONFIG_ARDUINO_LOOP_STACK_SIZE_OVERRIDE
#elif
    #define CONFIG_ARDUINO_LOOP_STACK_SIZE 8192
#endif
*/

#if CONFIG_ARDUINO_LOOP_STACK_SIZE <= 8192
#error "Please increase ARDUINO_LOOP_STACK_SIZE to 16384 !!! (.platformio\packages\framework-arduinoespressif32\tools\sdk\esp32\dio_qspi\include\sdkconfig.h)"
#endif

#define MEMORY_ID "mem"
#define LOC_MEMORY_ID "loc"

#define NOT_SET_MODE 0
#define CONFIG_MODE 1
#define VALIDATING_MODE 2
#define OPERATING_MODE 3

// http server for obtaining configuration
DNSServer dnsServer;
//AsyncWebServer server(80);
Preferences preferences;
//flag for saving data
bool shouldSaveConfig = false;
bool configOk = false;

//int cached_MODE = 0;
int curr_loc = 0;
int SLEEP_INTERVAL_MIN;
int wakeupHour;  // Wakeup after 07:00 to save battery power
int sleepHour;

struct WeatherRequest weather_request;
struct AirQualityRequest airquality_request;
struct GeocodingNominatimRequest location_request;
struct TimeZoneDbRequest datetime_request;

int location_cnt = 0;
struct Location location[2];
struct WifiCredentials wifi;
struct View view;
int boot_count;

int get_mode();
JsonDocument deserialize(WiFiClient& resp_stream, bool is_embeded=false);

// ----------------------------------
// FUNCTION DEFINITIONS -------------
// ----------------------------------

template<typename T>
T value_or_default(JsonObject jobj, String key, T default_value) {
    if (!jobj[key].is<T>()) {
        return default_value;
    } else {
        return jobj[key];
    }
}


template<typename T>
T nested_value_or_default(JsonObject parent_jobj, String key, String nested_key, T default_value) {
    if (!parent_jobj[key].is<T>()) {
        return default_value;
    } else {
        return parent_jobj[key][nested_key];
    }
}

void print_pt()
{
  printf("\n\nESP32 Partition table:\n\n");

  printf("| Type | Sub |  Offset  |   Size   | Size (b) |       Label      |\n");
  printf("| ---- | --- | -------- | -------- | -------- | ---------------- |\n");
  
  esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (pi != NULL) {
    do {
      const esp_partition_t* p = esp_partition_get(pi);
      printf("|  %02x  | %02x  | 0x%06X | 0x%06X | %8d | %-16s |\r\n", 
        p->type, p->subtype, p->address, p->size, p->size, p->label);
    } while (pi = (esp_partition_next(pi)));
  }

  uint32_t program_size = ESP.getSketchSize();
  uint32_t free_size = ESP.getFlashChipSize();
  uint32_t psram_size = ESP.getPsramSize();
  uint32_t free_sketch_space = ESP.getFreeSketchSpace();

  Serial.println("");
  infoPrintln("Build date time: " + String(__DATE__) + " " + __TIME__);
  infoPrintln("Sketch size: " + String(program_size));
  infoPrintln("Free sketch space: " + String(free_sketch_space));
  infoPrintln("Flash chip size: " + String(free_size));
  infoPrintln("Psram size: " + String(psram_size));
  infoPrintln("Stack size: " + String(CONFIG_ARDUINO_LOOP_STACK_SIZE));
  infoPrintln("uxTaskGetStackHighWaterMark: " + String(uxTaskGetStackHighWaterMark(NULL)) + "\n\n");    
}

String dbgPrintln(String _str) {
    #if defined(PROJ_DEBUG_ENABLE)
    Serial.printf("%.03f ",millis()/1000.0f); Serial.println("[D] " + _str);
    #endif
    return _str + '\n';
}

String infoPrintln(String _str) {
    Serial.printf("%.03f ",millis()/1000.0f); Serial.println("[I] " + _str);
    return _str + '\n';
}

void collectAndWriteLog(int mode, bool is_time_fetched, bool is_weather_fetched, bool is_aq_fetched)
{
    dbgPrintln("InfluxDBClient: collectAndWriteLog");

    logInfo.Mode = mode;
    logInfo.ConfigOk = configOk;
    logInfo.BootCount = boot_count;
    logInfo.UTCTimestamp = is_time_fetched? (datetime_request.response.dt - datetime_request.response.gmt_offset) : 0;
    logInfo.BatteryPct = get_battery_percent(analogRead(ADC_PIN));
    logInfo.TimeFetchOk = is_time_fetched;
    logInfo.WeatherFetchOk = is_weather_fetched;
    logInfo.AQIFetchOk = is_aq_fetched;

    writeLogInfo();
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  dbgPrintln("Should save config");
  shouldSaveConfig = true;
}

void update_header_view(View& view, bool data_updated) {
    view.location = location[curr_loc].name.substring(0,10);
    view.battery_percent = get_battery_percent(analogRead(ADC_PIN));
    int percent_display = view.battery_percent;
    
    if (percent_display > 100) {
        percent_display = 100;
    }
    view.battery_percent_display = String(percent_display) + "%";
    view.datetime = header_datetime(&datetime_request.response.dt, data_updated);

    dbgPrintln("Date time: " + view.datetime);
}


void update_air_quality_view(View& view, bool data_updated) {
    if (!data_updated) {
        return;
    }
    view.aq_pm25 = left_pad(String(airquality_request.response.pm25), 3);
    view.aq_pm25_unit = "PM2.5";
}


void update_weather_view(View& view, bool data_updated) {
    if (!data_updated) {
        return;
    }
    view.weather_icon = String(icon2meteo_font(weather_request.hourly[0].icon));
    
    String descr = weather_request.hourly[0].descr;
    view.weather_desc = capitalize(descr);
    
    view.temp_curr = left_pad(String(weather_request.hourly[0].temp), 3);
    view.temp_unit = '*';  // celsius
    view.temp_high = "Hi" + left_pad(String(weather_request.hourly[0].max_t), 3);
    view.temp_low = "Lo" + left_pad(String(weather_request.hourly[0].min_t), 3);
    view.temp_feel = "Fl" + left_pad(String(weather_request.hourly[0].feel_t), 3);

    view.pressure = left_pad(String(weather_request.hourly[0].pressure), 4);
    view.pressure_unit = "hPa";

    view.wind = left_pad(String(weather_request.hourly[0].wind_bft), 2);
    view.wind_deg = weather_request.hourly[0].wind_deg;

    for (int i = 0; i < PERCIP_SIZE; i++) {
        view.percip_time[i] = ts2H(weather_request.rain[i].date_ts + datetime_request.response.gmt_offset);
        view.percip_icon[i] = String(icon2meteo_font(weather_request.rain[i].icon));

        float cumulative_percip = weather_request.rain[i].snow + weather_request.rain[i].rain;
        if (cumulative_percip > 0) {
            view.percic_pop[i] = left_pad(fmt_2f1(cumulative_percip), 4);
        } else {
            view.percic_pop[i] = left_pad(String(min(weather_request.rain[i].pop, 99)) + "%", 4);
        }

        // temp TODO rename from percip
        view.percip[i] = fmt_2f1(weather_request.rain[i].feel_t);
    }
}


void update_location(GeocodingNominatimResponse& location_resp, JsonObject& jobj) {
    location_resp.lat = jobj["data"][0]["latitude"].as<float>();
    location_resp.lon = jobj["data"][0]["longitude"].as<float>();
    location_resp.label = String(jobj["data"][0]["label"].as<String>()).substring(0, 25); //char*
}


void update_datetime(TimeZoneDbResponse& datetime_resp, JsonObject& jobj) {
    datetime_resp.dt = jobj["timestamp"].as<int>();
    datetime_resp.gmt_offset = jobj["gmtOffset"].as<int>();
    datetime_resp.dst = jobj["dst"].as<int>();
}


void update_current_weather(WeatherResponseHourly& hourly, JsonObject& root) {
    hourly.date_ts = root["current"]["dt"].as<int>();
    hourly.sunr_ts = root["current"]["sunrise"].as<int>();
    hourly.suns_ts = root["current"]["sunset"].as<int>();
    hourly.temp = kelv2cels(root["current"]["temp"].as<float>());
    hourly.feel_t = kelv2cels(root["current"]["feels_like"].as<float>());
    hourly.max_t = kelv2cels(root["daily"][0]["temp"]["max"].as<float>());
    hourly.min_t = kelv2cels(root["daily"][0]["temp"]["min"].as<float>());
    hourly.pressure = root["current"]["pressure"].as<int>();
    hourly.clouds = root["current"]["clouds"].as<int>();
    hourly.wind_bft = wind_ms2bft(root["current"]["wind_speed"].as<float>());
    hourly.wind_deg = root["current"]["wind_deg"].as<int>();
    hourly.icon = String(root["current"]["weather"][0]["icon"].as<String>()).substring(0, 2); //char*
    hourly.descr = root["current"]["weather"][0]["description"].as<String>(); //char*
    hourly.pop = round(root["hourly"][1]["pop"].as<float>() * 100);
    hourly.snow = value_or_default(root["current"], "snow", 0.0f);
    hourly.rain = value_or_default(root["current"], "rain", 0.0f);
}


void update_forecast_weather(WeatherResponseDaily& daily, JsonObject& root, const int day_offset) {
    daily.date_ts = root["daily"][day_offset]["dt"].as<int>();
    daily.max_t = kelv2cels(root["daily"][day_offset]["temp"]["max"].as<float>());
    daily.min_t = kelv2cels(root["daily"][day_offset]["temp"]["min"].as<float>());
    daily.wind_bft = wind_ms2bft(root["daily"][day_offset]["wind_speed"].as<float>());
    daily.wind_deg = root["daily"][day_offset]["wind_deg"].as<int>();
    daily.pop = round(root["daily"][day_offset]["pop"].as<float>() * 100);
    daily.snow = value_or_default(root["daily"][day_offset], "snow", 0.0f);
    daily.rain = value_or_default(root["daily"][day_offset], "rain", 0.0f);
}


void update_percip_forecast(WeatherResponseRainHourly& percip, JsonObject& root, const int hour_offset) {
    percip.date_ts = root["hourly"][hour_offset]["dt"].as<int>();
    percip.pop = round(root["hourly"][hour_offset]["pop"].as<float>() * 100);
    percip.snow = nested_value_or_default(root["hourly"][hour_offset], "snow", "1h", 0.0f);
    percip.rain = nested_value_or_default(root["hourly"][hour_offset], "rain", "1h", 0.0f);
    percip.feel_t = kelv2cels1(root["hourly"][hour_offset]["feels_like"].as<float>());
    percip.icon = String(root["hourly"][hour_offset]["weather"][0]["icon"].as<String>()).substring(0, 2); //char*
}


bool location_handler(WiFiClient& resp_stream, Request request) {
    //const int json_size = 20 * 1024;
    JsonDocument doc = deserialize(resp_stream, true);
    JsonObject api_resp = doc.as<JsonObject>();

    if (api_resp.isNull()) {
        return false;
    }
    location_request = GeocodingNominatimRequest(request);
    GeocodingNominatimResponse& location_resp = location_request.response;
    dbgPrintln("Geocoding...");
    update_location(location_resp, api_resp);
    location_resp.print();
    return true;
}


bool datetime_handler(WiFiClient& resp_stream, Request request) {
    //const int json_size = 10 * 1024;
    JsonDocument doc = deserialize(resp_stream, true);
    JsonObject api_resp = doc.as<JsonObject>();

    if (api_resp.isNull()) {
        return false;
    }
    datetime_request = TimeZoneDbRequest(request);
    TimeZoneDbResponse& datetime_response = datetime_request.response;
    update_datetime(datetime_response, api_resp);
    datetime_response.print();
    return true;
}
    

bool weather_handler(WiFiClient& resp_stream, Request request) {
    //const int json_size = 35 * 1024;
    JsonDocument doc = deserialize(resp_stream);
    JsonObject api_resp = doc.as<JsonObject>();

    if (api_resp.isNull()) {
        return false;
    }
    weather_request = WeatherRequest(request);
    WeatherResponseHourly& hourly = weather_request.hourly[0];
    WeatherResponseDaily& next_day = weather_request.daily[0];
    WeatherResponseDaily& second_next_day = weather_request.daily[1];

    update_current_weather(hourly, api_resp);
    hourly.print();
    update_forecast_weather(next_day, api_resp, 1);
    next_day.print();
    update_forecast_weather(second_next_day, api_resp, 2);
    second_next_day.print();

    for (int hour = 0; hour < 5; hour++) {
        int offset = hour + 1;
        update_percip_forecast(weather_request.rain[hour], api_resp, offset);
        weather_request.rain[hour].print();
    }
    return true;
}


bool air_quality_handler(WiFiClient& resp_stream, Request request) {
    const int json_size = 6 * 1024;
    JsonDocument doc = deserialize(resp_stream, json_size);
    JsonObject api_resp = doc.as<JsonObject>();
    
    if (api_resp.isNull()) {
        return false;
    }
    if (!String(api_resp["status"].as<String>()).equals("ok")) { //char*
        return false;
    }
    airquality_request = AirQualityRequest(request);

    if (api_resp["data"]["iaqi"]["pm25"]["v"].is<int>()) {
        airquality_request.response.pm25 = api_resp["data"]["iaqi"]["pm25"]["v"].as<int>();
    } else if (api_resp["data"]["forecast"]["daily"]["pm25"][0]["max"].is<int>()) {
        airquality_request.response.pm25 = api_resp["data"]["forecast"]["daily"]["pm25"][0]["max"].as<int>();
    }

    if (api_resp["data"]["iaqi"]["pm10"]["v"].is<int>()) {
        airquality_request.response.pm10 = api_resp["data"]["iaqi"]["pm10"]["v"].as<int>();
    } else if (api_resp["data"]["forecast"]["daily"]["pm10"][0]["max"].is<int>()) {
        airquality_request.response.pm10 = api_resp["data"]["forecast"]["daily"]["pm10"][0]["max"].as<int>();
    }
    
    if (api_resp["data"]["iaqi"]["no2"]["v"].is<float>()) {
        airquality_request.response.no2 = api_resp["data"]["iaqi"]["no2"]["v"].as<float>();
    }

    if (api_resp["data"]["iaqi"]["o3"]["v"].is<float>()) {
        airquality_request.response.o3 = api_resp["data"]["iaqi"]["o3"]["v"].as<float>();
    }

    if (api_resp["data"]["iaqi"]["so2"]["v"].is<float>()) {
        airquality_request.response.so2 = api_resp["data"]["iaqi"]["so2"]["v"].as<float>();
    }

    if (api_resp["data"]["iaqi"]["co"]["v"].is<float>()) {
        airquality_request.response.co = api_resp["data"]["iaqi"]["co"]["v"].as<float>();
    }

/*
    if (api_resp["data"]["iaqi"].containsKey("pm25")) {
        airquality_request.response.pm25 = api_resp["data"]["iaqi"]["pm25"]["v"].as<int>();
    } else if (api_resp["data"]["forecast"]["daily"].containsKey("pm25")) {
        airquality_request.response.pm25 = api_resp["data"]["forecast"]["daily"]["pm25"][0]["max"].as<int>();
    }
    */
    airquality_request.response.print();
    
    return true;
}


JsonDocument deserialize(WiFiClient& resp_stream, bool is_embeded) {
    // https://arduinojson.org/v6/assistant/
    dbgPrintln("\nDeserializing json...");
    JsonDocument doc;
    DeserializationError error;
    
    if (is_embeded) {
        String stream_as_string = resp_stream.readString();
        int begin = stream_as_string.indexOf('{');
        int end = stream_as_string.lastIndexOf('}');
        dbgPrintln("Embeded json algorithm obtained document...");
        String trimmed_json = stream_as_string.substring(begin, end+1);
        dbgPrintln("Json size: " + String(trimmed_json.length()) + " bytes...");
        dbgPrintln(trimmed_json);
        error = deserializeJson(doc, trimmed_json);
    } else {
        error = deserializeJson(doc, resp_stream);
    }
    if (error) {
        dbgPrintln(F("deserialization error:"));
        dbgPrintln(error.c_str());
    } else {
        dbgPrintln("deserialized.");
    }
    dbgPrintln("");
    return doc;
}


int get_battery_percent(int adc_value) {
    float voltage = adc_value / 4095.0 * 7.5;
    dbgPrintln("Battery voltage ~" + String(voltage) + "V");
    if (voltage > 4.35) {
        return 101;  // charging / DC powered
    }
    if (voltage > 4.2) {
        return 100;
    } 
    if (voltage < 3.3) {
        return 1;
    }
    int percent = (int)((voltage - 3.3)/(4.1 - 3.3)*99) + 1;
    dbgPrintln("Battery percent: " + String(percent) + "%");
    return percent;
}


bool http_request_data(WiFiClientSecure& client, Request request, unsigned int retry=3) {
    
    bool ret_val = false;

    while(!ret_val && retry--) {
        ret_val = true;
        client.stop();
        HTTPClient http;
        infoPrintln("HTTP connecting to " + request.server + " [retry left: " + String(retry) + "]"); //request.server.c_str(), request.path.c_str(), String(retry).c_str());
        client.setCACert(request.ROOT_CA);
        http.begin(client, request.server, 443, request.path);
        int http_code = http.GET();
        
        if(http_code == HTTP_CODE_OK) {
            infoPrintln("HTTP connection OK");
            if (!request.handler(http.getStream(), request)) {
                ret_val = false;
            }
        } else {
            infoPrintln("HTTP connection failed " + String(http_code) + ", error: " + http.errorToString(http_code) + ", path: " + request.path); // String(http_code).c_str(), http.errorToString(http_code).c_str());
            ret_val = false;
        }
        client.stop();
        http.end();
    }
    return ret_val;
}


bool connect_to_wifi(unsigned int retry=5) {

    int wifi_conn_status = WiFi.status();
    
    dbgPrintln("WiFi Connecting to: " + wifi.ssid);

    if (wifi_conn_status == WL_CONNECTED)
    {
        dbgPrintln("WiFi already connected, exit..");
        return true;
    }
    
    //WL_IDLE_STATUS;
    WiFi.mode(WIFI_STA); // Access Point mode off
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);

    while(wifi_conn_status != WL_CONNECTED && retry--) {
        dbgPrintln("Connecting to: " + wifi.ssid + " [retry left: " + retry +"]");
        unsigned long start = millis();
        wifi_conn_status = WiFi.begin(wifi.ssid.c_str(), wifi.pass.c_str());
        
        while (true) {
            if (millis() > start + 10000) { // 10s
                break;
            }
            delay(100);
    
            wifi_conn_status = WiFi.status();
            
            if (wifi_conn_status == WL_CONNECTED) {
                dbgPrintln("Wifi connected. IP: " + WiFi.localIP().toString());    
                break;
            } else if(wifi_conn_status == WL_CONNECT_FAILED) {
                dbgPrintln("Wifi failed to connect.");
                break;
            }
        }
        if (wifi_conn_status == WL_CONNECTED) {
            return true;
        }
        delay(2000); // 2sec
    }
    return false;
}

String print_reset_reason(RESET_REASON reason) {
    String ret = "";
    switch ( reason) {
        case 1 : ret = "POWERON_RESET"; break;
        case 3 : ret = "SW_RESET"; break;
        case 4 : ret = "OWDT_RESET"; break;
        case 5 : ret = "DEEPSLEEP_RESET"; break;
        case 6 : ret = "SDIO_RESET"; break; 
        case 7 : ret = "TG0WDT_SYS_RESET"; break;
        case 8 : ret = "TG1WDT_SYS_RESET"; break;
        case 9 : ret = "RTCWDT_SYS_RESET"; break;
        case 10 : ret = "INTRUSION_RESET"; break;
        case 11 : ret = "TGWDT_CPU_RESET"; break;
        case 12 : ret = "SW_CPU_RESET"; break;
        case 13 : ret = "RTCWDT_CPU_RESET"; break;
        case 14 : ret = "EXT_CPU_RESET"; break;
        case 15 : ret = "RTCWDT_BROWN_OUT_RESET"; break;
        case 16 : ret = "RTCWDT_RTC_RESET"; break;
        default : ret = "UNKNOWN";
    }
    return ret;
}


void wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    dbgPrintln("CPU0 reset reason: " + print_reset_reason(rtc_get_reset_reason(0)));
    dbgPrintln("CPU1 reset reason: " + print_reset_reason(rtc_get_reset_reason(1)));
    dbgPrintln("");
    
    dbgPrintln("Location variable: " + String(curr_loc));

    switch(wakeup_reason){        
        
        case ESP_SLEEP_WAKEUP_EXT0 : 
            dbgPrintln("Wakeup by ext signal RTC_IO -> GPIO39"); 
            if (get_mode() == OPERATING_MODE) {
                // Toggle between 2 screens caused by button press WAKE_BTN_PIN
                if (location_cnt > 1) {
                    curr_loc = (curr_loc+1) % 2;
                    // save location
                    save_location_to_memory(curr_loc);
                }
            }        
            break;
            
        case ESP_SLEEP_WAKEUP_EXT1 : 
            dbgPrintln("Wakeup by ext signal RTC_CNTL -> GPIO34"); 
            set_mode_and_reboot(CONFIG_MODE);
            break;
            
        case ESP_SLEEP_WAKEUP_TIMER : dbgPrintln("Wakeup by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : dbgPrintln("Wakeup by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP : dbgPrintln("Wakeup by ULP program"); break;
        default : dbgPrintln("Wakeup not caused by deep sleep: " + String(wakeup_reason)); 
            if (rtc_get_reset_reason(0) == POWERON_RESET && rtc_get_reset_reason(1) == EXT_CPU_RESET)
            {
                set_mode_and_reboot(CONFIG_MODE);
            }
        break;
    }
}

void enable_timed_sleep(int interval_minutes) {
    // sleep and wake up round minutes, ex every 15 mins
    // will wake up at 7:15, 7:30, 7:45 etc.
    dbgPrintln("enable_timed_sleep (MIN): " + String(interval_minutes));

    DateTime curTime = DateTime(datetime_request.response.dt);
    DateTime newTime;
    char date_format[] = "YYYY.MM.DD:hh.mm.ss";
    char date_format2[] = "YYYY.MM.DD:hh.mm.ss";

    int idx = (curTime.minute()/interval_minutes) + 1;

    if (interval_minutes * idx >= 60)
    {
        newTime = curTime + TimeSpan(0, 0, 60 - curTime.minute(), 0); //add time to xx.00.00
    }
    else
    {        
        newTime = curTime + TimeSpan(0, 0, interval_minutes * idx - curTime.minute(), 0);
    }

    if (newTime.second() > 0)
    {
        // set time to xx.xx.15
        newTime = newTime - TimeSpan(0, 0, 0, newTime.second()); 
        newTime = newTime + TimeSpan(0, 0, 0, 15); // add 15 sec to not wakeUp before expected min
    }

    if ((wakeupHour > 0 || sleepHour > 0) && wakeupHour < 24 && sleepHour < 24) // sleep/wake hours must be in range 0-23
    {
        int addHrs = 0;
        int loopCnt = 0;
        for (int idx = sleepHour; idx != wakeupHour; idx++)
        {
            if (idx == 24)
            {
                idx = 0;
            }

            if (addHrs > 0 
            || (newTime.hour() == idx && idx == sleepHour && newTime.minute() > 0) //for sleep hour start counting when minutes > 0
            || (newTime.hour() == idx && idx != sleepHour ) ) 
            {                
                addHrs++;        
            } 

            loopCnt++;
            if (loopCnt > 25) //exit loop
            {
                addHrs = 0;
                break;
            }
        }

        if (addHrs > 0) 
        {
            newTime = newTime + TimeSpan(0, addHrs, 0, 0); // add sleep hours
            newTime = newTime - TimeSpan(0, 0, newTime.minute(), 0); // reset minutes to 0    
        }
    }
   
    TimeSpan ts = (newTime - curTime);
    int sleep_time_seconds = ts.totalseconds();
    
    infoPrintln("DateTime Curreent :" + String(curTime.toString(date_format)));
    infoPrintln("DateTime WakeUp at:" + String(newTime.toString(date_format2)));
    char buffer[150];
    sprintf(buffer, "%d hours, %d minutes and %d seconds (total sec: %d)\n", ts.hours(), ts.minutes(), ts.seconds(), sleep_time_seconds);
    infoPrintln("DateTime Sleep for " + String(buffer));

    uint64_t sleep_time_micro_sec = sleep_time_seconds;
    sleep_time_micro_sec = sleep_time_micro_sec * 1000 * 1000;

    esp_sleep_enable_timer_wakeup(sleep_time_micro_sec);
}


void begin_deep_sleep() {

    dbgPrintln("Start begin_deep_sleep");

#ifdef BUILTIN_LED
    pinMode(BUILTIN_LED, INPUT); // If it's On, turn it off and some boards use GPIO-5 for SPI-SS, which remains low after screen use
    digitalWrite(BUILTIN_LED, HIGH);
#endif

    display.powerDown();
    disconnect_from_wifi();
    
#ifdef WAKE_BTN_PIN
    //  https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
    esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_BTN_PIN, LOW);
#endif

    // config triggering button
#define BUTTON_PIN_BITMASK 0x0400000000 // pin 34
    esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);

    // this should lower power consumtion
    // https://www.reddit.com/r/esp32/comments/idinjr/36ma_deep_sleep_in_an_eink_ttgo_t5_v23/
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
    gpio_deep_sleep_hold_en();

    // start sleep
    esp_deep_sleep_start();

    dbgPrintln("End begin_deep_sleep");
}


void disconnect_from_wifi() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}

void read_config_from_memory() {

    dbgPrintln("Read config from memory...");
    preferences.begin(MEMORY_ID, true);  // first param true means 'read only'

    wifi.ssid = preferences.getString("ssid");
    wifi.pass = preferences.getString("pass");

    apiKeys.POSITIONSTACK_KEY = preferences.getString("POSSTACK_KEY");
    apiKeys.WAQI_KEY = preferences.getString("WAQI_KEY");
    apiKeys.OPENWEATHER_KEY = preferences.getString("OPENWEATHER_KEY");
    apiKeys.TIMEZDB_KEY = preferences.getString("TIMEZDB_KEY");

    dbgPrintln("POSITIONSTACK_KEY: " + apiKeys.POSITIONSTACK_KEY);
    dbgPrintln("WAQI_KEY: "          + apiKeys.WAQI_KEY);
    dbgPrintln("OPENWEATHER_KEY: "   + apiKeys.OPENWEATHER_KEY);
    dbgPrintln("TIMEZDB_KEY: "       + apiKeys.TIMEZDB_KEY);

    location_cnt = preferences.getInt("locations");  // global variable
    dbgPrintln("Locations: " + String(location_cnt));

    SLEEP_INTERVAL_MIN = preferences.getInt("SLEEP_MIN");
    wakeupHour = preferences.getInt("WakeupHour", 7);
    sleepHour = preferences.getInt("SleepHour", 23);

    logSettings.INFLUXDB_URL = preferences.getString("INFLUXDB_URL");
    logSettings.INFLUXDB_BUCKET = preferences.getString("INFLUXDB_BUCKET");
    logSettings.INFLUXDB_ORG = preferences.getString("INFLUXDB_ORG");
    logSettings.INFLUXDB_TOKEN = preferences.getString("INFLUXDB_TOKEN");

    dbgPrintln("INFLUXDB_URL: " + logSettings.INFLUXDB_URL);
    dbgPrintln("INFLUXDB_BUCKET: " + logSettings.INFLUXDB_BUCKET);
    dbgPrintln("INFLUXDB_ORG: " + logSettings.INFLUXDB_ORG);
    dbgPrintln("INFLUXDB_TOKEN: " + logSettings.INFLUXDB_TOKEN);
    dbgPrintln("SLEEP_INTERVAL_MIN: " + String(SLEEP_INTERVAL_MIN));
    dbgPrintln("wakeupHour: " + String(wakeupHour));
    dbgPrintln("sleepHour: " + String(sleepHour));

    location[0].name = preferences.getString("loc1", "");
    location[0].lat = preferences.getFloat("lat1", 0.0f);
    location[0].lon = preferences.getFloat("lon1", 0.0f);
    location[0].print();

    if (location_cnt > 1) {
        location[1].name = preferences.getString("loc2", "");
        location[1].lat = preferences.getFloat("lat2", 0.0f);
        location[1].lon = preferences.getFloat("lon2", 0.0f);
        location[1].print();
    }
    preferences.end();

    wifi.print();
}


int read_location_from_memory() {
    dbgPrintln("Read current location from memory...");
    preferences.begin(LOC_MEMORY_ID, true);  // first param true means 'read only'
    int location_id = preferences.getInt("curr_loc");
    preferences.end();
    return location_id;
}


void save_config_to_memory() {
    dbgPrintln("Save config to memory.");
    
    preferences.begin(MEMORY_ID, false);  // first param false means 'read/write'

    preferences.putString("ssid", wifi.ssid);
    preferences.putString("pass", wifi.pass);

    preferences.putString("POSSTACK_KEY", apiKeys.POSITIONSTACK_KEY);
    preferences.putString("WAQI_KEY", apiKeys.WAQI_KEY);
    preferences.putString("OPENWEATHER_KEY", apiKeys.OPENWEATHER_KEY);
    preferences.putString("TIMEZDB_KEY", apiKeys.TIMEZDB_KEY);

    dbgPrintln("POSITIONSTACK_KEY: " + apiKeys.POSITIONSTACK_KEY);
    dbgPrintln("WAQI_KEY: "          + apiKeys.WAQI_KEY);
    dbgPrintln("OPENWEATHER_KEY: "   + apiKeys.OPENWEATHER_KEY);
    dbgPrintln("TIMEZDB_KEY: "       + apiKeys.TIMEZDB_KEY);

    dbgPrintln("Locations: " + String(location_cnt));
    preferences.putInt("locations", location_cnt);  // global variable

    preferences.putInt("SLEEP_MIN", SLEEP_INTERVAL_MIN); 
    preferences.putInt("WakeupHour", wakeupHour);
    preferences.putInt("SleepHour", sleepHour);

    preferences.putString("INFLUXDB_URL", logSettings.INFLUXDB_URL);
    preferences.putString("INFLUXDB_BUCKET", logSettings.INFLUXDB_BUCKET);
    preferences.putString("INFLUXDB_ORG", logSettings.INFLUXDB_ORG);
    preferences.putString("INFLUXDB_TOKEN", logSettings.INFLUXDB_TOKEN);

    dbgPrintln("INFLUXDB_URL: " + logSettings.INFLUXDB_URL);
    dbgPrintln("INFLUXDB_BUCKET: " + logSettings.INFLUXDB_BUCKET);
    dbgPrintln("INFLUXDB_ORG: " + logSettings.INFLUXDB_ORG);
    dbgPrintln("INFLUXDB_TOKEN: " + logSettings.INFLUXDB_TOKEN);
    dbgPrintln("SLEEP_INTERVAL_MIN: " + String(SLEEP_INTERVAL_MIN));
    dbgPrintln("wakeupHour: " + String(wakeupHour));
    dbgPrintln("sleepHour: " + String(sleepHour));

    location[0].print();
    preferences.putString("loc1", location[0].name);
    preferences.putFloat("lat1", location[0].lat);
    preferences.putFloat("lon1", location[0].lon);

    if (location_cnt > 1) {
        location[1].print();
        preferences.putString("loc2", location[1].name);
        preferences.putFloat("lat2", location[1].lat);
        preferences.putFloat("lon2", location[1].lon);
    }

    preferences.end();

    delay(1000);

    wifi.print();
}


void save_location_to_memory(int location_id) {
    dbgPrintln("Save current location to memory...");
    preferences.begin(LOC_MEMORY_ID, false);
    preferences.putInt("curr_loc", location_id);
    preferences.end();
}

void run_config_server() {
    
    String network = "ttgo-t5-2.13-weather-wifi";
    String pass = String(abs((int)esp_random())).substring(0, 4) + "0000";
    //WiFi.softAP(network.c_str(), pass.c_str());
    //WiFi.softAP(network.c_str());  // <- without password

    print_pt();

    read_config_from_memory();

    IPAddress localIp(192, 168, 4, 1);
    IPAddress localMask(255, 255, 255, 0);
    WiFi.softAPConfig(localIp, localIp, localMask);

    dnsServer.start(53, "*", WiFi.softAPIP());
    
    dbgPrintln("Start config server on ssid: " + network + ", pass: " + pass + ", ip: " + WiFi.softAPIP().toString());
    display_config_mode(network, pass, WiFi.softAPIP().toString(), "WiFi Configuration..");
    display.update();

    WiFi.mode(WIFI_STA); 

    WiFiManagerParameter parmLocation1("parmLocation1", "Location 1", location[0].name.c_str(), 15);
    WiFiManagerParameter parmLocation2("parmLocation2", "Location 2", location[1].name.c_str(), 15);
    WiFiManagerParameter parmPositionstackKey("parmPositionstackKey", "Positionstack key", apiKeys.POSITIONSTACK_KEY.c_str(), 40);
    WiFiManagerParameter parmWaqiKey("parmWaqiKey", "Waqi key", apiKeys.WAQI_KEY.c_str(), 50);
    WiFiManagerParameter parmOpenweatherKey("parmOpenweatherKey", "Openweather key", apiKeys.OPENWEATHER_KEY.c_str(), 40);
    WiFiManagerParameter parmTimezdbKey("parmTimezdbKey", "Timezdb key", apiKeys.TIMEZDB_KEY.c_str(), 20);
    WiFiManagerParameter parmSleepInterval("parmSleepInterval", "Sleep interval (5-60 min)", String(SLEEP_INTERVAL_MIN).c_str(), 20);
    WiFiManagerParameter parmWakeupHour("parmWakeupHour", "Wakeup hour", String(wakeupHour).c_str(), 10);
    WiFiManagerParameter parmSleepHour("parmSleepHour", "Sleep hour", String(sleepHour).c_str(), 10);

    WiFiManagerParameter parmINFLUXDB_URL("parmINFLUXDB_URL", "Log INFLUXDB_URL", logSettings.INFLUXDB_URL.c_str(), 50);
    WiFiManagerParameter parmINFLUXDB_BUCKET("parmINFLUXDB_BUCKET", "Log INFLUXDB_BUCKET", logSettings.INFLUXDB_BUCKET.c_str(), 50);
    WiFiManagerParameter parmINFLUXDB_ORG("parmINFLUXDB_ORG", "Log INFLUXDB_ORG", logSettings.INFLUXDB_ORG.c_str(), 50);
    WiFiManagerParameter parmINFLUXDB_TOKEN("parmINFLUXDB_TOKEN", "LogINFLUXDB_TOKEN", logSettings.INFLUXDB_TOKEN.c_str(), 100);

    WiFiManager wm;

    wm.addParameter(&parmLocation1);
    wm.addParameter(&parmLocation2);
    wm.addParameter(&parmPositionstackKey);
    wm.addParameter(&parmWaqiKey);
    wm.addParameter(&parmOpenweatherKey);
    wm.addParameter(&parmTimezdbKey);
    wm.addParameter(&parmSleepInterval);
    wm.addParameter(&parmWakeupHour);
    wm.addParameter(&parmSleepHour);

    wm.addParameter(&parmINFLUXDB_URL);
    wm.addParameter(&parmINFLUXDB_BUCKET);
    wm.addParameter(&parmINFLUXDB_ORG);
    wm.addParameter(&parmINFLUXDB_TOKEN);

    //wm.setTimeout(120);
    wm.setConfigPortalTimeout(60*5); //5 min
    wm.setSaveConfigCallback(saveConfigCallback);

    bool res = wm.startConfigPortal(network.c_str(), pass.c_str());

    if (!res && !configOk) {

        dbgPrintln("Config: WiFi timeout..");

        display.fillScreen(GxEPD_WHITE);
        display.setFont(&Cousine_Regular6pt7b);
        print_text(0, 5, String("Wifi connection timeout..\nConfig validation failed..\nPower off"));        
        display.update();
        delay(2000);

        //collectAndWriteLog(CONFIG_MODE, false, false, false);
        //dbgPrintln("Config: power off..");

        //long sleep_time_micro_sec = 24* 60 * 1000 * 1000 * 60; //24h
        esp_sleep_enable_timer_wakeup(3600000000*24);
        begin_deep_sleep();

        delay(5000);
        dbgPrintln("Config: Sleep..");
    } 
    else 
    {
        dbgPrintln("Config: WiFi manager exit..");

        display.fillScreen(GxEPD_WHITE);
        display.setFont(&Cousine_Regular6pt7b);
        print_text(0, 5, String("Wifi connected OK!"));
        display.update();
        delay(2000);

        if (shouldSaveConfig)
        {
            dbgPrintln("Config: WiFi manager save config..");

            wifi.pass = wm.getWiFiPass();
            wifi.ssid = wm.getWiFiSSID();

            location_cnt = 1;
            location[0].name = String(parmLocation1.getValue());
            location[1].name = String(parmLocation2.getValue());
            
            if (location[1].name != "") {
                location_cnt = 2;
            }

            apiKeys.POSITIONSTACK_KEY = String(parmPositionstackKey.getValue());
            apiKeys.WAQI_KEY = String(parmWaqiKey.getValue());
            apiKeys.OPENWEATHER_KEY = String(parmOpenweatherKey.getValue());
            apiKeys.TIMEZDB_KEY = String(parmTimezdbKey.getValue());

            SLEEP_INTERVAL_MIN = atoi(parmSleepInterval.getValue());
            wakeupHour = atoi(parmWakeupHour.getValue());
            sleepHour = atoi(parmSleepHour.getValue());

            logSettings.INFLUXDB_URL = String(parmINFLUXDB_URL.getValue());
            logSettings.INFLUXDB_BUCKET = String(parmINFLUXDB_BUCKET.getValue());
            logSettings.INFLUXDB_ORG = String(parmINFLUXDB_ORG.getValue());
            logSettings.INFLUXDB_TOKEN = String(parmINFLUXDB_TOKEN.getValue());
            
            save_config_to_memory();

            configOk = false;
        }        

        if (configOk)
        {
            if (connect_to_wifi()) 
            {
                collectAndWriteLog(CONFIG_MODE, false, false, false);
            }
            set_mode(OPERATING_MODE);
            print_text(0, 35, String("Restarting to operationg mode.."));
            display.update();
            delay(3000);
            display.fillScreen(GxEPD_WHITE);
            delay(1000);
            run_operating_mode(true);
        }
        else
        {
            set_mode(VALIDATING_MODE);
            print_text(0, 35, String("Restarting to validation mode.."));
            display.update();
        } 

        if (connect_to_wifi()) 
        {
            collectAndWriteLog(CONFIG_MODE, false, false, false);
        }

        delay(3000);
        ESP.restart();
    }

}


void run_validating_mode() {
    //server.end();
    String keyErrMsg;
    bool checkFailed = false;
    bool is_time_fetched = false;
    bool is_weather_fetched = false;
    bool is_aq_fetched = false;
        
    read_config_from_memory();
    
    display_validating_mode();
    display.update();
    delay(2000);

    configOk = false;
    
    if (connect_to_wifi()) {

        WiFiClientSecure client;
        //bool is_location_fetched = false;

        dbgPrintln("Wifi connected, validate settings...");

        if (apiKeys.POSITIONSTACK_KEY == "")
        {
          keyErrMsg = "POSITIONSTACK_KEY";
        }

        if (apiKeys.WAQI_KEY == "")
        {
          keyErrMsg += ",WAQI_KEY";
        } 

        if (apiKeys.OPENWEATHER_KEY == "")
        {
          keyErrMsg += ",OPENWEATHER_KEY";
        } 

        if (apiKeys.TIMEZDB_KEY == "")
        {
          keyErrMsg += ",TIMEZDB_KEY";
        }

        if (SLEEP_INTERVAL_MIN < 5 || SLEEP_INTERVAL_MIN > 60)
        {
          keyErrMsg += ",SLEEP_INTERVAL_MIN";
        }

        if (sleepHour > 23 || sleepHour < 0)
        {
          keyErrMsg += "SleepHour";
        }

        if (wakeupHour > 23 || wakeupHour < 0)
        {
          keyErrMsg += "WakeUpHour";
        }

        dbgPrintln("Validate key, missing keys: " + (keyErrMsg == ""? "No" : keyErrMsg));

        if (keyErrMsg != "")
        {
          display.setFont(&Cousine_Regular6pt7b);
          print_text(0, 45, "Key(s) is/are not configured:\n" + keyErrMsg + "\n" + "Restarting in 10 sec");
          display.update();

          collectAndWriteLog(VALIDATING_MODE, is_time_fetched, is_weather_fetched, is_aq_fetched);

          set_mode(CONFIG_MODE);
          delay(10000);
          ESP.restart();
          return;
        }
        
        dbgPrintln("Validate: get locations by names");

        if (location_cnt > 0) {

            dbgPrintln("Validate: for loop " + String(location_cnt) + " locations");

            for (int idx = 0; idx < location_cnt; idx++)
            {                
                location_request.name = location[idx].name;
                location_request.api_key = apiKeys.POSITIONSTACK_KEY;
                location_request.handler = location_handler;
                location_request.make_path();

                if (http_request_data(client, location_request))
                {
                    location[idx].lat = location_request.response.lat;
                    location[idx].lon = location_request.response.lon;

                    dbgPrintln("Validate: location " + location[idx].name + " fetched Ok");
                }
                else
                {
                    keyErrMsg += dbgPrintln("Validate: location " + location[idx].name + " fetch FAILED");
                    checkFailed = true;
                }              
            } //for

            if (!checkFailed)
            {
                datetime_request.api_key = apiKeys.TIMEZDB_KEY;
                datetime_request.handler = datetime_handler;
                datetime_request.make_path(location[0]);

                weather_request.api_key = apiKeys.OPENWEATHER_KEY;
                weather_request.handler = weather_handler;
                weather_request.make_path(location[0]);                

                airquality_request.api_key = apiKeys.WAQI_KEY;
                airquality_request.handler = air_quality_handler;
                airquality_request.make_path(location[0]);                

                is_time_fetched = http_request_data(client, datetime_request);
                is_weather_fetched = http_request_data(client, weather_request);
                is_aq_fetched = http_request_data(client, airquality_request);

                if (!is_time_fetched)
                {
                    keyErrMsg += dbgPrintln("Time fetch error\nTIMEZDB_KEY not valid");
                    checkFailed = true;
                }

                if (!is_weather_fetched)
                {
                    keyErrMsg += dbgPrintln("Weather fetch error\nOPENWEATHER_KEY not valid");
                    checkFailed = true;
                }

                if (!is_aq_fetched)
                {
                    keyErrMsg += dbgPrintln("Airquality fetch error\nWAQI_KEY not valid");
                    checkFailed = true;
                }
            }            
        }
        else
        {
            keyErrMsg += dbgPrintln("Validate: no locations configured to fetch");
            checkFailed = true;
        }

        if (checkFailed)
        {
          display.setFont(&Cousine_Regular6pt7b);
          print_text(0, 45, "Validation failed:\n" + keyErrMsg);
          display.update();

          collectAndWriteLog(VALIDATING_MODE, is_time_fetched, is_weather_fetched, is_aq_fetched);

          set_mode(CONFIG_MODE);
          delay(15000);
          ESP.restart();
          return;
        } 
    }
    else
    {
      dbgPrintln("Wifi connection failed, reboot to config...");

      display.setFont(&Cousine_Regular6pt7b);
      print_text(0, 45, "Wifi connection failed\nReboot to config...");
      display.update();

      //collectAndWriteLog(VALIDATING_MODE, is_time_fetched, is_weather_fetched, is_aq_fetched);
      set_mode(CONFIG_MODE);      
      delay(15000);
      ESP.restart();
    }

    configOk = true;
    save_config_to_memory();

    collectAndWriteLog(VALIDATING_MODE, is_time_fetched, is_weather_fetched, is_aq_fetched);

    set_mode(OPERATING_MODE);
    print_text(0, 35, String("Restarting to operationg mode.."));
    display.update();
    delay(3000);
    display.fillScreen(GxEPD_WHITE);
    delay(1000);
    run_operating_mode(true);

    dbgPrintln("End validation mode");
    delay(3000);
    ESP.restart();
    dbgPrintln("ESP restart");
}


void run_operating_mode(bool _skip_sleep_hours_check) {
    bool WakeUp = false;
    bool is_time_fetched = false;
    bool is_weather_fetched = false;
    bool is_aq_fetched = false;

    read_config_from_memory();
    curr_loc = read_location_from_memory();    

    if (connect_to_wifi()) {
        WiFiClientSecure client;

        datetime_request.api_key = apiKeys.TIMEZDB_KEY;        
        datetime_request.handler = datetime_handler;
        datetime_request.make_path(location[curr_loc]);

        is_time_fetched = http_request_data(client, datetime_request);   

        if (is_time_fetched)
        {
            struct tm* timeinfo = localtime(&datetime_request.response.dt);
            if (wakeupHour > sleepHour)
                WakeUp = (timeinfo->tm_hour >= wakeupHour || timeinfo->tm_hour <= sleepHour);
            else
                WakeUp = (timeinfo->tm_hour >= wakeupHour && timeinfo->tm_hour <= sleepHour);
        } 

        dbgPrintln("Time fetched: " + String(is_time_fetched) + ", WakeUp: " + String(WakeUp) + ", Skip sleep: " + String(_skip_sleep_hours_check));       

        if (WakeUp || _skip_sleep_hours_check) {

            dbgPrintln("Start weather_request");
            weather_request.api_key = apiKeys.OPENWEATHER_KEY;
            weather_request.handler = weather_handler;
            weather_request.make_path(location[curr_loc]);        

            airquality_request.api_key = apiKeys.WAQI_KEY;
            airquality_request.handler = air_quality_handler;
            airquality_request.make_path(location[curr_loc]); 
            
            is_weather_fetched = http_request_data(client, weather_request);
            is_aq_fetched = http_request_data(client, airquality_request);

            view = View();

            update_header_view(view, is_time_fetched); 
            update_weather_view(view, is_weather_fetched);
            update_air_quality_view(view, is_aq_fetched);
                
            dbgPrintln("Update display.");
            display_header(view);
            display_weather(view);
            display_air_quality(view);

            display.update();
            delay(100); // too fast display powerDown displays blank (white)??
            dbgPrintln("End weather_request");
        }

        collectAndWriteLog(OPERATING_MODE, is_time_fetched, is_weather_fetched, is_aq_fetched);
    } 
    
    // deep sleep stuff
    enable_timed_sleep(SLEEP_INTERVAL_MIN);
    begin_deep_sleep();
    dbgPrintln("End run_operating_mode");
}


void set_mode(int mode) {

    dbgPrintln("Set mode: " + String(mode) + ", Config OK: " + String(configOk));

    preferences.begin(MEMORY_ID, false);
    preferences.putInt("mode", mode);    
    preferences.putBool("configOk", configOk);  // global variable
    preferences.putInt("bootCount", boot_count); 
    preferences.end();    
}


void set_mode_and_reboot(int mode) {
    set_mode(mode);
    ESP.restart();
}


int get_mode() {

    preferences.begin(MEMORY_ID, false);
    int mode = preferences.getInt("mode", NOT_SET_MODE);
    configOk = preferences.getBool("configOk", false);  // global variable
    boot_count = preferences.getInt("bootCount"); 
    preferences.end();

    dbgPrintln("Get mode: " + String(mode) + ", Config OK: " + String(configOk));

    return mode;
}


void setup() {
    Serial.begin(115200); 
    dbgPrintln("\n\n=== WEATHER STATION ===");
    init_display();

    if (get_mode() == NOT_SET_MODE) {
        dbgPrintln("MODE: not set. Initializing mode to CONFIG_MODE.");
        boot_count++;
        set_mode_and_reboot(CONFIG_MODE);
    }
    const int mode = get_mode();
    boot_count++;
    
    if (mode == CONFIG_MODE) {
        dbgPrintln("MODE: Config");
        run_config_server();
    } else if (mode == VALIDATING_MODE) {
        dbgPrintln("MODE: Validating");
        run_validating_mode();
    } else if (mode == OPERATING_MODE) {
        dbgPrintln("MODE: Operating");
        wakeup_reason();
        run_operating_mode(false);
    }
}


void loop() {  
    // dbgPrintln("Handling DNS");

    // Used only in CONFIG mode 
    dnsServer.processNextRequest();
}
