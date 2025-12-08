/****************************************************
 * SIMPLE SMART TRAFFIC LIGHT USING ESP32
 * - 2 directions: North-South (NS), East-West (EW)
 * - Traffic counts via buttons
 * - Pedestrian request via button
 ****************************************************/

// ============= PIN DEFINITIONS =============

// LED pins for North-South direction
const int PIN_NS_RED    = 2;   // NS Red LED connected to GPIO 2
const int PIN_NS_YELLOW = 4;   // NS Yellow LED connected to GPIO 4
const int PIN_NS_GREEN  = 5;   // NS Green LED connected to GPIO 5

// LED pins for East-West direction
const int PIN_EW_RED    = 18;  // EW Red LED connected to GPIO 18
const int PIN_EW_YELLOW = 19;  // EW Yellow LED connected to GPIO 19
const int PIN_EW_GREEN  = 21;  // EW Green LED connected to GPIO 21

// Pedestrian LEDs
const int PIN_PED_RED   = 22;  // Pedestrian "DON'T WALK" LED
const int PIN_PED_GREEN = 23;  // Pedestrian "WALK" LED

// Button pins (all active LOW with internal pull-up)
const int PIN_BTN_NS_TRAFFIC  = 12; // Button to increase NS traffic count
const int PIN_BTN_EW_TRAFFIC  = 13; // Button to increase EW traffic count
const int PIN_BTN_PED_REQUEST = 14; // Button to request pedestrian crossing

// ============= CONSTANTS =============

const unsigned long YELLOW_TIME_MS    = 3000;  // Yellow light duration = 3 seconds
const unsigned long PED_TIME_MS       = 8000;  // Pedestrian crossing duration = 8 seconds
const int MAX_TRAFFIC_COUNT           = 10;    // Maximum traffic count for each side

// ============= ENUM FOR STATES =============

// All possible traffic states
enum SignalState {
  STATE_NS_GREEN,    // NS side green, EW red
  STATE_NS_YELLOW,   // NS yellow, EW red
  STATE_EW_GREEN,    // EW green, NS red
  STATE_EW_YELLOW,   // EW yellow, NS red
  STATE_PED_GREEN    // All vehicles red, pedestrians green
};

// ============= GLOBAL VARIABLES =============

SignalState currentState = STATE_NS_GREEN;  // Start with NS green

unsigned long lastStateChangeTime = 0;      // Time when the state last changed
unsigned long currentStateDuration = 0;     // How long the current state should last (in ms)

int trafficCountNS = 0;                     // Manual traffic count for NS direction
int trafficCountEW = 0;                     // Manual traffic count for EW direction

bool pedRequest = false;                    // True if pedestrian button was pressed

// To detect button "press" (edge detection)
bool lastNsBtnState   = HIGH;               // Last read state of NS traffic button
bool lastEwBtnState   = HIGH;               // Last read state of EW traffic button
bool lastPedBtnState  = HIGH;               // Last read state of Pedestrian button

const unsigned long DEBOUNCE_DELAY_MS = 50; // Debounce delay for buttons (50 ms)
unsigned long lastDebounceTime = 0;         // Last time we changed button state

// ============= FUNCTION DECLARATIONS =============

void setAllRed();                           // Function to turn all vehicle signals to red
void applyStateToLeds();                    // Function to update LEDs based on currentState
void updateStateMachine();                  // Function to change states based on timer
void readButtons();                         // Function to read button presses
unsigned long computeGreenTimeNS();         // Compute NS green duration from traffic count
unsigned long computeGreenTimeEW();         // Compute EW green duration from traffic count

// ============= SETUP FUNCTION =============

