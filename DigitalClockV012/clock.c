/*
 * clock.c
 *
 * Created: 24/07/2023 12:37:47 p. m.
 *  Author: dagmtz
 */ 


#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "clock.h"
#include "usart.h"

void readButtons( settings_control_t *p_settings )
{
	uint8_t reading = (~PINC) & BUTTONS_MASK;
	p_settings->lastReading = p_settings->currReading;
	p_settings->currReading = reading;

	if ( (reading == 1) || (reading == 2) || (reading == 4) )
	{
		if ( reading == p_settings->lastReading )
		{
			p_settings->accumulator ++;
		}
		else
		{
			p_settings->accumulator = 0;
		}
	}
	else if ( reading == 0 )
	{
		p_settings->accumulator = 0;
		p_settings->buttonState = NOT_PRESSED;
	}
	
	switch ( p_settings->buttonState )
	{
		case NOT_PRESSED:
		if ( p_settings->accumulator >= 2 )
		{
			p_settings->buttonState = SINGLE;
			p_settings->accumulator = 0;
		}
		break;
		
		case SINGLE:
		p_settings->buttonState = HOLD1;
		break;
		
		case HOLD1:
		if ( p_settings->accumulator >= 29 )
		{
			p_settings->buttonState = REPEAT;
			p_settings->accumulator = 0;
		}
		break;
		
		case REPEAT:
		p_settings->buttonState = HOLD2;
		break;
		
		case HOLD2:
		if ( p_settings->accumulator >= 9 )
		{
			p_settings->buttonState = REPEAT;
			p_settings->accumulator = 0;
		}
		break;
		
		default:
		break;
	}
	
}

void updateConfig( clock_control_t *p_clock, DS3231_buffer_t *p_RTCBuffer )
{
	if ( ( p_clock->settings.buttonState == SINGLE ) ||  ( p_clock->settings.buttonState == REPEAT ) )
	{
		switch ( p_clock->clockState )
		{
			case RUNNING:
			/* When clock is running */
			
			switch ( p_clock->settings.currReading )
			{
				case BUTTON_SETUP:
				if ( p_clock->settings.buttonState == SINGLE )
				{
					/* Change to set up seconds if SETUP is pressed */
					p_clock->clockState = SETUP;
					p_clock->settings.settingNow = SECOND;
				}
				clockToUSART( p_clock );
				break;
				
				/* Or adjust the brightness */
				case BUTTON_UP:
				update_clock( p_clock, p_clock->settings.settingNow, PLUS, true);
				break;
				
				case BUTTON_DOWN:
				update_clock( p_clock, p_clock->settings.settingNow, MINUS, true);
				break;
				
				default:
				break;
				
			}
			break;
			
			case SETUP:
			/* When clock is on setup */
			switch ( p_clock->settings.currReading )
			{
				/* Go to next setting */
				case BUTTON_SETUP:
				if ( p_clock->settings.buttonState == SINGLE )
				{
					switch ( p_clock->settings.settingNow )
					{
						case SECOND:
						syncFromClock( DS3231_SECONDS, p_clock, p_RTCBuffer );
						p_clock->settings.settingNow = MINUTE;
						break;
						
						case MINUTE:
						syncFromClock( DS3231_MINUTES, p_clock, p_RTCBuffer );
						p_clock->settings.settingNow = HOUR;
						break;
						
						case HOUR:
						syncFromClock( DS3231_HOURS, p_clock, p_RTCBuffer );
						p_clock->settings.settingNow = DAY;
						break;
						
						case DAY:
						syncFromClock( DS3231_DATE, p_clock, p_RTCBuffer );
						p_clock->settings.settingNow = MONTH;
						break;
						
						case MONTH:
						syncFromClock( DS3231_MONTH, p_clock, p_RTCBuffer );
						p_clock->settings.settingNow = YEAR;
						break;
						
						case YEAR:
						syncFromClock( DS3231_YEAR, p_clock, p_RTCBuffer );
						p_clock->clockState = RUNNING;
						p_clock->settings.settingNow = BRIGHTNESS;
						break;
						
						default:
						p_clock->clockState = RUNNING;
						p_clock->settings.settingNow = BRIGHTNESS;
						break;
					}
					clockToUSART( p_clock );
				}
				break;
				
				case BUTTON_UP:
				update_clock( p_clock, p_clock->settings.settingNow, PLUS, true);
				clockToUSART( p_clock );
				break;
				
				case BUTTON_DOWN:
				update_clock( p_clock, p_clock->settings.settingNow, MINUS, true);
				clockToUSART( p_clock );
				break;
				
				default:
				break;
			}
			break;
			
			default:
			break;
		}
	}
}

