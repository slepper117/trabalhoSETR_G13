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
const String urlPath = "clockIn";
const int servoPin = 16;

// Declare Constants
byte readCard[4];
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
String tagID = "";

// Create instances
Servo servo;
MFRC522 mfrc522(SS_PIN, RST_PIN);
EthernetClient EthernetClient;
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);  //Parameters: (rs, enable, d4, d5, d6, d7)
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
    int angle = 0;
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
      digitalWrite(LEDG, HIGH);

      // TODO: Parse JSON and output name in second line of LCD
      // https://arduinojson.org/v6/example/parser/
      // https://arduinojson.org/v6/doc/deserialization/
      // String response = client.responseBody();
      // lcd.print("Authorized");
      // lcd.setCursor(0, 1);
      // lcd.print( ---Name from JSON Response---);

      // TODO: Rotate Servo
      // for (angle = 0; angle < 25; angle += 1) {
      //   servo.write(angle);
      //   delay(20);
      // }

      // Debug Response
      Serial.println("Authorized");
      String response = client.responseBody();
      Serial.print("Response: ");
      Serial.println(response);
    } else {
      digitalWrite(LEDR, HIGH);
      // lcd.print("Not Authorized");

      // Debug Response
      Serial.println("Not Authorized");
      String response = client.responseBody();
      Serial.print("Response: ");
      Serial.println(response);
      digitalWrite(LEDR, HIGH);
    }

    delay(2000);

    digitalWrite(LEDG, LOW);
    digitalWrite(LEDR, LOW);

    lcd.clear();
    lcd.print(" Access Control ");
    lcd.setCursor(0, 1);
    lcd.print("Scan Your Card>>");
  }
}

//Read new tag if available
boolean getID() {
  // Getting ready for Reading PICCs
  if (!mfrc522.PICC_IsNewCardPresent()) {  //If a new PICC placed to RFID reader continue
    return false;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {  //Since a PICC placed get Serial and continue
    return false;
  }
  tagID = "";
  for (uint8_t i = 0; i < 4; i++) {  // The MIFARE PICCs that we use have 4 byte UID
    //readCard[i] = mfrc522.uid.uidByte[i];
    tagID.concat(String(mfrc522.uid.uidByte[i], HEX));  // Adds the 4 bytes in a single String variable
  }
  tagID.toUpperCase();
  mfrc522.PICC_HaltA();  // Stop reading
  return true;
}