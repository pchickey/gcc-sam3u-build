// -*-  tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: t -*-
//
// RTOS concurrent serial transmit/recieve library, for use with
// FreeRTOS kernel. Built and tested with the ArduinoFreeRTOS library.
// Pat Hickey, Dec 2011
//
// Interrupt-driven serial transmit/receive library.
//
//      Copyright (c) 2010 Michael Smith. All rights reserved.
//
// Receive and baudrate calculations derived from the Arduino
// HardwareSerial driver:
//
//      Copyright (c) 2006 Nicholas Zambetti.  All right reserved.
//
// Transmit algorithm inspired by work:
//
//      Code Jose Julio and Jordi Munoz. DIYDrones.com
//
//      This library is free software; you can redistribute it and/or
//      modify it under the terms of the GNU Lesser General Public
//      License as published by the Free Software Foundation; either
//      version 2.1 of the License, or (at your option) any later version.
//
//      This library is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//      Lesser General Public License for more details.
//
//      You should have received a copy of the GNU Lesser General Public
//      License along with this library; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

// XXX hack until need for AVR init code is removed:
#define F_CPU 16000000UL

//#include "../AP_Common/AP_Common.h"
#include "RTOSSerial.h"
extern "C" {
#include <queue.h>
}
// #include "WProgram.h"

#if   defined(UDR3)
# define FS_MAX_PORTS   4
#elif defined(UDR2)
# define FS_MAX_PORTS   3
#elif defined(UDR1)
# define FS_MAX_PORTS   2
#else
# define FS_MAX_PORTS   1
#endif

xQueueHandle __RTOSSerial__rxQueue[FS_MAX_PORTS];
xQueueHandle __RTOSSerial__txQueue[FS_MAX_PORTS];
uint8_t RTOSSerial::_serialInitialized = 0;

// Constructor /////////////////////////////////////////////////////////////////

RTOSSerial::RTOSSerial(const uint8_t portNumber, volatile uint8_t *ubrrh, volatile uint8_t *ubrrl,
					   volatile uint8_t *ucsra, volatile uint8_t *ucsrb, const uint8_t u2x,
					   const uint8_t portEnableBits, const uint8_t portTxBits) :
					   _ubrrh(ubrrh),
					   _ubrrl(ubrrl),
					   _ucsra(ucsra),
					   _ucsrb(ucsrb),
					   _u2x(u2x),
					   _portEnableBits(portEnableBits),
					   _portTxBits(portTxBits),
					   _rxQueue(&__RTOSSerial__rxQueue[portNumber]),
					   _txQueue(&__RTOSSerial__txQueue[portNumber]),
             _txQueueSpace(0)
{
	setInitialized(portNumber);
	begin(57600);
}

// Public Methods //////////////////////////////////////////////////////////////

void RTOSSerial::begin(long baud)
{
	begin(baud, 0, 0);
}

void RTOSSerial::begin(long baud, unsigned int rxSpace, unsigned int txSpace)
{
	uint16_t ubrr;
	bool use_u2x = true;

	// if we are currently open...
	if (_open) {
    // If the caller wants to preserve the buffer sizing, work out what
    // it currently is...
    if (0 == rxSpace)
      rxSpace = _rxQueueSpace;
    if (0 == txSpace)
      txSpace = _txQueueSpace;
		// close the port in its current configuration, clears _open
		end();
	}
  _rxQueueSpace = rxSpace ? : _default_rx_buffer_size ;
  _txQueueSpace = txSpace ? : _default_tx_buffer_size ;
  *_rxQueue = xQueueCreate( _rxQueueSpace, sizeof( uint8_t ));
  *_txQueue = xQueueCreate( _txQueueSpace, sizeof( uint8_t ));
  if (*_rxQueue == NULL || *_txQueue == NULL ) {
    end();
    return; // couldn't allocate queues - fatal
  }

	// mark the port as open
	_open = true;

	// If the user has supplied a new baud rate, compute the new UBRR value.
	if (baud > 0) {
#if F_CPU == 16000000UL
		// hardcoded exception for compatibility with the bootloader shipped
		// with the Duemilanove and previous boards and the firmware on the 8U2
		// on the Uno and Mega 2560.
		if (baud == 57600)
			use_u2x = false;
#endif

		if (use_u2x) {
			*_ucsra = 1 << _u2x;
			ubrr = (F_CPU / 4 / baud - 1) / 2;
		} else {
			*_ucsra = 0;
			ubrr = (F_CPU / 8 / baud - 1) / 2;
		}

		*_ubrrh = ubrr >> 8;
		*_ubrrl = ubrr;
	}

	*_ucsrb |= _portEnableBits;
}

void RTOSSerial::end()
{
	*_ucsrb &= ~(_portEnableBits | _portTxBits);

  vQueueDelete(*_rxQueue);
  vQueueDelete(*_txQueue);
	_open = false;
}

int RTOSSerial::available(void)
{
	if (!_open)
		return (-1);
  return uxQueueMessagesWaiting(*_rxQueue );
}

int RTOSSerial::txspace(void)
{
  unsigned portBASE_TYPE avail;
	if (!_open)
		return (-1);
  avail = uxQueueMessagesWaiting(*_rxQueue );
  return (int) (_txQueueSpace - avail);
}

int RTOSSerial::read(void)
{
	uint8_t c;

  if (!_open) return (-1);
  if ( xQueueReceive(*_rxQueue, &c, ( portTickType ) 0 ) != errQUEUE_EMPTY) {
    return (int) c;
  }
  return (-1);
}

int RTOSSerial::peek(void)
{
  uint8_t c;

  if (!_open) return (-1);
  if ( xQueuePeek (*_rxQueue, &c, ( portTickType ) 0 ) != errQUEUE_EMPTY) {
    return (int) c;
  }
  return (-1);
}

void RTOSSerial::flush(void)
{
  // Replaced with a no-op. Hopefully this does not cause problems.
}

size_t RTOSSerial::write(uint8_t c)
{
	uint16_t i;

	if (!_open) // drop bytes if not open
		return 0;

  xQueueSendToBack(*_txQueue, (void *) &c, portMAX_DELAY );

	// enable the data-ready interrupt, as it may be off if the buffer is empty
	*_ucsrb |= _portTxBits;
  return 1;
}

