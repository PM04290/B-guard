#pragma once

#define VERSION "1.1"

#define DEBUG_SERIAL
#define DSerial Serial

#define DEBUG_LED

#define PIN_MES_VBAT  33
#define PIN_IN1       16
#define PIN_IN2       18
#define PIN_IN3       34
#define PIN_LDR       38
#define PIN_OUT1       7
#define PIN_OUT2       8
#define PIN_WMODE     44
#define PIN_CFG       43
#define PIN_DEBUG_LED 21

#define PIN_LORALAN_RST 6
#define PIN_LORALAN_DIO 4
#define PIN_LORALAN_NSS 9

#define CHILD_ID_VBAT         1
#define CHILD_ID_BINSENSOR_1  2
#define CHILD_ID_BINSENSOR_2  3
#define CHILD_ID_SENSOR_LUM   4
#define CHILD_ID_SENSOR_TEMP  5
#define CHILD_ID_RELAY        6
#define CHILD_ID_INPUT_LUMMIN 7
#define CHILD_ID_REGUL_RELAY  8

#define EEPROM_MAX_SIZE    512

#define EEP_UID           0
#define EEP_FREQ          (EEP_UID+1)
#define EEP_RANGE         (EEP_FREQ+2)
#define EEP_HUBID         (EEP_RANGE+1)
#define EEP_INPUT_LUMMIN  (EEP_HUBID+1)
#define EEP_REGUL_RELAY   (EEP_INPUT_LUMMIN+sizeof(inputRecord))

#define EEPROM_TEXT_OFFSET  32
#define EEPROM_TEXT_SIZE    48
