/** @file
Agent Module to load other modules to deploy SMM Entry Vector for X86 CPU.

Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CPU_PISMMCPUDXESMM_H_
#define _CPU_PISMMCPUDXESMM_H_

#include <PiSmm.h>

#include <Protocol/MpService.h>
#include <Protocol/SmmConfiguration.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmAccess2.h>
#include <Protocol/SmmReadyToLock.h>
#include <Protocol/SmmCpuService.h>

#include <Guid/AcpiS3Context.h>

#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MtrrLib.h>
#include <Library/SmmCpuPlatformHookLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/HobLib.h>
#include <Library/LocalApicLib.h>
#include <Library/UefiCpuLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/SmmCpuFeaturesLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

#include <AcpiCpuData.h>
#include <CpuHotPlugData.h>

#include <Register/Cpuid.h>

#include "CpuService.h"
#include "SmmProfile.h"

//
// MSRs required for configuration of SMM Code Access Check
//
#define EFI_MSR_SMM_MCA_CAP                    0x17D
#define  SMM_CODE_ACCESS_CHK_BIT               BIT58

#define  SMM_FEATURE_CONTROL_LOCK_BIT          BIT0
#define  SMM_CODE_CHK_EN_BIT                   BIT2

///
/// Page Table Entry
///
#define IA32_PG_P                   BIT0
#define IA32_PG_RW                  BIT1
#define IA32_PG_U                   BIT2
#define IA32_PG_WT                  BIT3
#define IA32_PG_CD                  BIT4
#define IA32_PG_A                   BIT5
#define IA32_PG_D                   BIT6
#define IA32_PG_PS                  BIT7
#define IA32_PG_PAT_2M              BIT12
#define IA32_PG_PAT_4K              IA32_PG_PS
#define IA32_PG_PMNT                BIT62
#define IA32_PG_NX                  BIT63

#define PAGE_ATTRIBUTE_BITS         (IA32_PG_RW | IA32_PG_P)
//
// Bits 1, 2, 5, 6 are reserved in the IA32 PAE PDPTE
// X64 PAE PDPTE does not have such restriction
//
#define IA32_PAE_PDPTE_ATTRIBUTE_BITS    (IA32_PG_P)

//
// Size of Task-State Segment defined in IA32 Manual
//
#define TSS_SIZE              104
#define TSS_X64_IST1_OFFSET   36
#define TSS_IA32_CR3_OFFSET   28
#define TSS_IA32_ESP_OFFSET   56

//
// Code select value
//
#define PROTECT_MODE_CODE_SEGMENT          0x08
#define LONG_MODE_CODE_SEGMENT             0x38

//
// The size 0x20 must be bigger than
// the size of template code of SmmInit. Currently,
// the size of SmmInit requires the 0x16 Bytes buffer
// at least.
//
#define BACK_BUF_SIZE  0x20

#define EXCEPTION_VECTOR_NUMBER     0x20

#define INVALID_APIC_ID 0xFFFFFFFFFFFFFFFFULL

typedef UINT32                              SMM_CPU_ARRIVAL_EXCEPTIONS;
#define ARRIVAL_EXCEPTION_BLOCKED           0x1
#define ARRIVAL_EXCEPTION_DELAYED           0x2
#define ARRIVAL_EXCEPTION_SMI_DISABLED      0x4

//
// Private structure for the SMM CPU module that is stored in DXE Runtime memory
// Contains the SMM Configuration Protocols that is produced.
// Contains a mix of DXE and SMM contents.  All the fields must be used properly.
//
#define SMM_CPU_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('s', 'c', 'p', 'u')

typedef struct {
  UINTN                           Signature;

  EFI_HANDLE                      SmmCpuHandle;

  EFI_PROCESSOR_INFORMATION       *ProcessorInfo;
  SMM_CPU_OPERATION               *Operation;
  UINTN                           *CpuSaveStateSize;
  VOID                            **CpuSaveState;

  EFI_SMM_RESERVED_SMRAM_REGION   SmmReservedSmramRegion[1];
  EFI_SMM_ENTRY_CONTEXT           SmmCoreEntryContext;
  EFI_SMM_ENTRY_POINT             SmmCoreEntry;

  EFI_SMM_CONFIGURATION_PROTOCOL  SmmConfiguration;
} SMM_CPU_PRIVATE_DATA;

extern SMM_CPU_PRIVATE_DATA  *gSmmCpuPrivate;
extern CPU_HOT_PLUG_DATA      mCpuHotPlugData;
extern UINTN                  mMaxNumberOfCpus;
extern UINTN                  mNumberOfCpus;
extern BOOLEAN                mRestoreSmmConfigurationInS3;
extern EFI_SMM_CPU_PROTOCOL   mSmmCpu;

///
/// The mode of the CPU at the time an SMI occurs
///
extern UINT8  mSmmSaveStateRegisterLma;


//
// SMM CPU Protocol function prototypes.
//

/**
  Read information from the CPU save state.

  @param  This      EFI_SMM_CPU_PROTOCOL instance
  @param  Width     The number of bytes to read from the CPU save state.
  @param  Register  Specifies the CPU register to read form the save state.
  @param  CpuIndex  Specifies the zero-based index of the CPU save state
  @param  Buffer    Upon return, this holds the CPU register value read from the save state.

  @retval EFI_SUCCESS   The register was read from Save State
  @retval EFI_NOT_FOUND The register is not defined for the Save State of Processor
  @retval EFI_INVALID_PARAMTER   This or Buffer is NULL.

**/
EFI_STATUS
EFIAPI
SmmReadSaveState (
  IN CONST EFI_SMM_CPU_PROTOCOL         *This,
  IN UINTN                              Width,
  IN EFI_SMM_SAVE_STATE_REGISTER        Register,
  IN UINTN                              CpuIndex,
  OUT VOID                              *Buffer
  );

