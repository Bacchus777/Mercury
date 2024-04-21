#include "mercury200.h"
#include "Debug.h"
#include "OSAL.h"
#include "OnBoard.h"
#include "hal_led.h"
#include "hal_uart.h"

#ifndef MERCURY_PORT
#define MERCURY_PORT HAL_UART_PORT_1
#endif

static void Mercury200_RequestMeasure(uint32 serial_num, uint8 cmd);
static current_values_t Mercury200_ReadCurrentValues(void);
static energy_t Mercury200_ReadEnergy(void);
static uint16 MODBUS_CRC16( const unsigned char *buf, unsigned int len );

extern zclMercury_t mercury200_dev = {&Mercury200_RequestMeasure, &Mercury200_ReadCurrentValues, &Mercury200_ReadEnergy};

#define MERCURY200_CV_RESPONSE_LENGTH 14
#define MERCURY200_E_RESPONSE_LENGTH 23

void Mercury200_RequestMeasure(uint32 serial_num, uint8 cmd) 
{
  uint8 readMercury[7]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  readMercury[4] = cmd;// 0x63; // текущие значения
  readMercury[3] = serial_num & 0xFF;
  readMercury[2] = (serial_num>>8) & 0xFF;
  readMercury[1] = (serial_num>>16) & 0xFF;
  readMercury[0] = (serial_num>>24) & 0xFF;
  
  uint16 crc = MODBUS_CRC16(readMercury, 5);
  
  readMercury[5] = crc & 0xFF;
  readMercury[6] = (crc>>8) & 0xFF;

  HalUARTWrite(MERCURY_PORT, readMercury, sizeof(readMercury) / sizeof(readMercury[0])); 
  
  LREP("Mercury sent: ");
  for (int i = 0; (i < sizeof(readMercury) / sizeof(readMercury[0])); i++) 
  {
    LREP("0x%X ", readMercury[i]);
  }
  LREP("\r\n");
}


current_values_t Mercury200_ReadCurrentValues(void) 
{
  current_values_t result = {.Voltage = MERCURY_INVALID_RESPONSE, .Current = MERCURY_INVALID_RESPONSE, .Power = MERCURY_INVALID_RESPONSE};
  
  uint8 response[MERCURY200_CV_RESPONSE_LENGTH] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  HalUARTRead(MERCURY_PORT, (uint8 *)&response, sizeof(response) / sizeof(response[0]));

  LREP("Mercury received: ");
  for (int i = 0; i <= MERCURY200_CV_RESPONSE_LENGTH - 1; i++) 
  {
    LREP("0x%X ", response[i]);
  }
  LREP("\r\n");
  
  uint16 crc = MODBUS_CRC16(response, MERCURY200_CV_RESPONSE_LENGTH - 2);

  LREP("Real CRC: ");
  LREP("0x%X ", crc & 0xFF);
  LREP("0x%X\r\n", (crc>>8) & 0xFF);
  
  if (response[MERCURY200_CV_RESPONSE_LENGTH - 2] != (crc & 0xFF) || response[MERCURY200_CV_RESPONSE_LENGTH - 1] != ((crc>>8) & 0xFF)) {
    LREPMaster("Invalid response\r\n");
    HalUARTRead(MERCURY_PORT, (uint8 *)&response, sizeof(response) / sizeof(response[0]));
    return result;
  }

  result.Voltage = (uint16)(response[5] / 16) * 1000 + (response[5] % 16) * 100 + (uint16)(response[6] / 16) * 10 + (response[6] % 16);
  result.Current = (uint16)(response[7] / 16) * 1000 + (response[7] % 16) * 100 + (uint16)(response[8] / 16) * 10 + (response[8] % 16);
  result.Power =   (uint16)(response[9] / 16) * 100000 + (response[9] % 16) * 10000 + (uint16)(response[10] / 16) * 1000 + (response[10] % 16) * 100 + (uint16)(response[11] / 16) * 10 + (response[11] % 16);

  return result;
}

static uint32 from_bcd_to_dec(uint8 bcd) {

    uint32 dec = ((bcd >> 4) & 0x0f) * 10 + (bcd & 0x0f);

    return dec;
}

