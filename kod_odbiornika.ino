#include <WiFi.h>
#include <ESP32Time.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <esp_now.h>

#define SWITCH 15
Adafruit_SH1106 display(23, 19);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH 16

uint8_t broadcastAddress[] = {0xC8, 0xF0, 0x9E, 0xF1, 0x7B, 0xF8};
bool buttonPressed = false;
unsigned long buttonPressStartTime = 0;

bool newDataReceived = false;
struct myData {
  String Type;
  String cardNumber;
  String owner;
  bool Valid;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t day;
  uint8_t month;
  uint16_t year; 
};

const int maxData = 50;
myData Data[maxData];
int counter = 0;
int counterDisplay = 1;
int lastSwitch = 0;
esp_now_peer_info_t peerInfo;
unsigned long lastSendTime = 0;

void clearDataArray()
{
  for (int i = 0; i < maxData; i++)
  {
    Data[i] = {}; // Initialize with default values
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
  } else {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Alarm!");
    display.println("Brak lacznosci!");
    display.println("Sprawdz nadajnik.");
    digitalWrite(14, HIGH);
    display.display();
    delay(500);
    display.clearDisplay();
    display.display();
    digitalWrite(14, LOW);
    delay(500);
  }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  myData newData;
  memcpy(&newData, incomingData, sizeof(newData));
  if (counter < maxData && newData.Type == "alarm")
  {
    digitalWrite(14, HIGH);
    Data[counter] = newData;
    counter++;
    memcpy(&newData, incomingData, sizeof(newData));
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Data: ");
    display.print(Data[counter - 1].day);
    display.print(".");
    display.print(Data[counter - 1].month);
    display.print(".");
    display.print(Data[counter - 1].year);
    display.print(" ");
    display.print(Data[counter - 1].hour);
    display.print(":");
    display.print(Data[counter - 1].minute);
    display.print(":");
    display.println(Data[counter - 1].second);
    display.println("");
    display.println("Alarm!");
    display.println("Wykryty ruch w");
    display.println("strefie chronionej!");
    display.display();
    delay(1250);
    digitalWrite(14, LOW);
    delay(750);
    display.clearDisplay();
    display.display();
  }
  if (counter < maxData && newData.Type == "card")
  {
    Data[counter] = newData;
    newDataReceived = true;
    counter++;
  }
}
String lastCardNumber = "";

void setup()
{
  pinMode(14, OUTPUT);
  digitalWrite(14, LOW);
  pinMode(SWITCH, INPUT);
  Serial.begin(115200);
 
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
         
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.display();
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);
  Serial.println(WiFi.macAddress()); 
}

void loop()
{
 unsigned long currentTime = millis();
  int x = 1;
  int Switch = digitalRead(SWITCH);

  if (Switch == 1 && lastSwitch == 0)
  {
    buttonPressed = true;
    buttonPressStartTime = currentTime;
  }

  if (buttonPressed && Switch == 0)
  {
    buttonPressed = false;
  }

  if (buttonPressed && currentTime - buttonPressStartTime >= 2000)
  {
    clearDataArray();
    counter = 0;
    counterDisplay = 1;
    buttonPressed = false;
  }

  if (Switch == 1 && lastSwitch == 0)
  { 
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Odczyt numer: ");
    display.println(counterDisplay);
    display.println("Data: ");
    display.print(Data[counterDisplay - 1].day);
    display.print(".");
    display.print(Data[counterDisplay - 1].month);
    display.print(".");
    display.print(Data[counterDisplay - 1].year);
    display.print(" ");
    display.print(Data[counterDisplay - 1].hour);
    display.print(":");
    display.print(Data[counterDisplay - 1].minute);
    display.print(":");
    display.println(Data[counterDisplay - 1].second);
    display.println("");
    if(Data[counterDisplay - 1].Type == "alarm")
    {
      display.println("Typ zdarzenia: alarm");
      display.display();
    }
    else
    {
    display.print("Numer karty:");
    for (int x = 0; x < Data[counterDisplay - 1].cardNumber.length(); x += 2)
    {
      String hexPair = Data[counterDisplay - 1].cardNumber.substring(x, x + 2);
      uint8_t decimalValue = (uint8_t)strtol(hexPair.c_str(), nullptr, 16);
      char hexChar[3];
      sprintf(hexChar, "%02X", decimalValue);
      display.print(hexChar);
    }
    display.println();
    display.println("Wlasciciel karty:");
    display.println(Data[counterDisplay - 1].owner);
    if (Data[counterDisplay - 1].Valid)
    {
      display.println("Dostep przyznany");
      display.display();
    }
    else
    {
      display.println("Odmowa dostepu");
      display.display();
    }
    }
    counterDisplay++;
    if (counterDisplay >= counter + 1)
    {
      counterDisplay = 1;
    }
    delay(200);
  }
  lastSwitch = Switch;

  if (newDataReceived)
  {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Data: ");
    display.print(Data[counter - 1].day);
    display.print(".");
    display.print(Data[counter - 1].month);
    display.print(".");
    display.print(Data[counter - 1].year);
    display.print(" ");
    display.print(Data[counter - 1].hour);
    display.print(":");
    display.print(Data[counter - 1].minute);
    display.print(":");
    display.println(Data[counter - 1].second);
    display.println("");
    display.print("Numer karty:");
    for (int x = 0; x < Data[counter - 1].cardNumber.length(); x += 2)
    {
      String hexPair = Data[counter - 1].cardNumber.substring(x, x + 2);
      uint8_t decimalValue = (uint8_t)strtol(hexPair.c_str(), nullptr, 16);
      char hexChar[3];
      sprintf(hexChar, "%02X", decimalValue);
      display.print(hexChar);
    }
    display.println();
    display.println("Wlasciciel karty:");
    display.println(Data[counter - 1].owner);
    if (Data[counter - 1].Valid)
    {
      display.println("Dostep przyznany");
      display.display(); 
    }
    else
    {
      display.println("Odmowa dostepu");
      display.display();
    }
    newDataReceived = false;
    delay(500);
    display.clearDisplay();
    lastCardNumber = Data[counter-1].cardNumber;
  }
   if (currentTime - lastSendTime >= 500) {
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&x, sizeof(x));
    if (result == ESP_OK) {
      lastSendTime = currentTime;
    }
   }
}