/**
  Write data to the CPU save state.

  @param  This      EFI_SMM_CPU_PROTOCOL instance
  @param  Width     The number of bytes to read from the CPU save state.
  @param  Register  Specifies the CPU register to write to the save state.
  @param  CpuIndex  Specifies the zero-based index of the CPU save state
  @param  Buffer    Upon entry, this holds the new CPU register value.

  @retval EFI_SUCCESS   The register was written from Save State
  @retval EFI_NOT_FOUND The register is not defined for the Save State of Processor
  @retval EFI_INVALID_PARAMTER   ProcessorIndex or Width is not correct

**/
EFI_STATUS
EFIAPI
SmmWriteSaveState (
  IN CONST EFI_SMM_CPU_PROTOCOL         *This,
  IN UINTN                              Width,
  IN EFI_SMM_SAVE_STATE_REGISTER        Register,
  IN UINTN                              CpuIndex,
  IN CONST VOID                         *Buffer
  );

/**
Read a CPU Save State register on the target processor.

This function abstracts the differences that whether the CPU Save State register is in the
IA32 CPU Save State Map or X64 CPU Save State Map.

This function supports reading a CPU Save State register in SMBase relocation handler.

@param[in]  CpuIndex       Specifies the zero-based index of the CPU save state.
@param[in]  RegisterIndex  Index into mSmmCpuWidthOffset[] look up table.
@param[in]  Width          The number of bytes to read from the CPU save state.
@param[out] Buffer         Upon return, this holds the CPU register value read from the save state.

@retval EFI_SUCCESS           The register was read from Save State.
@retval EFI_NOT_FOUND         The register is not defined for the Save State of Processor.
@retval EFI_INVALID_PARAMTER  This or Buffer is NULL.

**/
EFI_STATUS
EFIAPI
ReadSaveStateRegister (
  IN UINTN                        CpuIndex,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        Width,
  OUT VOID                        *Buffer
  );

/**
Write value to a CPU Save State register on the target processor.

This function abstracts the differences that whether the CPU Save State register is in the
IA32 CPU Save State Map or X64 CPU Save State Map.

This function supports writing a CPU Save State register in SMBase relocation handler.

@param[in] CpuIndex       Specifies the zero-based index of the CPU save state.
@param[in] RegisterIndex  Index into mSmmCpuWidthOffset[] look up table.
@param[in] Width          The number of bytes to read from the CPU save state.
@param[in] Buffer         Upon entry, this holds the new CPU register value.

@retval EFI_SUCCESS           The register was written to Save State.
@retval EFI_NOT_FOUND         The register is not defined for the Save State of Processor.
@retval EFI_INVALID_PARAMTER  ProcessorIndex or Width is not correct.

**/
EFI_STATUS
EFIAPI
WriteSaveStateRegister (
  IN UINTN                        CpuIndex,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        Width,
  IN CONST VOID                   *Buffer
  );

//
//
//
typedef struct {
  UINT32                            Offset;
  UINT16                            Segment;
  UINT16                            Reserved;
} IA32_FAR_ADDRESS;

extern IA32_FAR_ADDRESS             gSmmJmpAddr;