void update_clock( clock_control_t *p_clock, clock_settings_t setting, const sign_t sign, const bool affectNextUnit )
{
	switch ( setting )
	{
		case BRIGHTNESS:
		switch ( sign )
		{
			case MINUS:
			p_clock->brightness--;
			if ( p_clock->brightness == 0 )
			{
				p_clock->brightness = 1;
			}
			break;

			case PLUS:
			p_clock->brightness++;
			if ( p_clock->brightness >= 10 )
			{
				p_clock->brightness = 9;
			}
			break;

			default:
			break;
		}
		break;
		
		case SECOND:
		switch ( sign )
		{
			case MINUS:
			p_clock->time.second--;
			if ( p_clock->time.second >= SECONDS_PER_MINUTE )
			{
				p_clock->time.second = SECONDS_PER_MINUTE - 1;
			}
			break;

			case PLUS:
			p_clock->time.second++;
			if ( p_clock->time.second >= SECONDS_PER_MINUTE )
			{
				p_clock->time.second = 0;
				if (affectNextUnit)
				{
					update_clock( p_clock, MINUTE, PLUS, true );
				}
			}
			break;

			default:
			break;
		}
		break;

		case MINUTE:
		switch (sign)
		{
			case MINUS:
			p_clock->time.minute--;
			if (p_clock->time.minute >= MINUTES_PER_HOUR)
			{
				p_clock->time.minute = MINUTES_PER_HOUR - 1;
			}
			break;

			case PLUS:
			p_clock->time.minute++;
			if (p_clock->time.minute >= MINUTES_PER_HOUR)
			{
				p_clock->time.minute = 0;
				if (affectNextUnit)
				{
					update_clock( p_clock, HOUR, PLUS, true );
				}
			}
			break;

			default:
			break;
		}
		break;

		case HOUR:
		switch (sign)
		{
			case MINUS:
			p_clock->time.hour--;
			if (p_clock->time.hour >= HOURS_PER_DAY)
			{
				p_clock->time.hour = HOURS_PER_DAY - 1;
			}
			break;

			case PLUS:
			p_clock->time.hour++;
			if (p_clock->time.hour >= HOURS_PER_DAY)
			{
				p_clock->time.hour = 0;
				if (affectNextUnit)
				{
					update_clock( p_clock, DAY, PLUS, true );
				}
			}
			break;

			default:
			break;
		}
		break;

		case DAY:
		switch (sign)
		{
			case MINUS:
			p_clock->date.day--;
			if ((p_clock->date.day == 0) || (p_clock->date.day >= p_clock->daysInCurrentMonth))
			{
				p_clock->date.day = month_length(p_clock->date.month, p_clock->date.year);
			}
			break;

			case PLUS:
			p_clock->date.day++;
			if (p_clock->date.day >= p_clock->daysInCurrentMonth)
			{
				p_clock->date.day = 1;
				if (affectNextUnit)
				{
					update_clock( p_clock, MONTH, PLUS, true );
				}
			}
			break;

			case NO_SIGN:
			if (p_clock->date.day >= p_clock->daysInCurrentMonth)
			{
				p_clock->date.day = month_length(p_clock->date.month, p_clock->date.year);
			}
			break;

			default:
			break;
		}
		p_clock->weekday = getWeekday( &p_clock->date );
		break;

		case MONTH:
		switch (sign)
		{
			case MINUS:
			p_clock->date.month--;
			if ((p_clock->date.month == 0) || (p_clock->date.month >= MONTHS_PER_YEAR))
			{
				p_clock->date.month = MONTHS_PER_YEAR;
			}
			p_clock->daysInCurrentMonth = month_length(p_clock->date.month, p_clock->date.year);
			update_clock( p_clock, DAY, NO_SIGN, false );
			break;

			case PLUS:
			p_clock->date.month++;
			if (p_clock->date.month >= MONTHS_PER_YEAR)
			{
				p_clock->date.month = 1;
				if (affectNextUnit)
				{
					update_clock( p_clock, YEAR, PLUS, true );
				}
			}
			p_clock->daysInCurrentMonth = month_length(p_clock->date.month, p_clock->date.year);
			update_clock( p_clock, DAY, NO_SIGN, false );
			break;

			default:
			break;
		}
		break;

		case YEAR:
		switch (sign)
		{
			case MINUS:
			p_clock->date.year--;
			if ((p_clock->date.year == LOWER_TOP_YEARS) || (p_clock->date.year >= UPPER_TOP_YEARS))
			{
				p_clock->date.year = LOWER_TOP_YEARS;
			}
			break;

			case PLUS:
			p_clock->date.year++;
			if (p_clock->date.year >= UPPER_TOP_YEARS)
			{
				p_clock->date.year = UPPER_TOP_YEARS;
			}
			break;

			default:
			break;
		}
		break;

		default:
		break;
	}
}

