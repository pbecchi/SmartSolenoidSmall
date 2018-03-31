#ifndef nocomp
#include "BLESERVER.h"

#include <TimeLib.h>
#define LORA
#include "defines.h"
#ifdef LORA
#include <SPI.h>              // include libraries
//#include <LoRa.h>
#include <RH_RF95.h>
#include <LowPower.h>
//const int csPin = 10;          // LoRa radio chip select
//const int resetPin = 9;       // LoRa radio reset
//const int irqPin = 1;         // change for your board; must be a hardware interrupt pin
#endif
// Peripheral uart service
#define NARG 7
static char argname[NARG][8];
static char argval[80];
static char* pointarg[NARG];
static byte narg;

char html[HTML_LEN];
#ifdef LORA
RH_RF95 LoRa;
#endif
//#define SER 



BLESERVER::BLESERVER() {
}


BLESERVER::~BLESERVER() {
}
void BLESERVER::begin() {

#ifdef LORA


	if (!LoRa.init()) {          
		DEBUG_PRINTLN(F("LoRa init failed. Check your connections."));
		while (true);                  
	}
//	LoRa.setModemConfig(LoRa.Bw31_25Cr48Sf512);// Bw31_25Cr48Sf512);
	LoRa.setThisAddress(device_code);
	LoRa.setHeaderFrom(device_code);
	LoRa.setPreambleLength(500);
	DEBUG_PRINT(F("Device code")); DEBUG_PRINTLN(device_code);
	delay(1000);
	LoRa.isChannelActive();
	DEBUG_PRINTLN(F("LoRa init succeeded."));
#endif
}
void BLESERVER::addr(int address) {
	LoRa.setThisAddress(address);
	LoRa.setHeaderFrom(address);
}
void BLESERVER::handleClient(char* RXpointer, int iRX) {

#ifdef SER
	if (Serial.available()) {
		char a = Serial.read();
		RXpointer[iRX++] = a;
		DEBUG_PRINT(a);
	}

	if (RXpointer[iRX - 1] != 10)return;//wait EOL or CR
	RXpointer[iRX] = 0;
	Serial.flush();
#endif
	//  decode RX buffer----------------------------------------------------	
	//	if (RXpointer[iRX] != 0)return;
	if (iRX == 0) return;
	RXpointer[iRX] = 0;
	DEBUG_PRINT(F("INPUT:")); DEBUG_PRINTLN(RXpointer);
//	char s[50];
	narg = 0;
	char *  str = "\0";
	str = strtok(RXpointer, "?\0");
	if (str == NULL)return;
	DEBUG_PRINTLN(str);
	str = strtok(NULL, "=\0\10\13");
    pointarg[0] = &argval[0];
	//int ii = 0;
	while (str != NULL) {
	//	DEBUG_PRINTLN(str);
		strcpy(argname[narg],str);
	//	DEBUG_PRINT(strlen(argname[narg])); DEBUG_PRINT(argname[narg][strlen(argname[narg])]); DEBUG_PRINTLN(argname[narg]);
		str = strtok(NULL, "&\0\10\13");
		if (str == NULL) { narg++; break; }
		pointarg[narg + 1] = pointarg[narg]+ strlen(str) +2;
		strcpy(pointarg[narg++],str);
	//	ii = ii + strlen(str) + 1;
		str = strtok(NULL, "=\0");
		DEBUG_PRINT(argname[narg - 1]); DEBUG_PRINT("="); DEBUG_PRINTLN(pointarg[narg - 1]);
		if (narg == NARG)DEBUG_PRINTLN(F("Too many arguments!"));
	}

	iRX = 0;
	for (byte i = 0; i < Handler_count; i++) {
		for (byte j = 0; j < 3; j++) {
			if (Handler_u[i][j] != RXpointer[j])break;
			if (j == 2) {
				DEBUG_PRINT(F("handler ")); DEBUG_PRINTLN(i);
				Handler_f[i]();
				return;
			}
		}
	}
	}
#ifdef LORA
#define CAD

#ifndef DEBUG
#define DEBbegin(x) {}
#define DEBprint(x) {}
#define DEBprintln(x) {}
#else
#define DEBbegin(x) Serial.begin(x)
#define DEBprint(x) Serial.print(x)
#define DEBprintln(x) Serial.println(x)
#endif
//ulong premil = 1000;

