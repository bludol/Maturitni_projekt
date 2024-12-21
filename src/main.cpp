#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SimpleDHT.h>
#include "password.h"  // Soubor pro SSID a heslo WiFi

// Nastavení pro senzory a pumpu
#define DHTPIN 0 // DHT senzor na GPIO0 (D3 na Wemos D1 Mini)
#define WATER_SENSOR_PIN A0 // Pin pro analogový Water Level Sensor
#define SOIL_SENSOR_PIN 13 // Digitální pin pro HW-080 přes HW-103 (GPIO13)
#define PUMP_PIN 2 // Pin pro spínání pumpy (GPIO2)

// Nastavení MQTT
const char* mqtt_server = "10.0.0.70";  
const char* mqtt_user = "homeassistant";
const char* mqtt_pass = "Heslo1234";
const char* temperature_topic = "homeassistant/sensor/temperature/state";
const char* humidity_topic = "homeassistant/sensor/humidity/state";
const char* water_level_topic = "homeassistant/sensor/water_level/state";
const char* soil_moisture_topic = "homeassistant/sensor/soil_moisture/state";
const char* pump_control_topic = "home/plant/pump/set";
const char* pump_state_topic = "home/plant/pump/state";

// MQTT klient a DHT senzor
WiFiClient espClient;
PubSubClient client(espClient);
SimpleDHT11 dht11(DHTPIN);

// Proměnné pro senzory a stav pumpy
float temperature = 0;
float humidity = 0;
bool pumpState = false;

// Funkce pro odesílání MQTT Discovery zpráv
void sendDiscoveryMessages() {
    String temperatureConfig = "{\"name\":\"Temperature\",\"state_topic\":\"homeassistant/sensor/temperature/state\",\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\",\"qos\":0}";
    String humidityConfig = "{\"name\":\"Humidity\",\"state_topic\":\"homeassistant/sensor/humidity/state\",\"unit_of_measurement\":\"%\",\"device_class\":\"humidity\",\"qos\":0}";
    String soilMoistureConfig = "{\"name\":\"Soil Moisture\",\"state_topic\":\"homeassistant/sensor/soil_moisture/state\",\"unit_of_measurement\":\"%\",\"qos\":0}";
    
    client.publish("homeassistant/sensor/temperature/config", temperatureConfig.c_str(), true);
    client.publish("homeassistant/sensor/humidity/config", humidityConfig.c_str(), true);
    client.publish("homeassistant/sensor/soil_moisture/config", soilMoistureConfig.c_str(), true);
}

// Funkce pro čtení senzorů a odesílání hodnot na MQTT
void readSensors() {
    byte temp = 0;
    byte hum = 0;
    int err = dht11.read(&temp, &hum, NULL);
    if (err == SimpleDHTErrSuccess) {
        temperature = temp;
        humidity = hum;

        // Výpis na serial monitor
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.print(" °C, Humidity: ");
        Serial.print(humidity);
        Serial.println(" %");

        // Převod na string a odeslání
        String tempStr = String(temperature);
        client.publish(temperature_topic, tempStr.c_str(), true);

        String humStr = String(humidity);
        client.publish(humidity_topic, humStr.c_str(), true);
    }

    // Čtení úrovně vody
    int waterLevel = analogRead(WATER_SENSOR_PIN);
    String waterStr = String(waterLevel);

    // Výpis na serial monitor
    Serial.print("Water level: ");
    Serial.println(waterLevel);
    client.publish(water_level_topic, waterStr.c_str(), true);

    // Čtení vlhkosti půdy
    int soilMoisture = digitalRead(SOIL_SENSOR_PIN);
    String soilStr = soilMoisture ? "1" : "0";

    // Výpis na serial monitor
    Serial.print("Soil moisture: ");
    Serial.println(soilMoisture);
    client.publish(soil_moisture_topic, soilStr.c_str(), true);
}

// Callback funkce pro zpracování příchozích MQTT zpráv
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    if (String(topic) == pump_control_topic) {
        if (message == "ON") {
            digitalWrite(PUMP_PIN, HIGH);
            pumpState = true;
            client.publish(pump_state_topic, "ON", true);
        } else if (message == "OFF") {
            digitalWrite(PUMP_PIN, LOW);
            pumpState = false;
            client.publish(pump_state_topic, "OFF", true);
        }
    }
}

// Funkce pro připojení k MQTT brokeru
void reconnectMQTT() {
    while (!client.connected()) {
        Serial.print("Připojuji se k MQTT brokeru...");
        if (client.connect("ESP8266Client", mqtt_user, mqtt_pass)) {
            Serial.println("MQTT připojeno!");
            client.subscribe(pump_control_topic);
            sendDiscoveryMessages();  // Odeslání Discovery zpráv
        } else {
            Serial.print("MQTT chyba, zkusím znovu za 5 sekund. Kód: ");
            Serial.println(client.state());
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(WATER_SENSOR_PIN, INPUT);
    pinMode(SOIL_SENSOR_PIN, INPUT);
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);

    // WiFi připojení
    Serial.print("Připojuji se k WiFi...");
    WiFi.begin(ssid, password);  // Ujistěte se, že ssid a password jsou definovány v password.h
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi připojeno");

    // Vypíše IP adresu ESP
    Serial.print("ESP IP adresa: ");
    Serial.println(WiFi.localIP());

    // MQTT nastavení
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);  // Připojení callback funkce pro MQTT
}

void loop() {
    if (!client.connected()) {
        reconnectMQTT();  // Funkce pro opětovné připojení k MQTT
    }
    client.loop();

    static unsigned long lastRead = 0;
    if (millis() - lastRead > 60000) { // Každou minutu
        lastRead = millis();
        readSensors();  // Funkce pro čtení senzorů
    }
}