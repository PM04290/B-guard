/*
  Version 1.3
  Designed for ATTiny3216 (internal oscillator 4MHz)
  Hardware : GB-R1 ruche v1.0 (min)

  ##########################################

*/
//                 ATtiny3216
//                   _____
//            VDD  1|*    |20 GND
// (nSS)  PA4  0~  2|     |19 16~ PA3 (SCK)
// (IN1)  PA5  1~  3|     |18 15  PA2 (MISO)
// (DIO0) PA6  2   4|     |17 14  PA1 (MOSI)
// (nRST) PA7  3   5|     |16 17  PA0 (UPDI)
// (IN3)  PB5  4   6|     |15 13  PC3 (IN4)
// (IN2)  PB4  5   7|     |14 12  PC2 (LEDs)
// (RXD)  PB3  6   8|     |13 11~ PC1 (OUT1)
// (TXD)  PB2  7~  9|     |12 10~ PC0 (OUT2)
// (SDA)  PB1  8~ 10|_____|11  9~ PB0 (SCL)

//                  MLT3216
//                   _____
//            VDD  1| [O] |20 GND
// (IN1)      PA5  2|     |19 PA3     (SCK)
// (IN2)      PB4  3|     |18 PA2     (MISO)
// (IN3)      PB5  4|     |17 PA1     (MOSI)
// (IN4)      PC3  5|     |16 PA6     (DIO0)
// (RX)       PB3  6|     |15 PA4     (NSS)
// (TX)       PB2  7|     |14 PA7     (RST)
// (SDA)      PB1  8|     |13 PC2     (LEDs)
// (SCL)      PB0  9|     |12 PC1     (OUT1)
// (UPDI)     PA0 10|_____|11 PC0     (OUT2)

#define DEBUG_LED
#define DEBUG_SERIAL

// uncomment the line below to activate Weight sensor
#define WEIGHT 7802

#include <Arduino.h>
#include <EEPROM.h>

//--- I/O pin ---
#define PIN_IN1       PIN_PA5
#define PIN_IN2       PIN_PB4
#define PIN_IN3       PIN_PB5
#define PIN_IN4       PIN_PC3
#define PIN_OUT1      PIN_PC1
#define PIN_OUT2      PIN_PC0
#define PIN_RX        PIN_PB3
#define PIN_TX        PIN_PB2
#define PIN_SDA       PIN_PB1
#define PIN_SCL       PIN_PB0
#define PIN_DEBUG_LED PIN_PC2

#ifdef DEBUG_SERIAL
#include <SoftwareSerial.h>
SoftwareSerial DSerial(PIN_RX, PIN_TX); // (RX, TX)
#endif

#define ML_SX1278
#include <MLiotComm.h>
#include <MLiotElements.h>

// EEPROM dictionnary
#define EEP_UID      0
#define EEP_HUBID    (EEP_UID+1)
#define EEP_WREF     (EEP_HUBID+1)
#define EEP_TARE     (EEP_WREF+sizeof(inputRecord))
#define EEP_CALIB    (EEP_TARE+sizeof(int32_t))


// LoRa
extern uint8_t uid;
extern uint8_t hubid;

const uint32_t LRfreq = 433;
const uint8_t  LRrange = 1;
bool LoraOK = false;
bool needPairing = false;
bool pairingPending = false;
uint32_t needSynchro = 0;

bool needTare = false;
bool needCalibration = false;

// buffer table for LoRa incoming packet
#define MAX_PACKET 20
typedef struct __attribute__((packed))
{
  uint8_t version;
  int lqi;
  rl_packets packets;
} packet_version;
packet_version packetTable[MAX_PACKET];
volatile byte idxReadTable = 0;
volatile byte idxWriteTable = 0;

// RadioLink IDs
#define CHILD_ID_VBAT        1
#define CHILD_ID_INPUT1      2
#define CHILD_ID_INPUT2      3
#define CHILD_ID_INPUT3      4
#define CHILD_ID_TILT        5
#define CHILD_ID_WEIGHT      6
#define CHILD_ID_SCALE_TARE  7
#define CHILD_ID_WEIGHTREF   8
#define CHILD_ID_SCALE_CALIB 9

//--- debug tools ---

#ifdef DEBUG_LED
#define LED_INIT pinMode(PIN_DEBUG_LED, OUTPUT)
#define LED_ON   digitalWrite(PIN_DEBUG_LED, HIGH)
#define LED_OFF  digitalWrite(PIN_DEBUG_LED, LOW)
#define LED_PWM(x) analogWrite(PIN_DEBUG_LED, x);
#else
#define LED_INIT
#define LED_ON
#define LED_OFF
#define LED_PWM(x)
#endif

