#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h> // Hardware-specific library

#include "bitmapsLarge.h"
#include "bitmapsLarge2.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "TPLINKMESH";
const char* password = "Admin12345678*";
const char* serverUrl = "http://qrgobox.com/from_esp8266/to_esp8266.php";

// TFT PINS
#define TFT_CS     D8  // Define the CS pin
#define TFT_RST    D6  // Define the RESET pin
#define TFT_DC     D4  // Define the DC pin

// RGB AND LOCK PINS
const int redPin = D1;   // Red pin of RGB LED
const int greenPin = D2; // Green pin of RGB LED

const int fingerprintPin = D0;

const int lockPin = D3;

// Create an instance of the ILI9341 library
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Define variables to store previous vacancy status and image dimensions
int previousVacancyStatus = -1; // Initialize to an invalid value
int imgHeight = 180; // Define the height of the image

// Array of image pointers for vacant status
const uint16_t* evive_vacant[] = {evive_in_hand01, evive_in_hand02, evive_in_hand03};

// Array of image pointers for occupied status
const uint16_t* evive_occupied[] = {evive_in_hand11, evive_in_hand12, evive_in_hand13};

int currentEviveIndex = 0; // Initialize the index for evive arrays

void setup() {
  Serial.begin(115200);

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);

  pinMode(fingerprintPin, OUTPUT);

  pinMode(lockPin, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  tft.begin(); // Initialize the display

  // Optionally, you can set rotation and other parameters here
  tft.setRotation(0); // Set rotation to 90 degrees (landscape mode)

  tft.fillScreen(ILI9341_BLACK); // Fill the screen with black color or any other color of your choice

  // Display the initial image based on the initial vacancy status
  drawImage(evive_vacant[0]);

  // Add text below the image
  tft.setCursor(70, imgHeight + 20); // Set cursor position below the image
  tft.setTextColor(ILI9341_WHITE); // Set text color
  tft.setTextSize(3); // Set text size
}

void loop() {
  // Send HTTP GET request to the server
  WiFiClient client;
  HTTPClient http;
  http.begin(client, String(serverUrl) + "?request_status"); // Request both lock and vacancy status

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
      JsonObject obj = doc[0];
      // Extract id, vacancy status, lock status, and register fingerprint status from the first element
      int id = obj["id"];
      int lockStatus = obj["lock_status"];
      int vacancyStatus = obj["vacancy_status"];
      String currentOwner = obj["current_owner"];

      Serial.print("ID received: ");
      Serial.println(id);
      Serial.print("Lock status received: ");
      Serial.println(lockStatus);
      Serial.print("Vacancy status received: ");
      Serial.println(vacancyStatus);
      Serial.print("Current owner received: ");
      Serial.println(currentOwner);
      Serial.println("---------------------------------------------------------------------------");
    
      // Check if the vacancy status has changed
      if (vacancyStatus != previousVacancyStatus) {
        // Clear previous text
        tft.fillRect(0, imgHeight, tft.width(), tft.height() - imgHeight, ILI9341_BLACK);
        
        // Print the new vacancy status
        printVacancyTFT(vacancyStatus);
        
        // Update the previous vacancy status
        previousVacancyStatus = vacancyStatus;

        // Display the corresponding image based on vacancy status
        if (vacancyStatus == 0) {
          currentEviveIndex = (currentEviveIndex + 1) % 3; // Cycling through vacant images
          drawImage(evive_vacant[currentEviveIndex]);
        } else {
          currentEviveIndex = (currentEviveIndex + 1) % 3; // Cycling through occupied images
          drawImage(evive_occupied[currentEviveIndex]);
        }
      }

      // Handle LED and lock control based on vacancy and lock status
      if (vacancyStatus == 0 && lockStatus == 0) {
        // Vacant locked = LED green
        digitalWrite(redPin, LOW);
        digitalWrite(greenPin, HIGH);

        digitalWrite(lockPin, LOW);
      } else if (vacancyStatus == 1 && lockStatus == 1) {
        // Occupied unlocked = Blink the red LED
        digitalWrite(lockPin, HIGH);
        blinkRedLED();
      }
      else if (vacancyStatus == 1 && lockStatus == 0) {
        // Occupied locked = LED red
        digitalWrite(lockPin, LOW);
        digitalWrite(redPin, HIGH);
        digitalWrite(greenPin, LOW);
      }
    } else {
      Serial.println("Empty or invalid JSON array");
    }
  } else {
    Serial.println("Error on HTTP request");
  }
  http.end();

  delay(1000); // Poll server every 1 second
}

void drawImage(const uint16_t* image) {
  int imgWidth = 180; // Define the width of the image
  int imgHeight = 180; // Define the height of the image

  // Calculate the starting position to center the image horizontally
  int imgX = (tft.width() - imgWidth) / 2;

  // Display the image
  int buffidx = 1;
  for (int row = 0; row < imgHeight; row++) { // For each scanline...
      for (int col = 0; col < imgWidth; col++) { // For each pixel...
          // To read from Flash Memory, pgm_read_XXX is required.
          // Since the image is stored as uint16_t, pgm_read_word is used as it uses 16-bit address
          tft.drawPixel(col + imgX, row, pgm_read_word(image + buffidx));
          buffidx++;
      } // end pixel
  }
}

void printVacancyTFT(int vacancy) {
  if(vacancy == 0){
    tft.setCursor(70, imgHeight + 20); // Set cursor position below the image
    tft.setTextColor(ILI9341_GREEN); // Set text color
    tft.setTextSize(3); // Set text size
    tft.print("VACANT");
  }
  else {
    tft.setCursor(50, imgHeight + 20); // Set cursor position below the image
    tft.setTextColor(ILI9341_BLUE); // Set text color
    tft.setTextSize(3); // Set text size
    tft.print("OCCUPIED");
  }
}

void blinkRedLED() {
  digitalWrite(greenPin, LOW);
  digitalWrite(redPin, HIGH);
  delay(500); // Wait for 500 milliseconds
  digitalWrite(redPin, LOW);
  delay(500); // Wait for 500 milliseconds
}