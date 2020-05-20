/** @file

  Copyright (c) 2014 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_GLOBAL_DATA_H_
#define _FSP_GLOBAL_DATA_H_

#include <FspEas.h>

#define FSP_IN_API_MODE      0
#define FSP_IN_DISPATCH_MODE 1

#pragma pack(1)

typedef enum {
  TempRamInitApiIndex,
  FspInitApiIndex,
  NotifyPhaseApiIndex,
  FspMemoryInitApiIndex,
  TempRamExitApiIndex,
  FspSiliconInitApiIndex,
  FspMultiPhaseSiInitApiIndex,
  FspApiIndexMax
} FSP_API_INDEX;

typedef struct  {
   VOID               *DataPtr;
   UINT32             MicrocodeRegionBase;
   UINT32             MicrocodeRegionSize;
   UINT32             CodeRegionBase;
   UINT32             CodeRegionSize;
} FSP_PLAT_DATA;

#define FSP_GLOBAL_DATA_SIGNATURE  SIGNATURE_32 ('F', 'S', 'P', 'D')
#define FSP_PERFORMANCE_DATA_SIGNATURE  SIGNATURE_32 ('P', 'E', 'R', 'F')
#define FSP_PERFORMANCE_DATA_TIMER_MASK 0xFFFFFFFFFFFFFF

typedef struct  {
   UINT32             Signature;
   UINT8              Version;
   UINT8              Reserved1[3];
   UINT32             CoreStack;
   UINT32             StatusCode;
   UINT32             Reserved2[8];
   FSP_PLAT_DATA      PlatformData;
   FSP_INFO_HEADER    *FspInfoHeader;
   VOID               *UpdDataPtr;
   VOID               *TempRamInitUpdPtr;
   VOID               *MemoryInitUpdPtr;
   VOID               *SiliconInitUpdPtr;
   UINT8              ApiIdx;
   ///
   /// 0: FSP in API mode; 1: FSP in DISPATCH mode
   ///
   UINT8              FspMode;
   UINT8              OnSeparateStack;
   UINT8              Reserved3;
   UINT32             NumberOfPhases;
   UINT32             PhasesExecuted;
   ///
   /// To store function parameters pointer
   /// so it can be retrieved after stack switched.
   ///
   VOID               *FunctionParameterPtr;
   UINT8              Reserved4[16];
   UINT32             PerfSig;
   UINT16             PerfLen;
   UINT16             Reserved5;
   UINT32             PerfIdx;
   UINT64             PerfData[32];
} FSP_GLOBAL_DATA;

#pragma pack()

#endif
