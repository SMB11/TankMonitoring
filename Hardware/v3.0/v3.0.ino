#include <NewPing.h>  // Include the NewPing library
#include <ArduinoJson.h>


// Define pins for each sensor
#define TRIGGER_PIN_1 22
#define ECHO_PIN_1 23
#define TRIGGER_PIN_2 24
#define ECHO_PIN_2 25
#define TRIGGER_PIN_3 26
#define ECHO_PIN_3 27


#define buttonPin 2
#define alarmRelayPin 4


// Constants
const int IDLE = 0;
const int FILLING = 1;
const int DRAINING = 2;
const int ALARM = 3;


const int debounceDelay = 50;   // Delay for button debouncing

// int tankHeights[] = {210, 300, 150};

int tankMaximums[] = { 53, 53, 20 };
int tankMinimums[] = { 210, 195, 170 };

// Variables for consistency checks
unsigned long lastFillingCheckTimes[3] = { 0, 0, 0 };
unsigned long lastDrainingCheckTimes[3] = { 0, 0, 0 };
unsigned long lastIdleCheckTimes[3] = { 0, 0, 0 };
const unsigned long consistencyInterval = 20000;  // Time interval for consistency checks
const int distanceThreshold = 5;                 // Threshold for significant distance change


// Variables for each tank
int tankStates[] = { IDLE, IDLE, IDLE };
int distances[] = { 0, 0, 0 };

int lastDistances[] = { 0, 0, 0 };
unsigned long lastUpdateTime = 0;

bool alarmActiveFlags[] = { false, false, false };
bool acknowledgedFlags[] = { false, false, false };


const String flaskAppHost = "192.168.0.7"; // Replace with your Flask app's hostname or IP
const int flaskAppPort = 5001; // or 5000 if running Flask on its default port
unsigned long lastSendTime = 0;
const long interval = 30000; // 30 seconds



int lastActiveTank = -1;  // To track the last tank that triggered the alarm

const int numReadings = 10;          // Number of readings to average
int sensorReadings[3][numReadings];  // 2D array to store readings for each sensor
int readIndex[3] = { 0, 0, 0 };      // Index for current reading for each sensor
int total[3] = { 0, 0, 0 };          // Total of readings for each sensor
int average[3] = { 0, 0, 0 };        // Average of readings for each sensor


int fillingCounter[3] = {0, 0, 0};
int drainingCounter[3] = {0, 0, 0};
const int requiredConsistentReadings = 3;  // Number of consistent readings required


int previousState[]={ IDLE, IDLE, IDLE };

int sensorReadingsCount[3] = {0, 0, 0};
const int readingsToIgnore = 50;  // Number of readings to ignore for stabilization

const unsigned long stateChangeDelay = 300000; // 5 minutes in milliseconds



unsigned long lastAlarmAcknowledgmentTime = 0; // Tracks the last time an alarm was acknowledged
const unsigned long alarmTriggerDelay = 300000; // 60,000 milliseconds = 1 minute delay before allowing a new alarm



// Define maximum distance for sensors (in centimeters)
#define MAX_DISTANCE 400

// Declare an array of NewPing objects
NewPing sonar[3] = {
  NewPing(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE),
  NewPing(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE),
  NewPing(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE)
};


const int numMedianReadings = 7; // Number of median readings to check for consistency
int medianReadings[3][numMedianReadings]; // 2D array to store median readings for each sensor
int medianReadIndex[3] = {0, 0, 0}; // Current index for storing median readings




void setup() {

  Serial.begin(115200);  // Start serial communication
  pinMode(buttonPin, INPUT);
  pinMode(alarmRelayPin, OUTPUT);
  digitalWrite(alarmRelayPin, HIGH);
  Serial1.begin(115200); // Adjust the baud rate as necessary for ESP01
  Serial.println("Initializing...");

  setupWiFi(); // Attempt to set up WiFi connection
// Extended warm-up time
  delay(5000); // Consider extending if needed

  for (int i = 0; i < 3; i++) {
    bool validReadingObtained = false;
    int validReadingsCount = 0;
    const int requiredValidReadings = 10; // Number of consecutive valid readings to consider sensor stabilized
    while (!validReadingObtained) {
      int reading = sonar[i].ping_cm();
      // Check if reading is within a reasonable range
      if (reading > 0 && reading < MAX_DISTANCE) {
        sensorReadings[i][validReadingsCount] = reading;
        validReadingsCount++;
        if (validReadingsCount >= requiredValidReadings) {
          validReadingObtained = true;
          // Calculate initial average from valid readings
          int sum = 0;
          for (int j = 0; j < requiredValidReadings; j++) {
            sum += sensorReadings[i][j];
          }
          lastDistances[i] = sum / requiredValidReadings;
        }
      } else {
        // Reset if an invalid reading is encountered
        validReadingsCount = 0;
      }
      delay(100); // Short delay between readings to not overwhelm the sensor
    }
  }

  // startupTime = millis();
}

