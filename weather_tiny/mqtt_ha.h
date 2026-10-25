#ifndef _mqtt_ha_h
#define _mqtt_ha_h

// Home Assistant MQTT over WebSocket TLS (wss) so a Cloudflare Access
// application can read CF-Access-Client-Id and CF-Access-Client-Secret
// on the HTTP upgrade, then proxy MQTT to the broker.

#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <esp_system.h>
#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>

#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 16384
#endif
#include <PubSubClient.h>
#include "logInfo.h"

extern String dbgPrintln(String _str);
extern String infoPrintln(String _str);
extern String mqttCity;

extern "C" {
extern const uint8_t x509_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
}

static const size_t MQTT_FRAME_MAX = 16384;

struct MqttEndpoint {
    String host;
    uint16_t port;
    String path;
};

inline bool parseHaHost(const String& raw, MqttEndpoint& out) {
    String s = raw;
    s.trim();
    if (s.startsWith("wss://")) s = s.substring(6);
    else if (s.startsWith("https://")) s = s.substring(8);
    else if (s.startsWith("ws://") || s.startsWith("http://")) return false;
    s.trim();
    if (!s.length() || s.indexOf(' ') >= 0) return false;

    int slash = s.indexOf('/');
    String hostport = slash >= 0 ? s.substring(0, slash) : s;
    String path = slash >= 0 ? s.substring(slash) : String("/mqtt");
    if (!path.length()) path = "/";
    if (!path.startsWith("/")) path = "/" + path;

    int colon = hostport.lastIndexOf(':');
    out.port = 443;
    if (colon > 0) {
        out.host = hostport.substring(0, colon);
        int port = hostport.substring(colon + 1).toInt();
        if (port <= 0 || port > 65535) return false;
        out.port = (uint16_t)port;
    } else {
        out.host = hostport;
    }
    out.host.trim();
    if (!out.host.length() || out.host.indexOf(' ') >= 0 || out.host.indexOf('/') >= 0) return false;
    out.path = path;
    return true;
}

// Arduino Client that speaks MQTT framing over one masked binary WebSocket.
class CloudflareMqttClient : public Client {
public:
    void prepare(const String& path, const String& clientId, const String& clientSecret) {
        wsPath = path.length() ? path : String("/mqtt");
        accessId = clientId;
        accessSecret = clientSecret;
        wsPath.replace("\r", "");
        wsPath.replace("\n", "");
        accessId.replace("\r", "");
        accessId.replace("\n", "");
        accessSecret.replace("\r", "");
        accessSecret.replace("\n", "");
    }

    int connect(IPAddress ip, uint16_t port) override {
        (void)ip;
        (void)port;
        infoPrintln("MQTT connect needs a hostname for TLS and Cloudflare");
        return 0;
    }

