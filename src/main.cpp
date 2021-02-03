#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <driver/touch_pad.h>

#define RST_PIN         17
// #define IRQ_PIN         16
#define SPI_MOSI_PIN    23
#define SPI_MISO_PIN    19
#define SPI_SCK_PIN     18
#define SPI_SS1_PIN     5
#define RED_PIN 26
#define YELLOW_PIN 27
#define GREEN_PIN 14
// #define SPI_SS2_PIN     22
// #define SPI_SS3_PIN     7
// #define SPI_SS4_PIN     8
// #define SPI_SS5_PIN     10
// #define SPI_SS6_PIN     11
// #define SPI_SS7_PIN     12
// #define SPI_SS8_PIN     13
#define SLEEP_DELAY 30000
#define AT_WORK
#define RASPAP
#define IN_TRAILER
// #define BT_PAN
// #define BLE
#define WIFI_MODE

#ifdef BT_PAN
#include <BluetoothSerial.h>
BluetoothSerial btPan;
bool btConnected = false;
#endif
#ifdef BLE
#include <BLEDevice.h>
static bool doConnect = false;
static bool connected = false;
static bool doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
#endif
#ifdef WIFI_MODE
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#ifdef AT_WORK
const char* work_ssid = "RGTG";
const char* work_password = "FreshLook333";
String work_host = "10.0.13.148:8080";
#endif
// const char* ssid = "BowlesTools";
// const char* password = "8058705395";
#ifdef RASPAP
const char* raspap_ssid = "raspi-webgui";
const char* raspap_password = "ChangeMe";
//String host = "10.3.141.1";
// String host = "10.3.141.1:8080";
String raspap_host = "192.168.50.1:8080";
#endif
#ifdef IN_TRAILER
const char* sun_ssid = "SunWiFi";
const char* sun_password = "";
String sun_host = "172.16.0.234:8080";
// #define HACK_SUNSHINE
#endif
String host = "";

const char* checkin = "/checkin";
const char* arfid = "/rfid";

MFRC522 rfid1(SPI_SS1_PIN, RST_PIN);
#ifdef SPI_SS2_PIN
MFRC522 rfid2(SPI_SS2_PIN, RST_PIN);
#endif
#ifdef SPI_SS3_PIN
MFRC522 rfid3(SPI_SS3_PIN, RST_PIN);
#endif
#ifdef SPI_SS4_PIN
MFRC522 rfid4(SPI_SS4_PIN, RST_PIN);
#endif
#ifdef SPI_SS5_PIN
MFRC522 rfid5(SPI_SS5_PIN, RST_PIN);
#endif
#ifdef SPI_SS6_PIN
MFRC522 rfid6(SPI_SS6_PIN, RST_PIN);
#endif
#ifdef SPI_SS7_PIN
MFRC522 rfid7(SPI_SS7_PIN, RST_PIN);
#endif
#ifdef SPI_SS8_PIN
MFRC522 rfid8(SPI_SS8_PIN, RST_PIN);
#endif

RTC_DATA_ATTR String lastUID = "";
bool bLastRead1 = false;
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR String checkinTag = "";
RTC_DATA_ATTR volatile bool bNewTouch = false;
volatile bool bNewInt = false;
byte regVal = 0x7F;
long lastRead = 0;

void activateRec(MFRC522 rfid);
void clearInt(MFRC522 rfid);
void setup_rfid(MFRC522 rfid);
bool check_rfid();
void tp_callback();
void connect_wifi();
void send_checkin();
void disconnect_wifi();

#ifdef BLE
class MyBTCallback : public BLEClientCallbacks {
  void onConnect (BLEClient* bc) {

  }
  void onDisconnect (BLEClient* bc) {
    connected = false;
    Serial.println("onDisconnect");
  }
};
class MyBTDeviceCallback : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice device) {
    if(strcmp(device.getName().c_str(),"bpi0tools")==0)
    {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(device);
      doConnect = true;
      doScan = true;
    }
  }
};
#endif

