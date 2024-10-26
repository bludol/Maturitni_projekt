#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SimpleDHT.h>  // Knihovna SimpleDHT pro čtení z DHT senzoru
#include <LittleFS.h>
#include "password.h"

#define DHTPIN 0         // DHT senzor je připojen na GPIO0 (D3 na Wemos D1 Mini)
#define WATER_SENSOR_PIN A0  // Pin pro analogový Water Level Sensor

SimpleDHT11 dht11(DHTPIN);  // Inicializace DHT11 senzoru

AsyncWebServer server(80);  // Inicializace asynchronního webového serveru na portu 80

unsigned long lastDHTRead = 0;  // Čas posledního čtení DHT senzoru
const unsigned long dhtDelay = 2000;  // Zpoždění mezi čteními (2 sekundy)
float temperature = 0;
float humidity = 0;

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
    pinMode(WATER_SENSOR_PIN, INPUT);  // Nastavení pinu pro Water Level Sensor jako vstup

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

            response.replace("{{temperature}}", String(temperature));
            response.replace("{{humidity}}", String(humidity));
            response.replace("{{waterLevel}}", String(waterLevel));
            response.replace("{{soilMoisture}}", "Načítání...");

            request->send(200, "text/html", response);
        } else {
            request->send(404, "text/plain", "404: Not Found");
        }
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
    yield();
}