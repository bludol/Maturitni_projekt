#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>  // Knihovna pro Async web server
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <LittleFS.h>  // Knihovna pro práci s LittleFS
#include "password.h"  // Tento soubor by měl obsahovat definice ssid a password

#define DHTPIN 0         // DHT senzor je připojen na GPIO0 (D3 na Wemos D1 Mini)
#define DHTTYPE DHT11    // Používáte DHT11 senzor
#define WATER_SENSOR_PIN A0  // Pin pro analogový Water Level Sensor

DHT dht(DHTPIN, DHTTYPE);  // Inicializace DHT senzoru

AsyncWebServer server(80);  // Inicializace asynchronního webového serveru na portu 80

unsigned long lastDHTRead = 0;  // Čas posledního čtení DHT senzoru
const unsigned long dhtDelay = 2000;  // Zpoždění mezi čteními (2 sekundy)

// Funkce pro výpis souborů v LittleFS
void listFiles() {
    Serial.println("Výpis souborů v LittleFS:");
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        String fileName = dir.fileName();
        Serial.print(" - ");
        Serial.println(fileName);
        yield();  // Umožní zpracování systémových úkolů
    }
    Serial.println("Konec výpisu souborů.");
}

void setup() {
    Serial.begin(115200);
    pinMode(WATER_SENSOR_PIN, INPUT);  // Nastavení pinu pro Water Level Sensor jako vstup

    // Inicializace DHT senzoru
    dht.begin();

    Serial.print("Připojuji se k WiFi");
    WiFi.begin(ssid, password);
    
    // Čekání na připojení k WiFi
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        yield();  // Umožní zpracování systémových úkolů
    }
    Serial.println("WiFi připojeno");

    Serial.print("IP adresa sítě: ");
    Serial.println(WiFi.localIP());

    // Inicializace LittleFS
    if (!LittleFS.begin()) {
        Serial.println("Chyba při inicializaci LittleFS!");
        return;
    }
    Serial.println("LittleFS inicializován.");
    listFiles();

    // Obsluha požadavků na root nebo index.html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open("/index.html", "r");
        if (file) {
            String response = file.readString();
            file.close();

            // Načtení dat ze senzorů
            float temperature = 0;
            float humidity = 0;

            unsigned long currentMillis = millis(); // Získání aktuálního času

            // Čtení teploty a vlhkosti pouze pokud uplynulo 2 sekundy
            if (currentMillis - lastDHTRead >= dhtDelay) {
                lastDHTRead = currentMillis; // Aktualizace času posledního čtení
                temperature = NULL;//dht.readTemperature();  // Čtení teploty
                humidity = NULL;//dht.readHumidity();        // Čtení vlhkosti
            //nefunkční čtení DHT senzoru. Při pokusu číst padá celý program

                
                // Kontrola platnosti hodnot
                if (isnan(temperature) || isnan(humidity)) {
                    temperature = 0; // Výchozí hodnota, pokud není platná
                    humidity = 0;    // Výchozí hodnota, pokud není platná
                    Serial.println("Chyba při čtení z DHT senzoru!");
                }
            }

            int waterLevel = analogRead(WATER_SENSOR_PIN);

            // Náhrada placeholderů daty
            response.replace("{{temperature}}", String(temperature));
            response.replace("{{humidity}}", String(humidity));
            response.replace("{{waterLevel}}", String(waterLevel));
            response.replace("{{soilMoisture}}", "Načítání...");

            // Odeslání odpovědi
            request->send(200, "text/html", response);
        } else {
            request->send(404, "text/plain", "404: Not Found");
        }
    });

    // Obsluha požadavků na styles.css
    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open("/styles.css", "r");
        if (file) {
            request->send(LittleFS, "/styles.css", "text/css");
            file.close();
        } else {
            request->send(404, "text/plain", "404: Not Found");
        }
    });

    // Obsluha požadavků na script.js
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open("/script.js", "r");
        if (file) {
            request->send(LittleFS, "/script.js", "application/javascript");
            file.close();
        } else {
            request->send(404, "text/plain", "404: Not Found");
        }
    });

    // Spuštění serveru
    server.begin();
}

void loop() {
    // Asynchronní server nevyžaduje žádný kód v loop
    yield();  // Umožní zpracování systémových úkolů
}