extern CONST UINT8                  gcSmmInitTemplate[];
extern CONST UINT16                 gcSmmInitSize;
extern UINT32                       gSmmCr0;
extern UINT32                       gSmmCr3;
extern UINT32                       gSmmCr4;
extern UINTN                        gSmmInitStack;

/**
  Semaphore operation for all processor relocate SMMBase.
**/
VOID
EFIAPI
SmmRelocationSemaphoreComplete (
  VOID
  );

///
/// The type of SMM CPU Information
///
typedef struct {
  SPIN_LOCK                         Busy;
  volatile EFI_AP_PROCEDURE         Procedure;
  volatile VOID                     *Parameter;
  volatile UINT32                   Run;
  volatile BOOLEAN                  Present;
} SMM_CPU_DATA_BLOCK;

typedef enum {
  SmmCpuSyncModeTradition,
  SmmCpuSyncModeRelaxedAp,
  SmmCpuSyncModeMax
} SMM_CPU_SYNC_MODE;

typedef struct {
  //
  // Pointer to an array. The array should be located immediately after this structure
  // so that UC cache-ability can be set together.
  //
  SMM_CPU_DATA_BLOCK            *CpuData;
  volatile UINT32               Counter;
  volatile UINT32               BspIndex;
  volatile BOOLEAN              InsideSmm;
  volatile BOOLEAN              AllCpusInSync;
  volatile SMM_CPU_SYNC_MODE    EffectiveSyncMode;
  volatile BOOLEAN              SwitchBsp;
  volatile BOOLEAN              *CandidateBsp;
} SMM_DISPATCHER_MP_SYNC_DATA;

typedef struct {
  SPIN_LOCK    SpinLock;
  UINT32       MsrIndex;
} MP_MSR_LOCK;

#define SMM_PSD_OFFSET              0xfb00

typedef struct {
  UINT64                            Signature;              // Offset 0x00
  UINT16                            Reserved1;              // Offset 0x08
  UINT16                            Reserved2;              // Offset 0x0A
  UINT16                            Reserved3;              // Offset 0x0C
  UINT16                            SmmCs;                  // Offset 0x0E
  UINT16                            SmmDs;                  // Offset 0x10
  UINT16                            SmmSs;                  // Offset 0x12
  UINT16                            SmmOtherSegment;        // Offset 0x14
  UINT16                            Reserved4;              // Offset 0x16
  UINT64                            Reserved5;              // Offset 0x18
  UINT64                            Reserved6;              // Offset 0x20
  UINT64                            Reserved7;              // Offset 0x28
  UINT64                            SmmGdtPtr;              // Offset 0x30
  UINT32                            SmmGdtSize;             // Offset 0x38
  UINT32                            Reserved8;              // Offset 0x3C
  UINT64                            Reserved9;              // Offset 0x40
  UINT64                            Reserved10;             // Offset 0x48
  UINT16                            Reserved11;             // Offset 0x50
  UINT16                            Reserved12;             // Offset 0x52
  UINT32                            Reserved13;             // Offset 0x54
  UINT64                            MtrrBaseMaskPtr;        // Offset 0x58
} PROCESSOR_SMM_DESCRIPTOR;

extern IA32_DESCRIPTOR                     gcSmiGdtr;
extern IA32_DESCRIPTOR                     gcSmiIdtr;
extern VOID                                *gcSmiIdtrPtr;
extern CONST PROCESSOR_SMM_DESCRIPTOR      gcPsd;
extern UINT64                              gPhyMask;
extern ACPI_CPU_DATA                       mAcpiCpuData;
extern SMM_DISPATCHER_MP_SYNC_DATA         *mSmmMpSyncData;
extern VOID                                *mGdtForAp;
extern VOID                                *mIdtForAp;
extern VOID                                *mMachineCheckHandlerForAp;
extern UINTN                               mSmmStackArrayBase;
extern UINTN                               mSmmStackArrayEnd;
extern UINTN                               mSmmStackSize;
extern EFI_SMM_CPU_SERVICE_PROTOCOL        mSmmCpuService;
extern IA32_DESCRIPTOR                     gcSmiInitGdtr;

/**
  Create 4G PageTable in SMRAM.

  @param          ExtraPages       Additional page numbers besides for 4G memory
  @param          Is32BitPageTable Whether the page table is 32-bit PAE
  @return         PageTable Address

**/
UINT32
Gen4GPageTable (
  IN      UINTN                     ExtraPages,
  IN      BOOLEAN                   Is32BitPageTable
  );


