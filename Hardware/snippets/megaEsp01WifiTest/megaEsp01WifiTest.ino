#include <ArduinoJson.h> // Include the ArduinoJson library for easy JSON creation
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
void setup() {
  // Initialize serial communication with PC and ESP01
  Serial.begin(115200);
  Serial1.begin(115200);
  while (!Serial) continue; // Wait for Serial to be ready
  Serial.println("Serial is ready");

  // Connect to WiFi (Assuming you've already set up this part)

  // Prepare the JSON data
  StaticJsonDocument<200> jsonDoc; // Adjust size based on your data needs
  jsonDoc["A"] = 75; // Example: setting tank 'A' to 75% full
  jsonDoc["B"] = 60;
  jsonDoc["C"] = 25;

  // Convert JSON object to String
  String jsonData;
  serializeJson(jsonDoc, jsonData);

  // Prepare the HTTP POST request
String httpRequest = String("POST /update HTTP/1.1\r\n") +
                     "Host: 192.168.0.5:5001\r\n" + // Adjusted IP address and port
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + jsonData.length() + "\r\n\r\n" +
                     jsonData + "\r\n";

// Send the HTTP POST request to the Flask app
sendCommand("AT+CIPSTART=\"TCP\",\"192.168.0.5\",5001", 5000); // Start a TCP connection to 192.168.0.5:5001
sendCommand("AT+CIPSEND=" + String(httpRequest.length()), 5000); // Indicate the number of bytes to send
sendCommand(httpRequest, 5000); // Send the actual HTTP request
sendCommand("AT+CIPCLOSE", 5000); // Close the TCP connection

}

void loop() {
  // If there's data from the ESP01, print it to the Serial Monitor
  if (Serial1.available()) {
    Serial.write(Serial1.read());
  }
}


