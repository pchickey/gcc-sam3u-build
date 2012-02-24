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

#ifdef BOARD_TSC_ADS7843

#include "tsd.h"
#include "tsd_com.h"
#include <irq/irq.h>
#include <pio/pio.h>
#include <pio/pio_it.h>
#if defined(cortexm3)
#include <systick/systick.h>
#else
#include <pit/pit.h>
#endif
#include <ads7843/ads7843.h>
#include <drivers/lcd/lcdd.h>
#include <drivers/lcd/draw.h>
#include <drivers/lcd/font.h>

#include <string.h>

//------------------------------------------------------------------------------
//         Local definitions
//------------------------------------------------------------------------------

/// PIT period value in µseconds.
#define PIT_PERIOD          10000 // 10 ms

/// Delay for pushbutton debouncing (in PIT_PERIOD time-base).
#define DEBOUNCE_TIME       6 // PIT_PERIOD * 6 = 60 ms

/// Color of calibration points.
#define POINTS_COLOR        0x0000FF

/// Size in pixels of calibration points.
#define POINTS_SIZE         4

/// Maximum difference in pixels between the test point and the measured point.
#define POINTS_MAX_ERROR    5

//------------------------------------------------------------------------------
//         Local types
//------------------------------------------------------------------------------

/// pen state
typedef enum {
    STATE_PEN_RELEASED  = 0,
    STATE_PEN_PRESSED   = 1,
    STATE_PEN_DEBOUNCE  = 2
} e_pen_state;


//------------------------------------------------------------------------------
//         Local variables
//------------------------------------------------------------------------------

/// Pins used by Interrupt Signal for Touch Screen Controller
static const Pin pinPenIRQ  = PIN_TCS_IRQ;

/// Global timestamp in milliseconds since start of application.
static volatile unsigned int timestamp = 0;

/// last time when the pen is pressed on the touchscreen
static volatile unsigned int timePress = 0;

/// last time when the pen is released
static volatile unsigned int timeRelease = 0;

/// pen state
static volatile e_pen_state penState = STATE_PEN_RELEASED;

/// Touch screen initiallized flag 
static unsigned int tsInitFlag = 0;

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

#if defined(cortexm3)
//------------------------------------------------------------------------------
/// Timer handler for touch screen. Increments the timestamp counter.
/// Determine the state "Pen Pressed" or "Pen Released". To change state,
/// the penIRQ has to keep the same value during DEBOUNCE_TIME.
/// For AT91SAM3, SysTick interrupt should call it per 10ms.
//------------------------------------------------------------------------------
void TSD_TimerHandler(void)
{
    unsigned int data[2];
    static unsigned int point[2];

    if (!tsInitFlag) return;

    timestamp++;
    // Get the current position of the pen if penIRQ has low value (pen pressed)
    if (PIO_Get(&pinPenIRQ) == 0) {

        // Get the current position of the pressed pen
        if(TSDCom_IsCalibrationOk()) {
            TSD_GetRawMeasurement(data);
            TSDCom_InterpolateMeasurement(data, point);
        }

        // call the callback function
        if(penState == STATE_PEN_PRESSED) {
            if(TSDCom_IsCalibrationOk()) {
                TSD_PenMoved(point[0], point[1]);
            }
        }
    }

    // Determine the pen state
    if (PIO_Get(&pinPenIRQ) == 0) {

        // reinit the last time when release
        timeRelease = timestamp;

        if(penState == STATE_PEN_DEBOUNCE) {

            if( (timestamp - timePress) > DEBOUNCE_TIME) {

                // pen is pressed during an enough time : the state change
                penState = STATE_PEN_PRESSED;
                // call the callback function
                if(TSDCom_IsCalibrationOk()) {
                    TSD_PenPressed(point[0], point[1]);
                }
            }
        }
    }
    else {
        // reinit the last time when release
        timePress = timestamp;

        if(penState == STATE_PEN_DEBOUNCE) {

            if( (timestamp - timeRelease) > DEBOUNCE_TIME) {

                // pen is released during an enough time : the state change
                penState = STATE_PEN_RELEASED;
                // call the callback function
                if(TSDCom_IsCalibrationOk()) {
                    TSD_PenReleased(point[0], point[1]);
                }
            }
        }
    }
}

#else // For SAM7/SAM9

//------------------------------------------------------------------------------
/// Handler for PIT interrupt. Increments the timestamp counter.
/// Determine the state "Pen Pressed" or "Pen Released". To change state,
/// the penIRQ has to keep the same value during DEBOUNCE_TIME.
//------------------------------------------------------------------------------
static void ISR_Pit(void)
{
    unsigned int status;
    unsigned int data[2];
    static unsigned int point[2];

    // Read the PIT status register
    status = PIT_GetStatus() & AT91C_PITC_PITS;
    if (status != 0) {

        // Read the PIVR to acknowledge interrupt and get number of ticks
        timestamp += (PIT_GetPIVR() >> 20);
    }

    // Get the current position of the pen if penIRQ has low value (pen pressed)
    if (PIO_Get(&pinPenIRQ) == 0) {

        // Get the current position of the pressed pen
        PIO_DisableIt(&pinPenIRQ);
        ADS7843_GetPosition(&data[0], &data[1]);
        PIO_EnableIt(&pinPenIRQ);

        TSD_GetRawMeasurement(data);
        TSDCom_InterpolateMeasurement(data, point);

        // call the callback function
        if(penState == STATE_PEN_PRESSED) {
            if(TSDCom_IsCalibrationOk()) {
                TSD_PenMoved(point[0], point[1]);
            }
        }
    }

    // Determine the pen state
    if (PIO_Get(&pinPenIRQ) == 0) {

        // reinit the last time when release
        timeRelease = timestamp;

        if(penState == STATE_PEN_DEBOUNCE) {

            if( (timestamp - timePress) > DEBOUNCE_TIME) {

                // pen is pressed during an enough time : the state change
                penState = STATE_PEN_PRESSED;
                // call the callback function
                if(TSDCom_IsCalibrationOk()) {
                    TSD_PenPressed(point[0], point[1]);
                }
            }
        }
    }
    else {
        // reinit the last time when release
        timePress = timestamp;

        if(penState == STATE_PEN_DEBOUNCE) {

            if( (timestamp - timeRelease) > DEBOUNCE_TIME) {

                // pen is released during an enough time : the state change
                penState = STATE_PEN_RELEASED;
                // call the callback function
                if(TSDCom_IsCalibrationOk()) {
                    TSD_PenReleased(point[0], point[1]);
                }
            }
        }
    }
}
#endif

