/*
  This is an Arduino library written for the NAU7802 24-bit wheatstone
  bridge and load cell amplifier.
  By Nathan Seidle @ SparkFun Electronics, March 3nd, 2019

  The NAU7802 is an I2C device that converts analog signals to a 24-bit
  digital signal. This makes it possible to create your own digital scale
  either by hacking an off-the-shelf bathroom scale or by creating your
  own scale using a load cell.

  The NAU7802 is a better version of the popular HX711 load cell amplifier.
  It uses a true I2C interface so that it can share the bus with other
  I2C devices while still taking very accurate 24-bit load cell measurements
  up to 320Hz.

  https://github.com/sparkfun/SparkFun_NAU7802_Scale_Arduino_Library

  SparkFun labored with love to create this code. Feel like supporting open
  source? Buy a board from SparkFun!
  https://www.sparkfun.com/products/15242
*/

#ifndef SparkFun_Qwiic_Scale_NAU7802_Arduino_Library_h
#define SparkFun_Qwiic_Scale_NAU7802_Arduino_Library_h

#include "Arduino.h"
#include <Wire.h>

//Register Map
typedef enum
{
  NAU7802_PU_CTRL = 0x00,
  NAU7802_CTRL1,
  NAU7802_CTRL2,
  NAU7802_OCAL1_B2,
  NAU7802_OCAL1_B1,
  NAU7802_OCAL1_B0,
  NAU7802_GCAL1_B3,
  NAU7802_GCAL1_B2,
  NAU7802_GCAL1_B1,
  NAU7802_GCAL1_B0,
  NAU7802_OCAL2_B2,
  NAU7802_OCAL2_B1,
  NAU7802_OCAL2_B0,
  NAU7802_GCAL2_B3,
  NAU7802_GCAL2_B2,
  NAU7802_GCAL2_B1,
  NAU7802_GCAL2_B0,
  NAU7802_I2C_CONTROL,
  NAU7802_ADCO_B2,
  NAU7802_ADCO_B1,
  NAU7802_ADCO_B0,
  NAU7802_ADC = 0x15, //Shared ADC and OTP 32:24
  NAU7802_OTP_B1,     //OTP 23:16 or 7:0?
  NAU7802_OTP_B0,     //OTP 15:8
  NAU7802_PGA = 0x1B,
  NAU7802_PGA_PWR = 0x1C,
  NAU7802_DEVICE_REV = 0x1F,
} Scale_Registers;

//Bits within the PU_CTRL register
typedef enum
{
  NAU7802_PU_CTRL_RR = 0,
  NAU7802_PU_CTRL_PUD,
  NAU7802_PU_CTRL_PUA,
  NAU7802_PU_CTRL_PUR,
  NAU7802_PU_CTRL_CS,
  NAU7802_PU_CTRL_CR,
  NAU7802_PU_CTRL_OSCS,
  NAU7802_PU_CTRL_AVDDS,
} PU_CTRL_Bits;

//Bits within the CTRL1 register
typedef enum
{
  NAU7802_CTRL1_GAIN = 2,
  NAU7802_CTRL1_VLDO = 5,
  NAU7802_CTRL1_DRDY_SEL = 6,
  NAU7802_CTRL1_CRP = 7,
} CTRL1_Bits;

//Bits within the CTRL2 register
typedef enum
{
  NAU7802_CTRL2_CALMOD = 0,
  NAU7802_CTRL2_CALS = 2,
  NAU7802_CTRL2_CAL_ERROR = 3,
  NAU7802_CTRL2_CRS = 4,
  NAU7802_CTRL2_CHS = 7,
} CTRL2_Bits;

//Bits within the PGA register
typedef enum
{
  NAU7802_PGA_CHP_DIS = 0,
  NAU7802_PGA_INV = 3,
  NAU7802_PGA_BYPASS_EN,
  NAU7802_PGA_OUT_EN,
  NAU7802_PGA_LDOMODE,
  NAU7802_PGA_RD_OTP_SEL,
} PGA_Bits;

//Bits within the PGA PWR register
typedef enum
{
  NAU7802_PGA_PWR_PGA_CURR = 0,
  NAU7802_PGA_PWR_ADC_CURR = 2,
  NAU7802_PGA_PWR_MSTR_BIAS_CURR = 4,
  NAU7802_PGA_PWR_PGA_CAP_EN = 7,
} PGA_PWR_Bits;

