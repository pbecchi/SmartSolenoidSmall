#ifndef nocomp
/* Smart Solenoid  Firmware
 *
 * freely taken from OSBeeWiFi library
 * December 2016 @ bee.opensprinkler.com
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <Arduino.h>
#include "OSBeeWiFi.h"
#include "program.h"
#ifdef NRF52
#include <bluefruit.h>
#include <Nffs.h>
#define File NffsFile
#endif
#include <Time/TimeLib.h>

//byte OSBeeWiFi::state = OSB_STATE_INITIAL;

char* OSBeeWiFi::name = DEFAULT_NAME;
char* OSBeeWiFi::dkey = DEFAULT_DKEY;

byte OSBeeWiFi::has_rtc = false;
byte OSBeeWiFi::curr_zbits = 0;
byte OSBeeWiFi::next_zbits = 0;
byte OSBeeWiFi::program_busy = 0;
//byte OSBeeWiFi::st_pins[] = {PIN_ZS0, PIN_ZS1, PIN_ZS2};
ulong OSBeeWiFi::open_tstamp[]={0,0,0};
ulong OSBeeWiFi::curr_utc_time = 0;

//const char* config_fname = CONFIG_FNAME;
//const char* log_fname = LOG_FNAME;
//const char* prog_fname = PROG_FNAME;

extern ProgramData pd;

/* Options name, default integer value, max value, default string value
 * Integer options don't have string value
 * String options don't have integer or max value
 */
OptionStruct OSBeeWiFi::options[] = {
  {"fwv", OSB_FWV,     255},// "" },
  {"tmz", 62,          104},// "" },
  { "sot", 255,      255},// "" },
  { "htp", 0,        65535},// "" },
  {"dkey", 0, 0},// DEFAULT_DKEY },
  { "name", 0, 0 },// DEFAULT_NAME},
  { "raDe",0,         356}//,"" }
 
};


ulong OSBeeWiFi::curr_loc_time() {
  return curr_utc_time + (ulong)options[OPTION_TMZ].ival*900L - 43200L;
}

void OSBeeWiFi::begin() {
	DEBUG_PRINTLN(F("pin setup"));
 // digitalWrite(PIN_BST_PWR, LOW);
 // pinMode(PIN_BST_PWR, OUTPUT);
	for (byte i = 1; i < sizeof(pinOpen); i++) {
		digitalWrite(pinOpen[i], LOW);
		digitalWrite(pinClose[i], LOW);;
		pinMode(pinOpen[i], OUTPUT);
		pinMode(pinClose[i], OUTPUT);
	}
 // digitalWrite(PIN_BST_EN, LOW);
  DEBUG_PRINTLN(F("output setup"));
 // setallpins(HIGH);
  
 // state = OSB_STATE_INITIAL;
#ifdef NRF52
  DEBUG_PRINTLN("BLE  setup");
   Bluefruit.begin();
  DEBUG_PRINTLN("Nffs  setup");

 if(!  Nffs.begin())
 {
    DEBUG_PRINTLN(F("failed to mount file system!"));
  }
 if (!Nffs.testFile(config_fname))Nffs.format();
#endif
//  if(RTC.detect()) {
//    has_rtc = true;
//    DEBUG_PRINTLN(F("RTC detected"));
//  }
//  else
  {
    has_rtc = false;
    DEBUG_PRINTLN(F("RTC is NOT detected!"));
  }

}

void OSBeeWiFi::boost() {
  // turn on boost converter for 500ms
  digitalWrite(PIN_BST_PWR, HIGH);
  delay(500);
  digitalWrite(PIN_BST_PWR, LOW);  
}

/* Set all pins (including COM)
 * to a given value
 */
/*
void OSBeeWiFi::setallpins(byte value) {
  digitalWrite(PIN_COM, value);
  for(byte i=0;i<MAX_NUMBER_ZONES;i++) {
    digitalWrite(st_pins[i], value);
  }
  static byte first=true;
  if(first) {
    first=false;
    pinMode(PIN_COM, OUTPUT);
    for(byte i=0;i<MAX_NUMBER_ZONES;i++) {
      pinMode(st_pins[i], OUTPUT);
    }
  }
}
*/
void OSBeeWiFi::options_setup() {
	
	DEBUG_PRINTLN(eeprom_read_byte(0));
		if(resetcond){
			eeprom_write_byte(0, 127);
			DEBUG_PRINTLN(F("RESET TO FACTORY"));
			options_save(); // save default option values

		return;
	}
//	DEBUG_PRINTLN(F("load options"));
//	file.open(config_fname, FS_ACCESS_READ);
	options_load();

	if (options[OPTION_FWV].ival != OSB_FWV) {
		// if firmware version has changed
		// re-save options, thus preserving
		// shared options with previous firmwares
//		Nffs.remove(config_fname);
		options[OPTION_FWV].ival = OSB_FWV;
		options_save();
		return;
	}
}

