/* OSBeeWiFi Firmware
 *
 * OSBeeWiFi macro defines and hardware pin assignments
 * December 2016 @ bee.opensprinkler.com
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

#ifndef _DEFINES_H
//#define _DEFINES_H
//#define MYPIN_LED2 PIN_LED2
#define PIN_LED3 25
#define OS217
#ifndef NRF52 
#define HTML_LEN 180
#include <avr/eeprom.h>
#define nvm_read_block  eeprom_read_block
#define nvm_write_block eeprom_write_block
#define nvm_read_byte   eeprom_read_byte
#define nvm_write_byte  eeprom_write_byte
#define EEPROMS
#define resetcond  eeprom_read_byte(0)!=127
#endif
#define STEP_TIME 500
//#define SLEEP_MILLI 263L
//#define SLEEP_TIME SLEEP_250MS
#define SLEEP_MILLI 133L
#define SLEEP_TIME SLEEP_120MS
 /** Firmware version, hardware version, and maximal values */
#define OSB_FWV    100    // Firmware version: 100 means 1.0.0
#define LEDPIN 7
/** GPIO pins */
#define PIN_BST_PWR PIN_LED3    // Boost converter power
#define PIN_BST_EN  PIN_LED3    // Boost converter enable
#define PIN_OPEN_1     3    // Zone switch 0
#define PIN_CLOSE_1    4    // Zone switch 0
#define PIN_OPEN_2     5    // Zone switch 0
#define PIN_CLOSE_2    6    // Zone switch 0

// Default device name
#define DEFAULT_NAME    "My SmartSolenoid"
// Default device key
#define DEFAULT_DKEY    ""
#define ADDR_PROGRAMCOUNTER 400
#define OSB_SOT_LATCH    0x00
#define OSB_SOT_NONLATCH 0x01
#define MAX_NUMBER_SCHEDULED 10
#define MAX_NUMBER_ZONES         2

#define MAX_LOG_RECORDS           200
#define MANUAL_PROGRAM_INDEX      'M' // 77
#define QUICK_PROGRAM_INDEX       'Q' // 81
#define TESTZONE_PROGRAM_INDEX    'T' // 84

#define MIN_EPOCH_TIME  978307200

typedef enum {
  OPTION_FWV = 0, // firmware version
  OPTION_TMZ,     // time zone
  OPTION_SOT,     // device address 
  OPTION_HTP,     // controller address
  OPTION_DKEY,    // device key
  OPTION_NAME,    // device name
  OPTION_RAIN_D,  // rain delay days
  NUM_OPTIONS     // number of options
} OSB_OPTION_enum;

static byte pinOpen[] = { 0,PIN_OPEN_1,PIN_OPEN_2 };
static byte pinClose[] = { 0,PIN_CLOSE_1,PIN_CLOSE_2 };

#define TIME_SYNC_TIMEOUT  3600

/** Serial debug functions */
#define SERIAL_DEBUG

#if defined(SERIAL_DEBUG)
#define DEBUG
#define DEBUG_BEGIN(x)   { Serial.begin(x); }

  #define DEBUG_PRINT(x)   Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)

#else

  #define DEBUG_BEGIN(x)   {}
  #define DEBUG_PRINT(x)   {}
  #define DEBUG_PRINTLN(x) {}

#endif

typedef unsigned char byte;
typedef unsigned long ulong;

#endif  // _DEFINES_H