//Allowed Low drop out regulator voltages
typedef enum
{
  NAU7802_LDO_2V4 = 0b111,
  NAU7802_LDO_2V7 = 0b110,
  NAU7802_LDO_3V0 = 0b101,
  NAU7802_LDO_3V3 = 0b100,
  NAU7802_LDO_3V6 = 0b011,
  NAU7802_LDO_3V9 = 0b010,
  NAU7802_LDO_4V2 = 0b001,
  NAU7802_LDO_4V5 = 0b000,
} NAU7802_LDO_Values;

//Allowed gains
typedef enum
{
  NAU7802_GAIN_128 = 0b111,
  NAU7802_GAIN_64 = 0b110,
  NAU7802_GAIN_32 = 0b101,
  NAU7802_GAIN_16 = 0b100,
  NAU7802_GAIN_8 = 0b011,
  NAU7802_GAIN_4 = 0b010,
  NAU7802_GAIN_2 = 0b001,
  NAU7802_GAIN_1 = 0b000,
} NAU7802_Gain_Values;

//Allowed samples per second
typedef enum
{
  NAU7802_SPS_320 = 0b111,
  NAU7802_SPS_80 = 0b011,
  NAU7802_SPS_40 = 0b010,
  NAU7802_SPS_20 = 0b001,
  NAU7802_SPS_10 = 0b000,
} NAU7802_SPS_Values;

//Select between channel values
typedef enum
{
  NAU7802_CHANNEL_1 = 0,
  NAU7802_CHANNEL_2 = 1,
} NAU7802_Channels;

//Calibration state
typedef enum
{
  NAU7802_CAL_SUCCESS = 0,
  NAU7802_CAL_IN_PROGRESS = 1,
  NAU7802_CAL_FAILURE = 2,
} NAU7802_Cal_Status;

//Calibration mode
typedef enum
{
  NAU7802_CALMOD_INTERNAL = 0,
  NAU7802_CALMOD_OFFSET = 2,
  NAU7802_CALMOD_GAIN,
} NAU7802_Cal_Mode;

class NAU7802 {
  public:
    NAU7802() {
    };
    bool begin(TwoWire &wirePort = Wire, bool initialize = true) {
      //Get user's options
      _i2cPort = &wirePort;

      //Check if the device ack's over I2C
      if (isConnected() == false)
      {
        //There are rare times when the sensor is occupied and doesn't ack. A 2nd try resolves this.
        if (isConnected() == false) {
          return (false);
        }
      }

      bool result = true; //Accumulate a result as we do the setup

      if (initialize)
      {
        result &= reset(); //Reset all registers

        result &= powerUp(); //Power on analog and digital sections of the scale

        result &= setLDO(NAU7802_LDO_3V3);

        result &= setGain(NAU7802_GAIN_32);

        result &= setSampleRate(NAU7802_SPS_20);

        //Turn off CLK_CHP. From 9.1 power on sequencing.
        uint8_t adc = getRegister(NAU7802_ADC);
        adc |= 0x30;
        result &= setRegister(NAU7802_ADC, adc);

        result &= setBit(NAU7802_PGA_PWR_PGA_CAP_EN, NAU7802_PGA_PWR); //Enable 330pF decoupling cap on chan 2. From 9.14 application circuit note.

        result &= clearBit(NAU7802_PGA_LDOMODE, NAU7802_PGA); //Ensure LDOMODE bit is clear - improved accuracy and higher DC gain, with ESR < 1 ohm

        delay(_ldoRampDelay); //Wait for LDO to stabilize - takes about 200ms

        getWeight(true, 10); //Flush

        result &= calibrateAFE(); //Re-cal analog front end when we change gain, sample rate, or channel
      }

      return (result);
    };

    bool isConnected() {
      _i2cPort->beginTransmission(_deviceAddress);
      if (_i2cPort->endTransmission() != 0)
        return (false); //Sensor did not ACK
      return (true);    //All good
    };

    bool available() {
      return (getBit(NAU7802_PU_CTRL_CR, NAU7802_PU_CTRL));
    };

    int32_t getReading() {
      return get24BitRegister(NAU7802_ADCO_B2);
    };

