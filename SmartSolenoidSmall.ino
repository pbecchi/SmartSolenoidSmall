/* OSBeeWiFi Firmware
 *
 * Main loop wrapper for Arduino
 * June 2016 @ bee.opensprinkler.com
 *
 * This file is part of the OSBeeWiFi library
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
//#define DIRECT        for Nffs directory printout
#define no

#include <Arduino.h>
//#include "defines.h"
#ifdef no
#ifdef NRF52
#include <bluefruit.h>
#include <Nffs.h>
#endif
#include "BLESERVER.h"
#include <SPI.h>
//#include <LoRa.h>
#include <avr/eeprom.h>

#include "OSBeeWiFi.h"
#include "program.h"
#include <Time/TimeLib.h>

#define SECS_PER_DAY 24*3600L

#define HTML_OK                0x00
#define HTML_SUCCESS           0x01
#define HTML_UNAUTHORIZED      0x02
#define HTML_MISMATCH          0x03
#define HTML_DATA_MISSING      0x10
#define HTML_DATA_OUTOFBOUND   0x11
#define HTML_PAGE_NOT_FOUND    0x20
#define HTML_FILE_NOT_FOUND    0x21
#define HTML_NOT_PERMITTED     0x30
#define HTML_UPLOAD_FAILED     0x40
#define HTML_WRONG_MODE        0x50
#define HTML_REDIRECT_HOME     0xFF

OSBeeWiFi osb;
ProgramData pd;
static bool time_set = false;


BLESERVER server;

struct Waterings {
public:
	ulong start;
	long dur;
	byte valv;
};
byte schedN = 0;
Waterings timesched[MAX_NUMBER_SCHEDULED];

extern char html[];
static bool curr_cloud_access_en = false;
static ulong restart_timeout = 0;
//static byte disp_mode = DISP_MODE_IP;
static byte curr_mode;
static ulong& curr_utc_time = OSBeeWiFi::curr_utc_time;
long Tot_sleep_count = 0; byte next = 0;
int sleepcount = 0;
// sleepadj=-(now()-right_time)*100000/Tot_sleep_count;
int sleepadj = 0;
void reset_zones();
void start_manual_program(byte, uint16_t);
void start_testzone_program(byte, uint16_t);
void start_quick_program(uint16_t[]);
void start_program(byte);

String two_digits(uint8_t x) {
	return String(x / 10) + (x % 10);
}

String toHMS(ulong t) {
	return two_digits(t / 3600) + ":" + two_digits((t / 60) % 60) + ":" + two_digits(t % 60);
}

void server_send_html(String html) {
	server.send( html);
}

void server_send_result(byte code, const char* item = NULL) {
	
	strcpy_P( html,PSTR(  "{\"result\":"));
	char cc[4];
	strcat(html, itoa(code,cc,10));
	strcat_P(html,PSTR( ",\"item\":\""));
	if (item) strcat(html, item);
	strcat_P(html,PSTR( "\""));
	strcat(html, "}");
	server.send();
}

bool get_value_by_key(const char* key, long& val) {
	if (server.hasArg(key)) {
		val = atol(server.arg(key));
		return true;
	} else {
		return false;
	}
}
bool get_value_by_key_P(const PROGMEM char* key, long& val) {
	if (server.hasArg_P(key)) {
		val = atol(server.arg_P(key));
		DEBUG_PRINTLN(val);
		return true;
	} else {
		return false;
	}
}
bool get_value_by_key(const char* key, String& val) {
	if (server.hasArg(key)) {
		val = server.arg(key);
		return true;
	} else {
		return false;
	}
}
bool get_value_by_key_P(const PROGMEM char* key, String& val) {
	if (server.hasArg_P(key)) {
		val = server.arg(key);
		return true;
	} else {
		return false;
	}
}
bool get_value_by_key(const char* key, char* val) {
	if (server.hasArg(key)) {
		strcpy(val,server.arg(key));
		return true;
	} else {
		return false;
	}
}
bool get_value_by_key_P(const PROGMEM char* key, char* val) {
	if (server.hasArg_P(key)) {
		strcpy(val, server.arg_P(key));
		return true;
	} else {
		return false;
	}
}
void append_key_value(char* html, const char* key, const ulong value) {
	char str[10];
	if (strlen(html) > HTML_LEN - 10) return;
	strcat(html,"\"");
	strcat(html, key);
	strcat_P(html,PSTR("\":"));
	strcat(html,ultoa( value,str,10));
	strcat(html, ",");
//	sprintf("\"%c\":&d,",key,value)
	DEBUG_PRINT(html); DEBUG_PRINTLN(value);
}
void append_key_value_P(char* html, const PROGMEM char* key, const ulong value) {
	char str[14];
	if (strlen(html) > HTML_LEN - 10) return;
	strcat_P(html,PSTR("\""));
	strcat_P(html, key);
	strcat_P(html,PSTR("\":"));
	strcat(html, ultoa(value, str, 10));
	strcat(html, ",");
	//	sprintf("\"%c\":&d,",key,value)
	DEBUG_PRINT(html); DEBUG_PRINTLN(value);
}
void append_key_value(char* html, const char* key, const int16_t value) {
	char str[10];
	if (strlen(html) > HTML_LEN - 10) return;
	strcat_P(html,PSTR("\""));
	strcat(html, key);
	strcat_P(html,PSTR("\":"));
	strcat(html, itoa(value, str, 10));
	strcat(html, ",");
	//	sprintf("\"%c\":&d,",key,value)
	DEBUG_PRINT(html); DEBUG_PRINTLN(value);
}
void append_key_value_P(char* html, const PROGMEM char* key, const int16_t value) {
	char str[10];
	if (strlen(html) > HTML_LEN - 10) return;
	strcat_P(html,PSTR("\""));
	strcat_P(html, key);
	strcat_P(html,PSTR("\":"));
	strcat(html, itoa(value, str, 10));
	strcat(html, ",");
	//	sprintf("\"%c\":&d,",key,value)
	DEBUG_PRINT(html); DEBUG_PRINTLN(value);
}

void append_key_value(char* html, const char* key,  String& value) {
	if (strlen(html) > HTML_LEN - 10) return;
	strcat_P(html,PSTR("\""));
	strcat(html, key);
	strcat_P(html, PSTR("\":\""));
	strcat(html, value.c_str());
	strcat(html, "\",");
	DEBUG_PRINTLN(html);
}

void append_key_value(char* html, const char* key, const char* value) {
	if (strlen(html) > HTML_LEN - 10) return;
	strcat_P(html,PSTR("\""));
	strcat(html, key);
	strcat(html, "\":\"");
	strcat(html, value);
	strcat(html, "\",");
	DEBUG_PRINTLN(html);
}

char dec2hexchar(byte dec) {
	if (dec<10) return '0' + dec;
	else return 'A' + (dec - 10);
}
/*
String get_mac() {
	static String hex;
	if (!hex.length()) {
		byte mac[6];
		//    WiFi.macAddress(mac);

		for (byte i = 0; i<6; i++) {
			hex += dec2hexchar((mac[i] >> 4) & 0x0F);
			hex += dec2hexchar(mac[i] & 0x0F);
		}
	}
	return hex;
}

String get_ap_ssid() {
	static String ap_ssid;
	if (!ap_ssid.length()) {
		byte mac[6];
		//    WiFi.macAddress(mac);
		ap_ssid += "OSB_";
		for (byte i = 3; i<6; i++) {
			ap_ssid += dec2hexchar((mac[i] >> 4) & 0x0F);
			ap_ssid += dec2hexchar(mac[i] & 0x0F);
		}
	}
	return ap_ssid;
}
*/
bool verify_dkey() {
	//if(curr_mode == OSB_MOD_AP) {
	//  server_send_result(HTML_WRONG_MODE);
	//  return false;
	// }
	//return true;
	if (server.hasArg("dkey")) {
		const char* comps = osb.options[OPTION_DKEY].sval.c_str();
		if (strcmp(server.arg("dkey") , comps)==0)
			return true;
		server_send_result(HTML_UNAUTHORIZED);
		return false;
	}
	return true;
}

