#include <NewPing.h>

// Define pins for ultrasonic sensors
#define TRIGGER_PIN_1  22
#define ECHO_PIN_1     23
#define TRIGGER_PIN_2  24
#define ECHO_PIN_2     25
#define TRIGGER_PIN_3  26
#define ECHO_PIN_3     27

// Define pin for pushbutton and alarm relay
#define PUSHBUTTON_PIN  2
#define ALARM_RELAY_PIN 4

// Maximum distance to measure (in cm)
#define MAX_DISTANCE 300

// Create NewPing objects for each ultrasonic sensor
NewPing sonar1(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE);
NewPing sonar2(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE);
NewPing sonar3(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE);

// Tank level thresholds
int tankMaximums[] = {60, 55, 35}; // Example maximum values in cm
int tankMinimums[] = {210, 190, 169}; // Example minimum values in cm

// Debounce variables for pushbutton
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Variable to track if user has reacted
bool userReacted = false;

// Startup delay variables
const unsigned long startupDelay = 5000; // 5 seconds delay for sensor stabilization
unsigned long startupTime;

void setup() {
  pinMode(PUSHBUTTON_PIN, INPUT);
  pinMode(ALARM_RELAY_PIN, OUTPUT);
  digitalWrite(ALARM_RELAY_PIN,HIGH);
  startupTime = millis();
  Serial.begin(9600);
}

void loop() {

  if (millis() - startupTime < startupDelay) {
    Serial.println("Startup delay. Waiting for sensor stabilization...");
    return; // Skip the rest of the loop during startup delay
  }

  int distances[] = {getFilteredDistance(&sonar1), getFilteredDistance(&sonar2), getFilteredDistance(&sonar3)};
  bool alarmState = false;

  // Debug: Print distances
  Serial.print("Distances: Tank 1 = "); Serial.print(distances[0]);
  Serial.print(", Tank 2 = "); Serial.print(distances[1]);
  Serial.print(", Tank 3 = "); Serial.println(distances[2]);

  // Check if any tank is full or empty
  for (int i = 0; i < 3; i++) {
    if ((distances[i] <= tankMaximums[i] || distances[i] >= tankMinimums[i]) && !userReacted) {
      alarmState = true;
      Serial.print("Alarm triggered by Tank "); Serial.println(i+1);
      break;
    }
  }

  // Debounce the pushbutton
  int reading = digitalRead(PUSHBUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == HIGH) {
      userReacted = true; // User has reacted
      alarmState = false; // Turn off alarm when button is pressed
      Serial.println("User reacted. Alarm off.");
    }
  }
  lastButtonState = reading;

  // If user has reacted, check if the situation has changed
  if (userReacted) {
    bool situationChanged = false;
    for (int i = 0; i < 3; i++) {
      if (distances[i] > tankMaximums[i] && distances[i] < tankMinimums[i]) {
        situationChanged = true;
        break;
      }
    }
    if (situationChanged) {
      userReacted = false;
      Serial.println("Situation changed. User reaction reset.");
    }
  }

  // Control the alarm
  digitalWrite(ALARM_RELAY_PIN, alarmState ? LOW : HIGH);
}

int getFilteredDistance(NewPing *sensor) {
  int sum = 0;
  const int sampleSize = 5;
  for (int i = 0; i < sampleSize; i++) {
    int distance = sensor->ping_cm();
    if (distance > 0 && distance <= MAX_DISTANCE) {
      sum += distance;
    } else {
      sum += MAX_DISTANCE; // Consider out of range as max distance
    }
    delay(10); // Short delay between pings
  }
  return sum / sampleSize;
}