//############################
//## Equipment dictionnary ###
//############################

extern uint16_t mVCC;

volatile uint16_t sleepCounter = 0;

const uint16_t SLICE_MS = 200;
const uint8_t NUM_SLICES = 10; // 2s / 200ms

volatile uint16_t slices[NUM_SLICES] = {0};
volatile uint8_t currentSlice = 0;
volatile uint32_t totalSlice = 0;

Analog* HiveTemp;
Button* ScaleTare = nullptr;
Button* ScaleCalib = nullptr;
Input* ScaleWeightRef = nullptr;

#if (WEIGHT == 7802)
#include "NAU7802.hpp"
NAU7802 BGR2_scale;
#endif

int32_t scaleTare = 0;
float scaleCalibration = 1.;

ISR(RTC_PIT_vect)
{
  RTC.PITINTFLAGS = RTC_PI_bm; // Clear interrupt flag by writing '1' (required)
  sleepCounter++;
}

ISR(TCA0_OVF_vect) {
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // Clear flag

  currentSlice = (currentSlice + 1) % NUM_SLICES;
  slices[currentSlice] = 0;

  totalSlice = 0;
  for (uint8_t i = 0; i < NUM_SLICES; i++) {
    totalSlice += slices[i];
  }
  if (totalSlice > 100) {
    sleepCounter = 0x1FFF;
  }
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
    rl_packet_t* pIn = &p.packets.current;
    DEBUG(pIn->senderID); DEBUG(" => "); DEBUG(pIn->destinationID); DEBUG(":"); DEBUGln(pIn->childID);
    if (pIn->destinationID == RL_ID_BROADCAST)
    {
      if ((pIn->senderID == hubid) && (pIn->childID == RL_ID_SYNCHRO))
      {
        uint8_t minID = pIn->data.num.value;                // Hub send lower ID off network
        uint8_t delta = uid - minID;                        // position from mini ID
        uint16_t packetTime[] = { 30, 200, 700, 1500 };     // busy time of one packet for range
        uint32_t sendingTime = packetTime[LRrange % 4] * 9;
        needSynchro = (millis() + 2000 + (sendingTime * delta)) | 1;
      }
      return;
    }
    if (pIn->destinationID == uid)
    {
      if (pairingPending && pIn->childID == RL_ID_CONFIG)
      {
        rl_conf_t cnfIdx = (rl_conf_t)(pIn->sensordataType & 0x07);
        if (cnfIdx == C_PARAM)
        {
          rl_configParam_t* pcnfp = (rl_configParam_t*)&pIn->data.configs;
          if (pcnfp->childID == 0) // 0 is value to change UID device
          {
            uid = pcnfp->pInt;
            EEPROM.put(EEP_UID, uid);
            DEBUG("New ID "); DEBUGln(uid);
          }
        }
        if (cnfIdx == C_END)
        {
          // HUB who send C_END become destination HUB
          hubid = pIn->senderID;
          EEPROM.put(EEP_HUBID, hubid);
          pairingPending = false;
        }
        return;
      }
      if (ScaleWeightRef && pIn->childID == CHILD_ID_WEIGHTREF)
      {
        float f = (float)pIn->data.num.value / (float)pIn->data.num.divider;
        ScaleWeightRef->setFloat(f);
        return;
      }
      if (ScaleTare && (pIn->childID == CHILD_ID_SCALE_TARE))
      {
        needTare = true;
        DEBUGln("Set tare");
        return;
      }
      if (ScaleWeightRef && pIn->childID == CHILD_ID_SCALE_CALIB)
      {
        needCalibration = true;
        DEBUGln("Set calibration");
        return;
      }
    }
  }
}
void onTilt()
{
  slices[currentSlice]++;
}

void onPinWakeup()
{
  sleepCounter = 0x1FFF;
}

