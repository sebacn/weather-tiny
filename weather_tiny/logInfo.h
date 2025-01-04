#ifndef _logInfo_h
#define _logInfo_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <InfluxDbClient.h>
//Include also InfluxCloud 2 CA certificate
//#include <InfluxDbCloud.h>

#define NOT_SET_MODE 0
#define CONFIG_MODE 1
#define VALIDATING_MODE 2
#define OPERATING_MODE 3

extern String dbgPrintln(String _str);

// Server certificate in PEM format, placed in the program (flash) memory to save RAM
const char InfluxDBServerCert[] PROGMEM =  R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

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
    long Timestamp;
    int BatteryPct;
    bool ConfigOk;
    int Mode;
    bool TimeFetchOk; 
    bool WeatherFetchOk; 
    bool AQIFetchOk; 

    JsonDocument doc;
    
    String to_string() {

        doc["BootCount"] = BootCount;
        doc["Timestamp"] = Timestamp;
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

// Single InfluxDB instance

bool writeLogInfo(){

    if (   logSettings.INFLUXDB_URL == ""
        || logSettings.INFLUXDB_ORG == ""
        || logSettings.INFLUXDB_BUCKET == ""
        || logSettings.INFLUXDB_TOKEN == "")
    {
        dbgPrintln("InfluxDBClient: missing configuration");
        return false;
    }

    Point pointDevice("ttgo-t5-2.13-weather");

    pointDevice.addTag("ModeTag", logInfo.mode2String());
    pointDevice.addTag("ConfigOkTag", String(logInfo.ConfigOk));

    pointDevice.setTime(logInfo.Timestamp);
    pointDevice.addField("BootCount", logInfo.BootCount);
    //pointDevice.addField("Timestamp", logInfo.Timestamp);
    pointDevice.addField("BatteryPct", logInfo.BatteryPct);
    pointDevice.addField("ConfigOk", logInfo.ConfigOk);
    pointDevice.addField("Mode", logInfo.Mode);
    pointDevice.addField("TimeFetchOk", logInfo.TimeFetchOk);
    pointDevice.addField("WeatherFetchOk", logInfo.WeatherFetchOk);
    pointDevice.addField("AQIFetchOk", logInfo.AQIFetchOk);

    InfluxDBClient client(logSettings.INFLUXDB_URL, logSettings.INFLUXDB_ORG, logSettings.INFLUXDB_BUCKET, logSettings.INFLUXDB_TOKEN, InfluxDBServerCert);

    client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::MS));
    client.setWriteOptions(WriteOptions().useServerTimestamp(logInfo.Timestamp == 0));
    // Write data
    bool ret = client.writePoint(pointDevice);

    dbgPrintln("InfluxDBClient write: " + String(ret));

    return ret;
}


#endif