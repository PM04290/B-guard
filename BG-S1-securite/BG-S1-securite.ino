/*
  Designed for ESP32 S3 Mini
  Radio module : SX1278 (Ra-02)

  Wifi AP
    - default ssid : bgsecure0
    - default password : 12345678
    - default address : 192.164.4.1 (bgsecure0.local)
*/
#include <Arduino.h> // native
#include <EEPROM.h>  // native

#ifndef ESP32
#error Only designed for ESP32
#endif

#include "config.h"

#define ML_SX1278
#include <MLotaComm.h>           // GitHub project : https://github.com/PM04290/MLotaComm
#include <MLiotElements.h>       // GitHub project : https://github.com/PM04290/MLiotElements

#define commHub MLradio   // native object rename for friendly using
RadioLink commLocal(PIN_LORALAN_NSS, PIN_LORALAN_RST, PIN_LORALAN_DIO);

uint16_t FreqHub = 433;
uint8_t RangeHub = 1;
bool loraHubOK;

uint16_t FreqLocal = 434;
uint8_t RangeLocal = 0;
bool loraLocalOK;

bool onBoot = true;
bool needPairing = false;
uint32_t needSynchro = 0;

Analog* vbat;      // Battery voltage
Binary* BinAlarm1; // binary input1, e.g PIR
Binary* BinAlarm2; // binary input2
Analog* Lumi;      // Luminosoty
Analog* Temp;      // Temperature
Relay* RelayOut1;  // Relay
Regul* RegOut1;    // Relay regulation
Input* LumMin;     // Luminosity mini

#ifdef DEBUG_LED
#include <Adafruit_NeoPixel.h>
#define NUMLEDS 3
Adafruit_NeoPixel leds(NUMLEDS, PIN_DEBUG_LED, NEO_GRB + NEO_KHZ800);
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
  uint8_t module;
  rl_packets packets;
} packet_version;
packet_version packetTable[MAX_PACKET];
volatile byte idxReadTable = 0;
volatile byte idxWriteTable = 0;

// for 1 Second interrup
hw_timer_t* secTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool topSecond = false;
uint8_t cntMinute = 0;
uint8_t cntHour = 7;

#include "tools.hpp"

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

void onLoRaHubReceive(uint8_t len, rl_packet_t* p)
{
  noInterrupts();
  memcpy(&packetTable[idxWriteTable].packets.current, p, len);
  packetTable[idxWriteTable].version = 1;
  packetTable[idxWriteTable].module = 0;
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

void onLoRaLocalReceive(uint8_t len, rl_packet_t* p)
{
  noInterrupts();
  memcpy(&packetTable[idxWriteTable].packets.current, p, len);
  packetTable[idxWriteTable].version = 1;
  packetTable[idxWriteTable].module = 1;
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
    rl_packet_t* pIn = &p.packets.current;
    DEBUGf("%d => %d:%d\n", pIn->senderID, pIn->destinationID, pIn->childID);

    // for this module and not broadcast
    if (pIn->destinationID == uid)
    {
      if (needPairing && pIn->childID == RL_ID_CONFIG)
      {
        rl_conf_t cnfIdx = (rl_conf_t)(pIn->sensordataType & 0x07);
        if (cnfIdx == C_PARAM)
        {
          rl_configParam_t* pcnfp = (rl_configParam_t*)&pIn->data.configs;
          if (pcnfp->childID == 0) // 0 is value to change UID device
          {
            uid = pcnfp->pInt;
            EEPROM.put(EEP_UID, uid);
            EEPROM.commit();
            DEBUGf("New ID %d\n", uid);
          }
        }
        if (cnfIdx == C_END)
        {
          // HUB who send C_END become destination HUB
          hubid = pIn->senderID;
          EEPROM.put(EEP_HUBID, hubid);
          EEPROM.commit();
          DEBUGf("Pairing done from %d\n", hubid);
          needPairing = false;
        }
        return;
      }
      if (pIn->childID == CHILD_ID_RELAY)
      {
        RelayOut1->setBool(pIn->data.num.value);
        return;
      }
      if (pIn->childID == CHILD_ID_INPUT_LUMMIN)
      {
        LumMin->setFloat(pIn->data.num.value / pIn->data.num.divider);
        return;
      }
      if (pIn->childID == CHILD_ID_REGUL_RELAY)
      {
        char txt[sizeof(rl_packet_t)];
        strncpy(txt, pIn->data.text, sizeof(pIn->data.text));
        txt[sizeof(pIn->data.text)] = 0;
        RegOut1->setText(txt);
        return;
      }
      return;
    }
    // here is not for me, route all
    // -----------------------------
    // Every end device have internal ID > 100, but Hub konwn it less 100
    // to prevent direct communication from end device tu Hub
    // eg. Hub known ID 10, router had 100, end device is 110

    // From HUB to Device
    if ((p.module == 0) && pIn->senderID == hubid)
    {
      if (pIn->destinationID == RL_ID_BROADCAST)
      {
        if (pIn->childID == RL_ID_DATETIME)
        {
          //
          cntHour = pIn->data.rawByte[3] % 24;
          cntMinute = pIn->data.rawByte[4] % 60;
        }
        if (pIn->childID == RL_ID_SYNCHRO)
        {
          DEBUGln("Need synchro");
          // trig Syncho for me
          needSynchro = millis() + 500;
        }
        // route for all other devices
        if (loraLocalOK) {
          commLocal.publishPaquet((rl_packets*)pIn);
        }
      } else
      {
        if (loraLocalOK) {
          commLocal.publishPaquet((rl_packets*)pIn);
        }
      }
      return;
    }
    // From routed Device to HUB
    if (p.module == 1)
    {
      DEBUGf("route %d => %d ", pIn->senderID, pIn->destinationID);
      commHub.publishPaquet(&p.packets);
      DEBUGln("ok");
      return;
    }
  }
}