void clockToUSART( clock_control_t *p_clock )
{
	char buffer[50];
	uart_puts_P( "\x1b[2;0H");
	uart_puts_P( "\x1b[0J");
	timeToString( p_clock );
	dateToString( p_clock );
	weekdayToString( p_clock );
	sprintf(buffer, "%s\t%s\r\n%s\r\n", p_clock->weekdayString, p_clock->dateString, p_clock->timeString );
	uart_puts(buffer);
}

void settingsToUSART( clock_control_t *p_clock )
{
	char buffer[50];
	uart_puts_P( "SettingNow: " );
	itoa( (int)p_clock->settings.settingNow, buffer, 10 );
	uart_puts( buffer );
	uart_puts_P( "\tBrightness: " );
	itoa( (int)p_clock->brightness, buffer, 10 );
	uart_puts( buffer );
	uart_puts_P( "\tButton: " );
	itoa( (int)p_clock->settings.currReading, buffer, 2 );
	uart_puts( buffer );
	uart_puts_P( "\tButtonState: " );
	itoa( (int)p_clock->settings.buttonState, buffer, 10 );
	uart_puts( buffer );
	uart_puts( "\r\n" );
}

void timeToString( clock_control_t *p_clock )
{
	switch( p_clock->settings.settingNow )
	{
		case SECOND:
		sprintf(p_clock->timeString, "%02d:%02d:\x1b[4;33m%02d\x1b[0m", p_clock->time.hour, p_clock->time.minute, p_clock->time.second);
		break;
		
		case MINUTE:
		sprintf(p_clock->timeString, "%02d:\x1b[4;33m%02d\x1b[0m:%02d", p_clock->time.hour, p_clock->time.minute, p_clock->time.second);
		break;
		
		case HOUR:
		sprintf(p_clock->timeString, "\x1b[4;33m%02d\x1b[0m:%02d:%02d", p_clock->time.hour, p_clock->time.minute, p_clock->time.second);
		break;
		
		default:
		sprintf(p_clock->timeString, "%02d:%02d:%02d", p_clock->time.hour, p_clock->time.minute, p_clock->time.second);
		break;
	}
	
}

void dateToString( clock_control_t *p_clock )
{
	switch( p_clock->settings.settingNow )
	{
		case DAY:
		sprintf(p_clock->dateString, "%04d-%02d-\x1b[1;33m%02d\x1b[0m", p_clock->date.year, p_clock->date.month, p_clock->date.day);
		break;
		
		case MONTH:
		sprintf(p_clock->dateString, "%04d-\x1b[1;33m%02d\x1b[0m-%02d", p_clock->date.year, p_clock->date.month, p_clock->date.day);
		break;
		
		case YEAR:
		sprintf(p_clock->dateString, "\x1b[1;33m%02d\x1b[0m-%02d-%02d", p_clock->date.year, p_clock->date.month, p_clock->date.day);
		break;
		
		default:
		sprintf(p_clock->dateString, "%04d-%02d-%02d", p_clock->date.year, p_clock->date.month, p_clock->date.day);
		break;
	}
}

void weekdayToString( clock_control_t *p_clock )
{
	switch (p_clock->weekday)
	{
		case SUNDAY:
		sprintf(p_clock->weekdayString, "Sunday" );
		break;

		case MONDAY:
		sprintf(p_clock->weekdayString, "Monday" );
		break;

		case TUESDAY:
		sprintf(p_clock->weekdayString, "Tuesday" );
		break;

		case WEDNESDAY:
		sprintf(p_clock->weekdayString, "Wednesday" );
		break;

		case THURSDAY:
		sprintf(p_clock->weekdayString, "Thursday" );
		break;

		case FRIDAY:
		sprintf(p_clock->weekdayString, "Friday" );
		break;

		case SATURDAY:
		sprintf(p_clock->weekdayString, "Saturday" );
		break;

		default:
		sprintf(p_clock->weekdayString, "------" );
		break;
	}
	
}

