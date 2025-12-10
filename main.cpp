#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);   // Change address to 0x3F if needed

const int PIN_NS_RED    = 2;
const int PIN_NS_YELLOW = 4;
const int PIN_NS_GREEN  = 5;

const int PIN_EW_RED    = 18;
const int PIN_EW_YELLOW = 19;
const int PIN_EW_GREEN  = 21;

const int PIN_PED_RED   = 22;
const int PIN_PED_GREEN = 23;

const int PIN_BTN_NS_TRAFFIC  = 12;   
const int PIN_BTN_EW_TRAFFIC  = 13;   
const int PIN_BTN_PED_REQUEST = 14;   

const int YELLOW_TIME_SEC   = 3;
const int PED_TIME_SEC      = 8;
const int BASE_GREEN_SEC    = 10;     

enum Phase {
  PHASE_NS_GREEN,
  PHASE_NS_YELLOW,
  PHASE_EW_GREEN,
  PHASE_EW_YELLOW,
  PHASE_PED_GREEN
};

Phase currentPhase = PHASE_NS_GREEN;

int trafficCountNS = 0;   
int trafficCountEW = 0;   

bool pedRequest = false;  

bool lastNsBtnState  = HIGH;
bool lastEwBtnState  = HIGH;
bool lastPedBtnState = HIGH;

void readButtons();
void waitOneSecondWithButtons();

void phaseNsGreen();
void phaseNsYellow();
void phaseEwGreen();
void phaseEwYellow();
void phasePedestrianIfRequested();

void setAllVehicleRed();
void setNsGreenState();
void setNsYellowState();
void setEwGreenState();
void setEwYellowState();
void setPedestrianGreenState();

bool isNsRed();
bool isEwRed();

int  computeNsGreenSeconds();
int  computeEwGreenSeconds();

void lcdShowTwoLines(const char* line1, const char* line2);

void setup() {
  // Use GPIO32 as SDA and GPIO33 as SCL for I2C
  Wire.begin(32, 33);

  lcd.init();
  lcd.backlight();
  lcdShowTwoLines("Traffic System", "Starting...");
  delay(1000);

  pinMode(PIN_NS_RED, OUTPUT);
  pinMode(PIN_NS_YELLOW, OUTPUT);
  pinMode(PIN_NS_GREEN, OUTPUT);

  pinMode(PIN_EW_RED, OUTPUT);
  pinMode(PIN_EW_YELLOW, OUTPUT);
  pinMode(PIN_EW_GREEN, OUTPUT);

  pinMode(PIN_PED_RED, OUTPUT);
  pinMode(PIN_PED_GREEN, OUTPUT);

  pinMode(PIN_BTN_NS_TRAFFIC, INPUT_PULLUP);
  pinMode(PIN_BTN_EW_TRAFFIC, INPUT_PULLUP);
  pinMode(PIN_BTN_PED_REQUEST, INPUT_PULLUP);

  setAllVehicleRed();
  digitalWrite(PIN_PED_RED, HIGH);
  digitalWrite(PIN_PED_GREEN, LOW);

  lcdShowTwoLines("Traffic System", "Ready");
  delay(1000);
}

void setAllVehicleRed() {
  digitalWrite(PIN_NS_RED, HIGH);
  digitalWrite(PIN_NS_YELLOW, LOW);
  digitalWrite(PIN_NS_GREEN, LOW);

  digitalWrite(PIN_EW_RED, HIGH);
  digitalWrite(PIN_EW_YELLOW, LOW);
  digitalWrite(PIN_EW_GREEN, LOW);
}

void loop() {

  phaseNsGreen();
  phaseNsYellow();
  phasePedestrianIfRequested();   

  phaseEwGreen();
  phaseEwYellow();
  phasePedestrianIfRequested();   
}

void readButtons() {
  bool nsBtn = digitalRead(PIN_BTN_NS_TRAFFIC);
  if (nsBtn == LOW && lastNsBtnState == HIGH) {      
    if (isNsRed()) {                                 
      trafficCountNS++;                              

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("NS RED: Count");
      lcd.setCursor(0, 1);
      lcd.print("NS=");
      lcd.print(trafficCountNS);
    } else {
      lcdShowTwoLines("NS not RED", "No count");
    }
    delay(30);   
  }
  lastNsBtnState = nsBtn;

  bool ewBtn = digitalRead(PIN_BTN_EW_TRAFFIC);
  if (ewBtn == LOW && lastEwBtnState == HIGH) {      
    if (isEwRed()) {                                 
      trafficCountEW++;                              

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("EW RED: Count");
      lcd.setCursor(0, 1);
      lcd.print("EW=");
      lcd.print(trafficCountEW);
    } else {
      lcdShowTwoLines("EW not RED", "No count");
    }
    delay(30);   
  }
  lastEwBtnState = ewBtn;

  bool pedBtn = digitalRead(PIN_BTN_PED_REQUEST);
  if (pedBtn == LOW && lastPedBtnState == HIGH) {    
    pedRequest = true;                               
    lcdShowTwoLines("Pedestrian Request", "Recieved");
    delay(30);
  }
  lastPedBtnState = pedBtn;
}
bool isNsRed() {
  return (currentPhase == PHASE_EW_GREEN ||
          currentPhase == PHASE_EW_YELLOW ||
          currentPhase == PHASE_PED_GREEN);
}