    int32_t getAverage(uint8_t averageAmount, unsigned long timeout_ms = 1000) {
      int32_t total = 0; // Readings are 24-bit. We're good to average 255 if needed
      uint8_t samplesAquired = 0;

      unsigned long startTime = millis();
      while (millis() - startTime < timeout_ms)
      {
        if (available() == true)
        {
          total += getReading();
          if (++samplesAquired == averageAmount)
            break; //All done
        }
        delay(1);
      }
      total /= averageAmount;
      return (total);
    };

    void calculateZeroOffset(uint8_t averageAmount = 8, unsigned long timeout_ms = 1000) {
      setZeroOffset(getAverage(averageAmount, timeout_ms));
    };

    void setZeroOffset(int32_t newZeroOffset) {
      _zeroOffset = newZeroOffset;
    };

    int32_t getZeroOffset() {
      return (_zeroOffset);
    };

    void calculateCalibrationFactor(float weightOnScale, uint8_t averageAmount = 8, unsigned long timeout_ms = 1000) {
      int32_t onScale = getAverage(averageAmount, timeout_ms);
      float newCalFactor = ((float)(onScale - _zeroOffset)) / weightOnScale;
      setCalibrationFactor(newCalFactor);
    };

    void setCalibrationFactor(float calFactor) {
      _calibrationFactor = calFactor;
    };

    float getCalibrationFactor() {
      return (_calibrationFactor);
    };

    float getWeight(bool allowNegativeWeights = false, uint8_t samplesToTake = 8, unsigned long timeout_ms = 1000) {
      int32_t onScale = getAverage(samplesToTake, timeout_ms);

      //Prevent the current reading from being less than zero offset
      //This happens when the scale is zero'd, unloaded, and the load cell reports a value slightly less than zero value
      //causing the weight to be negative or jump to millions of pounds
      if (allowNegativeWeights == false)
      {
        if (onScale < _zeroOffset)
          onScale = _zeroOffset; //Force reading to zero
      }

      float weight = ((float)(onScale - _zeroOffset)) / _calibrationFactor;
      return (weight);
    };

    bool setGain(uint8_t gainValue) {
      if (gainValue > 0b111)
        gainValue = 0b111; //Error check

      uint8_t value = getRegister(NAU7802_CTRL1);
      value &= 0b11111000; //Clear gain bits
      value |= gainValue;  //Mask in new bits

      return (setRegister(NAU7802_CTRL1, value));
    };

    bool setLDO(uint8_t ldoValue) {
      if (ldoValue > 0b111)
        ldoValue = 0b111; //Error check

      //Set the value of the LDO
      uint8_t value = getRegister(NAU7802_CTRL1);
      value &= 0b11000111;    //Clear LDO bits
      value |= ldoValue << 3; //Mask in new LDO bits
      setRegister(NAU7802_CTRL1, value);

      return (setBit(NAU7802_PU_CTRL_AVDDS, NAU7802_PU_CTRL)); //Enable the internal LDO
    };

    void setLDORampDelay(unsigned long delay) {
      _ldoRampDelay = delay;
    };

    unsigned long getLDORampDelay() {
      return _ldoRampDelay;
    };

    bool setSampleRate(uint8_t rate) {
      if (rate > 0b111)
        rate = 0b111; //Error check

      uint8_t value = getRegister(NAU7802_CTRL2);
      value &= 0b10001111; //Clear CRS bits
      value |= rate << 4;  //Mask in new CRS bits

      return (setRegister(NAU7802_CTRL2, value));
    };

    bool setChannel(uint8_t channelNumber) {
      if (channelNumber == NAU7802_CHANNEL_1)
        return (clearBit(NAU7802_CTRL2_CHS, NAU7802_CTRL2)); //Channel 1 (default)
      else
        return (setBit(NAU7802_CTRL2_CHS, NAU7802_CTRL2)); //Channel 2
    };

    bool calibrateAFE(NAU7802_Cal_Mode mode = NAU7802_CALMOD_INTERNAL) {
      beginCalibrateAFE(mode);
      return waitForCalibrateAFE(1000);
    };

    void beginCalibrateAFE(NAU7802_Cal_Mode mode = NAU7802_CALMOD_INTERNAL) {
      uint8_t value = getRegister(NAU7802_CTRL2);
      value &= 0xFC; // Clear CALMOD bits
      uint8_t calMode = (uint8_t)mode;
      calMode &= 0x03; // Limit mode to 2 bits
      value |= calMode; // Set the mode
      setRegister(NAU7802_CTRL2, value);

      setBit(NAU7802_CTRL2_CALS, NAU7802_CTRL2);
    };

