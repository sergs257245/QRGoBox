#include <SoftwareSerial.h>

SoftwareSerial gsmSerial(10, 11); // RX, TX for Arduino and TXD, RXD for the module

int led = 13;

void setup() {
  pinMode(led, OUTPUT);

  Serial.begin(115200);
  gsmSerial.begin(115200);

  Serial.println("Starting...");
  delay(1000);
}

void loop() {
  if (Serial.available()) {
    String serialData = Serial.readStringUntil('\n'); // Read the entire message until newline character

    if (serialData.startsWith("INVALID_FINGERPRINT") || serialData.startsWith("ADMIN_UNLOCK")) {
      if (serialData.startsWith("INVALID_FINGERPRINT")) {
        Serial.println("---------------------------------------------------------------------------");
        Serial.println("INVALID_FINGERPRINT");
        digitalWrite(led, HIGH);
        delay(250);
        digitalWrite(led, LOW);
        delay(250);
        digitalWrite(led, HIGH);
        delay(250);
        digitalWrite(led, LOW);
        delay(250);
      } else if (serialData.startsWith("ADMIN_UNLOCK")) {
        Serial.println("---------------------------------------------------------------------------");
        Serial.println("ADMIN_UNLOCK");
        digitalWrite(led, HIGH);
        delay(250);
        digitalWrite(led, LOW);
        delay(250);
        digitalWrite(led, HIGH);
        delay(250);
        digitalWrite(led, LOW);
        delay(250);
      }

      // Extract ID, current owner's name, and phone number from the serial message
      int delimiterIndex1 = serialData.indexOf(":") + 1; // Find the index of ":" and add 1 to get the start of the ID
      int delimiterIndex2 = serialData.indexOf(",", delimiterIndex1); // Find the index of "," after the ID
      int delimiterIndex3 = serialData.indexOf(",", delimiterIndex2 + 1); // Find the index of "," after the owner's name

      // Extract ID
      String id = serialData.substring(delimiterIndex1, delimiterIndex2);
      id.trim();

      // Extract current owner's name
      String currentOwner = serialData.substring(delimiterIndex2 + 1, delimiterIndex3);
      currentOwner.trim();

      // Extract phone number
      String phoneNumber = serialData.substring(delimiterIndex3 + 1);
      phoneNumber.trim();

      // Print extracted information
      Serial.println("ID: " + id);
      Serial.println("Phone Number: " + phoneNumber);
      Serial.println("Current Owner: " + currentOwner);

      String message;
      String link1 = "qrgobox%2Ecom/from%5Fsms%2Ephp?n=1&b=stical&u=" + currentOwner + "&i=" + id;
      String link2 = "qrgobox%2Ecom/from%5Fsms%2Ephp?n=2&b=stical&u=" + currentOwner + "&i=" + id;

      if (serialData.startsWith("INVALID_FINGERPRINT")) {
        message = "QRGOBOX SMS ALERT:\n\n" + currentOwner + ", your GoBox has scanned an invalid fingerprint! Proceed to this link to view your GoBox now:\n\n" + link1;
      } else if (serialData.startsWith("ADMIN_UNLOCK")) {
        message = "QRGOBOX SMS ALERT:\n\n" + currentOwner + ", your GoBox has been unlocked by an administrator! Proceed to this link to view your GoBox now:\n\n" + link2;
      }

      Serial.println(message);
      sendSMS(phoneNumber, message);
    }
  }
}

void sendSMS(String phoneNumber, String message) {
  // Set SIM800L to SMS text mode
  gsmSerial.println("AT+CMGF=1");
  delay(100);

  // Send SMS command with the phone number
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(phoneNumber);
  gsmSerial.println("\"");
  delay(500);

  // Send SMS content
  gsmSerial.print(message);

  // End SMS message with CTRL+Z
  gsmSerial.write(26);
  delay(8000); // Wait for the SMS to be sent

  Serial.println("SMS Sent.");
  digitalWrite(led, HIGH);
  delay(1000);
  digitalWrite(led, LOW);
}