void loop() {

   unsigned long currentMillis = millis();

  for (int i = 0; i < 3; i++) {

   // Subtract the last reading
    // total[i] -= sensorReadings[i][readIndex[i]];

    // Read from the sensor
    int newReading = sonar[i].ping_cm();
    if (newReading <= 0 || newReading > MAX_DISTANCE) continue; // Validate reading

    // Store the reading in a circular buffer
    sensorReadings[i][readIndex[i]] = newReading;
    readIndex[i] = (readIndex[i] + 1) % numReadings; // numReadings might be smaller for median calculation, like 5 or 10

    // Calculate median from sensorReadings[i] and use it as the current distance
    int sortedReadings[numReadings]; // Temporary array to store values for sorting
    memcpy(sortedReadings, sensorReadings[i], sizeof(sortedReadings));
    int medianDistance = calculateMedian(sortedReadings, numReadings);

    // Use 'medianDistance' instead of 'newReading' for your logic below


    medianReadings[i][medianReadIndex[i]] = medianDistance;
    medianReadIndex[i] = (medianReadIndex[i] + 1) % numMedianReadings;

    // After storing the new medianDistance, check for consistency
    bool isConsistent = areMedianReadingsConsistent(i);
    if (isConsistent) {
    // Check if it's Tank 3 and the distance is greater than 175
        if (i == 2 && medianDistance > 180) {
            distances[i] = 180; // Set distance to 180 if greater than 175
        } else {
            distances[i] = medianDistance; // Update distances[i] with the median distance
        }
    }

    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= 360000) {
      for (int j = 0; j < 3; j++) {
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
        if (currentTime > lastAlarmAcknowledgmentTime + alarmTriggerDelay) {
        alarmActiveFlags[i] = true;
        lastActiveTank = i;
        }
      } else if (checkConsistentIdle(i)) {
        tankStates[i] = IDLE;
      }
    } else if (tankStates[i] == DRAINING) {
      if (distances[i] >= tankMinimums[i]) {
        if (currentTime > lastAlarmAcknowledgmentTime + alarmTriggerDelay) {
        alarmActiveFlags[i] = true;
        lastActiveTank = i;
        }
      } else if (checkConsistentIdle(i)) {
        tankStates[i] = IDLE;
      }
    }
    // Inside loop, after updating tank states
    if (tankStates[i] != previousState[i]) {
        fillingCounter[i] = 0;
        drainingCounter[i] = 0;
    }
    delay(50);
  }


  // Manage alarm activation and user acknowledgment
  if (anyAlarmActive()) {
    activateAlarm();
    Serial.print("Alarm triggered by Tank ");
    Serial.println(lastActiveTank);
  }
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
      lastAlarmAcknowledgmentTime = millis(); // Update the acknowledgment time
      Serial.println("Button press acknowledged. Resetting alarm flags.");
      for (int i = 0; i < 3; i++) {
        acknowledgedFlags[i] = true;  // Acknowledge for all tanks
        alarmActiveFlags[i] = false;  // Reset alarm flags for all tanks
        tankStates[i] = IDLE;         // Set all tanks to IDLE state
        lastDistances[i] = distances[i];
      }
      lastActiveTank = -1;  // Reset lastActiveTank to a neutral value
      deactivateAlarm();
    }
  }
  Serial.print("CUR: ");
  Serial.print(distances[0]);
  Serial.print(" , ");
  Serial.print(distances[1]);
  Serial.print(" , ");
  Serial.print(distances[2]);
  Serial.println(" , ");

  Serial.print("LST: ");
  Serial.print(lastDistances[0]);
  Serial.print(" , ");
  Serial.print(lastDistances[1]);
  Serial.print(" , ");
  Serial.print(lastDistances[2]);
  Serial.println(" , ");

  Serial.print("STT: ");
  Serial.print(tankStates[0]);
  Serial.print(" , ");
  Serial.print(tankStates[1]);
  Serial.print(" , ");
  Serial.print(tankStates[2]);
  Serial.println(" , ");

  if (currentMillis - lastSendTime >= interval) {
    lastSendTime = currentMillis;
    sendData(distances); // distances is your array of distance values
  }

}


