#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>

const char* ssid = "TPLINKMESH";
const char* password = "Admin12345678*";
const char* serverUrl = "http://qrgobox.com/from_esp8266/to_esp8266.php";

#define Finger_Rx 0  //D3
#define Finger_Tx 2  //D4

String branch_id = "stical";

int fingerprintLED = D5;

int lockPin = D6;

int FingerID = 0;
uint8_t id;

SoftwareSerial mySerial(Finger_Rx, Finger_Tx);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup() {
  Serial.begin(115200);

  pinMode(fingerprintLED, OUTPUT);
  pinMode(lockPin, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

unsigned long previousMillis = 0;
const long interval = 1000;  // Interval to poll the server (in milliseconds)

void loop() {
  // Send HTTP GET request to the server
  WiFiClient client;
  HTTPClient http;
  http.begin(client, String(serverUrl) + "?request_status");  // Request both lock and vacancy status

  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.print("Received JSON payload: ");
    Serial.println(payload);

    // Parse JSON response
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    // Check if the response is an array and not empty
    if (doc.is<JsonArray>() && doc.size() > 0) {
      // Get the first element of the array
      JsonObject obj = doc[1];

      // Extract id, vacancy status, lock status, and register fingerprint status from the first element
      int id = obj["id"];
      String goboxNo = obj["gobox_no"];
      int lockStatus = obj["lock_status"];
      int vacancyStatus = obj["vacancy_status"];
      String currentOwner = obj["current_owner"];
      String phoneNumber = obj["phone_number"];
      int fingerprintStatus = obj["fingerprint_status"];
      int fingerprintSensor = obj["fingerprint_sensor"];
      int fingerprintID = obj["fingerprint_id"];
      int admin_fingerprintSensor = obj["admin_fingerprint_sensor"];
      int admin_fingerprintID = obj["admin_fingerprint_id"];
      int unlockAll = obj["unlock_all"];

      const int MAX_FINGERPRINTS = 127;  // Assuming a maximum of 10 fingerprints

      Serial.print("ID: ");
      Serial.println(id);
      Serial.print("GoBox: ");
      Serial.println(goboxNo);
      Serial.print("Lock status: ");
      Serial.println(lockStatus);
      Serial.print("Vacancy status: ");
      Serial.println(vacancyStatus);
      Serial.print("Current owner: ");
      Serial.println(currentOwner);
      Serial.print("Phone number: ");
      Serial.println(phoneNumber);
      Serial.print("Fingerprint status: ");
      Serial.println(fingerprintStatus);
      Serial.print("Fingerprint sensor: ");
      Serial.println(fingerprintSensor);
      Serial.print("Fingerprint ID: ");
      Serial.println(fingerprintID);
      Serial.print("Admin fingerprint sensor: ");
      Serial.println(admin_fingerprintSensor);
      Serial.print("Admin fingerprint ID: ");
      Serial.println(admin_fingerprintID);
      Serial.print("Unlock All: ");
      Serial.println(unlockAll);

      Serial.println("---------------------------------------------------------------------------");

      if (vacancyStatus == 1 && fingerprintStatus == 0 && fingerprintSensor == 0) {
        digitalWrite(fingerprintLED, LOW);
        verifyFingerprint(id, fingerprintID, currentOwner, phoneNumber);
      } else if (fingerprintStatus == 0 && fingerprintSensor == 1) {
        digitalWrite(fingerprintLED, HIGH);
        enrollFingerprint(id, fingerprintID, goboxNo, currentOwner);
      } else if (fingerprintStatus == 1 && fingerprintSensor == 1) {
        digitalWrite(fingerprintLED, LOW);
        verifyFingerprint(id, fingerprintID, currentOwner, phoneNumber);
      }
      if (admin_fingerprintSensor == 1) {
        digitalWrite(fingerprintLED, HIGH);
        enrollAdminFingerprint(id, admin_fingerprintID, goboxNo, currentOwner);
      }
    } else {
      Serial.println("Empty or invalid JSON array");
    }
  } else {
    Serial.println("Error on HTTP request");
  }
  http.end();

  delay(1500);  // Poll server every 1.5 second
}

void enrollAdminFingerprint(int id, int fingerprintID, String goboxNo, String currentOwner) {
  // Initialize fingerprint sensor
  finger.begin(57600);

  // Wait for fingerprint sensor to be ready
  while (!finger.verifyPassword()) {
    delay(100);
  }

  Serial.println("ADMIN FINGERPRINT ENROLLMENT");
  Serial.println("Place your finger on the sensor...");

  // Get the current time
  long startTime = millis();

  // Wait for a finger to be detected or until timeout
  while (finger.getImage() != FINGERPRINT_OK) {
    // Check if 5 seconds have elapsed
    if (millis() - startTime > 5000) {
      Serial.println("Timeout: No finger detected within 5 seconds");
      Serial.println("---------------------------------------------------------------------------");
      digitalWrite(fingerprintLED, LOW);  // Turn off the LED
      return;
    }
  }

  // Convert the fingerprint image to a template
  if (finger.image2Tz(1) != FINGERPRINT_OK) {
    Serial.println("Failed to convert fingerprint image to template");
    Serial.println("---------------------------------------------------------------------------");
    return;
  }

  // Store the fingerprint template to the specified ID
  if (finger.storeModel(fingerprintID) != FINGERPRINT_OK) {
    Serial.println("Failed to store fingerprint template");
    Serial.println("---------------------------------------------------------------------------");
    return;
  }

  // Turn off the LED after successful enrollment
  digitalWrite(fingerprintLED, LOW);

  Serial.println("Fingerprint enrolled successfully!");

  // Send data to database
  WiFiClient client;
  HTTPClient http;

  // Prepare the data to send
  String data = "fingerprint_enroll=2&id=" + String(id) + "&gobox_no=" + goboxNo + "&current_owner=" + currentOwner + "&fingerprint_id=" + String(fingerprintID);

  // Send POST request to the database
  http.begin(client, "http://qrgobox.com/from_esp8266/enroll_fingerprint.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(data);

  // Check if request was successful
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Data sent to database successfully");
    Serial.println("---------------------------------------------------------------------------");
  } else {
    Serial.print("Error sending data to database. HTTP error code: ");
    Serial.println(httpCode);
    Serial.println("---------------------------------------------------------------------------");
  }

  // End HTTP connection
  http.end();
}

void enrollFingerprint(int id, int fingerprintID, String goboxNo, String currentOwner) {
  // Initialize fingerprint sensor
  finger.begin(57600);

  // Delete the fingerprint from the sensor before proceeding
  finger.deleteModel(fingerprintID);

  // Wait for fingerprint sensor to be ready
  while (!finger.verifyPassword()) {
    delay(100);
  }

  Serial.println("Place your finger on the sensor...");

  // Turn on the LED
  digitalWrite(fingerprintLED, HIGH);

  // Get the current time
  long startTime = millis();

  // Wait for a finger to be detected or until timeout
  while (finger.getImage() != FINGERPRINT_OK) {
    // Check if 5 seconds have elapsed
    if (millis() - startTime > 5000) {
      Serial.println("Timeout: No finger detected within 5 seconds");
      Serial.println("---------------------------------------------------------------------------");
      digitalWrite(fingerprintLED, LOW);  // Turn off the LED
      return;
    }
  }

  // Convert the fingerprint image to a template
  if (finger.image2Tz(1) != FINGERPRINT_OK) {
    Serial.println("Failed to convert fingerprint image to template");
    Serial.println("---------------------------------------------------------------------------");
    return;
  }

  // Store the fingerprint template to the specified ID
  if (finger.storeModel(fingerprintID) != FINGERPRINT_OK) {
    Serial.println("Failed to store fingerprint template");
    Serial.println("---------------------------------------------------------------------------");
    return;
  }

  // Turn off the LED after successful enrollment
  digitalWrite(fingerprintLED, LOW);

  Serial.println("Fingerprint enrolled successfully!");

  // Send data to database
  WiFiClient client;
  HTTPClient http;

  // Prepare the data to send
  String data = "fingerprint_enroll=1&id=" + String(id) + "&gobox_no=" + goboxNo + "&current_owner=" + currentOwner + "&fingerprint_id=" + String(fingerprintID);

  // Send POST request to the database
  http.begin(client, "http://qrgobox.com/from_esp8266/enroll_fingerprint.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(data);

  // Check if request was successful
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Data sent to database successfully");
    Serial.println("---------------------------------------------------------------------------");
  } else {
    Serial.print("Error sending data to database. HTTP error code: ");
    Serial.println(httpCode);
    Serial.println("---------------------------------------------------------------------------");
  }

  // End HTTP connection
  http.end();
}

void verifyFingerprint(int id, int fingerprintID, String currentOwner, String phoneNumber) {
  // Initialize fingerprint sensor
  finger.begin(57600);

  // Wait for fingerprint sensor to be ready
  while (!finger.verifyPassword()) {
    delay(100);
  }

  Serial.println("Place your finger on the sensor...");

  // Wait for a finger to be detected
  long startTime = millis();  // Get the current time
  while (finger.getImage() != FINGERPRINT_OK) {
    // Check if 5 seconds have elapsed
    if (millis() - startTime > 5000) {
      Serial.println("Timeout: No finger detected within 5 seconds");
      return;
    }
  }

  // Convert the fingerprint image to a template
  if (finger.image2Tz(1) != FINGERPRINT_OK) {
    Serial.println("Failed to convert fingerprint image to template");
    return;
  }

  // Search for a matching fingerprint in the database
  uint8_t searchResult = finger.fingerSearch();

  // Check if a matching fingerprint is found
  if (searchResult == FINGERPRINT_OK) {
    // Get the ID of the found fingerprint
    int foundID = finger.fingerID;

    // Check if the found fingerprint matches the current fingerprint ID
    if (foundID == fingerprintID) {
      Serial.println("Fingerprint matched!");

      // Send data to database for unlocking
      WiFiClient client;
      HTTPClient http;

      // Send data to unlock
      String unlockData = "fingerprint_unlock=1&id=" + String(id) + "&lock_status=1";

      // Send POST request to unlock the GoBox
      http.begin(client, "http://qrgobox.com/from_esp8266/unlock_fingerprint.php");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int unlockHttpCode = http.POST(unlockData);

      // Check if request was successful
      if (unlockHttpCode == HTTP_CODE_OK) {
        Serial.println("GoBox has been unlocked for 10 seconds.");

        // Print countdown timer
        for (int i = 10; i > 0; i--) {
          Serial.print("Timer: ");
          Serial.print(i);
          Serial.println(" seconds remaining...");
          delay(1000);  // 1-second delay
        }

        // Send data to unpair
        String unpairData = "fingerprint_unpair=1&id=" + String(id) + "&current_owner=" + currentOwner + "&fingerprint_id=" + String(fingerprintID);

        // Send POST request to unpair the fingerprint
        http.begin(client, "http://qrgobox.com/from_esp8266/unpair_fingerprint.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        int unpairHttpCode = http.POST(unpairData);

        // Check if request was successful
        if (unpairHttpCode == HTTP_CODE_OK) {
          Serial.println("Unpaired with the GoBox.");
          Serial.println("---------------------------------------------------------------------------");
        } else {
          Serial.print("Error sending unpair data to database. HTTP error code: ");
          Serial.println(unpairHttpCode);
          Serial.println("---------------------------------------------------------------------------");
        }
      } else {
        Serial.print("Error sending unlock data to database. HTTP error code: ");
        Serial.println(unlockHttpCode);
        Serial.println("---------------------------------------------------------------------------");
      }
      // End HTTP connection
      http.end();

      // Delete the fingerprint from the sensor
      finger.deleteModel(fingerprintID);
    } else {
      adminUnlock(id, currentOwner, phoneNumber);
    }
  } else if (searchResult == FINGERPRINT_NOTFOUND) {
    Serial.println("INVALID_FINGERPRINT: " + String(id) + ", " + String(currentOwner) + ", " + String(phoneNumber));
    Serial.println("---------------------------------------------------------------------------");

    digitalWrite(fingerprintLED, HIGH);
    delay(250);
    digitalWrite(fingerprintLED, LOW);
    delay(250);
    digitalWrite(fingerprintLED, HIGH);
    delay(250);
    digitalWrite(fingerprintLED, LOW);
    delay(250);
  } else {
    Serial.println("Unknown error during fingerprint search");
  }
}

void adminUnlock(int id, String currentOwner, String phoneNumber) {
  // Send data to database for unlocking
  WiFiClient client;
  HTTPClient http;

  // Send data to unlock
  String unlockData = "fingerprint_unlock=1&id=" + String(id) + "&lock_status=1";

  // Send POST request to unlock the GoBox
  http.begin(client, "http://qrgobox.com/from_esp8266/unlock_fingerprint.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int unlockHttpCode = http.POST(unlockData);

  // Check if request was successful
  if (unlockHttpCode == HTTP_CODE_OK) {
    Serial.println("ADMIN_UNLOCK: "  + String(id) + ", " + String(currentOwner) + ", " + String(phoneNumber));

    Serial.println("---------------------------------------------------------------------------");
  } else {
    Serial.print("Error sending unlock data to database. HTTP error code: ");
    Serial.println(unlockHttpCode);
    Serial.println("---------------------------------------------------------------------------");
  }
  // End HTTP connection
  http.end();
}