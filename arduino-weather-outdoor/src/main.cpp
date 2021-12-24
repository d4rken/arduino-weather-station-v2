#include <../../Config.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>

//for LED status
#include <Ticker.h>
Ticker ticker;

const char *SSID = WIFI_SSID;
const char *PSK = WIFI_PW;
const char *MQTT_BROKER = MQTT_SERVER;
const char *MQTT_BROKER_IP = MQTT_SERVER_IP;

WiFiClient espClient;
PubSubClient client(espClient);

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;

// CRC function used to ensure data validity
uint32_t calculateCRC32(const uint8_t *data, size_t length);

// helper function to dump memory contents as hex
void printMemory();

// Structure which will be stored in RTC memory.
// First field is CRC32, which is calculated based on the
// rest of structure contents.
// Any fields can go after CRC32.
// We use byte array as an example.
struct {
    unsigned long millis;
} rtcDataStruct;

struct {
    uint32_t crc32;
    byte data[508];
} rtcData;

void tick() {
    //toggle state
    int state = digitalRead(BUILTIN_LED); // get the current state of GPIO1 pin
    digitalWrite(BUILTIN_LED, !state);    // set pin to the opposite state
}

void setup() {
    Serial.begin(74880);
    Serial.println("Setup...");

    pinMode(BUILTIN_LED, OUTPUT);
    ticker.attach(0.6, tick);

    // Read struct from RTC memory
    if (ESP.rtcUserMemoryRead(0, (uint32_t *)&rtcData, sizeof(rtcData))) {
        Serial.println("Reading rtcData");
        //Serial.println("Read: ");
        //printMemory();
        //Serial.println();
        uint32_t crcOfData = calculateCRC32((uint8_t *)&rtcData.data[0], sizeof(rtcData.data));
        Serial.print("CRC32 of data: ");
        Serial.println(crcOfData, HEX);
        Serial.print("CRC32 read from RTC: ");
        Serial.println(rtcData.crc32, HEX);
        if (crcOfData != rtcData.crc32) {
            Serial.println("CRC32 in RTC memory doesn't match CRC32 of data. Data is probably invalid!");
        } else {
            Serial.println("CRC32 check ok, data is probably valid.");
            memcpy(&rtcDataStruct, rtcData.data, sizeof(rtcDataStruct));
        }
    }

    Serial.println("Connecting to WiFi" + String(SSID));
    WiFi.setAutoReconnect(true);
    WiFi.hostname("Weather-Station-Outdoor");
    WiFi.begin(SSID, PSK);

    client.setServer(MQTT_BROKER, 1883);

    if (!bme.begin(0x76)) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
    }

    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF);

    pinMode(A0, INPUT);
}

const float BAT_RAW_EMPTY = 625.0;
const float BAT_RAW_FULL = 1024.0;

char itoaBuf[64];
char dtostrfBuf[64];

int deepSleepMillis = 60000;

void updateSystemStats() {
    long rssi = WiFi.RSSI();
    client.publish("weather-station/outdoor1/wifi/rssi", itoa(rssi, itoaBuf, 10));
    Serial.println("RSSI: " + String(rssi));

    int batteryRaw = analogRead(A0);
    client.publish("weather-station/outdoor1/battery/raw", itoa(batteryRaw, itoaBuf, 10));

    float batPercent = (batteryRaw - BAT_RAW_EMPTY) / (BAT_RAW_FULL - BAT_RAW_EMPTY);
    if (batPercent > 0.95) {
        deepSleepMillis = deepSleepMillis * 0.25;
    } else if (batPercent > 0.90) {
        deepSleepMillis = deepSleepMillis * 0.50;
    } else if (batPercent > 0.80) {
        deepSleepMillis = deepSleepMillis * 1;
    } else if (batPercent > 0.70) {
        deepSleepMillis = deepSleepMillis * 2;
    } else if (batPercent > 0.60) {
        deepSleepMillis = deepSleepMillis * 3;
    } else if (batPercent > 0.50) {
        deepSleepMillis = deepSleepMillis * 5;
    } else {
        deepSleepMillis = deepSleepMillis * 10;
    }
    client.publish("weather-station/outdoor1/battery/percent", dtostrf(batPercent * 100, 4, 3, dtostrfBuf));

    float batVoltage = (batteryRaw / BAT_RAW_FULL ) * 4.1;
    client.publish("weather-station/outdoor1/battery/voltage", dtostrf(batVoltage, 4, 3, dtostrfBuf));
    Serial.println("Battery: " + String(batPercent * 100) + "% (" + String(batVoltage) + "V raw: " + String(batteryRaw) + ")");

    client.publish("weather-station/outdoor1/uptime/milliseconds", itoa(rtcDataStruct.millis + deepSleepMillis + millis(), itoaBuf, 10));
}