// Helper functions
bool anyAlarmActive() {
  for (int i = 0; i < 3; i++) {
    if (lastActiveTank != -1 && alarmActiveFlags[i] && !acknowledgedFlags[i]) {
      return true;
    }
  }
  return false;
}
bool checkConsistentFilling(int tankIndex) {
  unsigned long currentTime = millis();
  if (currentTime - lastFillingCheckTimes[tankIndex] >= consistencyInterval) {
    int currentDistance = distances[tankIndex];
    int previousDistance = lastDistances[tankIndex];

    if (currentDistance <= previousDistance - distanceThreshold) {
      fillingCounter[tankIndex]++;
      if (fillingCounter[tankIndex] >= requiredConsistentReadings) {
          fillingCounter[tankIndex] = 0;
          lastFillingCheckTimes[tankIndex] = currentTime;
          return true;
      }
    } else {
      fillingCounter[tankIndex] = 0;
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
    if (currentDistance >= previousDistance + distanceThreshold) {
      drainingCounter[tankIndex]++;
      if (drainingCounter[tankIndex] >= requiredConsistentReadings) {
          drainingCounter[tankIndex] = 0;
          lastDrainingCheckTimes[tankIndex] = currentTime;
          return true;
      }
    } else {
      drainingCounter[tankIndex] = 0;
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

bool isOutlier(int newReading, int averageReading) {
    int threshold = 10;  // Set a threshold for what you consider an outlier
    return abs(newReading - averageReading) > threshold;
}


void activateAlarm() {
  digitalWrite(alarmRelayPin, LOW);
  Serial.println("Alarm ON");
}

void deactivateAlarm() {
  digitalWrite(alarmRelayPin, HIGH);
  acknowledgedFlags[lastActiveTank] = true;
  Serial.println("Alarm OFF");
}


// Comparison function for qsort
int compare(const void * a, const void * b) {
  return (*(int*)a - *(int*)b);
}

// Updated calculateMedian function using qsort
int calculateMedian(int arr[], int n) {
    qsort(arr, n, sizeof(int), compare); // Use qsort to sort the array
    if (n % 2 != 0) // If odd number of elements
        return arr[n / 2];
    return (arr[(n - 1) / 2] + arr[n / 2]) / 2; // If even number of elements, calculate average of middle two
}


bool areMedianReadingsConsistent(int sensorIndex) {
    for (int i = 0; i < numMedianReadings - 1; i++) {
        if (abs(medianReadings[sensorIndex][i] - medianReadings[sensorIndex][i + 1]) > 10) {
            return false; // Readings differ by more than 5, not consistent
        }
    }
    return true; // All readings are consistent
}



int calculateWaterLevelPercentage(int distance, int tankIndex) {
  // Ensure tankIndex is within the bounds of the arrays
  if (tankIndex < 0 || tankIndex >= sizeof(tankMinimums) / sizeof(tankMinimums[0])) {
    return -1; // Return an error value if tankIndex is out of bounds
  }
  
  // Calculate the inverted distance to represent the water level
  // This calculation assumes that a lower distance indicates a higher water level
  int level = 100 * (tankMinimums[tankIndex] - distance) / (tankMinimums[tankIndex] - tankMaximums[tankIndex]);

  // Constrain the result to ensure it is within 0 to 100%
  int percentage = constrain(level, 0, 100);

  return percentage;
}
bool readESP01Response(long timeout = 3000) {
  long int time = millis();
  bool responseReceived = false;
  while ((time + timeout) > millis()) {
    while (Serial1.available()) {
      char c = Serial1.read();
      Serial.write(c); // Print the ESP01's response to the Serial Monitor
      responseReceived = true;
    }
  }
  return responseReceived; // Return true if any response was received
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






void sendData(int distances[]) {
  // Determine the size of the JSON document
  // Use ArduinoJson's Assistant to compute the capacity: https://arduinojson.org/v6/assistant/
  const size_t capacity = JSON_OBJECT_SIZE(3) + 60;
  StaticJsonDocument<capacity> doc;

  // Set the values
  doc["A"] = distances[0];
  doc["B"] = distances[1];
  doc["C"] = distances[2];

  // Generate the JSON string
  String payload;
  serializeJson(doc, payload);

  // Calculate the length of the request
  int contentLength = payload.length();

  // Start a TCP connection
  Serial1.println("AT+CIPSTART=\"TCP\",\"" + flaskAppHost + "\"," + flaskAppPort);
  if (readESP01Response(5000)) {
    Serial.println("Connected to the server...");
  } else {
    Serial.println("Connection to server failed.");
    return;
  }

  // Prepare the HTTP POST request
  String httpRequest = "POST /update HTTP/1.1\r\n";
  httpRequest += "Host: " + flaskAppHost + "\r\n";
  httpRequest += "Content-Type: application/json\r\n";
  httpRequest += "Content-Length: " + String(contentLength) + "\r\n";
  httpRequest += "Connection: close\r\n\r\n";
  httpRequest += payload + "\r\n";

  // Send data length for the AT+CIPSEND command
  Serial1.println("AT+CIPSEND=" + String(httpRequest.length()));
  if (readESP01Response(5000)) {
    // Send the actual HTTP request
    Serial1.print(httpRequest);
    if (readESP01Response(10000)) {
      Serial.println("Data sent successfully.");
    } else {
      Serial.println("Failed to send data.");
    }
  } else {
    Serial.println("CIPSEND command failed.");
  }

  // Close the TCP connection
  Serial1.println("AT+CIPCLOSE");
}
