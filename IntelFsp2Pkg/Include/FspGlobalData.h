/** @file

  Copyright (c) 2014 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_GLOBAL_DATA_H_
#define _FSP_GLOBAL_DATA_H_

#include <FspEas.h>

#define FSP_IN_API_MODE          0
#define FSP_IN_DISPATCH_MODE     1
#define FSP_GLOBAL_DATA_VERSION  0x3

#pragma pack(1)

typedef enum {
  TempRamInitApiIndex,
  FspInitApiIndex,
  NotifyPhaseApiIndex,
  FspMemoryInitApiIndex,
  TempRamExitApiIndex,
  FspSiliconInitApiIndex,
  FspMultiPhaseSiInitApiIndex,
  FspSmmInitApiIndex,
  FspMultiPhaseMemInitApiIndex,
  FspApiIndexMax
} FSP_API_INDEX;

typedef struct  {
  VOID     *DataPtr;
  UINTN    MicrocodeRegionBase;
  UINTN    MicrocodeRegionSize;
  UINTN    CodeRegionBase;
  UINTN    CodeRegionSize;
  UINTN    Reserved;
} FSP_PLAT_DATA;

#define FSP_GLOBAL_DATA_SIGNATURE        SIGNATURE_32 ('F', 'S', 'P', 'D')
#define FSP_PERFORMANCE_DATA_SIGNATURE   SIGNATURE_32 ('P', 'E', 'R', 'F')
#define FSP_PERFORMANCE_DATA_TIMER_MASK  0xFFFFFFFFFFFFFF

typedef struct  {
  UINT32             Signature;
  UINT8              Version;
  UINT8              Reserved1[3];
  ///
  /// Offset 0x08
  ///
  UINTN              CoreStack;
  VOID               *SmmInitUpdPtr;
  ///
  /// IA32: Offset 0x10; X64: Offset 0x18
  ///
  UINT32             StatusCode;
  UINT8              ApiIdx;
  ///
  /// 0: FSP in API mode; 1: FSP in DISPATCH mode
  ///
  UINT8              FspMode;
  UINT8              OnSeparateStack;
  UINT8              Reserved2;
  UINT32             NumberOfPhases;
  UINT32             PhasesExecuted;
  UINT32             Reserved3[8];
  ///
  /// IA32: Offset 0x40; X64: Offset 0x48
  /// Start of UINTN and pointer section
  /// All UINTN and pointer members are put in this section
  /// for maintaining natural alignment for both IA32 and X64 builds.
  ///
  FSP_PLAT_DATA      PlatformData;
  VOID               *TempRamInitUpdPtr;
  VOID               *MemoryInitUpdPtr;
  VOID               *SiliconInitUpdPtr;
  ///
  /// IA32: Offset 0x64; X64: Offset 0x90
  /// To store function parameters pointer
  /// so it can be retrieved after stack switched.
  ///
  VOID               *FunctionParameterPtr;
  FSP_INFO_HEADER    *FspInfoHeader;
  VOID               *UpdDataPtr;
  VOID               *FspHobListPtr;
  VOID               *VariableRequestParameterPtr;
  ///
  /// End of UINTN and pointer section
  /// At this point, next field offset must be either *0h or *8h to
  /// meet natural alignment requirement.
  ///
  UINT8              Reserved4[16];
  UINT32             PerfSig;
  UINT16             PerfLen;
  UINT16             Reserved5;
  UINT32             PerfIdx;
  UINT32             Reserved6;
  UINT64             PerfData[32];
} FSP_GLOBAL_DATA;

#pragma pack()

#endif
