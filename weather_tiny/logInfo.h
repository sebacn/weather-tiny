#ifndef _logInfo_h
#define _logInfo_h

#include <Arduino.h>

#define NOT_SET_MODE 0
#define CONFIG_MODE 1
#define VALIDATING_MODE 2
#define OPERATING_MODE 3

// Home Assistant MQTT over Cloudflare Access (wss). Empty host leaves MQTT off.
struct MqttSettings {
    String HaHost = "";
    String CfAccessId = "";
    String CfAccessSecret = "";
    String MqttUser = "";
    String MqttPass = "";

    void print() {
        extern String dbgPrintln(String _str);
        dbgPrintln(String("MQTT: home_assistant_server_host: ") + HaHost
            + ", CF-Access-Client-Id: " + String(CfAccessId.length() ? "set" : "empty")
            + ", CF-Access-Client-Secret: " + String(CfAccessSecret.length() ? "set" : "empty")
            + ", mqtt-user: " + String(MqttUser.length() ? "set" : "empty")
            + ", mqtt-password: " + String(MqttPass.length() ? "set" : "empty"));
    }
};

struct LogInfo {
    int BootCount;
    long UTCTimestamp;
    int BatteryPct;
    float BatteryVoltage;
    bool ConfigOk;
    int Mode;
    bool TimeFetchOk;
    bool WeatherFetchOk;
    bool AQIFetchOk;

    String mode2String()
    {
        switch (Mode)
        {
            case CONFIG_MODE:
                return "CONFIG_MODE";
            case VALIDATING_MODE:
                return "VALIDATING_MODE";
            case OPERATING_MODE:
                return "OPERATING_MODE";
            default:
                return "NOT_SET_MODE";
        }
    }
};

MqttSettings mqttSettings;
LogInfo logInfo;

#endif
