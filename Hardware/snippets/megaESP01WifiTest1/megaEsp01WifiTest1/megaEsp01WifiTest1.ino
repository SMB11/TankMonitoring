#include <SoftwareSerial.h>

// SoftwareSerial Serial1(10, 11); // RX, TX

unsigned long lastSendTime = 0;
const long interval = 30000; // 30 seconds

void sendCommand(String command, const int timeout = 10000) {
  Serial1.println(command); // Send the command to the ESP01
  long int time = millis();

  while((time + timeout) > millis()) {
    while(Serial1.available()) {
      // Print the ESP01's response to the Serial Monitor
      Serial.write(Serial1.read());
    }
  }
}

bool checkWiFiConnection() {
  long int time = millis();
  bool isConnected = false; // Assume not connected initially
  while ((time + 3000) > millis()) {
    while (Serial1.available()) {
      String line = Serial1.readStringUntil('\n');
      if (line.indexOf("No AP") != -1) {
        Serial.println("WiFi Not Connected");
        isConnected = false;
        break; // Exit the loop if "No AP" found
      }
      if (line.indexOf("+CWJAP:") != -1) {
        Serial.println("WiFi Connected");
        isConnected = true;
        break; // Exit the loop if connection info found
      }
    }
  }
  return isConnected; // Return the connection status
}



void readESP01Response(long timeout = 3000) {
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (Serial1.available()) {
      char c = Serial1.read();
      Serial.write(c); // Print the ESP01's response to the Serial Monitor
    }
  }
}

void setupWiFi() {
  Serial.println("Setting up WiFi...");

  // Reset the ESP01 module
  Serial1.println("AT+RST");
  delay(2000); // Wait for the module to reset
  Serial.println("Resetting ESP01...");
  readESP01Response(5000); // Read and display the reset response

  // Set ESP01 to Station mode
  Serial.println("Setting ESP01 to Station Mode...");
  Serial1.println("AT+CWMODE=1");
  readESP01Response(); // Read and display the mode set response

  // Connect to WiFi network
  Serial.println("Connecting to WiFi network...");
  Serial1.println("AT+CWJAP=\"Rostelecom_202589\",\"12345678\"");
  readESP01Response(10000); // Give more time for connection to establish and read response

  // Check connection status
  Serial.println("Checking WiFi connection status...");
  Serial1.println("AT+CWJAP?");
  if(checkWiFiConnection()) {
    Serial.println("WiFi is connected successfully.");
  } else {
    Serial.println("Failed to connect to WiFi. Please check credentials and try again.");
  }
}







void setup() {
  Serial.begin(115200);
  Serial1.begin(115200); // Adjust the baud rate as necessary for ESP01
  Serial.println("Initializing...");

  setupWiFi(); // Attempt to set up WiFi connection
}

void loop() {
  // Main loop code
}