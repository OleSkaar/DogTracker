 /*
  DogTracker 0.2.4  Alpha

  Bruker LED-ringen til å vise om nåværende bruker ligger foran eller bak den andre brukeren.

  Dings som kan hjelpe til aa motivere hundeeiere ved aa lagre og vise ukentlig treningsprogresjon.

  Lar to forskjellige brukere bruke samme dings til å konkurrere. 
  Denne versjonen bruker LED-ring for å vise om aktiv bruker er foran, lik, eller bak den andre brukeren i poeng.

  Med LED-ringe, piezo, og potentiometer som kontroller

  Guide til LED: 

  https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use

  Kode basert paa SDFat sitt eksempel: 

  https://github.com/bgreiman/SdFat/blob/master/examples/ReadWrite/ReadWrite.ino

*/

// Import
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include "SdFat.h"
#include <EEPROM.h>

// Oppsett av pins
const int kp = 13, br = 8, pz = 6, ledRing = 9, potPin = A0;

// Oppsett av LEDs
Adafruit_NeoPixel ring = Adafruit_NeoPixel(12, ledRing, NEO_GRB + NEO_KHZ800);
uint32_t gul = ring.Color(255, 255, 0);
uint32_t hvit = ring.Color(255, 255, 255);
uint32_t blaa = ring.Color(0, 0, 255);
uint32_t rod = ring.Color(255, 0, 0);
uint32_t gronn = ring.Color(0, 255, 0);

// oppsett
//int repetisjoner = 0;
int ovelserPerDag = 3;
const int maksRepsPerOvelse = 7;
int antallTrykk = 0;
int knapp;
int bryter;
// debounce 
const int debounceTid = 300;
unsigned long sisteDebounceReps = 0;
boolean harTrykketReps = false;
unsigned long sisteDebouncePot = 0;
boolean harTrykketPot = false;
int sjekkOvelse;

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

int repsPerOvelse[antallBrukere][antallOvelser];
int brukerOgPoengMatrise[antallBrukere][antallOvelser];
int aktivOvelsePoeng;
int ukentligPoeng[antallBrukere];

// brukervariabler

int bruker1Sitt;
int bruker1Bli;
int bruker1Kom;

int bruker2Sitt = 0;
int bruker2Bli = 0;
int bruker2Kom = 0;

void setup() {
  Serial.begin(9600);
  ring.begin();
  ring.setBrightness(32);
  ring.show();

  fyllPoeng();
  sjekkAktivBrukerBryter();

  SDKortoppsett();
  int annenBruker = hentAnnenBruker();
  //ukentligPoeng[aktivBruker] = hentVerdiIFil(filnavn(aktivBruker));
  //ukentligPoeng[annenBruker] = hentVerdiIFil(filnavn(annenBruker));
  ukentligPoeng[aktivBruker] = lesFraEEPROM(aktivBruker);
  ukentligPoeng[annenBruker] = lesFraEEPROM(annenBruker);
  Serial.println(aktivBruker);
  Serial.println(annenBruker);
  Serial.println(hentVerdiIFil(filnavn(aktivBruker)));
  Serial.println(hentVerdiIFil(filnavn(annenBruker)));
  Serial.println("Aktiv bruker poeng:");
  Serial.println(ukentligPoeng[aktivBruker]);
  Serial.println("Annen bruker poeng:");
  Serial.println(ukentligPoeng[annenBruker]);
  aktivOvelse = lesOvelse();
  aktivOvelsePoeng = brukerOgPoengMatrise[aktivBruker][aktivOvelse];
  oppdaterOvelseLED(aktivOvelsePoeng);
  
  pinMode(kp, INPUT);
  pinMode(pz, OUTPUT);
  // Sett opp LED. Brightness gaar fra 0 til 255

  hentUkentligPoeng();
  oppdaterUkentligLED();
}

void sjekkAktivBrukerBryter() {
  bryter = digitalRead(br);
  if (bryter != aktivBruker) {
    fyllRepLEDs();
    aktivBruker = bryter;
    oppdaterUkentligLED();
    oppdaterOvelseLED(brukerOgPoengMatrise[aktivBruker][aktivOvelse]);
    repsPerOvelse[aktivBruker][aktivOvelse] = 0;
  }
}


