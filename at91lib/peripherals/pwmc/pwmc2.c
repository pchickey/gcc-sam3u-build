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

#include "pwmc2.h"
#include <board.h>
#include <utility/assert.h>
#include <utility/trace.h>

//------------------------------------------------------------------------------
//         Local functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Finds a prescaler/divisor couple to generate the desired frequency from
/// MCK.
/// Returns the value to enter in PWMC_MR or 0 if the configuration cannot be
/// met.
/// \param frequency  Desired frequency in Hz.
/// \param mck  Master clock frequency in Hz.
//------------------------------------------------------------------------------
static unsigned short FindClockConfiguration(
    unsigned int frequency,
    unsigned int mck)
{
    unsigned int divisors[11] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    unsigned char divisor = 0;
    unsigned int prescaler;

    SANITY_CHECK(frequency < mck);

    // Find prescaler and divisor values
    prescaler = (mck / divisors[divisor]) / frequency;
    while ((prescaler > 255) && (divisor < 11)) {

        divisor++;
        prescaler = (mck / divisors[divisor]) / frequency;
    }

    // Return result
    if (divisor < 11) {

        TRACE_DEBUG("Found divisor=%u and prescaler=%u for freq=%uHz\n\r",
                  divisors[divisor], prescaler, frequency);
        return prescaler | (divisor << 8);
    }
    else {

        return 0;
    }
}

//------------------------------------------------------------------------------
//         Global functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Configures PWM a channel with the given parameters, basic configure function.
/// The PWM controller must have been clocked in the PMC prior to calling this
/// function.
/// Beware: this function disables the channel. It waits until disable is effective.
/// \param channel  Channel number.
/// \param prescaler  Channel prescaler.
/// \param alignment  Channel alignment.
/// \param polarity  Channel polarity.
//------------------------------------------------------------------------------
void PWMC_ConfigureChannel(
    unsigned char channel,
    unsigned int prescaler,
    unsigned int alignment,
    unsigned int polarity)
{
    SANITY_CHECK(prescaler < AT91C_PWMC_CPRE_MCKB);
    SANITY_CHECK((alignment & ~AT91C_PWMC_CALG) == 0);
    SANITY_CHECK((polarity & ~AT91C_PWMC_CPOL) == 0);

    // Disable channel (effective at the end of the current period)
    if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) != 0) {
        AT91C_BASE_PWMC->PWMC_DIS = 1 << channel;
        while ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) != 0);
    }

    // Configure channel
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR = prescaler | alignment | polarity;
}

//------------------------------------------------------------------------------
/// Configures PWM a channel with the given parameters, extend configure function.
/// The PWM controller must have been clocked in the PMC prior to calling this
/// function.
/// Beware: this function disables the channel. It waits until disable is effective.
/// \param channel            Channel number.
/// \param prescaler          Channel prescaler.
/// \param alignment          Channel alignment.
/// \param polarity           Channel polarity.
/// \param countEventSelect   Channel counter event selection.
/// \param DTEnable           Channel dead time generator enable.
/// \param DTHInverte         Channel Dead-Time PWMHx output Inverted.
/// \param DTLInverte         Channel Dead-Time PWMHx output Inverted.
//------------------------------------------------------------------------------
void PWMC_ConfigureChannelExt(
    unsigned char channel,
    unsigned int prescaler,
    unsigned int alignment,
    unsigned int polarity,
    unsigned int countEventSelect,
    unsigned int DTEnable,
    unsigned int DTHInverte,
    unsigned int DTLInverte)
{
    SANITY_CHECK(prescaler < AT91C_PWMC_CPRE_MCKB);
    SANITY_CHECK((alignment & ~AT91C_PWMC_CALG) == 0);
    SANITY_CHECK((polarity & ~AT91C_PWMC_CPOL) == 0);
    SANITY_CHECK((countEventSelect & ~AT91C_PWMC_CES) == 0);
    SANITY_CHECK((DTEnable & ~AT91C_PWMC_DTE) == 0);
    SANITY_CHECK((DTHInverte & ~AT91C_PWMC_DTHI) == 0);
    SANITY_CHECK((DTLInverte & ~AT91C_PWMC_DTLI) == 0);

    // Disable channel (effective at the end of the current period)
    if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) != 0) {
        AT91C_BASE_PWMC->PWMC_DIS = 1 << channel;
        while ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) != 0);
    }

    // Configure channel
    AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR = prescaler | alignment | polarity |
        countEventSelect | DTEnable | DTHInverte | DTLInverte;
}

