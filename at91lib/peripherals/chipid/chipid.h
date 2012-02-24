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
/// \unit
///
/// !Purpose
/// 
/// Methods and definitions for access Chip ID peripheral in AT91 microcontrollers.
/// 
/// !Usage
///
/// -# 
///
//------------------------------------------------------------------------------

#ifndef CHIPID_H
#define CHIPID_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <board.h>

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Definition for chip id register
//------------------------------------------------------------------------------
typedef struct _ChipId {

   /// Version of the device
   unsigned int version;
   /// Embedded processor
   unsigned int eProc;
   /// Nonvolatile program memory size
   unsigned int nvpSiz;
   /// Second nonvolatile program memory size
   unsigned int nvpSiz2;
   /// Internal SRAM size
   unsigned int sramSiz;
   /// Architecture identifier
   unsigned int arch;
   /// Nonvolatile program memory type
   unsigned int nvpTyp;
   /// Extension flag
   unsigned int extFlag;
   /// Chip ID extersion extension
   unsigned int extID;
}ChipId;

#if 1
struct ChipIDType {

   /// Identifier
   unsigned int num;
   /// Type
   unsigned char pStr[80];
};
#else
typedef struct _ChipIDType {

   /// Identifier
   unsigned int num;
   /// Type
   unsigned char pStr[80];
}ChipIDType;
#endif

//------------------------------------------------------------------------------
//         Global functions
//------------------------------------------------------------------------------

/// Get chip ID
extern unsigned char CHIPID_Get(ChipId* pChipId);

/// Print chip ID
extern void CHIPID_Print(ChipId* pChipId);

#endif //#ifndef CHIPID_H