// Callback for memory problem
void heap_caps_alloc_failed_hook(size_t requested_size, uint32_t caps, const char *function_name)
{
#ifdef DEBUG_SERIAL
  printf("!!! %s failed to allocate %d bytes with 0x%X capabilities. \n", function_name, requested_size, caps);
  printf("heap_caps_get_largest_free_block: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
#endif
}

float getSimulatedLux(int hour, int minute) {
  // Convertir l'heure en fraction du jour (0.0 = minuit, 0.5 = midi)
  float timeOfDay = (hour * 60.0 + minute) / 1440.0; // 1440 = minutes/jour

  // Sinusoïde centrée sur midi (0.5)
  // La luminosité est nulle de 18h à 6h (nuit)
  float lux = 0.0;
  if (timeOfDay >= 0.25 && timeOfDay <= 0.75) { // entre 6h et 18h
    // Mappe la plage [6h..18h] sur [0..PI]
    float angle = (timeOfDay - 0.25) * (PI / 0.5);
    lux = 100000.0 * sin(angle);
  }

  return lux;
}
float onAnalogLumi(uint16_t adcRAW, uint16_t r_fixed, uint16_t p2, uint16_t p3)
{
  return getSimulatedLux(cntHour, cntMinute);
  /* TODO
    float voltage = (adcRAW * 3.3) / 4095UL;
    float r_ldr = (voltage * r_fixed) / (3.3 - voltage);
    float lux = 500 / pow(r_ldr / 1000.0, 1.4);
    return adcRAW;//lux;
  */
}

#define T_REF       298.15 // nominal temperature (Kelvin) (25°)
// coef calculator if unknown : https://www.thinksrs.com/downloads/programs/Therm%20Calc/NTCCalibrator/NTCcalculator.htm
float onAnalogNTC(uint16_t adcNTC, uint16_t beta, uint16_t resistor, uint16_t thermistor)
{
  float celcius = NAN;
  // ! formula with NTC on GND
  float r_ntc = (float)resistor / (4096. / (float)adcNTC - 1.);
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