void updateSensor() {
    float temperature = bme.readTemperature();
    client.publish("weather-station/outdoor1/sensors/temperature/celsius", dtostrf(temperature, 4, 2, dtostrfBuf));
    Serial.println("Temperature = " + String(temperature) + "*C");

    float humidity = bme.readHumidity();
    client.publish("weather-station/outdoor1/sensors/humidity/percent", dtostrf(humidity, 4, 2, dtostrfBuf));
    Serial.println("Humidity = " + String(humidity) + "%");

    float pressure = bme.readPressure() / 100.0F;
    client.publish("weather-station/outdoor1/sensors/pressure/hPa", dtostrf(pressure, 4, 2, dtostrfBuf));
    Serial.println("Pressure = " + String(pressure) + "hpa");

    float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    client.publish("weather-station/outdoor1/sensors/pressure/altitude", dtostrf(altitude, 4, 2, dtostrfBuf));
    Serial.println("Approx. Altitude = " + String(altitude) + "m");

    float seaLevelForAltitude = bme.seaLevelForAltitude(174.0, pressure);
    client.publish("weather-station/outdoor1/sensors/pressure/seaLevelForAltitude", dtostrf(seaLevelForAltitude, 4, 2, dtostrfBuf));
    Serial.println("Sea level for altitude = " + String(seaLevelForAltitude) + "hPa");
}

void loop() {
    bool wifiConnected = true;
    int retryWifi = 0;
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connecting to WiFi...");
        if (retryWifi > 10) {
            wifiConnected = false;
            break;
        } else {
            retryWifi++;
        }
        delay(500);
    }

    if (wifiConnected) {
        Serial.println("Connected, my IP is:");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Failed to connect to WiFi");
    }

    bool mqttConnected = wifiConnected;
    int retryMqtt = 0;
    while (wifiConnected && !client.connected()) {
        Serial.println("Connecting to MQTT broker...");
        client.connect("Weather-Station-Outdoor");
        if (retryMqtt > 3) {
            mqttConnected = false;
            break;
        } else {
            if (retryMqtt == 2) {
                Serial.println("MQTT DNS not resolved, trying IP...");
                client.disconnect();
                client.setServer(MQTT_BROKER_IP, 1883);
            }
            retryMqtt++;
        }
        delay(500);
    }

    if (mqttConnected) {
        Serial.println("Connected to MQTT Broker");
    } else {
        Serial.println("Failed to connect to MQTT broker");
    }

    ticker.detach();
    digitalWrite(BUILTIN_LED, LOW);

    updateSystemStats();
    updateSensor();

    client.loop();

    delay(1000);

    Serial.println("Data published, waiting for transmission...");
    client.disconnect();
    espClient.flush();

    // wait until connection is closed completely
    int delayCounter = 0;
    while (client.state() != -1) {
        delay(10);
        if (delayCounter > 100) {
            break;
        } else {
            delayCounter++;
        }
    }
    Serial.println("Network flushed and disconnected, going to deep sleep...");

    // Generate new data set for the struct
    rtcDataStruct.millis = rtcDataStruct.millis + deepSleepMillis + millis();
    memcpy(rtcData.data, &rtcDataStruct, sizeof(rtcDataStruct));
    // Update CRC32 of data
    rtcData.crc32 = calculateCRC32((uint8_t *)&rtcData.data[0], sizeof(rtcData.data));
    // Write struct to RTC memory
    if (ESP.rtcUserMemoryWrite(0, (uint32_t *)&rtcData, sizeof(rtcData))) {
        Serial.println("rtcData written");
        //Serial.println("Write: ");
        //printMemory();
        //Serial.println();
    }

    unsigned long int actSleepTime = deepSleepMillis * 1000; //microseconds
    Serial.println("Going to sleep for " + String(deepSleepMillis) + "ms");
    ESP.deepSleep(actSleepTime);
}

uint32_t calculateCRC32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xffffffff;
    while (length--) {
        uint8_t c = *data++;
        for (uint32_t i = 0x80; i > 0; i >>= 1) {
            bool bit = crc & 0x80000000;
            if (c & i) {
                bit = !bit;
            }
            crc <<= 1;
            if (bit) {
                crc ^= 0x04c11db7;
            }
        }
    }
    return crc;
}

//prints all rtcData, including the leading crc32
void printMemory() {
    char buf[3];
    uint8_t *ptr = (uint8_t *)&rtcData;
    for (size_t i = 0; i < sizeof(rtcData); i++) {
        sprintf(buf, "%02X", ptr[i]);
        Serial.print(buf);
        if ((i + 1) % 32 == 0) {
            Serial.println();
        } else {
            Serial.print(" ");
        }
    }
    Serial.println();
}