int16_t get_pid() {
	// if(curr_mode == OSB_MOD_AP) return -2;
	long v;
	if (get_value_by_key_P(PSTR("pid"), v)) {
		return v;
	} else {
		server_send_result(HTML_DATA_MISSING, "pid");
		return -2;
	}
}
/*
String get_zone_names_json() {
	String str = F("\"zons\":[");
	for (byte i = 0; i<MAX_NUMBER_ZONES; i++) {
		str += "\"";
		str += osb.options[OPTION_ZON0 + i].sval;
		str += "\",";
	}
	str.remove(str.length() - 1);
	str += "]";
	return str;
}
*/
long sleepTime = 0;

void on_sta_controller() {
	// if(curr_mode == OSB_MOD_AP) return;
	//char html[200]= "{";
	strcpy(html, "{");
	append_key_value_P(html, PSTR("fwv"), (int16_t)osb.options[OPTION_FWV].ival);
//	append_key_value(html, "sot", (int16_t)osb.options[OPTION_SOT].ival);
	append_key_value_P(html, PSTR("utct"), curr_utc_time);
	append_key_value_P(html,PSTR( "pid"), (int16_t)pd.curr_prog_index);
	append_key_value_P(html,PSTR( "tid"), (int16_t)pd.curr_task_index);
	append_key_value_P(html,PSTR( "np"), (int16_t)pd.nprogs);
	append_key_value_P(html,PSTR( "nt"), (int16_t)pd.scheduled_ntasks);
	append_key_value_P(html,PSTR( "mnp"), (int16_t)MAX_NUM_PROGRAMS);
	append_key_value_P(html, PSTR("prem"), pd.curr_prog_remaining_time);
	append_key_value_P(html,PSTR( "trem"), pd.curr_task_remaining_time);
	append_key_value_P(html, PSTR("zbits"), (int16_t)osb.curr_zbits);
//	append_key_value(html, "name", osb.options[OPTION_NAME].sval);
	append_key_value_P(html,PSTR( "bat"),int (analogRead(A7)*31)/512);
	append_key_value_P(html,PSTR( "raiDe"), (ulong)osb.options[OPTION_RAIN_D].ival);
	//  append_key_value(html, "rssi", (int16_t)WiFi.RSSI());
	//String htm =html+ get_zone_names_json();
	//htm += "}";
//	strcat(html, osb.options[OPTION_ZON0].sval.c_str());
	strcat(html, "}");
	server.send();
}

