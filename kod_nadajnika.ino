#include <virtuabotixRTC.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <String.h>
#include <vector>

#define PIR 22
#define SS_PIN 21


#define RST_PIN 25
#define Status_LED_GREEN 14
#define Status_LED_RED 12
#define BUZZER 27

virtuabotixRTC myRTC(26, 25, 13);

MFRC522 mfrc522(SS_PIN, RST_PIN);
uint8_t broadcastAddress[] = {0x08, 0xB6, 0x1F, 0x80, 0xCD, 0xE8};
esp_now_peer_info_t peerInfo;

struct CardData {
  String card_ID;
  String owner_ID;
};

  std::vector<CardData> cardDataList = {
      {"d54aa643", "Damian"}
    };

bool alarmActive = false;    
bool personIn = false;
unsigned long pirTriggerTime = 0;
    
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


void setup() {
  Serial.begin(9600);
  pinMode(PIR, INPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  pinMode(Status_LED_GREEN, OUTPUT);
  digitalWrite(Status_LED_GREEN, LOW);
  pinMode(Status_LED_RED, OUTPUT);
  digitalWrite(Status_LED_GREEN, LOW);

//myRTC.setDS1302Time(15, 20, 19, 5, 1, 9, 2023);
  
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
   Serial.println("Error initializing ESP-NOW");
   return;
 }
  esp_now_register_send_cb(OnDataSent);
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println(WiFi.macAddress());

  Serial.println("Proszę zbliżyć kartę");
}

void loop() {
  myRTC.updateTime();
  bool check;

struct Send {
  String Type;
  String selectedCard_ID;
  String selectedowner_ID;
  bool Valid;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t day;
  uint8_t month;
  uint16_t year;
};

if (digitalRead(PIR) == HIGH && !alarmActive) {
  unsigned long currentTime = millis();
  
  if (!pirTriggerTime) {
    pirTriggerTime = currentTime;
  }
  
  if (currentTime - pirTriggerTime > 10 000 && !personIn) {
    Send SendData;
    SendData.Type = "alarm";
    SendData.hour = myRTC.hours;
    SendData.minute = myRTC.minutes;
    SendData.second = myRTC.seconds;
    SendData.day = myRTC.dayofmonth;
    SendData.month = myRTC.month;
    SendData.year = myRTC.year;
    esp_now_send(broadcastAddress, (uint8_t *)&SendData, sizeof(SendData));
    digitalWrite(BUZZER, HIGH);
    delay(500);
    digitalWrite(BUZZER, LOW);
    Serial.println("Alarm");
    pirTriggerTime = 0;  
  }
}
Send SendData;

if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.print("Numer seryjny karty: ");
    String read_ID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) {
            read_ID += "0";  // Add leading zero if needed
        }
        read_ID += String(mfrc522.uid.uidByte[i], HEX);
    }
    mfrc522.PICC_HaltA();

      for (const auto& cardData : cardDataList) {
      if (cardData.card_ID == read_ID) {
        check = true;
        SendData.Type = "card";
        SendData.selectedCard_ID = cardData.card_ID;
        SendData.selectedowner_ID = cardData.owner_ID;
        SendData.Valid = check;
        SendData.hour = myRTC.hours;
        SendData.minute = myRTC.minutes;
        SendData.second = myRTC.seconds;
        SendData.day = myRTC.dayofmonth;
        SendData.month = myRTC.month;
        SendData.year = myRTC.year;
      }
      else
      {
        check = false;
        SendData.Type = "card";
        SendData.selectedCard_ID = read_ID;
        SendData.selectedowner_ID = "Nieznany ";
        SendData.Valid = check;
        SendData.hour = myRTC.hours;
        SendData.minute = myRTC.minutes;
        SendData.second = myRTC.seconds;
        SendData.day = myRTC.dayofmonth;
        SendData.month = myRTC.month;
        SendData.year = myRTC.year;
      }
    }
    
  if (check) {
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&SendData, sizeof(SendData));
   
  if (result == ESP_OK) {
      Serial.println("Sent with success");
      }
      else {
      Serial.println("Error sending the data");
      }
      if(!alarmActive)
      {
        alarmActive = true;
      }
      else 
      {
        alarmActive = false;
      }
      if(!personIn){
        personIn = true;
      }
      else{
        personIn = false;
        pirTriggerTime = 0; 
      }
      digitalWrite(Status_LED_GREEN, HIGH);
      digitalWrite(BUZZER, HIGH);
      delay(500);
      digitalWrite(Status_LED_GREEN, LOW);
      digitalWrite(BUZZER, LOW);
      delay(300);
      }
    else {
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&SendData, sizeof(SendData));
      digitalWrite(Status_LED_RED, HIGH);
      digitalWrite(BUZZER, HIGH);
      delay(500);
      digitalWrite(Status_LED_RED, LOW);
      digitalWrite(BUZZER, LOW);
      delay(300);
      digitalWrite(Status_LED_RED, HIGH);
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(Status_LED_RED, LOW);
      digitalWrite(BUZZER, LOW);
      delay(100);
    }
  }
}
