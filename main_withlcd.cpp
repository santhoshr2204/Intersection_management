/****************************************************
 * SIMPLE SMART TRAFFIC LIGHT USING ESP32
 * WITH 16x2 I2C LCD DISPLAY (NO SERIAL MONITOR)
 * - 2 directions: NS and EW
 * - Traffic count via buttons
 * - Pedestrian phase via button
 ****************************************************/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Adjust these if your LCD address is different
// Common I2C addresses: 0x27 or 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ============= PIN DEFINITIONS =============

// LED pins for North-South direction
const int PIN_NS_RED    = 2;
const int PIN_NS_YELLOW = 4;
const int PIN_NS_GREEN  = 5;

// LED pins for East-West direction
const int PIN_EW_RED    = 18;
const int PIN_EW_YELLOW = 19;
const int PIN_EW_GREEN  = 21;

// Pedestrian LEDs
const int PIN_PED_RED   = 22;
const int PIN_PED_GREEN = 23;

// Buttons (active LOW, with INPUT_PULLUP)
const int PIN_BTN_NS_TRAFFIC  = 12; // Increase NS traffic count
const int PIN_BTN_EW_TRAFFIC  = 13; // Increase EW traffic count
const int PIN_BTN_PED_REQUEST = 14; // Request pedestrian crossing

// ============= CONSTANTS =============

const int YELLOW_TIME_SEC   = 3;   // 3 seconds yellow
const int PED_TIME_SEC      = 8;   // 8 seconds pedestrian green
const int MAX_TRAFFIC_COUNT = 10;  // Max count per direction

// ============= GLOBAL VARIABLES =============

// Manual traffic counts
int trafficCountNS = 0;
int trafficCountEW = 0;

// Pedestrian request flag
bool pedRequest = false;

// For simple edge detection & debounce using delay()
bool lastNsBtnState  = HIGH;
bool lastEwBtnState  = HIGH;
bool lastPedBtnState = HIGH;

// ============= FUNCTION DECLARATIONS =============

void readButtons();                    // handle button presses
void phaseNsGreen();                   // NS green phase
void phaseNsYellow();                  // NS yellow phase
void phaseEwGreen();                   // EW green phase
void phaseEwYellow();                  // EW yellow phase
void phasePedestrianIfRequested();     // Pedestrian phase (only if requested)

void setAllVehicleRed();               // helper: all vehicle signals red
void setNsGreenState();                // helper: NS green, EW red
void setNsYellowState();               // helper: NS yellow, EW red
void setEwGreenState();                // helper: EW green, NS red
void setEwYellowState();               // helper: EW yellow, NS red
void setPedestrianGreenState();        // helper: pedestrians green

int computeNsGreenSeconds();           // compute NS green from traffic count
int computeEwGreenSeconds();           // compute EW green from traffic count

void lcdShowTwoLines(const char* line1, const char* line2); // helper to print on LCD

// ============= SETUP =============

void setup() {
  // Initialize LCD
  lcd.init();          // Initialize LCD
  lcd.backlight();     // Turn on backlight
  lcdShowTwoLines("Traffic System", "Starting...");

  // LED pins
  pinMode(PIN_NS_RED, OUTPUT);
  pinMode(PIN_NS_YELLOW, OUTPUT);
  pinMode(PIN_NS_GREEN, OUTPUT);

  pinMode(PIN_EW_RED, OUTPUT);
  pinMode(PIN_EW_YELLOW, OUTPUT);
  pinMode(PIN_EW_GREEN, OUTPUT);

  pinMode(PIN_PED_RED, OUTPUT);
  pinMode(PIN_PED_GREEN, OUTPUT);

  // Button pins with pull-ups
  pinMode(PIN_BTN_NS_TRAFFIC, INPUT_PULLUP);
  pinMode(PIN_BTN_EW_TRAFFIC, INPUT_PULLUP);
  pinMode(PIN_BTN_PED_REQUEST, INPUT_PULLUP);

  // Initial safe state
  setAllVehicleRed();
  digitalWrite(PIN_PED_RED, HIGH);
  digitalWrite(PIN_PED_GREEN, LOW);

  lcdShowTwoLines("Traffic System", "Ready.");
  delay(1500);
}

// ============= MAIN LOOP =============

void loop() {
  // Run one full traffic cycle repeatedly

  phaseNsGreen();               // 1) NS green, EW red (adaptive time)
  phaseNsYellow();              // 2) NS yellow
  phasePedestrianIfRequested(); // optional 3) pedestrian crossing

  phaseEwGreen();               // 4) EW green, NS red (adaptive time)
  phaseEwYellow();              // 5) EW yellow
  phasePedestrianIfRequested(); // optional 6) pedestrian crossing
}

// ============= BUTTON HANDLING =============

