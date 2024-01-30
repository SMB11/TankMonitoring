#include <SoftwareSerial.h>
SoftwareSerial espSerial(2, 3); // RX, TX for ESP-01

const int triggerPin1 = 4;
const int echoPin1 = 5;
const int triggerPin2 = 6;
const int echoPin2 = 7;
const int triggerPin3 = 8;
const int echoPin3 = 9;
const int buttonPin = 10;
const int alarmRelayPin = 11;

const unsigned long minDistance = 10; // Minimum tank distance in cm
const unsigned long maxDistance = 100; // Maximum tank distance in cm

bool alarmActive = false;
bool userAcknowledged1 = false; // User acknowledgment for tank 1
bool userAcknowledged2 = false; // User acknowledgment for tank 2
bool userAcknowledged3 = false; // User acknowledgment for tank 3

void setup() {
  pinMode(triggerPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(triggerPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(triggerPin3, OUTPUT);
  pinMode(echoPin3, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(alarmRelayPin, OUTPUT);
  
  Serial.begin(9600);
  espSerial.begin(9600); // Change baud rate as needed

  // Initialize ESP-01 and connect to Wi-Fi
  initializeESP();

  // Turn off the alarm relay initially
  digitalWrite(alarmRelayPin, LOW);
}

void loop() {
  // Read tank levels
  unsigned long tank1Level = readUltrasonicSensor(triggerPin1, echoPin1);
  unsigned long tank2Level = readUltrasonicSensor(triggerPin2, echoPin2);
  unsigned long tank3Level = readUltrasonicSensor(triggerPin3, echoPin3);

  // Check tank levels and activate the alarm if needed
  checkTankLevels(tank1Level, tank2Level, tank3Level);

  // Monitor the push button for each tank
  if (digitalRead(buttonPin) == LOW) {
    if (!userAcknowledged1) {
      userAcknowledged1 = true; // User has acknowledged the alarm for tank 1
    } else if (!userAcknowledged2) {
      userAcknowledged2 = true; // User has acknowledged the alarm for tank 2
    } else if (!userAcknowledged3) {
      userAcknowledged3 = true; // User has acknowledged the alarm for tank 3
    }
    alarmActive = false; // Turn off the alarm when the button is pressed
    digitalWrite(alarmRelayPin, LOW);
  }

  // Send tank levels and alarm status to a remote server (optional)
  sendDataToServer(tank1Level, tank2Level, tank3Level, alarmActive, userAcknowledged1, userAcknowledged2, userAcknowledged3);
  
  delay(1000); // Delay to avoid flooding the serial monitor
}

unsigned long readUltrasonicSensor(int triggerPin, int echoPin) {
  // Measure the distance from the ultrasonic sensor
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  unsigned long duration = pulseIn(echoPin, HIGH);
  return (duration / 2) / 29.1; // Convert to centimeters
}

void checkTankLevels(unsigned long level1, unsigned long level2, unsigned long level3) {
  if ((!userAcknowledged1 && (level1 < minDistance || level1 > maxDistance)) ||
      (!userAcknowledged2 && (level2 < minDistance || level2 > maxDistance)) ||
      (!userAcknowledged3 && (level3 < minDistance || level3 > maxDistance))) {
    alarmActive = true;
    digitalWrite(alarmRelayPin, HIGH);
  }
}

void initializeESP() {
  espSerial.println("AT+RST");
  delay(1000);
  espSerial.println("AT+CWMODE=1"); // Set ESP to Station mode
  delay(1000);
  espSerial.println("AT+CWJAP=\"YourSSID\",\"YourPassword\""); // Replace with your Wi-Fi credentials
  delay(5000);
}

void sendDataToServer(unsigned long tank1, unsigned long tank2, unsigned long tank3, bool alarmStatus, bool ack1, bool ack2, bool ack3) {
  // You can implement code here to send data to a remote server using ESP-01.
  // This part depends on your specific server and communication protocol.
}
