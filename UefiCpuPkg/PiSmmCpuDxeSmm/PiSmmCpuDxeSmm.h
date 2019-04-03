/** @file
Agent Module to load other modules to deploy SMM Entry Vector for X86 CPU.

Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

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
#include <Protocol/SmmMemoryAttribute.h>

#include <Guid/AcpiS3Context.h>
#include <Guid/MemoryAttributesTable.h>
#include <Guid/PiSmmMemoryAttributesTable.h>

#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/MtrrLib.h>
#include <Library/SmmCpuPlatformHookLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/LocalApicLib.h>
#include <Library/UefiCpuLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/SmmCpuFeaturesLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/RegisterCpuFeaturesLib.h>

#include <AcpiCpuData.h>
#include <CpuHotPlugData.h>

#include <Register/Cpuid.h>
#include <Register/Msr.h>

#include "CpuService.h"
#include "SmmProfile.h"

//
// CET definition
//
#define CPUID_CET_SS   BIT7
#define CPUID_CET_IBT  BIT20

#define CR4_CET_ENABLE  BIT23

#define MSR_IA32_S_CET                     0x6A2
#define MSR_IA32_PL0_SSP                   0x6A4
#define MSR_IA32_INTERRUPT_SSP_TABLE_ADDR  0x6A8

typedef union {
  struct {
    // enable shadow stacks
    UINT32  SH_STK_ENP:1;
    // enable the WRSS{D,Q}W instructions.
    UINT32  WR_SHSTK_EN:1;
    // enable tracking of indirect call/jmp targets to be ENDBRANCH instruction.
    UINT32  ENDBR_EN:1;
    // enable legacy compatibility treatment for indirect call/jmp tracking.
    UINT32  LEG_IW_EN:1;
    // enable use of no-track prefix on indirect call/jmp.
    UINT32  NO_TRACK_EN:1;
    // disable suppression of CET indirect branch tracking on legacy compatibility.
    UINT32  SUPPRESS_DIS:1;
    UINT32  RSVD:4;
    // indirect branch tracking is suppressed.
    // This bit can be written to 1 only if TRACKER is written as IDLE.
    UINT32  SUPPRESS:1;
    // Value of the endbranch state machine
    // Values: IDLE (0), WAIT_FOR_ENDBRANCH(1).
    UINT32  TRACKER:1;
    // linear address of a bitmap in memory indicating valid
    // pages as target of CALL/JMP_indirect that do not land on ENDBRANCH when CET is enabled
    // and not suppressed. Valid when ENDBR_EN is 1. Must be machine canonical when written on
    // parts that support 64 bit mode. On parts that do not support 64 bit mode, the bits 63:32 are
    // reserved and must be 0. This value is extended by 12 bits at the low end to form the base address
    // (this automatically aligns the address on a 4-Kbyte boundary).
    UINT32  EB_LEG_BITMAP_BASE_low:12;
    UINT32  EB_LEG_BITMAP_BASE_high:32;
  } Bits;
  UINT64   Uint64;
} MSR_IA32_CET;

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

#define PAGE_ATTRIBUTE_BITS         (IA32_PG_D | IA32_PG_A | IA32_PG_U | IA32_PG_RW | IA32_PG_P)
//
// Bits 1, 2, 5, 6 are reserved in the IA32 PAE PDPTE
// X64 PAE PDPTE does not have such restriction
//
#define IA32_PAE_PDPTE_ATTRIBUTE_BITS    (IA32_PG_P)

#define PAGE_PROGATE_BITS           (IA32_PG_NX | PAGE_ATTRIBUTE_BITS)

#define PAGING_4K_MASK  0xFFF
#define PAGING_2M_MASK  0x1FFFFF
#define PAGING_1G_MASK  0x3FFFFFFF

#define PAGING_PAE_INDEX_MASK  0x1FF

#define PAGING_4K_ADDRESS_MASK_64 0x000FFFFFFFFFF000ull
#define PAGING_2M_ADDRESS_MASK_64 0x000FFFFFFFE00000ull
#define PAGING_1G_ADDRESS_MASK_64 0x000FFFFFC0000000ull

#define SMRR_MAX_ADDRESS       BASE_4GB

typedef enum {
  PageNone,
  Page4K,
  Page2M,
  Page1G,
} PAGE_ATTRIBUTE;

typedef struct {
  PAGE_ATTRIBUTE   Attribute;
  UINT64           Length;
  UINT64           AddressMask;
} PAGE_ATTRIBUTE_TABLE;

//
// Size of Task-State Segment defined in IA32 Manual
//
#define TSS_SIZE              104
#define EXCEPTION_TSS_SIZE    (TSS_SIZE + 4) // Add 4 bytes SSP
#define TSS_X64_IST1_OFFSET   36
#define TSS_IA32_CR3_OFFSET   28
#define TSS_IA32_ESP_OFFSET   56
#define TSS_IA32_SSP_OFFSET   104

#define CR0_WP                BIT16

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

extern CONST UINT8                  gcSmmInitTemplate[];
extern CONST UINT16                 gcSmmInitSize;
X86_ASSEMBLY_PATCH_LABEL            gPatchSmmCr0;
extern UINT32                       mSmmCr0;
X86_ASSEMBLY_PATCH_LABEL            gPatchSmmCr3;
extern UINT32                       mSmmCr4;
X86_ASSEMBLY_PATCH_LABEL            gPatchSmmCr4;
X86_ASSEMBLY_PATCH_LABEL            gPatchSmmInitStack;
X86_ASSEMBLY_PATCH_LABEL            mPatchCetSupported;
extern BOOLEAN                      mCetSupported;

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
  SPIN_LOCK                         *Busy;
  volatile EFI_AP_PROCEDURE         Procedure;
  volatile VOID                     *Parameter;
  volatile UINT32                   *Run;
  volatile BOOLEAN                  *Present;
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
  volatile UINT32               *Counter;
  volatile UINT32               BspIndex;
  volatile BOOLEAN              *InsideSmm;
  volatile BOOLEAN              *AllCpusInSync;
  volatile SMM_CPU_SYNC_MODE    EffectiveSyncMode;
  volatile BOOLEAN              SwitchBsp;
  volatile BOOLEAN              *CandidateBsp;
} SMM_DISPATCHER_MP_SYNC_DATA;

#define SMM_PSD_OFFSET              0xfb00

///
/// All global semaphores' pointer
///
typedef struct {
  volatile UINT32      *Counter;
  volatile BOOLEAN     *InsideSmm;
  volatile BOOLEAN     *AllCpusInSync;
  SPIN_LOCK            *PFLock;
  SPIN_LOCK            *CodeAccessCheckLock;
} SMM_CPU_SEMAPHORE_GLOBAL;

///
/// All semaphores for each processor
///
typedef struct {
  SPIN_LOCK                         *Busy;
  volatile UINT32                   *Run;
  volatile BOOLEAN                  *Present;
} SMM_CPU_SEMAPHORE_CPU;

///
/// All semaphores' information
///
typedef struct {
  SMM_CPU_SEMAPHORE_GLOBAL          SemaphoreGlobal;
  SMM_CPU_SEMAPHORE_CPU             SemaphoreCpu;
} SMM_CPU_SEMAPHORES;

extern IA32_DESCRIPTOR                     gcSmiGdtr;
extern EFI_PHYSICAL_ADDRESS                mGdtBuffer;
extern UINTN                               mGdtBufferSize;
extern IA32_DESCRIPTOR                     gcSmiIdtr;
extern VOID                                *gcSmiIdtrPtr;
extern UINT64                              gPhyMask;
extern SMM_DISPATCHER_MP_SYNC_DATA         *mSmmMpSyncData;
extern UINTN                               mSmmStackArrayBase;
extern UINTN                               mSmmStackArrayEnd;
extern UINTN                               mSmmStackSize;
extern EFI_SMM_CPU_SERVICE_PROTOCOL        mSmmCpuService;
extern IA32_DESCRIPTOR                     gcSmiInitGdtr;
extern SMM_CPU_SEMAPHORES                  mSmmCpuSemaphores;
extern UINTN                               mSemaphoreSize;
extern SPIN_LOCK                           *mPFLock;
extern SPIN_LOCK                           *mConfigSmmCodeAccessCheckLock;
extern EFI_SMRAM_DESCRIPTOR                *mSmmCpuSmramRanges;
extern UINTN                               mSmmCpuSmramRangeCount;
extern UINT8                               mPhysicalAddressBits;

//
// Copy of the PcdPteMemoryEncryptionAddressOrMask
//
extern UINT64  mAddressEncMask;

/**
  Create 4G PageTable in SMRAM.

  @param[in]      Is32BitPageTable Whether the page table is 32-bit PAE
  @return         PageTable Address

**/
UINT32
Gen4GPageTable (
  IN      BOOLEAN                   Is32BitPageTable
  );


