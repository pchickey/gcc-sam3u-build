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
/// !Purpose
/// 
/// Specialization of the Media class for interfacing with the internal Flash.
/// 
/// !Usage
/// 
/// TODO
//------------------------------------------------------------------------------

#ifndef MEDFLASH_H
#define MEDFLASH_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "Media.h"
#include <board.h>

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------

// Redefinition of EFC structure and base address.
#if !defined(AT91C_BASE_EFC) && defined(AT91C_BASE_EFC0)
    #define AT91C_BASE_EFC  AT91C_BASE_EFC0

#elif !defined(AT91C_BASE_EFC) && defined(AT91C_BASE_MC)
    #define AT91S_EFC       AT91S_MC
    #define AT91C_BASE_EFC  AT91C_BASE_MC
    #define EFC_FCR         MC_FCR
    #define EFC_FMR         MC_FMR
    #define EFC_FSR         MC_FSR
#endif

// Definition for the SAM9XE (which has an EEFC)
#if defined(at91sam9xe128) || defined(at91sam9xe256) || defined(at91sam9xe512)
    #define AT91C_MC_FCMD_START_PROG    AT91C_EFC_FCMD_EWP
    #define AT91C_MC_FRDY               AT91C_EFC_FRDY
#endif

#if defined(AT91C_BASE_EFC)

//------------------------------------------------------------------------------
//      Exported functions
//------------------------------------------------------------------------------

extern void FLA_Initialize(Media *media, AT91S_EFC *efc);
 
#endif //#if defined(AT91C_BASE_EFC)
#endif //#ifndef MEDFLASH_H

