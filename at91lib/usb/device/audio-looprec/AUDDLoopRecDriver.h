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

/**
 \unit

 !Purpose

    Definition of a USB Audio Loop Record Driver with two playback channels
    and one record channel.

 !Usage

    -# Enable and setup USB related pins (see pio & board.h).
    -# Configure the USB Audio Loop Record driver using
       AUDDLoopRecDriver_Initialize
    -# To get %audio stream frames from host, use
       AUDDLoopRecDriver_Read
    -# To send %audio sampling stream to host, use
       AUDDLoopRecDriver_Write

*/

#ifndef AUDDLoopRecDriver_H
#define AUDDLoopRecDriver_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <board.h>
#include <usb/common/core/USBGenericRequest.h>
#include <usb/device/core/USBD.h>

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \page "Audio Loop Record stream information"
///
/// This page lists codes for USB Audio Loop Record stream information.
///
/// !Code
/// - AUDDLoopRecDriver_SAMPLERATE
/// - AUDDLoopRecDriver_NUMCHANNELS
/// - AUDDLoopRecDriver_BYTESPERSAMPLE
/// - AUDDLoopRecDriver_BITSPERSAMPLE
/// - AUDDLoopRecDriver_SAMPLESPERFRAME
/// - AUDDLoopRecDriver_BYTESPERFRAME

#if defined(at91sam7s) || defined(at91sam9xe)
 /// Sample rate in Hz.
 #define AUDDLoopRecDriver_SAMPLERATE        32000
 /// Number of channels in audio stream.
 #define AUDDLoopRecDriver_NUMCHANNELS       1
 /// Number of bytes in one sample.
 #define AUDDLoopRecDriver_BYTESPERSAMPLE    2
#else
 /// Sample rate in Hz.
 #define AUDDLoopRecDriver_SAMPLERATE        48000
 /// Number of channels in audio stream.
 #define AUDDLoopRecDriver_NUMCHANNELS       2
 /// Number of bytes in one sample.
 #define AUDDLoopRecDriver_BYTESPERSAMPLE    2
#endif
/// Number of bits in one sample.
#define AUDDLoopRecDriver_BITSPERSAMPLE     (AUDDLoopRecDriver_BYTESPERSAMPLE * 8)
/// Number of bytes in one USB subframe.
#define AUDDLoopRecDriver_BYTESPERSUBFRAME  (AUDDLoopRecDriver_NUMCHANNELS * \
                                             AUDDLoopRecDriver_BYTESPERSAMPLE)
/// Number of samples in one USB frame.
#define AUDDLoopRecDriver_SAMPLESPERFRAME   (AUDDLoopRecDriver_SAMPLERATE / 1000 \
                                             * AUDDLoopRecDriver_NUMCHANNELS)
/// Number of bytes in one USB frame.
#define AUDDLoopRecDriver_BYTESPERFRAME     (AUDDLoopRecDriver_SAMPLESPERFRAME * \
                                             AUDDLoopRecDriver_BYTESPERSAMPLE)
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \page "Audio Loop Record Channel Numbers"
///
/// This page lists codes for USB Audio Loop Record channel numbers.
///
/// !Playback channel numbers
/// - AUDDLoopRecDriver_MASTERCHANNEL
/// - AUDDLoopRecDriver_LEFTCHANNEL
/// - AUDDLoopRecDriver_RIGHTCHANNEL
///
/// !Record channel number
/// - AUDDLoopRecDriver_RECCHANNEL

/// Master channel of playback.
#define AUDDLoopRecDriver_MASTERCHANNEL     0
/// Front left channel of playback.
#define AUDDLoopRecDriver_LEFTCHANNEL       1
/// Front right channel of playback.
#define AUDDLoopRecDriver_RIGHTCHANNEL      2
/// Channel of record.
#define AUDDLoopRecDriver_RECCHANNEL        0
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//         Exported functions
//------------------------------------------------------------------------------

extern void AUDDLoopRecDriver_Initialize();

extern void AUDDLoopRecDriver_RequestHandler(const USBGenericRequest *request);

extern unsigned char AUDDLoopRecDriver_Read(void *buffer,
                                            unsigned int length,
                                            TransferCallback callback,
                                            void *argument);

extern unsigned char AUDDLoopRecDriver_Write(void *buffer,
                                             unsigned int length,
                                             TransferCallback callback,
                                             void *argument);

#endif //#ifndef AUDDLoopRecDriver_H

//#ifndef Loop Record_DRIVER_H
//#define Loop Record_DRIVER_H
//
////------------------------------------------------------------------------------
////         Definitions
////------------------------------------------------------------------------------
//
///*! Sampling frequency in Hz */
//#define Loop Record_SAMPLERATE          48000
///*! Number of samples in one isochronous packet (1ms frame) */
//#define Loop Record_SAMPLESPERPACKET     (Loop Record_SAMPLERATE / 1000)
///*! Size of one sample (in bytes) */
//#define Loop Record_SAMPLESIZE          2
///*! Number of channels */
//#define Loop Record_NUMCHANNELS         2
///*! Size of one frame (number of bytes sent for one sample on all channels) */
//#define Loop Record_FRAMESIZE           (Loop Record_SAMPLESIZE * Loop Record_NUMCHANNELS)
///*! Required bit rate given the sample frequency, sample size and number of
//    channels. */
//#define Loop Record_BITRATE             (Loop Record_SAMPLERATE * Loop Record_FRAMESIZE)
///*! Size of one isochronous packet */
//#define Loop Record_PACKETSIZE          (Loop Record_SAMPLESPERPACKET * Loop Record_FRAMESIZE)
//
////------------------------------------------------------------------------------
////         Structures
////------------------------------------------------------------------------------
///*!
//    Holds the Loop Record driver state.
// */
//typedef struct {
//
//    S_std_class   standardDriver;
//
//    unsigned char isOutStreamEnabled;
//    unsigned char isChannelMuted[Loop Record_NUMCHANNELS+1];
//
//    Callback_f    outStreamStatusChanged;
//    Callback_f    outStreamMuteChanged;
//
//} __attribute__((packed)) S_Loop Record;
//
////------------------------------------------------------------------------------
////         Exported functions
////------------------------------------------------------------------------------
//
//extern void SPK_Init(S_Loop Record *Loop RecordDriver, const S_usb *usbDriver);
//extern void SPK_SetCallbacks(S_Loop Record *Loop RecordDriver,
//                             Callback_f outStreamStatusChanged,
//                             Callback_f outStreamMuteChanged);
//extern void SPK_RequestHandler(S_Loop Record *Loop RecordDriver);
//extern char SPK_Read(S_Loop Record *Loop RecordDriver,
//                     void *buffer,
//                     unsigned int length,
//                     Callback_f callback,
//                     void *argument);
//
//#endif //#ifndef Loop Record_DRIVER_H
//