    int connect(const char* host, uint16_t port) override {
        closeSocket();
        if (!host || !host[0]) return 0;

        tls.setCACertBundle(x509_crt_bundle_start);
        tls.setHandshakeTimeout(30);
        dbgPrintln("MQTT TLS " + String(host) + ":" + String(port)
            + wsPath + " heap " + String(ESP.getFreeHeap()));
        if (!tls.connect(host, port)) {
            char err[96];
            err[0] = 0;
            int code = tls.lastError(err, sizeof(err));
            infoPrintln("MQTT TLS failed (" + String(code) + ") " + String(err));
            closeSocket();
            return 0;
        }

        uint8_t keyRaw[16];
        uint32_t r0 = esp_random();
        uint32_t r1 = esp_random();
        uint32_t r2 = esp_random();
        uint32_t r3 = esp_random();
        memcpy(keyRaw, &r0, 4);
        memcpy(keyRaw + 4, &r1, 4);
        memcpy(keyRaw + 8, &r2, 4);
        memcpy(keyRaw + 12, &r3, 4);
        String wsKey = b64(keyRaw, sizeof(keyRaw));
        if (!wsKey.length()) {
            infoPrintln("MQTT WebSocket key failed");
            closeSocket();
            return 0;
        }

        String req;
        req.reserve(640);
        req += "GET ";
        req += wsPath;
        req += " HTTP/1.1\r\nHost: ";
        req += host;
        if (port != 443) {
            req += ":";
            req += String(port);
        }
        req += "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: ";
        req += wsKey;
        req += "\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Protocol: mqtt\r\n";
        if (accessId.length() && accessSecret.length()) {
            req += "CF-Access-Client-Id: ";
            req += accessId;
            req += "\r\nCF-Access-Client-Secret: ";
            req += accessSecret;
            req += "\r\n";
        }
        req += "\r\n";

        if (!writeAll((const uint8_t*)req.c_str(), req.length())) {
            infoPrintln("MQTT WebSocket request failed");
            closeSocket();
            return 0;
        }
        req = "";

        String resp;
        uint32_t start = millis();
        while (millis() - start < 15000) {
            int avail = tls.available();
            if (avail < 0 || (!tls.connected() && avail <= 0)) break;
            if (avail == 0) {
                delay(10);
                continue;
            }
            while (tls.available() > 0 && resp.length() < 2048) {
                int c = tls.read();
                if (c < 0) break;
                resp += (char)c;
                if (resp.endsWith("\r\n\r\n")) break;
            }
            if (resp.endsWith("\r\n\r\n")) break;
        }

        int lineEnd = resp.indexOf("\r\n");
        String status = lineEnd >= 0 ? resp.substring(0, lineEnd) : resp;
        int sp = status.indexOf(' ');
        int sp2 = sp >= 0 ? status.indexOf(' ', sp + 1) : -1;
        int httpCode = sp >= 0 ? status.substring(sp + 1, sp2 > sp ? sp2 : status.length()).toInt() : 0;
        if (httpCode != 101) {
            infoPrintln("MQTT WebSocket upgrade failed, HTTP " + String(httpCode));
            closeSocket();
            return 0;
        }

        String lower = resp;
        lower.toLowerCase();
        int acc = lower.indexOf("sec-websocket-accept:");
        if (acc < 0) {
            infoPrintln("MQTT WebSocket handshake has no accept key");
            closeSocket();
            return 0;
        }
        int accVal = resp.indexOf(':', acc) + 1;
        int accEnd = resp.indexOf("\r\n", accVal);
        String got = resp.substring(accVal, accEnd >= 0 ? accEnd : resp.length());
        got.trim();
        String expect = acceptKey(wsKey);
        if (!expect.length() || got != expect) {
            infoPrintln("MQTT WebSocket accept key mismatch");
            closeSocket();
            return 0;
        }
        int proto = lower.indexOf("sec-websocket-protocol:");
        if (proto >= 0) {
            int protoVal = lower.indexOf(':', proto) + 1;
            int protoEnd = lower.indexOf("\r\n", protoVal);
            String agreed = lower.substring(protoVal, protoEnd >= 0 ? protoEnd : lower.length());
            agreed.trim();
            if (agreed.indexOf("mqtt") < 0) {
                infoPrintln("MQTT WebSocket protocol was not accepted");
                closeSocket();
                return 0;
            }
        }

        upgraded = true;
        dbgPrintln("MQTT WebSocket open");
        return 1;
    }

    size_t write(uint8_t data) override {
        return write(&data, 1);
    }

    size_t write(const uint8_t* buf, size_t size) override {
        if (!upgraded || !buf || size == 0) return 0;
        pull();
        if (txLen + size > MQTT_FRAME_MAX) return 0;
        if (!ensureTx(size)) return 0;
        memcpy(tx + txLen, buf, size);
        txLen += size;
        flushComplete();
        return upgraded ? size : 0;
    }

    int available() override {
        flushComplete();
        pull();
        if (rxPos >= rxLen) return 0;
        return (int)(rxLen - rxPos);
    }

    int read() override {
        if (available() <= 0) return -1;
        return rx[rxPos++];
    }

    int read(uint8_t* buf, size_t size) override {
        if (!buf || size == 0) return 0;
        int avail = available();
        if (avail <= 0) return -1;
        if ((size_t)avail < size) size = (size_t)avail;
        memcpy(buf, rx + rxPos, size);
        rxPos += size;
        return (int)size;
    }

    int peek() override {
        if (available() <= 0) return -1;
        return rx[rxPos];
    }

    void flush() override {
        flushComplete();
        if (txLen && upgraded) {
            if (!sendFrame(0x2, tx, txLen)) upgraded = false;
            txLen = 0;
        }
    }

    void stop() override {
        flush();
        if (upgraded && tls.connected()) sendFrame(0x8, nullptr, 0);
        closeSocket();
    }

    uint8_t connected() override {
        return upgraded && tls.connected();
    }

    operator bool() override {
        return connected() != 0;
    }

