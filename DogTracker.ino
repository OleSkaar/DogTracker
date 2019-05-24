/*
  DogTracker 0.2.2  Alpha

  Bruker LED-ringen til å vise om nåværende bruker ligger foran eller bak den andre brukeren.

  Dings som kan hjelpe til aa motivere hundeeiere ved aa lagre og vise ukentlig treningsprogresjon.

  Lar to forskjellige brukere bruke samme dings til å konkurrere. 
  Denne versjonen bruker LED-ring for å vise om aktiv bruker er foran, lik, eller bak den andre brukeren i poeng.

  Med LED-ringe, piezo, og potentiometer som kontroller

  Guide til LED: 

  https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use

  Kode basert paa SDFat sitt eksempel: 

  https://github.com/greiman/SdFat/blob/master/examples/ReadWrite/ReadWrite.ino

*/

// Import
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include "SdFat.h"

// Oppsett av pins
const int kp = 10, br = 8, pz = 6, ledRing = 9, potPin = A0;

// Oppsett av LEDs
Adafruit_NeoPixel ring = Adafruit_NeoPixel(12, ledRing, NEO_GRB + NEO_KHZ800);
uint32_t gul = ring.Color(255, 255, 0);
uint32_t hvit = ring.Color(255, 255, 255);
uint32_t blaa = ring.Color(0, 0, 255);
uint32_t rod = ring.Color(255, 0, 0);
uint32_t gronn = ring.Color(0, 255, 0);

// oppsett
int repetisjoner = 0;
int ovelserPerDag = 3;
const int maksmaksRepsPerOvelse = 7;
int antallTrykk = 0;
int knapp;
int bryter;
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
SdFat SD;
File dataFil;
#define SD_CS_PIN SS

int poengPerOvelse[antallBrukere][antallOvelser];
int brukerOgPoengMatrise[antallBrukere][antallOvelser];
int aktivOvelsePoeng;
int ukentligPoeng[antallBrukere];

// brukervariabler
int bruker1Poeng;
int bruker2Poeng;

int bruker1Sitt;
int bruker1Bli;
int bruker1Kom;

int bruker2Sitt = 3;
int bruker2Bli = 1;
int bruker2Kom = 2;

void setup() {
  Serial.begin(9600);

  fyllPoeng();
  aktivBruker = 1;

  SDKortoppsett();
  byte test = hentVerdiIFil(aktivBrukerFilnavn());
  Serial.println(test);
  aktivOvelsePoeng = brukerOgPoengMatrise[aktivBruker][aktivOvelse];
  oppdaterOvelseLED(aktivOvelsePoeng);
  
  pinMode(kp, INPUT);
  pinMode(pz, OUTPUT);
  // Sett opp LED. Brightness gaar fra 0 til 255
  ring.begin();
  ring.setBrightness(100);
  ring.show();
  hentUkentligPoeng();
  oppdaterUkentligLED();
  Serial.println(hentVerdiIFil(aktivBrukerFilnavn()));
  Serial.println("Indeks:");
  Serial.println(maksRepsPerOvelse
}

void sjekkAktivBrukerBryter() {
  bryter = digitalRead(br);
  if (bryter != aktivBruker) {
    byttBruker();
  }
}

void byttBruker() {
  if (aktivBruker == 0) {
    aktivBruker = 1;
  } else {
    aktivBruker = 0;
  }
}

void loop() {
  sjekkAktivBrukerBryter();
  lesOvelse();
  if (repetisjoner == maksRepsPerOvelse) {
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
          if (repetisjoner < maksRepsPerOvelse && 
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
   ring.setPixelColor(repetisjoner - 1, hvit);
   ring.show();
 }

void reset() {
  repetisjoner = 0;
  // Fjern LED lys
  ring.fill(0, 0, maksRepsPerOvelse);
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
  ring.setPixelColor(startPunktOvelseLED - brukerOgPoengMatrise[aktivBruker][aktivOvelse],blaa);
  ukentligPoeng[aktivBruker]++;
  oppdaterVerdiIFil(aktivBrukerFilnavn(), ukentligPoeng[aktivBruker]);
  oppdaterUkentligLED();
}

void oppdaterOvelseLED(int poeng) {
  ring.setPixelColor(10, 0);
  ring.setPixelColor(9, 0);
  ring.setPixelColor(8, 0);
  if (poeng >= 1) {
    ring.setPixelColor(10, blaa);
  }
  if (poeng >= 2) {
    ring.setPixelColor(9, blaa);
  }
  if (poeng == 3) {
    ring.setPixelColor(8, blaa);
  }
  ring.show();
}


void oppdaterUkentligLED() {
  // Aktiv bruker = 11
  // Annen bruker = 7
  int annenBruker = hentAnnenBruker();
  int aktivBrukerPoeng = ukentligPoeng[aktivBruker];
  int annenBrukerPoeng = ukentligPoeng[annenBruker];
  if (aktivBrukerPoeng == annenBrukerPoeng) {
    ring.setPixelColor(7, gul);
    ring.setPixelColor(11, gul);
  } else if (aktivBrukerPoeng > annenBrukerPoeng) {
    ring.setPixelColor(7, rod);
    ring.setPixelColor(11, gronn);
  } else {
    ring.setPixelColor(7, gronn);
    ring.setPixelColor(11, rod);
  }
}


void lesOvelse() {
    potVal = analogRead(potPin);
    angle = map(potVal, 0, 1023, 0, 179);

  if (aktivOvelse != 0 && angle >= 10 && angle < 85) {
    
    Serial.println(angle);
    Serial.println("kom");
    
    aktivOvelse = 0;
    oppdaterOvelseLED(brukerOgPoengMatrise[aktivBruker][aktivOvelse]);
  }

  if (aktivOvelse != 1 && angle >= 85 && angle < 100) {
    
    Serial.println(angle);
    Serial.println("sitt");
    
    aktivOvelse = 1;
    oppdaterOvelseLED(brukerOgPoengMatrise[aktivBruker][aktivOvelse]);
  }

  if (aktivOvelse != 2 && angle >= 105 && angle < 170) {
    
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

void oppdaterVerdiIFil(String filN, int nyVerdi) {
  File filen = SD.open(filN, FILE_WRITE);
  filen.remove();
  filen = SD.open(filN, FILE_WRITE);
  filen.write(nyVerdi);
  filen.close();
}
byte hentVerdiIFil(String filN) {
  File filen = SD.open(filN);
  byte verdi;
  while (filen.available()) {
    verdi += filen.read();
  }
  filen.close();
  return verdi;
}

void SDKortoppsett() {
    // Kobler opp SD-kort
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Oppsett av SD-kort feilet.");
    return;
  }
  Serial.println("Oppsett av SD-kort fullført.");
}

String aktivBrukerFilnavn() {
  String filNavn = "bruker" + String(aktivBruker) + ".txt";
  return filNavn;
}
