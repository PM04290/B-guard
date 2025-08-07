// Callback for memory problem
void heap_caps_alloc_failed_hook(size_t requested_size, uint32_t caps, const char *function_name)
{
#ifdef DEBUG_SERIAL
  printf("!!! %s failed to allocate %d bytes with 0x%X capabilities. \n", function_name, requested_size, caps);
  printf("heap_caps_get_largest_free_block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
#endif
}

void listDir(const char * dirname)
{
  DEBUGf("Listing directory: %s\n", dirname);
  File root = SPIFFS.open(dirname);
  if (!root.isDirectory())
  {
    DEBUGln("No Dir");
  }
  File file = root.openNextFile();
  while (file)
  {
    DEBUGf(" %-30s %d\n", file.name(), file.size());
    file = root.openNextFile();
  }
}

boolean isValidFloat(String str)
{
  if (str.startsWith("-"))
    str.remove(0, 1);
  for (byte i = 0; i < str.length(); i++)
  {
    if (!isDigit(str.charAt(i)) && (str.charAt(i) != '.'))
    {
      return false;
    }
  }
  return str.length() > 0;
}

boolean isValidInt(String str)
{
  if (str.startsWith("-"))
    str.remove(0, 1);
  for (byte i = 0; i < str.length(); i++)
  {
    if (!isDigit(str.charAt(i)))
    {
      return false;
    }
  }
  return str.length() > 0;
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

float onAnalogLumi(uint16_t adcRAW, uint16_t r_fixed, uint16_t p2, uint16_t p3)
{
  float voltage = (adcRAW * 3.3) / 4095UL;
  float r_ldr = (voltage * r_fixed) / (3.3 - voltage);
  float lux = 500 / pow(r_ldr / 1000.0, 1.4);
  return adcRAW;//lux;
}

#define T_REF       298.15 // nominal temperature (Kelvin) (25°)
// coef calculator if unknown : https://www.thinksrs.com/downloads/programs/Therm%20Calc/NTCCalibrator/NTCcalculator.htm
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

float onAnalogVbat(uint16_t adcRAW, uint16_t p1, uint16_t p2, uint16_t p3)
{
  // 5 x 3.3v fullscale (16.5v)
  float mVbat = 5.0 * (adcRAW * 3300) / 4095UL;
  return mVbat / 1000.;
}