    ~CloudflareMqttClient() {
        closeSocket();
        free(tx);
        free(rx);
        free(pay);
        tx = nullptr;
        rx = nullptr;
        pay = nullptr;
    }

private:
    WiFiClientSecure tls;
    String wsPath;
    String accessId;
    String accessSecret;
    bool upgraded = false;

    uint8_t* tx = nullptr;
    size_t txCap = 0;
    size_t txLen = 0;

    uint8_t* rx = nullptr;
    size_t rxCap = 0;
    size_t rxLen = 0;
    size_t rxPos = 0;

    enum { PH_HDR, PH_EXT, PH_MASK, PH_PAY };
    int phase = PH_HDR;
    uint8_t hdr[2];
    size_t hdrGot = 0;
    uint8_t ext[8];
    size_t extNeed = 0;
    size_t extGot = 0;
    uint8_t mask[4];
    size_t maskGot = 0;
    bool masked = false;
    bool fin = true;
    uint8_t opcode = 0;
    uint8_t* pay = nullptr;
    size_t payNeed = 0;
    size_t payGot = 0;

    static String b64(const uint8_t* in, size_t len) {
        unsigned char out[64];
        size_t olen = 0;
        if (mbedtls_base64_encode(out, sizeof(out) - 1, &olen, in, len) != 0) return "";
        out[olen] = 0;
        return String((char*)out);
    }

    static String acceptKey(const String& key) {
        String src = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        unsigned char sha[20];
        if (mbedtls_sha1_ret((const unsigned char*)src.c_str(), src.length(), sha) != 0) return "";
        return b64(sha, sizeof(sha));
    }

    bool writeAll(const uint8_t* buf, size_t len) {
        size_t off = 0;
        while (off < len) {
            int n = tls.write(buf + off, len - off);
            if (n <= 0) return false;
            off += (size_t)n;
            yield();
        }
        return true;
    }

    bool ensureTx(size_t extra) {
        size_t need = txLen + extra;
        if (need <= txCap) return true;
        size_t cap = txCap ? txCap * 2 : 256;
        while (cap < need) cap *= 2;
        if (cap > MQTT_FRAME_MAX) cap = MQTT_FRAME_MAX;
        if (cap < need) return false;
        uint8_t* next = (uint8_t*)realloc(tx, cap);
        if (!next) return false;
        tx = next;
        txCap = cap;
        return true;
    }

    bool sendFrame(uint8_t op, const uint8_t* data, size_t len) {
        if (!tls.connected()) return false;
        uint32_t rnd = esp_random();
        uint8_t msk[4];
        memcpy(msk, &rnd, 4);
        if ((msk[0] | msk[1] | msk[2] | msk[3]) == 0) msk[0] = 0xA5;

        size_t hlen = 2;
        if (len >= 126 && len <= 0xFFFF) hlen = 4;
        else if (len > 0xFFFF) hlen = 10;
        size_t frameLen = hlen + 4 + len;
        uint8_t* frame = (uint8_t*)malloc(frameLen);
        if (!frame) return false;

        frame[0] = 0x80 | (op & 0x0F);
        if (len < 126) {
            frame[1] = 0x80 | (uint8_t)len;
        } else if (len <= 0xFFFF) {
            frame[1] = 0x80 | 126;
            frame[2] = (uint8_t)((len >> 8) & 0xFF);
            frame[3] = (uint8_t)(len & 0xFF);
        } else {
            frame[1] = 0x80 | 127;
            uint64_t L = len;
            for (int i = 0; i < 8; i++) frame[2 + i] = (uint8_t)((L >> ((7 - i) * 8)) & 0xFF);
        }
        memcpy(frame + hlen, msk, 4);
        for (size_t i = 0; i < len; i++) frame[hlen + 4 + i] = data[i] ^ msk[i & 3];
        bool ok = writeAll(frame, frameLen);
        free(frame);
        return ok;
    }

    enum { PKT_NEED, PKT_OK, PKT_BAD };

    static int packetBound(const uint8_t* b, size_t len, size_t& end) {
        if (len < 2) return PKT_NEED;
        size_t rem = 0;
        size_t mul = 1;
        size_t i = 1;
        bool done = false;
        for (int k = 0; k < 4; k++) {
            if (i >= len) return PKT_NEED;
            uint8_t enc = b[i++];
            rem += (size_t)(enc & 0x7F) * mul;
            mul *= 128;
            if ((enc & 0x80) == 0) {
                done = true;
                break;
            }
        }
        if (!done || rem > MQTT_FRAME_MAX) return PKT_BAD;
        if (len - i < rem) return PKT_NEED;
        end = i + rem;
        return PKT_OK;
    }

