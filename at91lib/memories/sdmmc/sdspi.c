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
//         Headers
//------------------------------------------------------------------------------

#include "sdspi.h"
#include <utility/assert.h>
#include <utility/trace.h>
#include <board.h>
#include <crc7.h>
#include <crc-itu-t.h>
#include <crc16.h>
#include <crc-ccitt.h>
#include <string.h>

//------------------------------------------------------------------------------
//         Macros
//------------------------------------------------------------------------------

/// Transfer is pending.
#define SDSPI_STATUS_PENDING      1
/// Transfer has been aborted because an error occured.
#define SDSPI_STATUS_ERROR        2

/// SPI driver is currently in use.
#define SDSPI_ERROR_LOCK    1

// Data Tokens
#define SDSPI_START_BLOCK_1 0xFE  // Single/Multiple read, single write
#define SDSPI_START_BLOCK_2 0xFC  // Multiple block write
#define SDSPI_STOP_TRAN     0xFD  // Cmd12

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Initializes the SD Spi structure and the corresponding SPI hardware.
/// \param pSpid  Pointer to a Spid instance.
/// \param pSpiHw  Associated SPI peripheral.
/// \param spiId  SPI peripheral identifier.
//------------------------------------------------------------------------------
void SDSPI_Configure(SdSpi *pSdSpi,
                     AT91PS_SPI pSpiHw,
                     unsigned char spiId)
{
    // Initialize the SPI structure
    pSdSpi->pSpiHw = pSpiHw;
    pSdSpi->spiId = spiId;
    pSdSpi->semaphore = 1;

    // Enable the SPI clock
    AT91C_BASE_PMC->PMC_PCER = (1 << pSdSpi->spiId);
    
    // Execute a software reset of the SPI twice
    pSpiHw->SPI_CR = AT91C_SPI_SWRST;
    pSpiHw->SPI_CR = AT91C_SPI_SWRST;

    // Configure SPI in Master Mode with No CS selected !!!
    pSpiHw->SPI_MR = AT91C_SPI_MSTR | AT91C_SPI_MODFDIS  | AT91C_SPI_PCS;

    // Disables the receiver PDC transfer requests
    // Disables the transmitter PDC transfer requests.
    pSpiHw->SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;

    // Enable the SPI
    pSpiHw->SPI_CR = AT91C_SPI_SPIEN;

    // Disable the SPI clock
    AT91C_BASE_PMC->PMC_PCDR = (1 << pSdSpi->spiId);
}

//------------------------------------------------------------------------------
/// Configures the parameters for the device corresponding to the cs.
/// \param pSdSpi  Pointer to a SdSpi instance.
/// \param cs  number corresponding to the SPI chip select.
/// \param csr  SPI_CSR value to setup.
//------------------------------------------------------------------------------
void SDSPI_ConfigureCS(SdSpi *pSdSpi, unsigned char cs, unsigned int csr)
{
    unsigned int spiMr;
    AT91S_SPI *pSpiHw = pSdSpi->pSpiHw;

    // Enable the SPI clock
    AT91C_BASE_PMC->PMC_PCER = (1 << pSdSpi->spiId);

    //TRACE_DEBUG("CSR[%d]=0x%8X\n\r", cs, csr);
    pSpiHw->SPI_CSR[cs] = csr;

//jcb to put in sendcommand
    // Write to the MR register
    spiMr = pSpiHw->SPI_MR;
    spiMr |= AT91C_SPI_PCS;
    spiMr &= ~((1 << cs) << 16);
    pSpiHw->SPI_MR = spiMr;

    // Disable the SPI clock
    AT91C_BASE_PMC->PMC_PCDR = (1 << pSdSpi->spiId);
}

