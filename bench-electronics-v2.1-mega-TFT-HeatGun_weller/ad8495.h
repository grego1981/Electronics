/* Code from https://learn.adafruit.com/ad8495-thermocouple-amplifier/arduino */
#define AREF 5.0           // set to AREF, typically board voltage like 3.3 or 5.0
#define ADC_RESOLUTION 10  // set to ADC bit resolution, 10 is default


float ad8495_getVoltage(int raw_adc) {
	return raw_adc * (AREF / (pow(2, ADC_RESOLUTION)-1));
}

float ad8495_getTemperature(float voltage) {
	return (voltage - 1.25) / 0.005;
}