energy_t Mercury200_ReadEnergy(void) 
{
    energy_t result = {
      .Energy_T1 = MERCURY_INVALID_RESPONSE, 
      .Energy_T2 = MERCURY_INVALID_RESPONSE, 
      .Energy_T3 = MERCURY_INVALID_RESPONSE, 
      .Energy_T3 = MERCURY_INVALID_RESPONSE
    };
    
    uint8 response[MERCURY200_E_RESPONSE_LENGTH] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    HalUARTRead(MERCURY_PORT, (uint8 *)&response, sizeof(response) / sizeof(response[0]));

    LREP("Mercury received: ");
    for (int i = 0; i <= MERCURY200_E_RESPONSE_LENGTH - 1; i++) 
    {
      LREP("0x%X ", response[i]);
    }
    LREP("\r\n");
    
    uint16 crc = MODBUS_CRC16(response, MERCURY200_E_RESPONSE_LENGTH - 2);

    LREP("Real CRC: ");
    LREP("0x%X ", crc & 0xFF);
    LREP("0x%X\r\n", (crc>>8) & 0xFF);
    
    if (response[MERCURY200_E_RESPONSE_LENGTH - 2] != (crc & 0xFF) || response[MERCURY200_E_RESPONSE_LENGTH - 1] != ((crc>>8) & 0xFF)) {
        LREPMaster("Invalid response\r\n");
        HalUARTRead(MERCURY_PORT, (uint8 *)&response, sizeof(response) / sizeof(response[0]));
        return result;
    }
/*
    result.Energy_T1 = (uint32)(response[ 5] / 16) * 10000000 + (response[ 5] % 16) * 1000000 + (uint32)(response[ 6] / 16) * 100000 + (response[ 6] % 16) * 10000 + (uint32)(response[ 7] / 16) * 1000 + (response[ 7] % 16) * 100 + (uint16)(response[ 8] / 16) * 10 + (response[ 8] % 16);
    result.Energy_T2 = (uint32)(response[ 9] / 16) * 10000000 + (response[ 9] % 16) * 1000000 + (uint32)(response[10] / 16) * 100000 + (response[10] % 16) * 10000 + (uint32)(response[11] / 16) * 1000 + (response[11] % 16) * 100 + (uint16)(response[12] / 16) * 10 + (response[12] % 16);
    result.Energy_T3 = (uint32)(response[13] / 16) * 10000000 + (response[13] % 16) * 1000000 + (uint32)(response[14] / 16) * 100000 + (response[14] % 16) * 10000 + (uint32)(response[15] / 16) * 1000 + (response[15] % 16) * 100 + (uint16)(response[16] / 16) * 10 + (response[16] % 16);
    result.Energy_T4 = (uint32)(response[17] / 16) * 10000000 + (response[17] % 16) * 1000000 + (uint32)(response[18] / 16) * 100000 + (response[18] % 16) * 10000 + (uint32)(response[19] / 16) * 1000 + (response[19] % 16) * 100 + (uint16)(response[20] / 16) * 10 + (response[20] % 16);
*/
    result.Energy_T1 = 0;
    result.Energy_T1 += from_bcd_to_dec(response[ 5]) * 1000000;
    result.Energy_T1 += from_bcd_to_dec(response[ 6]) * 10000;
    result.Energy_T1 += from_bcd_to_dec(response[ 7]) * 100;
    result.Energy_T1 += from_bcd_to_dec(response[ 8]);

    result.Energy_T2 = 0;
    result.Energy_T2 += from_bcd_to_dec(response[ 9]) * 1000000;
    result.Energy_T2 += from_bcd_to_dec(response[10]) * 10000;
    result.Energy_T2 += from_bcd_to_dec(response[11]) * 100;
    result.Energy_T2 += from_bcd_to_dec(response[12]);

    result.Energy_T3 = 0;
    result.Energy_T3 += from_bcd_to_dec(response[13]) * 1000000;
    result.Energy_T3 += from_bcd_to_dec(response[14]) * 10000;
    result.Energy_T3 += from_bcd_to_dec(response[15]) * 100;
    result.Energy_T3 += from_bcd_to_dec(response[16]);

    result.Energy_T4 = 0;
    result.Energy_T4 += from_bcd_to_dec(response[17]) * 1000000;
    result.Energy_T4 += from_bcd_to_dec(response[18]) * 10000;
    result.Energy_T4 += from_bcd_to_dec(response[19]) * 100;
    result.Energy_T4 += from_bcd_to_dec(response[20]);

    return result;
}

static uint16 MODBUS_CRC16 ( const unsigned char *buf, unsigned int len )
{
	uint16 crc = 0xFFFF;
	unsigned int i = 0;
	char bit = 0;

	for( i = 0; i < len; i++ )
	{
		crc ^= buf[i];

		for( bit = 0; bit < 8; bit++ )
		{
			if( crc & 0x0001 )
			{
				crc >>= 1;
				crc ^= 0xA001;
			}
			else
			{
				crc >>= 1;
			}
		}
	}

	return crc;
}