/**
  Initialize global data for MP synchronization.

  @param Stacks       Base address of SMI stack buffer for all processors.
  @param StackSize    Stack size for each processor in SMM.

**/
UINT32
InitializeMpServiceData (
  IN VOID        *Stacks,
  IN UINTN       StackSize
  );

/**
  Initialize Timer for SMM AP Sync.

**/
VOID
InitializeSmmTimer (
  VOID
  );

/**
  Start Timer for SMM AP Sync.

**/
UINT64
EFIAPI
StartSyncTimer (
  VOID
  );

/**
  Check if the SMM AP Sync timer is timeout.

  @param Timer  The start timer from the begin.

**/
BOOLEAN
EFIAPI
IsSyncTimerTimeout (
  IN      UINT64                    Timer
  );

/**
  Initialize IDT for SMM Stack Guard.

**/
VOID
EFIAPI
InitializeIDTSmmStackGuard (
  VOID
  );

/**
  Initialize Gdt for all processors.
  
  @param[in]   Cr3          CR3 value.
  @param[out]  GdtStepSize  The step size for GDT table.

  @return GdtBase for processor 0.
          GdtBase for processor X is: GdtBase + (GdtStepSize * X)
**/
VOID *
InitGdt (
  IN  UINTN  Cr3,
  OUT UINTN  *GdtStepSize
  );

/**

  Register the SMM Foundation entry point.

  @param          This              Pointer to EFI_SMM_CONFIGURATION_PROTOCOL instance
  @param          SmmEntryPoint     SMM Foundation EntryPoint

  @retval         EFI_SUCCESS       Successfully to register SMM foundation entry point

**/
EFI_STATUS
EFIAPI
RegisterSmmEntry (
  IN CONST EFI_SMM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_SMM_ENTRY_POINT                   SmmEntryPoint
  );

/**
  Create PageTable for SMM use.

  @return     PageTable Address

**/
UINT32
SmmInitPageTable (
  VOID
  );

/**
  Schedule a procedure to run on the specified CPU.

  @param   Procedure        The address of the procedure to run
  @param   CpuIndex         Target CPU number
  @param   ProcArguments    The parameter to pass to the procedure

  @retval   EFI_INVALID_PARAMETER    CpuNumber not valid
  @retval   EFI_INVALID_PARAMETER    CpuNumber specifying BSP
  @retval   EFI_INVALID_PARAMETER    The AP specified by CpuNumber did not enter SMM
  @retval   EFI_INVALID_PARAMETER    The AP specified by CpuNumber is busy
  @retval   EFI_SUCCESS - The procedure has been successfully scheduled

**/
EFI_STATUS
EFIAPI
SmmStartupThisAp (
  IN      EFI_AP_PROCEDURE          Procedure,
  IN      UINTN                     CpuIndex,
  IN OUT  VOID                      *ProcArguments OPTIONAL
  );

/**
  Schedule a procedure to run on the specified CPU in a blocking fashion.

  @param  Procedure                The address of the procedure to run
  @param  CpuIndex                 Target CPU Index
  @param  ProcArguments            The parameter to pass to the procedure

  @retval EFI_INVALID_PARAMETER    CpuNumber not valid
  @retval EFI_INVALID_PARAMETER    CpuNumber specifying BSP
  @retval EFI_INVALID_PARAMETER    The AP specified by CpuNumber did not enter SMM
  @retval EFI_INVALID_PARAMETER    The AP specified by CpuNumber is busy
  @retval EFI_SUCCESS              The procedure has been successfully scheduled

**/
EFI_STATUS
EFIAPI
SmmBlockingStartupThisAp (
  IN      EFI_AP_PROCEDURE          Procedure,
  IN      UINTN                     CpuIndex,
  IN OUT  VOID                      *ProcArguments OPTIONAL
  );

/**
  Initialize MP synchronization data.

**/
VOID
EFIAPI
InitializeMpSyncData (
  VOID
  );

/**

  Find out SMRAM information including SMRR base and SMRR size.

  @param          SmrrBase          SMRR base
  @param          SmrrSize          SMRR size

**/
VOID
FindSmramInfo (
  OUT UINT32   *SmrrBase,
  OUT UINT32   *SmrrSize
  );

/**
  The function is invoked before SMBASE relocation in S3 path to restores CPU status.

  The function is invoked before SMBASE relocation in S3 path. It does first time microcode load
  and restores MTRRs for both BSP and APs.

**/
VOID
EarlyInitializeCpu (
  VOID
  );

