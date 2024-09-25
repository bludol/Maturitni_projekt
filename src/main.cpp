#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "password.h"

#define DHTPIN 0        // DHT senzor je připojen na GPIO0 (D3 na Wemos D1 Mini)
#define DHTTYPE DHT11   // Používáte DHT11 senzor
#define WATER_SENSOR_PIN A0 // Pin pro analogový Water Level Sensor

DHT dht(DHTPIN, DHTTYPE);  // Inicializace DHT senzoru

int state = LOW;
int LED = LED_BUILTIN;
char on = LOW;
char off = HIGH;

WiFiServer server(80);  // Inicializace webového serveru na portu 80

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, off);
  pinMode(WATER_SENSOR_PIN, INPUT); // Nastavení pinu pro Water Level Sensor jako vstup

  // Inicializace DHT senzoru
  dht.begin();

  Serial.print("Connecting");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  server.begin();  // Start serveru
  Serial.println("Server started");

  Serial.print("IP Address of network: ");  // Zobrazení IP adresy na sériovém monitoru
  Serial.println(WiFi.localIP());
  Serial.print("Copy and paste the following URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
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

  // Výpis teploty a vlhkosti na sériový monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");
  
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  // Čtení hodnoty z Water Level Sensor (analogové čtení)
  int waterLevel = analogRead(WATER_SENSOR_PIN);
  Serial.print("Water Level Sensor: ");
  Serial.println(waterLevel); // Zobrazí hodnotu mezi 0-1023

  delay(1000);  // Zpoždění 1 sekunda

  WiFiClient client = server.available();
  if(!client) {
    return;
  }
  Serial.println("Waiting for new client");
  while(!client.available()) {
    delay(1);
  }

  // Zpracování HTTP požadavků
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Zapnutí a vypnutí LED
  if (request.indexOf("/LEDON") != -1) {
    digitalWrite(LED, on);  // Zapnout LED
    state = on;
  }
  if (request.indexOf("/LEDOFF") != -1) {
    digitalWrite(LED, off);  // Vypnout LED
    state = off;
  }

  // Odpověď webového serveru
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<h1>ESP8266 DHT11 Sensor</h1>");
  client.print("<p>Temperature: ");
  client.print(temperature);
  client.println(" &deg;C</p>");
  client.print("<p>Humidity: ");
  client.print(humidity);
  client.println(" %</p>");

  // Zobrazení hodnoty senzoru hladiny vody
  client.print("<p>Hladina vody (analog): ");
  client.print(waterLevel);
  client.println("</p>");

  client.println("<p><a href=\"/LEDON\">Turn ON LED</a></p>");
  client.println("<p><a href=\"/LEDOFF\">Turn OFF LED</a></p>");
  client.println("</html>");
}