void syncFromClock( uint8_t address, clock_control_t *p_clock, DS3231_buffer_t *p_RTCbuffer )
{
	uint8_t byte;
	bitfield8_t buffer1;
	bitfield8_t buffer2;
	switch (address)
	{
		case DS3231_SECONDS: 
		byte = p_clock->time.second;
		buffer1.nibbles.ms_nibble = div10( byte );
		buffer1.nibbles.ls_nibble = byte - buffer1.nibbles.ls_nibble;
		DS3231_setByte( address, buffer1.byte );
		break;

		case DS3231_MINUTES: 
		byte = p_clock->time.minute;
		buffer1.nibbles.ms_nibble = div10( byte );
		buffer1.nibbles.ls_nibble = byte - buffer1.nibbles.ls_nibble;
		DS3231_setByte( address, buffer1.byte );
		break;

		case DS3231_HOURS: 
		byte = p_clock->time.hour;
		buffer2.byte = p_RTCbuffer->hours;
		buffer1.nibbles.ms_nibble = div10( byte );
		buffer1.nibbles.ls_nibble = byte - buffer1.nibbles.ls_nibble;
		buffer1.bits.b6 = buffer2.bits.b6;
		DS3231_setByte( address, buffer1.byte );
		break;

		case DS3231_DAYS: 
		DS3231_setByte( address, p_clock->weekday );
		break;

		case DS3231_DATE: 
		byte = p_clock->date.day;
		buffer1.nibbles.ms_nibble = div10( byte );
		buffer1.nibbles.ls_nibble = byte - buffer1.nibbles.ls_nibble;
		DS3231_setByte( address, buffer1.byte );
		break;

		case DS3231_MONTH: 
		byte = p_clock->date.month;
		buffer1.nibbles.ms_nibble = div10( byte );
		buffer1.nibbles.ls_nibble = byte - buffer1.nibbles.ls_nibble;
		buffer2.byte = p_RTCbuffer->month_century;
		buffer1.bits.b7 = buffer2.bits.b7;
		DS3231_setByte( address, buffer1.byte );
		break;

		case DS3231_YEAR: 
		byte = (uint8_t)( p_clock->date.year - 2000 );
		buffer1.nibbles.ms_nibble = div10( byte );
		buffer1.nibbles.ls_nibble = byte - buffer1.nibbles.ls_nibble;
		DS3231_setByte( address, buffer1.byte );
		break;

		default:
		break;
	}
}

void syncFromRTC( uint8_t address, DS3231_buffer_t *p_RTCbuffer, clock_control_t *p_clock )
{
	bitfield8_t buffer;
	switch (address)
	{
		case DS3231_SECONDS:
		buffer.byte = p_RTCbuffer->seconds;
		p_clock->time.second = ((buffer.nibbles.ms_nibble * 5) << 1) + buffer.nibbles.ls_nibble;
		break;

		case DS3231_MINUTES:
		buffer.byte = p_RTCbuffer->minutes;
		p_clock->time.minute = ((buffer.nibbles.ms_nibble * 5) << 1) + buffer.nibbles.ls_nibble;
		break;

		case DS3231_HOURS:
		buffer.byte = p_RTCbuffer->hours;
		if (buffer.bits.b6)
		{
			p_clock->time.hour = ((buffer.bits.b4 * 5) << 1) + buffer.nibbles.ls_nibble;
		}
		else
		{
			p_clock->time.hour = ((buffer.nibbles.ms_nibble * 5) << 1) + buffer.nibbles.ls_nibble;
		}
		break;

		case DS3231_DAYS:
		buffer.byte = p_RTCbuffer->days;
		p_clock->weekday = buffer.byte;
		break;

		case DS3231_DATE:
		buffer.byte = p_RTCbuffer->date;
		p_clock->date.day = ((buffer.nibbles.ms_nibble * 5) << 1) + buffer.nibbles.ls_nibble;
		break;

		case DS3231_MONTH:
		buffer.byte = p_RTCbuffer->month_century;
		p_clock->date.month = ((buffer.bits.b4 * 5) << 1) + buffer.nibbles.ls_nibble;
		break;

		case DS3231_YEAR:
		buffer.byte = p_RTCbuffer->years;
		p_clock->date.year = 2000U + ((buffer.nibbles.ms_nibble * 5) << 1) + buffer.nibbles.ls_nibble;
		break;

		default:
		break;
	}
}

uint8_t div10(uint8_t number)
{
	uint8_t q, r;
	q = (number >> 1) + (number >> 2);
	q = q + (q >> 4);
	q = q + (q >> 8);
	q = q >> 3;
	r = number - (((q << 2) + q) << 1);
	return q + (r > 9);
}

uint8_t getWeekday(const date_ymd_t *date)
{
	uint16_t y = date->year;
	uint8_t m = date->month;
	uint8_t d = date->day;

	return (d += m < 3 ? y-- : y - 2, 23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7;
}

uint8_t tickSeconds(clock_control_t *p_clock)
{
	p_clock->time.second++;

	if (p_clock->time.second >= SECONDS_PER_MINUTE)
	{
		p_clock->time.second = 0;
	}

	return p_clock->time.second;
}