int8_t OSBeeWiFi::find_option(char* name) {
	for (byte i = 0; i<NUM_OPTIONS; i++) {
		//String comp = options[i].name;
	//	DEBUG_PRINT(comp); DEBUG_PRINT(comp.length()); DEBUG_PRINT(name); DEBUG_PRINTLN(name.length());
		if (strcmp(name , options[i].name)==0) {
			return i;
		}
	}
	return -1;
}
#define OPTIONS_ADDR 10
#define OPTION_SIZE 10
#ifndef NRF52
void OSBeeWiFi::options_save() {
	OptionChar MyOptions;
	DEBUG_PRINT(F("eeprom write"));
	for (byte i = 0; i < NUM_OPTIONS; i++) {
		strcpy(MyOptions.name, options[i].name);
		if (i == OPTION_NAME)strcpy(MyOptions.sval, name);//options[i].sval.c_str());
		if (i == OPTION_DKEY)strcpy(MyOptions.sval, dkey);
		MyOptions.ival = options[i].ival;
		MyOptions.max = options[i].max;
		DEBUG_PRINT(options[i].name); DEBUG_PRINT(options[i].ival);
		eeprom_write_block((void *)&MyOptions, (void *)(OPTIONS_ADDR + sizeof(MyOptions)*i), sizeof(MyOptions));
		 DEBUG_PRINTLN(MyOptions.sval);
	}
}
void OSBeeWiFi::options_load() {
	//eeprom_read_block((void *)options, (void *)10, sizeof(options));
	OptionChar MyOptions;
	DEBUG_PRINT(F("eeprom read"));
	for (byte i = 0; i < NUM_OPTIONS; i++) {
		eeprom_read_block((void *)&MyOptions, (void *)(OPTIONS_ADDR + sizeof(MyOptions)*i), sizeof(MyOptions));
	
		strcpy(options[i].name , MyOptions.name);
	//	options[i].sval = MyOptions.sval;
		if (i == OPTION_NAME)strcpy( OSBeeWiFi::name, MyOptions.sval );
		if (i == OPTION_DKEY)strcpy(OSBeeWiFi::dkey, MyOptions.sval);

		options[i].ival = MyOptions.ival;
		options[i].max = MyOptions.max;
		DEBUG_PRINT(options[i].name); DEBUG_PRINT(options[i].ival); DEBUG_PRINTLN(MyOptions.sval);

	}
}
void OSBeeWiFi::options_reset() { 
	eeprom_write_byte((byte *)0, 0);
	DEBUG_PRINTLN(F("RESETTED TO FACTORY"));
}
void OSBeeWiFi::progs_reset() { 
	eeprom_write_byte(ADDR_PROGRAMCOUNTER,0); }
#else
void OSBeeWiFi::options_reset() {
  DEBUG_PRINT(F("reset to factory default..."));
   if(!Nffs.remove(config_fname)) {
    DEBUG_PRINT(F("failed to remove config file"));
    return;
  }
  DEBUG_PRINTLN(F("ok"));
}
void OSBeeWiFi::progs_reset() {
  if(!Nffs.remove(prog_fname)) {
    DEBUG_PRINTLN(F("failed to remove program file"));
    return;
  }
  DEBUG_PRINTLN(F("ok"));    
}