void loop() {
  sjekkAktivBrukerBryter();
  sjekkOvelse = lesOvelse();
  byttOvelse(sjekkOvelse);
  
  if (sjekkOvelse != aktivOvelse) {
    if (debounce(sisteDebouncePot)) {
      if (!harTrykketPot) {
        byttOvelse(sjekkOvelse); 
      }
      harTrykketPot = true;
      sisteDebouncePot = millis();
    }
  } else {
    harTrykketPot = false;
  }
  
 
  if (repsPerOvelse[aktivBruker][aktivOvelse] == maksRepsPerOvelse) {
    if (aktivOvelsePoeng < ovelserPerDag) {
      brukerOgPoengMatrise[aktivBruker][aktivOvelse]++;
      fullfortOvelse();
    }
    reset();
  }
  
  knapp = digitalRead(kp);
  if (knapp == 1) {
    if (digitalRead(kp) == 1 && debounce(sisteDebounceReps)) {
      if (!harTrykketReps) {
          if (repsPerOvelse[aktivBruker][aktivOvelse] < maksRepsPerOvelse && 
          brukerOgPoengMatrise[aktivBruker][aktivOvelse] != ovelserPerDag) {
            buttonClick();
          }
        harTrykketReps = true;
        sisteDebounceReps = millis();
      } 
    } 
  } else {
    harTrykketReps = false;
  }
}

void buttonClick() {
   repsPerOvelse[aktivBruker][aktivOvelse]++;
   // Lys opp en ny LED
   ring.setPixelColor(repsPerOvelse[aktivBruker][aktivOvelse] - 1, hvit);
   ring.show();
   Serial.println("Knapp trykket");
 }

void reset() {
  repsPerOvelse[aktivBruker][aktivOvelse] = 0;
  // Fjern LED lys
  resetRepLEDs();
}

void resetRepLEDs() {
  ring.fill(0, 0, maksRepsPerOvelse);
  ring.show();
}

boolean debounce(unsigned long sisteDebounce) {
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
  Serial.println("Ukentlig poeng lagt til:");
  Serial.println(ukentligPoeng[aktivBruker]);
  byte poengIByte = intTilByte(ukentligPoeng[aktivBruker]);
  //oppdaterVerdiIFil(filnavn(aktivBruker), poengIByte);
  skrivTilEEPROM(ukentligPoeng[aktivBruker]);
  //Serial.println("Naavaerende poeng for aktiv bruker: ");
  //Serial.println(hentVerdiIFil(filnavn(aktivBruker)));
  oppdaterUkentligLED();
  lesFraEEPROM(aktivBruker);
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
  ring.show();
}

int lesOvelse() {
    potVal = analogRead(potPin);
    angle = map(potVal, 0, 1023, 0, 179);
    
    int ovelse = aktivOvelse;
    if (angle >= 0 && angle < 60) {
      ovelse = 0;
      //Serial.println(angle);
      Serial.println("kom");
    } else if (angle >= 60 && angle < 120) {
      ovelse = 1;
      //Serial.println(angle);
      Serial.println("sitt");
    } else if (angle >= 120 && angle < 180) {
      ovelse = 2;
      //Serial.println(angle);
      Serial.println("bli");
    } 
    
    /*else {
      ovelse = aktivOvelse;
    }
    */
    
    return ovelse;
}


void byttOvelse(int nyOvelse) {
  
  if (aktivOvelse != 0 && nyOvelse == 0) {
      aktivOvelse = 0;
      Serial.println("Ovelse 0");
  } else if (aktivOvelse != 1 && nyOvelse == 1) {
      aktivOvelse = 1;
      Serial.println("Ovelse 1");
  } else if (aktivOvelse != 2 && nyOvelse == 2) {
      aktivOvelse = 2;
      Serial.println("Ovelse 2");
  }
 
  aktivOvelse = nyOvelse;
  // Sett repLEDs til 0, og fjern repetisjoner paa forrige ovelse
    resetRepLEDs();
    fyllRepLEDs();
    oppdaterOvelseLED(brukerOgPoengMatrise[aktivBruker][aktivOvelse]);
}

void fyllRepLEDs() {
  int reps = repsPerOvelse[aktivBruker][aktivOvelse];
  for (int i = 0; i < reps; i++) {
    ring.setPixelColor(i, hvit);
  }
  ring.show();
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

void oppdaterVerdiIFil(String filN, String nyVerdi) {
  File filen = SD.open(filN, FILE_WRITE);
  filen.remove();
  filen = SD.open(filN, FILE_WRITE);
  filen.println(nyVerdi);
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

void skrivTilEEPROM(int verdi) {
  EEPROM.write(aktivBruker, verdi);
}

byte lesFraEEPROM(int bruker) {
  byte verdi;
  verdi = EEPROM.read(bruker);
  Serial.print(bruker);
  Serial.print("\t");
  Serial.print(verdi, DEC);
  Serial.println();
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

String filnavn(int bruker) {
  String filNavn = "bruker" + String(bruker) + ".txt";
  return filNavn;
}

byte intTilByte (int verdi) {
  if (verdi < 256 && verdi > 0) {
    return (byte) verdi;
  }
}