void on_sta_logs() {
	// if(curr_mode == OSB_MOD_AP) return;
/*	String html = "{";
	append_key_value(html, "name", osb.options[OPTION_NAME].sval);

	html += F("\"logs\":[");
	if (!osb.read_log_start()) {
		html += F("]}");
		server.send();
		return;
	}
	LogStruct l;
	bool remove_comma = false;
	for (uint16_t i = 0; i<MAX_LOG_RECORDS; i++) {
		if (!osb.read_log_next(l)) break;
		if (!l.tstamp) continue;
		html += "[";
		html += l.tstamp;
		html += ",";
		html += l.dur;
		html += ",";
		html += l.event;
		html += ",";
		html += l.zid;
		html += ",";
		html += l.pid;
		html += ",";
		html += l.tid;
		html += "],";
		remove_comma = true;
	}
	osb.read_log_end();
	if (remove_comma) html.remove(html.length() - 1); // remove the extra ,
	html += F("],");
//	html += get_zone_names_json();
	html += "}";
	server_send_html(html);*/
}


void on_sta_change_controller() {
	if (!verify_dkey())  return;
	DEBUG_PRINTLN(F("CHANGE CONTROLLER"));
	if (server.hasArg("FRes")) {
		server_send_result(HTML_SUCCESS);
		osb.options_reset();
		restart_timeout = millis() + 1000;
		//osb.state = OSB_STATE_RESTART;
	} else if (server.hasArg("reset")) {
		server_send_result(HTML_SUCCESS);
		reset_zones();
	} else if (server.hasArg("t")) {

		DEBUG_PRINTLN(F("set time"));
		long ntime;
		get_value_by_key_P(PSTR("t"), ntime);
		DEBUG_PRINTLN(ntime);
		//	if (sleepadj != 0) sleepadj = -(time - ntime) * 100000L / Tot_sleep_count ;else
		sleepadj = 1;
		time_t time = ntime;
		setTime(time);
		curr_utc_time = time - (ulong)osb.options[OPTION_TMZ].ival * 900L + 43200L;
		DEBUG_PRINTLN(sleepadj);
		Tot_sleep_count = 0;

		time_set = true;

		server_send_result(HTML_SUCCESS);
	} else if (server.hasArg("reboot")) {
			server_send_result(HTML_SUCCESS);
			restart_timeout = millis() + 1000;
			//	osb.state = OSB_STATE_RESTART;
	}

}

// convert absolute remainder (reference time 1970 01-01) to relative remainder (reference time today)
// absolute remainder is stored in eeprom, relative remainder is presented to web
void drem_to_relative(byte days[2]) {
	byte rem_abs = days[0];
	byte inv = days[1];
	days[0] = (byte)((rem_abs + inv - (osb.curr_loc_time() / SECS_PER_DAY) % inv) % inv);
}

// relative remainder -> absolute remainder
void drem_to_absolute(byte days[2]) {
	byte rem_rel = days[0];
	byte inv = days[1];
	days[0] = (byte)(((osb.curr_loc_time() / SECS_PER_DAY) + rem_rel) % inv);
}
/*
long parse_listdata(const String& s, uint16_t& pos) {
	uint16_t p;
	char tmp[13];
	tmp[0] = 0;
	DEBUG_PRINTLN();
	// copy to tmp until a non-number is encountered
	for (p = pos; p<pos + 12; p++) {
		char c = s.charAt(p);
		DEBUG_PRINT(c);
		if (c == '-' || c == '+' || (c >= '0'&&c <= '9'))
			tmp[p - pos] = c;
		else
			break;
	}
	DEBUG_PRINTLN();
	tmp[p - pos] = 0;
	pos = p + 1;
	return atol(tmp);
}
*/
#ifdef OS217
uint32_t sh_parse_listdata(char **p) {
	char tmp_buffer[50];
	char* pv;
	int i = 0;
	tmp_buffer[i] = 0;
	// copy to tmp_buffer until a non-number is encountered
	for (pv = (*p); pv< (*p) + 10; pv++) {
		if ((*pv) == '-' || (*pv) == '+' || ((*pv) >= '0' && (*pv) <= '9'))
			tmp_buffer[i++] = (*pv);
		else
			break;
	}
	tmp_buffer[i] = 0;
	*p = pv + 1;
	return atol(tmp_buffer);
}

