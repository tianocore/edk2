/** @file
  Common header file for MP Initialize Library.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MP_LIB_H_
#define _MP_LIB_H_

#include <PiPei.h>

#include <Register/Cpuid.h>
#include <Register/Msr.h>
#include <Register/LocalApic.h>
#include <Register/Microcode.h>

#include <Library/MpInitLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/LocalApicLib.h>
#include <Library/CpuLib.h>
#include <Library/UefiCpuLib.h>
#include <Library/TimerLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/MtrrLib.h>
#include <Library/HobLib.h>

#define WAKEUP_AP_SIGNAL SIGNATURE_32 ('S', 'T', 'A', 'P')

#define CPU_INIT_MP_LIB_HOB_GUID \
  { \
    0x58eb6a19, 0x3699, 0x4c68, { 0xa8, 0x36, 0xda, 0xcd, 0x8e, 0xdc, 0xad, 0x4a } \
  }

//
// AP loop state when APs are in idle state
// It's value is the same with PcdCpuApLoopMode
//
typedef enum {
  ApInHltLoop   = 1,
  ApInMwaitLoop = 2,
  ApInRunLoop   = 3
} AP_LOOP_MODE;

//
// AP initialization state during APs wakeup
//
typedef enum {
  ApInitConfig   = 1,
  ApInitReconfig = 2,
  ApInitDone     = 3
} AP_INIT_STATE;

//
// AP state
//
typedef enum {
  CpuStateIdle,
  CpuStateReady,
  CpuStateBusy,
  CpuStateFinished,
  CpuStateDisabled
} CPU_STATE;

//
// CPU volatile registers around INIT-SIPI-SIPI
//
typedef struct {
  UINTN                          Cr0;
  UINTN                          Cr3;
  UINTN                          Cr4;
  UINTN                          Dr0;
  UINTN                          Dr1;
  UINTN                          Dr2;
  UINTN                          Dr3;
  UINTN                          Dr6;
  UINTN                          Dr7;
} CPU_VOLATILE_REGISTERS;

//
// AP related data
//
typedef struct {
  SPIN_LOCK                      ApLock;
  volatile UINT32                *StartupApSignal;
  volatile UINTN                 ApFunction;
  volatile UINTN                 ApFunctionArgument;
  UINT32                         InitialApicId;
  UINT32                         ApicId;
  UINT32                         Health;
  BOOLEAN                        CpuHealthy;
  volatile CPU_STATE             State;
  CPU_VOLATILE_REGISTERS         VolatileRegisters;
  BOOLEAN                        Waiting;
  BOOLEAN                        *Finished;
  UINT64                         ExpectedTime;
  UINT64                         CurrentTime;
  UINT64                         TotalTime;
  EFI_EVENT                      WaitEvent;
} CPU_AP_DATA;

//
// Basic CPU information saved in Guided HOB.
// Because the contents will be shard between PEI and DXE,
// we need to make sure the each fields offset same in different
// architecture.
//
typedef struct {
  UINT32                         InitialApicId;
  UINT32                         ApicId;
  UINT32                         Health;
} CPU_INFO_IN_HOB;

//
// AP reset code information including code address and size,
// this structure will be shared be C code and assembly code.
// It is natural aligned by design.
//
typedef struct {
  UINT8             *RendezvousFunnelAddress;
  UINTN             ModeEntryOffset;
  UINTN             RendezvousFunnelSize;
  UINT8             *RelocateApLoopFuncAddress;
  UINTN             RelocateApLoopFuncSize;
} MP_ASSEMBLY_ADDRESS_MAP;

typedef struct _CPU_MP_DATA  CPU_MP_DATA;

#pragma pack(1)

//
// MP CPU exchange information for AP reset code
// This structure is required to be packed because fixed field offsets
// into this structure are used in assembly code in this module
//
typedef struct {
  UINTN                 Lock;
  UINTN                 StackStart;
  UINTN                 StackSize;
  UINTN                 CFunction;
  IA32_DESCRIPTOR       GdtrProfile;
  IA32_DESCRIPTOR       IdtrProfile;
  UINTN                 BufferStart;
  UINTN                 ModeOffset;
  UINTN                 NumApsExecuting;
  UINTN                 CodeSegment;
  UINTN                 DataSegment;
  UINTN                 EnableExecuteDisable;
  UINTN                 Cr3;
  CPU_MP_DATA           *CpuMpData;
} MP_CPU_EXCHANGE_INFO;

#pragma pack()

//
// CPU MP Data save in memory
//
struct _CPU_MP_DATA {
  UINT64                         CpuInfoInHob;
  UINT32                         CpuCount;
  UINT32                         BspNumber;
  //
  // The above fields data will be passed from PEI to DXE
  // Please make sure the fields offset same in the different
  // architecture.
  //
  SPIN_LOCK                      MpLock;
  UINTN                          Buffer;
  UINTN                          CpuApStackSize;
  MP_ASSEMBLY_ADDRESS_MAP        AddressMap;
  UINTN                          WakeupBuffer;
  UINTN                          BackupBuffer;
  UINTN                          BackupBufferSize;
  BOOLEAN                        EndOfPeiFlag;

  volatile UINT32                StartCount;
  volatile UINT32                FinishedCount;
  volatile UINT32                RunningCount;
  BOOLEAN                        SingleThread;
  EFI_AP_PROCEDURE               Procedure;
  VOID                           *ProcArguments;
  BOOLEAN                        *Finished;
  UINT64                         ExpectedTime;
  UINT64                         CurrentTime;
  UINT64                         TotalTime;
  EFI_EVENT                      WaitEvent;
  UINTN                          **FailedCpuList;

  AP_INIT_STATE                  InitFlag;
  BOOLEAN                        X2ApicEnable;
  MTRR_SETTINGS                  MtrrTable;
  UINT8                          ApLoopMode;
  UINT8                          ApTargetCState;
  UINT16                         PmCodeSegment;
  CPU_AP_DATA                    *CpuData;
  volatile MP_CPU_EXCHANGE_INFO  *MpCpuExchangeInfo;
};

extern EFI_GUID mCpuInitMpLibHobGuid;

/**
  Assembly code to place AP into safe loop mode.

  Place AP into targeted C-State if MONITOR is supported, otherwise
  place AP into hlt state.
  Place AP in protected mode if the current is long mode. Due to AP maybe
  wakeup by some hardware event. It could avoid accessing page table that
  may not available during booting to OS.

  @param[in] MwaitSupport    TRUE indicates MONITOR is supported.
                             FALSE indicates MONITOR is not supported.
  @param[in] ApTargetCState  Target C-State value.
  @param[in] PmCodeSegment   Protected mode code segment value.
**/
typedef
VOID
(EFIAPI * ASM_RELOCATE_AP_LOOP) (
  IN BOOLEAN                 MwaitSupport,
  IN UINTN                   ApTargetCState,
  IN UINTN                   PmCodeSegment
  );

