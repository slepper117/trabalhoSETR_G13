#include <SPI.h>
#include <Servo.h>
#include <MFRC522.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include <LiquidCrystal.h>
#include <ArduinoHttpClient.h>

#define SS_PIN 7
#define RST_PIN 9
#define LEDG 14
#define LEDR 15

// Set Constants
const String urlPath = "clockin";
const int servoPin = 16;

// Declare Constants
byte readCard[4];
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
String tagID = "";

// Create instances
Servo servo;
MFRC522 mfrc522(SS_PIN, RST_PIN);
EthernetClient EthernetClient;
LiquidCrystal lcd(3, 4, 5, 6, 17, 2);  //Parameters: (rs, enable, d4, d5, d6, d7)
HttpClient client = HttpClient(EthernetClient, "setr.braintechcloud.com", 80);

void setup() {
  // Initialize Serial for debug
  Serial.begin(9600);

  // Initialize Ethernet
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to obtaining an IP address using DHCP");
    while (true)
      ;
  }

  // Initialize RFID Reader
  mfrc522.PCD_Init();

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print(" Access Control ");
  lcd.setCursor(0, 1);
  lcd.print("Scan Your Card>>");

  // Initialize Led
  pinMode(LEDG, OUTPUT);
  pinMode(LEDR, OUTPUT);

  // Initialize Servo
  servo.attach(servoPin);
}

void loop() {
  //Wait until new tag is available
  while (getID()) {
    // Set Default Variables
    String postData = "";
    // Serialize JSON
    StaticJsonDocument<32> doc;
    doc["tag"] = tagID;
    serializeJson(doc, postData);

    // Make Request
    client.beginRequest();
    client.post("/access/" + urlPath);
    client.sendHeader("Authorization", "Basic enpLbjRpdjZNVXBDeUNWYjpTQWRnNHQzU2RiUkdsSHZtS2J0ZEVadHExbWg5Y0dEVw==");
    client.sendHeader("Content-Type", "application/json");
    client.sendHeader("Content-Length", postData.length());
    client.beginBody();
    client.print(postData);
    client.endRequest();

    // Get Status Code from response
    int statusCode = client.responseStatusCode();

    // Clears LCD
    lcd.clear();
    lcd.setCursor(0, 0);

    if (statusCode == 200) {
      String response = client.responseBody();

      // Debug Response
      Serial.println("Authorized");
      Serial.print("Response: ");
      Serial.println(response);

      // Turns the Green Led
      digitalWrite(LEDG, HIGH);

      // Parse JSON and output name in second line of LCD
      StaticJsonDocument<100> res;

      // Deserialize the JSON document
      DeserializationError error = deserializeJson(res, response);

      // Test if parsing succeeds.
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      // Fetch Values
      const char* name = res["name"];

      // Prints
      lcd.print("Authorized");
      lcd.setCursor(0, 1);
      lcd.print(name);

      // Rotate Servo
      for (int i = 1; i < 91; i += 1) {
        servo.write(i);
        delay(20);
      }

    } else {
      // Debug Response
      Serial.println("Not Authorized");
      String response = client.responseBody();
      Serial.print("Response: ");
      Serial.println(response);

      // Turn On Red Led
      digitalWrite(LEDR, HIGH);
      // LCD Print Not Auhtorized
      lcd.print("Not Authorized");
    }

    // Delays 2 secs
    delay(2000);

    // Turns Off Leds
    digitalWrite(LEDG, LOW);
    digitalWrite(LEDR, LOW);

    // Clears LCD
    lcd.clear();
    lcd.print(" Access Control ");
    lcd.setCursor(0, 1);
    lcd.print("Scan Your Card>>");
  }
}

//Read new tag if available
boolean getID() {
  // Getting ready for Reading PICCs
  //If a new PICC placed to RFID reader continue
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }
  //Since a PICC placed get Serial and continue
  if (!mfrc522.PICC_ReadCardSerial()) {
    return false;
  }
  // Reset TagID to empty
  tagID = "";
  // The MIFARE PICCs that we use have 4 byte UID
  for (uint8_t i = 0; i < 4; i++) {
    // Adds the 4 bytes in a single String variable
    tagID.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  // Stop reading
  mfrc522.PICC_HaltA();
  return true;
}
