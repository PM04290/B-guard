#pragma once

#define VERSION "1.2"

#define DEBUG_SERIAL
#define DSerial Serial

#define DEBUG_LED

#define PIN_IN1      16
#define PIN_IN2      18
#define PIN_IN3      34
#define PIN_LED      21
#define PIN_MES_VBAT 33
#define PIN_OUT1     43
#define PIN_WMODE    44

#define CHILD_ID_VBAT         1
#define CHILD_ID_BINSENSOR_1  2
#define CHILD_ID_BINSENSOR_2  3
#define CHILD_ID_SENSOR_LUM   4
#define CHILD_ID_SENSOR_TEMP  5
#define CHILD_ID_RELAY        6
#define CHILD_ID_INPUT_LUMMIN 7
#define CHILD_ID_REGUL_RELAY  8

#define DEFAULT_RETRY_NTP 10

#define EEPROM_MAX_SIZE    512

#define EEPROM_DATA_UID      0
#define EEPROM_DATA_FREQ     (EEPROM_DATA_UID+1)
#define EEPROM_DATA_RANGE    (EEPROM_DATA_FREQ+2)
#define EEPROM_DATA_HUBID    (EEPROM_DATA_RANGE+1)
#define EEPROM_DATA_ROUTEID  (EEPROM_DATA_HUBID+1)
#define EEP_INPUT_LUMMIN     (EEPROM_DATA_ROUTEID+1)
#define EEP_REGUL_RELAY      (EEP_INPUT_LUMMIN+sizeof(inputRecord))

#define EEPROM_TEXT_OFFSET  16
#define EEPROM_TEXT_SIZE    48

char Wifi_ssid[EEPROM_TEXT_SIZE+1] = "";  // WiFi SSID
char Wifi_pass[EEPROM_TEXT_SIZE+1] = "";  // WiFi password

char AP_ssid[10] = "bgsecure0";  // AP WiFi SSID
char AP_pass[9] = "12345678";  // AP WiFi password

bool loraOK;
JsonDocument docJson;

uint8_t routeID = 0;
uint16_t RadioFreq = 433;
uint8_t RadioRange = 1;
bool pairToHubPending = false;
bool pairModulePending = false;
