#pragma once

#define VERSION "1.0"

#define DEBUG_SERIAL
#define DSerial Serial

#define DEBUG_LED

#if defined(ARDUINO_LOLIN_S2_MINI)
#define PIN_WMODE 13 // MLH1
#elif defined(ARDUINO_ESP32_POE_ISO) || defined(ARDUINO_ESP32_POE)
#define PIN_WMODE 32 // MLH2
#elif defined(ARDUINO_WT32_ETH01)
#define PIN_WMODE 32 // MLH3
#elif defined(ARDUINO_D1_MINI32)
#define PIN_WMODE 32 // MLH4
#elif defined(ARDUINO_TTGO_LoRa32_v21new)
#define PIN_WMODE  4 // MLH5
#else
#define PIN_WMODE 32 // custon ESP32 WROOM
#endif

#define DEFAULT_RETRY_NTP 10
#define DEFAULT_ONLINE_WATCHDOG 10 // in min

#define EEPROM_MAX_SIZE    512

#define EEPROM_DATA_UID      0
#define EEPROM_DATA_FREQ     (EEPROM_DATA_UID+1)
#define EEPROM_DATA_RANGE    (EEPROM_DATA_FREQ+2)

#define EEPROM_TEXT_OFFSET  16
#define EEPROM_TEXT_SIZE    48

#ifdef DEBUG_SERIAL
#define DEBUGinit() DSerial.begin(115200)
#define DEBUG(x) DSerial.print(x)
#define DEBUGln(x) DSerial.println(x)
#define DEBUGf(...) DSerial.printf(__VA_ARGS__)
#else
#define DEBUGinit()
#define DEBUG(x)
#define DEBUGln(x)
#define DEBUGf(x,y)
#endif

char Wifi_ssid[EEPROM_TEXT_SIZE+1] = "";  // WiFi SSID
char Wifi_pass[EEPROM_TEXT_SIZE+1] = "";  // WiFi password

char mqtt_host[EEPROM_TEXT_SIZE] = "";  // MQTT host eg. 192.168.0.100
char mqtt_user[EEPROM_TEXT_SIZE] = "";  // MQTT user
char mqtt_pass[EEPROM_TEXT_SIZE] = "";  // MQTT password
uint16_t mqtt_port = 1883;

char datetimeTZ[EEPROM_TEXT_SIZE] = "CET-1CEST,M3.5.0,M10.5.0/3";
char datetimeNTP[EEPROM_TEXT_SIZE] = "europe.pool.ntp.org";
char defaultNTP[EEPROM_TEXT_SIZE] = "pool.ntp.org";

char smsUser[EEPROM_TEXT_SIZE] = "12594858";
char smsPass[EEPROM_TEXT_SIZE] = "w2IkpiLFgQRbxn";

char AP_ssid[10] = "bghub0";   // AP WiFi SSID
char AP_pass[9] = "12345678";  // AP WiFi password

// Home Assistant
char HAuniqueId[13];

const char HAConfigRoot[]                PROGMEM = {"homeassistant"};
const char HAlora2ha[]                   PROGMEM = {"lora2ha"};

const char HAComponentBinarySensor[]     PROGMEM = {"binary_sensor"};
const char HAComponentSensor[]           PROGMEM = {"sensor"};
const char HAComponentNumber[]           PROGMEM = {"number"};
const char HAComponentSelect[]           PROGMEM = {"select"};
const char HAComponentSwitch[]           PROGMEM = {"switch"};
const char HAComponentTag[]              PROGMEM = {"tag"};
const char HAComponentDeviceAutomation[] PROGMEM = {"device_automation"};

const char HAIdentifiers[]       PROGMEM = {"ids"};
const char HAManufacturer[]      PROGMEM = {"mf"};
const char HAModel[]             PROGMEM = {"mdl"};
const char HASwVersion[]         PROGMEM = {"sw"};
const char HAViaDevice[]         PROGMEM = {"via_device"};
const char HAUniqueID[]          PROGMEM = {"unique_id"};
const char HAObjectID[]          PROGMEM = {"objt_id"};
const char HAName[]              PROGMEM = {"name"};
const char HATopic[]             PROGMEM = {"topic"};
const char HAPayload[]           PROGMEM = {"payload"};
const char HAState[]             PROGMEM = {"state"};
const char HAStateTopic[]        PROGMEM = {"stat_t"};
const char HAStateOpen[]         PROGMEM = {"stat_open"};
const char HAStateOpening[]      PROGMEM = {"stat_opening"};
const char HAStateClosed[]       PROGMEM = {"stat_closed"};
const char HAStateClosing[]      PROGMEM = {"stat_closing"};
const char HAStateStopped[]      PROGMEM = {"stat_stopped"};
const char HAPayloadOpen[]       PROGMEM = {"pl_open"};
const char HAPayloadStop[]       PROGMEM = {"pl_stop"};
const char HAPayloadClose[]      PROGMEM = {"pl_close"};
const char HACommand[]           PROGMEM = {"command"};
const char HACommandTopic[]      PROGMEM = {"cmd_t"};
const char HADeviceClass[]       PROGMEM = {"dev_cla"};
const char HAEntityCategory[]    PROGMEM = {"ent_cat"};
const char HAUnitOfMeasurement[] PROGMEM = {"unit_of_meas"};
const char HAStateNone[]         PROGMEM = {"none"};
const char HAStateOn[]           PROGMEM = {"ON"};
const char HAStateOff[]          PROGMEM = {"OFF"};
const char HAExpireAfter[]       PROGMEM = {"exp_aft"};
const char HAMin[]               PROGMEM = {"min"};
const char HAMax[]               PROGMEM = {"max"};
const char HAIcon[]              PROGMEM = {"icon"};
