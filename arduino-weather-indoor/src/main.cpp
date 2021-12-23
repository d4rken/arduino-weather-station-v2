#include "uMQTTBroker.h"
#include <../../Config.h>
#include <ESP8266WiFi.h>
#include <gfxfont.h> 
#include <Fonts/FreeSans12pt7b.h>
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <Wire.h>

/*
 * Your WiFi config here
 */
char ssid[] = WIFI_SSID; // your network SSID (name)
char pass[] = WIFI_PW;   // your network password
bool WiFiAP = false;     // Do yo want the ESP as AP?

long outdoorLastSeenMillis = 0;
long outdoorUptimeMillis = 0;
int outdoorWifiRssi = 0;
float outdoorBatteryVoltage = 0;
float outdoorBatteryPercent = 0;

float outdoorSensorTempCelsus = 0;
float outdoorSensorHumidityPercent = 0;
float outdoorSensorPressurehPa = 0;

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(SS, 0, 5, 4)); // CS, DC, RST, Busy  // GDEH0154D67

/*
 * Custom broker class with overwritten callback functions
 */
class myMQTTBroker : public uMQTTBroker {
public:
    virtual bool onConnect(IPAddress addr, uint16_t client_count) {
        Serial.println(addr.toString() + " connected");
        return true;
    }

    virtual void onDisconnect(IPAddress addr, String client_id) {
        Serial.println(addr.toString() + " (" + client_id + ") disconnected");
    }

    virtual bool onAuth(String username, String password, String client_id) {
        Serial.println("Username/Password/ClientId: " + username + "/" + password + "/" + client_id);
        return true;
    }
    char buffer[256];
    virtual void onData(String topic, const char *data, uint32_t length) {
        char data_str[length + 1];
        os_memcpy(data_str, data, length);
        data_str[length] = '\0';

        if (topic == "weather-station/outdoor1/wifi/rssi") {
            outdoorLastSeenMillis = millis();
            outdoorWifiRssi = atoi(data_str);
        } else if (topic == "weather-station/outdoor1/battery/voltage") {
            outdoorBatteryVoltage = atof(data_str);
        } else if (topic == "weather-station/outdoor1/battery/percent") {
            outdoorBatteryPercent = atof(data_str);
        } else if (topic == "weather-station/outdoor1/uptime/milliseconds") {
            outdoorUptimeMillis = atoi(data_str);
        } else if (topic == "weather-station/outdoor1/sensors/temperature/celsius") {
            outdoorSensorTempCelsus = atof(data_str);
        } else if (topic == "weather-station/outdoor1/sensors/humidity/percent") {
            outdoorSensorHumidityPercent = atof(data_str);
        } else if (topic == "weather-station/outdoor1/sensors/pressure/hPa") {
            outdoorSensorPressurehPa = atof(data_str);
        }
        Serial.println("received topic '" + topic + "' with data '" + (String)data_str + "'");
        //printClients();
    }

    virtual void printClients() {
        for (int i = 0; i < getClientCount(); i++) {
            IPAddress addr;
            String client_id;

            getClientAddr(i, addr);
            getClientId(i, client_id);
            Serial.println("Client " + client_id + " on addr: " + addr.toString());
        }
    }
};

myMQTTBroker myBroker;

void startWiFiClient() {
    Serial.println("Connecting to " + (String)ssid);
    WiFi.mode(WIFI_STA);
    WiFi.hostname("Weather-Station-Indoor");
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("WiFi connected");
    Serial.println("IP address: " + WiFi.localIP().toString());
}

void setup() {
    Serial.begin(74880);
    Serial.println("Starting setup");

    display.init();
    display.setRotation(3);
    display.setFullWindow();
    display.setFont(&FreeSans12pt7b);
    display.setTextColor(GxEPD_BLACK);

    display.fillScreen(GxEPD_WHITE);
    display.setCursor(5, 60);
    display.print("Booting...");
    display.display(false);

    startWiFiClient();

    Serial.println("Starting MQTT broker");
    myBroker.init();

    myBroker.subscribe("#");
}

int printHeight = 0;

void printLine(String str) {
    int str_len = str.length() + 1;
    char msg[str_len];
    str.toCharArray(msg, str_len);

    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(msg, 0, 0, &tbx, &tby, &tbw, &tbh);

    printHeight += 22;
    display.setCursor(0, printHeight);
    display.print(msg);
}

void doDisplay() {
    display.display();
    printHeight = 0;
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Wifi is connected");
    } else {
        Serial.println("Wifi is not connected, attempting reconnect");
        WiFi.reconnect();
    }

    myBroker.publish("broker/clients/count", (String)myBroker.getClientCount());
    for (int i = 0; i < myBroker.getClientCount(); i++) {
        IPAddress addr;
        String client_id;

        myBroker.getClientAddr(i, addr);
        myBroker.getClientId(i, client_id);

        myBroker.publish("broker/clients/" + (String)client_id, (String)addr.toString());
    }

    myBroker.publish("broker/uptime", (String)millis());

    display.fillScreen(GxEPD_WHITE);
    printLine("Outdoor station 1");

    String upTimeString;
    if (outdoorUptimeMillis > 86400000) {
        upTimeString = String(outdoorUptimeMillis / 86400000) + "days";
    } else {
        upTimeString = String((outdoorUptimeMillis / 1000) / 60) + "min";
    }
    long lastSeenSeconds = (millis() - outdoorLastSeenMillis) / 1000;
    if (outdoorLastSeenMillis <= 0) {
        printLine("Not seen yet.");
    } else {
        printLine(String(lastSeenSeconds) + "s ago (Up " + upTimeString + ")");

        int wifi = 100 - outdoorWifiRssi * -1;
        printLine("Wifi: " + String(wifi) + "% (" + String(WiFi.RSSI()) + "%)");
        printLine("Batt: " + String((int)outdoorBatteryPercent) + "% @ " + String(outdoorBatteryVoltage) + "V");

        printLine("");

        printLine("Temp: " + String(outdoorSensorTempCelsus) + "C");
        printLine("Humid: " + String(outdoorSensorHumidityPercent) + "%");
        printLine("Baro: " + String(outdoorSensorPressurehPa) + "hPa");
    }

    doDisplay();

    delay(30 * 1000);
}
