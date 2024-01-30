#include <NewPing.h>  // Include the NewPing library

// Define pins for each sensor
#define TRIGGER_PIN_1  22
#define ECHO_PIN_1     23
#define TRIGGER_PIN_2  24
#define ECHO_PIN_2     25
#define TRIGGER_PIN_3  26
#define ECHO_PIN_3     27


#define buttonPin 2
#define alarmRelayPin 4


// Constants
const int IDLE = 0;
const int FILLING = 1;
const int DRAINING = 2;
const int ALARM = 3;


const int fillThreshold = 90;  // Percentage for filling alarm
const int drainThreshold = 10; // Percentage for draining alarm
const int debounceDelay = 50;  // Delay for button debouncing

// int tankHeights[] = {210, 300, 150}; 

int tankMaximums[] = {60, 55, 35}; 
int tankMinimums[] = {210, 190, 169}; 

// Variables for consistency checks
unsigned long lastFillingCheckTimes[3] = {0, 0, 0};
unsigned long lastDrainingCheckTimes[3] = {0, 0, 0};
unsigned long lastIdleCheckTimes[3] = {0, 0, 0};
const unsigned long consistencyInterval = 5000; // Time interval for consistency checks
const int distanceThreshold = 3; // Threshold for significant distance change

// Variables for each tank
int tankStates[] = {IDLE, IDLE, IDLE};
int distances[] = {0, 0, 0};

int lastDistances[] = {0, 0, 0};
unsigned long lastUpdateTime = 0;

bool alarmActiveFlags[] = {false, false, false};
bool acknowledgedFlags[] = {false, false, false};

int lastActiveTank = -1; // To track the last tank that triggered the alarm

const int numReadings = 10; // Number of readings to average
int sensorReadings[3][numReadings]; // 2D array to store readings for each sensor
int readIndex[3] = {0, 0, 0}; // Index for current reading for each sensor
int total[3] = {0, 0, 0}; // Total of readings for each sensor
int average[3] = {0, 0, 0}; // Average of readings for each sensor


// Startup delay variables
const unsigned long startupDelay = 5000; // 5 seconds delay for sensor stabilization
unsigned long startupTime;


// Define maximum distance for sensors (in centimeters)
#define MAX_DISTANCE 400

// Declare an array of NewPing objects
NewPing sonar[3] = {
  NewPing(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE),
  NewPing(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE),
  NewPing(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE)
};
void setup() {
  Serial.begin(9600);  // Start serial communication
  pinMode(buttonPin, INPUT);
  pinMode(alarmRelayPin, OUTPUT);
  digitalWrite(alarmRelayPin,HIGH);
  startupTime = millis();

}

