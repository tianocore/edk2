/** @file
  Definitions to install Multiple Processor PPI.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CPU_MP_PEI_H_
#define _CPU_MP_PEI_H_

#include <PiPei.h>

#include <Ppi/SecPlatformInformation.h>
#include <Ppi/SecPlatformInformation2.h>

#include <Register/LocalApic.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/LocalApicLib.h>
#include <Library/MtrrLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiCpuLib.h>

#include "Microcode.h"

//
// AP state
//
typedef enum {
  CpuStateIdle,
  CpuStateBusy,
  CpuStateDisabled
} CPU_STATE;

//
// AP reset code information
//
typedef struct {
  UINT8             *RendezvousFunnelAddress;
  UINTN             PModeEntryOffset;
  UINTN             LModeEntryOffset;
  UINTN             RendezvousFunnelSize;
} MP_ASSEMBLY_ADDRESS_MAP;

typedef struct _PEI_CPU_MP_DATA  PEI_CPU_MP_DATA;

#pragma pack()

typedef union {
  struct {
    UINT32  LimitLow    : 16;
    UINT32  BaseLow     : 16;
    UINT32  BaseMid     : 8;
    UINT32  Type        : 4;
    UINT32  System      : 1;
    UINT32  Dpl         : 2;
    UINT32  Present     : 1;
    UINT32  LimitHigh   : 4;
    UINT32  Software    : 1;
    UINT32  Reserved    : 1;
    UINT32  DefaultSize : 1;
    UINT32  Granularity : 1;
    UINT32  BaseHigh    : 8;
  } Bits;
  UINT64  Uint64;
} IA32_GDT;

//
// MP CPU exchange information for AP reset code
//
typedef struct {
  UINTN                 Lock;
  UINTN                 StackStart;
  UINTN                 StackSize;
  UINTN                 CFunction;
  IA32_DESCRIPTOR       GdtrProfile;
  IA32_DESCRIPTOR       IdtrProfile;
  UINTN                 BufferStart;
  UINTN                 PmodeOffset;
  UINTN                 NumApsExecuting;
  UINTN                 LmodeOffset;
  UINTN                 Cr3;
  PEI_CPU_MP_DATA       *PeiCpuMpData;
} MP_CPU_EXCHANGE_INFO;

#pragma pack()

typedef struct {
  UINT32                         ApicId;
  EFI_HEALTH_FLAGS               Health;
  CPU_STATE                      State;
  BOOLEAN                        CpuHealthy;
} PEI_CPU_DATA;

//
// PEI CPU MP Data save in memory
//
struct _PEI_CPU_MP_DATA {
  UINT32                         CpuCount;
  UINT32                         BspNumber;
  UINTN                          Buffer;
  UINTN                          CpuApStackSize;
  MP_ASSEMBLY_ADDRESS_MAP        AddressMap;
  UINTN                          WakeupBuffer;
  UINTN                          BackupBuffer;
  UINTN                          BackupBufferSize;
  UINTN                          ApFunction;
  UINTN                          ApFunctionArgument;
  volatile UINT32                FinishedCount;
  BOOLEAN                        InitFlag;
  MTRR_SETTINGS                  MtrrTable;
  PEI_CPU_DATA                   *CpuData;
  volatile MP_CPU_EXCHANGE_INFO  *MpCpuExchangeInfo;
};

/**
  Assembly code to get starting address and size of the rendezvous entry for APs.
  Information for fixing a jump instruction in the code is also returned.

  @param AddressMap  Output buffer for address map information.
**/
VOID
EFIAPI
AsmGetAddressMap (
  OUT MP_ASSEMBLY_ADDRESS_MAP    *AddressMap
  );

/**
  Assembly code to load GDT table and update segment accordingly.

  @param Gdtr   Pointer to GDT descriptor
**/
VOID
EFIAPI
AsmInitializeGdt (
  IN IA32_DESCRIPTOR  *Gdtr
  );


/**
  Get CPU MP Data pointer from the Guided HOB.

  @return  Pointer to Pointer to PEI CPU MP Data
**/
PEI_CPU_MP_DATA *
GetMpHobData (
  VOID
  );

/**
  Collects BIST data from PPI.

  This function collects BIST data from Sec Platform Information2 PPI
  or SEC Platform Information PPI.

  @param PeiServices         Pointer to PEI Services Table
  @param PeiCpuMpData        Pointer to PEI CPU MP Data

**/
VOID
CollectBistDataFromPpi (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN PEI_CPU_MP_DATA                    *PeiCpuMpData
  );

/**
  Implementation of the PlatformInformation2 service in EFI_SEC_PLATFORM_INFORMATION2_PPI.

  @param  PeiServices                The pointer to the PEI Services Table.
  @param  StructureSize              The pointer to the variable describing size of the input buffer.
  @param  PlatformInformationRecord2 The pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD2.

  @retval EFI_SUCCESS                The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL       The buffer was too small. The current buffer size needed to
                                     hold the record is returned in StructureSize.

**/
EFI_STATUS
EFIAPI
SecPlatformInformation2 (
  IN CONST EFI_PEI_SERVICES                   **PeiServices,
  IN OUT UINT64                               *StructureSize,
     OUT EFI_SEC_PLATFORM_INFORMATION_RECORD2 *PlatformInformationRecord2
  );

#endif