void setup_rfids()
{
  setup_rfid(rfid1);
  #ifdef SPI_SS2_PIN
  setup_rfid(rfid2);
  #endif
  #ifdef SPI_SS3_PIN
  setup_rfid(rfid3);
  #endif
  #ifdef SPI_SS4_PIN
  setup_rfid(rfid4);
  #endif
  #ifdef SPI_SS5_PIN
  setup_rfid(rfid5);
  #endif
  #ifdef SPI_SS6_PIN
  setup_rfid(rfid6);
  #endif
  #ifdef SPI_SS7_PIN
  setup_rfid(rfid7);
  #endif
  #ifdef SPI_SS8_PIN
  setup_rfid(rfid8);
  #endif
}
void setup() {
  Serial.begin(9600);
  while(!Serial);

  #ifdef RED_PIN
  pinMode(RED_PIN, OUTPUT);
  digitalWrite(RED_PIN, 1);
  #endif
  #ifdef YELLOW_PIN
  pinMode(YELLOW_PIN, OUTPUT);
  digitalWrite(YELLOW_PIN, 0);
  #endif
  pinMode(GREEN_PIN, OUTPUT);
  digitalWrite(GREEN_PIN, 0);


  Serial.println();

  // setup_rfid();

  if(bootCount == 0 || checkinTag.equals(""))
  {
    connect_wifi();
    send_checkin();
  }
  
 
  if(bootCount > 0)
  {
    esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
    Serial.print("Wake reason: ");
    Serial.println(reason);
    if(reason == ESP_SLEEP_WAKEUP_TOUCHPAD) {
      int touchPin = esp_sleep_get_touchpad_wakeup_status();
      Serial.print("Touch #: ");
      Serial.println(touchPin);
    }
  }

  
  
  Serial.print(F("Start number: "));
  Serial.println(++bootCount);

  bNewTouch = false;
  for(uint8_t t = T0; t <= T9; t++)
    if(t != T7 && t != T6 && t != T5)
     touchAttachInterrupt(t, tp_callback, 40);

  Serial.println("Waiting for RFID...");

  setup_rfids();

  unsigned long start = millis();
  do {
    if(lastRead == 0 || millis() - lastRead > 500) {
    check_rfid();
    activateRec(rfid1);
    }
    #ifdef SPI_SS2_PIN
    activateRec(rfid2);
    #endif
    delay(250);
    if(bNewTouch)
    {
      start = millis();
      send_checkin();
      bNewTouch = false;
      Serial.println(F("You got touched"));
    }
  } while (millis() - start < SLEEP_DELAY);

  Serial.println(F("Going to sleep..."));

  digitalWrite(RED_PIN, 1);
  digitalWrite(YELLOW_PIN, 0);
  digitalWrite(GREEN_PIN, 0);
  rfid1.PCD_SoftPowerDown();
  disconnect_wifi();
  SPI.end();
  esp_sleep_enable_touchpad_wakeup();
  esp_deep_sleep_start();

  Serial.println(F("You shouldn't be here"));
}

bool is_connected()
{
  #ifdef BLE
  return connected;
  #endif
  #ifdef WIFI_MODE
  digitalWrite(YELLOW_PIN, LOW);
  if(WiFi.status() != WL_CONNECTED) return false;
  // if(strcmp(WiFi.localIP().toString().c_str(), "0.0.0.0") != 0) {
    digitalWrite(YELLOW_PIN, HIGH);
    return true;
  // }
  #endif
  return false;
}
void connect_wifi()
{
  if(is_connected()) return;
  digitalWrite(YELLOW_PIN, HIGH);
  #ifdef BLE
  Serial.println("Scanning Bluetooth...");
  BLEDevice::init("");
  BLEScan* scanner = BLEDevice::getScan();
  scanner->setAdvertisedDeviceCallbacks(new MyBTDeviceCallback());
  scanner->setInterval(1349);
  scanner->setWindow(449);
  scanner->setActiveScan(true);
  scanner->start(5, false);
  while(!connected) 
  {
    Serial.print(".");
    delay(1000);
  }
  //c->connect()
  #endif
  #ifdef WIFI_MODE
  Serial.println(F("Connecting to WiFi..."));
  WiFi.mode(WIFI_MODE_STA);
  #ifdef RASPAP
  unsigned long start = millis();
  WiFi.begin(raspap_ssid, raspap_password);
  delay(1000);
  while(!is_connected())
  {
    Serial.print(".");
    delay(250);
    if(millis() - start > 5000) break;
    digitalWrite(RED_PIN, 1);
    Serial.print(".");
    delay(250);
    if(millis() - start > 5000) break;
    digitalWrite(RED_PIN, 0);
  }
  if(is_connected()) host = raspap_host;
  #endif
  #ifdef AT_WORK
  if(!is_connected())
  {
    start = millis();
    WiFi.begin(work_ssid, work_password);
    delay(1000);
    while(!is_connected())
    {
      Serial.print(".");
      delay(250);
      if(millis() - start > 5000) break;
    }
    if(is_connected()) host = work_host;
  }
  #endif
  #ifdef IN_TRAILER
  if(!is_connected())
  {
    start = millis();
    WiFi.begin(sun_ssid, sun_password);
    delay(1000);
    while(!is_connected())
    {
      Serial.print(".");
      delay(250);
      if(millis() - start > 5000) break;
    }
    if(is_connected()) host = sun_host;
  }
  #endif
  if(!is_connected()) {
    Serial.println("Unable to connect to WiFi!");
  }
  Serial.print(F("WiFi connected to "));
  Serial.print(WiFi.SSID());
  Serial.print(F(" with IP "));
  Serial.println(WiFi.localIP());
  #ifdef HACK_SUNSHINE
  HTTPClient client;
  client.begin("http://172.16.0.1/");
  client.sendRequest("GET", "");
  client.begin("https://sunwifime.com/login/?NODE_MAC=00%3a0D%3aB9%3a44%3a74%3a20&LOCAL_IP=172%2e16%2e0%2e175&MAC=AC%3a67%3aB2%3a11%3a68%3a74");
  client.sendRequest("GET", "");
  client.begin("https://sunwifime.com/");
  client.GET();
  Serial.println(client.getString());
  client.end();
  #endif
  #endif
}

