 /*
  DogTracker 0.2.5  Alpha

  Dings som kan hjelpe til aa motivere hundeeiere ved aa lagre og vise ukentlig treningsprogresjon.

  Lar to forskjellige brukere bruke samme dings til å konkurrere. 

  Med LED-ring og piezo til feedback, og knapp, slideswitch, og potensiometer som input.

  Poeng lagres i EEPROM-minnet til Arduino, ettersom det ikke er behov for stor lagringsplass.

  Guide til LED: 

  https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use

*/

// Import
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
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

// Innstillinger
const int ovelserPerDag = 3;
const int maksRepsPerOvelse = 7;
const int antallOvelser = 3;
const int antallBrukere = 2;
const int startPunktOvelseLED = 11;

// Debounce
const int debounceTid = 300;

// Debounce repetisjonsknapp
int knapp;
unsigned long sisteDebounceReps = 0;
boolean harTrykketReps = false;

// Debounce for bytte ovelse, for aa unngaa at input fra potensiometer
// "hopper" fram og tilbake
int bryter;
unsigned long sisteDebouncePot = 0;
boolean harEndretPot = false;

// Variabler som oppdateres globalt
int sjekkOvelse;
int aktivOvelse;
int aktivBruker;

// Potensiometer
int potVal;
int angle;

// Poengoversikt
// Aktiv bruker leses fra en bryter som er enten 0 eller 1. 
// Spiller 1 er 0, spiller 2 er 1. Antall repetisjoner og sett
// lagres i de todimensjonale arrayne under, hvor variablen aktivBruker
// brukes som indeksen for aa angi hvilke bruker man skal registrere for 
// eller lese fra.

int repsPerOvelse[antallBrukere][antallOvelser];
int brukerOgPoengMatrise[antallBrukere][antallOvelser];
int aktivOvelsePoeng;
int ukentligPoeng[antallBrukere];

void setup() {
  Serial.begin(9600);
  // Start LED-ring
  ring.begin();
  ring.setBrightness(32);
  ring.show();

  sjekkAktivBrukerBryter();
  int annenBruker = hentAnnenBruker();

  // Hent ukentlige poeng
  ukentligPoeng[aktivBruker] = lesFraEEPROM(aktivBruker);
  ukentligPoeng[annenBruker] = lesFraEEPROM(annenBruker);

  // Sjekk aktiv ovelse, oppdater LED
  aktivOvelse = lesOvelse();
  aktivOvelsePoeng = brukerOgPoengMatrise[aktivBruker][aktivOvelse];
  oppdaterOvelseLED(aktivOvelsePoeng);

  // Oppsett for knapp og piezo
  pinMode(kp, INPUT);
  pinMode(pz, OUTPUT);

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

  // Les aktiv ovelse, debounce signalet, og oppdater hvis endring
  if (sjekkOvelse != aktivOvelse) {
    if (debounce(sisteDebouncePot)) {
      if (!harEndretPot) {
        byttOvelse(sjekkOvelse); 
      }
      harEndretPot = true;
      sisteDebouncePot = millis();
    }
  } else {
    harEndretPot = false;
  }
  
 // Hvis man har naadd maks repetisjoner per ovelse, legg til poeng 
 // nullstill repetisjonsLEDs
  if (repsPerOvelse[aktivBruker][aktivOvelse] == maksRepsPerOvelse) {
    if (aktivOvelsePoeng < ovelserPerDag) {
      brukerOgPoengMatrise[aktivBruker][aktivOvelse]++;
      fullfortOvelse();
    }
    reset();
  }

  // Les knapp med debounce, registrer repetisjon hvis trykket inn
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
 }

void reset() {
  // Nullstill repetisjoner
  repsPerOvelse[aktivBruker][aktivOvelse] = 0;
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
  // Lyd fra piezo:
  tone(pz,988,100);
  delay(100);
  tone(pz,1319,850);
  delay(800);
  noTone(pz);

  // Oppdater ring og skriv poeng til EEPROM:
  ring.setPixelColor(startPunktOvelseLED - brukerOgPoengMatrise[aktivBruker][aktivOvelse],blaa);
  ukentligPoeng[aktivBruker]++;
  skrivTilEEPROM(ukentligPoeng[aktivBruker]);
  oppdaterUkentligLED();
  lesFraEEPROM(aktivBruker);
}

void oppdaterOvelseLED(int poeng) {
  // Nullstill de tre ovelse-LEDene
  ring.setPixelColor(10, 0);
  ring.setPixelColor(9, 0);
  ring.setPixelColor(8, 0);

  // Sjekk antall poeng og oppdater LEDs
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
  // Aktiv bruker = LED 11
  // Annen bruker = LED 7
  // Hent poeng for aktiv og annen bruker
  
  int annenBruker = hentAnnenBruker();
  int aktivBrukerPoeng = ukentligPoeng[aktivBruker];
  int annenBrukerPoeng = ukentligPoeng[annenBruker];
  
  // Sammenlign poeng og oppdater. Brukeren som leder skal ha 
  // gront lys, den andre skal ha rodt. Hvis de har like mye poeng
  // skal begge ha gult lys
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
    // Konverterer input fra potensiometer til grader
    
    angle = map(potVal, 0, 1023, 0, 179);
    
    int ovelse = aktivOvelse;
    if (angle >= 0 && angle < 60) {
      ovelse = 0;
    } else if (angle >= 60 && angle < 120) {
      ovelse = 1;
    } else if (angle >= 120 && angle < 180) {
      ovelse = 2;
    } 
    
    return ovelse;
}


void byttOvelse(int nyOvelse) {
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

void hentUkentligPoeng() {
  // Hent ukentlig poeng fra matrisene til global variabel
  for (int i = 0; i < antallBrukere; i++) {
    for (int j = 0; j < antallOvelser; j++) {
      ukentligPoeng[i] += brukerOgPoengMatrise[i][j];
    }
  }
}

void skrivTilEEPROM(int verdi) {
  // Sjekk om verdi ikke er storre enn en byte
  if (verdi < 256 && verdi >= 0) {
    EEPROM.write(aktivBruker, verdi);
  }
  
}

byte lesFraEEPROM(int bruker) {
  // Henter data fra EEPROM-minne, og 
  byte verdi;
  verdi = EEPROM.read(bruker);
  return verdi;
}