bool BLESERVER::LoRaReceiver(int &sleepcount) {


	long	premil = millis() + STEP_TIME;
	// 
	while (millis() < premil)

		if (!(LoRa.isChannelActive())) {

			LoRa.sleep();
			LowPower.powerDown(SLEEP_TIME, ADC_OFF, BOD_OFF);
			sleepcount++;
			premil = premil - SLEEP_MILLI;

		} else {
			DEBprint("Message to:"); DEBprintln(LoRa.headerTo());
			int i = 0;
			if (LoRa.waitAvailableTimeout(5000)) {
				if (true)//device_code == LoRa.headerTo())
				{
					if (controller_code != LoRa.headerFrom())DEBUG_PRINTLN(F("not from my controller"));

					DEBprintln(LoRa.headerTo());
					// Should be a message for us now   
		
		//			char buf[RH_RF95_MAX_MESSAGE_LEN];
					char RXpointer[100];// RH_RF95_MAX_MESSAGE_LEN];
					uint8_t len = sizeof(RXpointer);// RH_RF95_MAX_MESSAGE_LEN;a
					if (LoRa.recv((byte *)RXpointer, &len)) {
						//	digitalWrite(led, HIGH);
							//      RH_RF95::printBuffer("request: ", buf, len);
						DEBprint(F("Text: "));
						RXpointer[len] = 0;
						DEBprint((char*)RXpointer);
						DEBprint(F("RSSI: "));
						DEBprintln(LoRa.lastRssi());
						// Send a reply
						//char data[20] = "Received ";
						//strcat(data, (char *)buf);
						//LoRa.send((byte *)data, sizeof(data));=[

						handleClient(RXpointer, len);

						DEBprintln(F("Sent a reply"));

						

					} else DEBprintln(F("recv failed"));

				} else DEBprintln(F("not for me"));
			} else DEBprintln(F("timeout"));
			LoRa.sleep();
			sleepcount++;
			LowPower.powerDown(SLEEP_TIME, ADC_OFF, BOD_OFF);
			premil = premil - SLEEP_MILLI;
			//		return true;
		}


}
#endif

char iiii;
void BLESERVER::on(char * uri, THandlerFunction handler) {
	if (Handler_count >= MAX_HANDLER) return;
	for (byte j = 0; j<3; j++) Handler_u[Handler_count][j] = uri[j];
	Handler_f[Handler_count] = handler;
	Handler_count++;
}


char* BLESERVER::arg(const char* name) {
	for (byte i = 0; i < narg; i++)
		if (strcmp(argname[i] , name)==0) {
			DEBUG_PRINT(i); DEBUG_PRINT(F(" arg=")); DEBUG_PRINTLN(pointarg[ i ]);
			
			return pointarg[i];
		}
	return "";
}
char* BLESERVER::arg_P(const PROGMEM char* name) {
	for (byte i = 0; i < narg; i++)
		if (strcmp_P(argname[i], name) == 0) {
			DEBUG_PRINT(i); DEBUG_PRINT(F(" arg=")); DEBUG_PRINTLN(pointarg[i]);

			return pointarg[i];
		}
	return "";
}

char * BLESERVER::arg(int i) {
	DEBUG_PRINT(i); DEBUG_PRINT(F(" arg=")); DEBUG_PRINTLN(argval[i]);
	char ret[12];
	strcpy(ret	,pointarg[i]);
	return ret;
}

char* BLESERVER::argName(int i) {
	return argname[i];
}

int BLESERVER::args() {
	return narg;
}

bool BLESERVER::hasArg(const char* name) {
	DEBUG_PRINT(strlen(name)); DEBUG_PRINTLN(name);

	for (byte i = 0; i < narg; i++) {
	   DEBUG_PRINT(strlen(argname[i])); DEBUG_PRINTLN(argname[i]);
	//	DEBUG_PRINTLN(strcmp(argname[i], name));
		if (strcmp(argname[i], name) == 0) return true;
	/*	for (byte j = 0; j < 8; j++) {
		DEBUG_PRINT(int(argname[i][j])); DEBUG_PRINT(" "); DEBUG_PRINTLN(int(name[j]));
			if (argname[i][j] == 0 && name[j] == 0)return true;
			if (argname[i][j] != name[j])return false;

		}*/
	}
	return false;
}
bool BLESERVER::hasArg_P(const PROGMEM char* name) {
	char str[10];
	strcpy_P(str, name);
	for (byte i = 0; i < narg; i++) {
		DEBUG_PRINT(strlen(str)); DEBUG_PRINT(str); DEBUG_PRINT(strlen(argname[i])); DEBUG_PRINTLN(argname[i]);
			DEBUG_PRINTLN(strcmp(argname[i], str));
		if (strcmp(argname[i], str) == 0) return true;
	}
	DEBUG_PRINTLN("F");
	return false;
}
//void BLESERVER::send(int code, const String & content_type, const String & content) {
	//DEBUG_PRINTLN( content);
	void BLESERVER::send(const String & content) {
#ifdef SER
	DEBUG_PRINT(content);
#endif
	
	for (byte i = 0; i < content.length()+1; i++)
		html[i] = content[i];
	DEBUG_PRINTLN(html);
	LoRa.send((const uint8_t *)html ,strlen(html));
	LoRa.waitPacketSent();
}
	void BLESERVER::send(const char* content) {

		if (!LoRa.send((const uint8_t *)content, strlen(content))) { DEBUG_PRINTLN(F("not send!")); }
		LoRa.waitPacketSent();
		
	}
void BLESERVER::send() {
#ifdef LORA
	LoRa.send((const uint8_t *)html, strlen(html));
	LoRa.waitPacketSent();
#endif
}




#endif