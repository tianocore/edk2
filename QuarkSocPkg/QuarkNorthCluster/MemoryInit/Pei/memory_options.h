/** @file
Common definitions and compilation switches for MRC

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef __MEMORY_OPTIONS_H
#define __MEMORY_OPTIONS_H

#include "core_types.h"

// MRC COMPILE TIME SWITCHES:
// ==========================



//#define MRC_SV              // enable some validation opitons

#if defined (SIM) || defined(EMU)
#define QUICKSIM              // reduce execution time using shorter rd/wr sequences
#endif

#define CLT                   // required for Quark project



//#define BACKUP_RCVN           // enable STATIC timing settings for RCVN (BACKUP_MODE)
//#define BACKUP_WDQS           // enable STATIC timing settings for WDQS (BACKUP_MODE)
//#define BACKUP_RDQS           // enable STATIC timing settings for RDQS (BACKUP_MODE)
//#define BACKUP_WDQ            // enable STATIC timing settings for WDQ (BACKUP_MODE)



//#define BACKUP_COMPS          // enable *COMP overrides (BACKUP_MODE)
//#define RX_EYE_CHECK          // enable the RD_TRAIN eye check
#define HMC_TEST              // enable Host to Memory Clock Alignment
#define R2R_SHARING           // enable multi-rank support via rank2rank sharing

#define FORCE_16BIT_DDRIO     // disable signals not used in 16bit mode of DDRIO



//
// Debug support
//

#ifdef NDEBUG
#define DPF        if(0) dpf
#else
#define DPF        dpf
#endif

void dpf( uint32_t mask, char_t *bla, ...);


uint8_t mgetc(void);
uint8_t mgetch(void);


// Debug print type
#define D_ERROR      0x0001
#define D_INFO       0x0002
#define D_REGRD      0x0004
#define D_REGWR      0x0008
#define D_FCALL      0x0010
#define D_TRN        0x0020
#define D_TIME       0x0040

#define ENTERFN()     DPF(D_FCALL, "<%s>\n", __FUNCTION__)
#define LEAVEFN()     DPF(D_FCALL, "</%s>\n", __FUNCTION__)
#define REPORTFN()    DPF(D_FCALL, "<%s/>\n", __FUNCTION__)

extern uint32_t DpfPrintMask;

#endif