//------------------------------------------------------------------------------
/// Configures PWM clocks A & B to run at the given frequencies. This function
/// finds the best MCK divisor and prescaler values automatically.
/// \param clka  Desired clock A frequency (0 if not used).
/// \param clkb  Desired clock B frequency (0 if not used).
/// \param mck  Master clock frequency.
//------------------------------------------------------------------------------
void PWMC_ConfigureClocks(unsigned int clka, unsigned int clkb, unsigned int mck)
{
    unsigned int mode = 0;
    unsigned int result;

    // Clock A
    if (clka != 0) {

        result = FindClockConfiguration(clka, mck);
        ASSERT(result != 0, "-F- Could not generate the desired PWM frequency (%uHz)\n\r", clka);
        mode |= result;
    }

    // Clock B
    if (clkb != 0) {

        result = FindClockConfiguration(clkb, mck);
        ASSERT(result != 0, "-F- Could not generate the desired PWM frequency (%uHz)\n\r", clkb);
        mode |= (result << 16);
    }

    // Configure clocks
    TRACE_DEBUG("Setting PWMC_MR = 0x%08X\n\r", mode);
    AT91C_BASE_PWMC->PWMC_MR = mode;
}

//------------------------------------------------------------------------------
/// Sets the period value used by a PWM channel. This function writes directly
/// to the CPRD register if the channel is disabled; otherwise, it uses the
/// update register CUPD.
/// \param channel  Channel number.
/// \param period  Period value.
//------------------------------------------------------------------------------
void PWMC_SetPeriod(unsigned char channel, unsigned short period)
{
    // If channel is disabled, write to CPRD
    if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) == 0) {

        AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CPRDR = period;
    }
    // Otherwise use update register
    else {

        AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CPRDUPDR = period;
    }
}

//------------------------------------------------------------------------------
/// Sets the duty cycle used by a PWM channel. This function writes directly to
/// the CDTY register if the channel is disabled; otherwise it uses the
/// update register CUPD.
/// Note that the duty cycle must always be inferior or equal to the channel
/// period.
/// \param channel  Channel number.
/// \param duty  Duty cycle value.
//------------------------------------------------------------------------------
void PWMC_SetDutyCycle(unsigned char channel, unsigned short duty)
{
    SANITY_CHECK(duty <= AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CPRDR);

    // SAM7S errata
#if defined(at91sam7s16) || defined(at91sam7s161) || defined(at91sam7s32) \
    || defined(at91sam7s321) || defined(at91sam7s64) || defined(at91sam7s128) \
    || defined(at91sam7s256) || defined(at91sam7s512)
    ASSERT(duty > 0, "-F- Duty cycle value 0 is not permitted on SAM7S chips.\n\r");
    ASSERT((duty > 1) || (AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CMR & AT91C_PWMC_CALG),
           "-F- Duty cycle value 1 is not permitted in left-aligned mode on SAM7S chips.\n\r");
#endif

    // If channel is disabled, write to CDTY
    if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) == 0) {

        AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CDTYR = duty;
    }
    // Otherwise use update register
    else {

        AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CDTYUPDR = duty;
    }
}

//------------------------------------------------------------------------------
/// Sets the dead time used by a PWM channel. This function writes directly to
/// the DTR register if the channel is disabled; otherwise it uses the
/// update register DTUPDR.
/// Note that the dead time must always be inferior or equal to the channel
/// period.
/// \param channel  Channel number.
/// \param timeH    Dead time value for PWMHx output.
/// \param timeL    Dead time value for PWMLx output.
//------------------------------------------------------------------------------
void PWMC_SetDeadTime(unsigned char channel, unsigned short timeH, unsigned short timeL)
{
    SANITY_CHECK(timeH <= AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CPRDR);
    SANITY_CHECK(timeL <= AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_CPRDR);

    // If channel is disabled, write to DTR
    if ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) == 0) {

        AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_DTR = timeH | (timeL << 16);
    }
    // Otherwise use update register
    else {
        AT91C_BASE_PWMC->PWMC_CH[channel].PWMC_DTUPDR = timeH | (timeL << 16);
    }
}

