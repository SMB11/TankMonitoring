const int triggerPin1 = 22;
const int echoPin1 = 23;
const int triggerPin2 = 24;
const int echoPin2 = 25;
const int triggerPin3 = 26;
const int echoPin3 = 27;
const int buttonPin = 2;
const int alarmSirenPin = 3;
const int alarmLightPin = 4;


void setup(){

  Serial.begin(115200);


  pinMode(triggerPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(triggerPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(triggerPin3, OUTPUT);
  pinMode(echoPin3, INPUT);
  pinMode(buttonPin, INPUT);
  pinMode(alarmSirenPin, OUTPUT);
  pinMode(alarmLightPin, OUTPUT);


  digitalWrite(alarmSirenPin,HIGH);
  digitalWrite(alarmLightPin,HIGH);


}

void loop(){

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


void activateAlarm() {
  alarmActive = true;
  digitalWrite(alarmPin, HIGH);
}

void deactivateAlarm() {
  alarmActive = false;
  digitalWrite(alarmPin, LOW);
}