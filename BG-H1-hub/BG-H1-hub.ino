/*
  Designed for ESP32
  Radio module : SX1278 (Ra-02)

  Wifi AP
    - default ssid : bghub0
    - default password : 12345678
    - default address : 192.164.4.1 (bgsecure0.local)
*/
#include <Arduino.h> // native
#include <EEPROM.h>  // native
#include <FS.h>      // native
#include <SPIFFS.h>  // native

#ifndef ESP32
#error Only designed for ESP32
#endif

// needed for ArduinoJson library
#define ARDUINOJSON_ENABLE_NAN 1
#include <ArduinoJson.h>          // available in library manager (V7)
#include <ESPAsyncWebServer.h>    // https://github.com/me-no-dev/ESPAsyncWebServer (don't get fork in library manager)
// and https://github.com/me-no-dev/AsyncTCP

#include "config.h"

#define ML_SX1278
#include <MLiotComm.h>           // GitHub project : https://github.com/PM04290/MLiotComm
iotCommClass MLiotComm;
uint8_t uid;
uint16_t RadioFreq = 433;
uint8_t RadioRange = 1;
bool loraOK;
bool pairModulePending = false;

JsonDocument docJson;

// for rtc
#include "src/datetime.h"
DateTime rtc = DateTime(2020, 1, 1, 0, 0, 0);
volatile bool newRTC = false;
portMUX_TYPE ntpMux = portMUX_INITIALIZER_UNLOCKED;
// for NTP update
TaskHandle_t xHandleNTP = NULL;

// for Home Assistant
#include <PubSubClient.h> // available in library manager
WiFiClient wifimqttclient;
PubSubClient mqtt(wifimqttclient);
bool needPublishOnline = true;
uint8_t Watchdog = DEFAULT_ONLINE_WATCHDOG;

#include "tools.hpp"
#include "devices.hpp"
#include "webserver.hpp"

// buffer table for LoRa incoming packet
#define MAX_PACKET 60
typedef struct __attribute__((packed))
{
  uint8_t version;
  int lqi;
  rl_packets packets;
} packet_version;
packet_version packetTable[MAX_PACKET];
volatile byte idxReadTable = 0;
volatile byte idxWriteTable = 0;

// for 1 Second interrup
hw_timer_t* secTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool topSecond = false;

//
bool onBoot = true;

void MQTTcallback(char* topic, byte* payload, unsigned int length)
{
  char p[length + 1];
  strncpy(p, (char*)payload, length);
  p[length] = 0;
  Hub.onMessageMQTT(topic, p);
}

void IRAM_ATTR onTimer()
{
  portENTER_CRITICAL_ISR(&timerMux);
  topSecond = true;
  portEXIT_CRITICAL_ISR(&timerMux);
}


bool getPacket(packet_version* p)
{
  if (idxReadTable != idxWriteTable)
  {
    *p = packetTable[idxReadTable];
    //
    idxReadTable++;
    if (idxReadTable >= MAX_PACKET)
    {
      idxReadTable = 0;
    }
    return true;
  }
  return false;
}

void onLoRaReceive(uint8_t len, rl_packet_t* p)
{
  noInterrupts();
  memcpy(&packetTable[idxWriteTable].packets.current, p, len);
  packetTable[idxWriteTable].version = 1;
  packetTable[idxWriteTable].lqi = MLiotComm.lqi();
  if (len == RL_PACKETV1_SIZE) packetTable[idxWriteTable].version = 1;
  idxWriteTable++;
  if (idxWriteTable >= MAX_PACKET)
  {
    idxWriteTable = 0;
  }
  if (idxWriteTable == idxReadTable) // overload
  {
    idxReadTable++;
    if (idxReadTable >= MAX_PACKET)
    {
      idxReadTable = 0;
    }
  }
  interrupts();
}

void processLoRa()
{
  packet_version p;
  if (getPacket(&p))
  {

    rl_packet_t* cp = &p.packets.current;

    DEBUGf("%d => %d[%d]\n", cp->senderID, cp->destinationID, cp->childID);
    if (cp->destinationID == RL_ID_BROADCAST)
    {
      // pairing
      if (pairModulePending && cp->childID == RL_ID_CONFIG && (rl_element_t)(cp->sensordataType >> 3) == E_CONFIG)
      { // config packet
        Hub.pairingConfig(cp);
        return;
      }
    }
    if (cp->destinationID == uid)
    {
      // update new state
      Hub.loraToState(cp);
    }
  }
}

