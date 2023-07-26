/*
 * ds3231.c
 *
 * Created: 24/07/2023 12:39:21 p. m.
 *  Author: dagmtz
 */ 

#include "ds3231.h"

static uint8_t addressPtr = 0x00;

/**
 *  @brief   
 *  @param   
 *  @return  none
 */
void addressInc()
{
  addressPtr++;
  if ( addressPtr >= 0x12 )
  {
    addressPtr = 0x00;
  }
}


/**
 *  @brief   
 *  @param   
 *  @return  none
 */
void DS3231_getAll( DS3231_buffer_t *p_buffer )
{
  if( addressPtr != 0x00 )
  {
    addressPtr = 0x00;
    /* Set DS3231 Address Pointer to the desired address */
    i2c_start_sla( TW_SLA_W( DS3231_ADDRESS ) );
    i2c_write( addressPtr );
    i2c_stop();
  }
  
  i2c_start_sla( TW_SLA_R( DS3231_ADDRESS ) );
  
  p_buffer->seconds = i2c_readAck();
  p_buffer->minutes = i2c_readAck();
  p_buffer->hours = i2c_readAck();
  p_buffer->days = i2c_readAck();
  p_buffer->date = i2c_readAck();
  p_buffer->month_century = i2c_readAck();
  p_buffer->years = i2c_readAck();
  p_buffer->a1seconds = i2c_readAck();
  p_buffer->a1minutes = i2c_readAck();
  p_buffer->a1hour = i2c_readAck();
  p_buffer->a1day = i2c_readAck();
  p_buffer->a2minutes = i2c_readAck();
  p_buffer->a2hour = i2c_readAck();
  p_buffer->a2day = i2c_readAck();
  p_buffer->control = i2c_readAck();
  p_buffer->control_status = i2c_readAck();
  p_buffer->aging = i2c_readAck();
  p_buffer->temp_msb = i2c_readAck();
  p_buffer->temp_lsb = i2c_readNAck();
  
  i2c_stop();  
}

/**
 *  @brief   Write byte to DS3231
 *  @param   byteToSet the address to write to
 *  @return  none
 */
void DS3231_setByte( uint8_t byteToSet, uint8_t value )
{
  
  /* Set DS3231 Address Pointer to the desired address */
  i2c_start_sla( TW_SLA_W( DS3231_ADDRESS ) );
  i2c_write( byteToSet );
  
  /* Set the desired value to the address set */
  i2c_write( value );
  i2c_stop();
  
  addressInc();
}

/**
 *  @brief		Set AM/PM bit to DS3231
 *				A logic 1 in bit 6 of the byte
 *
 *  @param		h12 True means 12h format
 *  @return		none
 */
void DS3231_setTwelveHourFormat( bool value )
{
	 bitfield8_t buffer;
	 buffer.byte = DS3231_getByte( DS3231_HOURS );
	 
	if ( buffer.bits.b6 != value )
	{
		buffer.bits.b6 = ~buffer.bits.b6;
		/* Set DS3231 Address Pointer to the desired address */
		i2c_start_sla( TW_SLA_W( DS3231_ADDRESS ) );
		i2c_write( DS3231_HOURS );
		
		/* Set the desired value to the address set */
		i2c_write( buffer.byte );
		i2c_stop();
		
		addressInc();	
	}
}

/**
 *  @brief   
 *  @param   
 *  @return  none
 */
bool DS3231_getAMPM()
{
  bitfield8_t buffer = {0};
  buffer.byte = DS3231_getByte( DS3231_HOURS );
  
  /* 0 = 24h, 1 = 12h */
  return buffer.bits.b6;
}

/**
 *  @brief   
 *  @param   
 *  @return  none
 */
bool DS3231_getCentury()
{
  bitfield8_t buffer = {0};
  buffer.byte = DS3231_getByte( DS3231_MONTH );
  
  /* 0 = XXI, 1 = XXII */
  return buffer.bits.b7;
}

/**
 *  @brief   
 *  @param   
 *  @return  none
 */
uint8_t DS3231_getByte( uint8_t byteToGet )
{
  uint8_t buffer = 0;
  
  if( addressPtr != byteToGet )
  {
    addressPtr = byteToGet;
    /* Set DS3231 Address Pointer to the desired address */
    i2c_start_sla( TW_SLA_W( DS3231_ADDRESS ) );
    i2c_write( byteToGet );
    i2c_stop();
  }
  else
  {
    /* Request one byte and send NACK to end transmission */
    i2c_start_sla( TW_SLA_R( DS3231_ADDRESS ) );
    buffer = i2c_readNAck();
    i2c_stop();
  }
  
  addressInc();
  return buffer;
}

/**
 *  @brief   
 *  @param   
 *  @return  none
 */
uint8_t DS3231_getMonth()
{
  bitfield8_t buffer = {0};
  buffer.byte = DS3231_getByte( DS3231_MONTH );
  buffer.bits.b7 = 0;
  
  return buffer.byte;
}

/**
 *  @brief   
 *  @param   
 *  @return  none
 */
float DS3231_getTemp()
{

  uint8_t tMSB, tLSB;
  float temp;
  
  if( addressPtr != DS3231_TEMP_MSB )
  {
    addressPtr = DS3231_TEMP_MSB;
    /* Set DS3231 Address Pointer to the desired address */
    i2c_start_sla( TW_SLA_W( DS3231_ADDRESS ) );
    i2c_write( DS3231_TEMP_MSB );
    i2c_stop();
  }
  
  i2c_start_sla( TW_SLA_R( DS3231_ADDRESS ) );
  tMSB = i2c_readAck();
  addressInc();
  tLSB = i2c_readNAck();
  addressInc();
  i2c_stop();
  
  int16_t _temp = ( tMSB << 8 | ( tLSB & 0xC0 ) );  // Shift upper byte, add lower
  temp = ( ( float ) _temp / 256.0 );              // Scale and return
  
  return temp;
}