//------------------------------------------------------------------------------
/// Configures Syncronous channel with the given parameters.
/// Beware: At this time, the channels should be disabled.
/// \param channels                 Bitwise OR of Syncronous channels.
/// \param updateMode               Syncronous channel update mode.
/// \param requestMode              PDC transfer request mode.
/// \param requestComparisonSelect  PDC transfer request comparison selection.
//------------------------------------------------------------------------------
void PWMC_ConfigureSyncChannel(
    unsigned int channels,
    unsigned int updateMode,
    unsigned int requestMode,
    unsigned int requestComparisonSelect)
{
    AT91C_BASE_PWMC->PWMC_SYNC = channels | updateMode | requestMode
        | requestComparisonSelect;
}

//------------------------------------------------------------------------------
/// Sets the update period of the synchronous channels.
/// This function writes directly to
/// the SCUP register if the channel #0 is disabled; otherwise it uses the
/// update register SCUPUPD.
/// \param period   update period.
//------------------------------------------------------------------------------
void PWMC_SetSyncChannelUpdatePeriod(unsigned char period)
{
    // If channel is disabled, write to SCUP
    if ((AT91C_BASE_PWMC->PWMC_SR & (1 << 0)) == 0) {

        AT91C_BASE_PWMC->PWMC_SCUP = period;
    }
    // Otherwise use update register
    else {

        AT91C_BASE_PWMC->PWMC_SCUPUPD = period;
    }
}

//------------------------------------------------------------------------------
/// Sets synchronous channels update unlock.
/// Note: If the UPDM field is set to 0, writing the UPDULOCK bit to 1
/// triggers the update of the period value, the duty-cycle and
/// the dead-time values of synchronous channels at the beginning
/// of the next PWM period. If the field UPDM is set to 1 or 2,
/// writing the UPDULOCK bit to 1 triggers only the update of
/// the period value and of the dead-time values of synchronous channels.
/// This bit is automatically reset when the update is done.
//------------------------------------------------------------------------------
void PWMC_SetSyncChannelUpdateUnlock(void)
{
    AT91C_BASE_PWMC->PWMC_UPCR = AT91C_PWMC_UPDULOCK;
}

//------------------------------------------------------------------------------
/// Enables the given PWM channel. This does NOT enable the corresponding pin;
/// this must be done in the user code.
/// \param channel  Channel number.
//------------------------------------------------------------------------------
void PWMC_EnableChannel(unsigned char channel)
{
    AT91C_BASE_PWMC->PWMC_ENA = 1 << channel;
}

//------------------------------------------------------------------------------
/// Disables the given PWM channel.
/// Beware, channel will be effectively disabled at the end of the current period.
/// Application can check channel is disabled using the following wait loop:
/// while ((AT91C_BASE_PWMC->PWMC_SR & (1 << channel)) != 0);
/// \param channel  Channel number.
//------------------------------------------------------------------------------
void PWMC_DisableChannel(unsigned char channel)
{
    AT91C_BASE_PWMC->PWMC_DIS = 1 << channel;
}

//------------------------------------------------------------------------------
/// Enables the period interrupt for the given PWM channel.
/// \param channel  Channel number.
//------------------------------------------------------------------------------
void PWMC_EnableChannelIt(unsigned char channel)
{
    AT91C_BASE_PWMC->PWMC_IER1 = 1 << channel;
}

//------------------------------------------------------------------------------
/// Disables the period interrupt for the given PWM channel.
/// \param channel  Channel number.
//------------------------------------------------------------------------------
void PWMC_DisableChannelIt(unsigned char channel)
{
    AT91C_BASE_PWMC->PWMC_IDR1 = 1 << channel;
}

//-----------------------------------------------------------------------------
/// Enables the selected interrupts sources on a PWMC peripheral.
/// \param sources1  Bitwise OR of selected interrupt sources of PWMC_IER1.
/// \param sources2  Bitwise OR of selected interrupt sources of PWMC_IER2.
//-----------------------------------------------------------------------------
void PWMC_EnableIt(unsigned int sources1, unsigned int sources2)
{
    AT91C_BASE_PWMC->PWMC_IER1 = sources1;
    AT91C_BASE_PWMC->PWMC_IER2 = sources2;
}

