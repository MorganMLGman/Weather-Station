#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "SSD1306AsciiWire.h"

#define I2C_ADDRESS 0x3C

SSD1306AsciiWire oled;
Adafruit_BME280 bme;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 20);
EthernetClient client;
EthernetUDP Udp;

void setup() {
	Wire.begin();
	Wire.setClock(400000L);
	Serial.begin(9600);
	bme.begin(0x76);
	oled.begin(&Adafruit128x64, I2C_ADDRESS);
	bme.setSampling(
		Adafruit_BME280::MODE_NORMAL,
		Adafruit_BME280::SAMPLING_X1,
		Adafruit_BME280::SAMPLING_X1,
		Adafruit_BME280::SAMPLING_X1,
		Adafruit_BME280::FILTER_OFF,
		Adafruit_BME280::STANDBY_MS_0_5);
	//bme.takeForcedMeasurement();
}
void loop() {
	Serial.println(bme.readTemperature());
	delay(60000);
}