//byte server_change_program ( char *p )
byte OS_Prog(ProgramStruct &prog, char *p) {
	byte i;

	char *pv;
	boolean found = false;

	for (pv = p; (*pv) != 0 && pv<p + 100; pv++) {
		if (pv[0] == '[') {
			found = true;
			break;
		}
	}

	if (!found)  return HTML_DATA_MISSING;
	pv += 1;
	// parse headers
	//parse flags to be modified
	// * ( char* ) ( &prog ) 
	int v = sh_parse_listdata(&pv);
	// parse config bytes
	prog.enabled = v & 0x01;
	prog.sttype = (v >> 1) & 0x01;
	prog.restr = (v >> 2) & 0x03;
	prog.daytype = (v >> 4) & 0x03;
	//	prog.days[0] = (v >> 8) & 0xFF;
	//	prog.days[1] = (v >> 16) & 0xFF;
	//	byte u=(byte)prog;
	//	DEBUG_PRINTLN(u);
	prog.days[0] = sh_parse_listdata(&pv);
	prog.days[1] = sh_parse_listdata(&pv);
	DEBUG_PRINTLN(prog.days[0]);
	if (prog.daytype == DAY_TYPE_INTERVAL && prog.days[1] > 1) {
		drem_to_absolute(prog.days);
	}
	// parse start times
	pv++; // this should be a '['
	for (i = 0; i<MAX_NUM_STARTTIMES; i++) {
		prog.starttimes[i] = sh_parse_listdata(&pv) / 60;
		DEBUG_PRINTLN(prog.starttimes[i]);

		
	}
	pv++; // this should be a ','
	pv++; // this should be a '['
	byte j = 1;

	for (i = 0; i < MAX_NUM_TASKS; i++) {
		if (strlen(pv) <= 2)break;
		prog.tasks[i].dur = sh_parse_listdata(&pv);
		DEBUG_PRINTLN(prog.tasks[i].dur);
		prog.tasks[i].zbits = j;
		j = j << 1;
	}
	prog.ntasks = i;
	pv++; // this should be a ']'
	pv++; // this should be a ']'
		 return HTML_SUCCESS;
}
#endif
void on_sta_change_program() {
	if (!verify_dkey())  return;
	int16_t pid = get_pid();
	DEBUG_PRINTLN(pid);
	if (!(pid >= -1 && pid<pd.nprogs)) {
		server_send_result(HTML_DATA_OUTOFBOUND, "pid");
		return;
	}

	ProgramStruct prog;
	long v; char json[50];
	char* sv="                 ";
	// read /cp according to OpenSprinkler 2.1.7 API
	DEBUG_PRINTLN(F("/cp")); 
#ifdef OS217
	if (get_value_by_key_P(PSTR("v"), json)) {
		DEBUG_PRINTLN(json);
		byte res=OS_Prog( prog, json);
	}

	// read /cp for OS Bee 2.0 API
	else
#endif
	{
		DEBUG_PRINTLN(F("OS bee"));
		if (get_value_by_key_P(PSTR("config"), v)) {
			// parse config bytes
			prog.enabled = v & 0x01;
			prog.daytype = (v >> 1) & 0x01;
			prog.restr = (v >> 2) & 0x03;
			prog.sttype = (v >> 4) & 0x03;
			prog.days[0] = (v >> 8) & 0xFF;
			prog.days[1] = (v >> 16) & 0xFF;
			if (prog.daytype == DAY_TYPE_INTERVAL && prog.days[1] > 1) {
				drem_to_absolute(prog.days);
			}
		} else {
			server_send_result(HTML_DATA_MISSING, "config");
			return;
		}


		if (get_value_by_key_P(PSTR("sts"), sv)) {
			// parse start times
			//uint16_t pos = 0;
			byte i;
			for (i = 0; i < MAX_NUM_STARTTIMES; i++) {
			//	prog.starttimes[i] = parse_listdata(sv, pos);
				prog.starttimes[i] = sh_parse_listdata(&sv);

			}
			if (prog.starttimes[0] < 0) {
				server_send_result(HTML_DATA_OUTOFBOUND, "sts[0]");
				return;
			}
		} else {
			server_send_result(HTML_DATA_MISSING, "sts");
			return;
		}

		if (get_value_by_key_P(PSTR("nt"), v)) {
			if (!(v > 0 && v < MAX_NUM_TASKS)) {
				server_send_result(HTML_DATA_OUTOFBOUND, "nt");
				return;
			}
			prog.ntasks = v;
		} else {
			server_send_result(HTML_DATA_MISSING, "nt");
			return;
		}

		if (get_value_by_key_P(PSTR("pt"), sv)) {
			byte i = 0;
		//	uint16_t pos = 0;
			for (i = 0; i < prog.ntasks; i++) {
		//		ulong e = parse_listdata(sv, pos);
				ulong e = sh_parse_listdata(&sv);
				prog.tasks[i].zbits = e & 0xFF;
				prog.tasks[i].dur = e >> 8;
			}
		} else {
			server_send_result(HTML_DATA_MISSING, "pt");
			return;
		}
	}
	if (!get_value_by_key_P(PSTR("name"), sv)) {
	//	sv = F("Program ");
	//(	sv += (pid == -1) ? (pd.nprogs + 1) : (pid + 1);
		strcpy_P(sv, "Program");
		char n[2]; n[1] = pid + '1'; n[2] = 0;
		strcat(sv, n);
	}
	strncpy(prog.name, sv, PROGRAM_NAME_SIZE);
	prog.name[PROGRAM_NAME_SIZE - 1] = 0;

	if (pid == -1) {
		if (pd.add(&prog) == 0) {
			server_send_result(HTML_DATA_OUTOFBOUND, "Tot pids"); return;
		}
	}
	else {
		pd.modify(pid, &prog);
	}
	schedN = reschedule(now());
	server_send_result(HTML_SUCCESS);
}

void on_sta_delete_program() {
	if (!verify_dkey())  return;
	int16_t pid = get_pid();
	if (!(pid >= -1 && pid<pd.nprogs)) { server_send_result(HTML_DATA_OUTOFBOUND, "pid"); return; }

	if (pid == -1) pd.eraseall();
	else pd.del(pid);
	schedN = reschedule(now());
	server_send_result(HTML_SUCCESS);
}

