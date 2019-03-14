#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "SSD1306AsciiWire.h"

#define LED_RED 5
#define LED_ORANGE 6
#define LED_GREEN 7
#define I2C_ADDRESS 0x3C
SSD1306AsciiWire oled;
Adafruit_BME280 bme;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 20);
EthernetClient client;
EthernetUDP Udp;

const String APIKEY = "f5fc1bb6b03f4411d20ad5f924b12e4e";
const String CityID = "765876";
const String servername = "api.openweathermap.org";
const unsigned int localPort = 8888;
const char timeServer[] = "tempus1.gum.gov.pl";
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

float temp;
float humi;
float press;
String city;
String result;
unsigned long poprzedni = 0;
unsigned long poprzedni_pom = 0;
unsigned long poprzedni_oled = 0;
const int przerwa = 30000;
const int pomiar = 100;
const int przerwa_oled = 5000;
double sumatemp = 0;
double sumahumi = 0;
double sumapress = 0;
double tempavr = 0;
double humiavr = 0;
double pressavr = 0;
int ile_pom = 0;
int godziny;
int minuty;
bool state_r = 1;
bool state_g = 0;
enum oled_state{DOM, MIASTO, CZAS};
oled_state oled_s = DOM;

void wyswietl_oled(oled_state stan);

void setup() {
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_ORANGE, OUTPUT);
	pinMode(LED_RED, OUTPUT);
	digitalWrite(LED_GREEN, state_g);
	digitalWrite(LED_ORANGE, LOW);
	digitalWrite(LED_RED, state_r);
	Wire.begin();
	Wire.setClock(400000L);
	//Serial.begin(9600);
	bme.begin(0x76);
	oled.begin(&Adafruit128x64, I2C_ADDRESS);
	oled.setFont(fixed_bold10x15);
	oled.clear();
	if(Ethernet.begin(mac) == 0) {
		while (Ethernet.begin(mac) == 0) {
			oled.clear();
			oled.println("NO DHCP");
			delay(1000);
		}
	}
	else {
		oled.clear();
		oled.println(Ethernet.localIP());
		delay(3000);
	}
	// Sprawdzenie poprawnoœci po³¹czeñ
	if (Ethernet.hardwareStatus() == EthernetNoHardware) {
		oled.clear();
		oled.println("NO SHIELD");
		while (true) {
			delay(1); //Nic nie rób
		}
	}
	Udp.begin(localPort);
	oled.clear();
	oled.set2X();
	oled.println("\n OK");
	oled.set1X();
	delay(5000);
}

void loop() {

	if (millis() - poprzedni_pom >= pomiar) {
		sumatemp = sumatemp + bme.readTemperature();
		sumahumi = sumahumi + bme.readHumidity();
		sumapress = sumapress + bme.readPressure();
		ile_pom++;
		poprzedni_pom = millis();
	}
	
	if (millis() - poprzedni_oled >= przerwa_oled) {
		wyswietl_oled(oled_s);
		poprzedni_oled = millis();
	}

	if (millis() - poprzedni >= przerwa) {
		tempavr = sumatemp / (float)ile_pom;
		humiavr = sumahumi / (float)ile_pom;
		pressavr = sumapress / ((float)ile_pom * 100.0);
		sumatemp = 0;
		sumahumi = 0;
		sumapress = 0;
		ile_pom = 0;

		if (getWeatherData() == 0) {
			state_g = 1;
			state_r = 0;
		}	
		else {
			state_g = 0;
			state_r = 1;
		}
		digitalWrite(LED_GREEN, state_g);
		digitalWrite(LED_RED, state_r);
		sendNTPpacket(timeServer);
		delay(1000);
		if (Udp.parsePacket()) {
			Udp.readBytes(packetBuffer, NTP_PACKET_SIZE); 
			unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
			unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
			unsigned long secsSince1900 = highWord << 16 | lowWord;
			const unsigned long seventyYears = 2208988800UL;
			unsigned long epoch = secsSince1900 - seventyYears;
			epoch = epoch + 3600;
			godziny = (epoch % 86400L) / 3600;
			minuty = (epoch % 3600) / 60;
		}
		poprzedni = millis();
	}
}

int getWeatherData()
{
	digitalWrite(LED_ORANGE, HIGH);
	if (client.connect(servername.c_str(), 80)) {
		result = "";
		client.println("GET /data/2.5/weather?id=" + CityID + "&units=metric&APPID=" + APIKEY);
		client.println("Host: " + servername);
		client.println("User-Agent: ArduinoWiFi/1.1");
		client.println("Connection: close");
		client.println();
		while (client.connected() && !client.available()) delay(1);
		while (client.connected() || client.available()) {
			char c = client.read();
			result = result + c;
		}
		client.stop();
		client.flush();
		result.replace('[', ' ');
		result.replace(']', ' ');
		char jsonArray[result.length() + 1];
		result.toCharArray(jsonArray, sizeof(jsonArray));
		jsonArray[result.length()] = '\0';
		StaticJsonBuffer<1024> json_buf;
		JsonObject &root = json_buf.parseObject(jsonArray);
		String location = root["name"];
		String country = root["sys"]["country"];
		float temperature = root["main"]["temp"];
		float humidity = root["main"]["humidity"];
		String weather = root["weather"]["main"];
		String description = root["weather"]["description"];
		float pressure = root["main"]["pressure"];
		temp = temperature;
		humi = humidity;
		press = pressure;
		city = location;
		digitalWrite(LED_ORANGE, LOW);
		return 0;
	}
	else {
		digitalWrite(LED_ORANGE, LOW);
		return -1;
	}
	
}

void wyswietl_oled(oled_state stan) {
	oled.clear();
	switch (stan) {
	case DOM:
		oled.println("Dom");
		oled.println(String(tempavr) + " ^C");
		oled.println(String(humiavr) + " %");
		oled.print(String(pressavr) + " hPa");
		oled_s = MIASTO;
		break;
	case MIASTO:
		oled.println(city);
		oled.println(String(temp) + " ^C");
		oled.println(String(humi) + " %");
		oled.print(String(press) + " hPa");
		oled_s = CZAS;
		break;
	case CZAS:
		oled.println();
		oled.print(" ");
		oled.set2X();
		oled.print(godziny);
		oled.print(":");
		if (minuty < 10) {
			oled.print("0");			
		}
		oled.print(minuty);
		oled.set1X();
		oled_s = DOM;
		break;
	}
}

void sendNTPpacket(const char * address) {
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	Udp.beginPacket(address, 123); // NTP requests are to port 123
	Udp.write(packetBuffer, NTP_PACKET_SIZE);
	Udp.endPacket();
}

