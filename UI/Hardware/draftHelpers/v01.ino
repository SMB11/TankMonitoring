const int trigPins[] = {22, 24, 26};  // Trigger pins for ultrasonic sensors
const int echoPins[] = {23, 25, 27};  // Echo pins for ultrasonic sensors

const int buttonPin = 2;              // Single button pin
const int alarmRelayPin = 4;         // Relay pin for alarm   3 for siren

// Constants
const int IDLE = 0;
const int FILLING = 1;
const int DRAINING = 2;
const int ALARM = 3;

const int fillThreshold = 90;  // Percentage for filling alarm
const int drainThreshold = 10; // Percentage for draining alarm
const int debounceDelay = 50;  // Delay for button debouncing

// Variables for consistency checks
unsigned long lastFillingCheckTimes[3] = {0, 0, 0};
unsigned long lastDrainingCheckTimes[3] = {0, 0, 0};
unsigned long lastIdleCheckTimes[3] = {0, 0, 0};
const unsigned long consistencyInterval = 5000; // Time interval for consistency checks
const int distanceThreshold = 10; // Threshold for significant distance change

// Variables for each tank
int tankStates[] = {IDLE, IDLE, IDLE};
int distances[] = {0, 0, 0};
bool alarmActiveFlags[] = {false, false, false};
bool acknowledgedFlags[] = {false, false, false};

int lastActiveTank = -1; // To track the last tank that triggered the alarm

void setup() {
  for (int i = 0; i < 3; i++) {
    pinMode(sensorPins[i], INPUT);
  }
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(alarmRelayPin, OUTPUT);
}

void loop() {
  for (int i = 0; i < 3; i++) {
    distances[i] = getDistance(sensorPins[i]);  // Read sensor values

    // Update tank states based on sensor readings and consistency checks
    if (tankStates[i] == IDLE) {
      if (checkConsistentFilling(i)) {
        tankStates[i] = FILLING;
      } else if (checkConsistentDraining(i)) {
        tankStates[i] = DRAINING;
      }
    } else if (tankStates[i] == FILLING) {
      if (distances[i] >= (fillThreshold * tankHeight / 100)) {
        alarmActiveFlags[i] = true;
        lastActiveTank = i;
      } else if (checkConsistentIdle(i)) {
        tankStates[i] = IDLE;
      }
    } else if (tankStates[i] == DRAINING) {
      if (distances[i] <= (drainThreshold * tankHeight / 100)) {
        alarmActiveFlags[i] = true;
        lastActiveTank = i;
      } else if (checkConsistentIdle(i)) {
        tankStates[i] = IDLE;
      }
    }
  }

  // Manage alarm activation and user acknowledgment
  if (anyAlarmActive()) {
    activateAlarm(alarmRelayPin);
  } else if (acknowledgedFlags[lastActiveTank]) {
    deactivateAlarm(alarmRelayPin);
    tankStates[lastActiveTank] = IDLE;
    alarmActiveFlags[lastActiveTank] = false;
    acknowledgedFlags[lastActiveTank] = false;
  }

  // Check for button press (with debouncing)
  if (digitalRead(buttonPin) == LOW) {
    delay(debounceDelay);
    if (digitalRead(buttonPin) == LOW) {
      acknowledgedFlags[lastActiveTank] = true; // Acknowledge the last active tank
    }
  }
}

// Helper functions
bool anyAlarmActive() {
  for (int i = 0; i < 3; i++) {
    if (alarmActiveFlags[i]) {
      return true;
    }
  }
  return false;
}




int getDistance(int sensorPin) {
  // Code to read distance using JSN-SR04T sensor (replace with library function