bool isEwRed() {
  return (currentPhase == PHASE_NS_GREEN ||
          currentPhase == PHASE_NS_YELLOW ||
          currentPhase == PHASE_PED_GREEN);
}




void phaseNsGreen() {
  currentPhase = PHASE_NS_GREEN;
  int totalSecs = computeNsGreenSeconds();
  int baseSecs  = BASE_GREEN_SEC;
  int extraSecs = totalSecs - baseSecs;
  if (extraSecs < 0) extraSecs = 0;

  setNsGreenState();
  for (int remaining = totalSecs; remaining > 0; remaining--) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NS Green ");
    lcd.print(baseSecs);
    lcd.print("+");
    lcd.print(extraSecs);
    lcd.print("s");
    lcd.setCursor(0, 1);
    lcd.print("T=");
    lcd.print(remaining);
    lcd.print(" EW=");
    lcd.print(trafficCountEW);   

    waitOneSecondWithButtons();  
  }
  trafficCountNS = 0;
}

int computeNsGreenSeconds() {
  int extra = 0;
  if (trafficCountNS >= 15) {
    extra = 30;
  } else if (trafficCountNS >= 10) {
    extra = 20;
  } else if (trafficCountNS >= 5) {
    extra = 10;
  } else {
    extra = 0;
  }
  return BASE_GREEN_SEC + extra;
}

void setNsGreenState() {
  setAllVehicleRed();
  digitalWrite(PIN_NS_RED, LOW);
  digitalWrite(PIN_NS_GREEN, HIGH);
}

void waitOneSecondWithButtons() {
  for (int i = 0; i < 50; i++) {
    readButtons();
    delay(20);
  }
}

void phaseNsYellow() {
  currentPhase = PHASE_NS_YELLOW;
  for (int remaining = YELLOW_TIME_SEC; remaining > 0; remaining--) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NS Yellow T=");
    lcd.print(remaining);
    lcd.print("s");

    lcd.setCursor(0, 1);
    lcd.print("EW=");
    lcd.print(trafficCountEW);

    setNsYellowState();
    waitOneSecondWithButtons();
  }
}

void setNsYellowState() {
  setAllVehicleRed();
  digitalWrite(PIN_NS_RED, LOW);
  digitalWrite(PIN_NS_YELLOW, HIGH);
}


void phaseEwGreen() {
  currentPhase = PHASE_EW_GREEN;

  int totalSecs = computeEwGreenSeconds();
  int baseSecs  = BASE_GREEN_SEC;
  int extraSecs = totalSecs - baseSecs;
  if (extraSecs < 0) extraSecs = 0;

  setEwGreenState();
  for (int remaining = totalSecs; remaining > 0; remaining--) {
    lcd.clear();
    lcd.setCursor(0, 0);
    
    lcd.print("EW Green ");
    lcd.print(baseSecs);
    lcd.print("+");
    lcd.print(extraSecs);
    lcd.print("s");
    lcd.setCursor(0, 1);
    lcd.print("T=");
    lcd.print(remaining);
    lcd.print(" NS=");
    lcd.print(trafficCountNS);   

    waitOneSecondWithButtons();
  }

  trafficCountEW = 0;
}

int computeEwGreenSeconds() {
  int extra = 0;
  if (trafficCountEW >= 15) {
    extra = 30;
  } else if (trafficCountEW >= 10) {
    extra = 20;
  } else if (trafficCountEW >= 5) {
    extra = 10;
  } else {
    extra = 0;
  }
  return BASE_GREEN_SEC + extra;
}

void lcdShowTwoLines(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void setEwGreenState() {
  setAllVehicleRed();
  digitalWrite(PIN_EW_RED, LOW);
  digitalWrite(PIN_EW_GREEN, HIGH);
}

void phaseEwYellow() {
  currentPhase = PHASE_EW_YELLOW;
  for (int remaining = YELLOW_TIME_SEC; remaining > 0; remaining--) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("EW Yellow T=");
    lcd.print(remaining);
    lcd.print("s");

    lcd.setCursor(0, 1);
    lcd.print("NS=");
    lcd.print(trafficCountNS);

    setEwYellowState();
    waitOneSecondWithButtons();
  }
}

void setEwYellowState() {
  setAllVehicleRed();
  digitalWrite(PIN_EW_RED, LOW);
  digitalWrite(PIN_EW_YELLOW, HIGH);
}

void phasePedestrianIfRequested() {
  if (!pedRequest) return;   

  currentPhase = PHASE_PED_GREEN;

  setPedestrianGreenState();

  for (int remaining = PED_TIME_SEC; remaining > 0; remaining--) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PEDESTRIAN");
    lcd.setCursor(0, 1);
    lcd.print("T=");
    lcd.print(remaining);
    lcd.print(" WALK");

    waitOneSecondWithButtons();
  }

  setAllVehicleRed();
  digitalWrite(PIN_PED_RED, HIGH);
  digitalWrite(PIN_PED_GREEN, LOW);

  lcdShowTwoLines("PEDESTRIAN", "STOP");
  delay(500);

  pedRequest = false;
}

void setPedestrianGreenState() {
  setAllVehicleRed();
  digitalWrite(PIN_PED_RED, LOW);
  digitalWrite(PIN_PED_GREEN, HIGH);
}
