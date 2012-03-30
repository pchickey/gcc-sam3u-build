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

#include <stddef.h>

#include "board.h"
#include "exceptions.h"
#include "board_lowlevel.h"

//------------------------------------------------------------------------------
//         External Variables
//------------------------------------------------------------------------------

// Stack top
extern unsigned int _estack;

// Vector start address
extern unsigned int _vect_start;

// Initialize segments
extern unsigned int _sfixed;
extern unsigned int _sfixed;
extern unsigned int _efixed;
extern unsigned int _etext;
extern unsigned int _srelocate;
extern unsigned int _erelocate;
extern unsigned int _szero;
extern unsigned int _ezero;
#if defined(psram)
extern unsigned int _svectorrelocate;
extern unsigned int _evectorrelocate;
#endif

//------------------------------------------------------------------------------
//         ProtoTypes
//------------------------------------------------------------------------------

extern int main(void);

//------------------------------------------------------------------------------
/// Run C++ preinit and init arrays.
/// These are constructors for static objects.
//------------------------------------------------------------------------------

extern void (*__preinit_array_start []) (void) __attribute__((weak));
extern void (*__preinit_array_end []) (void) __attribute__((weak));
extern void (*__init_array_start []) (void) __attribute__((weak));
extern void (*__init_array_end []) (void) __attribute__((weak));

void __libc_init_array(void)
{
  size_t count;
  size_t i;
  count = __preinit_array_end - __preinit_array_start;
  for (i = 0; i < count; i++)
    __preinit_array_start[i] ();

  count = __init_array_end - __init_array_start;
  for (i = 0; i < count; i++)
    __init_array_start[i] ();


}



//------------------------------------------------------------------------------
/// This is the code that gets called on processor reset. To initialize the
/// device. And call the main() routine.
//------------------------------------------------------------------------------
void ResetException(void)
{
    unsigned int *pSrc, *pDest;

    LowLevelInit();
#if defined(psram)
    pDest = &_vect_start;
    pSrc = &_svectorrelocate;
    for(; pSrc < &_evectorrelocate;) {
            *pDest++ = *pSrc++;
    }
#endif

    // Initialize data
    pSrc = &_etext;
    pDest = &_srelocate;
    if (pSrc != pDest) {
        for(; pDest < &_erelocate;) {

            *pDest++ = *pSrc++;
        }
    }

    // Zero fill bss
    for(pDest = &_szero; pDest < &_ezero;) {

        *pDest++ = 0;
    }
    
#if defined(psram)
    pSrc = (unsigned int *)&_vect_start;
#else
    pSrc = (unsigned int *)&_sfixed;
#endif        
    
    AT91C_BASE_NVIC->NVIC_VTOFFR = ((unsigned int)(pSrc)) | (0x0 << 7);

    __libc_init_array();

    main();
}