void on_sta_run_program() {
	if (!verify_dkey())  return;
	int16_t pid = get_pid();
	long v = 0;
	switch (pid) {
	case MANUAL_PROGRAM_INDEX:
	{
		if (get_value_by_key_P(PSTR("zbits"), v)) {
			byte zbits = v;
			if (get_value_by_key_P(PSTR("dur"), v)) start_manual_program(zbits, (uint16_t)v);
			else { server_send_result(HTML_DATA_MISSING, "dur"); return; }
		} else { server_send_result(HTML_DATA_MISSING, "zbits"); return; }
	}
	break;

	case QUICK_PROGRAM_INDEX:
	{
		char bu[30];
		char* pos = bu;
		//String sv;
		//if (get_value_by_key_P(PSTR("durs"), sv)) {
		if (get_value_by_key_P(PSTR("durs"), pos)) {
			//uint16_t pos = 0;
			uint16_t durs[MAX_NUMBER_ZONES];
			bool valid = false;
			//DEBUG_PRINTLN(sv);
			DEBUG_PRINTLN(pos);
			for (byte i = 0; i<MAX_NUMBER_ZONES; i++) {
				durs[i] = (uint16_t)sh_parse_listdata(&pos);

				//durs[i] = (uint16_t)parse_listdata(sv, pos);

				if (durs[i]) valid = true;
			}
			if (!valid) { server_send_result(HTML_DATA_OUTOFBOUND, "durs"); return; }
			else start_quick_program(durs);
		} else { server_send_result(HTML_DATA_MISSING, "durs"); return; }
	}
	break;

	case TESTZONE_PROGRAM_INDEX:
	{
		if (get_value_by_key_P(PSTR("zid"), v)) {
			byte zid = v;
			if (get_value_by_key_P(PSTR("dur"), v))  start_testzone_program(zid, (uint16_t)v);
			else { server_send_result(HTML_DATA_MISSING, "dur"); return; }
		} else { server_send_result(HTML_DATA_MISSING, "zid"); return; }
	}
	break;
	case -1://STOP_IMMEDIATE
	{   timesched[next].dur = now()- timesched[next].start;
		//next++;
		server_send_result(HTML_SUCCESS);
		return; }
	default:
	{
		if (!(pid >= 0 && pid<pd.nprogs)) { 
			server_send_result(HTML_DATA_OUTOFBOUND, "pid"); return; } 
		else start_program(pid);
	}
	}
	schedN=reschedule(now());
	server_send_result(HTML_SUCCESS);
}

void on_sta_change_options() {
	DEBUG_PRINTLN(F("change opt"));
	if (!verify_dkey())  return;
	long ival = 0;
	String sval;
	byte i;
	OptionStruct *o = osb.options;

	// FIRST ROUND: check option validity
	// do not save option values yet
	for (i = 0; i<NUM_OPTIONS; i++, o++) {
		const char *key = o->name;
		// these options cannot be modified here
		if (i == OPTION_FWV || i == OPTION_DKEY)
			continue;

		if (o->max) {  // integer options
			if (get_value_by_key(key, ival)) {
				if (strcmp(o->name, "sot") == 0) { server.addr(ival); DEBUG_PRINT(F("new LoRa Addr.")); DEBUG_PRINTLN(ival); }
				if (ival>o->max) {
					server_send_result(HTML_DATA_OUTOFBOUND, key);
					return;
				}
			}
		}

	}


	// Check device key change
	String nkey, ckey;
	const char* _nkey = "nkey";
	const char* _ckey = "ckey";

	if (get_value_by_key(_nkey, nkey)) {
		if (get_value_by_key(_ckey, ckey)) {
			if (!nkey.equals(ckey)) {
				server_send_result(HTML_MISMATCH, _ckey);
				return;
			}
		} else {
			server_send_result(HTML_DATA_MISSING, _ckey);
			return;
		}
	}
	
	// SECOND ROUND: change option values
	o = osb.options;
	for (i = 0; i<NUM_OPTIONS; i++, o++) {
		const char *key = o->name;
		// these options cannot be modified here
		if (i == OPTION_FWV || i == OPTION_DKEY)
			continue;

		if (o->max) {  // integer options
			if (get_value_by_key(key, ival)) {
				o->ival = ival;
				DEBUG_PRINTLN(ival);
			}
		} else {
			if (get_value_by_key(key, sval)) {
				o->sval = sval;
				DEBUG_PRINTLN(ival);
			}
		}
	}

	if (get_value_by_key(_nkey, nkey)) {
		osb.options[OPTION_DKEY].sval = nkey;
	}
#ifdef NFR52

	Nffs.remove(CONFIG_FNAME);
#endif  
	osb.options_save();
	server_send_result(HTML_SUCCESS);
}
void on_sta_options() {
	DEBUG_PRINTLN(F("set options"));
	//  if(curr_mode == OSB_MOD_AP) return;
	//char html[200] = "{";
	strcpy(html, "{");
	OptionStruct *o = osb.options;
	for (byte i = 0; i<NUM_OPTIONS; i++, o++) {
		DEBUG_PRINTLN(o->name);
		if (!o->max) {
			if (i == OPTION_NAME ||i==OPTION_DKEY) {  // only output selected string options
				DEBUG_PRINT(("sv=")); DEBUG_PRINTLN(o->sval);

				append_key_value(html, o->name, o->sval);
			}
		} else {  // if this is a int option
			DEBUG_PRINT(F("iv=")); DEBUG_PRINTLN(o->ival);
			append_key_value(html, o->name, (ulong)o->ival);
		}
		
	}
	// output zone names
	//strcat(html, get_zone_names_json().c_str());
	//html += get_zone_names_json();
	//html += "}";
//	strcat(html, osb.options[OPTION_ZON0].sval.c_str());
	html[strlen(html) - 1] = 0;
	strcat(html, "}");
	DEBUG_PRINTLN(html);
	server.send();
}

