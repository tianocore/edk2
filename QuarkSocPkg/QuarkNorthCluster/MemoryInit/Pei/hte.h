/** @file
HTE handling routines for MRC use.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef __HTE_H
#define __HTE_H

#define STATIC   static
#define VOID     void

#if !defined(__GNUC__) && (__STDC_VERSION__ < 199901L)
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t UINT8;
#endif

typedef enum
{
  MrcNoHaltSystemOnError,
  MrcHaltSystemOnError,
  MrcHaltHteEngineOnError,
  MrcNoHaltHteEngineOnError
} HALT_TYPE;

typedef enum
{
  MrcMemInit, MrcMemTest
} MEM_INIT_OR_TEST;

#define READ_TRAIN      1
#define WRITE_TRAIN     2

#define HTE_MEMTEST_NUM                 2
#define HTE_LOOP_CNT                    5  // EXP_LOOP_CNT field of HTE_CMD_CTL. This CANNOT be less than 4
#define HTE_LFSR_VICTIM_SEED   0xF294BA21  // Random seed for victim.
#define HTE_LFSR_AGRESSOR_SEED 0xEBA7492D  // Random seed for aggressor.
UINT32
HteMemInit(
    MRC_PARAMS *CurrentMrcData,
    UINT8 MemInitFlag,
    UINT8 HaltHteEngineOnError);

UINT16
BasicWriteReadHTE(
    MRC_PARAMS *CurrentMrcData,
    UINT32 Address,
    UINT8 FirstRun,
    UINT8 Mode);

UINT16
WriteStressBitLanesHTE(
    MRC_PARAMS *CurrentMrcData,
    UINT32 Address,
    UINT8 FirstRun);

VOID
HteMemOp(
    UINT32 Address,
    UINT8 FirstRun,
    UINT8 IsWrite);

#endif