/**
  Initialize global data for MP synchronization.

  @param Stacks             Base address of SMI stack buffer for all processors.
  @param StackSize          Stack size for each processor in SMM.
  @param ShadowStackSize    Shadow Stack size for each processor in SMM.

**/
UINT32
InitializeMpServiceData (
  IN VOID        *Stacks,
  IN UINTN       StackSize,
  IN UINTN       ShadowStackSize
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
  This function sets the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param[in]  BaseAddress      The physical address that is the start address of a memory region.
  @param[in]  Length           The size in bytes of the memory region.
  @param[in]  Attributes       The bit mask of attributes to set for the memory region.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of attributes that
                                cannot be set together.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
SmmSetMemoryAttributes (
  IN  EFI_PHYSICAL_ADDRESS                       BaseAddress,
  IN  UINT64                                     Length,
  IN  UINT64                                     Attributes
  );

/**
  This function clears the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param[in]  BaseAddress      The physical address that is the start address of a memory region.
  @param[in]  Length           The size in bytes of the memory region.
  @param[in]  Attributes       The bit mask of attributes to clear for the memory region.

  @retval EFI_SUCCESS           The attributes were cleared for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of attributes that
                                cannot be set together.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
SmmClearMemoryAttributes (
  IN  EFI_PHYSICAL_ADDRESS                       BaseAddress,
  IN  UINT64                                     Length,
  IN  UINT64                                     Attributes
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
  Relocate SmmBases for each processor.

  Execute on first boot and all S3 resumes

**/
VOID
EFIAPI
SmmRelocateBases (
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
  This function sets memory attribute according to MemoryAttributesTable.
**/
VOID
SetMemMapAttributes (
  VOID
  );

/**
  This function sets UEFI memory attribute according to UEFI memory map.
**/
VOID
SetUefiMemMapAttributes (
  VOID
  );

/**
  Return if the Address is forbidden as SMM communication buffer.

  @param[in] Address the address to be checked

  @return TRUE  The address is forbidden as SMM communication buffer.
  @return FALSE The address is allowed as SMM communication buffer.
**/
BOOLEAN
IsSmmCommBufferForbiddenAddress (
  IN UINT64  Address
  );

/**
  This function caches the UEFI memory map information.
**/
VOID
GetUefiMemoryMap (
  VOID
  );

/**
  This function sets memory attribute for page table.
**/
VOID
SetPageTableAttributes (
  VOID
  );

/**
  Return page table base.

  @return page table base.
**/
UINTN
GetPageTableBase (
  VOID
  );

/**
  This function sets the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param[in]   BaseAddress      The physical address that is the start address of a memory region.
  @param[in]   Length           The size in bytes of the memory region.
  @param[in]   Attributes       The bit mask of attributes to set for the memory region.
  @param[out]  IsSplitted       TRUE means page table splitted. FALSE means page table not splitted.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of attributes that
                                cannot be set together.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
SmmSetMemoryAttributesEx (
  IN  EFI_PHYSICAL_ADDRESS                       BaseAddress,
  IN  UINT64                                     Length,
  IN  UINT64                                     Attributes,
  OUT BOOLEAN                                    *IsSplitted  OPTIONAL
  );

/**
  This function clears the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param[in]   BaseAddress      The physical address that is the start address of a memory region.
  @param[in]   Length           The size in bytes of the memory region.
  @param[in]   Attributes       The bit mask of attributes to clear for the memory region.
  @param[out]  IsSplitted       TRUE means page table splitted. FALSE means page table not splitted.

  @retval EFI_SUCCESS           The attributes were cleared for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of attributes that
                                cannot be set together.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
SmmClearMemoryAttributesEx (
  IN  EFI_PHYSICAL_ADDRESS                       BaseAddress,
  IN  UINT64                                     Length,
  IN  UINT64                                     Attributes,
  OUT BOOLEAN                                    *IsSplitted  OPTIONAL
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

/**
  Allocate pages for code.

  @param[in]  Pages Number of pages to be allocated.

  @return Allocated memory.
**/
VOID *
AllocateCodePages (
  IN UINTN           Pages
  );

/**
  Allocate aligned pages for code.

  @param[in]  Pages                 Number of pages to be allocated.
  @param[in]  Alignment             The requested alignment of the allocation.
                                    Must be a power of two.
                                    If Alignment is zero, then byte alignment is used.

  @return Allocated memory.
**/
VOID *
AllocateAlignedCodePages (
  IN UINTN            Pages,
  IN UINTN            Alignment
  );


//
// S3 related global variable and function prototype.
//

extern BOOLEAN                mSmmS3Flag;

/**
  Initialize SMM S3 resume state structure used during S3 Resume.

  @param[in] Cr3    The base address of the page tables to use in SMM.

**/
VOID
InitSmmS3ResumeState (
  IN UINT32  Cr3
  );

/**
  Get ACPI CPU data.

**/
VOID
GetAcpiCpuData (
  VOID
  );

/**
  Restore SMM Configuration in S3 boot path.

**/
VOID
RestoreSmmConfigurationInS3 (
  VOID
  );

/**
  Get ACPI S3 enable flag.

**/
VOID
GetAcpiS3EnableFlag (
  VOID
  );

/**
  Transfer AP to safe hlt-loop after it finished restore CPU features on S3 patch.

  @param[in] ApHltLoopCode          The address of the safe hlt-loop function.
  @param[in] TopOfStack             A pointer to the new stack to use for the ApHltLoopCode.
  @param[in] NumberToFinishAddress  Address of Semaphore of APs finish count.

**/
VOID
TransferApToSafeState (
  IN UINTN  ApHltLoopCode,
  IN UINTN  TopOfStack,
  IN UINTN  NumberToFinishAddress
  );

/**
  Set ShadowStack memory.

  @param[in]  Cr3              The page table base address.
  @param[in]  BaseAddress      The physical address that is the start address of a memory region.
  @param[in]  Length           The size in bytes of the memory region.

  @retval EFI_SUCCESS           The shadow stack memory is set.
**/
EFI_STATUS
SetShadowStack (
  IN  UINTN                                      Cr3,
  IN  EFI_PHYSICAL_ADDRESS                       BaseAddress,
  IN  UINT64                                     Length
  );

/**
  Set not present memory.

  @param[in]  Cr3              The page table base address.
  @param[in]  BaseAddress      The physical address that is the start address of a memory region.
  @param[in]  Length           The size in bytes of the memory region.

  @retval EFI_SUCCESS           The not present memory is set.
**/
EFI_STATUS
SetNotPresentPage (
  IN  UINTN                                      Cr3,
  IN  EFI_PHYSICAL_ADDRESS                       BaseAddress,
  IN  UINT64                                     Length
  );

/**
  Initialize the shadow stack related data structure.

  @param CpuIndex     The index of CPU.
  @param ShadowStack  The bottom of the shadow stack for this CPU.
**/
VOID
InitShadowStack (
  IN UINTN  CpuIndex,
  IN VOID   *ShadowStack
  );

/**
  This function set given attributes of the memory region specified by
  BaseAddress and Length.

  @param  This              The EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL instance.
  @param  BaseAddress       The physical address that is the start address of
                            a memory region.
  @param  Length            The size in bytes of the memory region.
  @param  Attributes        The bit mask of attributes to set for the memory
                            region.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of
                                attributes that cannot be set together.
  @retval EFI_UNSUPPORTED       The processor does not support one or more
                                bytes of the memory resource range specified
                                by BaseAddress and Length.
                                The bit mask of attributes is not supported for
                                the memory resource range specified by
                                BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
EdkiiSmmSetMemoryAttributes (
  IN  EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL   *This,
  IN  EFI_PHYSICAL_ADDRESS                  BaseAddress,
  IN  UINT64                                Length,
  IN  UINT64                                Attributes
  );

/**
  This function clears given attributes of the memory region specified by
  BaseAddress and Length.

  @param  This              The EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL instance.
  @param  BaseAddress       The physical address that is the start address of
                            a memory region.
  @param  Length            The size in bytes of the memory region.
  @param  Attributes        The bit mask of attributes to clear for the memory
                            region.

  @retval EFI_SUCCESS           The attributes were cleared for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of
                                attributes that cannot be cleared together.
  @retval EFI_UNSUPPORTED       The processor does not support one or more
                                bytes of the memory resource range specified
                                by BaseAddress and Length.
                                The bit mask of attributes is not supported for
                                the memory resource range specified by
                                BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
EdkiiSmmClearMemoryAttributes (
  IN  EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL   *This,
  IN  EFI_PHYSICAL_ADDRESS                  BaseAddress,
  IN  UINT64                                Length,
  IN  UINT64                                Attributes
  );

/**
  This function retrieves the attributes of the memory region specified by
  BaseAddress and Length. If different attributes are got from different part
  of the memory region, EFI_NO_MAPPING will be returned.

  @param  This              The EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL instance.
  @param  BaseAddress       The physical address that is the start address of
                            a memory region.
  @param  Length            The size in bytes of the memory region.
  @param  Attributes        Pointer to attributes returned.

  @retval EFI_SUCCESS           The attributes got for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes is NULL.
  @retval EFI_NO_MAPPING        Attributes are not consistent cross the memory
                                region.
  @retval EFI_UNSUPPORTED       The processor does not support one or more
                                bytes of the memory resource range specified
                                by BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
EdkiiSmmGetMemoryAttributes (
  IN  EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL   *This,
  IN  EFI_PHYSICAL_ADDRESS                  BaseAddress,
  IN  UINT64                                Length,
  IN  UINT64                                *Attributes
  );

/**
  This function fixes up the address of the global variable or function
  referred in SmmInit assembly files to be the absoute address.
**/
VOID
EFIAPI
PiSmmCpuSmmInitFixupAddress (
 );

/**
  This function fixes up the address of the global variable or function
  referred in SmiEntry assembly files to be the absoute address.
**/
VOID
EFIAPI
PiSmmCpuSmiEntryFixupAddress (
 );

/**
  This function reads CR2 register when on-demand paging is enabled
  for 64 bit and no action for 32 bit.

  @param[out]  *Cr2  Pointer to variable to hold CR2 register value.
**/
VOID
SaveCr2 (
  OUT UINTN  *Cr2
  );

/**
  This function writes into CR2 register when on-demand paging is enabled
  for 64 bit and no action for 32 bit.

  @param[in]  Cr2  Value to write into CR2 register.
**/
VOID
RestoreCr2 (
  IN UINTN  Cr2
  );

#endif
