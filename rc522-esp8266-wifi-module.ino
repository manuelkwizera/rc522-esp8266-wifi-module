#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>


/* define wifi credentials */
#define SERVER_IP "192.168.150.102"
#ifndef STASSID
#define STASSID "COFE_53A8"
#define STAPSK "cofe8347"
#endif

/* rc522 card  reader pinout */
constexpr uint8_t RST_PIN = D3;  // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = D4;   // Configurable, see typical pin layout above

MFRC522 rfid(SS_PIN, RST_PIN);  // Instance of the class
MFRC522::MIFARE_Key key;

/* this variable hold the serial number of the rfid card */
String tag;

/* associative array */

void setup() {
  Serial.begin(115200);
  SPI.begin();      // Init SPI bus
  rfid.PCD_Init();  // Init MFRC522

  /* connect node mcu board to wifi */
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  String tag = scanRfid();

  if (!tag.isEmpty()) {
    Serial.println("");
    Serial.print("CARD DETECTED " + tag);
    Serial.println("");
    delay(1000);
    Serial.println("PROCESSING...");
    delay(2000);
    postRequest(tag);
  }

  Serial.println("");
  Serial.print("TAP THE CARD...");
  delay(5000);
}

String scanRfid() {
  String tag = "";

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    for (byte i = 0; i < 4; i++) {
      tag += rfid.uid.uidByte[i];
    }
    Serial.println(tag);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  return tag;
}

void postRequest(String data) {
  // Ensure there is a valid WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] No WiFi connection");
    return;
  }

  WiFiClient client;
  HTTPClient http;

  // configure traged server and url
  http.begin(client, "http://" + String(SERVER_IP) + "/kwanzaa-pay/public/api/register-card");
  http.addHeader("Content-Type", "application/json");


  // start connection and send HTTP header and body
  int httpCode = http.POST(data);

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      const String& payload = http.getString();
      Serial.println("");
      Serial.print("SERVER RESPONSE: " + payload);
      Serial.println("");
    } else {
      Serial.printf("[HTTP] POST... server returned error code: %d\n", httpCode);
    }
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

