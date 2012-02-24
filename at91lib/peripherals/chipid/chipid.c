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

#include <stdio.h>
#include <chipid/chipid.h>
#include <string.h>

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------

/// ChipID register, version field
#define AT91C_CHIPID_CIDR_VERSION                     (0x1fUL << 0)
#define AT91C_CHIPID_CIDR_VERSION_BITFLD              0
#define AT91C_CHIPID_CIDR_VERSION_BITS                5
/// ChipID register, embedded processor field
#define AT91C_CHIPID_CIDR_EPROC                       (0x7UL << 5)
#define AT91C_CHIPID_CIDR_EPROC_BITFLD                5
#define AT91C_CHIPID_CIDR_EPROC_BITS                  3
/// ChipID register, nonvolatile program memory size field
#define AT91C_CHIPID_CIDR_NVPSIZ                      (0xfUL << 8)
#define AT91C_CHIPID_CIDR_NVPSIZ_BITFLD               8
#define AT91C_CHIPID_CIDR_NVPSIZ_BITS                 4
/// ChipID register, second nonvolatile program memory size field
#define AT91C_CHIPID_CIDR_NVPSIZ2                     (0xfUL << 12)
#define AT91C_CHIPID_CIDR_NVPSIZ2_BITFLD              12
#define AT91C_CHIPID_CIDR_NVPSIZ2_BITS                4
/// ChipID register, Internal SRAM siize field
#define AT91C_CHIPID_CIDR_SRAMSIZ                     (0xfUL << 16)
#define AT91C_CHIPID_CIDR_SRAMSIZ_BITFLD              16
#define AT91C_CHIPID_CIDR_SRAMSIZ_BITS                4
/// ChipID register, Architecture identifier field
#define AT91C_CHIPID_CIDR_ARCH                        (0xffUL << 20)
#define AT91C_CHIPID_CIDR_ARCH_BITFLD                 20
#define AT91C_CHIPID_CIDR_ARCH_BITS                   8
/// ChipID register, nonvolatile program memory type field
#define AT91C_CHIPID_CIDR_NVPTYP                      (0x7UL << 28)
#define AT91C_CHIPID_CIDR_NVPTYP_BITFLD               28
#define AT91C_CHIPID_CIDR_NVPTYP_BITS                 3
/// ChipID register, extersion flag field
#define AT91C_CHIPID_CIDR_EXT                         (0x1UL << 31)
#define AT91C_CHIPID_CIDR_EXT_BITFLD                  31
#define AT91C_CHIPID_CIDR_EXT_BITS                    1

#define CHIPID_ID(chipid, bitfield, bits)   ((chipid >> bitfield) & ((1 << (bits)) - 1))

//------------------------------------------------------------------------------
//         Internal variables
//------------------------------------------------------------------------------
#define AT91C_CHIPID_EPROC_SIZE    5
const struct ChipIDType CHIPID_eProc[AT91C_CHIPID_EPROC_SIZE] = {

    // identifier       description
    {0x1,   "ARM946ES"},
    {0x2,   "ARM7TDMI"},
    {0x3,   "Cortex-M3"},
    {0x4,   "ARM920T"},
    {0x5,   "ARM926EJS"},
};

#define AT91C_CHIPID_NVPSIZE_SIZE    16
const struct ChipIDType CHIPID_nvpSiz[AT91C_CHIPID_NVPSIZE_SIZE] = {

    // identifier       description
    {0x0,   "None"},
    {0x1,   "8K bytes"},
    {0x2,   "16K bytes"},
    {0x3,   "32K bytes"},
    {0x4,   "Reserved"},
    {0x5,   "64K bytes"},
    {0x6,   "Reserved"},
    {0x7,   "128K bytes"},
    {0x8,   "Reserved"},
    {0x9,   "256K bytes"},
    {0xA,   "512K bytes"},
    {0xB,   "Reserved"},
    {0xC,   "1024K bytes"},
    {0xD,   "Reserved"},
    {0xE,   "2048K bytes"},
    {0xF,   "Reserved"}
};

