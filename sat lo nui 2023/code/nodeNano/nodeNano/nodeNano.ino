/*
 Name:		nodeNano.ino
 Created:	11/10/2023 10:38:05 PM
 Author:	linhb
*/


//Incuding arduino default SPI library
#include <SPI.h>
//Incuding LoRa library
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"            
const int DHTPIN = 2;
const int DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);

Adafruit_MPU6050 mpu;

//define the pins used by the transceiver module
#define NSS 10
#define RST 9
#define DI0 8
#define LED 14


String LoRaData;
uint32_t lastSend, lastReadSensor, startTimeDetected, startTimeOnLed;
float normalX, normalY;
const float rangeGyro = 7.0;
const uint8_t MYID = 1;
struct rev
{
	uint8_t id;
	uint8_t hum;
	uint8_t temp;
	uint8_t detect;

};
rev sendData;

struct req
{
	uint8_t id;
	bool ledState;

};
req request;

void setup() {
	Serial.begin(115200);
	pinMode(LED, OUTPUT);
	dht.begin();
	Serial.println("LoRa Sender");
	LoRa.setPins(NSS, RST, DI0);

	//Select the frequency accordng to your location
	//433E6 for Asia
	//866E6 for Europe
	//915E6 for North America
	while (!LoRa.begin(433E6)) {
		Serial.println(".");
		delay(100);
	}
	// Change sync word (0xF1) to match the receiver LoRa
	// This code ensure that you don't get LoRa messages
	// from other LoRa transceivers
	// ranges from 0-0xFF
	LoRa.setSyncWord(0xFA);
	Serial.println("LoRa Initializing Successful!");

	while (!mpu.begin()) {
		Serial.println("Could not find a valid MPU6050 sensor, check wiring!");
		delay(500);
	}
	//setupt motion detection
	mpu.setHighPassFilter(MPU6050_HIGHPASS_5_HZ);
	mpu.setMotionDetectionThreshold(1);
	mpu.setMotionDetectionDuration(10);
	mpu.setInterruptPinLatch(true);	// Keep it latched.  Will turn off when reinitialized.
	mpu.setInterruptPinPolarity(true);
	mpu.setMotionInterrupt(true);
	calibSensor();
	sendData.id = MYID;
}

void loop() {
	// LoRa data packet size received from LoRa sender
	int packetSize = LoRa.parsePacket();
	// if the packer size is not 0, then execute this if condition
	if (packetSize) {
		if (LoRa.available()) {
			LoRa.readBytes((uint8_t*)&request, sizeof(request));
			Serial.println("RSSI: " + String(LoRa.packetRssi()));
			if (request.id == MYID ) {
				startTimeOnLed = millis();
				digitalWrite(LED, HIGH);
				LoRa.beginPacket();
				LoRa.write((uint8_t*)&sendData, sizeof(sendData));
				LoRa.endPacket();
			}
		}
		while (LoRa.available()) {
			LoRa.read();
		}
		
	}
	if (digitalRead(LED) == 1 && startTimeOnLed + 100 < millis()) {
		digitalWrite(LED, LOW);
	}
	if (startTimeOnLed + 10000 < millis()) {
		ResetBoard();
	}

	if (lastReadSensor + 500 <= millis()) {
		lastReadSensor = millis();
		sendData.hum = dht.readHumidity();
		sendData.temp = dht.readTemperature();
	/*	Serial.print("h ");
		Serial.print(dht.readHumidity());
		Serial.print("t ");
		Serial.println(dht.readTemperature());*/
	}

	// nếu có chuyển động lớn thì in ra góc nghiêng, đặt trạng thái có chuyển động
	if (mpu.getMotionInterruptStatus()) {
		sensors_event_t a, g, temp;
		mpu.getEvent(&a, &g, &temp);
		float GyroX = a.acceleration.x;
		float GyroY = a.acceleration.y;
		if (GyroX > normalX + rangeGyro || GyroX < normalX - rangeGyro||
			GyroY > normalY + rangeGyro || GyroY < normalY - rangeGyro) {
			Serial.print("AccelX:");
			Serial.print(a.acceleration.x);
			Serial.print(",");
			Serial.print("AccelY:");
			Serial.print(a.acceleration.y);
			Serial.print(",");
			Serial.print("AccelZ:");
			Serial.print(a.acceleration.z);
			startTimeDetected = millis();
			sendData.detect = 1;
		}
		if (GyroX > normalX + rangeGyro || GyroX < normalX - rangeGyro) {
			Serial.println("     motionX");
		}
		if (GyroY > normalY + rangeGyro || GyroY < normalY - rangeGyro) {
			Serial.println("    motionY");
		}
	}
	// xóa trạng thái chuyển động sau 5 giây
	if (startTimeDetected + 2000 < millis() && sendData.detect == 1) {
		sendData.detect = 0;
	}

}


void calibSensor() 	{
	static float x, y;
	for (uint8_t i = 0; i < 10; i++) {
		sensors_event_t a, g, temp;
		mpu.getEvent(&a, &g, &temp);
		Serial.print("AccelX:");
		Serial.print(a.acceleration.x);
		Serial.print(",");
		Serial.print("AccelY:");
		Serial.println(a.acceleration.y);
		x += a.acceleration.x;
		y += a.acceleration.y;
		delay(20);
	}
	normalX = x / 10;
	normalY = y / 10;
	Serial.print("normalX :");
	Serial.print(normalX);
	Serial.print(",");
	Serial.print("nomalY :");
	Serial.println(normalY);




}
void ResetBoard() {
	asm volatile ("jmp 0");
}