//-----------------------------------------------------------------------------
/// Disables the selected interrupts sources on a PWMC peripheral.
/// \param sources1  Bitwise OR of selected interrupt sources of PWMC_IDR1.
/// \param sources2  Bitwise OR of selected interrupt sources of PWMC_IDR2.
//-----------------------------------------------------------------------------
void PWMC_DisableIt(unsigned int sources1, unsigned int sources2)
{
    AT91C_BASE_PWMC->PWMC_IDR1 = sources1;
    AT91C_BASE_PWMC->PWMC_IDR2 = sources2;
}

//------------------------------------------------------------------------------
/// Sends the contents of buffer through a PWMC peripheral, using the PDC to
/// take care of the transfer.
/// Note: Duty cycle of syncronous channels can update by PDC
///       when the field UPDM (Update Mode) in the PWM_SCM register is set to 2.
/// \param pwmc    Pointer to an AT91S_PWMC instance.
/// \param buffer  Data buffer to send.
/// \param length  Length of the data buffer.
//------------------------------------------------------------------------------
unsigned char PWMC_WriteBuffer(AT91S_PWMC *pwmc,
    void *buffer,
    unsigned int length)
{
    // Check if first bank is free
    if (pwmc->PWMC_TCR == 0) {

        pwmc->PWMC_TPR = (unsigned int) buffer;
        pwmc->PWMC_TCR = length;
        pwmc->PWMC_PTCR = AT91C_PDC_TXTEN;
        return 1;
    }
    // Check if second bank is free
    else if (pwmc->PWMC_TNCR == 0) {

        pwmc->PWMC_TNPR = (unsigned int) buffer;
        pwmc->PWMC_TNCR = length;
        return 1;
    }

    // No free banks
    return 0;
}

//-----------------------------------------------------------------------------
/// Set PWM output override value
/// \param value  Bitwise OR of output override value.
//-----------------------------------------------------------------------------
void PWMC_SetOverrideValue(unsigned int value)
{
    AT91C_BASE_PWMC->PWMC_OOV = value;
}

//-----------------------------------------------------------------------------
/// Enalbe override output.
/// \param value  Bitwise OR of output selection.
/// \param sync   0: enable the output asyncronously, 1: enable it syncronously
//-----------------------------------------------------------------------------
void PWMC_EnableOverrideOutput(unsigned int value, unsigned int sync)
{
    if (sync) {

        AT91C_BASE_PWMC->PWMC_OSSUPD = value;
    } else {

        AT91C_BASE_PWMC->PWMC_OSS = value;
    }
}

//-----------------------------------------------------------------------------
/// Disalbe override output.
/// \param value  Bitwise OR of output selection.
/// \param sync   0: enable the output asyncronously, 1: enable it syncronously
//-----------------------------------------------------------------------------
void PWMC_DisableOverrideOutput(unsigned int value, unsigned int sync)
{
    if (sync) {

        AT91C_BASE_PWMC->PWMC_OSCUPD = value;
    } else {

        AT91C_BASE_PWMC->PWMC_OSC = value;
    }
}

//-----------------------------------------------------------------------------
/// Set PWM fault mode.
/// \param mode  Bitwise OR of fault mode.
//-----------------------------------------------------------------------------
void PWMC_SetFaultMode(unsigned int mode)
{
    AT91C_BASE_PWMC->PWMC_FMR = mode;
}

//-----------------------------------------------------------------------------
/// PWM fault clear.
/// \param fault  Bitwise OR of fault to clear.
//-----------------------------------------------------------------------------
void PWMC_FaultClear(unsigned int fault)
{
    AT91C_BASE_PWMC->PWMC_FCR = fault;
}

//-----------------------------------------------------------------------------
/// Set PWM fault protection value.
/// \param value  Bitwise OR of fault protection value.
//-----------------------------------------------------------------------------
void PWMC_SetFaultProtectionValue(unsigned int value)
{
    AT91C_BASE_PWMC->PWMC_FPV = value;
}

//-----------------------------------------------------------------------------
/// Enable PWM fault protection.
/// \param value  Bitwise OR of FPEx[y].
//-----------------------------------------------------------------------------
void PWMC_EnableFaultProtection(unsigned int value)
{
    AT91C_BASE_PWMC->PWMC_FPER1 = value;
}