#define AT91C_CHIPID_NVPSIZE2_SIZE    16
const struct ChipIDType CHIPID_nvpSiz2[AT91C_CHIPID_NVPSIZE2_SIZE] = {

    // identifier       description
    {0x0,   "None"},
    {0x1,   "8K bytes"},
    {0x2,   "16K bytes"},
    {0x3,   "32K bytes"},
    {0x4,   "Reserved"},
    {0x5,   "64K bytes"},
    {0x6,   "Reserved"},
    {0x7,   "128K bytes"},
    {0x8,   "Reserved"},
    {0x9,   "256K bytes"},
    {0xA,   "512K bytes"},
    {0xB,   "Reserved"},
    {0xC,   "1024K bytes"},
    {0xD,   "Reserved"},
    {0xE,   "2048K bytes"},
    {0xF,   "Reserved"}
};

#define AT91C_CHIPID_SRAMSIZE_SIZE    16
const struct ChipIDType CHIPID_sramSiz[AT91C_CHIPID_SRAMSIZE_SIZE] = {

    // identifier       description
    {0x0,   "48K bytes"},
    {0x1,   "1K bytes"},
    {0x2,   "2K bytes"},
    {0x3,   "6K bytes"},
    {0x4,   "112K bytes"},
    {0x5,   "4K bytes"},
    {0x6,   "80K bytes"},
    {0x7,   "160K bytes"},
    {0x8,   "8K bytes"},
    {0x9,   "16K bytes"},
    {0xA,   "32K bytes"},
    {0xB,   "64K bytes"},
    {0xC,   "128K bytes"},
    {0xD,   "256K bytes"},
    {0xE,   "96K bytes"},
    {0xF,   "512K bytes"}
};

#define AT91C_CHIPID_ARCH_SIZE    22
const struct ChipIDType CHIPID_archSiz[AT91C_CHIPID_ARCH_SIZE] = {

    // identifier       description
    {0x19,   "AT91SAM9xx Series"},
    {0x29,   "AT91SAM9XExx Series"},
    {0x34,   "AT91x34 series"},
    {0x37,   "CAP7 Series"},
    {0x39,   "CAP9 Series"},
    {0x3B,   "CAP11 Series"},
    {0x40,   "AT91x40 Series"},
    {0x42,   "AT91x42 Series"},
    {0x55,   "AT91x55 Series"},
    {0x60,   "AT91SAM7Axx Series"},
    {0x61,   "AT91SAM7AQxx Series"},
    {0x63,   "AT91x63 Series"},
    {0x70,   "AT91SAM7Sxx Series"},
    {0x71,   "AT91SAM7XCxx Series"},
    {0x72,   "AT91SAM7SExx Series"},
    {0x73,   "AT91SAM7Lxx Series"},
    {0x75,   "AT91SAM7Xxx Series"},
    {0x76,   "AT91SAM7SLxx Series"},
    {0x80,   "AT91SAM3Uxx Series"},
    {0x81,   "AT91SAM3UExx Series"},
    {0x92,   "AT91x92 Series"},
    {0xF0,   "AT75Cxx Series"}
};

#define AT91C_CHIPID_NVPTYPE_SIZE    5
const struct ChipIDType CHIPID_nvpTyp[AT91C_CHIPID_NVPTYPE_SIZE] = {

    // identifier       description
    {0x0,   "ROM"},
    {0x1,   "ROMless or on-chip Flash"},
    {0x4,   "SRAM emulating ROM"},
    {0x2,   "Embedded Flash Memory"},
    {0x3,   "ROM and Embedded Flash Memory, NVPSIZ is ROM size, NVPSIZ2 is Flash size"}
};

//------------------------------------------------------------------------------
/// Internal functions
//------------------------------------------------------------------------------
unsigned char CHIPID_Find(const struct ChipIDType* pChipIDTypeList,
                               unsigned int size,
                               unsigned int id, 
                               struct ChipIDType* pChipIDType)
{
    unsigned int i;

    for(i=0; i<size; i++)
    {
        if(pChipIDTypeList[i].num == id)
       	{
            memcpy(pChipIDType, &pChipIDTypeList[i], sizeof(struct ChipIDType));
            return 0;
        }
    }

    return 1;
}