/**
  Assembly code to get starting address and size of the rendezvous entry for APs.
  Information for fixing a jump instruction in the code is also returned.

  @param[out] AddressMap  Output buffer for address map information.
**/
VOID
EFIAPI
AsmGetAddressMap (
  OUT MP_ASSEMBLY_ADDRESS_MAP    *AddressMap
  );

/**
  Get the pointer to CPU MP Data structure.

  @return  The pointer to CPU MP Data structure.
**/
CPU_MP_DATA *
GetCpuMpData (
  VOID
  );

/**
  Save the pointer to CPU MP Data structure.

  @param[in] CpuMpData  The pointer to CPU MP Data structure will be saved.
**/
VOID
SaveCpuMpData (
  IN CPU_MP_DATA   *CpuMpData
  );

/**
  Allocate reset vector buffer.

  @param[in, out]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
AllocateResetVector (
  IN OUT CPU_MP_DATA          *CpuMpData
  );

/**
  Free AP reset vector buffer.

  @param[in]  CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
FreeResetVector (
  IN CPU_MP_DATA              *CpuMpData
  );

/**
  This function will be called by BSP to wakeup AP.

  @param[in] CpuMpData          Pointer to CPU MP Data
  @param[in] Broadcast          TRUE:  Send broadcast IPI to all APs
                                FALSE: Send IPI to AP by ApicId
  @param[in] ProcessorNumber    The handle number of specified processor
  @param[in] Procedure          The function to be invoked by AP
  @param[in] ProcedureArgument  The argument to be passed into AP function
**/
VOID
WakeUpAP (
  IN CPU_MP_DATA               *CpuMpData,
  IN BOOLEAN                   Broadcast,
  IN UINTN                     ProcessorNumber,
  IN EFI_AP_PROCEDURE          Procedure,              OPTIONAL
  IN VOID                      *ProcedureArgument      OPTIONAL
  );

/**
  Initialize global data for MP support.

  @param[in] CpuMpData  The pointer to CPU MP Data structure.
**/
VOID
InitMpGlobalData (
  IN CPU_MP_DATA               *CpuMpData
  );

/**
  Get pointer to CPU MP Data structure from GUIDed HOB.

  @return  The pointer to CPU MP Data structure.
**/
CPU_MP_DATA *
GetCpuMpDataFromGuidedHob (
  VOID
  );
  
/**
  Detect whether specified processor can find matching microcode patch and load it.

  @param[in] PeiCpuMpData        Pointer to PEI CPU MP Data
**/
VOID
MicrocodeDetect (
  IN CPU_MP_DATA             *CpuMpData
  );

/**
  Notify function on End Of PEI PPI.

  On S3 boot, this function will restore wakeup buffer data.
  On normal boot, this function will flag wakeup buffer to be un-used type.

  @param[in]  PeiServices        The pointer to the PEI Services Table.
  @param[in]  NotifyDescriptor   Address of the notification descriptor data structure.
  @param[in]  Ppi                Address of the PPI that was installed.

  @retval EFI_SUCCESS        When everything is OK.
**/
EFI_STATUS
EFIAPI
CpuMpEndOfPeiCallback (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR    *NotifyDescriptor,
  IN VOID                         *Ppi
  );

#endif

