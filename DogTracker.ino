/*
  DogTracker 0.2 Alpha

  Dings som kan hjelpe til aa motivere hundeeiere ved aa lagre og vise ukentlig treningsprogresjon.

  Med LED-ringer, piezo, og potentiometer som kontroller

  Guide til LED: 

  https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use

*/

// Import
#include <Adafruit_NeoPixel.h>

// Oppsett av pins
const int kp = 10, pz = 6, ledStrip = 5, ledRing = 9, potPin = A0;

// Oppsett av LEDs
Adafruit_NeoPixel ring = Adafruit_NeoPixel(12, ledRing, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, ledStrip, NEO_GRB + NEO_KHZ800);

// oppsett
int repetisjoner = 0;
//int ovelser = 0;
int ovelserPerDag = 3;
const int repsPerOvelse = 7;
int antallTrykk = 0;
int knapp;
int debounceTid = 50;
unsigned long sisteDebounce = 0;
boolean harTrykket = false;
int startPunktOvelseLED = 11;
int potVal;
int angle;
int aktivOvelse;
int aktivBruker;
const int antallOvelser = 3;
const int antallBrukere = 2;

int brukerOgPoengMatrise[antallBrukere][antallOvelser];
int aktivOvelsePoeng;
int ukentligPoeng[antallBrukere];

// datoer
// Date forrigeDato;
// Date dagensDato;

// brukervariabler
int bruker1Poeng;
int bruker2Poeng;

int bruker1Sitt;
int bruker1Bli;
int bruker1Kom;

int bruker2Sitt = 1;
int bruker2Bli = 2;
int bruker2Kom = 0;

int sittReps;
int bliReps;
int komReps;

void setup() {
  // Hent dato fra GPS, sjekker om det er en ny uke, 
  // hvis ja nullstill, hvis ikke, last inn brukerpoeng for denne uken
  // Hvis det er en ny dag, nullstill repetisjoner og ovelser, hvis ikke,
  // hent ukentlig poeng fra minnet

   fyllPoeng();
   aktivBruker = 0;
   
   aktivOvelsePoeng = brukerOgPoengMatrise[aktivBruker][aktivOvelse];
   oppdaterOvelseLED(aktivOvelsePoeng);
  
  pinMode(kp, INPUT);
  pinMode(pz, OUTPUT);
  // Sett opp LED. Brightness gaar fra 0 til 255
  ring.begin();
  ring.setBrightness(32);
  ring.show(); // Initialize all pixels to 'off'
  strip.begin();
  strip.setBrightness(255);
  strip.show(); // Initialize all pixels to 'off'
  Serial.begin(9600);
  hentUkentligPoeng();
  oppdaterUkentligLED();
}

void loop() {
  lesOvelse();
  if (repetisjoner == repsPerOvelse) {
    if (aktivOvelsePoeng < ovelserPerDag) {
      brukerOgPoengMatrise[aktivBruker][aktivOvelse]++;
      fullfortOvelse();
    }
    reset();
  }
  
  knapp = digitalRead(kp);
  if (knapp == 1) {
    if (digitalRead(kp) == 1 && debounce()) {
      if (!harTrykket) {
        antallTrykk += 1;
          if (repetisjoner < repsPerOvelse && 
          brukerOgPoengMatrise[aktivBruker][aktivOvelse] != ovelserPerDag) {
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
   ring.setPixelColor(repetisjoner - 1, 0, 255, 05);
   ring.show();
 }

void reset() {
  repetisjoner = 0;
  // Fjern LED lys
  ring.fill(0, 0, repsPerOvelse);
  ring.show();
}

boolean debounce() {
  return (millis() - sisteDebounce) >= debounceTid;
}

int hentAnnenBruker() {
  int bruker;
  if (aktivBruker == 1) {
    bruker = 0;
  } else {
    bruker = 1;
  }
  return bruker;
}

void fullfortOvelse() {
  tone(pz,988,100);
  delay(100);
  tone(pz,1319,850);
  delay(800);
  noTone(pz);
  ring.setPixelColor(startPunktOvelseLED - brukerOgPoengMatrise[aktivBruker][aktivOvelse],255);
  ukentligPoeng[aktivBruker]++;
  oppdaterUkentligLED();
}

void oppdaterOvelseLED(int poeng) {
  //ring.fill(0, startPunktOvelseLED-2, 3);
  ring.setPixelColor(10, 0);
  ring.setPixelColor(9, 0);
  ring.setPixelColor(8, 0);
  if (poeng >= 1) {
    ring.setPixelColor(10, 255);
  }
  if (poeng >= 2) {
    ring.setPixelColor(9, 255);
  }
  if (poeng == 3) {
    ring.setPixelColor(8, 255);
  }
  ring.show();
}

void oppdaterUkentligLED() {
  int poeng = ukentligPoeng[aktivBruker];
  int annenBruker = hentAnnenBruker();
  strip.clear();
  Serial.println(poeng);
  for (int i = 0; i < poeng; i++) {
    int teller = 0;
    int farge = strip.getPixelColor(teller);
    if (farge + 42 > 255) {
      teller++;
      farge = strip.getPixelColor(teller);
    }
    if (ukentligPoeng[aktivBruker] >= ukentligPoeng[annenBruker]) {
      strip.setPixelColor(teller, 0, 0, farge + 42);
    } else {
      strip.setPixelColor(teller, farge + 42, 0, 0);
    }
    
  }
  strip.show();
}

void lesOvelse() {
    potVal = analogRead(potPin);
    angle = map(potVal, 0, 1023, 0, 179);

  if (aktivOvelse != 0 && angle >= 0 && angle < 60) {
    
    Serial.println(angle);
    Serial.println("kom");
    
    aktivOvelse = 0;
    oppdaterOvelseLED(brukerOgPoengMatrise[aktivBruker][aktivOvelse]);
  }

  if (aktivOvelse != 1 && angle >= 60 && angle < 120) {
    
    Serial.println(angle);
    Serial.println("sitt");
    
    aktivOvelse = 1;
    oppdaterOvelseLED(brukerOgPoengMatrise[aktivBruker][aktivOvelse]);
  }

  if (aktivOvelse != 2 && angle >= 120 && angle < 180) {
    
    Serial.println(angle);
    Serial.println("bli");
    
    aktivOvelse = 2;
    oppdaterOvelseLED(brukerOgPoengMatrise[aktivBruker][aktivOvelse]);
  }
  delay(50);
}

void fyllPoeng() {
  brukerOgPoengMatrise[0][0] = 0;
  brukerOgPoengMatrise[0][1] = 0;
  brukerOgPoengMatrise[0][2] = 0;
  brukerOgPoengMatrise[1][0] = bruker2Kom;
  brukerOgPoengMatrise[1][1] = bruker2Sitt;
  brukerOgPoengMatrise[1][2] = bruker2Bli;
}

void hentUkentligPoeng() {
  for (int i = 0; i < antallBrukere; i++) {
    for (int j = 0; j < antallOvelser; j++) {
      ukentligPoeng[i] += brukerOgPoengMatrise[i][j];
    }
  }
}