void OSBeeWiFi::options_load() {
	NffsFile file;
	file.open(config_fname, FS_ACCESS_READ);
  DEBUG_PRINT(F("loading config file..."));
  if(!file.exists()) {
    DEBUG_PRINTLN(F("failed"));
    return;
  }
#ifndef BADNFFS
  while (file.available()) {
	  String name = file.readStringUntil(':');
	  String sval = file.readStringUntil('\n');
	  sval.trim();
	  name.trim();
	 
	  int8_t idx = find_option(name);
	//  DEBUG_PRINT(idx); DEBUG_PRINT(name); DEBUG_PRINTLN(sval);
	  if (idx<0) continue;
	  if (options[idx].max) {  // this is an integer option
		  options[idx].ival = sval.toInt();
	//	  DEBUG_PRINTLN(sval.toInt());
	  } else {  // this is a string option
		  options[idx].sval = sval;
	  }
  }
#else  // read all file in a single buffer record and decode byte by byte
 int inam = 0, ival = 0,i=0;
  char nome[10], valor[10],c=0;

  uint32_t readlen;
  char buffer[464] = { 0 };
  readlen = file.read((char *)buffer, sizeof(buffer));
  DEBUG_PRINTLN(readlen);

  buffer[readlen] = 0;
  DEBUG_PRINTLN(buffer);

  while (i<readlen) {
	  c = char(buffer[i++]);
	  DEBUG_PRINT(c);
	  
	  if (inam >= 0)
		  if (c == ':') { nome[inam] = 0; inam = -1; ival = 0; } else nome[inam++] = c;
	  else
		  if (c != '\n')valor[ival++] = c;
		  else {
			  valor[ival] = 0; inam = 0;
			DEBUG_PRINT(nome); DEBUG_PRINTLN(valor);
			String name = nome;// file.readStringUntil(':');
			  DEBUG_PRINT(name); DEBUG_PRINT(":");
			  //	char sval[10];
			  String val = valor;// file.readStringUntil('\n');
			  DEBUG_PRINT(sval); DEBUG_PRINTLN("");

			  sval.trim();
			  int8_t idx = find_option(name);
			  if (idx < 0) continue;
			  if (options[idx].max) {  // this is an integer option
				  options[idx].ival = sval.toInt();
			  } else {  // this is a string option
				  options[idx].sval = sval;
			  }
		  }
  }
#endif
  DEBUG_PRINTLN(F("ok"));
  file.close();
}
void OSBeeWiFi::options_save() {

	NffsFile file;
	if (!Nffs.remove(config_fname))DEBUG_PRINTLN("No delete");
	file.open(config_fname, FS_ACCESS_WRITE);
	DEBUG_PRINT(F("saving config file..."));

	OptionStruct *o = options;
	char scr[300] = "";

	for (byte i = 0; i < 13; i++, o++) {
		//	DEBUG_PRINT("Write " + o->name + ":");
		strncat(scr, o->name.c_str(), o->name.length());
		char dot[2] = ":";
		strncat(scr, dot, 1);
		if (o->max) {
			char sval[10];
			//		DEBUG_PRINTLN(o->ival);
			itoa(o->ival, sval, 10);
			strncat(scr, sval, strlen(sval));
		} else {
			strncat(scr, o->sval.c_str(), o->sval.length());
			//		DEBUG_PRINTLN(o->sval);
		}
		strncat(scr, "\n\r", 2);
	}
	//file.write("\n\r", 2);
//			for (byte i = 0; i < 100; i++) {
//				DEBUG_PRINT(int(scr[i])); DEBUG_PRINT(" ");
//			}
	DEBUG_PRINT("Buffer=");
	DEBUG_PRINTLN(scr);
	file.write(scr, strlen(scr));
	DEBUG_PRINTLN(F("ok"));
	file.close();
}

#endif

#ifdef LOGS
void OSBeeWiFi::write_log(const LogStruct& data) {

	NffsFile file;

  uint curr = 0;
  DEBUG_PRINT(F("saving log data..."));  
  if(!Nffs.testFile(log_fname)) {  // create log file

	  file.open(log_fname, FS_ACCESS_WRITE);
    if(!file.exists()) {
      DEBUG_PRINTLN(F("failed"));
      return;
    }
    // fill log file
    uint next = curr+1;
    file.write((const byte*)&next, sizeof(next));
    file.write((const byte*)&data, sizeof(LogStruct));
    LogStruct l;
    l.tstamp = 0;
    for(;next<MAX_LOG_RECORDS;next++) {
      file.write((const byte*)&l, sizeof(LogStruct));
    }
  } else {
	  file.open(log_fname, FS_ACCESS_READ);
	  if(!file.exists()) {
      DEBUG_PRINTLN(F("failed"));
      return;
    }
    file.readBytes((char*)&curr, sizeof(curr));
    uint next = (curr+1) % MAX_LOG_RECORDS;
    file.seek(0);
    file.write((const byte*)&next, sizeof(next));

    file.seek(sizeof(curr)+curr*sizeof(LogStruct));
    file.write((const byte*)&data, sizeof(LogStruct));
  }
  DEBUG_PRINTLN(F("ok"));      
  file.close();
}

bool OSBeeWiFi::read_log_start() {
  if(log_file.exists()) log_file.close();
  log_file.open(log_fname, FS_ACCESS_READ);
  if(!log_file.exists()) return false;
  uint curr;
  if(log_file.readBytes((char*)&curr, sizeof(curr)) != sizeof(curr)) return false;
  if(curr>=MAX_LOG_RECORDS) return false;
  return true;
}

bool OSBeeWiFi::read_log_next(LogStruct& data) {
  if(!log_file.exists()) return false;
  if(log_file.readBytes((char*)&data, sizeof(LogStruct)) != sizeof(LogStruct)) return false;
  return true;  
}