void setup() {
  Serial.begin(115200);                     // Start serial communication at 115200 baud

  // Set LED pins as outputs
  pinMode(PIN_NS_RED, OUTPUT);
  pinMode(PIN_NS_YELLOW, OUTPUT);
  pinMode(PIN_NS_GREEN, OUTPUT);

  pinMode(PIN_EW_RED, OUTPUT);
  pinMode(PIN_EW_YELLOW, OUTPUT);
  pinMode(PIN_EW_GREEN, OUTPUT);

  pinMode(PIN_PED_RED, OUTPUT);
  pinMode(PIN_PED_GREEN, OUTPUT);

  // Set button pins as inputs with internal pull-up resistors
  pinMode(PIN_BTN_NS_TRAFFIC, INPUT_PULLUP);
  pinMode(PIN_BTN_EW_TRAFFIC, INPUT_PULLUP);
  pinMode(PIN_BTN_PED_REQUEST, INPUT_PULLUP);

  setAllRed();                              // Start with all vehicle signals red
  digitalWrite(PIN_PED_RED, HIGH);          // Pedestrian initially "DON'T WALK"
  digitalWrite(PIN_PED_GREEN, LOW);         // Pedestrian green OFF

  currentState = STATE_NS_GREEN;            // Start state = NS green
  currentStateDuration = computeGreenTimeNS(); // Set first state's duration based on NS traffic
  lastStateChangeTime = millis();           // Remember the current time

  Serial.println("Simple Traffic Light System Started."); // Debug message
}

// ============= MAIN LOOP FUNCTION =============

void loop() {
  readButtons();                            // Check if any button is pressed
  updateStateMachine();                     // Update state if its time is over
  applyStateToLeds();                       // Turn ON/OFF LEDs based on state
}

// ============= BUTTON READING FUNCTION =============

void readButtons() {
  unsigned long now = millis();             // Read current time in milliseconds

  // ---- NS traffic count button ----
  bool currentNsBtnState = digitalRead(PIN_BTN_NS_TRAFFIC); // Read NS traffic button
  if (currentNsBtnState != lastNsBtnState &&                // If button state changed
      (now - lastDebounceTime) > DEBOUNCE_DELAY_MS) {       // and debounce time passed
    lastDebounceTime = now;                                 // Update debounce timer
    if (currentNsBtnState == LOW) {                         // If button is pressed (active LOW)
      if (trafficCountNS < MAX_TRAFFIC_COUNT) {             // Limit the maximum count
        trafficCountNS++;                                   // Increase NS traffic count
      }
      Serial.print("NS traffic count = ");                  // Print updated count
      Serial.println(trafficCountNS);
    }
  }
  lastNsBtnState = currentNsBtnState;                       // Store current state for next loop

  // ---- EW traffic count button ----
  bool currentEwBtnState = digitalRead(PIN_BTN_EW_TRAFFIC); // Read EW traffic button
  if (currentEwBtnState != lastEwBtnState &&                // If button state changed
      (now - lastDebounceTime) > DEBOUNCE_DELAY_MS) {       // and debounce time passed
    lastDebounceTime = now;                                 // Update debounce timer
    if (currentEwBtnState == LOW) {                         // If button is pressed
      if (trafficCountEW < MAX_TRAFFIC_COUNT) {             // Limit the maximum count
        trafficCountEW++;                                   // Increase EW traffic count
      }
      Serial.print("EW traffic count = ");                  // Print updated count
      Serial.println(trafficCountEW);
    }
  }
  lastEwBtnState = currentEwBtnState;                       // Store current state

  // ---- Pedestrian request button ----
  bool currentPedBtnState = digitalRead(PIN_BTN_PED_REQUEST); // Read pedestrian button
  if (currentPedBtnState != lastPedBtnState &&                // If button state changed
      (now - lastDebounceTime) > DEBOUNCE_DELAY_MS) {         // and debounce time passed
    lastDebounceTime = now;                                   // Update debounce timer
    if (currentPedBtnState == LOW) {                          // If button is pressed
      pedRequest = true;                                      // Set pedestrian request flag
      Serial.println("Pedestrian crossing requested.");       // Print debug message
    }
  }
  lastPedBtnState = currentPedBtnState;                       // Store current state
}

// ============= STATE MACHINE FUNCTION =============