    bool waitForCalibrateAFE(unsigned long timeout_ms = 0) {
      unsigned long startTime = millis();
      NAU7802_Cal_Status cal_ready;

      while ((cal_ready = calAFEStatus()) == NAU7802_CAL_IN_PROGRESS)
      {
        if ((timeout_ms > 0) && ((millis() - startTime) > timeout_ms))
        {
          break;
        }
        delay(1);
      }

      if (cal_ready == NAU7802_CAL_SUCCESS)
      {
        return (true);
      }
      return (false);
    };

    NAU7802_Cal_Status calAFEStatus() {
      if (getBit(NAU7802_CTRL2_CALS, NAU7802_CTRL2))
      {
        return NAU7802_CAL_IN_PROGRESS;
      }

      if (getBit(NAU7802_CTRL2_CAL_ERROR, NAU7802_CTRL2))
      {
        return NAU7802_CAL_FAILURE;
      }

      // Calibration passed
      return NAU7802_CAL_SUCCESS;
    };


    bool reset() {
      setBit(NAU7802_PU_CTRL_RR, NAU7802_PU_CTRL); //Set RR
      delay(1);
      return (clearBit(NAU7802_PU_CTRL_RR, NAU7802_PU_CTRL)); //Clear RR to leave reset state
    };

    bool powerUp() {
      setBit(NAU7802_PU_CTRL_PUD, NAU7802_PU_CTRL);
      setBit(NAU7802_PU_CTRL_PUA, NAU7802_PU_CTRL);

      //Wait for Power Up bit to be set - takes approximately 200us
      uint8_t counter = 0;
      while (1)
      {
        if (getBit(NAU7802_PU_CTRL_PUR, NAU7802_PU_CTRL) == true)
          break; //Good to go
        delay(1);
        if (counter++ > 100)
          return (false); //Error
      }
      return (setBit(NAU7802_PU_CTRL_CS, NAU7802_PU_CTRL)); // Set Cycle Start bit. See 9.1 point 5
    };

    bool powerDown() {
      clearBit(NAU7802_PU_CTRL_PUD, NAU7802_PU_CTRL);
      return (clearBit(NAU7802_PU_CTRL_PUA, NAU7802_PU_CTRL));
    };

    bool setIntPolarityHigh() {
      return (clearBit(NAU7802_CTRL1_CRP, NAU7802_CTRL1)); //0 = CRDY pin is high active (ready when 1)
    };

    bool setIntPolarityLow() {
      return (setBit(NAU7802_CTRL1_CRP, NAU7802_CTRL1)); //1 = CRDY pin is low active (ready when 0)
    };

    uint8_t getRevisionCode() {
      uint8_t revisionCode = getRegister(NAU7802_DEVICE_REV);
      return (revisionCode & 0x0F);
    };

    bool setBit(uint8_t bitNumber, uint8_t registerAddress) {
      uint8_t value = getRegister(registerAddress);
      value |= (1 << bitNumber); //Set this bit
      return (setRegister(registerAddress, value));
    };

    bool clearBit(uint8_t bitNumber, uint8_t registerAddress) {
      uint8_t value = getRegister(registerAddress);
      value &= ~(1 << bitNumber); //Set this bit
      return (setRegister(registerAddress, value));
    };

    bool getBit(uint8_t bitNumber, uint8_t registerAddress) {
      uint8_t value = getRegister(registerAddress);
      value &= (1 << bitNumber); //Clear all but this bit
      return (value);
    };

    uint8_t getRegister(uint8_t registerAddress) {
      _i2cPort->beginTransmission(_deviceAddress);
      _i2cPort->write(registerAddress);
      if (_i2cPort->endTransmission() != 0)
        return (-1); //Sensor did not ACK

      _i2cPort->requestFrom((uint8_t)_deviceAddress, (uint8_t)1);

      if (_i2cPort->available())
        return (_i2cPort->read());

      return (-1); //Error
    };

    bool setRegister(uint8_t registerAddress, uint8_t value) {
      _i2cPort->beginTransmission(_deviceAddress);
      _i2cPort->write(registerAddress);
      _i2cPort->write(value);
      if (_i2cPort->endTransmission() != 0)
        return (false); //Sensor did not ACK
      return (true);
    };

