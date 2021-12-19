#include "Adafruit_Si7021.h"
#include "uMQTTBroker.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <ESP8266WiFi.h>
#include <OneButton.h>
#include <SPI.h>
#include <Wire.h>
#include <../../Config.h>

/*
 * Your WiFi config here
 */
char ssid[] = WIFI_SSID; // your network SSID (name)
char pass[] = WIFI_PW;     // your network password
bool WiFiAP = false;                // Do yo want the ESP as AP?

long outdoorLastSeenMillis = 0;
long outdoorUptimeMillis = 0;
int outdoorWifiRssi = 0;
float outdoorBatteryVoltage = 0;
float outdoorBatteryPercent = 0;

float outdoorSensorTempCelsus = 0;
float outdoorSensorHumidityPercent = 0;
float outdoorSensorPressurehPa = 0;

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

Adafruit_Si7021 sensor = Adafruit_Si7021();

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET 0 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

OneButton buttonA = OneButton(
    D3,   // Input pin for the button
    true, // Button is active LOW
    true  // Enable internal pull-up resistor
);

OneButton buttonB = OneButton(
    D4,   // Input pin for the button
    true, // Button is active LOW
    true  // Enable internal pull-up resistor
);

void startWiFiClient() {
    Serial.println("Connecting to " + (String)ssid);
    WiFi.mode(WIFI_STA);
    WiFi.hostname("Weather-Station-Indoor");
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");

    Serial.println("WiFi connected");
    Serial.println("IP address: " + WiFi.localIP().toString());
}

void startWiFiAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, pass);
    Serial.println("AP started");
    Serial.println("IP address: " + WiFi.softAPIP().toString());
}

void setup() {
    Serial.begin(74880);
    Serial.println();

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    Serial.print("Found model ");
    switch (sensor.getModel()) {
    case SI_Engineering_Samples:
        Serial.print("SI engineering samples");
        break;
    case SI_7013:
        Serial.print("Si7013");
        break;
    case SI_7020:
        Serial.print("Si7020");
        break;
    case SI_7021:
        Serial.print("Si7021");
        break;
    case SI_UNKNOWN:
    default:
        Serial.print("Unknown");
    }

    Serial.print(" Rev(");
    Serial.print(sensor.getRevision());
    Serial.print(")");
    Serial.print(" Serial #");
    Serial.print(sensor.sernum_a, HEX);
    Serial.println(sensor.sernum_b, HEX);

    display.clearDisplay();

    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);

    display.println("Weather");
    display.println("Station");
    display.setTextSize(1);
    display.println("Booting...");
    display.display();

    if (WiFiAP)
        startWiFiAP();
    else
        startWiFiClient();

    Serial.println("Starting MQTT broker");
    myBroker.init();

    myBroker.subscribe("#");
}

int heaterCounter = 0;
bool enableHeater = false;

void loop() {
    buttonA.tick();
    buttonB.tick();

    myBroker.publish("broker/clients/count", (String)myBroker.getClientCount());
    for (int i = 0; i < myBroker.getClientCount(); i++) {
        IPAddress addr;
        String client_id;

        myBroker.getClientAddr(i, addr);
        myBroker.getClientId(i, client_id);

        myBroker.publish("broker/clients/" + (String)client_id, (String)addr.toString());
    }

    myBroker.publish("broker/uptime", (String)millis());

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);

    if (buttonA.isLongPressed()) {
        display.println("This station");
        String upTimeString;
        if (millis() > 86400000) {
            upTimeString = String(millis() / 86400000) + "days";
        } else {
            upTimeString = String((millis() / 1000) / 60) + "min";
        }
        long lastSeenSeconds = (millis() - outdoorLastSeenMillis) / 1000;

        display.println("Uptime: " + upTimeString);
        int wifi = 100 - WiFi.RSSI() * -1;
        display.println("WLN " + String(wifi) + "% Clients " + myBroker.getClientCount());
        display.println();
        display.println("Temperature " + String(sensor.readTemperature()) + "C");
        display.println("Humidity " + String(sensor.readHumidity()) + "%");
    } else {
        display.println("Outdoor station 1");
        String upTimeString;
        if (outdoorUptimeMillis > 86400000) {
            upTimeString = String(outdoorUptimeMillis / 86400000) + "days";
        } else {
            upTimeString = String((outdoorUptimeMillis / 1000) / 60) + "min";
        }
        long lastSeenSeconds = (millis() - outdoorLastSeenMillis) / 1000;

        display.println(String(lastSeenSeconds) + "s ago, Up: " + upTimeString);
        int wifi = 100 - outdoorWifiRssi * -1;
        display.println("WLN " + String(wifi) + "% Bat " + String((int)outdoorBatteryPercent) + "% " + String(outdoorBatteryVoltage) + "V");
        display.println();
        display.println("Temperature " + String(outdoorSensorTempCelsus) + "C");
        display.println("Humidity " + String(outdoorSensorHumidityPercent) + "%");
        display.println("Barometer " + String(outdoorSensorPressurehPa) + "hPa");
    }

    if (++heaterCounter == 30) {
        enableHeater = !enableHeater;
        sensor.heater(enableHeater);
        Serial.print("Heater Enabled State: ");
        if (sensor.isHeaterEnabled())
            Serial.println("ENABLED");
        else
            Serial.println("DISABLED");

        heaterCounter = 0;
    }

    display.display();

    delay(1000);
}