// initial setup
void setup()
{
  DEBUGinit();
  DEBUGln(F("\nStart"));
  LED_INIT;

  // use nRST pin from LoRa module to active "pairing"
  pinMode(RL_DEFAULT_RESET_PIN, INPUT);
  while (analogRead(RL_DEFAULT_RESET_PIN) < 100)
  {
    LED_ON; delay(millis() > 10000 ? 100 : 300);
    LED_OFF; delay(millis() > 10000 ? 100 : 300);
    needPairing = true;
  }

  if (millis() > 10000)
  { // CFG pressed more than 10s : factory setting
    uid = 0xFF;
  } else
  {
    uid = EEPROM.read(EEP_UID);
    DEBUG("UID "); DEBUGln(uid);
  }

  LED_ON;
  if (uid == 0xFF)
  { // blank EEPROM (factory settings)
    DEBUGln(F("Initialize EEPROM"));
    uid = 30;
    hubid = 0;

    // update
    EEPROM.put(EEP_UID, uid);
    EEPROM.put(EEP_HUBID, hubid);
    inputRecord ir;
    ir = { 10, 500, 150, 10};  // min:10 max:500 sel:100 lux div:10
    EEPROM.put(EEP_WREF, ir);
    EEPROM.put(EEP_TARE, scaleTare);
    EEPROM.put(EEP_CALIB, scaleCalibration);

    needPairing = true;
  }
  //  scaleCalibration = 5445;  EEPROM.put(EEP_CALIB, scaleCalibration);

  EEPROM.get(EEP_HUBID, hubid);
  EEPROM.get(EEP_TARE, scaleTare);
  EEPROM.get(EEP_CALIB, scaleCalibration);

  DEBUG("huid "); DEBUGln(hubid);
  DEBUG("tare "); DEBUGln(scaleTare);
  DEBUG("factor "); DEBUGln(scaleCalibration);

  // Internal VCC
  Analog* vcc = (Analog*)deviceManager.addElement(new Analog(PIN_NONE, CHILD_ID_VBAT, F("Vcc"), F("V"), 0.01, 100));
  vcc->setParams(onAnalogVCC, 0, 0, 0);

  // Input1 : Opening sensor
  deviceManager.addElement(new Binary(PIN_IN1, CHILD_ID_INPUT1, F("Open"), stateInverted, nullptr));
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), onPinWakeup, CHANGE);

  // Input 2 : Temperature sensor
  Analog* HiveTemp = (Analog*)deviceManager.addElement(new Analog(PIN_IN2, CHILD_ID_INPUT2, F("Temperature"), F("°C"), 0.2, 10));
  HiveTemp->setParams(onAnalogNTC, 3950, 4700, 5000);// set Thermistor parameters (Beta, Resistor, Thermistor)

  // Input 3
  // free

  // Inpur 4 : Internal Tilt (count change during short time)
  Analog* Tilt = (Analog*)deviceManager.addElement(new Analog(PIN_NONE, CHILD_ID_TILT, F("Tilt"), nullptr, 1, 1));
  Tilt->setParams(onAnalogTilt, 0, 0, 0);
  pinMode(PIN_IN4, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_IN4), onTilt, CHANGE);

  // NAU7802 (on I2C)
  /*
      Red wire to HX711/NAU7802
      [E+]   white   [A-]
      +  +-----------+  +
      |                 |
    black |                 | black
      |                 |
      +  +-----------+  +
      [A+]   white   [E-]
  */
#if (WEIGHT == 7802)
  Wire.begin();
  Wire.setClock(400000); //Qwiic Scale is capable of running at 400kHz if desired
  if (BGR2_scale.begin())
  {
    Analog* HiveWeight = (Analog*)deviceManager.addElement(new Analog(PIN_NONE, CHILD_ID_WEIGHT, F("Weight"), F("kg"), 0.1, 10));
    HiveWeight->setParams(onReadNAU7802, 0, 0, 0);// set parameters (none, none, none)
    ScaleTare = (Button*)deviceManager.addElement(new Button(CHILD_ID_SCALE_TARE, F("Tare")));
    ScaleWeightRef = (Input*)deviceManager.addElement(new Input(CHILD_ID_WEIGHTREF, EEP_WREF, F("Weight ref"), F("kg")));
    ScaleCalib = (Button*)deviceManager.addElement(new Button(CHILD_ID_SCALE_CALIB, F("Calib")));
    BGR2_scale.setZeroOffset(scaleTare);
    BGR2_scale.setCalibrationFactor(scaleCalibration);
    BGR2_scale.getWeight(true, 20);
    DEBUGln("Scale OK");
  } else
  {
    DEBUGln("Scale error");
  }