void setup()
{
  DEBUGinit();
  //

  heap_caps_register_failed_alloc_callback(heap_caps_alloc_failed_hook);
  DEBUGln("Start debug");

  if (!SPIFFS.begin())
  {
    DEBUGln(F("SPIFFS Mount failed"));
    // critical error, freeze
    while (true);
  } else
  {
    DEBUGln(F("SPIFFS Mount succesfull"));
    listDir("/");
  }
  delay(50);

  if (!EEPROM.begin(EEPROM_MAX_SIZE))
  {
    DEBUGln("failed to initialise EEPROM");
    // critical error, freeze
    while (true);
  } else {
    char code = EEPROM.readChar(EEPROM_DATA_UID);
    //
    if ((byte)code == 0xFF) {
      code = '0';
      AP_ssid[strlen(AP_ssid) - 1] = code;
      EEPROM.writeChar(EEPROM_DATA_UID, code);
      EEPROM.writeUShort(EEPROM_DATA_FREQ, RadioFreq);
      EEPROM.writeByte(EEPROM_DATA_RANGE, RadioRange);

      EEPROM.writeString(EEPROM_TEXT_OFFSET, Wifi_ssid);
      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 1), Wifi_pass);

      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 2), mqtt_host);
      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 3), mqtt_user);
      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 4), mqtt_pass);

      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 5), datetimeTZ);
      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 6), datetimeNTP);

      EEPROM.commit();
    }
    if (code >= '0' && code <= '9')
    {
      uid = (code - '0');
      AP_ssid[strlen(AP_ssid) - 1] = code;
      RadioFreq = EEPROM.readUShort(EEPROM_DATA_FREQ);
      if (RadioFreq == 0xFFFF) RadioFreq = 433;
      RadioRange = EEPROM.readByte(EEPROM_DATA_RANGE) % 4;

      strcpy(Wifi_ssid, EEPROM.readString(EEPROM_TEXT_OFFSET).c_str());
      strcpy(Wifi_pass, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 1).c_str());

      strcpy(mqtt_host, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 2).c_str());
      strcpy(mqtt_user, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 3).c_str());
      strcpy(mqtt_pass, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 4).c_str());

      strcpy(datetimeTZ, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 5).c_str());
      strcpy(datetimeNTP, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 6).c_str());

    } else {
      uid = 90;
    }
  }
  DEBUGln(AP_ssid);
  DEBUGf("UID:%d\n", uid);
  DEBUGf("Freq:%d\n", RadioFreq);
  DEBUGf("Range:%d\n", RadioRange);
  DEBUGln(Wifi_ssid);
  DEBUGln(Wifi_pass);
  DEBUGln(mqtt_host);
  DEBUGln(mqtt_user);
  DEBUGln(mqtt_pass);
  DEBUGln(datetimeTZ);
  DEBUGln(datetimeNTP);

  loraOK = MLiotComm.begin(RadioFreq * 1E6, onLoRaReceive, NULL, 20, RadioRange);
  if (loraOK)
  {
    DEBUGf("LoRa ok at %dMHz (range %d)\n", RadioFreq, RadioRange);
    //
  } else {
    DEBUGln("LoRa ERROR");
  }
  analogReadResolution(12);  // 12 bits (0-4095)
  analogSetAttenuation(ADC_11db);  // 0-3.3V

  Hub.loadConfig();

  initNetwork();
  initWeb();

  // define time ISR
  secTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(secTimer, &onTimer, true);
  timerAlarmWrite(secTimer, 1000000, true);
  timerAlarmEnable(secTimer);
  //
  xTaskCreate( TaskNTP, "getNTP", 2048, (void*)&rtc, tskIDLE_PRIORITY, &xHandleNTP );
  //
  DEBUGf("heap_caps_get_largest_free_block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

void loop()
{
  static uint32_t mtick = 0;
  static uint32_t htick = 0;
  static uint32_t dtick = 0;
  bool doWatchDog = false;
  bool doMinute = false;
  bool doHour = false;
  bool doDay = false;
  ws.cleanupClients();
  if (topSecond)
  {
    portENTER_CRITICAL(&timerMux);
    topSecond = false;
    portEXIT_CRITICAL(&timerMux);
    //
    uint32_t t = rtc.unixtime() + 1;
    portENTER_CRITICAL_ISR(&ntpMux);
    rtc = t;
    portEXIT_CRITICAL_ISR(&ntpMux);
    //
    mtick++;
    if (mtick >= 60) { // 1 minute
      mtick = 0;
      htick++;
      doWatchDog = ((htick % Watchdog) == 0);
      doMinute = true;
    }
    if (htick >= 60) { // 1 hour
      htick = 0;
      dtick++;
      doHour = true;
    }
    if (dtick >= 24) { // 1 day
      dtick = 0;
      doDay = true;
    }
  }
  //
  processLoRa();
  Hub.processMQTT();
  processMessagesWS();
  //
  if (doWatchDog) {
    // must publish "Online" every 10 minutes
    DEBUGln("Need publish Online");
    needPublishOnline = true;
  }
  if (doMinute) {
    doMinute = false;
    notifyDateTime();
    Hub.saveRecorder();
  }
  if (doHour) {
    doHour = false;
    vTaskResume( xHandleNTP );
  }
  if (doDay) {
    doDay = false;
  }
  if (newRTC) {
    // when NTP updated, process something
    portENTER_CRITICAL_ISR(&ntpMux);
    newRTC = false;
    portEXIT_CRITICAL_ISR(&ntpMux);
    //
    //TODO Hub.processDateTime(RL_ID_BROADCAST);
  }
  if (onBoot) {
    // At start, wait for NTP update
    if (rtc.year() > 2020) {
      onBoot = false;
      Hub.loadRecorder();
      DEBUGf("heap_caps_get_largest_free_block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
      //
      uint8_t minID = 199;
      CDevice* dev = nullptr;
      while ((dev = Hub.walkDevice(dev)))
      {
        if (dev->getAddress() < minID)
        {
          minID = dev->getAddress();
        }
      }
      MLiotComm.publishNum(RL_ID_BROADCAST, uid, RL_ID_SYNCHRO, minID);
    }
  }
}
