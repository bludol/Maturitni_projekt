#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SimpleDHT.h>
#include <LittleFS.h>
#include "password.h"

#define DHTPIN 0              // DHT senzor na GPIO0 (D3 na Wemos D1 Mini)
#define WATER_SENSOR_PIN A0   // Pin pro analogový Water Level Sensor
#define PUMP_PIN 2            // Pin pro spínání pumpy (GPIO2)
#define SOIL_SENSOR_PIN 13    // Digitální pin pro HW-080 přes HW-103 (GPIO15)

SimpleDHT11 dht11(DHTPIN);
AsyncWebServer server(80);

unsigned long lastDHTRead = 0;
const unsigned long dhtDelay = 2000;
float temperature = 0;
float humidity = 0;
bool pumpState = false;       // Stav pumpy, false = vypnuto, true = zapnuto
bool soilMoisture = false;    // Stav vlhkosti půdy, false = sucho, true = vlhko
const int WATER_LEVEL_NEEDED = 200; // Minimální hodnota vody pro spuštění pumpy
const int PUMP_RUN_TIME_MS = 2000;     // Doba běhu pumpy (v milisekundách)
unsigned long lastPumpRun = 0;         // Čas posledního spuštění pumpy

void listFiles() {
    Serial.println("Výpis souborů v LittleFS:");
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        String fileName = dir.fileName();
        Serial.print(" - ");
        Serial.println(fileName);
        yield();
    }
    Serial.println("Konec výpisu souborů.");
}

void setup() {
    Serial.begin(115200);
    pinMode(WATER_SENSOR_PIN, INPUT);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(SOIL_SENSOR_PIN, INPUT);
    digitalWrite(PUMP_PIN, LOW);

    Serial.print("Připojuji se k WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        yield();
    }
    Serial.println("WiFi připojeno");
    Serial.print("IP adresa sítě: ");
    Serial.println(WiFi.localIP());

    if (!LittleFS.begin()) {
        Serial.println("Chyba při inicializaci LittleFS!");
        return;
    }
    Serial.println("LittleFS inicializován.");
    listFiles();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open("/index.html", "r");
        if (file) {
            String response = file.readString();
            file.close();

            unsigned long currentMillis = millis();
            if (currentMillis - lastDHTRead >= dhtDelay) {
                lastDHTRead = currentMillis;
                byte tempByte = 0;
                byte humByte = 0;
                int err = dht11.read(&tempByte, &humByte, NULL);
                if (err == SimpleDHTErrSuccess) {
                    temperature = tempByte;
                    humidity = humByte;
                } else {
                    temperature = 0;
                    humidity = 0;
                    Serial.println("Chyba při čtení z DHT senzoru!");
                }
            }

            int waterLevel = analogRead(WATER_SENSOR_PIN);
            soilMoisture = digitalRead(SOIL_SENSOR_PIN);

            response.replace("{{temperature}}", String(temperature));
            response.replace("{{humidity}}", String(humidity));
            response.replace("{{waterLevel}}", String(waterLevel));
            response.replace("{{soilMoisture}}", soilMoisture ? "Vlhká" : "Suchá");
            response.replace("{{pumpState}}", pumpState ? "Zapnuto" : "Vypnuto");

            request->send(200, "text/html", response);
        } else {
            request->send(404, "text/plain", "404: Not Found");
        }
    });

    server.on("/togglePump", HTTP_GET, [](AsyncWebServerRequest *request) {
        pumpState = !pumpState;
        digitalWrite(PUMP_PIN, pumpState ? HIGH : LOW);
        request->send(200, "text/plain", pumpState ? "Pumpa zapnuta" : "Pumpa vypnuta");
    });

    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open("/styles.css", "r");
        if (file) {
            request->send(LittleFS, "/styles.css", "text/css");
            file.close();
        } else {
            request->send(404, "text/plain", "404: Not Found");
        }
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open("/script.js", "r");
        if (file) {
            request->send(LittleFS, "/script.js", "application/javascript");
            file.close();
        } else {
            request->send(404, "text/plain", "404: Not Found");
        }
    });

    server.begin();
}

void loop() {
    // Čtení vlhkosti půdy a hladiny vody
    int waterLevel = analogRead(WATER_SENSOR_PIN);
    soilMoisture = digitalRead(SOIL_SENSOR_PIN);

    // Automatické zavlažování
    if (!soilMoisture && waterLevel > WATER_LEVEL_NEEDED) {
        unsigned long currentMillis = millis();
        
        if (currentMillis - lastPumpRun >= PUMP_RUN_TIME_MS) {
            digitalWrite(PUMP_PIN, HIGH); // Zapne pumpu
            delay(PUMP_RUN_TIME_MS);      // Spustí na stanovenou dobu
            digitalWrite(PUMP_PIN, LOW);  // Vypne pumpu
            pumpState = false;
            lastPumpRun = currentMillis; // Aktualizace času posledního spuštění
        }
    } else if (waterLevel <= WATER_LEVEL_NEEDED) {
        Serial.println("Nedostatek vody v zásobníku!");
    }

    yield();
}