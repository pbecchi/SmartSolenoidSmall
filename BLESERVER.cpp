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
static char argval[70];
static char* pointarg[NARG];
static byte narg;

char html[HTML_LEN];
#ifdef LORA
RH_RF95 LoRa;
#endif
//#define SER 
#define LORA_MESH
#ifdef LORA_MESH
#define N_NODES 5
#define LIMIT_RSSI 120
#define nTimes 3
#define INTERVAL 2000L
byte asknode = 1;                         //<--------------identifcatore del nodo
byte LoraTo = 1, LoraFrom = 0;

bool scanmode = false;
byte mycode;                //
byte nNodes;                //read from scan command ????only for master in EEprom????
//long routing[6];
bool notscanned = true;
long resetMilli = 300000L,mytime; bool CADmode = true;
/*
byte address[5];//this is to be defined by controller so we will use only address[6] if address[0]!=0 

void LoraSend(byte add, char* s) {
	char buf[150] = "";
	if (address[0] == 0) {
		LoRa.setHeaderTo(add);
		strcpy(buf, s);
	} else {
		byte i = 0;
		LoRa.setHeaderTo(address[0]);
		do{
	//	long code = routing[add];
		
	//	while (code != 0) {
			char add[6];
			sprintf(add, "/%d", address[i]);
			strcat(buf, add);
	//		code = code >> 8;
		} while(address[i++] > 0);
		strcat(buf, s);
		Serial.println(buf);
	}
	LoRa.setHeaderFrom(mycode);
	Serial.println("Send..");
	LoRa.send((byte *)buf, strlen(buf));
	LoRa.waitPacketSent();
}
*/

void LoraSend(char* s) {
	LoRa.setHeaderFrom(mycode);
	LoRa.setHeaderTo(LoraTo);
	LoRa.send((byte *)s, strlen(s));
	LoRa.waitPacketSent();
}

void strshorten(char * buf, byte ii, byte len) {
	byte i;
	buf[0] = '/';
	for (i = ii; i < len; i++) {
		buf[i - ii + 1] = buf[i];
		Serial.print((char)buf[i]);
	}
	buf[i - ii + 1] = 0;
}

byte readLora(byte * buf, uint8_t len) {
	if (LoRa.available()) {
		Serial.print('-');
		// Should be a reply message for us now   
		if (LoRa.recv(buf, &len)) {
			LoraFrom = LoRa.headerFrom();
			Serial.print(F("received: "));
			Serial.println((char*)buf);
			Serial.print(F("RSSI: "));
			Serial.println(LoRa.lastRssi(), DEC);

			///////////////////    _________________________send to next recipient_______________
			buf[len] = 0;
			html[0] = 0;
			char * cc; byte add[5], i = 0, ii = 0;
			if (strchr((char *)buf + 1, '/') != NULL) {
				char buff[40];
				strcpy(buff, (char *)buf);
				cc = strtok((char *)buf + 1, "/");
				while (cc != NULL) {
					Serial.println(cc);
					ii = len - strlen(cc);
					add[i++] = atoi(cc);
					cc = strtok(NULL, "/");


				}
				Serial.println(add[i - 2]);
				if (add[i - 2] != mycode) {


					for (int j = i - 2; j >= 0; j--) {
						Serial.println(add[j]);

						if (add[j] == mycode) {
							LoraTo = add[j + 1];
							LoraSend((char*)buff);
							Serial.print(F("retransmitted to")); Serial.println(LoraTo);
							//	LoraTo = 0;
							//	LoraSend("retrasmitted");
							return 0;
						}
					}
				} else {
					Serial.println(F("received"));
					///////////////////////////remove addresses from buf////////////////////////////
					strshorten((char *)buf, ii-1, len);
					buf[0] = '/';
					//	strcpy((char *)buf, cc);
					//	len = strlen(cc);//
					Serial.println((char *)buf);
					//--------------------header for replay----------------------------------------------
					char header[20] = "";
					char cc[5];
					for (int j = i - 1; j >= 0; j--) {

						sprintf(cc, "/%d", add[j]);
						strcat(header, cc);
						Serial.println(header);
					}
					strcat(header, "/");
					strcpy(html, header);
				/*	strcat(header, "/0/received ");
					// _______________________________send ack to sender___________________________
					sprintf(cc, "from %d", mycode);
					strcat(header, cc);
					LoraTo = add[i - 1];
					Serial.println(LoraTo);
					Serial.println(header);
					LoraSend(header);*/
				}
			}
			return len;
		}

		/* add main program code here */
	}
	return 0;
}

#endif
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
#ifdef LORA_MESH
	mycode = device_code;
	if (CADmode) { LoRa.setPreambleLength(500); delay(500); LoRa.isChannelActive();
	}
#else
	LoRa.setPreambleLength(500);
	DEBUG_PRINT(F("Device code")); DEBUG_PRINTLN(device_code);
	delay(1000);
	LoRa.isChannelActive();
#endif
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
#define SCAN_MESH