void append_list_value(char * c, ulong  value, char* separ) { char cc[10]; strcat(c, ultoa(value, cc, 10)); strcat(c, separ); }
void on_sta_program() {
	strcpy(html, "{");

	 append_key_value(html, "tmz", (int16_t)osb.options[OPTION_TMZ].ival);
	strcat(html, "\"progs\":[");
	ulong v;
	ProgramStruct prog;
	bool remove_comma = false;
	DEBUG_PRINTLN(pd.nprogs);
	for (byte pid = 0; pid<pd.nprogs; pid++) {
		strcat(html , "{");
		pd.read(pid, &prog);

		v = *(byte*)(&prog);  // extract the first byte
		if (prog.daytype == DAY_TYPE_INTERVAL) {
			drem_to_relative(prog.days);
		}
		v |= ((ulong)prog.days[0] << 8);
		v |= ((ulong)prog.days[1] << 16);
		append_key_value_P(html,PSTR( "config"), (ulong)v);

		strcat(html ,"\"sts\":[");
		byte i;
		for (i = 0; i<MAX_NUM_STARTTIMES-1; i++) {
		//	html += prog.starttimes[i];
		//	html += ",";
			append_list_value(html, prog.starttimes[i], ",");
		}
		//html += prog.starttimes[MAX_NUM_STARTTIMES]; 
		//html += "],";
		append_list_value(html, prog.starttimes[i], "]");
		append_key_value(html, "nt", (int16_t)prog.ntasks);

		strcat(html ,"\"pt\":[");
		for (i = 0; i<prog.ntasks; i++) {
			v = prog.tasks[i].zbits;
			v |= ((long)prog.tasks[i].dur << 8);
			DEBUG_PRINT(prog.tasks[i].dur); DEBUG_PRINT((long)prog.tasks[i].dur << 8); DEBUG_PRINTLN(v);
			//html += v;
			//html += ",";
			append_list_value(html, v, ",");
		}
		//html.remove(html.length() - 1);
		//html += "],";
		html[strlen(html)-1]=']';

		append_key_value(html, "name", prog.name);
		//html.remove(html.length() - 1);
		html[strlen(html)-1] = 0;

		strcat(html ,"},");
		remove_comma = true;
	}
	if (remove_comma)html[strlen(html) - 1] = 0; //html.remove(html.length() - 1);
	strcat(html ,"]}");
	server.send();
}

void on_sta_delete_log() {
	if (!verify_dkey())  return;
	osb.log_reset();
	server_send_result(HTML_SUCCESS);
}