/**
  The function is invoked after SMBASE relocation in S3 path to restores CPU status.

  The function is invoked after SMBASE relocation in S3 path. It restores configuration according to
  data saved by normal boot path for both BSP and APs.

**/
VOID
InitializeCpu (
  VOID
  );

/**
  Page Fault handler for SMM use.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.
**/
VOID
EFIAPI
SmiPFHandler (
    IN EFI_EXCEPTION_TYPE   InterruptType,
    IN EFI_SYSTEM_CONTEXT   SystemContext
  );

/**
  Perform the remaining tasks.

**/
VOID
PerformRemainingTasks (
  VOID
  );

/**
  Perform the pre tasks.

**/
VOID
PerformPreTasks (
  VOID
  );

/**
  Initialize MSR spin lock by MSR index.

  @param  MsrIndex       MSR index value.

**/
VOID
InitMsrSpinLockByIndex (
  IN UINT32      MsrIndex
  );

/**
  Hook return address of SMM Save State so that semaphore code
  can be executed immediately after AP exits SMM to indicate to
  the BSP that an AP has exited SMM after SMBASE relocation.

  @param[in] CpuIndex     The processor index.
  @param[in] RebasedFlag  A pointer to a flag that is set to TRUE
                          immediately after AP exits SMM.

**/
VOID
SemaphoreHook (
  IN UINTN             CpuIndex,
  IN volatile BOOLEAN  *RebasedFlag
  );

/**
Configure SMM Code Access Check feature for all processors.
SMM Feature Control MSR will be locked after configuration.
**/
VOID
ConfigSmmCodeAccessCheck (
  VOID
  );

/**
  Hook the code executed immediately after an RSM instruction on the currently
  executing CPU.  The mode of code executed immediately after RSM must be
  detected, and the appropriate hook must be selected.  Always clear the auto
  HALT restart flag if it is set.

  @param[in] CpuIndex                 The processor index for the currently
                                      executing CPU.
  @param[in] CpuState                 Pointer to SMRAM Save State Map for the
                                      currently executing CPU.
  @param[in] NewInstructionPointer32  Instruction pointer to use if resuming to
                                      32-bit mode from 64-bit SMM.
  @param[in] NewInstructionPointer    Instruction pointer to use if resuming to
                                      same mode as SMM.

  @retval The value of the original instruction pointer before it was hooked.

**/
UINT64
EFIAPI
HookReturnFromSmm (
  IN UINTN              CpuIndex,
  SMRAM_SAVE_STATE_MAP  *CpuState,
  UINT64                NewInstructionPointer32,
  UINT64                NewInstructionPointer
  );

/**
  Get the size of the SMI Handler in bytes.

  @retval The size, in bytes, of the SMI Handler.

**/
UINTN
EFIAPI
GetSmiHandlerSize (
  VOID
  );

/**
  Install the SMI handler for the CPU specified by CpuIndex.  This function
  is called by the CPU that was elected as monarch during System Management
  Mode initialization.

  @param[in] CpuIndex   The index of the CPU to install the custom SMI handler.
                        The value must be between 0 and the NumberOfCpus field
                        in the System Management System Table (SMST).
  @param[in] SmBase     The SMBASE address for the CPU specified by CpuIndex.
  @param[in] SmiStack   The stack to use when an SMI is processed by the
                        the CPU specified by CpuIndex.
  @param[in] StackSize  The size, in bytes, if the stack used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] GdtBase    The base address of the GDT to use when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] GdtSize    The size, in bytes, of the GDT used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] IdtBase    The base address of the IDT to use when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] IdtSize    The size, in bytes, of the IDT used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] Cr3        The base address of the page tables to use when an SMI
                        is processed by the CPU specified by CpuIndex.
**/
VOID
EFIAPI
InstallSmiHandler (
  IN UINTN   CpuIndex,
  IN UINT32  SmBase,
  IN VOID    *SmiStack,
  IN UINTN   StackSize,
  IN UINTN   GdtBase,
  IN UINTN   GdtSize,
  IN UINTN   IdtBase,
  IN UINTN   IdtSize,
  IN UINT32  Cr3
  );

/**
  Search module name by input IP address and output it.

  @param CallerIpAddress   Caller instruction pointer.

**/
VOID
DumpModuleInfoByIp (
  IN  UINTN              CallerIpAddress
  );

/**
  This API provides a way to allocate memory for page table.

  This API can be called more once to allocate memory for page tables.

  Allocates the number of 4KB pages of type EfiRuntimeServicesData and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
AllocatePageTableMemory (
  IN UINTN           Pages
  );

#endif
