/*
  Designed for ESP32 wroom
  Radio module : SX1278 (Ra-01 / Ra-02)

  Wifi AP
    - default ssid : bgsecure0
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
#include <ArduinoJson.h>         // available in library manager
#include <ESPAsyncWebServer.h>   // https://github.com/me-no-dev/ESPAsyncWebServer (don't get fork in library manager)
                                 // https://github.com/me-no-dev/AsyncTCP
#include "config.h"

#define ML_SX1278
//#define ML_MAX485_PIN_RX PIN_485RX
//#define ML_MAX485_PIN_TX PIN_485TX
//#define ML_MAX485_PIN_DE PIN_485DE
#include <MLiotComm.h>           // GitHub project : https://github.com/PM04290/MLiotComm
#include <MLiotElements.h>       // GitHub project : https://github.com/PM04290/MLiotElements

#include "tools.hpp"
#include "devices.hpp"

Analog* vbat;      // Battery voltage
Binary* BinAlarm1; // binary input1, e.g PIR
Binary* BinAlarm2; // binary input2
Analog* Lumi;      // Luminosoty
Relay* RelayOut1;  // Relay
Regul* RegOut1;    // Relay regulation
Input* LumMin;     // Luminosity mini

#include "webserver.hpp"

#ifdef DEBUG_LED
#include <Adafruit_NeoPixel.h>
#define NUMLEDS 3
Adafruit_NeoPixel leds(NUMLEDS, PIN_LED, NEO_GRB + NEO_KHZ800);
#define LED_INIT {leds.begin(); leds.setBrightness(10);}
#define LED_RED(x) {leds.setPixelColor(0, leds.Color(x, 0, 0)); leds.show();}
#define LED_GREEN(x) {leds.setPixelColor(0, leds.Color(0, x, 0)); leds.show();}
#define LED_BLUE(x) {leds.setPixelColor(0, leds.Color(0, 0, x)); leds.show();}
#define LED_WHITE(x) {leds.setPixelColor(0, leds.Color(x, x, x)); leds.show();}
#define LED_OFF {leds.setPixelColor(0, leds.Color(0, 0, 0)); leds.show();}
#else
#define LED_INIT
#define LED_RED(x)
#define LED_GREEN(x)
#define LED_BLUE(x)
#define LED_WHITE(x)
#define LED_OFF
#endif

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
    DEBUGf("%d => %d:%d\n", cp->senderID, cp->destinationID, cp->childID);
    if ((cp->senderID == hubid) && (cp->destinationID == uid))
    {
      // From Hub to Me
      if (pairToHubPending && cp->childID == RL_ID_CONFIG)
      {
        rl_conf_t cnfIdx = (rl_conf_t)(cp->sensordataType & 0x07);
        if (cnfIdx == C_PARAM)
        {
          /* TODO
            rl_configParam_t* pcnfp = (rl_configParam_t*)&cp->data.configs;
            if (pcnfp->childID == 0)
            { // 0 to change UID device
            uid =  + pcnfp->pInt;
            EEPROM.put(EEPROM_DATA_UID, uid);
            }
          */
          return;
        }
        if (cnfIdx == C_END)
        {
          hubid = cp->senderID;
          EEPROM.put(EEPROM_DATA_HUBID, hubid);
          pairToHubPending = false;
          DEBUGf("Pairing done by HUB %d\n", hubid);
          return;
        }
      }
      if (cp->childID == CHILD_ID_RELAY)
      {
        RelayOut1->setBool(cp->data.num.value);
        return;
      }
      if (cp->childID == CHILD_ID_INPUT_LUMMIN)
      {
        LumMin->setFloat(cp->data.num.value / cp->data.num.divider);
        return;
      }
      if (cp->childID == CHILD_ID_REGUL_RELAY)
      {
        char txt[sizeof(rl_packet_t)];
        strncpy(txt, cp->data.text, sizeof(cp->data.text));
        txt[sizeof(cp->data.text)] = 0;
        RegOut1->setText(txt);
        return;
      }
    }
    if (cp->destinationID == RL_ID_BROADCAST)
    {
      // pairing
      if (pairModulePending && cp->childID == RL_ID_CONFIG && (rl_element_t)(cp->sensordataType >> 3) == E_CONFIG)
      { // config packet
        Router.pairingConfig(cp);
        return;
      }
    }
    if (cp->destinationID == hubid)
    {
      // display state
      Router.displayState(cp);
      if (routeID)
      {
        // Route from Module to Hub
        p.packets.current.destinationID = 0;
        MLiotComm.publishPaquet(&p.packets);
        return;
      }
    }
    if (cp->senderID == hubid)
    {
      if (routeID)
      {
        // Route from Hub to Module
        p.packets.current.senderID = hubid;
        MLiotComm.publishPaquet(&p.packets);
        return;
      }
    }
  }
}

