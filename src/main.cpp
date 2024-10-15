#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <LittleFS.h> // Knihovna pro práci s LittleFS
#include "password.h" // Tento soubor by měl obsahovat definice ssid a password

#define DHTPIN 0        // DHT senzor je připojen na GPIO0 (D3 na Wemos D1 Mini)
#define DHTTYPE DHT11   // Používáte DHT11 senzor
#define WATER_SENSOR_PIN A0 // Pin pro analogový Water Level Sensor

DHT dht(DHTPIN, DHTTYPE);  // Inicializace DHT senzoru

WiFiServer server(80);  // Inicializace webového serveru na portu 80

// Funkce pro výpis souborů v LittleFS
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

void setup() {
    Serial.begin(115200);
    pinMode(WATER_SENSOR_PIN, INPUT); // Nastavení pinu pro Water Level Sensor jako vstup

    // Inicializace DHT senzoru
    dht.begin();

    Serial.print("Připojuji se k WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi připojeno");
    server.begin();  // Spuštění serveru
    Serial.println("Server spuštěn");

    Serial.print("IP adresa sítě: ");  // Zobrazení IP adresy na sériovém monitoru
    Serial.println(WiFi.localIP());
    Serial.print("Zkopírujte a vložte následující URL: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");

    // Inicializace LittleFS
    if (!LittleFS.begin()) {
        Serial.println("Chyba při inicializaci LittleFS!");
        return;
    }
    Serial.println("LittleFS inicializován.");

    // Zobrazit všechny soubory v LittleFS
    listFiles();
}

void loop() {
    // Načtení teploty a vlhkosti z DHT senzoru
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Kontrola, zda se data načetla správně
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Nepodařilo se načíst data ze senzoru DHT!");
        return;
    }

    // Čtení hodnoty z Water Level Sensor (analogové čtení)
    int waterLevel = analogRead(WATER_SENSOR_PIN);

    // Zpracování HTTP požadavků
    WiFiClient client = server.available(); // Použití available() namísto accept()
    if (!client) {
        return;
    }

    Serial.println("Čekání na nového klienta");
    while (!client.available()) {
        delay(1);
    }

    // Čtení požadavku
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    // Odpověď webového serveru
    if (request.indexOf("GET / ") != -1 || request.indexOf("GET /index.html") != -1) {
        // Načtení obsahu index.html
        File file = LittleFS.open("/index.html", "r");
        if (file) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            while (file.available()) {
                String line = file.readStringUntil('\n');
                // Nahrazení placeholderů daty
                line.replace("{{temperature}}", String(temperature));
                line.replace("{{humidity}}", String(humidity));
                line.replace("{{waterLevel}}", String(waterLevel));
                line.replace("{{soilMoisture}}", "Načítání...");
                client.println(line);
            }
            file.close();
        } else {
            client.println("HTTP/1.1 404 Not Found");
            client.println("Connection: close");
            client.println();
            client.println("<h1>404 Not Found</h1>");
        }
    } else {
        // Pro ostatní požadavky (např. na CSS nebo JavaScript)
        if (request.indexOf("GET /styles.css") != -1) {
            File file = LittleFS.open("/styles.css", "r");
            if (file) {
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/css");
                client.println("Connection: close");
                client.println();
                while (file.available()) {
                    client.write(file.read());
                }
                file.close();
            }
        } else if (request.indexOf("GET /script.js") != -1) {
            File file = LittleFS.open("/script.js", "r");
            if (file) {
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: application/javascript");
                client.println("Connection: close");
                client.println();
                while (file.available()) {
                    client.write(file.read());
                }
                file.close();
            }
        }
    }
}