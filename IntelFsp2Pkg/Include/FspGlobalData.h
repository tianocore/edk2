/** @file

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FSP_GLOBAL_DATA_H_
#define _FSP_GLOBAL_DATA_H_

#include <FspEas.h>

#pragma pack(1)

typedef enum {
  TempRamInitApiIndex,
  FspInitApiIndex,
  NotifyPhaseApiIndex,
  FspMemoryInitApiIndex,
  TempRamExitApiIndex,
  FspSiliconInitApiIndex,
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
   UINT8              Reserved3[31];
   UINT32             PerfSig;
   UINT16             PerfLen;
   UINT16             Reserved4;
   UINT32             PerfIdx;
   UINT64             PerfData[32];
} FSP_GLOBAL_DATA;

#pragma pack()

#endif