    void flushComplete() {
        if (!upgraded || txLen == 0) return;
        size_t off = 0;
        while (off < txLen) {
            size_t end = 0;
            int kind = packetBound(tx + off, txLen - off, end);
            if (kind == PKT_NEED) break;
            if (kind == PKT_BAD) {
                upgraded = false;
                txLen = 0;
                return;
            }
            off += end;
        }
        if (off == 0) return;
        if (!sendFrame(0x2, tx, off)) {
            upgraded = false;
            txLen = 0;
            return;
        }
        size_t rest = txLen - off;
        memmove(tx, tx + off, rest);
        txLen = rest;
    }

    bool rxAppend(const uint8_t* data, size_t len) {
        if (!len) return true;
        if (rxPos > 0) {
            memmove(rx, rx + rxPos, rxLen - rxPos);
            rxLen -= rxPos;
            rxPos = 0;
        }
        if (rxLen + len > rxCap) {
            size_t cap = rxCap ? rxCap : 128;
            while (cap < rxLen + len) cap *= 2;
            if (cap > 4096) return false;
            uint8_t* next = (uint8_t*)realloc(rx, cap);
            if (!next) return false;
            rx = next;
            rxCap = cap;
        }
        memcpy(rx + rxLen, data, len);
        rxLen += len;
        return true;
    }

    void resetFrame() {
        free(pay);
        pay = nullptr;
        payNeed = 0;
        payGot = 0;
        hdrGot = 0;
        extGot = 0;
        extNeed = 0;
        maskGot = 0;
        phase = PH_HDR;
    }

    bool take(uint8_t* dst, size_t& got, size_t need) {
        while (got < need) {
            int avail = tls.available();
            if (avail <= 0) return false;
            size_t want = need - got;
            if ((size_t)avail < want) want = (size_t)avail;
            int n = tls.read(dst + got, want);
            if (n <= 0) {
                upgraded = false;
                return false;
            }
            got += (size_t)n;
        }
        return true;
    }

    void onFrame() {
        bool data = (opcode == 0x1 || opcode == 0x2 || opcode == 0x0);
        if (opcode == 0x9) {
            sendFrame(0xA, pay, payNeed);
        } else if (opcode == 0x8) {
            upgraded = false;
        } else if (data) {
            if (pay && payNeed && !rxAppend(pay, payNeed)) upgraded = false;
        }
        resetFrame();
    }

    void pull() {
        if (!tls.connected() && tls.available() <= 0) {
            if (upgraded && !tls.connected()) upgraded = false;
            return;
        }
        for (int guard = 0; guard < 16 && tls.available() > 0; guard++) {
            if (phase == PH_HDR) {
                if (!take(hdr, hdrGot, 2)) return;
                if (hdr[0] & 0x70) {
                    upgraded = false;
                    return;
                }
                fin = hdr[0] & 0x80;
                opcode = hdr[0] & 0x0F;
                masked = hdr[1] & 0x80;
                uint8_t len7 = hdr[1] & 0x7F;
                if (len7 == 126) {
                    phase = PH_EXT;
                    extNeed = 2;
                    extGot = 0;
                } else if (len7 == 127) {
                    phase = PH_EXT;
                    extNeed = 8;
                    extGot = 0;
                } else {
                    payNeed = len7;
                    phase = masked ? PH_MASK : PH_PAY;
                }
            }
            if (phase == PH_EXT) {
                if (!take(ext, extGot, extNeed)) return;
                payNeed = 0;
                if (extNeed == 2) payNeed = ((size_t)ext[0] << 8) | ext[1];
                else {
                    if (ext[0] || ext[1] || ext[2] || ext[3]) {
                        upgraded = false;
                        return;
                    }
                    for (size_t i = 4; i < 8; i++) payNeed = (payNeed << 8) | ext[i];
                }
                phase = masked ? PH_MASK : PH_PAY;
            }
            if (phase == PH_MASK) {
                if (!take(mask, maskGot, 4)) return;
                phase = PH_PAY;
            }
            if (phase == PH_PAY) {
                if (payNeed > 4096) {
                    upgraded = false;
                    return;
                }
                if (payNeed && !pay) {
                    pay = (uint8_t*)malloc(payNeed);
                    if (!pay) {
                        upgraded = false;
                        return;
                    }
                    payGot = 0;
                }
                if (payNeed && !take(pay, payGot, payNeed)) return;
                if (masked && pay) {
                    for (size_t i = 0; i < payNeed; i++) pay[i] ^= mask[i & 3];
                    masked = false;
                }
                (void)fin;
                onFrame();
            }
        }
    }