//------------------------------------------------------------------------------
/// Interrupt handler for Touchscreen.
//------------------------------------------------------------------------------
static void ISR_PenIRQ(void)
{
    // Check if the pen has been pressed
    if (!PIO_Get(&pinPenIRQ)) {

        if(penState == STATE_PEN_RELEASED) {

            timePress = timestamp;
            penState = STATE_PEN_DEBOUNCE;
        }
    }
    else {

        if(penState == STATE_PEN_PRESSED) {

            timeRelease = timestamp;
            penState = STATE_PEN_DEBOUNCE;
        }
    }
}

#if !defined(cortexm3)
//------------------------------------------------------------------------------
/// Configure the periodic interval timer to generate an interrupt every 10 ms
//------------------------------------------------------------------------------
static void ConfigurePit(void)
{
    // Initialize the PIT to the desired frequency
    PIT_Init(PIT_PERIOD, BOARD_MCK / 1000000);

    // Configure interrupt on PIT
    IRQ_DisableIT(AT91C_ID_SYS);
    IRQ_ConfigureIT(AT91C_ID_SYS, AT91C_AIC_PRIOR_LOWEST, ISR_Pit);
    IRQ_EnableIT(AT91C_ID_SYS);
    PIT_EnableIT();

    // Enable the pit
    PIT_Enable();
}
#endif

//-----------------------------------------------------------------------------
/// Configure PENIRQ for interrupt
//-----------------------------------------------------------------------------
void ConfigurePenIRQ(void)
{
    // Configure pios
    PIO_Configure(&pinPenIRQ, PIO_LISTSIZE(pinPenIRQ));

    // Initialize interrupts
#if defined(cortexm3)
    PIO_InitializeInterrupts(0);
#else    // For SAM7/SAM9
    PIO_InitializeInterrupts(AT91C_AIC_PRIOR_HIGHEST);
#endif
    PIO_ConfigureIt(&pinPenIRQ, (void (*)(const Pin *)) ISR_PenIRQ);

    // Enable the interrupt
    PIO_EnableIt(&pinPenIRQ);
}

//------------------------------------------------------------------------------
//         Global functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Reads and store a touchscreen measurement in the provided array.
/// \param pData  Array where the measurements will be stored
//------------------------------------------------------------------------------
void TSD_GetRawMeasurement(unsigned int *pData)
{
#if defined(cortexm3)
    // Get the current position of the pressed pen
    ADS7843_GetPosition(&pData[0], &pData[1]);
#else
    // Get the current position of the pressed pen
    PIO_DisableIt(&pinPenIRQ);
    ADS7843_GetPosition(&pData[0], &pData[1]);
    PIO_EnableIt(&pinPenIRQ);
#endif
}

//------------------------------------------------------------------------------
/// Wait pen pressed
//------------------------------------------------------------------------------
void TSD_WaitPenPressed(void)
{
    // Wait for touch & end of conversion
    while (penState != STATE_PEN_RELEASED);
    while (penState != STATE_PEN_PRESSED);
}

//------------------------------------------------------------------------------
/// Wait pen released
//------------------------------------------------------------------------------
void TSD_WaitPenReleased(void)
{
    // Wait for contact loss
    while (penState != STATE_PEN_PRESSED);
    while (penState != STATE_PEN_RELEASED);
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

    // Do calibration
    ret = TSDCom_Calibrate(pLcdBuffer);

    return ret;
}

//------------------------------------------------------------------------------
/// Initializes the touchscreen driver and starts the calibration process. When
/// finished, the touchscreen is operational.
///
/// Important: the LCD driver must have been initialized prior to calling this
/// function.
/// \param pBuffer  LCD buffer to use for displaying the calibration info.
//------------------------------------------------------------------------------
void TSD_Initialize(void *pLcdBuffer)
{
    ADS7843_Initialize();
    ConfigurePenIRQ();
#if !defined(cortexm3)
    ConfigurePit();
#endif
    tsInitFlag = 1;

    // Calibration
    if(pLcdBuffer) {
       while (!TSD_Calibrate(pLcdBuffer));
    }
}

//------------------------------------------------------------------------------
/// Stop the Touchscreen, disable interrupt and stop Pit
//------------------------------------------------------------------------------
void TSD_Reset(void)
{
    // Disable SPI 0
    ADS7843_Reset();

    // Disable the interrupt
    PIO_DisableIt(&pinPenIRQ);

#if !defined(cortexm3)
    // Stop Pit interrupt
    PIT_DisableIT();
    IRQ_DisableIT(AT91C_ID_SYS);
#endif
}

#endif //#ifdef BOARD_TSC_ADS7843