void loop() {

   if (millis() - startupTime < startupDelay) {
    Serial.println("Startup delay. Waiting for sensor stabilization...");
    return; // Skip the rest of the loop during startup delay
  }
  for (int i = 0; i < 3; i++) {
    distances[i] = sonar[i].ping_cm();
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= 300000) { 
      for(int j = 0; j<3; j++){
        lastDistances[j] = distances[j];
        }
        lastUpdateTime = currentTime;
    }

    // Update tank states based on sensor readings and consistency checks
    if (tankStates[i] == IDLE) {
      if (checkConsistentFilling(i)) {
        tankStates[i] = FILLING;
      } else if (checkConsistentDraining(i)) {
        tankStates[i] = DRAINING;
      }
    } else if (tankStates[i] == FILLING) {
      if (distances[i] <= tankMaximums[i]) { 
        alarmActiveFlags[i] = true;
        lastActiveTank = i;
      } else if (checkConsistentIdle(i)) {
        tankStates[i] = IDLE;
      }
    } else if (tankStates[i] == DRAINING) {
      if (distances[i] >= tankMinimums[i]){
        alarmActiveFlags[i] = true;
        lastActiveTank = i;
      } else if (checkConsistentIdle(i)) {
        tankStates[i] = IDLE;
      }

    }
    delay(100);
  }




  // Manage alarm activation and user acknowledgment
  if (anyAlarmActive()) {
    activateAlarm();
    Serial.print("Alarm triggered by Tank ");
    Serial.println(lastActiveTank);
  }  
  // delay(500);  // Adjust the delay duration as needed
  if (acknowledgedFlags[lastActiveTank]) {
    deactivateAlarm();
    tankStates[lastActiveTank] = IDLE;
    alarmActiveFlags[lastActiveTank] = false;
    acknowledgedFlags[lastActiveTank] = false;
  }

  // Check for button press (with debouncing)
  if (digitalRead(buttonPin) == HIGH) {
    delay(debounceDelay);
    if (digitalRead(buttonPin) == HIGH) {

      Serial.println("Button press acknowledged. Resetting alarm flags.");
       for (int i = 0; i < 3; i++) {
        acknowledgedFlags[i] = true; // Acknowledge for all tanks
        alarmActiveFlags[i] = false; // Reset alarm flags for all tanks
        tankStates[i] = IDLE; // Set all tanks to IDLE state
        lastDistances[i]=distances[i];
      }
      lastActiveTank = -1;  // Reset lastActiveTank to a neutral value
      deactivateAlarm();

    }
  }
  Serial.print("Distances: ");
  Serial.print(distances[0]);
  Serial.print(" , ");
  Serial.print(distances[1]);
  Serial.print(" , ");
  Serial.print(distances[2]);
  Serial.println(" , ");

    Serial.print("Last Distances: ");
  Serial.print(lastDistances[0]);
  Serial.print(" , ");
  Serial.print(lastDistances[1]);
  Serial.print(" , ");
  Serial.print(lastDistances[2]);
  Serial.println(" , ");

  Serial.print("Tank States: ");
  Serial.print(tankStates[0]);
  Serial.print(" , ");
  Serial.print(tankStates[1]);
  Serial.print(" , ");
  Serial.print(tankStates[2]);
  Serial.println(" , ");
  

}


// Helper functions
bool anyAlarmActive() {
  for (int i = 0; i < 3; i++) {
    if (lastActiveTank != -1 && alarmActiveFlags[i]&& !acknowledgedFlags[i]) {
      return true;
    }
  }
  return false;
}
bool checkConsistentFilling(int tankIndex) {
  unsigned long currentTime = millis();
  if (currentTime - lastFillingCheckTimes[tankIndex] >= consistencyInterval) {
    int currentDistance =  distances[tankIndex];
    int previousDistance = lastDistances[tankIndex];

    if (currentDistance < previousDistance - distanceThreshold) {  // Consistent filling
      lastFillingCheckTimes[tankIndex] = currentTime;
      return true;
    } else {
      // Filling not consistent, reset idle timer
      lastIdleCheckTimes[tankIndex] = currentTime;
    }
  }
  return false;
}

bool checkConsistentDraining(int tankIndex) {
  unsigned long currentTime = millis();
  if (currentTime - lastDrainingCheckTimes[tankIndex] >= consistencyInterval) {
    int currentDistance = distances[tankIndex];
    int previousDistance = lastDistances[tankIndex];
    if (currentDistance > previousDistance + distanceThreshold) {  // Consistent draining
      lastDrainingCheckTimes[tankIndex] = currentTime;
      return true;
    } else {
      // Draining not consistent, reset idle timer
      lastIdleCheckTimes[tankIndex] = currentTime;
    }
  }
  return false;
}

bool checkConsistentIdle(int tankIndex) {
  unsigned long currentTime = millis();
  if (currentTime - lastIdleCheckTimes[tankIndex] >= consistencyInterval) {
    int currentDistance = distances[tankIndex];
    int previousDistance = lastDistances[tankIndex];
    if (abs(currentDistance - previousDistance) <= distanceThreshold) {  // Idle
      return true;
    } else {
      // Not idle, reset corresponding timer
      if (currentDistance < previousDistance) {
        lastFillingCheckTimes[tankIndex] = currentTime;
      } else {
        lastDrainingCheckTimes[tankIndex] = currentTime;
      }
    }
  }
  return false;
}
void activateAlarm(){
  digitalWrite(alarmRelayPin,LOW);
  Serial.println("Alarm ON");
}

void deactivateAlarm(){
  digitalWrite(alarmRelayPin,HIGH);
  acknowledgedFlags[lastActiveTank] = true;
  Serial.println("Alarm OFF");
}