    void closeSocket() {
        upgraded = false;
        txLen = 0;
        rxLen = 0;
        rxPos = 0;
        resetFrame();
        tls.stop();
    }
};

inline const char* mqttStateText(int state) {
    switch (state) {
        case -4: return "timeout";
        case -3: return "connection lost";
        case -2: return "connect failed";
        case -1: return "disconnected";
        case 1: return "bad protocol";
        case 2: return "client id rejected";
        case 3: return "broker unavailable";
        case 4: return "bad credentials";
        case 5: return "not authorized";
        default: return "error";
    }
}

inline bool mqttPublishRetained(PubSubClient& mqtt, const String& topic, const String& payload) {
    if (!mqtt.publish(topic.c_str(), (const uint8_t*)payload.c_str(), payload.length(), true)) {
        return false;
    }
    mqtt.loop();
    return true;
}

inline void publishHaDiscovery(PubSubClient& mqtt, const String& deviceId, const String& stateTopic) {
    struct Item {
        const char* id;
        const char* name;
        const char* tpl;
        const char* devClass;
        const char* unitM;
        const char* unitI;
        bool measurement;
    };
    static const Item items[] = {
        {"temperature", "Temperature", "{{ value_json.current.temp }}", "temperature", "°C", "°F", true},
        {"feels_like", "Feels like", "{{ value_json.current.feels_like }}", "temperature", "°C", "°F", true},
        {"temp_high", "High temperature", "{{ value_json.current.high }}", "temperature", "°C", "°F", true},
        {"temp_low", "Low temperature", "{{ value_json.current.low }}", "temperature", "°C", "°F", true},
        {"pressure", "Pressure", "{{ value_json.current.pressure }}", "atmospheric_pressure", "hPa", "inHg", true},
        {"wind_bft", "Wind Beaufort", "{{ value_json.current.wind_bft }}", nullptr, nullptr, nullptr, true},
        {"wind_direction", "Wind direction", "{{ value_json.current.wind_deg }}", nullptr, "°", "°", true},
        {"cloud_cover", "Cloud cover", "{{ value_json.current.clouds }}", nullptr, "%", "%", true},
        {"pop", "Precipitation probability", "{{ value_json.current.pop }}", nullptr, "%", "%", true},
        {"description", "Conditions", "{{ value_json.current.description }}", nullptr, nullptr, nullptr, false},
        {"icon", "Icon", "{{ value_json.current.icon }}", nullptr, nullptr, nullptr, false},
        {"sunrise", "Sunrise", "{{ value_json.current.sunrise }}", nullptr, nullptr, nullptr, false},
        {"sunset", "Sunset", "{{ value_json.current.sunset }}", nullptr, nullptr, nullptr, false},
        {"pm25", "PM2.5", "{{ value_json.air.pm2_5 }}", "pm25", "µg/m³", "µg/m³", true},
        {"pm10", "PM10", "{{ value_json.air.pm10 }}", "pm10", "µg/m³", "µg/m³", true},
        {"carbon_monoxide", "Carbon monoxide", "{{ value_json.air.co }}", "carbon_monoxide", "µg/m³", "µg/m³", true},
        {"nitrogen_dioxide", "Nitrogen dioxide", "{{ value_json.air.no2 }}", "nitrogen_dioxide", "µg/m³", "µg/m³", true},
        {"ozone", "Ozone", "{{ value_json.air.o3 }}", "ozone", "µg/m³", "µg/m³", true},
        {"sulphur_dioxide", "Sulphur dioxide", "{{ value_json.air.so2 }}", "sulphur_dioxide", "µg/m³", "µg/m³", true},
        {"battery", "Battery", "{{ value_json.device.battery_pct }}", "battery", "%", "%", true},
        {"battery_voltage", "Battery voltage", "{{ value_json.device.battery_voltage }}", "voltage", "V", "V", true},
        {"wake_count", "Wake count", "{{ value_json.device.wake_count }}", nullptr, nullptr, nullptr, true},
    };

    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        const Item& it = items[i];
        JsonDocument doc;
        doc["name"] = it.name;
        doc["unique_id"] = deviceId + "_" + it.id;
        doc["state_topic"] = stateTopic;
        doc["value_template"] = it.tpl;
        if (it.measurement) doc["state_class"] = "measurement";
        if (it.devClass && it.devClass[0]) doc["device_class"] = it.devClass;
        if (it.unitM && it.unitM[0]) doc["unit_of_measurement"] = it.unitM;
        JsonObject dev = doc["device"].to<JsonObject>();
        dev["identifiers"].to<JsonArray>().add(deviceId);
        dev["name"] = "TTGO T5 Weather";
        dev["manufacturer"] = "LilyGo";
        dev["model"] = "T5 2.13";
        if (mqttCity.length()) dev["suggested_area"] = mqttCity;
        String payload;
        serializeJson(doc, payload);
        String topic = "homeassistant/sensor/" + deviceId + "/" + it.id + "/config";
        if (!mqttPublishRetained(mqtt, topic, payload)) {
            infoPrintln("MQTT discovery failed for " + String(it.id));
        }
    }

    JsonDocument snap;
    snap["name"] = "Weather";
    snap["unique_id"] = deviceId + "_snapshot";
    snap["state_topic"] = stateTopic;
    snap["value_template"] = "{{ value_json.current.description }}";
    snap["json_attributes_topic"] = stateTopic;
    JsonObject dev = snap["device"].to<JsonObject>();
    dev["identifiers"].to<JsonArray>().add(deviceId);
    dev["name"] = "TTGO T5 Weather";
    dev["manufacturer"] = "LilyGo";
    dev["model"] = "T5 2.13";
    if (mqttCity.length()) dev["suggested_area"] = mqttCity;
    String payload;
    serializeJson(snap, payload);
    String topic = "homeassistant/sensor/" + deviceId + "/snapshot/config";
    if (!mqttPublishRetained(mqtt, topic, payload)) {
        infoPrintln("MQTT discovery failed for snapshot");
    }
}