bool OSBeeWiFi::read_log_end() {
  if(!log_file.exists()) return false;
  log_file.close();
  return true;
}

void OSBeeWiFi::log_reset() {
	if (!Nffs.remove(log_fname)) {
		DEBUG_PRINTLN(F("failed to remove log file"));
		return;
	}
}
#else
void OSBeeWiFi::write_log(const LogStruct& data) {}
bool OSBeeWiFi::read_log_next(LogStruct& data) {}
void OSBeeWiFi::log_reset() {}
bool OSBeeWiFi::read_log_end() {}
bool OSBeeWiFi::read_log_start() {}

#endif
void OSBeeWiFi::apply_zbits() {
  static LogStruct l;
  for(byte i=0;i<MAX_NUMBER_ZONES;i++) {
    byte mask = (byte)1<<i;
    if(next_zbits & mask) {
      if(curr_zbits & mask)     continue; // bit is already set
      open(i); // open solenoid
      open_tstamp[i]=curr_utc_time;
      l.tstamp = curr_utc_time;
      l.dur = 0;
      l.event = 'o';
      l.zid = i;
      l.pid = pd.curr_prog_index;
      l.tid = pd.curr_task_index;
      write_log(l);
    } else {
      if(!(curr_zbits & mask))  continue; // bit is already reset
      close(i);  // close solenoid
      l.tstamp = curr_utc_time;
      l.dur = curr_utc_time-open_tstamp[i]+1;
      l.event = 'c';
      l.zid = i;
      l.pid = pd.curr_prog_index;
      l.tid = pd.curr_task_index;
      write_log(l);
      open_tstamp[i]=0;
    }
  }
  curr_zbits = next_zbits;  // update curr_zbits
}

void OSBeeWiFi::set_zone(byte zid, byte value) {
  if(zid>=MAX_NUMBER_ZONES)  return; // out of bound
  byte mask = (byte)1<<zid;
  if(value) next_zbits = next_zbits | mask;
  else      next_zbits = next_zbits & (~mask);
}

// open a zone by zone index
void OSBeeWiFi::open(byte zid=0) {
  byte pin = st_pins[zid];
  digitalWrite(pinOpen[zid], HIGH);
  delay(500);
  digitalWrite(pinOpen[zid], LOW);
  digitalWrite(LEDPIN, HIGH);
  /* {
    // for latching solenoid
    boost();  // boost voltage
    setallpins(HIGH);       // set all switches to HIGH, including COM
    digitalWrite(pin, LOW); // set the specified switch to LOW
    digitalWrite(PIN_BST_EN, HIGH); // dump boosted voltage
    delay(100);                     // for 250ms
    digitalWrite(pin, HIGH);        // set the specified switch back to HIGH
    digitalWrite(PIN_BST_EN, LOW);  // disable boosted voltage
  } else {
    DEBUG_PRINT("open_nl ");
    DEBUG_PRINTLN(zid);
    // for non-latching solenoid
    digitalWrite(PIN_BST_EN, LOW);  // disable output of boosted voltage 
    boost();                        // boost voltage
    digitalWrite(PIN_COM, HIGH);    // set COM to HIGH
    digitalWrite(PIN_BST_EN, HIGH); // dump boosted voltage    
    digitalWrite(pin, LOW);         // set specified switch to LOW
  }*/
#ifdef NRF52
  digitalWrite(PIN_LED1, 1);
#endif

}

// close a zone
void OSBeeWiFi::close(byte zid=0) {
	byte pin = st_pins[zid];
	digitalWrite(pinClose[zid], HIGH);
	delay(500);
	digitalWrite(pinClose[zid], LOW);
	digitalWrite(LEDPIN, LOW);
/*	if (options[OPTION_SOT].ival == OSB_SOT_LATCH) {
		// for latching solenoid
		boost();  // boost voltage
		setallpins(LOW);        // set all switches to LOW, including COM
		digitalWrite(pin, HIGH);// set the specified switch to HIGH
		digitalWrite(PIN_BST_EN, HIGH); // dump boosted voltage
		delay(100);                     // for 250ms
		digitalWrite(pin, LOW);     // set the specified switch back to LOW
		digitalWrite(PIN_BST_EN, LOW);  // disable boosted voltage
		setallpins(HIGH);               // set all switches back to HIGH
	} else {
		DEBUG_PRINT("close_nl ");
		DEBUG_PRINTLN(zid);
		// for non-latching solenoid
		digitalWrite(pin, HIGH);
#ifdef NRF52
		digitalWrite(PIN_LED1, 0);
#endif

	}
	*/
	}

#endif