//------------------------------------------------------------------------------
/// Use PDC for SPI data transfer.
/// Return 0 if no error, otherwise return error status.
/// \param pSdSpi  Pointer to a SdSpi instance.
/// \param pData  Data pointer.
/// \param size  Data transfer byte count.
//------------------------------------------------------------------------------
unsigned char SDSPI_PDC(SdSpi *pSdSpi, unsigned char *pData, unsigned int size)
{
    AT91PS_SPI pSpiHw = pSdSpi->pSpiHw;
    unsigned int spiIer;

    if (pSdSpi->semaphore == 0) {
        TRACE_DEBUG("No semaphore\n\r");
        return SDSPI_ERROR_LOCK;
    }
    pSdSpi->semaphore--;

    // Enable the SPI clock
    AT91C_BASE_PMC->PMC_PCER = (1 << pSdSpi->spiId);

    // Disable transmitter and receiver
    pSpiHw->SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;

    // Receive Pointer Register
    pSpiHw->SPI_RPR = (int)pData;
    // Receive Counter Register
    pSpiHw->SPI_RCR = size;
    // Transmit Pointer Register
    pSpiHw->SPI_TPR = (int) pData;
    // Transmit Counter Register
    pSpiHw->SPI_TCR = size;

    spiIer = AT91C_SPI_RXBUFF;

    // Enable transmitter and receiver
    pSpiHw->SPI_PTCR = AT91C_PDC_RXTEN | AT91C_PDC_TXTEN;

    // Interrupt enable shall be done after PDC TXTEN and RXTEN
    pSpiHw->SPI_IER = spiIer;

    return 0;
}

//! Should be moved to a new file
//------------------------------------------------------------------------------
/// Read data on SPI data bus; 
/// Returns 1 if read fails, returns 0 if no error.
/// \param pSdSpi  Pointer to a SD SPI driver instance.
/// \param pData  Data pointer.
/// \param size Data size.
//------------------------------------------------------------------------------
unsigned char SDSPI_Read(SdSpi *pSdSpi, unsigned char *pData, unsigned int size)
{
    unsigned char error;

    // MOSI should hold high during read, or there will be wrong data in received data.
    memset(pData, 0xff, size);

    error = SDSPI_PDC(pSdSpi, pData, size);

    while(SDSPI_IsBusy(pSdSpi) == 1);

    if( error == 0 ) {
        return 0;
    }
    else {
        TRACE_DEBUG("PB SDSPI_Read\n\r");
        return 1;
    }
}