inline bool mqttPublishWeather(const String& body) {
    if (!body.length()) return false;
    if (body.length() + 160 > MQTT_FRAME_MAX) {
        infoPrintln("MQTT payload is too large (" + String(body.length()) + " bytes)");
        return false;
    }

    MqttEndpoint ep;
    if (!parseHaHost(mqttSettings.HaHost, ep)) {
        infoPrintln("MQTT skipped, home_assistant_server_host is invalid");
        return false;
    }
    bool idSet = mqttSettings.CfAccessId.length() > 0;
    bool secretSet = mqttSettings.CfAccessSecret.length() > 0;
    if (idSet != secretSet) {
        infoPrintln("MQTT skipped, set both CF-Access-Client-Id and CF-Access-Client-Secret");
        return false;
    }

    char slug[8];
    snprintf(slug, sizeof(slug), "%06x", (unsigned)(ESP.getEfuseMac() & 0xFFFFFF));
    String deviceId = String("ttgo-t5-213-") + slug;
    String stateTopic = "ttgo/t5/" + String(slug) + "/weather";

    CloudflareMqttClient ws;
    ws.prepare(ep.path, mqttSettings.CfAccessId, mqttSettings.CfAccessSecret);
    PubSubClient mqtt(ws);
    mqtt.setServer(ep.host.c_str(), ep.port);
    mqtt.setSocketTimeout(20);
    mqtt.setKeepAlive(30);
    if (!mqtt.setBufferSize((uint16_t)(body.length() + 160))) {
        infoPrintln("MQTT buffer allocation failed");
        return false;
    }

    infoPrintln("MQTT connecting " + ep.host + ":" + String(ep.port) + ep.path
        + (idSet ? " with Cloudflare Access" : ""));
    bool ok = mqttSettings.MqttUser.length()
        ? mqtt.connect(deviceId.c_str(), mqttSettings.MqttUser.c_str(), mqttSettings.MqttPass.c_str())
        : mqtt.connect(deviceId.c_str());
    if (!ok) {
        infoPrintln("MQTT connect failed, " + String(mqttStateText(mqtt.state())));
        mqtt.disconnect();
        return false;
    }

    publishHaDiscovery(mqtt, deviceId, stateTopic);
    bool published = mqttPublishRetained(mqtt, stateTopic, body);
    if (published) infoPrintln("MQTT weather published to " + stateTopic + " (" + String(body.length()) + " bytes)");
    else infoPrintln("MQTT weather publish failed");

    mqtt.disconnect();
    return published;
}

#endif
