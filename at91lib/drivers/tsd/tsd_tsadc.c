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

#include <board.h>

#ifdef AT91C_BASE_TSADC

#include "tsd.h"
#include "tsd_com.h"
#include <irq/irq.h>
#include <pio/pio.h>
#include <tsadcc/tsadcc.h>
#include <drivers/lcd/lcdd.h>
#include <drivers/lcd/draw.h>
#include <drivers/lcd/font.h>

#include <string.h>

//------------------------------------------------------------------------------
//         Local definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//         Local types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//         Local variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//         External functions
//------------------------------------------------------------------------------

extern void TSD_PenPressed(unsigned int x, unsigned int y);
extern void TSD_PenMoved(unsigned int x, unsigned int y);
extern void TSD_PenReleased(unsigned int x, unsigned int y);

//------------------------------------------------------------------------------
//         Local functions
//------------------------------------------------------------------------------
extern void TSD_GetRawMeasurement(unsigned int *pData);

//------------------------------------------------------------------------------
/// Interrupt handler for the TSADC. Handles pen press, pen move and pen release
/// events by invoking three callback functions.
//------------------------------------------------------------------------------
static void InterruptHandler(void)
{
    unsigned int status;
    unsigned int data[2];
    static unsigned int newPoint[2];
    static unsigned int point[2];
    static unsigned char report = 0;

    // Retrieve TADC status
    // Bug fix: two step operation so IAR doesn't complain that the order of
    // volatile access is unspecified.
    status = AT91C_BASE_TSADC->TSADC_SR;
    status &= AT91C_BASE_TSADC->TSADC_IMR;

    // Pen release
    if ((status & AT91C_TSADC_NOCNT) == AT91C_TSADC_NOCNT) {

        // Invoke PenReleased callback
        if(TSDCom_IsCalibrationOk()) {
            TSD_PenReleased(point[0], point[1]);
        }

        // Stop periodic trigger mode
        TSADCC_SetDebounceTime(BOARD_TOUCHSCREEN_DEBOUNCE);
        AT91C_BASE_TSADC->TSADC_IDR = AT91C_TSADC_EOC3 | AT91C_TSADC_NOCNT;

        TSADCC_SetTriggerMode(AT91C_TSADC_TRGMOD_PENDET_TRIGGER);
        TSD_GetRawMeasurement(data); // Clear data registers
        AT91C_BASE_TSADC->TSADC_SR;
        AT91C_BASE_TSADC->TSADC_IER = AT91C_TSADC_PENCNT;
    }
    // Pen press
    else if ((status & AT91C_TSADC_PENCNT) == AT91C_TSADC_PENCNT) {

        // Invoke PenPressed callback with (x,y) coordinates
        while ((AT91C_BASE_TSADC->TSADC_SR & AT91C_TSADC_EOC3) != AT91C_TSADC_EOC3);
        TSD_GetRawMeasurement(data);
        TSDCom_InterpolateMeasurement(data, point);

        // Invoke PenPressed callback
        if(TSDCom_IsCalibrationOk()) {
            TSD_PenPressed(point[0], point[1]);
        }

        // Configure TSADC for periodic trigger
        TSADCC_SetDebounceTime(10); // 1ns
        AT91C_BASE_TSADC->TSADC_IER = AT91C_TSADC_EOC3 | AT91C_TSADC_NOCNT;
        AT91C_BASE_TSADC->TSADC_IDR = AT91C_TSADC_PENCNT;
        TSADCC_SetTriggerPeriod(10000000); // 10ms
        TSADCC_SetTriggerMode(AT91C_TSADC_TRGMOD_PERIODIC_TRIGGER);
        report = 0;
    }
    // Pen move
    else if ((status & AT91C_TSADC_EOC3) == AT91C_TSADC_EOC3) {

        // Invoke callback with LAST value measured
        // Explanation: the very last value that will be measured (just as the
        // pen is released) might be corrupted and there is no way to know. So
        // we just discard it.

        if (report) {

            memcpy(point, newPoint, sizeof(newPoint));
            // Invoke PenMoved callback
            if(TSDCom_IsCalibrationOk()) {
                TSD_PenMoved(point[0], point[1]);
            }
            report = 0;
        }

        TSD_GetRawMeasurement(data);
        TSDCom_InterpolateMeasurement(data, newPoint);
        if ((newPoint[0] != point[0]) || (newPoint[1] != point[1])) {

            report = 1;
        }
    }
}