void setup()
{
  LED_INIT;
  LED_WHITE(64);
  DEBUGinit();
  //
  pinMode(PIN_OUT1, OUTPUT);
  digitalWrite(PIN_OUT1, LOW);

  // put LoRa LAn Module in reset mode
  pinMode(PIN_LORALAN_RST, OUTPUT);
  digitalWrite(PIN_LORALAN_RST, LOW);

  //
  delay(3000);
  LED_WHITE(128);

  heap_caps_register_failed_alloc_callback(heap_caps_alloc_failed_hook);
  DEBUGln("Start debug");

  if (!EEPROM.begin(EEPROM_MAX_SIZE))
  {
    DEBUGln("failed to initialise EEPROM");
    // critical error, freeze
    LED_RED(200); while (true);
  }

  // active "pairing"
  pinMode(PIN_CFG, INPUT_PULLUP);
  while (digitalRead(PIN_CFG) == LOW)
  {
    LED_BLUE(200); delay(millis() > 10000 ? 100 : 300);
    LED_OFF; delay(millis() > 10000 ? 100 : 300);
    needPairing = true;
  }

  if (millis() > 10000)
  { // CFG pressed more than 10s : factory setting
    uid = 0xFF;
    for (int i = 0; i < EEPROM_MAX_SIZE; i++)
    {
      EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
  } else
  {
    EEPROM.get(EEP_UID, uid);
  }

  LED_GREEN(64);
  //
  if (uid == 0xFF) {
    uid = 90;
    hubid = 0;

    EEPROM.put(EEP_UID, uid);
    EEPROM.put(EEP_FREQ, FreqHub);
    EEPROM.put(EEP_RANGE, RangeHub);
    EEPROM.put(EEP_HUBID, hubid);

    inputRecord ir;
    ir = { 0, 5000, 100, 1};            // min:0 max:5000 sel:100 lux div:1
    EEPROM.put(EEP_INPUT_LUMMIN, ir);
    EEPROM.put(EEP_REGUL_RELAY, (int32_t)0); // manu

    EEPROM.commit();
    needPairing = true;
  }

  EEPROM.get(EEP_FREQ, FreqHub);
  if (FreqHub == 0xFFFF) FreqHub = 433;
  EEPROM.get(EEP_RANGE, RangeHub);
  RangeHub = RangeHub % 4;
  EEPROM.get(EEP_HUBID, hubid);
  if (hubid > 9) hubid = 0;

  DEBUGf("UID:%d\n", uid);
  DEBUGf("Freq:%d\n", FreqHub);
  DEBUGf("Range:%d\n", RangeHub);
  DEBUGf("hubID:%d\n", hubid);

  analogReadResolution(12);        // 12 bits (0-4095)
  analogSetAttenuation(ADC_11db);  // 0-3.3V

  vbat = (Analog*)deviceManager.addElement(new Analog(PIN_MES_VBAT, CHILD_ID_VBAT, F("Vbat"), F("V"), 0.1, 10));
  vbat->setParams(onAnalogVbat, 0, 0, 0);

  BinAlarm1 = (Binary*)deviceManager.addElement(new Binary(PIN_IN3, CHILD_ID_BINSENSOR_1, F("Alarm1"), stateInverted, nullptr));

  BinAlarm2 = (Binary*)deviceManager.addElement(new Binary(PIN_IN2, CHILD_ID_BINSENSOR_2, F("Alarm2"), stateInverted, nullptr));

  Temp = (Analog*)deviceManager.addElement(new Analog(PIN_IN1, CHILD_ID_SENSOR_TEMP, F("Temp"), F("°"), 3, 10));
  Temp->setParams(onAnalogNTC, 3950, 4700, 5000); // set Thermistor parameters (Beta, Resistor, Thermistor)

  Lumi = (Analog*)deviceManager.addElement(new Analog(PIN_LDR, CHILD_ID_SENSOR_LUM, F("Lumi"), F("lux"), 5, 1));
  Lumi->setParams(onAnalogLumi, 10000, 0, 0); // 10K resistor (vcc side)

  RelayOut1 = (Relay*)deviceManager.addElement(new Relay(PIN_OUT1, CHILD_ID_RELAY, F("Relay")));

  LumMin = (Input*)deviceManager.addElement(new Input(CHILD_ID_INPUT_LUMMIN, EEP_INPUT_LUMMIN, F("Lumi min"), F("lux")));

  RegOut1 = (Regul*)deviceManager.addElement(new Regul(CHILD_ID_REGUL_RELAY, EEP_REGUL_RELAY, Lumi, RelayOut1, LumMin, nullptr, ON_inner, F("Out mode")));

  loraHubOK = commHub.begin(FreqHub * 1E6, onLoRaHubReceive, 20, RangeHub);
  if (loraHubOK)
  {
    DEBUGf("LoRa Hub ok at %dMHz (range %d)\n", FreqHub, RangeHub);
    //
    if (needPairing)
    { // send pairing configuration and wait acknowledge
      hubid = RL_ID_BROADCAST;
      deviceManager.publishConfigElements(F("Secure"), F("BG-S1"));
      while (needPairing)
      { // wait for receive acknowledge from HUB (see "onReceive")
        int32_t tick = millis() / 3;
        LED_BLUE(abs((tick % 511) - 255));
        processLoRa();
      }
      LED_OFF;
    } else {
      LED_GREEN(255); delay(300); LED_OFF;
    }
  } else {
    DEBUGln("LoRa Hub ERROR");
    while (true)
    {
      LED_RED(255); delay(100); LED_OFF; delay(400);
    }
  }

  // LoRa 2
  loraLocalOK = commLocal.begin(FreqLocal * 1E6, onLoRaLocalReceive, 15, RangeLocal);
  if (loraLocalOK) {
    DEBUGf("LoRa Local ok at %dMHz (range %d)\n", FreqLocal, RangeLocal);
  } else {
    DEBUGln("LoRa Local ERROR");
    for (int i = 0; i < 2; i++)
    {
      LED_RED(255); delay(100); LED_OFF; delay(150);
      LED_RED(255); delay(100); LED_OFF; delay(150);
    }
  }

  // define time ISR
  secTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(secTimer, &onTimer, true);
  timerAlarmWrite(secTimer, 1000000, true);
  timerAlarmEnable(secTimer);
  //
  DEBUGf("heap_caps_get_largest_free_block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

void loop()
{
  static uint32_t ntick = 0;
  bool doMinute = false;
  bool doHour = false;
  bool doDay = false;
  bool forceSending = false;

  if (topSecond)
  {
    portENTER_CRITICAL(&timerMux);
    topSecond = false;
    portEXIT_CRITICAL(&timerMux);
    //
    ntick++;
    if (ntick >= 60) { // 1 minute
      ntick = 0;
      cntMinute++;
      doMinute = true;
    }
    if (cntMinute >= 60) { // 1 Hour
      cntMinute = 0;
      cntHour++;
      doHour = true;
    }
    if (cntHour >= 24) { // 1 day
      cntHour = 0;
      doDay = true;
    }
  }
  if (needSynchro && (millis() > needSynchro))
  {
    needSynchro = 0;
    forceSending = true;
  }
  processLoRa();
  deviceManager.processElements();
  deviceManager.sendElements(forceSending);
  //
  if (doMinute) {
    doMinute = false;
  }
  if (doHour) {
    doHour = false;
  }
  if (doDay) {
    doDay = false;
  }
  if (onBoot) {
    onBoot = false;
    //commLocal.publishNum(RL_ID_BROADCAST, hubid, RL_ID_SYNCHRO, 10);
  }
}
