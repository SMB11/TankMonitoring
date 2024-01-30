#include <Arduino.h>

// Define sensor pins
const int triggerPin1 = 5; // Adjust these pin numbers according to your ESP32 setup
const int echoPin1 = 18;
const int triggerPin2 = 19;
const int echoPin2 = 21;
const int triggerPin3 = 22;
const int echoPin3 = 23;

// Define constants
const float speedOfSound = 0.0343; // Speed of sound in cm/us
const int maxDistance = 450; // Maximum distance in cm
const int minDistance = 25; // Minimum distance in cm

// Function to read distance from a sensor
float readDistance(int triggerPin, int echoPin) {
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);
    long duration = pulseIn(echoPin, HIGH);
    
    if (duration == 0) { // Timeout or no echo
        return -1.0; // Error value
    }
    
    float distance = duration * speedOfSound / 2;
    
    if (distance > maxDistance || distance < minDistance) { // Out of range
        return -2.0; // Error value
    }
    
    return distance;
}

void setup() {
    Serial.begin(9600);
    pinMode(triggerPin1, OUTPUT);
    pinMode(echoPin1, INPUT);
    pinMode(triggerPin2, OUTPUT);
    pinMode(echoPin2, INPUT);
    pinMode(triggerPin3, OUTPUT);
    pinMode(echoPin3, INPUT);
}

void loop() {
    float distance1 = readDistance(triggerPin1, echoPin1);
    float distance2 = readDistance(triggerPin2, echoPin2);
    float distance3 = readDistance(triggerPin3, echoPin3);

    Serial.print("Tank 1: ");
    if (distance1 < 0) {
        Serial.println("Error reading sensor 1");
    } else {
        Serial.println(distance1);
    }

    Serial.print("Tank 2: ");
    if (distance2 < 0) {
        Serial.println("Error reading sensor 2");
    } else {
        Serial.println(distance2);
    }

    Serial.print("Tank 3: ");
    if (distance3 < 0) {
        Serial.println("Error reading sensor 3");
    } else {
        Serial.println(distance3);
    }

    delay(1000); // Delay between readings
}
