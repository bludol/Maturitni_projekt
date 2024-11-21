#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SimpleDHT.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "password.h"

#define DHTPIN 0               // DHT senzor na GPIO0 (D3 na Wemos D1 Mini)
#define WATER_SENSOR_PIN A0    // Pin pro analogový Water Level Sensor
#define PUMP_PIN 2             // Pin pro spínání pumpy (GPIO2)
#define SOIL_SENSOR_PIN 13     // Digitální pin pro HW-080 přes HW-103 (GPIO15)

SimpleDHT11 dht11(DHTPIN);
AsyncWebServer server(80);

unsigned long lastDHTRead = 0;
const unsigned long dhtDelay = 2000;
float temperature = 0;
float humidity = 0;
bool pumpState = false;
bool soilMoisture = false;
const int WATER_LEVEL_NEEDED = 200;
const int PUMP_RUN_TIME_MS = 2000;
unsigned long lastPumpRun = 0;
unsigned long lastSaveTime = 0;
const unsigned long saveInterval = 1800 * 1000; // 30 minut

// Funkce pro výpis souborů
void listFiles() {
    Serial.println("Výpis souborů v LittleFS:");
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        String fileName = dir.fileName();
        Serial.print(" - ");
        Serial.println(fileName);
    }
    Serial.println("Konec výpisu souborů.");
}

// Funkce pro uložení měření do souboru
void saveMeasurement(float temp, float humidity) {
    File file = LittleFS.open("/data_day.json", "r+"); // Otevření souboru pro čtení a zápis
    if (!file) {
        Serial.println("Chyba při otevírání souboru pro ukládání dat!");
        return;
    }

    // Načteme stávající data
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println("Chyba při načítání JSON dat");
        file.close();
        return;
    }

    JsonArray data = doc.as<JsonArray>();

    // Zkontrolujeme, zda máme více než 48 záznamů
    if (data.size() >= 48) {
        data.remove(0);  // Odstraníme nejstarší záznam
    }

    // Přidáme nový záznam
    JsonObject newMeasurement = data.createNestedObject();
    newMeasurement["temp"] = temp;
    newMeasurement["humidity"] = humidity;
    newMeasurement["timestamp"] = millis() / 1000; // Uložíme čas v sekundách

    // Uložíme zpět data do souboru
    file.close();
    file = LittleFS.open("/data_day.json", "w"); // Otevření pro zápis
    serializeJson(doc, file);
    file.close();
}

// Funkce pro výpočet denního průměru a reset datového souboru
void calculateDailyAverageAndReset() {
    // Otevření souboru data_day.json pro čtení
    File file = LittleFS.open("/data_day.json", "r");
    if (!file) {
        Serial.println("Chyba při otevírání souboru data_day.json pro výpočet průměru!");
        return;
    }

    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println("Chyba při načítání JSON dat z data_day.json!");
        file.close();
        return;
    }
    JsonArray data = doc.as<JsonArray>();
    file.close();

    float totalTemp = 0, totalHumidity = 0;
    int count = 0;

    // Sčítání všech teplot a vlhkostí
    for (JsonObject measurement : data) {
        totalTemp += measurement["temp"].as<float>();
        totalHumidity += measurement["humidity"].as<float>();
        count++;
    }

    if (count > 0) {
        // Výpočet průměrů
        float avgTemp = totalTemp / count;
        float avgHumidity = totalHumidity / count;

        // Otevření souboru pro denní průměry
        File dailyFile = LittleFS.open("/daily_data.json", "a+");
        if (!dailyFile) {
            Serial.println("Chyba při otevírání souboru daily_data.json pro zápis!");
            return;
        }

        StaticJsonDocument<4096> dailyDoc;
        DeserializationError dailyError = deserializeJson(dailyDoc, dailyFile);
        if (dailyError) {
            Serial.println("Chyba při načítání JSON dat z daily_data.json!");
            dailyFile.close();
            return;
        }

        JsonArray dailyData = dailyDoc.to<JsonArray>();

        if (dailyData.size() >= 30) {
            dailyData.remove(0);  // Odstraníme starší záznamy, pokud máme více než 30 dní
        }

        JsonObject newDay = dailyData.createNestedObject();
        newDay["temp"] = avgTemp;
        newDay["humidity"] = avgHumidity;

        dailyFile.close();
        dailyFile = LittleFS.open("/daily_data.json", "w");
        serializeJson(dailyDoc, dailyFile);
        dailyFile.close();
    }

    // Resetování souboru data_day.json (vyprázdnění)
    File resetFile = LittleFS.open("/data_day.json", "w");
    if (!resetFile) {
        Serial.println("Chyba při resetování souboru data_day.json!");
    }
    resetFile.close();
}

// Funkce pro vytvoření souborů, pokud neexistují
void createFilesIfNotExist() {
    if (!LittleFS.exists("/data_day.json")) {
        File file = LittleFS.open("/data_day.json", "w");
        if (!file) {
            Serial.println("Chyba při vytváření souboru data_day.json!");
            return;
        }
        file.close();
        Serial.println("Soubor data_day.json vytvořen.");
    }

    if (!LittleFS.exists("/daily_data.json")) {
        File file = LittleFS.open("/daily_data.json", "w");
        if (!file) {
            Serial.println("Chyba při vytváření souboru daily_data.json!");
            return;
        }

        StaticJsonDocument<4096> dailyDoc;
        JsonArray dailyData = dailyDoc.to<JsonArray>();
        serializeJson(dailyDoc, file);
        file.close();
        Serial.println("Soubor daily_data.json vytvořen.");
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(WATER_SENSOR_PIN, INPUT);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(SOIL_SENSOR_PIN, INPUT);
    digitalWrite(PUMP_PIN, LOW);

    Serial.print("Připojuji se k WiFi...");
    WiFi.begin(ssid, password);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {  // Počet pokusů o připojení
        delay(1000);
        Serial.print(".");
        retries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi připojeno");
        Serial.print("IP adresa: ");
        Serial.println(WiFi.localIP()); // Zobrazení IP adresy po připojení
    } else {
        Serial.println("Nepodařilo se připojit k WiFi");
        return;
    }

    if (!LittleFS.begin()) {
        Serial.println("Chyba při inicializaci LittleFS!");
        return;
    }
    Serial.println("LittleFS inicializován.");

    createFilesIfNotExist(); // Vytvoření souborů, pokud neexistují
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
            response.replace("{{soilMoisture}}", String(soilMoisture ? "Sucho" : "Vlhko"));

            request->send(200, "text/html", response);
        } else {
            request->send(404, "text/plain", "Soubor nenalezen");
        }
    });

    server.begin();
}

void loop() {
    unsigned long currentMillis = millis();

    // Logování měření do souboru každých 30 minut
    if (currentMillis - lastSaveTime >= saveInterval) {
        lastSaveTime = currentMillis;
        saveMeasurement(temperature, humidity);
    }

    // Pokud uplynul čas pro výpočet denního průměru (např. na konci dne)
    if (currentMillis % (24 * 60 * 60 * 1000) == 0) { // 24 hodin
        calculateDailyAverageAndReset();
    }
}