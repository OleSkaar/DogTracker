/*
  DogTracker 0.1 Alpha

  Dings som kan hjelpe til aa motivere hundeeiere ved aa lagre og vise ukentlig treningsprogresjon.

  Med Piezo, LEDs, og LCD-skjerm.

  Guide til LED: 

  https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use

*/

// Import
#include <LiquidCrystal.h>
#include <Adafruit_NeoPixel.h>

// Oppsett av pins
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2, kp = 10, pz = 6, led = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Oppsett av LEDs
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, led, NEO_GRB + NEO_KHZ800);

// datavariabler
int repetisjoner = 0;
int ovelser = 0;
const int repsPerOvelse = 7;
int leds[] = {7, 8, 13};
int antallTrykk = 0;
int knapp;
int debounceTid = 50;
unsigned long sisteDebounce = 0;
boolean harTrykket = false;

void setup() {
  // Sett opp antall kolonner og rader paa LCD-skjermen:
  lcd.begin(16, 2);
  // Skriv til skjermen:
  lcd.print("Ovelse: Sitt!");
  for (int i = 0; i < 3; i++) {
    pinMode(leds[i], OUTPUT);
  }
  Serial.begin(9600);
  pinMode(kp, INPUT);
  pinMode(pz, OUTPUT);
  // Sett opp LED. Brightness gaar fra 0 til 255
  strip.begin();
  strip.setBrightness(32);
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  if (repetisjoner == repsPerOvelse) {
    fullfortOvelse();
    reset();
  }
  knapp = digitalRead(kp);
  if (knapp == 1) {
    if (digitalRead(kp) == 1 && debounce()) {
      if (!harTrykket) {
        antallTrykk += 1;
          if (repetisjoner < repsPerOvelse) {
            buttonClick();
          }
        harTrykket = true;
        sisteDebounce = millis();
      } 
    } 
  } else {
    harTrykket = false;
  }
}

void buttonClick() {
 repetisjoner++;
 // Lys opp en ny LED
 strip.setPixelColor(repetisjoner - 1, 0, 255, 05);
 strip.show();
 }

void reset() {
    repetisjoner = 0;
    // Fjern LED lys
    strip.clear();
    strip.show();
    ovelser++;
    lcd.setCursor(0, 1);
    lcd.print(ovelser);
    if (ovelser < 4) {
      digitalWrite(leds[ovelser-1], HIGH);
   }
}

boolean debounce() {
  return (millis() - sisteDebounce) >= debounceTid;
}

void fullfortOvelse() {
  tone(pz,988,100);
  delay(100);
  tone(pz,1319,850);
  delay(800);
  noTone(pz);
}
