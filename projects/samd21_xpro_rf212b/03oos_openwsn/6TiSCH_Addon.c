/**
* Copyright (c) 2014 Atmel Corporation. All rights reserved. 
*  
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are met:
* 
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
* 
* 2. Redistributions in binary form must reproduce the above copyright notice, 
* this list of conditions and the following disclaimer in the documentation 
* and/or other materials provided with the distribution.
* 
* 3. The name of Atmel may not be used to endorse or promote products derived 
* from this software without specific prior written permission.  
* 
* 4. This software may only be redistributed and used in connection with an 
* Atmel microcontroller product.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* 
* 
*/

#include <stdint.h>

void mcu_smart_sleep_process(uint8_t state);

// the different states of the IEEE802.15.4e state machine
typedef enum {
	S_SLEEP                   = 0x00,   // ready for next slot
	// synchronizing
	S_SYNCLISTEN              = 0x01,   // listened for packet to synchronize to network
	S_SYNCRX                  = 0x02,   // receiving packet to synchronize to network
	S_SYNCPROC                = 0x03,   // processing packet just received
	// TX
	S_TXDATAOFFSET            = 0x04,   // waiting to prepare for Tx data
	S_TXDATAPREPARE           = 0x05,   // preparing for Tx data
	S_TXDATAREADY             = 0x06,   // ready to Tx data, waiting for 'go'
	S_TXDATADELAY             = 0x07,   // 'go' signal given, waiting for SFD Tx data
	S_TXDATA                  = 0x08,   // Tx data SFD received, sending bytes
	S_RXACKOFFSET             = 0x09,   // Tx data done, waiting to prepare for Rx ACK
	S_RXACKPREPARE            = 0x0a,   // preparing for Rx ACK
	S_RXACKREADY              = 0x0b,   // ready to Rx ACK, waiting for 'go'
	S_RXACKLISTEN             = 0x0c,   // idle listening for ACK
	S_RXACK                   = 0x0d,   // Rx ACK SFD received, receiving bytes
	S_TXPROC                  = 0x0e,   // processing sent data
	// RX
	S_RXDATAOFFSET            = 0x0f,   // waiting to prepare for Rx data
	S_RXDATAPREPARE           = 0x10,   // preparing for Rx data
	S_RXDATAREADY             = 0x11,   // ready to Rx data, waiting for 'go'
	S_RXDATALISTEN            = 0x12,   // idle listening for data
	S_RXDATA                  = 0x13,   // data SFD received, receiving more bytes
	S_TXACKOFFSET             = 0x14,   // waiting to prepare for Tx ACK
	S_TXACKPREPARE            = 0x15,   // preparing for Tx ACK
	S_TXACKREADY              = 0x16,   // Tx ACK ready, waiting for 'go'
	S_TXACKDELAY              = 0x17,   // 'go' signal given, waiting for SFD Tx ACK
	S_TXACK                   = 0x18,   // Tx ACK SFD received, sending bytes
	S_RXPROC                  = 0x19,   // processing received data
} ieee154e_state_t;

/* This sleep modes are common for all MCU types 
   Depending on the MAC state sleep modes must be selected
   If particular sleep modes are  not exist, it can be combined 
   with other possible sleep modes 
*/
typedef enum {
	BOARD_SLEEP_MODE0,
	BOARD_SLEEP_MODE1,
	BOARD_SLEEP_MODE2,
	BOARD_SLEEP_MODE3,
	BOARD_SLEEP_MODE4,
	BOARD_SLEEP_MODE5,
}board_sleepmode_t;

void board_sleep(board_sleepmode_t sleep_mode);
void radio_rfSleep(void);

/** \brief mcu_smart_sleep This function will enables the stack to enter the 
           sleep modes in smart way to save more power without affecting the 
		   stack functionality.  It chooses the sleep mode according to wakeup 
		   time as well as next state execution duration.     

    \param [in] None
    \return None
 */

void mcu_smart_sleep_process(uint8_t state)
{

   switch (state) 
   {
	   /* Sleep Mode 0 -  High Power Consumption */
	   case S_SYNCPROC:
	   case S_TXDATAREADY:
	   case S_TXDATADELAY:
	   case S_RXACKREADY:
	   case S_TXPROC:
	   case S_TXACKPREPARE:
	   case S_TXACKREADY:
	   case S_TXACKDELAY:
	   board_sleep(BOARD_SLEEP_MODE0);
	   break;
	   
	   
	   /* Sleep Mode 1 - Medium Power Consumption */
	   case S_SYNCRX:
	   case S_TXDATAOFFSET:
	   case S_TXDATAPREPARE:
	   case S_TXDATA:
	   case S_RXACKOFFSET:
	   case S_RXACKLISTEN:
	   case S_RXACK:
	   case S_RXDATAREADY:
	   case S_RXDATALISTEN:
	   case S_RXDATA:
	   case S_TXACKOFFSET:
	   case S_TXACK:
	   case S_RXPROC:
	   board_sleep(BOARD_SLEEP_MODE1);
	   break;
	   
	   /* Sleep Mode 2 - Low Power Consumption */
	   case S_SYNCLISTEN:
	   case S_RXACKPREPARE:
	   case S_RXDATAOFFSET:
	   case S_RXDATAPREPARE:
	   board_sleep(BOARD_SLEEP_MODE2);
	   break;
	   
	   /* Sleep Mode 3 - Ultra Low Power Consumption */
	   case S_SLEEP:
	 //  radio_rfSleep();
	   board_sleep(BOARD_SLEEP_MODE3);
	   break;
	   default:
	   board_sleep(BOARD_SLEEP_MODE0);
	   break;
   } 
}