//------------------------------------------------------------------------------
//         Global functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Reads and store a touchscreen measurement in the provided array.
/// The value stored are:
///  - data[0] = CDR3 * 1024 / CDR2
///  - data[1] = CDR1 * 1024 / CDR0
/// \param pData  Array where the measurements will be stored
//------------------------------------------------------------------------------
void TSD_GetRawMeasurement(unsigned int *pData)
{
    pData[0] = (AT91C_BASE_TSADC->TSADC_CDR3 * 1024);
    pData[0] /= AT91C_BASE_TSADC->TSADC_CDR2;
    pData[1] = (AT91C_BASE_TSADC->TSADC_CDR1 * 1024);
    pData[1] /= AT91C_BASE_TSADC->TSADC_CDR0;
}

//------------------------------------------------------------------------------
/// Wait pen pressed
//------------------------------------------------------------------------------
void TSD_WaitPenPressed(void)
{
    // Wait for touch & end of conversion
    while ((AT91C_BASE_TSADC->TSADC_SR & AT91C_TSADC_PENCNT) != AT91C_TSADC_PENCNT);
    while ((AT91C_BASE_TSADC->TSADC_SR & AT91C_TSADC_EOC3) != AT91C_TSADC_EOC3);
}

//------------------------------------------------------------------------------
/// Wait pen released
//------------------------------------------------------------------------------
void TSD_WaitPenReleased(void)
{
    // Wait for contact loss
    while ((AT91C_BASE_TSADC->TSADC_SR & AT91C_TSADC_NOCNT) != AT91C_TSADC_NOCNT);
}

//------------------------------------------------------------------------------
/// Do calibration
/// \param pLcdBuffer  LCD buffer to use for displaying the calibration info.
/// \return 1 if calibration is Ok, 0 else
//------------------------------------------------------------------------------
unsigned char TSD_Calibrate(void *pLcdBuffer)
{
    unsigned char ret = 0;

    // Calibration is done only once
    if(TSDCom_IsCalibrationOk()) {
        return 1;
    }

    // Enable trigger
    TSADCC_SetTriggerMode(AT91C_TSADC_TRGMOD_PENDET_TRIGGER);

    // Do calibration
    ret = TSDCom_Calibrate(pLcdBuffer);

    // Disable trigger
    TSADCC_SetTriggerMode(AT91C_TSADC_TRGMOD_NO_TRIGGER);

    // Configure interrupt generation
    // Do it only if the calibration is Ok.
    if(ret) {
        TSADCC_SetTriggerMode(AT91C_TSADC_TRGMOD_PENDET_TRIGGER);
        IRQ_ConfigureIT(AT91C_ID_TSADC, 0, InterruptHandler);
        IRQ_EnableIT(AT91C_ID_TSADC);
        AT91C_BASE_TSADC->TSADC_IER = AT91C_TSADC_PENCNT;
    }

    return ret;
}

//------------------------------------------------------------------------------
/// Initializes the touchscreen driver and starts the calibration process. When
/// finished, the touchscreen is operational.
/// The configuration is taken from the board.h of the device being compiled.
///
/// Important: the LCD driver must have been initialized prior to calling this
/// function.
/// \param pLcdBuffer  LCD buffer to use for displaying the calibration info.
//------------------------------------------------------------------------------
void TSD_Initialize(void *pLcdBuffer)
{
    const Pin pins[] = {PINS_TSADCC};

    // Configuration
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_TSADC;
    PIO_Configure(pins, PIO_LISTSIZE(pins));
    TSADCC_SetOperatingMode(AT91C_TSADC_TSAMOD_TS_ONLY_MODE);
    TSADCC_SetPenDetect(1);
    TSADCC_SetAdcFrequency(BOARD_TOUCHSCREEN_ADCCLK, BOARD_MCK);
    TSADCC_SetStartupTime(BOARD_TOUCHSCREEN_STARTUP);
    TSADCC_SetTrackAndHoldTime(BOARD_TOUCHSCREEN_SHTIM);
    TSADCC_SetDebounceTime(BOARD_TOUCHSCREEN_DEBOUNCE);

    // Do calibration if the LCd buffer is not a null pointer
    if(pLcdBuffer) {
        // Calibration
        while (!TSD_Calibrate(pLcdBuffer));
    }
}

//------------------------------------------------------------------------------
/// Reset/stop the touchscreen
//------------------------------------------------------------------------------
void TSD_Reset(void)
{
    IRQ_DisableIT(AT91C_ID_TSADC);
    AT91C_BASE_PMC->PMC_PCDR = 1 << AT91C_ID_TSADC;
}

#endif //#ifdef AT91C_BASE_TSADC