void disconnect_wifi()
{
  if(!is_connected()) return;
  #ifdef BT_PAN
    btPan.disconnect();
  #endif
  #ifdef BLE
    BLEDevice::createClient()->disconnect();
  #endif
  #ifdef WIFI_MODE
  WiFi.disconnect();
  #endif
}

String get_endpoint(const char* cmd)
{
  String ret = String();
  ret.concat("http://");
  ret.concat(host);
  ret.concat(cmd);
  return ret;
}
void send_checkin()
{
  connect_wifi();
  #ifdef BT_PAN
  
  #endif
  #ifdef BLE

  #endif
  #ifdef WIFI_MODE
  HTTPClient client;

  String url = get_endpoint(checkin);
  Serial.print("Requesting from ");
  Serial.println(url);
  client.begin(url);
  client.GET();
  checkinTag = client.getString();
  Serial.print("Received tag: ");
  Serial.println(checkinTag);
  client.end();
  #endif
}
void send_data(String data, const char* endpoint, const char* params)
{
  connect_wifi();
  #ifdef WIFI_MODE
  HTTPClient client;
  String url = get_endpoint(endpoint);
  if(sizeof(params)>2)
  {
    url.concat(params);
  }
  client.begin(url);
  client.PUT(data);
  client.end();
  #endif
}

String get_uid(MFRC522 rfid) {
  if(!rfid.uid.size) return lastUID;
  lastUID = "";
  for(int i=0;i<rfid.uid.size;i++)
  {
    //uid.concat(rfid.uid.uidByte[i]);
    if(i>0)
      lastUID.concat("-");
    lastUID.concat(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    lastUID.concat(String(rfid.uid.uidByte[i], HEX));
  }
  return lastUID;
}
bool check_rfid_pos(MFRC522 rfid) {
  if(!rfid.PICC_IsNewCardPresent()) return false;
  if(!rfid.PICC_ReadCardSerial()) return false;
  Serial.print(F("Card UID: "));
  
  get_uid(rfid);
  
  Serial.println(lastUID);
  
  #ifdef IRQ_PIN
  clearInt(rfid);
  #endif
  rfid.PICC_HaltA();
  // lastRead = millis();
  // rfid.
  bNewInt = false;
  return true;
}

bool check_rfid() {
  // if(!bNewInt) return false;
  if(bNewInt) digitalWrite(GREEN_PIN, HIGH);
  bool present = check_rfid_pos(rfid1);
  if(bLastRead1 != present)
  {
    bLastRead1 = present;
    if(present)
      send_data(lastUID, arfid, present?"":"?checkout=1");
    digitalWrite(GREEN_PIN, bLastRead1);
  }
  #ifdef SPI_SS2_PIN
  check_rfid_pos(rfid2);
  #endif
  return true;
}

void tp_callback() {
  bNewTouch = true;
}
void readCard() {
  bNewInt = true;
}

void setup_rfid(MFRC522 rfid)
{
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
  rfid.PCD_Init();		// Init MFRC522
  // rfid.PCD_SetAntennaGain(rfid.RxGain_max);
  // rfid.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
  #ifdef IRQ_PIN
    pinMode(IRQ_PIN, INPUT_PULLUP);
    regVal = 0xA0; //rx irq
    rfid.PCD_WriteRegister(rfid.ComIEnReg, regVal);
    attachInterrupt(digitalPinToInterrupt(IRQ_PIN), readCard, FALLING);
  #endif
  bNewInt = 0;
}

void loop() {
  Serial.println("hello yermom");
  delay(1000);
  
}

/*
 * The function sending to the MFRC522 the needed commands to activate the reception
 */
void activateRec(MFRC522 rfid) {
  rfid.PCD_WriteRegister(rfid.FIFODataReg, rfid.PICC_CMD_REQA);
  rfid.PCD_WriteRegister(rfid.CommandReg, rfid.PCD_Transceive);
  rfid.PCD_WriteRegister(rfid.BitFramingReg, 0x87);
}

/*
 * The function to clear the pending interrupt bits after interrupt serving routine
 */
void clearInt(MFRC522 rfid) {
  rfid.PCD_WriteRegister(rfid.ComIrqReg, 0x7F);
}