#ifdef LORA_MESH
	{
		byte buf[60], len = 60;
		if (!(LoRa.isChannelActive())) {

			LoRa.sleep();
			//	Serial.println("LoRa now sleep");
			//	delay(2000);
			LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
			/* add main program code here */
			//LoRa.setModeIdle();
			//delay(5);
			sleepcount++;
			premil = premil - SLEEP_MILLI;

		} else {

			Serial.print('_');
			long millistart = millis();
			while (!LoRa.available() && millis() < millistart + 3000L);


			if (readLora(buf, len)) {
				char* str;
#ifdef SCAN_MESH

				char buff[30];
				strcpy(buff, (char *)buf);
				str = strtok((char*)buf, ",\0");
				Serial.println(str);

				if (strcmp(str, "scan") == 0) {
					if (LoRa.headerTo() == mycode) {
						str = strtok(NULL, ",\0");
						if (nNodes == 0)nNodes = atoi(str);
						Serial.println(nNodes);
						//syncro = 0;

						if (notscanned&&LoRa.lastRssi() > -115) {
							scanmode = true; notscanned = false;
							asknode = LoRa.headerFrom();          ////read scan command
							if (mycode > asknode)
								mytime = (mycode - asknode) *nNodes* INTERVAL*nTimes + millis();
							else
								mytime = (asknode - mycode) *nNodes* INTERVAL*nTimes + millis() + nNodes*nNodes*nTimes*INTERVAL;
							Serial.println(mytime);

						}
						char  mess[36] = ""; ////send RSSI
						sprintf(mess, "RSSI,%d,%d", -LoRa.lastRssi(), (mytime - millis()) / 1000);
						LoraTo = LoRa.headerFrom();
						LoraSend(mess);
						Serial.print(F("scan request from")); Serial.println(LoRa.headerFrom());
					}
				} else
					if (strcmp(str, "RSSIres") == 0) {
						{
							Serial.println(F("resend ")); Serial.println(asknode);
							LoraTo = asknode;
							byte c = mycode; mycode = LoraFrom;
							LoraSend((char*)buff);
							mycode = c;
						}
					} else
						if (strcmp(str, "/cy") == 0) {
							CADmode = true;
							LoRa.setPreambleLength(500);
							Serial.println(F("In cad mode now"));

						} else
							if (strcmp(str, "/cn") == 0) {
								CADmode = false;
								LoRa.setPreambleLength(8);
								Serial.println(F("Normal mode now"));
							} else
#endif
							{
								handleClient(buf, len);
								buf[0] = 0; str[0] = 0;
							}
							//	if (str != NULL) {

							//	Serial.print("Ack"); Serial.println(LoraFrom);
								//	Serial.println((char *)buf);
								//	Serial.println(str);
							//	if (buff[1] != '.') { LoraTo = LoraFrom; LoraSend("r."); }
							//	buf[0] = 0; str[0] = 0;
			}
#ifdef SCAN_MESH
			if (scanmode&&millis() > mytime) {                   //////start myscan
				byte RSSI[8];
				notscanned = false;
				int result[N_NODES] = { 0 };
				for (byte node = 0; node < nNodes; node++) {
					for (byte n = 0; n < nTimes; n++)
						if (node != mycode) {
							Serial.print(F("Scanning node ")); Serial.println(node);
							LoraTo = node;
							char mess[15] = "scan,", cc[8];
							strcat(mess, itoa(nNodes, cc, 10));
							Serial.println(mess);
							LoraSend(mess);
							// ---------------wait for node answers------------------------
							LoraFrom = node;
							unsigned long start = millis();
							bool done = false;
							result[node] += 150;                             // assume the is no replay
							while ((millis() < start + INTERVAL) && !done)
								if (readLora(buf, len)) {
									char* str = "          ";
									str = strtok((char*)buf, ",\0");
									if (strcmp(str, "RSSI") == 0) {
										str = strtok(NULL, ",\0");
										byte r = atol(str);
										result[node] += r - 150;                 //insert received RSSI

																				 //	RSSItot[inde(node, mycode)] = atoi(str);
										Serial.print(result[node]);
										Serial.print(F("RSSI from")); Serial.println(node);
										done = true;
									}
								}
						}
					RSSI[node] = result[node] / nTimes;
					if (RSSI[node] > LIMIT_RSSI)RSSI[node] = 0;
				}
				scanmode = false;
				LoraTo = asknode;
				Serial.println(asknode);
				char  mess[30] = "RSSIres,", nn[10];
				strcat(mess, itoa(mycode, nn, 10)); strcat(mess, ",");
				for (byte i = 0; i < nNodes; i++) {
					strcat(mess, itoa(RSSI[i], nn, 10));
					strcat(mess, ",");
				}
				if (mycode > 0)LoraSend(mess);

			}
			if (millis() > resetMilli) {
				resetMilli = millis() + 1800000UL;
				notscanned = true;
				nNodes = 0;
			}
#endif
			LoRa.sleep();
			sleepcount++;
			LowPower.powerDown(SLEEP_TIME, ADC_OFF, BOD_OFF);
			premil = premil - SLEEP_MILLI;
		}
	}
#else
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

		//			char buf[RH_LoRa_MAX_MESSAGE_LEN];
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
						html[0] = 0;
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
#endif


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