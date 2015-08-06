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

#include <Ppi/MpServices.h>
#include <Ppi/SecPlatformInformation.h>
#include <Ppi/SecPlatformInformation2.h>
#include <Ppi/EndOfPeiPhase.h>

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

//
// CPU exchange information for switch BSP
//
typedef struct {
  UINT8             State;        // offset 0
  UINTN             StackPointer; // offset 4 / 8
  IA32_DESCRIPTOR   Gdtr;         // offset 8 / 16
  IA32_DESCRIPTOR   Idtr;         // offset 14 / 26
} CPU_EXCHANGE_ROLE_INFO;

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
  BOOLEAN                        EndOfPeiFlag;
  BOOLEAN                        InitFlag;
  CPU_EXCHANGE_ROLE_INFO         BSPInfo;
  CPU_EXCHANGE_ROLE_INFO         APInfo;
  MTRR_SETTINGS                  MtrrTable;
  PEI_CPU_DATA                   *CpuData;
  volatile MP_CPU_EXCHANGE_INFO  *MpCpuExchangeInfo;
};
extern EFI_PEI_PPI_DESCRIPTOR   mPeiCpuMpPpiDesc;


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
  Assembly code to do CLI-HALT loop.

**/
VOID
EFIAPI
AsmCliHltLoop (
  VOID
  );

/**
  Get available system memory below 1MB by specified size.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data
**/
VOID
BackupAndPrepareWakeupBuffer(
  IN PEI_CPU_MP_DATA         *PeiCpuMpData
  );

/**
  Restore wakeup buffer data.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data
**/
VOID
RestoreWakeupBuffer(
  IN PEI_CPU_MP_DATA         *PeiCpuMpData
  );

/**
  Notify function on End Of Pei PPI.

  On S3 boot, this function will restore wakeup buffer data.
  On normal boot, this function will flag wakeup buffer to be un-used type.

  @param  PeiServices        The pointer to the PEI Services Table.
  @param  NotifyDescriptor   Address of the notification descriptor data structure.
  @param  Ppi                Address of the PPI that was installed.

  @retval EFI_SUCCESS        When everything is OK.

**/
EFI_STATUS
EFIAPI
CpuMpEndOfPeiCallback (
  IN      EFI_PEI_SERVICES        **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR    *NotifyDescriptor,
  IN VOID                         *Ppi
  );

/**
  This function will be called by BSP to wakeup AP.

  @param PeiCpuMpData       Pointer to PEI CPU MP Data
  @param Broadcast          TRUE:  Send broadcast IPI to all APs
                            FALSE: Send IPI to AP by ApicId
  @param ApicId             Apic ID for the processor to be waked
  @param Procedure          The function to be invoked by AP
  @param ProcedureArgument  The argument to be passed into AP function
**/
VOID
WakeUpAP (
  IN PEI_CPU_MP_DATA           *PeiCpuMpData,
  IN BOOLEAN                   Broadcast,
  IN UINT32                    ApicId,
  IN EFI_AP_PROCEDURE          Procedure,              OPTIONAL
  IN VOID                      *ProcedureArgument      OPTIONAL
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
  Find the current Processor number by APIC ID.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data
  @param ProcessorNumber     Return the pocessor number found

  @retval EFI_SUCCESS        ProcessorNumber is found and returned.
  @retval EFI_NOT_FOUND      ProcessorNumber is not found.
**/
EFI_STATUS
GetProcessorNumber (
  IN PEI_CPU_MP_DATA         *PeiCpuMpData,
  OUT UINTN                  *ProcessorNumber
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