#endif

  // start LoRa module
  LoraOK = MLiotComm.begin(LRfreq * 1E6, onLoRaReceive, NULL, 14, LRrange);
  if (LoraOK)
  {
    DEBUGln(F("LoRa OK"));
    if (needPairing)
    { // send pairing configuration and wait acknowledge
      hubid = RL_ID_BROADCAST;
      deviceManager.publishConfigElements(F("Ruche"), F("BG-R1"));
      pairingPending = true;
      while (pairingPending)
      { // wait for receive acknowledge from HUB
        uint32_t tick = millis() / 3;
        LED_PWM(abs((tick % 511) - 255));
        processLoRa();
      }
    }
  } else
  {
    DEBUGln(F("LoRa Error"));
#ifdef DEBUG_LED
    // To show LoRa error on H1 LED
    for (byte b = 0; b < 3; b++)
    {
      LED_ON; delay(100); LED_OFF; delay(100);
    }
#endif
  }


  // init RTC Timer
  cli();

  while (RTC.STATUS > 0) {}                // Wait for all register to be synchronized
  RTC.CLKSEL      = RTC_CLKSEL_INT32K_gc;  // 32.768kHz Internal Ultra-Low-Power Oscillator (OSCULP32K)
  RTC.PITINTCTRL  = RTC_PI_bm;             // PIT Interrupt: enabled
  RTC.PITCTRLA    = RTC_PERIOD_CYC32768_gc // RTC Clock Cycles 32768, resulting in 32.768kHz/32768 = 1Hz
                    | RTC_PITEN_bm;        // Enable PIT counter

  // init TiltTimer
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc; // Prescaler
  TCA0.SINGLE.PER = (F_CPU / 1024 / (1000 / SLICE_MS)) - 1;
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;          // Activer interruption overflow
  TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;        // Activer TCA0

  sei();

  LED_OFF;
  DEBUGln(F("Setup done"));
  sleepCounter = 0x1FFF;
  needTare = false;
  needCalibration = false;
}

void loop()
{
  bool isSynchro = false;

  processLoRa();
  if (needSynchro && (millis() > needSynchro))
  {
    isSynchro = true;
  }
  if (needTare)
  {
    LED_ON;
    BGR2_scale.powerUp();
    BGR2_scale.calculateZeroOffset();
    scaleTare = BGR2_scale.getZeroOffset();
    EEPROM.put(EEP_TARE, scaleTare);
    DEBUGln(scaleTare);
    BGR2_scale.powerDown();
    LED_OFF;
    DEBUGln("Tare done");
    //
    needTare = false;
  }
  if (needCalibration)
  {
    LED_ON;
    BGR2_scale.powerUp();
    BGR2_scale.calculateCalibrationFactor(ScaleWeightRef->getFloat());
    scaleCalibration = BGR2_scale.getCalibrationFactor();
    EEPROM.put(EEP_CALIB, scaleCalibration);
    DEBUGln(scaleCalibration);
    BGR2_scale.powerDown();
    LED_OFF;
    DEBUGln("Calibration done");
    //
    needCalibration = false;
  }

  if (LoraOK && ((sleepCounter >= 30) || isSynchro))
  {
    LED_ON;
    sleepCounter = 0;
    ADC0.CTRLA |= ADC_ENABLE_bm;  // enable ADC
    BGR2_scale.powerUp();
    // Walk any sensor in Module to send data
    deviceManager.processElements();
    deviceManager.sendElements(isSynchro);
    //
    needSynchro = 0;
    //
    BGR2_scale.powerDown();
    ADC0.CTRLA &= ~ADC_ENABLE_bm;  // disable ADC
    LED_OFF;
  }
}

float onAnalogVCC(uint16_t adcRAW, uint16_t p1, uint16_t p2, uint16_t p3)
{
  analogReference(VDD);
  VREF.CTRLA = VREF_ADC0REFSEL_1V5_gc;
  // there is a settling time between when reference is turned on, and when it becomes valid.
  // since the reference is normally turned on only when it is requested, this virtually guarantees
  // that the first reading will be garbage; subsequent readings taken immediately after will be fine.
  // VREF.CTRLB|=VREF_ADC0REFEN_bm;
  // delay(10);
  uint16_t reading = analogRead(ADC_INTREF);
  reading = analogRead(ADC_INTREF);
  uint32_t intermediate = 1023UL * 1500;
  mVCC = intermediate / reading;
  return mVCC / 1000.;
}

float onAnalogTilt(uint16_t adcRAW, uint16_t p1, uint16_t p2, uint16_t p3)
{
  return totalSlice;
}

#define T_REF       298.15 // nominal temperature (Kelvin) (25°)
float onAnalogNTC(uint16_t adcNTC, uint16_t beta, uint16_t resistor, uint16_t thermistor)
{
  float celcius = NAN;
  // ! formula with NTC on GND
  float r_ntc = (float)resistor / (1023. / (float)adcNTC - 1.);
  celcius = r_ntc / (float)thermistor;
  celcius = log(celcius);
  celcius /= beta;
  celcius += 1.0 / T_REF;
  celcius = 1.0 / celcius;
  celcius -= 273.15;
  return celcius;
}

#if (WEIGHT == 7802)
float onReadNAU7802(uint16_t pinData, uint16_t p1, uint16_t p2, uint16_t p3)
{
  for (int i = 0; i < 4; i++)
  {
    if (BGR2_scale.available() == true)
    {
      return BGR2_scale.getWeight();
    }
    delay(100);
  }
  return -99.9;
}
#endif
