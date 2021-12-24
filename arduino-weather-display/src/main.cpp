#include <../../Config.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <gfxfont.h>
#include <Fonts/FreeSans12pt7b.h>
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <Wire.h>

const char *SSID = WIFI_SSID;
const char *PSK = WIFI_PW;
const char *MQTT_BROKER = MQTT_SERVER;
const char *MQTT_BROKER_IP = MQTT_SERVER_IP;
const char *MQTT_CLIENT_NAME = "Weather-Station-Display";

long outdoorLastSeenMillis = 0;
long outdoorUptimeMillis = 0;
int outdoorWifiRssi = 0;
float outdoorBatteryVoltage = 0;
float outdoorBatteryPercent = 0;

float outdoorSensorTempCelsus = 0;
float outdoorSensorHumidityPercent = 0;
float outdoorSensorPressurehPa = 0;

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(SS, 0, 5, 4)); // CS, DC, RST, Busy  // GDEH0154D67

WiFiClient espClient;
PubSubClient client(espClient);

char buffer[256];

void mqttCallback(char* _topic, byte* payload, unsigned int length) {
    String topic = String(_topic);
    char data_str[length + 1];
    os_memcpy(data_str, payload, length);
    data_str[length] = '\0';

    Serial.println("Received topic '" + topic + "' with data '" + (String)data_str + "'");

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
}

int printHeight = 0;

void displayStart() {
    printHeight = 0;
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(0,0);
}

void displayLine(String str) {
    int str_len = str.length() + 1;
    char msg[str_len];
    str.toCharArray(msg, str_len);

    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(msg, 0, printHeight, &tbx, &tby, &tbw, &tbh);

    if(printHeight == 0) {
        printHeight += 18;
    } else {
        printHeight += 24;
    }
    display.setCursor(0, printHeight);
    display.print(msg);
}

void displayFinish() {
    display.display();
}

void setup() {
    Serial.begin(74880);
    Serial.println("Starting setup");

    display.init();
    display.setRotation(3);
    display.setFullWindow();
    display.setFont(&FreeSans12pt7b);
    display.setTextColor(GxEPD_BLACK);

    displayStart();
    displayLine("Booting...");
    display.display(false);

    Serial.println("Connecting to WiFi" + String(SSID));
    WiFi.setAutoReconnect(true);
    WiFi.hostname("Weather-Station-Display");
    WiFi.begin(SSID, PSK);

    client.setServer(MQTT_BROKER, 1883);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("WiFi connected");
    Serial.println("IP address: " + WiFi.localIP().toString());

    displayLine("Got WiFi!");
    display.display(false); 
    delay(1000);

    client.setServer(MQTT_BROKER, 1883);
    client.setCallback(mqttCallback);

    while(!client.connected()) {
        client.connect(MQTT_CLIENT_NAME);
        delay(500);
        Serial.print(".");
    }

    displayLine("MQTT connected");
    display.display(false);
    delay(1000);

    Serial.println("Subscribing to topics...");

    client.subscribe("weather-station/outdoor1/wifi/rssi");
    client.subscribe("weather-station/outdoor1/battery/voltage");
    client.subscribe("weather-station/outdoor1/battery/percent");
    client.subscribe("weather-station/outdoor1/uptime/milliseconds");
    client.subscribe("weather-station/outdoor1/sensors/temperature/celsius");
    client.subscribe("weather-station/outdoor1/sensors/humidity/percent");
    client.subscribe("weather-station/outdoor1/sensors/pressure/hPa");

    displayLine("Waiting for data");
    display.display(false);
    delay(1000);
}

long lastDisplayUpdate = 0;

void updateDisplay() {
    if(millis() - lastDisplayUpdate < 27 * 1000) {
        return;
    }

    Serial.println("Updating display");
    displayStart();
    displayLine("Outdoor station 1");

    long lastSeenSeconds = (millis() - outdoorLastSeenMillis) / 1000;
    if (outdoorLastSeenMillis <= 0) {
        displayLine("Not seen yet.");
    } else {
        displayLine("Seen " + String(lastSeenSeconds) + "s ago");

        String upTimeString;
        if (outdoorUptimeMillis > 86400000) {
            upTimeString = String(outdoorUptimeMillis / 86400000) + " days";
        } else {
            upTimeString = String((outdoorUptimeMillis / 1000) / 60) + "min";
        }

        displayLine("Uptime " + upTimeString);

        int wifi = 100 - outdoorWifiRssi * -1;
        displayLine("Wifi: " + String(wifi) + "% (" + String(WiFi.RSSI()) + "%)");
        displayLine("Batt: " + String((int)outdoorBatteryPercent) + "% @ " + String(outdoorBatteryVoltage) + "V");

        displayLine("Temp: " + String(outdoorSensorTempCelsus) + "C");
        displayLine("Humid: " + String(outdoorSensorHumidityPercent) + "%");
        displayLine("Baro: " + String(outdoorSensorPressurehPa) + "hPa");
    }

    displayFinish();

    lastDisplayUpdate = millis();
}

void loop() {
    client.loop();

    updateDisplay();
}
