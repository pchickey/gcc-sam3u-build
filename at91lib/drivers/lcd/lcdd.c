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

#include "lcdd.h"
#include <board.h>
#include <lcd/lcd.h>
#include <pio/pio.h>

//------------------------------------------------------------------------------
//         Global functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Initializes the LCD controller using the board-specific parameters (stored
/// in the corresponding board.h). The LCD and DMA are not enabled by this
/// function; this is done during the first call to LCDD_DisplayBuffer.
//------------------------------------------------------------------------------
void LCDD_Initialize(void)
{
    const Pin pPins[] = {PINS_LCD};

    // Enable pins
    PIO_Configure(pPins, PIO_LISTSIZE(pPins));

    // Enable peripheral clock
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_LCDC;

#if defined(at91sam9g10)||defined(at91sam9261)
    AT91C_BASE_PMC->PMC_SCER = AT91C_PMC_HCK1;
#endif

    // Disable the LCD and the DMA
    LCD_DisableDma();
    LCD_Disable(0);

    // Configure the LCD controller
    LCD_SetPixelClock(BOARD_MCK, BOARD_LCD_PIXELCLOCK);
    LCD_SetDisplayType(BOARD_LCD_DISPLAYTYPE);
    LCD_SetScanMode(AT91C_LCDC_SCANMOD_SINGLESCAN);
    LCD_SetBitsPerPixel(BOARD_LCD_BPP);
    LCD_SetPolarities(BOARD_LCD_POLARITY_INVVD,
                      BOARD_LCD_POLARITY_INVFRAME,
                      BOARD_LCD_POLARITY_INVLINE,
                      BOARD_LCD_POLARITY_INVCLK,
                      BOARD_LCD_POLARITY_INVDVAL);
    LCD_SetClockMode(BOARD_LCD_CLOCKMODE);
    LCD_SetMemoryFormat((unsigned int) AT91C_LCDC_MEMOR_LITTLEIND);
    LCD_SetSize(BOARD_LCD_WIDTH, BOARD_LCD_HEIGHT);

    // Configure timings
    LCD_SetVerticalTimings(BOARD_LCD_TIMING_VFP,
                           BOARD_LCD_TIMING_VBP,
                           BOARD_LCD_TIMING_VPW,
                           BOARD_LCD_TIMING_VHDLY);
    LCD_SetHorizontalTimings(BOARD_LCD_TIMING_HBP,
                             BOARD_LCD_TIMING_HPW,
                             BOARD_LCD_TIMING_HFP);

    // Configure contrast (TODO functions)
    LCD_SetContrastPrescaler(AT91C_LCDC_PS_NOTDIVIDED);
    LCD_SetContrastPolarity(AT91C_LCDC_POL_POSITIVEPULSE);
    LCD_SetContrastValue(0x80);
    LCD_EnableContrast();

    // Configure DMA
    LCD_SetFrameSize(BOARD_LCD_FRAMESIZE);
    LCD_SetBurstLength(4);
}

//------------------------------------------------------------------------------
/// Displays the contents of the provided buffer on the LCD. The buffer is
/// provided as-is to the LCD DMA and is not copied.
/// If the LCD and DMA are not yet enabled, this function enables them.
/// \param pBuffer  Buffer to display.
/// \return The address of the previously displayed buffer.
//------------------------------------------------------------------------------
void * LCDD_DisplayBuffer(void *pBuffer)
{
    void *pOldBuffer;

    pOldBuffer = LCD_SetFrameBufferAddress(pBuffer);

    // Enable LCD & DMA if needed
    if ((AT91C_BASE_LCDC->LCDC_DMACON & AT91C_LCDC_DMAEN) != AT91C_LCDC_DMAEN) {

        LCD_EnableDma();
        LCD_Enable(0x0C);
    }

    return pOldBuffer;
}

//------------------------------------------------------------------------------
/// Shutdown the LCD
//------------------------------------------------------------------------------
void LCDD_Stop(void)
{
    // Enable peripheral clock
    AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_LCDC;

#if defined(at91sam9g10)||defined(at91sam9261)
    AT91C_BASE_PMC->PMC_SCDR = AT91C_PMC_HCK1;
#endif

    // Disable the LCD and the DMA
    LCD_DisableDma();
    LCD_Disable(0);
}
