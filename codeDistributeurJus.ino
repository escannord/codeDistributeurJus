#include <Keypad.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <NewPing.h>

#define TRIG_PIN 4
#define ECHO_PIN 15
#define MAX_DISTANCE 30

#define PRESENCE_SENSOR_PIN 2

#define RELAY_PUMP1 5
#define RELAY_PUMP2 24
#define RELAY_PUMP3 14

TFT_eSPI tft = TFT_eSPI();
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// Définition du clavier
const byte ROWS = 3;
const byte COLS = 1;
char keys[ROWS][COLS] = {
  {'1'},
  {'2'},
  {'3'}
};
byte rowPins[ROWS] = {16, 22, 13};
byte colPins[COLS] = {12};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int selectedPump = 0;

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PUMP1, OUTPUT);
  pinMode(RELAY_PUMP2, OUTPUT);
  pinMode(RELAY_PUMP3, OUTPUT);
  pinMode(PRESENCE_SENSOR_PIN, INPUT);

  digitalWrite(RELAY_PUMP1, HIGH);
  digitalWrite(RELAY_PUMP2, HIGH);
  digitalWrite(RELAY_PUMP3, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 10);
  tft.println("Selection du jus:");
  tft.setCursor(10, 40);
  tft.println("1. Jus 1");
  tft.setCursor(10, 60);
  tft.println("2. Jus 2");
  tft.setCursor(10, 80);
  tft.println("3. Jus 3");
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    if (key == '1') selectedPump = 1;
    if (key == '2') selectedPump = 2;
    if (key == '3') selectedPump = 3;

    tft.fillRect(10, 100, 200, 30, TFT_BLACK);
    tft.setCursor(10, 100);
    tft.print("Jus choisi: ");
    tft.println(selectedPump);
  }

  bool cupDetected = digitalRead(PRESENCE_SENSOR_PIN) == HIGH;

  if (cupDetected && selectedPump > 0) {
    int distance = sonar.ping_cm();
    if (distance > 0 && distance > 9.5) {  // 17 - 7.5 = 9.5 cm
      activerPompe(selectedPump);
    } else {
      desactiverToutesPompes();
    }
  } else {
    desactiverToutesPompes();
  }

  delay(300);
}

void activerPompe(int num) {
  digitalWrite(RELAY_PUMP1, num == 1 ? LOW : HIGH);
  digitalWrite(RELAY_PUMP2, num == 2 ? LOW : HIGH);
  digitalWrite(RELAY_PUMP3, num == 3 ? LOW : HIGH);
}

void desactiverToutesPompes() {
  digitalWrite(RELAY_PUMP1, HIGH);
  digitalWrite(RELAY_PUMP2, HIGH);
  digitalWrite(RELAY_PUMP3, HIGH);
}