//------------------------------------------------------------------------------
/// Write data on SPI data bus; 
/// Returns 1 if write fails, returns 0 if no error.
/// \param pSdSpi  Pointer to a SD SPI driver instance.
/// \param pData  Data pointer.
/// \param size Data size.
//------------------------------------------------------------------------------
unsigned char SDSPI_Write(SdSpi *pSdSpi, unsigned char *pData, unsigned int size)
{
    unsigned char error;

    error = SDSPI_PDC(pSdSpi, pData, size);

    while(SDSPI_IsBusy(pSdSpi) == 1);

    if( error == 0 ) {
        return 0;
    }
    else {
        TRACE_DEBUG("PB SDSPI_Write\n\r");
        return 1;
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
unsigned char SDSPI_WaitDataBusy(SdSpi *pSdSpi)
{
    unsigned char busyData;

    SDSPI_Read(pSdSpi, &busyData, 1);

    if (busyData != 0xff) {
        return 1;
    }
    else {
        return 0;
    }
}

//------------------------------------------------------------------------------
/// Convert SD MCI command to a SPI mode command token.
/// \param pCmdToken Pointer to the SD command token.
/// \param arg    SD command argument
//------------------------------------------------------------------------------
void SDSPI_MakeCmd(unsigned char *pCmdToken, unsigned int arg)
{
    unsigned char sdCmdNum;
    unsigned char crc = 0;
    unsigned char crcPrev = 0;

    sdCmdNum = 0x3f & *pCmdToken;
    *pCmdToken = sdCmdNum | 0x40;
    *(pCmdToken+1) = (arg >> 24) & 0xff;
    *(pCmdToken+2) = (arg >> 16) & 0xff;
    *(pCmdToken+3) = (arg >> 8) & 0xff;
    *(pCmdToken+4) = arg & 0xff;

    crc = crc7(crcPrev, (unsigned char *)(pCmdToken), 5);

    *(pCmdToken+5) = (crc << 1) | 1;
}

//------------------------------------------------------------------------------
/// Get response after send SD command.
/// Return 0 if no error, otherwise indicate an error.
/// \param pSdSpi Pointer to the SD SPI instance.
/// \param pCommand  Pointer to the SD command
//------------------------------------------------------------------------------
unsigned char SDSPI_GetCmdResp(SdSpi *pSdSpi, SdSpiCmd *pCommand)
{
    unsigned char resp[8];  // response 
    unsigned char error;
    unsigned int  respRetry = 8; //NCR max 8, refer to card datasheet

    memset(resp, 0, 8);

    // Wait for response start bit. 
    do {
        error = SDSPI_Read(pSdSpi, &resp[0], 1);
        if (error) {
            TRACE_DEBUG("\n\rpb SDSPI_GetCmdResp: 0x%X\n\r", error);
            return error;
        }
        if ((resp[0]&0x80) == 0) {
            break;
        }
        respRetry--;
    } while(respRetry > 0);

    switch (pCommand->resType) {
        case 1:
        *(pCommand->pResp) = resp[0];
        break;

        case 2:
        error = SDSPI_Read(pSdSpi, &resp[1], 1);
        if (error) {
            return error;
        }
        *(pCommand->pResp) = resp[0]
                          | (resp[1] << 8);
        break;

        // Response 3, get OCR
        case 3:
        error = SDSPI_Read(pSdSpi, &resp[1], 4);
        if (error) {
            return error;
        }
        *(pCommand->pResp) = resp[0] 
                          | (resp[1] << 8) 
                          | (resp[2] << 16)
                          | (resp[3] << 24);
        *(pCommand->pResp+1) = resp[4];
        break;

        case 7:
        TRACE_DEBUG("case 7\n\r");
        error = SDSPI_Read(pSdSpi, &resp[1], 4);
        if (error) {
            return error;
        }
        *(pCommand->pResp) = resp[0]
                          | (resp[1] << 8) 
                          | (resp[2] << 16)
                          | (resp[3] << 24);
        *(pCommand->pResp+1) = resp[4];
        break;

        default:
        TRACE_DEBUG("PB default\n\r");
        break;
    }

    return 0;
}

//------------------------------------------------------------------------------
/// Get response after send data.
/// Return 0 if no error, otherwise indicate an error.
/// \param pSdSpi Pointer to the SD SPI instance.
/// \param pCommand  Pointer to the SD command
//------------------------------------------------------------------------------
unsigned char SDSPI_GetDataResp(SdSpi *pSdSpi, SdSpiCmd *pCommand)
{
    unsigned char resp = 0;  // response 
    unsigned char error;
    unsigned int respRetry = 18; //NCR max 8, refer to card datasheet

    // Wait for response start bit. 
    do {
        error = SDSPI_Read(pSdSpi, &resp, 1);
        if (error) {
            return error;
        }
        if (((resp & 0x11) == 0x1) || ((resp & 0xf0) == 0))
            break;

        respRetry--;
    } while(respRetry > 0);
    //TRACE_DEBUG("SDSPI_GetDataResp 0x%X\n\r",resp);
    return resp;
}

//------------------------------------------------------------------------------
/// Starts a SPI master transfer. This is a non blocking function. It will
/// return as soon as the transfer is started.
/// Returns 0 if the transfer has been started successfully; otherwise returns
/// error.
/// \param pSdSpi  Pointer to a SdSpi instance.
/// \param pCommand Pointer to the SPI command to execute.
//------------------------------------------------------------------------------
unsigned char SDSPI_SendCommand(SdSpi *pSdSpi, SdSpiCmd *pCommand)
{
    AT91S_SPI *pSpiHw = pSdSpi->pSpiHw;
    unsigned char CmdToken[6];
    unsigned char *pData;
    unsigned int blockSize;
    unsigned int i;
    unsigned char error;
    unsigned char dataHeader;
    unsigned int dataRetry1 = 100;
    unsigned int dataRetry2 = 100;
    unsigned char crc[2];
    unsigned char crcPrev = 0;
    unsigned char crcPrev2 = 0;

    SANITY_CHECK(pSdSpi);
    SANITY_CHECK(pSpiHw);
    SANITY_CHECK(pCommand);

    CmdToken[0] = pCommand->cmd & 0x3F;
    pData = pCommand->pData;
    blockSize = pCommand->blockSize;

    SDSPI_MakeCmd((unsigned char *)&CmdToken, pCommand->arg);

    // Command is now being executed
    pSdSpi->pCommand = pCommand;
    pCommand->status = SDSPI_STATUS_PENDING;

    // Send the command
    if((pCommand->conTrans == SPI_NEW_TRANSFER) || (blockSize == 0)) {

        for(i = 0; i < 6; i++) {
            error = SDSPI_Write(pSdSpi, &CmdToken[i], 1);
            if (error) {
                TRACE_DEBUG("Error: %d\n\r", error);
                return error;
            }
        }
        // Specific for Cmd12()
        if ((pCommand->cmd & 0x3F) == 12) {
            if( 1 == SDSPI_Wait(pSdSpi, 2) ) {
                TRACE_DEBUG("Pb Send command 12\n\r");
            }
        }
        if (pCommand->pResp) {
            error = SDSPI_GetCmdResp(pSdSpi, pCommand);
            if (error) {
                TRACE_DEBUG("Error: %d\n\r", error);
                return error;
            }
        }
    }

    if( (blockSize > 0) && (pCommand->nbBlock == 0) ) {
        pCommand->nbBlock = 1;
    }

    // For data block operations
    while (pCommand->nbBlock > 0) {

        // If data block size is invalid, return error
        if (blockSize == 0) {
            TRACE_DEBUG("Block Size = 0\n\r");
            return 1;
        }

        // DATA transfer from card to host
        if (pCommand->isRead) {
            do {
                SDSPI_Read(pSdSpi, &dataHeader, 1);
                dataRetry1 --;
                if (dataHeader == SDSPI_START_BLOCK_1) {
                    break;
                }
                else if((dataHeader & 0xf0) == 0x00) {
                    pCommand->status = SDSPI_STATUS_ERROR;
                    TRACE_DEBUG("Data Error 0x%X!\n\r", dataHeader);
                    return 1;
                }
            } while(dataRetry1 > 0);

            if (dataRetry1 == 0) {
                TRACE_DEBUG("Timeout dataretry1\n\r");
                return 1;
            }

            SDSPI_Read(pSdSpi, pData, blockSize);

            // Specific for Cmd9()
            if ((pCommand->cmd & 0x3f) != 0x9) {

                SDSPI_Read(pSdSpi, crc, 2);
#ifdef SDSPI_CRC_ON
                // Check data CRC
                TRACE_DEBUG("Check Data CRC\n\r");
                crcPrev = 0;
                crcPrev2 = 0;
                if (crc[0] != ((crc_itu_t(crcPrev, pData, blockSize) & 0xff00) >> 8 )
                 || crc[1] !=  (crc_itu_t(crcPrev2, pData, blockSize) & 0xff)) {
                    TRACE_ERROR("CRC error 0x%X 0x%X 0x%X\n\r", \
                        crc[0], crc[1], crc_itu_t(pData, blockSize));
                    return 1;
                }
#endif
            }
        }

        // DATA transfer from host to card
        else {
            SDSPI_NCS(pSdSpi);
            if ((pCommand->conTrans == SPI_CONTINUE_TRANSFER) || ((pCommand->cmd & 0x3f) == 25)) {
                dataHeader = SDSPI_START_BLOCK_2;
            }
            else {
                dataHeader = SDSPI_START_BLOCK_1;
            }

            crcPrev = 0;
            crc[0] = (crc_itu_t(crcPrev, pData, blockSize) & 0xff00) >> 8;
            crcPrev2 = 0;
            crc[1] = (crc_itu_t(crcPrev2, pData, blockSize) & 0xff);
            SDSPI_Write(pSdSpi, &dataHeader, 1);
            SDSPI_Write(pSdSpi, pData, blockSize);
            SDSPI_Write(pSdSpi, crc, 2);

            // If status bits in data response is not "data accepted", return error
            if ((SDSPI_GetDataResp(pSdSpi, pCommand) & 0xe) != 0x4) {
                TRACE_ERROR("Write resp error!\n\r");
                return 1;
            }

            do {
                if (SDSPI_WaitDataBusy(pSdSpi) == 0) {
                    break;
                }
                dataRetry2--;
            } while(dataRetry2 > 0);
        }
        pData += blockSize;
        pCommand->nbBlock--;
    }

    if (pCommand->status == SDSPI_STATUS_PENDING) {
        pCommand->status = 0;
    }

    //TRACE_DEBUG("end SDSPI_SendCommand\n\r");
    return 0;
}
//!

//------------------------------------------------------------------------------
/// The SPI_Handler must be called by the SPI Interrupt Service Routine with the
/// corresponding Spi instance.
/// The SPI_Handler will unlock the Spi semaphore and invoke the upper application 
/// callback.
/// \param pSdSpi  Pointer to a SdSpi instance.
//------------------------------------------------------------------------------
void SDSPI_Handler(SdSpi *pSdSpi)
{
    SdSpiCmd *pCommand = pSdSpi->pCommand;
    AT91S_SPI *pSpiHw = pSdSpi->pSpiHw;
    volatile unsigned int spiSr;

    // Read the status register
    spiSr = pSpiHw->SPI_SR;
    if(spiSr & AT91C_SPI_RXBUFF) {

        if (pCommand->status == SDSPI_STATUS_PENDING) {
            pCommand->status = 0;
        }
        // Disable transmitter and receiver
        pSpiHw->SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;

        // Disable the SPI clock
        AT91C_BASE_PMC->PMC_PCDR = (1 << pSdSpi->spiId);

        // Disable buffer complete interrupt
        pSpiHw->SPI_IDR = AT91C_SPI_RXBUFF | AT91C_SPI_ENDTX;

        // Release the SPI semaphore
        pSdSpi->semaphore++;
    }

    // Invoke the callback associated with the current command
    if (pCommand && pCommand->callback) {
        pCommand->callback(0, pCommand);
    }
}

//------------------------------------------------------------------------------
/// Returns 1 if the given SPI transfer is complete; otherwise returns 0.
/// \param pCommand  Pointer to a SdSpiCmd instance.
//------------------------------------------------------------------------------
unsigned char SDSPI_IsTxComplete(SdSpiCmd *pCommand)
{
    if (pCommand->status != SDSPI_STATUS_PENDING) {
        if (pCommand->status != 0){
            TRACE_DEBUG("SPI_IsTxComplete %d\n\r", pCommand->status);
        }
        return 1;
    }
    else {
        return 0;
    }
}

//------------------------------------------------------------------------------
/// Close a SPI driver instance and the underlying peripheral.
/// \param pSdSpi  Pointer to a SD SPI driver instance.
//------------------------------------------------------------------------------
void SDSPI_Close(SdSpi *pSdSpi)
{
    AT91S_SPI *pSpiHw = pSdSpi->pSpiHw;

    SANITY_CHECK(pSdSpi);
    SANITY_CHECK(pSpiHw);

    // Enable the SPI clock
    AT91C_BASE_PMC->PMC_PCER = (1 << pSdSpi->spiId);

    // Disable the PDC transfer    
    pSpiHw->SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;

    // Enable the SPI
    pSpiHw->SPI_CR = AT91C_SPI_SPIDIS;

    // Disable the SPI clock
    AT91C_BASE_PMC->PMC_PCDR = (1 << pSdSpi->spiId);

    // Disable all the interrupts
    pSpiHw->SPI_IDR = 0xFFFFFFFF;
}

//------------------------------------------------------------------------------
/// Returns 1 if the SPI driver is currently busy programming; 
/// otherwise returns 0.
/// \param pSdSpi  Pointer to a SD SPI driver instance.
//------------------------------------------------------------------------------
unsigned char SDSPI_IsBusy(SdSpi *pSdSpi)
{
    if (pSdSpi->semaphore == 0) {
        return 1;
    }
    else {
        return 0;
    }
}

//------------------------------------------------------------------------------
/// Wait several cycles on SPI bus; 
/// Returns 0 to indicates no error, otherwise return 1.
/// \param pSdSpi  Pointer to a SD SPI driver instance.
/// \param cycles  Wait data cycles.
//------------------------------------------------------------------------------
unsigned char SDSPI_Wait(SdSpi *pSdSpi, unsigned int cycles)
{
    unsigned int i = cycles;
    unsigned char data = 0xff;

    for (; i > 0; i--) {
        if (SDSPI_Read(pSdSpi, &data, 1)) {
            return 1;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
/// Send stop transfer data token;
/// Returns 0 to indicates no error, otherwise return 1.
/// \param pSdSpi  Pointer to a SD SPI driver instance.
//------------------------------------------------------------------------------
unsigned char SDSPI_StopTranToken(SdSpi *pSdSpi)
{
    unsigned char stopToken = SDSPI_STOP_TRAN;

    TRACE_DEBUG("SDSPI_StopTranToken\n\r");
    return SDSPI_Write(pSdSpi, &stopToken, 1);
}

//------------------------------------------------------------------------------
/// Wait, SD card Ncs cycles; 
/// Returns 0 to indicates no error, otherwise return 1.
/// \param pSdSpi  Pointer to a SD SPI driver instance.
//------------------------------------------------------------------------------
unsigned char SDSPI_NCS(SdSpi *pSdSpi)
{
    unsigned int i;
    unsigned char ncs;

    for(i = 0; i < 15; i++) {
        ncs = 0xff;
        if (SDSPI_Write(pSdSpi, &ncs, 1)) {
            return 1;
        }
    }
    return 0;
}


