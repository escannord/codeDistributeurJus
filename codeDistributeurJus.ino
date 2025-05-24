#include <TFT_eSPI.h>
#include <Keypad.h>
#include <NewPing.h>
#include "HX711.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string>

// Wi-Fi
const char* ssid = "FAMILIA YUSUF HAB";
const char* password = "Yusuf2023";

// WebServer
WebServer server(80);

//API Server
const char* apiUrl = "https://juiceapi.onrender.com/logs";

// Capteurs HX711
#define HX711_DT1 26
#define HX711_SCK1 25
#define HX711_DT2 33
#define HX711_SCK2 32

HX711 scale1;
HX711 scale2;

// Pompes et capteurs
#define RELAY_PUMP1 15
#define RELAY_PUMP2 27
#define RELAY_PUMP3 14
#define PRESENCE_SENSOR_PIN 4
#define TRIG_PIN 17
#define ECHO_PIN 5

NewPing sonar(TRIG_PIN, ECHO_PIN, 200);
TFT_eSPI tft = TFT_eSPI();

#define FULL_HEIGHT_CM 22
#define SENSOR_TO_EMPTY_CUP_CM 26
#define DENSITE_JUS 1.04

// Clavier
const byte ROWS = 3;
const byte COLS = 1;
char keys[ROWS][COLS] = {{'1'}, {'2'}, {'3'}};
byte rowPins[ROWS] = {16, 22, 13};
byte colPins[COLS] = {12};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int selectedPump = 0;
String nomJus = "Aucun";
int TemoinQuantite = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connecté au WiFi !");
  Serial.println(WiFi.localIP());

  // Config Web Server
  server.on("/", handleWebPage);
  server.begin();

  // Pompes
  pinMode(RELAY_PUMP1, OUTPUT);
  pinMode(RELAY_PUMP2, OUTPUT);
  pinMode(RELAY_PUMP3, OUTPUT);
  digitalWrite(RELAY_PUMP1, LOW);
  digitalWrite(RELAY_PUMP2, LOW);
  digitalWrite(RELAY_PUMP3, LOW);

  pinMode(PRESENCE_SENSOR_PIN, INPUT);

  // Capteurs poids
  scale1.begin(HX711_DT1, HX711_SCK1);
  scale2.begin(HX711_DT2, HX711_SCK2);
  scale1.set_scale();
  scale2.set_scale();
  scale1.tare();
  scale2.tare();

  // Écran
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(10, 10);
  tft.println("Systeme Pret");
  tft.setCursor(10, 40);
  tft.println("Choisir 1-3:");
}

void loop() {
  server.handleClient();

  char key = keypad.getKey();
  if (key) {
    if (key == '1') { selectedPump = 1; nomJus = "Orange"; }
    else if (key == '2') { selectedPump = 2; nomJus = "Ananas"; }
    else if (key == '3') { selectedPump = 3; nomJus = "Maracouja"; }

    tft.fillRect(10, 70, 200, 30, TFT_BLACK);
    tft.setCursor(10, 70);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("Jus choisi: ");
    tft.println(nomJus);
  }

  // Lecture des poids
  float poids1 = scale1.get_units();
  float poids2 = scale2.get_units();
  float poidsTotal = poids1 + poids2;
  float volume = poidsTotal / DENSITE_JUS / 1000.0;

  tft.fillRect(10, 130, 240, 50, TFT_BLACK);
  tft.setCursor(10, 130);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.print("Poids: ");
  tft.print(poidsTotal, 0);
  tft.println(" kg");
  Serial.print(poidsTotal);
  Serial.println( " kg ");

  tft.setCursor(10, 150);
  tft.print("Volume: ");
  tft.print(volume, 2);
  tft.println(" L");
  Serial.print(volume);
  Serial.println( " L ");
  
  // Détection gobelet
  bool gobelet_present = digitalRead(PRESENCE_SENSOR_PIN) == LOW;
  if (gobelet_present && selectedPump > 0) {
    float distance = sonar.ping_cm();
    float niveau_jus = SENSOR_TO_EMPTY_CUP_CM - distance;

    if (distance > FULL_HEIGHT_CM || distance == 0) {
      activerPompe(selectedPump);
      tft.fillRect(10, 100, 240, 30, TFT_BLACK);
      tft.setCursor(10, 100);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.println("Remplissage...");
    } else {
      desactiverToutesPompes();
      // envoi vers le serveur d'API
      sendToAPI(selectedPump, TemoinQuantite, "Completed");
      TemoinQuantite = 0;
      
      tft.fillRect(10, 100, 240, 30, TFT_BLACK);
      tft.setCursor(10, 100);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Gobelet Plein");
      selectedPump = 0;
    }
  } else {
    desactiverToutesPompes();
    tft.fillRect(10, 100, 240, 30, TFT_BLACK);
    tft.setCursor(10, 100);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.println("En attente gobelet");
  }

  delay(500);
}

void activerPompe(int num) {
  digitalWrite(RELAY_PUMP1, num == 1 ? HIGH : LOW);
  digitalWrite(RELAY_PUMP2, num == 2 ? HIGH : LOW);
  digitalWrite(RELAY_PUMP3, num == 3 ? HIGH : LOW);
  TemoinQuantite +=1;
  Serial.println(TemoinQuantite);
}

void desactiverToutesPompes() {
  digitalWrite(RELAY_PUMP1, LOW);
  digitalWrite(RELAY_PUMP2, LOW);
  digitalWrite(RELAY_PUMP3, LOW);
}

void handleWebPage() {
  float poidsTotal = scale1.get_units() + scale2.get_units();
  float volume = poidsTotal / DENSITE_JUS ;

  String page = "<html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5' />";
  page += "<style>body{font-family:sans-serif;text-align:center;}</style></head><body>";
  page += "<h2>Distributeur de Jus</h2>";
  page += "<p><b>Jus sélectionné:</b> " + nomJus + "</p>";
  page += "<p><b>Poids total:</b> " + String(poidsTotal, 0) + " g</p>";
  page += "<p><b>Volume restant:</b> " + String(volume, 2) + " L</p>";
  page += "<p>Actualisation automatique toutes les 5 secondes</p>";
  page += "</body></html>";

  server.send(200, "text/html", page);
}

void sendToAPI(int pumpNumber, float fillLevel, std::string status) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Erreur WiFi");
    return;
  }

  HTTPClient http;
  http.begin(apiUrl);
  http.addHeader("Content-Type", "application/json");

  // Création du payload JSON
  StaticJsonDocument<200> doc;
  doc["juiceType"] = (pumpNumber == 1) ? "Orange" : (pumpNumber == 2) ? "Apple" : "Pineapple";
  doc["quantity"] = fillLevel;
  doc["status"] = status;
  
  String payload;
  serializeJson(doc, payload);

  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.printf("Code HTTP: %d\n", httpCode);
  } else {
    Serial.printf("Échec de la requête: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}