void readButtons() {
  // Very simple "edge detection" of button press + small delay-based debounce

  // --- NS traffic button ---
  bool currentNs = digitalRead(PIN_BTN_NS_TRAFFIC);
  if (currentNs == LOW && lastNsBtnState == HIGH) {   // just pressed
    if (trafficCountNS < MAX_TRAFFIC_COUNT) {
      trafficCountNS++;
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NS traffic cnt");
    lcd.setCursor(0, 1);
    lcd.print("NS = ");
    lcd.print(trafficCountNS);
    delay(400);  // small pause to read
  }
  lastNsBtnState = currentNs;

  // --- EW traffic button ---
  bool currentEw = digitalRead(PIN_BTN_EW_TRAFFIC);
  if (currentEw == LOW && lastEwBtnState == HIGH) {   // just pressed
    if (trafficCountEW < MAX_TRAFFIC_COUNT) {
      trafficCountEW++;
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("EW traffic cnt");
    lcd.setCursor(0, 1);
    lcd.print("EW = ");
    lcd.print(trafficCountEW);
    delay(400);
  }
  lastEwBtnState = currentEw;

  // --- Pedestrian request button ---
  bool currentPed = digitalRead(PIN_BTN_PED_REQUEST);
  if (currentPed == LOW && lastPedBtnState == HIGH) { // just pressed
    pedRequest = true;                                // set request flag
    lcdShowTwoLines("Pedestrian Req", "Stored.");
    delay(400);
  }
  lastPedBtnState = currentPed;
}

// ============= PHASE FUNCTIONS =============

void phaseNsGreen() {
  int greenSeconds = computeNsGreenSeconds();   // green time based on NS traffic

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("NS GREEN");
  lcd.setCursor(0, 1);
  lcd.print("T=");
  lcd.print(greenSeconds);
  lcd.print("s  ");

  setNsGreenState();  // set LEDs: NS green, EW red

  for (int s = 0; s < greenSeconds; s++) {
    readButtons();    // allow new inputs
    delay(1000);      // wait 1 second
  }

  trafficCountNS = 0; // reset after serving
}

void phaseNsYellow() {
  lcdShowTwoLines("NS YELLOW", "T=3s");

  setNsYellowState(); // NS yellow, EW red

  for (int s = 0; s < YELLOW_TIME_SEC; s++) {
    readButtons();
    delay(1000);
  }
}

void phaseEwGreen() {
  int greenSeconds = computeEwGreenSeconds();   // green time based on EW traffic

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EW GREEN");
  lcd.setCursor(0, 1);
  lcd.print("T=");
  lcd.print(greenSeconds);
  lcd.print("s  ");

  setEwGreenState();  // EW green, NS red

  for (int s = 0; s < greenSeconds; s++) {
    readButtons();
    delay(1000);
  }

  trafficCountEW = 0; // reset after serving
}

void phaseEwYellow() {
  lcdShowTwoLines("EW YELLOW", "T=3s");

  setEwYellowState(); // EW yellow, NS red

  for (int s = 0; s < YELLOW_TIME_SEC; s++) {
    readButtons();
    delay(1000);
  }
}

void phasePedestrianIfRequested() {
  if (!pedRequest) {
    return; // nothing to do
  }

  lcdShowTwoLines("PEDESTRIAN", "GREEN WALK");

  setPedestrianGreenState(); // vehicles red, pedestrians green

  for (int s = 0; s < PED_TIME_SEC; s++) {
    // Can still read buttons to update traffic counts
    readButtons();
    delay(1000);
  }

  // End of pedestrian phase
  setAllVehicleRed();
  digitalWrite(PIN_PED_RED, HIGH);
  digitalWrite(PIN_PED_GREEN, LOW);

  lcdShowTwoLines("PEDESTRIAN", "STOP");

  pedRequest = false;
  delay(500);
}

// ============= LED STATE HELPERS =============

void setAllVehicleRed() {
  digitalWrite(PIN_NS_RED, HIGH);
  digitalWrite(PIN_NS_YELLOW, LOW);
  digitalWrite(PIN_NS_GREEN, LOW);

  digitalWrite(PIN_EW_RED, HIGH);
  digitalWrite(PIN_EW_YELLOW, LOW);
  digitalWrite(PIN_EW_GREEN, LOW);
}

void setNsGreenState() {
  setAllVehicleRed();
  digitalWrite(PIN_NS_RED, LOW);    // NS red off
  digitalWrite(PIN_NS_GREEN, HIGH); // NS green on
}

void setNsYellowState() {
  setAllVehicleRed();
  digitalWrite(PIN_NS_RED, LOW);     // NS red off
  digitalWrite(PIN_NS_YELLOW, HIGH); // NS yellow on
}

void setEwGreenState() {
  setAllVehicleRed();
  digitalWrite(PIN_EW_RED, LOW);     // EW red off
  digitalWrite(PIN_EW_GREEN, HIGH);  // EW green on
}

void setEwYellowState() {
  setAllVehicleRed();
  digitalWrite(PIN_EW_RED, LOW);      // EW red off
  digitalWrite(PIN_EW_YELLOW, HIGH);  // EW yellow on
}

void setPedestrianGreenState() {
  setAllVehicleRed();                 // all vehicles red
  digitalWrite(PIN_PED_RED, LOW);     // ped red off
  digitalWrite(PIN_PED_GREEN, HIGH);  // ped green on
}

// ============= GREEN TIME COMPUTATION =============

int computeNsGreenSeconds() {
  if (trafficCountNS <= 3) {
    return 10;   // light traffic
  } else if (trafficCountNS <= 7) {
    return 20;   // medium
  } else {
    return 30;   // heavy
  }
}

int computeEwGreenSeconds() {
  if (trafficCountEW <= 3) {
    return 10;
  } else if (trafficCountEW <= 7) {
    return 20;
  } else {
    return 30;
  }
}

// ============= LCD HELPER =============

void lcdShowTwoLines(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}