    int32_t get24BitRegister(uint8_t registerAddress) {
      _i2cPort->beginTransmission(_deviceAddress);
      _i2cPort->write(registerAddress);
      if (_i2cPort->endTransmission() != 0)
        return (false); //Sensor did not ACK

      _i2cPort->requestFrom((uint8_t)_deviceAddress, (uint8_t)3);

      if (_i2cPort->available())
      {
        union
        {
          uint32_t unsigned32;
          int32_t signed32;
        } signedUnsigned32; // Avoid ambiguity

        signedUnsigned32.unsigned32 = (uint32_t)_i2cPort->read() << 16; //MSB
        signedUnsigned32.unsigned32 |= (uint32_t)_i2cPort->read() << 8; //MidSB
        signedUnsigned32.unsigned32 |= (uint32_t)_i2cPort->read();      //LSB

        if ((signedUnsigned32.unsigned32 & 0x00800000) == 0x00800000)
          signedUnsigned32.unsigned32 |= 0xFF000000; // Preserve 2's complement

        return (signedUnsigned32.signed32);
      }

      return (0); //Error
    };

    bool set24BitRegister(uint8_t registerAddress, int32_t value) {
      union
      {
        uint32_t unsigned32;
        int32_t signed32;
      } signedUnsigned32; // Avoid ambiguity

      signedUnsigned32.signed32 = value;

      _i2cPort->beginTransmission(_deviceAddress);
      _i2cPort->write(registerAddress);

      _i2cPort->write((uint8_t)((signedUnsigned32.unsigned32 >> 16) & 0xFF));
      _i2cPort->write((uint8_t)((signedUnsigned32.unsigned32 >> 8) & 0xFF));
      _i2cPort->write((uint8_t)(signedUnsigned32.unsigned32 & 0xFF));

      if (_i2cPort->endTransmission() != 0)
        return (false); //Sensor did not ACK
      return (true);
    };

    uint32_t get32BitRegister(uint8_t registerAddress) {
      _i2cPort->beginTransmission(_deviceAddress);
      _i2cPort->write(registerAddress);
      if (_i2cPort->endTransmission() != 0)
        return (false); //Sensor did not ACK

      _i2cPort->requestFrom((uint8_t)_deviceAddress, (uint8_t)4);

      if (_i2cPort->available())
      {
        uint32_t unsigned32;

        unsigned32 = (uint32_t)_i2cPort->read() << 24; //MSB
        unsigned32 |= (uint32_t)_i2cPort->read() << 16;
        unsigned32 |= (uint32_t)_i2cPort->read() << 8;
        unsigned32 |= (uint32_t)_i2cPort->read();      //LSB

        return (unsigned32);
      }

      return (0); //Error
    };

    bool set32BitRegister(uint8_t registerAddress, uint32_t value) {
      _i2cPort->beginTransmission(_deviceAddress);
      _i2cPort->write(registerAddress);

      _i2cPort->write((uint8_t)((value >> 24) & 0xFF));
      _i2cPort->write((uint8_t)((value >> 16) & 0xFF));
      _i2cPort->write((uint8_t)((value >> 8) & 0xFF));
      _i2cPort->write((uint8_t)(value & 0xFF));

      if (_i2cPort->endTransmission() != 0)
        return (false); //Sensor did not ACK
      return (true);
    };

    int32_t getChannel1Offset() {
      return get24BitRegister(NAU7802_OCAL1_B2);
    };
    bool setChannel1Offset(int32_t value) {
      return set24BitRegister(NAU7802_OCAL1_B2, value);
    };
    uint32_t getChannel1Gain() {
      return get32BitRegister(NAU7802_GCAL1_B3);
    };
    bool setChannel1Gain(uint32_t value) {
      return set32BitRegister(NAU7802_GCAL1_B3, value);
    };
  private:
    TwoWire *_i2cPort;                   //This stores the user's requested i2c port
    const uint8_t _deviceAddress = 0x2A; //Default unshifted 7-bit address of the NAU7802

    //y = mx+b
    int32_t _zeroOffset = 0;        //This is b
    float _calibrationFactor = 1.0; //This is m. User provides this number so that we can output y when requested

    unsigned long _ldoRampDelay = 250; //During begin, wait this many millis after configuring the LDO before performing calibrateAFE
};
#endif
