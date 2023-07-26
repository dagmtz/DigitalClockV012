/*
 * ds3231.h
 *
 * Created: 24/07/2023 12:25:33 p. m.
 *  Author: dagmtz
 */ 


#ifndef DS3231_H_
#define DS3231_H_


#define DS3231_ADDRESS   0x68U
#define DS3231_SECONDS   0x00U
#define DS3231_MINUTES   0x01U
#define DS3231_HOURS     0x02U
#define DS3231_DAYS      0x03U
#define DS3231_DATE      0x04U
#define DS3231_MONTH     0x05U
#define DS3231_YEAR      0x06U
#define DS3231_A1S       0x07U
#define DS3231_A1M       0x08U
#define DS3231_A1H       0x09U
#define DS3231_A1D       0x0AU
#define DS3231_A2M       0x0BU
#define DS3231_A2H       0x0CU
#define DS3231_A2D       0x0DU
#define DS3231_CONTROL   0x0EU
#define DS3231_STATUS    0x0FU
#define DS3231_AGING     0x10U
#define DS3231_TEMP_MSB  0x11U
#define DS3231_TEMP_lSB  0x12U

#include <stdbool.h>
#include "i2c.h"

typedef union {
	uint8_t byte;
	struct {
		uint8_t ls_nibble : 4;
		uint8_t ms_nibble : 4;
	} nibbles;
	struct {
		bool b0 : 1;
		bool b1 : 1;
		bool b2 : 1;
		bool b3 : 1;
		bool b4 : 1;
		bool b5 : 1;
		bool b6 : 1;
		bool b7 : 1;
	} bits;
} bitfield8_t;

typedef struct
{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t days;
	uint8_t date;
	uint8_t month_century;
	uint8_t years;
	uint8_t a1seconds;
	uint8_t a1minutes;
	uint8_t a1hour;
	uint8_t a1day;
	uint8_t a2minutes;
	uint8_t a2hour;
	uint8_t a2day;
	uint8_t control;
	uint8_t control_status;
	uint8_t aging;
	uint8_t temp_msb;
	uint8_t temp_lsb;
} DS3231_buffer_t;

void addressInc( void );
void DS3231_getAll( DS3231_buffer_t *p_buffer );
void DS3231_setByte( uint8_t byteToSet, uint8_t value );
void DS3231_setTwelveHourFormat( bool value );

bool DS3231_getAMPM( void );
bool DS3231_getCentury( void );

uint8_t DS3231_getByte( uint8_t byteToGet );
uint8_t DS3231_getMonth( void );

float DS3231_getTemp( void );


#endif /* DS3231_H_ */