void setup()
{
  Serial1.begin(115200, SERIAL_8N1, PIN_485RX, PIN_485TX);
  DEBUGinit();
  LED_INIT;
  LED_WHITE(64);
  //
  pinMode(PIN_OUT1, OUTPUT);
  digitalWrite(PIN_OUT1, LOW);
  //
  delay(3000);
  LED_WHITE(128);

  heap_caps_register_failed_alloc_callback(heap_caps_alloc_failed_hook);
  DEBUGln("Start debug");

  if (!SPIFFS.begin())
  {
    DEBUGln(F("SPIFFS Mount failed"));
    // critical error, freeze
    LED_RED(200); while (true);
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
    LED_RED(200); while (true);
  } else {
    char code = EEPROM.readChar(EEPROM_DATA_UID);
    //
    if ((byte)code == 0xFF) {
      code = '0';
      AP_ssid[strlen(AP_ssid) - 1] = code;
      EEPROM.writeChar(EEPROM_DATA_UID, code);
      EEPROM.writeUShort(EEPROM_DATA_FREQ, RadioFreq);
      EEPROM.writeByte(EEPROM_DATA_RANGE, RadioRange);
      EEPROM.writeByte(EEPROM_DATA_HUBID, (uint8_t)0);
      EEPROM.writeByte(EEPROM_DATA_ROUTEID, (uint8_t)0);

      inputRecord ir;
      ir = { 0, 5000, 100, 1};            // min:0 max:5000 sel:100 lux div:1
      EEPROM.put(EEP_INPUT_LUMMIN, ir);
      EEPROM.write(EEP_REGUL_RELAY, (int32_t)0); // manu

      EEPROM.writeString(EEPROM_TEXT_OFFSET, Wifi_ssid);
      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 1), Wifi_pass);

      EEPROM.commit();
    }
    if (code >= '0' && code <= '9')
    {
      // local router is 100 + code
      uid = 100 + (code - '0');
      AP_ssid[strlen(AP_ssid) - 1] = code;
      RadioFreq = EEPROM.readUShort(EEPROM_DATA_FREQ);
      if (RadioFreq == 0xFFFF) RadioFreq = 433;
      RadioRange = EEPROM.readByte(EEPROM_DATA_RANGE) % 4;
      hubid = EEPROM.readByte(EEPROM_DATA_HUBID);
      routeID = EEPROM.readByte(EEPROM_DATA_ROUTEID);

      strcpy(Wifi_ssid, EEPROM.readString(EEPROM_TEXT_OFFSET).c_str());
      strcpy(Wifi_pass, EEPROM.readString(EEPROM_TEXT_OFFSET + EEPROM_TEXT_SIZE * 1).c_str());

      DEBUGln(AP_ssid);
      DEBUGf("UID:%d\n", uid);
      DEBUGf("Freq:%d\n", RadioFreq);
      DEBUGf("Range:%d\n", RadioRange);
      DEBUGf("hubID:%d\n", hubid);
      DEBUGf("routeID:%d\n", routeID);
      DEBUGln(Wifi_ssid);
      DEBUGln(Wifi_pass);
    } else {
      uid = 100;
      hubid = 0;
    }
  }

  loraOK = MLiotComm.begin(RadioFreq * 1E6, onLoRaReceive, NULL, 20, RadioRange);
  //loraOK = MLiotComm.begin(115200, onLoRaReceive, NULL);
  if (loraOK)
  {
    DEBUGf("LoRa ok at %dMHz (range %d)\n", RadioFreq, RadioRange);
    //
    /*
      if (pairToHubPending)
      {
      uint8_t h = hubid;
      hubid = RL_ID_BROADCAST;
      ML_PublishConfigElements(F("BG secure"), F("BG-S1"));
      pairingPending = true;
      while (pairingPending)
      { // wait for receive acknowledge from HUB (see "onReceive")
        int32_t tick = millis() / 3;
        LED_BLUE(abs((tick % 511) - 255));
      }
      }*/
    LED_GREEN(255); delay(300); LED_OFF;
  } else {
    DEBUGln("LoRa ERROR");
    for (int i = 0; i < 3; i++)
    {
      LED_RED(255); delay(300); LED_OFF; delay(200);
    }
  }

  vbat = (Analog*)deviceManager.addElement(new Analog(PIN_MES_VBAT, CHILD_ID_VBAT, F("Vbat"), F("V"), 0.1, 10));
  vbat->setParams(onAnalogVbat, 0, 0, 0);

  BinAlarm1 = (Binary*)deviceManager.addElement(new Binary(PIN_IN1, CHILD_ID_BINSENSOR_1, F("Alarm1"), stateNormal, nullptr));
  BinAlarm2 = (Binary*)deviceManager.addElement(new Binary(PIN_IN2, CHILD_ID_BINSENSOR_2, F("Alarm2"), stateNormal, nullptr));

  Lumi = (Analog*)deviceManager.addElement(new Analog(PIN_IN3, CHILD_ID_SENSOR_LUM, F("Lumi"), F("Lux"), 5, 1));
  Lumi->setParams(onAnalogLumi, 10000, 0, 0); // 10K resistor (vcc side)

  RelayOut1 = (Relay*)deviceManager.addElement(new Relay(PIN_OUT1, CHILD_ID_RELAY, F("Relay")));
  RelayOut1->onChange(notifyRelay);

  LumMin = (Input*)deviceManager.addElement(new Input(CHILD_ID_INPUT_LUMMIN, EEP_INPUT_LUMMIN, F("Lumi min"), F("lux")));
  LumMin->onChange(notifyLumMin);

  RegOut1 = (Regul*)deviceManager.addElement(new Regul(CHILD_ID_REGUL_RELAY, EEP_REGUL_RELAY, Lumi, RelayOut1, LumMin, nullptr, ON_inner, F("Out mode")));
  RegOut1->onChange(notifyOutMode);

  Router.loadConfig();

  initNetwork();
  initWeb();

  // define time ISR
  secTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(secTimer, &onTimer, true);
  timerAlarmWrite(secTimer, 1000000, true);
  timerAlarmEnable(secTimer);
  //
  MLiotComm.publishBool(RL_ID_BROADCAST, hubid, RL_ID_SYNCHRO, true);
  //
  DEBUGf("heap_caps_get_largest_free_block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

void loop()
{
  static uint32_t ntick = 0;
  bool do24Hour = false;
  ws.cleanupClients();
  if (topSecond)
  {
    portENTER_CRITICAL(&timerMux);
    topSecond = false;
    portEXIT_CRITICAL(&timerMux);
    //
    ntick++;
    if (ntick >= 86400) { // 1 day
      ntick = 0;
      do24Hour = true;
    }
  }
  if (do24Hour) {
    do24Hour = false;
  }
  if (pairToHubPending)
  { // wait for receive acknowledge from HUB (see "onReceive")
    int32_t tick = millis() / 3;
    LED_BLUE(abs((tick % 511) - 255));
  }
  processLoRa();
  deviceManager.processElements();
  deviceManager.sendElements();
}