void updateStateMachine() {
  unsigned long now = millis();                               // Get current time

  // Check if current state's time is over
  if (now - lastStateChangeTime < currentStateDuration) {
    return;                                                   // If not over, do nothing
  }

  // If we reach here, time for current state is over
  switch (currentState) {
    case STATE_NS_GREEN:                                      // If NS was green
      currentState = STATE_NS_YELLOW;                         // Next: NS yellow
      currentStateDuration = YELLOW_TIME_MS;                  // Yellow fixed time
      break;

    case STATE_NS_YELLOW:                                     // If NS was yellow
      if (pedRequest) {                                       // If pedestrian requested
        currentState = STATE_PED_GREEN;                       // Next: pedestrian green
        currentStateDuration = PED_TIME_MS;                   // Pedestrian time
        pedRequest = false;                                   // Reset request
      } else {
        currentState = STATE_EW_GREEN;                        // Else go to EW green
        currentStateDuration = computeGreenTimeEW();          // EW green based on traffic
      }
      break;

    case STATE_EW_GREEN:                                      // If EW was green
      currentState = STATE_EW_YELLOW;                         // Next: EW yellow
      currentStateDuration = YELLOW_TIME_MS;                  // Yellow fixed time
      break;

    case STATE_EW_YELLOW:                                     // If EW was yellow
      if (pedRequest) {                                       // If pedestrian requested
        currentState = STATE_PED_GREEN;                       // Next: pedestrian green
        currentStateDuration = PED_TIME_MS;                   // Pedestrian time
        pedRequest = false;                                   // Reset request
      } else {
        currentState = STATE_NS_GREEN;                        // Else go back to NS green
        currentStateDuration = computeGreenTimeNS();          // NS green based on traffic
      }
      break;

    case STATE_PED_GREEN:                                     // If pedestrians were green
      currentState = STATE_NS_GREEN;                          // Go back to NS green
      currentStateDuration = computeGreenTimeNS();            // NS green based on traffic
      break;
  }

  lastStateChangeTime = now;                                  // Remember time of this change
}

// ============= LED CONTROL FUNCTIONS =============

void applyStateToLeds() {
  setAllRed();                                                // Start with all vehicle signals red
  digitalWrite(PIN_PED_RED, HIGH);                            // Pedestrian default: red ON
  digitalWrite(PIN_PED_GREEN, LOW);                           // Pedestrian green OFF

  switch (currentState) {
    case STATE_NS_GREEN:                                      // NS green state
      digitalWrite(PIN_NS_GREEN, HIGH);                       // NS green ON
      digitalWrite(PIN_NS_RED, LOW);                          // NS red OFF
      // EW remains red from setAllRed()
      break;

    case STATE_NS_YELLOW:                                     // NS yellow state
      digitalWrite(PIN_NS_YELLOW, HIGH);                      // NS yellow ON
      digitalWrite(PIN_NS_RED, LOW);                          // NS red OFF
      break;

    case STATE_EW_GREEN:                                      // EW green state
      digitalWrite(PIN_EW_GREEN, HIGH);                       // EW green ON
      digitalWrite(PIN_EW_RED, LOW);                          // EW red OFF
      break;

    case STATE_EW_YELLOW:                                     // EW yellow state
      digitalWrite(PIN_EW_YELLOW, HIGH);                      // EW yellow ON
      digitalWrite(PIN_EW_RED, LOW);                          // EW red OFF
      break;

    case STATE_PED_GREEN:                                     // Pedestrian crossing state
      setAllRed();                                            // All vehicle signals red
      digitalWrite(PIN_PED_RED, LOW);                         // Ped red OFF
      digitalWrite(PIN_PED_GREEN, HIGH);                      // Ped green ON (WALK)
      break;
  }
}

void setAllRed() {
  digitalWrite(PIN_NS_RED, HIGH);                             // NS red ON
  digitalWrite(PIN_NS_YELLOW, LOW);                           // NS yellow OFF
  digitalWrite(PIN_NS_GREEN, LOW);                            // NS green OFF

  digitalWrite(PIN_EW_RED, HIGH);                             // EW red ON
  digitalWrite(PIN_EW_YELLOW, LOW);                           // EW yellow OFF
  digitalWrite(PIN_EW_GREEN, LOW);                            // EW green OFF
}

// ============= GREEN TIME COMPUTATION FUNCTIONS =============

unsigned long computeGreenTimeNS() {
  // Map NS traffic count to green time
  if (trafficCountNS <= 3) {                                  // Few vehicles
    return 10000;                                             // 10 seconds
  } else if (trafficCountNS <= 7) {                           // Medium traffic
    return 20000;                                             // 20 seconds
  } else {                                                    // Heavy traffic
    return 30000;                                             // 30 seconds
  }
}

unsigned long computeGreenTimeEW() {
  // Map EW traffic count to green time
  if (trafficCountEW <= 3) {                                  // Few vehicles
    return 10000;                                             // 10 seconds
  } else if (trafficCountEW <= 7) {                           // Medium traffic
    return 20000;                                             // 20 seconds
  } else {                                                    // Heavy traffic
    return 30000;                                             // 30 seconds
  }
}