const char* weekday_name(ulong t) {
	t /= 86400L;
	t = (t + 3) % 7;  // Jan 1, 1970 is a Thursday
	static const char* weekday_names[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
	return weekday_names[t];
}

void reset_zones() {
	DEBUG_PRINTLN(F("reset zones"));
	osb.clear_zbits();
	osb.apply_zbits();
	pd.reset_runtime();
}

void start_testzone_program(byte zid, uint16_t dur) {
	if (zid >= MAX_NUMBER_ZONES) return;
	pd.reset_runtime();
	TaskStruct *e = &pd.manual_tasks[0];
	e->zbits = (1 << zid);
	e->dur = dur;
	pd.scheduled_stop_times[0] = curr_utc_time + dur;
	pd.curr_prog_index = TESTZONE_PROGRAM_INDEX;
	pd.scheduled_ntasks = 1;
//	osb.program_busy = 1;
}

void start_manual_program(byte zbits, uint16_t dur) {
	pd.reset_runtime();
	TaskStruct *e = &pd.manual_tasks[0];
	e->zbits = zbits;
	e->dur = dur;
	pd.scheduled_stop_times[0] = curr_utc_time + dur;
	pd.curr_prog_index = MANUAL_PROGRAM_INDEX;
	pd.scheduled_ntasks = 1;
//	osb.program_busy = 1;
}

void schedule_run_program() {
	byte tid;
	ulong start_time = curr_utc_time;
	for (tid = 0; tid<pd.ntasks; tid++) {
		pd.scheduled_stop_times[tid] = start_time + pd.scheduled_stop_times[tid];
		start_time = pd.scheduled_stop_times[tid];
	}
	pd.scheduled_ntasks = pd.ntasks;
//	osb.program_busy = 1;
}

void start_quick_program(uint16_t durs[]) {
	pd.reset_runtime();
	TaskStruct *e = pd.manual_tasks;
	byte nt = 0;
	ulong start_time = curr_utc_time;
	for (byte i = 0; i<MAX_NUMBER_ZONES; i++) {
		if (durs[i]) {
			e[i].zbits = (1 << i);
			e[i].dur = durs[i];
			pd.scheduled_stop_times[i] = start_time + e[i].dur;
			start_time = pd.scheduled_stop_times[i];
			nt++;
		}
	}
	if (nt>0) {
		pd.curr_prog_index = QUICK_PROGRAM_INDEX;
		pd.scheduled_ntasks = nt;
	//	osb.program_busy = 1;

	} else {
		pd.reset_runtime();
	}
}

void start_program(byte pid) {
	ProgramStruct prog;
	byte tid;
	uint16_t dur;
	if (pid >= pd.nprogs) return;
	pd.reset_runtime();
	pd.read(pid, &prog);
	if (!prog.ntasks) return;
	for (tid = 0; tid<prog.ntasks; tid++) {
		dur = prog.tasks[tid].dur;
		pd.scheduled_stop_times[tid] = dur;
	}
	pd.curr_prog_index = pid;
	schedule_run_program();
}
#ifndef NEW


byte reschedule(time_t timenow) {

	ProgramStruct dayprog;
	DEBUG_PRINT(F("RESCHEDULE")); DEBUG_PRINT(pd.scheduled_ntasks); DEBUG_PRINT(" "); DEBUG_PRINTLN(pd.nprogs);
			byte ii = 0; time_t start = now();
	for (byte i = 0; i < pd.scheduled_ntasks; i++) {
		timesched[ii].start = start;
		timesched[ii].dur = pd.scheduled_stop_times[i] - start;
		timesched[ii++].valv = i+1;

		start = pd.scheduled_stop_times[i];
		DEBUG_PRINT(timesched[ii - 1].start); DEBUG_PRINT(" "); DEBUG_PRINTLN(timesched[ii - 1].dur);

	}
	
	for (byte day = 0; day < 30; day++) {
		ulong day_time = (now()/86400UL)*SECS_PER_DAY + day*86400UL + 1;
		//DEBUG_PRINT(now()); DEBUG_PRINT(" "); DEBUG_PRINT(now() / 86400UL); DEBUG_PRINT(" "); DEBUG_PRINTLN((now() / 86400UL)*SECS_PER_DAY);
		for (byte i = 0; i < pd.nprogs; i++) {
			if (pd.nprogs > 1 || day == 0)pd.read(i, &dayprog);
				if (dayprog.check_day_match(day_time&&osb.options[OPTION_RAIN_D].ival==0)) {
				for (byte j = 0; j < MAX_NUM_STARTTIMES; j++) {
					if (dayprog.starttimes[j] + day_time > start && dayprog.starttimes[j]>0) {
						timesched[ii].start = dayprog.starttimes[j]*60 + day_time;
						timesched[ii].dur = dayprog.tasks[0].dur;
						timesched[ii++].valv = dayprog.tasks[0].zbits;

						for (byte iii = 1; iii < dayprog.ntasks; iii++) {
							timesched[ii].start = timesched[ii - 1].start + timesched[ii - 1].dur;
							timesched[ii].dur = dayprog.tasks[iii].dur;
							timesched[ii++].valv = dayprog.tasks[iii].zbits;
						}
						if (ii == MAX_NUMBER_SCHEDULED) {
							DEBUG_PRINT(F("NEXT IN min.")); DEBUG_PRINTLN(timesched[0].start - now());
							next = 0;
							DEBUG_PRINT(F("busy")); DEBUG_PRINTLN(osb.program_busy);
							osb.program_busy = false;
							return ii;

						}
						DEBUG_PRINT(timesched[ii - 1].start); DEBUG_PRINT(" "); DEBUG_PRINTLN(timesched[ii - 1].dur);
					}
				}
			}
		}
	}
	DEBUG_PRINT(F("NEXT IN min.")); DEBUG_PRINTLN(timesched[0].start - now());
	next = 0;
	DEBUG_PRINT(F("busy")); DEBUG_PRINTLN(osb.program_busy);
	osb.program_busy = false;
	return ii;
}
#endif
#endif



byte onoff = 0;
static ulong last_time = 0;
//static ulong last_minute = 0;

void setup() {

	{DEBUG_BEGIN(115200); }
	DEBUG_PRINTLN(F("SmartSolenoid"));
#ifdef no
	//while (!Serial.available()) delay(100);
	osb.begin();
	delay(1000);
	DEBUG_PRINTLN(F("Opt setup"));
	osb.options_setup();
	// close all zones at the beginning.
	//for (byte i = 0; i<MAX_NUMBER_ZONES; i++) osb.close(i);
	server.device_code = osb.options[OPTION_SOT].ival;
	server.controller_code = osb.options[OPTION_HTP].ival;

	DEBUG_PRINT(F("PD setup")); DEBUG_PRINTLN(server.device_code);
	pd.init();
//	curr_mode = osb.get_mode();
	//!!!!!!!!!!!!!!!!pushing reset in next 5 sec with led on reboot to factory defaults!!!!!!!!!!!!!!!!!!!!
	pinMode(LEDPIN, OUTPUT);
	eeprom_write_byte(0, 0);
	digitalWrite(LEDPIN, HIGH);
	delay(5000);
	digitalWrite(LEDPIN, LOW);
	eeprom_write_byte(0, 127);
#endif

	server.on("/jc", on_sta_controller);
	server.on("/jo", on_sta_options);
	server.on("/jl", on_sta_logs);
	server.on("/jp", on_sta_program);
	server.on("/cc", on_sta_change_controller);
	server.on("/co", on_sta_change_options);
	server.on("/cp", on_sta_change_program);
	server.on("/dp", on_sta_delete_program);
	server.on("/rp", on_sta_run_program);
	server.on("/dl", on_sta_delete_log);



	server.begin();
	delay(1000);
	
	server.send("Begin");
	osb.program_busy = false;

	DEBUG_PRINTLN(F("END setup"));
	if (schedN == 0 && time_set)schedN = reschedule(now());
}
unsigned long old_millis = 0;
bool newday = true;
void loop() {
	if (now() % SECS_PER_DAY < 50 && newday) {
		newday = false;
		if (osb.options[OPTION_RAIN_D].ival > 0) {
			osb.options[OPTION_RAIN_D].ival--;
			osb.options_save();
		}
	}
	if (now() % SECS_PER_DAY > 60)newday = true;
	
	server.LoRaReceiver(sleepcount);
	

	if (sleepcount > 200){
	//------correct time for sleep milliseconds-or read time from RTC if available	
		//Serial.print(hour()); Serial.print('-');
		//Serial.print(minute()); Serial.print(':'); Serial.println(second());
		setTime(now() + (SLEEP_MILLI*sleepcount + millis() - old_millis) / 1000 + sleepadj / 500);
		
		Tot_sleep_count += sleepcount;
		sleepcount = 0;

		//DEBUG_PRINTLN(millis());
		old_millis = millis();

}   
	curr_utc_time = now();

	//else  pd.next_prog_time = timesched[next].start - curr_utc_time;
	// ------------granularity 1 second------------------------------------------
	if (last_time != curr_utc_time) {
		if (osb.program_busy)	pd.curr_prog_remaining_time = timesched[next].start + timesched[next].dur - curr_utc_time;
		last_time = curr_utc_time;
#ifndef PR_TIME
		Serial.print(hour()); Serial.print(':');
		Serial.print(minute()); Serial.print(':'); Serial.println(second());
#endif
#ifndef NEW
		if (schedN > next)
			if (!osb.program_busy) {
				if (now() >= timesched[next].start) {
					/////start---------------------------------
				//	String st = "Start ";
				//	st += timesched[next].valv;
				//	server.send(st);
					char sta[7]="Start ";
					sta[6] = '0' + timesched[next].valv;
					server.send(sta);
					DEBUG_PRINTLN(sta);
					osb.program_busy = true;
					osb.open(timesched[next].valv);
				}
			} else {
				// osb.program_busy check end time to close
				if (now() >= timesched[next].start + timesched[next].dur) {
					//////stop-----------------------------------
					osb.program_busy = false;
					osb.close(timesched[next].valv);
					DEBUG_PRINT(F("STOP next in min."));
					next++;
					ulong nextTime;
					if (now() >timesched[next].start)nextTime = 0;
					else nextTime = (timesched[next].start - now()) / 60;

					DEBUG_PRINTLN(nextTime);
					char resp[30];
					sprintf_P(resp, "Stop %d next start in %d min", timesched[next-1].valv,nextTime);
					server.send(resp);
					if (schedN>=3&&next > schedN - 2) {
						next = 0;
						schedN = reschedule(now());
					}
				} else
					//         ??????????????????check if valve is open???????
				{
					DEBUG_PRINT(F("RUNNING remaining sec."));
					DEBUG_PRINTLN(timesched[next].start + timesched[next].dur - now());
				}
			}

#endif
	}
#ifndef no

	

	curr_utc_time = now();
	//  if(curr_mode == OSB_MOD_STA) {
	// time_keeping();
	check_status();
	// }
	// process_button();



	if (last_time != curr_utc_time) {
		last_time = curr_utc_time;
		//waitForEvent();
		//sleepTime += now() - last_time;
		//  process_display();
		//--------- 1 second granularity------------------------
		// ==== Schedule program data ===
		ulong curr_minute = curr_utc_time / 60;
		byte pid;
		ProgramStruct prog;
		//----------1 minute granularity---------------------------
		if (curr_minute != last_minute) {
			last_minute = curr_minute;
			if (!osb.program_busy) {
				for (pid = 0; pid<pd.nprogs; pid++) {
					pd.read(pid, &prog, true);

					if (prog.check_match(osb.curr_loc_time())) {
						//pd.read(pid, &prog);
						start_program(pid);
						DEBUG_PRINTLN("START");
						break;
					}
				}
			}
		}
		//----------------------1 second granularity-------------------------
		// ==== Run program tasks ====
		// Check there is any program running currently
		// If do, do zone book keeping
		if (osb.program_busy) {
			DEBUG_PRINTLN("RUNNING");
#ifdef  MYPIN_LED2
			digitalWrite(MYPIN_LED2, onoff & 0x01);
#endif
			// check stop time
			if (pd.curr_task_index == -1 ||
				curr_utc_time >= pd.scheduled_stop_times[pd.curr_task_index]) {
				// move on to the next task
				pd.curr_task_index++;
				if (pd.curr_task_index >= pd.scheduled_ntasks) {
					// the program is now over
					DEBUG_PRINTLN("program finished");
					reset_zones();
					osb.program_busy = 0;
					DEBUG_PRINTLN("STOP");

				} else {
					TaskStruct e;
					pd.load_curr_task(&e);
					osb.next_zbits = e.zbits;
					pd.curr_prog_remaining_time = pd.scheduled_stop_times[pd.scheduled_ntasks - 1] - curr_utc_time;
					pd.curr_task_remaining_time = pd.scheduled_stop_times[pd.curr_task_index] - curr_utc_time;
				}
			} else {
				pd.curr_prog_remaining_time = pd.scheduled_stop_times[pd.scheduled_ntasks - 1] - curr_utc_time;
				pd.curr_task_remaining_time = pd.scheduled_stop_times[pd.curr_task_index] - curr_utc_time;
			}
		}
		osb.apply_zbits();
	}
#endif
}

