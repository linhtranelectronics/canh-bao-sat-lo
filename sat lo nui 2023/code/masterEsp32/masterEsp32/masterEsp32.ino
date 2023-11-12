/*
 Name:		masterEsp32.ino
 Created:	11/10/2023 10:16:58 PM
 Author:	linhb
*/

//Incuding arduino default SPI library
#include <SPI.h>
//Incuding LoRa library
#include <LoRa.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <ThingSpeak.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
//define the pins used by the transceiver module
#define NSS 4
#define RST 5
#define DI0 15
#define LED 2
#define NUMOFNODE 2
#define BUTTON 13
#define BUZZER 14

char *ssid = "free";   // your network SSID (name) 
char *pass = "12345678";   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;

unsigned long myChannelNumber = 943446;
const char * myWriteAPIKey = "JHPOR3QZMQY70BAN";

uint32_t  startTimeWanring, lastimeSendSMS, lastTimeReceiveWanring, lastTimeSendThingSpeak;
bool enableSendSMS;
uint32_t NOW;
struct rev
{
	uint8_t id;
	uint8_t hum;
	uint8_t temp;
	uint8_t detect;

};
rev receiveData;

struct req
{
	uint8_t id;
	bool ledState;

};
req request;

struct logWanring
{
	uint8_t id;
	uint8_t hum;
	uint8_t temp;
	uint32_t startTime;
	uint8_t numOfSMS;
	bool wanringDone;
};
logWanring wanring;

void setup() {
	Serial.begin(115200);
	Serial2.begin(115200);
	lcd.init();
	lcd.backlight();
	pinMode(LED, OUTPUT);
	pinMode(BUZZER, OUTPUT);
	pinMode(BUTTON, INPUT_PULLUP);
	Serial.println("LoRa Master");
	LoRa.setPins(NSS, RST, DI0);
	while (!LoRa.begin(433E6)) {
		Serial.println(".");
		delay(100);
	}
	LoRa.setSyncWord(0xFA);
	Serial.println("LoRa Initializing Successful!");
	//sendSMS("0335644677", "canh bao!!!");
	wanring.wanringDone = true;
	WiFi.mode(WIFI_STA);
	ThingSpeak.begin(client);  // Initialize ThingSpeak
	connectWifi();
}

void loop() {
	NOW = millis();
	receiveDataFromNode();
	requetNode();
	if (!wanring.wanringDone) {
		digitalWrite(BUZZER, HIGH);
		if (digitalRead(BUTTON) == 0) {
			digitalWrite(BUZZER, LOW);
			wanring.wanringDone = true;
			enableSendSMS = false;
			wanring.numOfSMS = 0;
		}
		if (wanring.startTime + 5000 <= NOW && wanring.wanringDone == false) {
			enableSendSMS = true;
		}
	}
	if (enableSendSMS && lastimeSendSMS + 5000 <= NOW && wanring.numOfSMS < 5) {
		lastimeSendSMS = NOW;
		//sendSMS("0335644677", "canh bao!!!");
		Serial.println("SEND SMS");
		enableSendSMS = false;
		wanring.numOfSMS++;
	}
	if (lastTimeSendThingSpeak + 10000 <= NOW) {
		lastTimeSendThingSpeak = NOW;
		int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
		if (x == 200) {
			Serial.println("Channel update successful.");
		} else {
			Serial.println("Problem updating channel. HTTP error code " + String(x));
		}
	}


}

void sendSMS(String number, String message) {
	String cmd = "AT+CMGS=\"" + number + "\"\r\n";
	Serial2.print(cmd);
	delay(100);
	Serial2.println(message);
	delay(100);
	Serial2.write(0x1A); // send ctrl-z
	delay(100);
}
void requetNode() 	{
	static uint32_t lastTimeRequest;
	if (NOW - lastTimeRequest >= 500) {
		lastTimeRequest = NOW;
		static uint8_t i = 0;
		request.id = i % NUMOFNODE;
		i++;
		LoRa.beginPacket();
		LoRa.write((uint8_t*)&request, sizeof(request));
		LoRa.endPacket();
	}
}
void receiveDataFromNode() {
	int packetSize = LoRa.parsePacket();
	if (packetSize) {
		while (LoRa.available()) {
			LoRa.readBytes((uint8_t*)&receiveData, sizeof(receiveData));
		}
		lcd.setCursor(0, receiveData.id);
		lcd.print("h ");
		lcd.print(receiveData.hum);
		lcd.print(" t ");
		lcd.print(receiveData.temp);
		lcd.print("  W ");
		lcd.print(receiveData.detect);
		lcd.print("  ");
		if (receiveData.detect == 1 && lastTimeReceiveWanring + 5000 <= NOW) {
			lastTimeReceiveWanring = NOW;
			wanring.id = receiveData.id;
			wanring.hum = receiveData.hum;
			wanring.temp = receiveData.temp;
			wanring.startTime = NOW;
			wanring.wanringDone = false;
		}
		if (receiveData.id == 0) {
			ThingSpeak.setField(1, receiveData.temp);
			ThingSpeak.setField(2, receiveData.hum);
			//ThingSpeak.setField(3, receiveData.detect);
		}
		else if (receiveData.id == 1) {
			ThingSpeak.setField(4, receiveData.temp);
			ThingSpeak.setField(5, receiveData.hum);
			//ThingSpeak.setField(6, receiveData.detect);
		}
		if (wanring.wanringDone == false) {
			if (wanring.id == 0) {
				ThingSpeak.setField(3, receiveData.detect);
			}
			else {
				ThingSpeak.setField(6, receiveData.detect);
			}
		}
		else {
			ThingSpeak.setField(3, receiveData.detect);
			ThingSpeak.setField(6, receiveData.detect);
		}
	}
}
void connectWifi() {
	// Connect or reconnect to WiFi
	if (WiFi.status() != WL_CONNECTED) {
		Serial.print("Attempting to connect to SSID: ");
		Serial.println(ssid);
		WiFi.begin(ssid, pass);
		while (WiFi.status() != WL_CONNECTED) {
			 // Connect to WPA/WPA2 network. Change this line if using open or WEP network
			Serial.print("c");
			delay(200);
		}
		Serial.println("\nConnected.");
	}
}