//-----------------------------------------------------------------------------
/// Configure comparison unit.
/// \param x     comparison x index
/// \param value comparison x value.
/// \param mode  comparison x mode
//-----------------------------------------------------------------------------
void PWMC_ConfigureComparisonUnit(unsigned int x, unsigned int value, unsigned int mode)
{
    // If channel is disabled, write to CMPxM & CMPxV
    if ((AT91C_BASE_PWMC->PWMC_SR & (1 << 0)) == 0) {
        if (x == 0) {
            AT91C_BASE_PWMC->PWMC_CMP0M = mode;
            AT91C_BASE_PWMC->PWMC_CMP0V = value;
        } else if (x == 1) {
            AT91C_BASE_PWMC->PWMC_CMP1M = mode;
            AT91C_BASE_PWMC->PWMC_CMP1V = value;
        } else if (x == 2) {
            AT91C_BASE_PWMC->PWMC_CMP2M = mode;
            AT91C_BASE_PWMC->PWMC_CMP2V = value;
        } else if (x == 3) {
            AT91C_BASE_PWMC->PWMC_CMP3M = mode;
            AT91C_BASE_PWMC->PWMC_CMP3V = value;
        } else if (x == 4) {
            AT91C_BASE_PWMC->PWMC_CMP4M = mode;
            AT91C_BASE_PWMC->PWMC_CMP4V = value;
        } else if (x == 5) {
            AT91C_BASE_PWMC->PWMC_CMP5M = mode;
            AT91C_BASE_PWMC->PWMC_CMP5V = value;
        } else if (x == 6) {
            AT91C_BASE_PWMC->PWMC_CMP6M = mode;
            AT91C_BASE_PWMC->PWMC_CMP6V = value;
        } else if (x == 7) {
            AT91C_BASE_PWMC->PWMC_CMP7M = mode;
            AT91C_BASE_PWMC->PWMC_CMP7V = value;
        }
    } 
    // Otherwise use update register
    else {
        if (x == 0) {
            AT91C_BASE_PWMC->PWMC_CMP0MUPD = mode;
            AT91C_BASE_PWMC->PWMC_CMP0VUPD = value;
        } else if (x == 1) {
            AT91C_BASE_PWMC->PWMC_CMP1MUPD = mode;
            AT91C_BASE_PWMC->PWMC_CMP1VUPD = value;
        } else if (x == 2) {
            AT91C_BASE_PWMC->PWMC_CMP2MUPD = mode;
            AT91C_BASE_PWMC->PWMC_CMP2VUPD = value;
        } else if (x == 3) {
            AT91C_BASE_PWMC->PWMC_CMP3MUPD = mode;
            AT91C_BASE_PWMC->PWMC_CMP3VUPD = value;
        } else if (x == 4) {
            AT91C_BASE_PWMC->PWMC_CMP4MUPD = mode;
            AT91C_BASE_PWMC->PWMC_CMP4VUPD = value;
        } else if (x == 5) {
            AT91C_BASE_PWMC->PWMC_CMP5MUPD = mode;
            AT91C_BASE_PWMC->PWMC_CMP5VUPD = value;
        } else if (x == 6) {
            AT91C_BASE_PWMC->PWMC_CMP6MUPD = mode;
            AT91C_BASE_PWMC->PWMC_CMP6VUPD = value;
        } else if (x == 7) {
            AT91C_BASE_PWMC->PWMC_CMP7MUPD = mode;
            AT91C_BASE_PWMC->PWMC_CMP7VUPD = value;
        }
    }
}

//-----------------------------------------------------------------------------
/// Configure event line mode.
/// \param x    Line x
/// \param mode Bitwise OR of line mode selection
//-----------------------------------------------------------------------------
void PWMC_ConfigureEventLineMode(unsigned int x, unsigned int mode)
{
    if (x == 0) {
        AT91C_BASE_PWMC->PWMC_EL0MR = mode;
    } else if (x == 1) {
        AT91C_BASE_PWMC->PWMC_EL1MR = mode;
    } else if (x == 2) {
        AT91C_BASE_PWMC->PWMC_EL2MR = mode;
    } else if (x == 3) {
        AT91C_BASE_PWMC->PWMC_EL3MR = mode;
    } else if (x == 4) {
        AT91C_BASE_PWMC->PWMC_EL4MR = mode;
    } else if (x == 5) {
        AT91C_BASE_PWMC->PWMC_EL5MR = mode;
    } else if (x == 6) {
        AT91C_BASE_PWMC->PWMC_EL6MR = mode;
    } else if (x == 7) {
        AT91C_BASE_PWMC->PWMC_EL7MR = mode;
    }
}
