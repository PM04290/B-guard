const char HexMap[] PROGMEM = {"0123456789abcdef"};

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

void byteArrayToStr(char* dst, const byte* src, const uint16_t length)
{
  for (uint8_t i = 0; i < length; i++) {
    dst[i * 2] = pgm_read_byte(&HexMap[((char)src[i] & 0XF0) >> 4]);
    dst[i * 2 + 1] = pgm_read_byte(&HexMap[((char)src[i] & 0x0F)]);
  }
  dst[length * 2] = 0;
}
char* byteArrayToStr(const byte* src, const uint16_t length)
{
  char* dst = new char[(length * 2) + 1]; // include null terminator
  byteArrayToStr(dst, src, length);
  return dst;
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

void TaskNTP(void *pvParameters)
{
  DateTime* _rtc = (DateTime*)pvParameters;
  const TickType_t xDelay = 2000 / portTICK_PERIOD_MS;
  configTzTime(datetimeTZ, datetimeNTP, defaultNTP);
  while (true)
  {
    struct tm timeinfo;
    uint8_t retry = DEFAULT_RETRY_NTP;
    while (retry > 0) {
      if (getLocalTime(&timeinfo))
      {
        timeinfo.tm_year += 1900;
        timeinfo.tm_mon += 1;
        DEBUGf("NTP:%04d-%02d-%02d %02d:%02d:%02d\n", timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        portENTER_CRITICAL_ISR(&ntpMux);
        *_rtc = DateTime(timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        newRTC = true;
        portEXIT_CRITICAL_ISR(&ntpMux);
        retry = 0;
      } else {
        retry--;
        DEBUGf("NTP:no time yet %d/10\n", DEFAULT_RETRY_NTP - retry);
        vTaskDelay( xDelay );
      }
    }
    vTaskSuspend( NULL );
  }
}
