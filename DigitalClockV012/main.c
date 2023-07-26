/*
 * DigitalClockV012.c
 *
 * Created: 25/07/2023 09:43:20 p. m.
 * Author : dagmtz
 */ 

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "usart.h"
#include "clock.h"
#include "ds3231.h"
#include "oled.h"

#define RTC_RESPONSE_MAX_TRIES 200
#define COUNTER2_TOP_MS 10

/* Size for usartBuffer used in main() */
#define BUFF_SIZE   25

/* 1Hz heartbeat coming from RTC */
volatile bool g_heartbeat_1s = false;

/* Increases close to every 1ms. Counts up to 33.55s */
volatile uint16_t g_msCounter1 = 0;
volatile uint16_t g_msCounter2 = 0;
volatile uint16_t g_msCounter3 = 0;

clock_control_t *gp_clockCtrl;

int main(void)
{	
	/* <<<<<<<<<< SETUP >>>>>>>>>> */

	/* Set up external interrupt ( 1Hz from RTC ) */
	DDRD &= _BV( PORTD2 );
	EICRA = ( _BV( ISC01 ) | _BV( ISC00 ) );
	
	/* Set up Timer/Counter 0 in normal mode with prescaler 64 */
	TCCR0A = 0;
	TCCR0B = ( _BV( CS00 ) | _BV( CS01 ) );

	/* Enable INT0 external interrupt */
	EIMSK = _BV( INT0 );
	
	/* Enable Timer/Counter 0 Overflow interrupt */
	TIMSK0 = _BV( TOIE0 );
	
	DDRB |= _BV( PORTB5 );

#ifdef USART_DEBUG
	/*** SET UP USART ***/
	uart_init( BAUD_CALC( 115200 ) ); // 8n1 transmission is set as default
	
	stdout = &uart0_io; // attach uart stream to stdout & stdin
	stdin = &uart0_io; // uart0_in and uart0_out are only available if NO_USART_RX or NO_USART_TX is defined
	
	// char usartBuffer[BUFF_SIZE];
#endif /* USART_DEBUG */

	sei();
	
#ifdef USART_DEBUG
	uart_puts_P( "\x1b[2J\x1b[H" );
	uart_puts_P( "\x1b[1;32mUSART Ready\r\n" );
#endif /* USART_DEBUG */
	
	g_msCounter2 = 0;
	while( g_msCounter2 < 100 );
	
	/*** SET UP I2C ***/
#ifdef USART_DEBUG
	uart_puts_P( "\x1b[1;33m>Initializing I2C\r\n" );
#endif /* USART_DEBUG */
	i2c_init();
	
	///*** SET UP OLED ***/
#ifdef USART_DEBUG
	uart_puts_P( "\x1b[1;33m>Initializing OLED Display\r\n" );
#endif /* USART_DEBUG */

	oled_init( LCD_DISP_ON ); // init lcd and turn on
	oled_puts_p( PSTR( "Initializing..." ) );
	
#ifdef USART_DEBUG
	uart_puts_P( "\x1b[1;33m>Initializing DS3231\r\n" );
#endif /* USART_DEBUG */
	
	 i2c_start_sla( TW_SLA_W( DS3231_ADDRESS ) );
	 i2c_write( DS3231_CONTROL );
	 
	 /* Set the desired value to the address set */
	 i2c_write( 0 );
	 i2c_stop();
	 
	uart_putuint( DS3231_getByte( DS3231_CONTROL ) );
	
	
	#ifdef USART_DEBUG
	uart_puts_P( "\x1b[0m" );
	#endif /* USART_DEBUG */
	
	while (1)
	{
		if ( g_heartbeat_1s )
		{
			g_heartbeat_1s = false;
			uart_putuint( DS3231_getByte( DS3231_SECONDS ) );
		}
	}
	
}

ISR( INT0_vect )
{
	g_heartbeat_1s = true;
	PORTB ^= _BV( PORTB5 );	
}

ISR( TIMER0_OVF_vect )
{
	g_msCounter1++;
	g_msCounter2++;
	g_msCounter3++;
	
}
