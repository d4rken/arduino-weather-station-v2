#include "uMQTTBroker.h"
#include <../../Config.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include "epd1in54_V2.h"
#include "epdpaint.h"
#include <GxEPD2_BW.h>
#include <Fonts/FreeSans12pt7b.h>

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

    // Sample for the usage of the client info methods

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
    WiFi.hostname("Weather-Station-Indoor2");
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
  display.setRotation(0);
  display.setFullWindow();

    startWiFiClient();

    Serial.println("Starting MQTT broker");
    myBroker.init();

    myBroker.subscribe("#");
}


void updateHum(float humidity) {
  char temp_string[] = {'0', '0', '\0'};
  int16_t tbx, tby;
  uint16_t tbw, tbh;

  // Zahlen in String schreiben
  dtostrf(humidity, 2, 0, temp_string);

  // x und y = linke UNTERE Ecke (Grundlinie)
  uint16_t x = 30;
  uint16_t y = 112;

  // berechnet die Größe des Fensters
  display.getTextBounds(temp_string, x, y, &tbx, &tby, &tbw, &tbh);
  
  display.setFont(&FreeSans12pt7b);
  display.setTextColor(GxEPD_BLACK);

  // benötigt die linke OBERE Ecke und Groesse
  display.setPartialWindow(tbx, tby, tbw, tbh);

  // Ausgabe
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(temp_string);
  } while (display.nextPage());
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

updateHum(23.0);
  
        //display.println("Outdoor station 1");
        String upTimeString;
        if (outdoorUptimeMillis > 86400000) {
            upTimeString = String(outdoorUptimeMillis / 86400000) + "days";
        } else {
            upTimeString = String((outdoorUptimeMillis / 1000) / 60) + "min";
        }
        long lastSeenSeconds = (millis() - outdoorLastSeenMillis) / 1000;
        if (outdoorLastSeenMillis > 0) {
            //display.println(String(lastSeenSeconds) + "s ago, Up: " + upTimeString);
        } else {
            //display.println("Not seen yet");
        }

        int wifi = 100 - outdoorWifiRssi * -1;
        //display.println("WLN " + String(wifi) + "% Bat " + String((int)outdoorBatteryPercent) + "% " + String(outdoorBatteryVoltage) + "V");
        //display.println();
        //display.println("Temperature " + String(outdoorSensorTempCelsus) + "C");
        //display.println("Humidity " + String(outdoorSensorHumidityPercent) + "%");
        //display.println("Barometer " + String(outdoorSensorPressurehPa) + "hPa");

    delay(1000);
}
