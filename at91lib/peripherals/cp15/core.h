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

// core list
//-------------------
// arm7tdmi
// arm926ej_s
// arm1176jzf_s
// cortexm3

#include "board.h"

#ifndef _CORE_H
#define _CORE_H

#if defined(at91sam7a3) \
    || defined(at91sam7l) \
    || defined(at91sam7s32) \
    || defined(at91sam7s321) \
    || defined(at91sam7s64) \
    || defined(at91sam7s128) \
    || defined(at91sam7s256) \
    || defined(at91sam7s512) \
    || defined(at91sam7se32) \
    || defined(at91sam7se256) \
    || defined(at91sam7se512) \
    || defined(at91sam7x128) \
    || defined(at91sam7x256) \
    || defined(at91sam7x512) \
    || defined(at91sam7xc128) \
    || defined(at91sam7xc256) \
    || defined(at91sam7xc512)

#define arm7tdmi

#elif  defined(at91cap9) \
    || defined(at91sam9260) \
    || defined(at91sam9261) \
    || defined(at91sam9263) \
    || defined(at91sam9g20) \
    || defined(at91sam9m10) \
    || defined(at91sam9m11) \
    || defined(at91sam9rl) \
    || defined(at91sam9xe)

#define arm926ej_s

#elif defined(at91cap11)

#define arm1176jzf_s

#elif defined(at91sam3u)

#define cortexm3

#else

#error ARM core not defined!

#endif

#endif // #ifndef _CORE_H