/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support 
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

//------------------------------------------------------------------------------
//      Includes
//------------------------------------------------------------------------------

#include "MEDFlash.h"
#include <utility/trace.h>
#include <utility/assert.h>

#include <memories/flash/flashd.h>

#if !defined (CHIP_FLASH_EFC) && !defined (CHIP_FLASH_EEFC)
#error eefc/efc not supported
#endif

//------------------------------------------------------------------------------
//      Local definitions
//------------------------------------------------------------------------------

// Missing FRDY bit on the SAM7A3
#if defined(at91sam7a3)
    #define AT91C_MC_FRDY   (AT91C_MC_EOP | AT91C_MC_EOL)
#endif

//------------------------------------------------------------------------------
//      Internal Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! \brief  Reads a specified amount of data from a flash memory
//! \param  media    Pointer to a Media instance
//! \param  address  Address of the data to read
//! \param  data     Pointer to the buffer in which to store the retrieved
//!                   data
//! \param  length   Length of the buffer
//! \param  callback Optional pointer to a callback function to invoke when
//!                   the operation is finished
//! \param  argument Optional pointer to an argument for the callback
//! \return Operation result code
//------------------------------------------------------------------------------
static unsigned char FLA_Read(Media      *media,
                               unsigned int address,
                               void         *data,
                               unsigned int length,
                               MediaCallback   callback,
                               void         *argument)
{
    unsigned char *source = (unsigned char *) address;
    unsigned char *dest = (unsigned char *) data;

    // Check that the media is ready
    if (media->state != MED_STATE_READY) {

        TRACE_INFO("Media busy\n\r");
        return MED_STATUS_BUSY;
    }

    // Check that the data to read is not too big
    if ((length + address) > media->size) {

        TRACE_WARNING("FLA_Read: Data too big\n\r");
        return MED_STATUS_ERROR;
    }

    // Enter Busy state
    media->state = MED_STATE_BUSY;

    // Read data
    while (length > 0) {

        *dest = *source;

        dest++;
        source++;
        length--;
    }

    // Leave the Busy state
    media->state = MED_STATE_READY;

    // Invoke callback
    if (callback != 0) {

        callback(argument, MED_STATUS_SUCCESS, 0, 0);
    }

    return MED_STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
//! \brief  Writes data on a flash media
//! \param  media    Pointer to a Media instance
//! \param  address  Address at which to write
//! \param  data     Pointer to the data to write
//! \param  length   Size of the data buffer
//! \param  callback Optional pointer to a callback function to invoke when
//!                   the write operation terminates
//! \param  argument Optional argument for the callback function
//! \return Operation result code
//! \see    Media
//! \see    Callback_f
//------------------------------------------------------------------------------
static unsigned char FLA_Write(Media      *media,
                                unsigned int address,
                                void         *data,
                                unsigned int length,
                                MediaCallback   callback,
                                void         *argument)
{
    unsigned char error;

    // Check that the media if ready
    if (media->state != MED_STATE_READY) {

        TRACE_WARNING("FLA_Write: Media is busy\n\r");
        return MED_STATUS_BUSY;
    }

    // Check that address is dword-aligned
    if (address%4 != 0) {

        TRACE_DEBUG("address = 0x%X\n\r", address);
        TRACE_WARNING("FLA_Write: Address must be dword-aligned\n\r");
        return MED_STATUS_ERROR;
    }

    // Check that length is a multiple of 4
    if (length%4 != 0) {

        TRACE_WARNING("FLA_Write: Data length must be a multiple of 4 bytes\n\r");
        return MED_STATUS_ERROR;
    }

    // Check that the data to write is not too big
    if ((length + address) > media->size) {

        TRACE_WARNING("FLA_Write: Data too big\n\r");
        return MED_STATUS_ERROR;
    }

    // Put the media in Busy state
    media->state = MED_STATE_BUSY;

    // Initialize the transfer descriptor
    media->transfer.data = data;
    media->transfer.address = address;
    media->transfer.length = length;
    media->transfer.callback = callback;
    media->transfer.argument = argument;

    // Start the write operation
    error = FLASHD_Write( address, data, length);
    ASSERT(!error, "-F- Error when trying to write page (0x%02X)\n\r", error);

    // End of transfer
    // Put the media in Ready state
    media->state = MED_STATE_READY;

    // Invoke the callback if it exists
    if (media->transfer.callback != 0) {

        media->transfer.callback(media->transfer.argument, 0, 0, 0);
    }

    return MED_STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
//      Exported Functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//! \brief  Initializes a Media instance and the associated physical interface
//! \param  media Pointer to the Media instance to initialize
//! \param  efc   Pointer to AT91S_EFC interface.
//! \see    Media
//------------------------------------------------------------------------------
void FLA_Initialize(Media *media, AT91S_EFC *efc)
{
    TRACE_INFO("Flash init\n\r");

    // Initialize media fields
    media->write = FLA_Write;
    media->read = FLA_Read;
    media->lock = FLASHD_Lock;
    media->unlock = FLASHD_Unlock;
    media->flush = 0;
    media->handler = 0;

    media->blockSize = 1;
    media->baseAddress = 0; // Address based on whole memory space
    media->size = AT91C_IFLASH_SIZE;
    media->interface = efc;

    media->mappedRD  = 0;
    media->mappedWR  = 0;
    media->protected = 0;
    media->removable = 0;
    media->state = MED_STATE_READY;

    media->transfer.data = 0;
    media->transfer.address = 0;
    media->transfer.length = 0;
    media->transfer.callback = 0;
    media->transfer.argument = 0;

    // Initialize low-level interface
    // Configure Flash Mode register
    efc->EFC_FMR |= (BOARD_MCK / 666666) << 16;
}


