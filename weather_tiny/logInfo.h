#ifndef _logInfo_h
#define _logInfo_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <InfluxDbClient.h>
#include "ca_cert.h"

#define NOT_SET_MODE 0
#define CONFIG_MODE 1
#define VALIDATING_MODE 2
#define OPERATING_MODE 3

extern String dbgPrintln(String _str);
extern String infoPrintln(String _str);

// Server certificate in PEM format, placed in the program (flash) memory to save RAM
constexpr char const *ROOT_CA_INFLUXDB = ROOT_CA_POSITIONSTACK;

const char MTLS_CERT[] PROGMEM =  R"EOF(
-----BEGIN CERTIFICATE-----
TODO: SET CERTIFICATE HERE
-----END CERTIFICATE-----)EOF";

const char MTLS_PKEY[] PROGMEM =  R"EOF(
-----BEGIN PRIVATE KEY-----
TODO: SET PRIVATE KEY HERE
-----END PRIVATE KEY-----)EOF";

struct LogSettings {
    // InfluxDB 2 server or cloud url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
    String INFLUXDB_URL; // = "influxdb-url"
    // InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
    String INFLUXDB_TOKEN; // "token"
    // InfluxDB 2 organization name or id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
    String INFLUXDB_ORG; // "org"
    // InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
    String INFLUXDB_BUCKET; // "bucket"
};

struct LogInfo {
    int BootCount;
    long UTCTimestamp;
    int BatteryPct;
    bool ConfigOk;
    int Mode;
    bool TimeFetchOk; 
    bool WeatherFetchOk; 
    bool AQIFetchOk; 
/*
    JsonDocument doc;
    
    String to_string() {

        doc["BootCount"] = BootCount;
        doc["Timestamp"] = UTCTimestamp;
        doc["BatteryPct"] = BatteryPct;
        doc["ConfigOk"] = ConfigOk;
        doc["Mode"] = Mode;
        doc["TimeFetchOk"] = TimeFetchOk;
        doc["WeatherFetchOk"] = WeatherFetchOk;
        doc["AQIFetchOk"] = AQIFetchOk;

        String ret;

        serializeJson(doc, ret);

        return ret;
    }
    
    void print() {
        Serial.println("=== DBG: " + this->to_string() + "\n");
    }
*/
    String mode2String()
    {
        String ret;

        switch (Mode)
        {
            case CONFIG_MODE:
            ret = "CONFIG_MODE";
            break;

            case VALIDATING_MODE:
            ret = "VALIDATING_MODE";
            break;

            case OPERATING_MODE:
            ret = "OPERATING_MODE";
            break;
        
        default:
            ret = "NOT_SET_MODE";
            break;
        }

        return ret;
    }
} ;

LogSettings logSettings;
LogInfo logInfo;

InfluxDBClient client;
// Single InfluxDB instance

void writeLogInfo(){

    dbgPrintln("InfluxDBClient: writeLogInfo");

    if (   logSettings.INFLUXDB_URL == ""
        || logSettings.INFLUXDB_ORG == ""
        || logSettings.INFLUXDB_BUCKET == ""
        || logSettings.INFLUXDB_TOKEN == "")
    {
        dbgPrintln("InfluxDBClient: missing configuration");
        return;
    }

    client.setConnectionParams(logSettings.INFLUXDB_URL, logSettings.INFLUXDB_ORG, logSettings.INFLUXDB_BUCKET, logSettings.INFLUXDB_TOKEN, ROOT_CA_INFLUXDB, MTLS_CERT, MTLS_PKEY);

    Point pointDevice("ttgo-t5-2.13-weather");
    pointDevice.addTag("ModeTag", logInfo.mode2String());
    pointDevice.addTag("ConfigOkTag", String(logInfo.ConfigOk));
    pointDevice.addTag("TimeFetchOk", String(logInfo.TimeFetchOk));
    pointDevice.addTag("WeatherFetchOk", String(logInfo.WeatherFetchOk));
    pointDevice.addTag("AQIFetchOk", String(logInfo.AQIFetchOk));

    //pointDevice.addField("Timestamp", logInfo.Timestamp);
    dbgPrintln("InfluxDBClient UTCTimestamp (s): " + String(logInfo.UTCTimestamp));
    if (logInfo.UTCTimestamp > 0)
    {        
        pointDevice.setTime(logInfo.UTCTimestamp); //Unix timestamp WritePrecision::S
    }
    
    pointDevice.addField("BootCount", logInfo.BootCount);
    pointDevice.addField("BatteryPct", logInfo.BatteryPct);

    client.setInsecure(false);

    // Check server connection
    if (client.validateConnection()) 
    {
        infoPrintln("InfluxDBClient Connected OK to: " + client.getServerUrl());

        client.setWriteOptions(WriteOptions().useServerTimestamp(logInfo.UTCTimestamp == 0).writePrecision(WritePrecision::S));

        // Write point
        if (client.writePoint(pointDevice)) 
        {
            if (!client.isBufferEmpty()) {
                dbgPrintln("InfluxDBClient flushBuffer"); //Write all remaining points to db
                client.flushBuffer();
            }
            infoPrintln("InfluxDBClient write OK");            
        }
        else
        {
            infoPrintln("InfluxDBClient write failed: " + client.getLastErrorMessage());
        }
    } 
    else 
    {
        infoPrintln("InfluxDB connection failed: " + client.getLastErrorMessage());
    }
}

#endif