//------------------------------------------------------------------------------
/// Exported functions
//------------------------------------------------------------------------------
unsigned char CHIPID_Get(ChipId* pChipId)
{
    unsigned int chipId, chipIdExt;

    chipId = AT91C_BASE_DBGU->DBGU_CIDR;
    chipIdExt = AT91C_BASE_DBGU->DBGU_EXID;

    pChipId->version = CHIPID_ID(chipId, AT91C_CHIPID_CIDR_VERSION_BITFLD, AT91C_CHIPID_CIDR_VERSION_BITS);
    pChipId->eProc = CHIPID_ID(chipId, AT91C_CHIPID_CIDR_EPROC_BITFLD, AT91C_CHIPID_CIDR_EPROC_BITS);
    pChipId->nvpSiz = CHIPID_ID(chipId, AT91C_CHIPID_CIDR_NVPSIZ_BITFLD, AT91C_CHIPID_CIDR_NVPSIZ_BITS);
    pChipId->nvpSiz2 = CHIPID_ID(chipId, AT91C_CHIPID_CIDR_NVPSIZ2_BITFLD, AT91C_CHIPID_CIDR_NVPSIZ2_BITS);
    pChipId->sramSiz = CHIPID_ID(chipId, AT91C_CHIPID_CIDR_SRAMSIZ_BITFLD, AT91C_CHIPID_CIDR_SRAMSIZ_BITS);
    pChipId->arch = CHIPID_ID(chipId, AT91C_CHIPID_CIDR_ARCH_BITFLD, AT91C_CHIPID_CIDR_ARCH_BITS);
    pChipId->nvpTyp = CHIPID_ID(chipId, AT91C_CHIPID_CIDR_NVPTYP_BITFLD, AT91C_CHIPID_CIDR_NVPTYP_BITS);
    pChipId->extFlag= CHIPID_ID(chipId, AT91C_CHIPID_CIDR_EXT_BITFLD, AT91C_CHIPID_CIDR_EXT_BITS);
    pChipId->extID = chipIdExt;

    return 0;
}

void CHIPID_Print(ChipId* pChipId)
{
    unsigned char status;
    struct ChipIDType  chipIdType;

    // Version
    printf("Version                                   0x%x.\r\n", pChipId->version);

    // Find Embedded Processor
    status = CHIPID_Find(CHIPID_eProc, AT91C_CHIPID_EPROC_SIZE, pChipId->eProc, &chipIdType);
    if(!status)
    {
        printf("Embedded Processor                        %s.\r\n", chipIdType.pStr);
    }

	// Find nonvolatile program memory size
    status = CHIPID_Find(CHIPID_nvpSiz, AT91C_CHIPID_NVPSIZE_SIZE, pChipId->nvpSiz, &chipIdType);
    if(!status)
    {
        printf("Nonvolatile program memory size           %s.\r\n", chipIdType.pStr);
    }

	// Find Second nonvolatile program memory size
    status = CHIPID_Find(CHIPID_nvpSiz2, AT91C_CHIPID_NVPSIZE2_SIZE, pChipId->nvpSiz2, &chipIdType);
    if(!status)
    {
        printf("Second nonvolatile program memory size    %s.\r\n", chipIdType.pStr);
    }

	// Find Internal SRAM size
    status = CHIPID_Find(CHIPID_sramSiz, AT91C_CHIPID_SRAMSIZE_SIZE, pChipId->sramSiz, &chipIdType);
    if(!status)
    {
        printf("Internal SRAM size                        %s.\r\n", chipIdType.pStr);
    }

	// Find Architecture identifier
    status = CHIPID_Find(CHIPID_archSiz, AT91C_CHIPID_ARCH_SIZE, pChipId->arch, &chipIdType);
    if(!status)
    {
        printf("Architecture identifier                   %s.\r\n", chipIdType.pStr);
    }

	// Find nonvolatile program memory type
    status = CHIPID_Find(CHIPID_nvpTyp, AT91C_CHIPID_NVPTYPE_SIZE, pChipId->nvpTyp, &chipIdType);
    if(!status)
    {
        printf("Nonvolatile program memory type           %s.\r\n", chipIdType.pStr);
    }

	// Find extension flag
    if(pChipId->extFlag)
    {
        printf("Extended chip ID is                       0x%x. \r\n", pChipId->extID);
    }
    else
    {
        printf("Extended chip ID is not existed. \r\n");
    }

}
