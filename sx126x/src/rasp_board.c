/*!
 * \file      sx1261dvk1bas-board.c
 *
 * \brief     Target board SX1261DVK1BAS shield driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "board.h"
#include "radio.h"
#include "sx126x-board.h"

#define CHANNEL 0
uint8_t fd;

void SX126xIoInit( void )
{
    wiringPiSetup();

    pinMode(RADIO_NSS, OUTPUT);
    pinMode(RADIO_BUSY, INPUT);
    pinMode(RADIO_DIO_1, INPUT);
    //pullUpDnControl(RADIO_RESET, PUD_DOWN);
    pullUpDnControl(RADIO_NSS, PUD_UP);
    pullUpDnControl(RADIO_DIO_1, PUD_DOWN);
    digitalWrite(RADIO_NSS, HIGH);

    fd = wiringPiSPISetup(CHANNEL, 100000);
    if (fd < 0 )
    {
	printf("can't init SPI\r\n");
        while(1);
    }
    uint8_t mode = SPI_MODE_0, msb = 0, data = 0;
    ioctl(fd,SPI_IOC_WR_MODE, &mode);
    ioctl(fd, SPI_IOC_WR_LSB_FIRST, &msb);
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &data);
}

void SX126xIoIrqInit( DioIrqHandler dioIrq )
{
    wiringPiISR(RADIO_DIO_1, INT_EDGE_RISING, dioIrq, NULL);
    //attachInterrupt(digitalPinToInterrupt(RADIO_DIO_1), dioIrq, RISING );
    //GpioSetInterrupt( &SX126x.DIO1, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY, dioIrq ); 
}

void SX126xIoDeInit( void )
{
    // GpioInit( &SX126x.Spi.Nss, RADIO_NSS, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
    // GpioInit( &SX126x.BUSY, RADIO_BUSY, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    // GpioInit( &SX126x.DIO1, RADIO_DIO_1, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
}

uint8_t SPItransfer(uint8_t dataIn)
{
    uint8_t dataOut;
    //write(fd, &dataIn, 1);
    //usleep(10);
    //read(fd, &dataOut, 1);
#ifdef DEBUG
    printf("sending : %#02x\r\n",dataIn);
#endif
    wiringPiSPIDataRW(0,&dataIn,1);
    dataOut = dataIn;
#ifdef DEBUG
    if (dataOut == 0)
	printf("receive :     NONE\r\n");
    else
    	printf("receive : %#02x\r\n",dataOut);
#endif
    return dataOut;
}

uint32_t SX126xGetBoardTcxoWakeupTime( void )
{
    return BOARD_TCXO_WAKEUP_TIME;
}

void SX126xReset( void )
{
    usleep( 10000 );
    pinMode(RADIO_RESET,OUTPUT);
    digitalWrite(RADIO_RESET, LOW);
    //GpioInit( &SX126x.Reset, RADIO_RESET, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    usleep( 10000 );
    digitalWrite(RADIO_RESET, HIGH);
    //pinMode(RADIO_RESET,INPUT);
    //GpioInit( &SX126x.Reset, RADIO_RESET, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 ); // internal pull-up
    usleep( 10000 );
}

void SX126xWaitOnBusy( void )
{
  usleep(20000);
    //while(digitalRead(RADIO_BUSY) == HIGH);
    //while( GpioRead( &SX126x.BUSY ) == 1 );
}

void SX126xWakeup( void )
{
    BoardDisableIrq( );

    digitalWrite(RADIO_NSS,LOW); 
    //GpioWrite( &SX126x.Spi.Nss, 0 );

    SPItransfer(RADIO_GET_STATUS);
    SPItransfer(0x00);
    // SpiInOut( &SX126x.Spi, RADIO_GET_STATUS );
    // SpiInOut( &SX126x.Spi, 0x00 );

    digitalWrite(RADIO_NSS,HIGH); 
    // GpioWrite( &SX126x.Spi.Nss, 1 );

    // Wait for chip to be ready.
    SX126xWaitOnBusy( );

    BoardEnableIrq( );
}

void SX126xWriteCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );

    digitalWrite(RADIO_NSS,LOW); 
    //GpioWrite( &SX126x.Spi.Nss, 0 );

    SPItransfer((uint8_t)command);
    //SpiInOut( &SX126x.Spi, ( uint8_t )command );

    for( uint16_t i = 0; i < size; i++ )
    {
        SPItransfer(buffer[i]);
        //SpiInOut( &SX126x.Spi, buffer[i] );
    }

    digitalWrite(RADIO_NSS,HIGH); 
    //GpioWrite( &SX126x.Spi.Nss, 1 );

    if( command != RADIO_SET_SLEEP )
    {
        SX126xWaitOnBusy( );
    }
}

void SX126xReadCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );

    digitalWrite(RADIO_NSS,LOW); 
    //GpioWrite( &SX126x.Spi.Nss, 0 );

    SPItransfer((uint8_t)command);
    SPItransfer(0x00);
    //SpiInOut( &SX126x.Spi, ( uint8_t )command );
    //SpiInOut( &SX126x.Spi, 0x00 );
    //SPI.setBitOrder(LSBFIRST);
    for( uint16_t i = 0; i < size; i++ )
    {
        buffer[i] = SPItransfer(0x00);
        //buffer[i] = SpiInOut( &SX126x.Spi, 0 );
    }
    digitalWrite(RADIO_NSS,HIGH); 
    //GpioWrite( &SX126x.Spi.Nss, 1 );

    SX126xWaitOnBusy( );
}

void SX126xWriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );

    //SPI.beginTransaction(settings);
    digitalWrite(RADIO_NSS,LOW);
    //GpioWrite( &SX126x.Spi.Nss, 0 );
    
    SPItransfer(RADIO_WRITE_REGISTER);
    SPItransfer(( address & 0xFF00 ) >> 8);
    SPItransfer(address & 0x00FF);
    // SpiInOut( &SX126x.Spi, RADIO_WRITE_REGISTER );
    // SpiInOut( &SX126x.Spi, ( address & 0xFF00 ) >> 8 );
    // SpiInOut( &SX126x.Spi, address & 0x00FF );
    
    for( uint16_t i = 0; i < size; i++ )
    {
        SPItransfer(buffer[i]);
        //SpiInOut( &SX126x.Spi, buffer[i] );
    }

    digitalWrite(RADIO_NSS,HIGH);
    //GpioWrite( &SX126x.Spi.Nss, 1 );

    //SPI.endTransaction();

    SX126xWaitOnBusy( );
}

void SX126xWriteRegister( uint16_t address, uint8_t value )
{
    SX126xWriteRegisters( address, &value, 1 );
}

void SX126xReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    SX126xCheckDeviceReady( );

    //SPI.beginTransaction(settings);
    digitalWrite(RADIO_NSS,LOW);
    //GpioWrite( &SX126x.Spi.Nss, 0 );

    SPItransfer(RADIO_READ_REGISTER);
    SPItransfer(( address & 0xFF00 ) >> 8);
    SPItransfer(address & 0x00FF);
    SPItransfer(0x00);
    // SpiInOut( &SX126x.Spi, RADIO_READ_REGISTER );
    // SpiInOut( &SX126x.Spi, ( address & 0xFF00 ) >> 8 );
    // SpiInOut( &SX126x.Spi, address & 0x00FF );
    // SpiInOut( &SX126x.Spi, 0 );
    for( uint16_t i = 0; i < size; i++ )
    {
        buffer[i] = SPItransfer(0x00);
        //buffer[i] = SpiInOut( &SX126x.Spi, 0 );
    }

    digitalWrite(RADIO_NSS,HIGH);
    //GpioWrite( &SX126x.Spi.Nss, 1 );

    //SPI.endTransaction();

    SX126xWaitOnBusy( );
}

uint8_t SX126xReadRegister( uint16_t address )
{
    uint8_t data;
    SX126xReadRegisters( address, &data, 1 );
    return data;
}

void SX126xWriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    SX126xCheckDeviceReady( );

    //SPI.beginTransaction(settings);
    digitalWrite(RADIO_NSS,LOW);
    //GpioWrite( &SX126x.Spi.Nss, 0 );

    SPItransfer(RADIO_WRITE_BUFFER);
    SPItransfer(offset);
    // SpiInOut( &SX126x.Spi, RADIO_WRITE_BUFFER );
    // SpiInOut( &SX126x.Spi, offset );
    for( uint16_t i = 0; i < size; i++ )
    {
        SPItransfer(buffer[i]);
        // SpiInOut( &SX126x.Spi, buffer[i] );
    }
    digitalWrite(RADIO_NSS,HIGH);
    //GpioWrite( &SX126x.Spi.Nss, 1 );

    //SPI.endTransaction();

    SX126xWaitOnBusy( );
}

void SX126xReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    SX126xCheckDeviceReady( );

    //SPI.beginTransaction(settings);
    digitalWrite(RADIO_NSS,LOW);
    //GpioWrite( &SX126x.Spi.Nss, 0 );

    SPItransfer(RADIO_READ_BUFFER);
    SPItransfer(offset);
    SPItransfer(0x00);
    // SpiInOut( &SX126x.Spi, RADIO_READ_BUFFER );
    // SpiInOut( &SX126x.Spi, offset );
    // SpiInOut( &SX126x.Spi, 0 );
    for( uint16_t i = 0; i < size; i++ )
    {
        buffer[i] = SPItransfer(0x00);
        // buffer[i] = SpiInOut( &SX126x.Spi, 0 );
    }
    digitalWrite(RADIO_NSS,HIGH);
    // GpioWrite( &SX126x.Spi.Nss, 1 );

    //SPI.endTransaction();

    SX126xWaitOnBusy( );
}

void SX126xSetRfTxPower( int8_t power )
{
    SX126xSetTxParams( power, RADIO_RAMP_40_US );
}

uint8_t SX126xGetPaSelect( uint32_t channel )
{
//     if( GpioRead( &DeviceSel ) == 1 )
//     {
//         return SX1261;
//     }
//     else
//     {
//         return SX1262;
//     }
  return SX1261; 
}

void SX126xAntSwOn( void )
{
  //pinMode(RADIO_DEVICE_SEL , OUTPUT);
  //digitalWrite(RADIO_DEVICE_SEL, HIGH);
    // GpioInit( &AntPow, RADIO_ANT_SWITCH_POWER, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
}

void SX126xAntSwOff( void )
{
  //pinMode(RADIO_DEVICE_SEL, INPUT);
    // GpioInit( &AntPow, RADIO_ANT_SWITCH_POWER, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
}

bool SX126xCheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}

void BoardDisableIrq( void )
{
  
}

void BoardEnableIrq( void )
{
  
}

