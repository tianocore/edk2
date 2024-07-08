/** @file
Agent Module to load other modules to deploy SMM Entry Vector for X86 CPU.

Copyright (c) 2009 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>
Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CPU_PISMMCPUDXESMM_H_
#define _CPU_PISMMCPUDXESMM_H_

#include <PiSmm.h>

#include <Protocol/SmmConfiguration.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmReadyToLock.h>
#include <Protocol/SmmCpuService.h>
#include <Protocol/SmmMemoryAttribute.h>
#include <Protocol/MmMp.h>
#include <Protocol/SmmVariable.h>

#include <Guid/AcpiS3Context.h>
#include <Guid/MemoryAttributesTable.h>
#include <Guid/PiSmmMemoryAttributesTable.h>
#include <Guid/SmramMemoryReserve.h>
#include <Guid/SmmBaseHob.h>
#include <Guid/MpInformation2.h>
#include <Guid/MmProfileData.h>
#include <Guid/MmAcpiS3Enable.h>
#include <Guid/MmCpuSyncConfig.h>

#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/MtrrLib.h>
#include <Library/SmmCpuPlatformHookLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/LocalApicLib.h>
#include <Library/CpuLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/SmmCpuFeaturesLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/RegisterCpuFeaturesLib.h>
#include <Library/PerformanceLib.h>
#include <Library/CpuPageTableLib.h>
#include <Library/MmSaveStateLib.h>
#include <Library/SmmCpuSyncLib.h>

#include <AcpiCpuData.h>
#include <CpuHotPlugData.h>

#include <Register/Intel/Cpuid.h>
#include <Register/Intel/Msr.h>

#include "CpuService.h"
#include "SmmProfile.h"
#include "SmmMpPerf.h"

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
    UINT32    SH_STK_ENP              : 1;
    // enable the WRSS{D,Q}W instructions.
    UINT32    WR_SHSTK_EN             : 1;
    // enable tracking of indirect call/jmp targets to be ENDBRANCH instruction.
    UINT32    ENDBR_EN                : 1;
    // enable legacy compatibility treatment for indirect call/jmp tracking.
    UINT32    LEG_IW_EN               : 1;
    // enable use of no-track prefix on indirect call/jmp.
    UINT32    NO_TRACK_EN             : 1;
    // disable suppression of CET indirect branch tracking on legacy compatibility.
    UINT32    SUPPRESS_DIS            : 1;
    UINT32    RSVD                    : 4;
    // indirect branch tracking is suppressed.
    // This bit can be written to 1 only if TRACKER is written as IDLE.
    UINT32    SUPPRESS                : 1;
    // Value of the endbranch state machine
    // Values: IDLE (0), WAIT_FOR_ENDBRANCH(1).
    UINT32    TRACKER                 : 1;
    // linear address of a bitmap in memory indicating valid
    // pages as target of CALL/JMP_indirect that do not land on ENDBRANCH when CET is enabled
    // and not suppressed. Valid when ENDBR_EN is 1. Must be machine canonical when written on
    // parts that support 64 bit mode. On parts that do not support 64 bit mode, the bits 63:32 are
    // reserved and must be 0. This value is extended by 12 bits at the low end to form the base address
    // (this automatically aligns the address on a 4-Kbyte boundary).
    UINT32    EB_LEG_BITMAP_BASE_low  : 12;
    UINT32    EB_LEG_BITMAP_BASE_high : 32;
  } Bits;
  UINT64    Uint64;
} MSR_IA32_CET;

//
// MSRs required for configuration of SMM Code Access Check
//
#define EFI_MSR_SMM_MCA_CAP       0x17D
#define  SMM_CODE_ACCESS_CHK_BIT  BIT58

#define  SMM_FEATURE_CONTROL_LOCK_BIT  BIT0
#define  SMM_CODE_CHK_EN_BIT           BIT2

///
/// Page Table Entry
///
#define IA32_PG_P       BIT0
#define IA32_PG_RW      BIT1
#define IA32_PG_U       BIT2
#define IA32_PG_WT      BIT3
#define IA32_PG_CD      BIT4
#define IA32_PG_A       BIT5
#define IA32_PG_D       BIT6
#define IA32_PG_PS      BIT7
#define IA32_PG_PAT_2M  BIT12
#define IA32_PG_PAT_4K  IA32_PG_PS
#define IA32_PG_PMNT    BIT62
#define IA32_PG_NX      BIT63

#define PAGE_ATTRIBUTE_BITS  (IA32_PG_D | IA32_PG_A | IA32_PG_U | IA32_PG_RW | IA32_PG_P)
//
// Bits 1, 2, 5, 6 are reserved in the IA32 PAE PDPTE
// X64 PAE PDPTE does not have such restriction
//
#define IA32_PAE_PDPTE_ATTRIBUTE_BITS  (IA32_PG_P)

#define PAGE_PROGATE_BITS  (IA32_PG_NX | PAGE_ATTRIBUTE_BITS)

#define PAGING_4K_MASK  0xFFF
#define PAGING_2M_MASK  0x1FFFFF
#define PAGING_1G_MASK  0x3FFFFFFF

#define PAGING_PAE_INDEX_MASK  0x1FF

#define PAGING_4K_ADDRESS_MASK_64  0x000FFFFFFFFFF000ull
#define PAGING_2M_ADDRESS_MASK_64  0x000FFFFFFFE00000ull
#define PAGING_1G_ADDRESS_MASK_64  0x000FFFFFC0000000ull

#define SMRR_MAX_ADDRESS  BASE_4GB

typedef enum {
  PageNone,
  Page4K,
  Page2M,
  Page1G,
} PAGE_ATTRIBUTE;

typedef struct {
  PAGE_ATTRIBUTE    Attribute;
  UINT64            Length;
  UINT64            AddressMask;
} PAGE_ATTRIBUTE_TABLE;

//
// Size of Task-State Segment defined in IA32 Manual
//
#define TSS_SIZE             104
#define EXCEPTION_TSS_SIZE   (TSS_SIZE + 4)  // Add 4 bytes SSP
#define TSS_X64_IST1_OFFSET  36
#define TSS_IA32_CR3_OFFSET  28
#define TSS_IA32_ESP_OFFSET  56
#define TSS_IA32_SSP_OFFSET  104

#define CR0_WP  BIT16

//
// Code select value
//
#define PROTECT_MODE_CODE_SEGMENT  0x08
#define LONG_MODE_CODE_SEGMENT     0x38

#define EXCEPTION_VECTOR_NUMBER  0x20

#define INVALID_APIC_ID  0xFFFFFFFFFFFFFFFFULL

//
// Wrapper used to convert EFI_AP_PROCEDURE2 and EFI_AP_PROCEDURE.
//
typedef struct {
  EFI_AP_PROCEDURE    Procedure;
  VOID                *ProcedureArgument;
} PROCEDURE_WRAPPER;

#define PROCEDURE_TOKEN_SIGNATURE  SIGNATURE_32 ('P', 'R', 'T', 'S')

typedef struct {
  UINTN              Signature;
  LIST_ENTRY         Link;

  SPIN_LOCK          *SpinLock;
  volatile UINT32    RunningApCount;
} PROCEDURE_TOKEN;

#define PROCEDURE_TOKEN_FROM_LINK(a)  CR (a, PROCEDURE_TOKEN, Link, PROCEDURE_TOKEN_SIGNATURE)

#define TOKEN_BUFFER_SIGNATURE  SIGNATURE_32 ('T', 'K', 'B', 'S')

typedef struct {
  UINTN         Signature;
  LIST_ENTRY    Link;

  UINT8         *Buffer;
} TOKEN_BUFFER;

#define TOKEN_BUFFER_FROM_LINK(a)  CR (a, TOKEN_BUFFER, Link, TOKEN_BUFFER_SIGNATURE)

//
// Private structure for the SMM CPU module that is stored in DXE Runtime memory
// Contains the SMM Configuration Protocols that is produced.
// Contains a mix of DXE and SMM contents.  All the fields must be used properly.
//
#define SMM_CPU_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('s', 'c', 'p', 'u')

typedef struct {
  UINTN                             Signature;

  EFI_HANDLE                        SmmCpuHandle;

  EFI_PROCESSOR_INFORMATION         *ProcessorInfo;
  SMM_CPU_OPERATION                 *Operation;
  UINTN                             *CpuSaveStateSize;
  VOID                              **CpuSaveState;

  EFI_SMM_RESERVED_SMRAM_REGION     SmmReservedSmramRegion[1];
  EFI_SMM_ENTRY_CONTEXT             SmmCoreEntryContext;
  EFI_SMM_ENTRY_POINT               SmmCoreEntry;

  EFI_SMM_CONFIGURATION_PROTOCOL    SmmConfiguration;

  PROCEDURE_WRAPPER                 *ApWrapperFunc;
  LIST_ENTRY                        TokenList;
  LIST_ENTRY                        *FirstFreeToken;
} SMM_CPU_PRIVATE_DATA;

extern const BOOLEAN  mIsStandaloneMm;

extern SMM_CPU_PRIVATE_DATA  *gSmmCpuPrivate;
extern CPU_HOT_PLUG_DATA     mCpuHotPlugData;
extern UINTN                 mMaxNumberOfCpus;
extern UINTN                 mNumberOfCpus;
extern EFI_SMM_CPU_PROTOCOL  mSmmCpu;
extern EFI_MM_MP_PROTOCOL    mSmmMp;
extern BOOLEAN               m5LevelPagingNeeded;
extern PAGING_MODE           mPagingMode;
extern UINTN                 mSmmShadowStackSize;

///
/// The mode of the CPU at the time an SMI occurs
///
extern UINT8  mSmmSaveStateRegisterLma;

extern BOOLEAN  mAcpiS3Enable;

#define PAGE_TABLE_POOL_ALIGNMENT   BASE_128KB
#define PAGE_TABLE_POOL_UNIT_SIZE   BASE_128KB
#define PAGE_TABLE_POOL_UNIT_PAGES  EFI_SIZE_TO_PAGES (PAGE_TABLE_POOL_UNIT_SIZE)
#define PAGE_TABLE_POOL_ALIGN_MASK  \
  (~(EFI_PHYSICAL_ADDRESS)(PAGE_TABLE_POOL_ALIGNMENT - 1))

typedef struct {
  VOID     *NextPool;
  UINTN    Offset;
  UINTN    FreePages;
} PAGE_TABLE_POOL;

/**
  Disable CET.
**/
VOID
EFIAPI
DisableCet (
  VOID
  );

/**
  Enable CET.
**/
VOID
EFIAPI
EnableCet (
  VOID
  );

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
  @retval EFI_INVALID_PARAMETER   This or Buffer is NULL.

**/
EFI_STATUS
EFIAPI
SmmReadSaveState (
  IN CONST EFI_SMM_CPU_PROTOCOL   *This,
  IN UINTN                        Width,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        CpuIndex,
  OUT VOID                        *Buffer
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
  @retval EFI_INVALID_PARAMETER   ProcessorIndex or Width is not correct

**/
EFI_STATUS
EFIAPI
SmmWriteSaveState (
  IN CONST EFI_SMM_CPU_PROTOCOL   *This,
  IN UINTN                        Width,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        CpuIndex,
  IN CONST VOID                   *Buffer
  );

/**
  Initialize SMM environment.

**/
VOID
InitializeSmm (
  VOID
  );

/**
  Issue SMI IPI (All Excluding Self SMM IPI + BSP SMM IPI) to execute first SMI init.

**/
VOID
ExecuteFirstSmiInit (
  VOID
  );

extern volatile  BOOLEAN  *mSmmInitialized;
extern UINT32             mBspApicId;

X86_ASSEMBLY_PATCH_LABEL  mPatchCetSupported;
extern BOOLEAN            mCetSupported;

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
  SPIN_LOCK                     *Busy;
  volatile EFI_AP_PROCEDURE2    Procedure;
  volatile VOID                 *Parameter;
  volatile BOOLEAN              *Present;
  PROCEDURE_TOKEN               *Token;
  EFI_STATUS                    *Status;
} SMM_CPU_DATA_BLOCK;

typedef struct {
  //
  // Pointer to an array. The array should be located immediately after this structure
  // so that UC cache-ability can be set together.
  //
  SMM_CPU_DATA_BLOCK           *CpuData;
  volatile UINT32              BspIndex;
  volatile BOOLEAN             *InsideSmm;
  volatile BOOLEAN             *AllCpusInSync;
  volatile MM_CPU_SYNC_MODE    EffectiveSyncMode;
  volatile BOOLEAN             SwitchBsp;
  volatile BOOLEAN             *CandidateBsp;
  volatile BOOLEAN             AllApArrivedWithException;
  EFI_AP_PROCEDURE             StartupProcedure;
  VOID                         *StartupProcArgs;
  SMM_CPU_SYNC_CONTEXT         *SyncContext;
} SMM_DISPATCHER_MP_SYNC_DATA;

#define SMM_PSD_OFFSET  0xfb00

///
/// All global semaphores' pointer
///
typedef struct {
  volatile BOOLEAN    *InsideSmm;
  volatile BOOLEAN    *AllCpusInSync;
  SPIN_LOCK           *PFLock;
  SPIN_LOCK           *CodeAccessCheckLock;
} SMM_CPU_SEMAPHORE_GLOBAL;

///
/// All semaphores for each processor
///
typedef struct {
  SPIN_LOCK           *Busy;
  volatile BOOLEAN    *Present;
  SPIN_LOCK           *Token;
} SMM_CPU_SEMAPHORE_CPU;

///
/// All semaphores' information
///
typedef struct {
  SMM_CPU_SEMAPHORE_GLOBAL    SemaphoreGlobal;
  SMM_CPU_SEMAPHORE_CPU       SemaphoreCpu;
} SMM_CPU_SEMAPHORES;

extern IA32_DESCRIPTOR               gcSmiGdtr;
extern EFI_PHYSICAL_ADDRESS          mGdtBuffer;
extern UINTN                         mGdtBufferSize;
extern IA32_DESCRIPTOR               gcSmiIdtr;
extern VOID                          *gcSmiIdtrPtr;
extern UINT64                        gPhyMask;
extern SMM_DISPATCHER_MP_SYNC_DATA   *mSmmMpSyncData;
extern UINTN                         mSmmStackArrayBase;
extern UINTN                         mSmmStackArrayEnd;
extern UINTN                         mSmmStackSize;
extern EFI_SMM_CPU_SERVICE_PROTOCOL  mSmmCpuService;
extern SMM_CPU_SEMAPHORES            mSmmCpuSemaphores;
extern UINTN                         mSemaphoreSize;
extern SPIN_LOCK                     *mPFLock;
extern SPIN_LOCK                     *mConfigSmmCodeAccessCheckLock;
extern EFI_SMRAM_DESCRIPTOR          *mSmmCpuSmramRanges;
extern UINTN                         mSmmCpuSmramRangeCount;
extern UINT8                         mPhysicalAddressBits;
extern BOOLEAN                       mSmmDebugAgentSupport;
extern BOOLEAN                       mSmmCodeAccessCheckEnable;

//
// Copy of the PcdPteMemoryEncryptionAddressOrMask
//
extern UINT64  mAddressEncMask;

extern UINT64  mTimeoutTicker;
extern UINT64  mTimeoutTicker2;

typedef struct {
  ///
  /// Address of the first byte in the memory region.
  ///
  EFI_PHYSICAL_ADDRESS    Base;
  ///
  /// Length in bytes of the memory region.
  ///
  UINT64                  Length;
  ///
  /// Attributes of the memory region
  ///
  UINT64                  Attribute;
} MM_CPU_MEMORY_REGION;

/**
  Create 4G PageTable in SMRAM.

  @param[in]      Is32BitPageTable Whether the page table is 32-bit PAE
  @return         PageTable Address

**/
UINT32
Gen4GPageTable (
  IN      BOOLEAN  Is32BitPageTable
  );

/**
  Create page table based on input PagingMode and PhysicalAddressBits in smm.

  @param[in]      PagingMode           The paging mode.
  @param[in]      PhysicalAddressBits  The bits of physical address to map.

  @retval         PageTable Address

**/
UINTN
GenSmmPageTable (
  IN PAGING_MODE  PagingMode,
  IN UINT8        PhysicalAddressBits
  );

/**
  Initialize global data for MP synchronization.

  @param Stacks             Base address of SMI stack buffer for all processors.
  @param StackSize          Stack size for each processor in SMM.
  @param ShadowStackSize    Shadow Stack size for each processor in SMM.

**/
UINT32
InitializeMpServiceData (
  IN VOID   *Stacks,
  IN UINTN  StackSize,
  IN UINTN  ShadowStackSize
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
  Check if the SMM AP Sync Timer is timeout specified by Timeout.

  @param Timer    The start timer from the begin.
  @param Timeout  The timeout ticker to wait.

**/
BOOLEAN
EFIAPI
IsSyncTimerTimeout (
  IN      UINT64  Timer,
  IN      UINT64  Timeout
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
  Initialize IDT IST Field.

  @param[in]  ExceptionType       Exception type.
  @param[in]  Ist                 IST value.

**/
VOID
EFIAPI
InitializeIdtIst (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN UINT8               Ist
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
  IN      EFI_AP_PROCEDURE  Procedure,
  IN      UINTN             CpuIndex,
  IN OUT  VOID              *ProcArguments OPTIONAL
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
  IN      EFI_AP_PROCEDURE  Procedure,
  IN      UINTN             CpuIndex,
  IN OUT  VOID              *ProcArguments OPTIONAL
  );

/**
  This function modifies the page attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  Caller should make sure BaseAddress and Length is at page boundary.

  @param[in]   PageTableBase    The page table base.
  @param[in]   BaseAddress      The physical address that is the start address of a memory region.
  @param[in]   Length           The size in bytes of the memory region.
  @param[in]   Attributes       The bit mask of attributes to modify for the memory region.
  @param[in]   IsSet            TRUE means to set attributes. FALSE means to clear attributes.
  @param[out]  IsModified       TRUE means page table modified. FALSE means page table not modified.

  @retval RETURN_SUCCESS           The attributes were modified for the memory region.
  @retval RETURN_ACCESS_DENIED     The attributes for the memory resource range specified by
                                   BaseAddress and Length cannot be modified.
  @retval RETURN_INVALID_PARAMETER Length is zero.
                                   Attributes specified an illegal combination of attributes that
                                   cannot be set together.
  @retval RETURN_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                   the memory resource range.
  @retval RETURN_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                   resource range specified by BaseAddress and Length.
                                   The bit mask of attributes is not support for the memory resource
                                   range specified by BaseAddress and Length.
**/
RETURN_STATUS
ConvertMemoryPageAttributes (
  IN  UINTN             PageTableBase,
  IN  PAGING_MODE       PagingMode,
  IN  PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64            Length,
  IN  UINT64            Attributes,
  IN  BOOLEAN           IsSet,
  OUT BOOLEAN           *IsModified   OPTIONAL
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
SmmSetMemoryAttributes (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT64                Attributes
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
SmmClearMemoryAttributes (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT64                Attributes
  );

/**
  Retrieves a pointer to the system configuration table from the SMM System Table
  based on a specified GUID.

  @param[in]   TableGuid       The pointer to table's GUID type.
  @param[out]  Table           The pointer to the table associated with TableGuid in the EFI System Table.

  @retval EFI_SUCCESS     A configuration table matching TableGuid was found.
  @retval EFI_NOT_FOUND   A configuration table matching TableGuid could not be found.

**/
EFI_STATUS
EFIAPI
SmmGetSystemConfigurationTable (
  IN  EFI_GUID  *TableGuid,
  OUT VOID      **Table
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
  OUT UINT32  *SmrrBase,
  OUT UINT32  *SmrrSize
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
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

/**
  Check SmmProfile is enabled or not.

  @return TRUE     SmmProfile is enabled.
          FALSE    SmmProfile is not enabled.

**/
BOOLEAN
IsSmmProfileEnabled (
  VOID
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
  IN UINT32  MsrIndex
  );

/**
Configure SMM Code Access Check feature on an AP.
SMM Feature Control MSR will be locked after configuration.

@param[in,out] Buffer  Pointer to private data buffer.
**/
VOID
EFIAPI
ConfigSmmCodeAccessCheckOnCurrentProcessor (
  IN OUT VOID  *Buffer
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
  IN  UINTN  CallerIpAddress
  );

/**
  This function sets memory attribute according to MemoryAttributesTable.

  @param  MemoryAttributesTable  A pointer to the buffer of SmmMemoryAttributesTable.

**/
VOID
SetMemMapAttributes (
  EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE  *MemoryAttributesTable
  );

/**
  Get SmmProfileData.

  @param[in, out]     Size     Return Size of SmmProfileData.

  @return Address of SmmProfileData

**/
EFI_PHYSICAL_ADDRESS
GetSmmProfileData (
  IN OUT  UINT64  *Size
  );

/**
  Return if the Address is the NonMmram logging Address.

  @param[in] Address the address to be checked

  @return TRUE  The address is the NonMmram logging Address.
  @return FALSE The address is not the NonMmram logging Address.
**/
BOOLEAN
IsNonMmramLoggingAddress (
  IN UINT64  Address
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
  Build extended protection MemoryRegion.

  The caller is responsible for freeing MemoryRegion via FreePool().

  @param[out]     MemoryRegion         Returned Non-Mmram Memory regions.
  @param[out]     MemoryRegionCount    A pointer to the number of Memory regions.

**/
VOID
CreateExtendedProtectionRange (
  OUT MM_CPU_MEMORY_REGION  **MemoryRegion,
  OUT UINTN                 *MemoryRegionCount
  );

/**
  Create the Non-Mmram Memory Region.

  The caller is responsible for freeing MemoryRegion via FreePool().

  @param[in]      PhysicalAddressBits  The bits of physical address to map.
  @param[out]     MemoryRegion         Returned Non-Mmram Memory regions.
  @param[out]     MemoryRegionCount    A pointer to the number of Memory regions.

**/
VOID
CreateNonMmramMemMap (
  IN  UINT8                 PhysicalAddressBits,
  OUT MM_CPU_MEMORY_REGION  **MemoryRegion,
  OUT UINTN                 *MemoryRegionCount
  );

/**
  This function updates UEFI memory attribute according to UEFI memory map.

**/
VOID
UpdateUefiMemMapAttributes (
  VOID
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
  This function sets the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param[in]   PageTableBase    The page table base.
  @param[in]   PagingMode       The paging mode.
  @param[in]   BaseAddress      The physical address that is the start address of a memory region.
  @param[in]   Length           The size in bytes of the memory region.
  @param[in]   Attributes       The bit mask of attributes to set for the memory region.

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
SmmSetMemoryAttributesEx (
  IN  UINTN             PageTableBase,
  IN  PAGING_MODE       PagingMode,
  IN  PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64            Length,
  IN  UINT64            Attributes
  );

/**
  This function clears the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param[in]   PageTableBase    The page table base.
  @param[in]   PagingMode       The paging mode.
  @param[in]   BaseAddress      The physical address that is the start address of a memory region.
  @param[in]   Length           The size in bytes of the memory region.
  @param[in]   Attributes       The bit mask of attributes to clear for the memory region.

  @retval EFI_SUCCESS           The attributes were cleared for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of attributes that
                                cannot be cleared together.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not supported for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
SmmClearMemoryAttributesEx (
  IN  UINTN                 PageTableBase,
  IN  PAGING_MODE           PagingMode,
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  IN  UINT64                Attributes
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
  IN UINTN  Pages
  );

/**
  Allocate pages for code.

  @param[in]  Pages Number of pages to be allocated.

  @return Allocated memory.
**/
VOID *
AllocateCodePages (
  IN UINTN  Pages
  );

//
// S3 related global variable and function prototype.
//

extern BOOLEAN  mSmmS3Flag;

/**
  Initialize SMM S3 resume state structure used during S3 Resume.

**/
VOID
InitSmmS3ResumeState (
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
  Get SmmCpuSyncConfig data: RelaxedMode, SyncTimeout, SyncTimeout2.

  @param[in,out] RelaxedMode   It indicates if Relaxed CPU synchronization method or
                               traditional CPU synchronization method is used when processing an SMI.
  @param[in,out] SyncTimeout   It indicates the 1st BSP/AP synchronization timeout value in SMM.
  @param[in,out] SyncTimeout2  It indicates the 2nd BSP/AP synchronization timeout value in SMM.

 **/
VOID
GetSmmCpuSyncConfigData (
  IN OUT BOOLEAN *RelaxedMode, OPTIONAL
  IN OUT UINT64  *SyncTimeout, OPTIONAL
  IN OUT UINT64  *SyncTimeout2 OPTIONAL
  );

/**
  Get ACPI S3 enable flag.

**/
VOID
GetAcpiS3EnableFlag (
  VOID
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
  IN  UINTN                 Cr3,
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
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
  IN  EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS                 BaseAddress,
  IN  UINT64                               Length,
  IN  UINT64                               Attributes
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
  IN  EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS                 BaseAddress,
  IN  UINT64                               Length,
  IN  UINT64                               Attributes
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
  IN  EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS                 BaseAddress,
  IN  UINT64                               Length,
  IN  UINT64                               *Attributes
  );

/**
  This function fixes up the address of the global variable or function
  referred in SmiEntry assembly files to be the absolute address.
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

/**
  Schedule a procedure to run on the specified CPU.

  @param[in]       Procedure                The address of the procedure to run
  @param[in]       CpuIndex                 Target CPU Index
  @param[in,out]   ProcArguments            The parameter to pass to the procedure
  @param[in,out]   Token                    This is an optional parameter that allows the caller to execute the
                                            procedure in a blocking or non-blocking fashion. If it is NULL the
                                            call is blocking, and the call will not return until the AP has
                                            completed the procedure. If the token is not NULL, the call will
                                            return immediately. The caller can check whether the procedure has
                                            completed with CheckOnProcedure or WaitForProcedure.
  @param[in]       TimeoutInMicroseconds    Indicates the time limit in microseconds for the APs to finish
                                            execution of Procedure, either for blocking or non-blocking mode.
                                            Zero means infinity. If the timeout expires before all APs return
                                            from Procedure, then Procedure on the failed APs is terminated. If
                                            the timeout expires in blocking mode, the call returns EFI_TIMEOUT.
                                            If the timeout expires in non-blocking mode, the timeout determined
                                            can be through CheckOnProcedure or WaitForProcedure.
                                            Note that timeout support is optional. Whether an implementation
                                            supports this feature can be determined via the Attributes data
                                            member.
  @param[in,out]   CpuStatus                This optional pointer may be used to get the status code returned
                                            by Procedure when it completes execution on the target AP, or with
                                            EFI_TIMEOUT if the Procedure fails to complete within the optional
                                            timeout. The implementation will update this variable with
                                            EFI_NOT_READY prior to starting Procedure on the target AP.

  @retval EFI_INVALID_PARAMETER    CpuNumber not valid
  @retval EFI_INVALID_PARAMETER    CpuNumber specifying BSP
  @retval EFI_INVALID_PARAMETER    The AP specified by CpuNumber did not enter SMM
  @retval EFI_INVALID_PARAMETER    The AP specified by CpuNumber is busy
  @retval EFI_SUCCESS              The procedure has been successfully scheduled

**/
EFI_STATUS
InternalSmmStartupThisAp (
  IN      EFI_AP_PROCEDURE2  Procedure,
  IN      UINTN              CpuIndex,
  IN OUT  VOID               *ProcArguments OPTIONAL,
  IN OUT  MM_COMPLETION      *Token,
  IN      UINTN              TimeoutInMicroseconds,
  IN OUT  EFI_STATUS         *CpuStatus
  );

/**
  Checks whether the input token is the current used token.

  @param[in]  Token      This parameter describes the token that was passed into DispatchProcedure or
                         BroadcastProcedure.

  @retval TRUE           The input token is the current used token.
  @retval FALSE          The input token is not the current used token.
**/
BOOLEAN
IsTokenInUse (
  IN SPIN_LOCK  *Token
  );

/**
  Checks status of specified AP.

  This function checks whether the specified AP has finished the task assigned
  by StartupThisAP(), and whether timeout expires.

  @param[in]  Token             This parameter describes the token that was passed into DispatchProcedure or
                                BroadcastProcedure.

  @retval EFI_SUCCESS           Specified AP has finished task assigned by StartupThisAPs().
  @retval EFI_NOT_READY         Specified AP has not finished task and timeout has not expired.
**/
EFI_STATUS
IsApReady (
  IN SPIN_LOCK  *Token
  );

/**
  Check whether it is an present AP.

  @param   CpuIndex      The AP index which calls this function.

  @retval  TRUE           It's a present AP.
  @retval  TRUE           This is not an AP or it is not present.

**/
BOOLEAN
IsPresentAp (
  IN UINTN  CpuIndex
  );

/**
  Worker function to execute a caller provided function on all enabled APs.

  @param[in]     Procedure               A pointer to the function to be run on
                                         enabled APs of the system.
  @param[in]     TimeoutInMicroseconds   Indicates the time limit in microseconds for
                                         APs to return from Procedure, either for
                                         blocking or non-blocking mode.
  @param[in,out] ProcedureArguments      The parameter passed into Procedure for
                                         all APs.
  @param[in,out] Token                   This is an optional parameter that allows the caller to execute the
                                         procedure in a blocking or non-blocking fashion. If it is NULL the
                                         call is blocking, and the call will not return until the AP has
                                         completed the procedure. If the token is not NULL, the call will
                                         return immediately. The caller can check whether the procedure has
                                         completed with CheckOnProcedure or WaitForProcedure.
  @param[in,out] CPUStatus               This optional pointer may be used to get the status code returned
                                         by Procedure when it completes execution on the target AP, or with
                                         EFI_TIMEOUT if the Procedure fails to complete within the optional
                                         timeout. The implementation will update this variable with
                                         EFI_NOT_READY prior to starting Procedure on the target AP.

  @retval EFI_SUCCESS             In blocking mode, all APs have finished before
                                  the timeout expired.
  @retval EFI_SUCCESS             In non-blocking mode, function has been dispatched
                                  to all enabled APs.
  @retval others                  Failed to Startup all APs.

**/
EFI_STATUS
InternalSmmStartupAllAPs (
  IN       EFI_AP_PROCEDURE2  Procedure,
  IN       UINTN              TimeoutInMicroseconds,
  IN OUT   VOID               *ProcedureArguments OPTIONAL,
  IN OUT   MM_COMPLETION      *Token,
  IN OUT   EFI_STATUS         *CPUStatus
  );

/**

  Register the SMM Foundation entry point.

  @param[in]      Procedure            A pointer to the code stream to be run on the designated target AP
                                       of the system. Type EFI_AP_PROCEDURE is defined below in Volume 2
                                       with the related definitions of
                                       EFI_MP_SERVICES_PROTOCOL.StartupAllAPs.
                                       If caller may pass a value of NULL to deregister any existing
                                       startup procedure.
  @param[in,out]  ProcedureArguments   Allows the caller to pass a list of parameters to the code that is
                                       run by the AP. It is an optional common mailbox between APs and
                                       the caller to share information

  @retval EFI_SUCCESS                  The Procedure has been set successfully.
  @retval EFI_INVALID_PARAMETER        The Procedure is NULL but ProcedureArguments not NULL.

**/
EFI_STATUS
RegisterStartupProcedure (
  IN     EFI_AP_PROCEDURE  Procedure,
  IN OUT VOID              *ProcedureArguments OPTIONAL
  );

/**
  Initialize PackageBsp Info. Processor specified by mPackageFirstThreadIndex[PackageIndex]
  will do the package-scope register programming. Set default CpuIndex to (UINT32)-1, which
  means not specified yet.

**/
VOID
InitPackageFirstThreadIndexInfo (
  VOID
  );

/**
  Allocate buffer for SpinLock and Wrapper function buffer.

**/
VOID
InitializeDataForMmMp (
  VOID
  );

/**
  Return whether access to non-SMRAM is restricted.

  @retval TRUE  Access to non-SMRAM is restricted.
  @retval FALSE Access to non-SMRAM is not restricted.
**/
BOOLEAN
IsRestrictedMemoryAccess (
  VOID
  );

/**
  Choose blocking or non-blocking mode to Wait for all APs.

  @param[in]  This                  A pointer to the EDKII_SMM_CPU_RENDEZVOUS_PROTOCOL instance.
  @param[in]  BlockingMode          Blocking or non-blocking mode.

  @retval EFI_SUCCESS               All APs have arrived SMM mode except SMI disabled APs.
  @retval EFI_TIMEOUT               There are APs not in SMM mode in given timeout constraint.

**/
EFI_STATUS
EFIAPI
SmmCpuRendezvous (
  IN  EDKII_SMM_CPU_RENDEZVOUS_PROTOCOL  *This,
  IN  BOOLEAN                            BlockingMode
  );

/**
  Insure when this function returns, no AP will execute normal mode code before entering SMM, except SMI disabled APs.

**/
VOID
SmmWaitForApArrival (
  VOID
  );

/**
  Write unprotect read-only pages if Cr0.Bits.WP is 1.

  @param[out]  WriteProtect      If Cr0.Bits.WP is enabled.

**/
VOID
SmmWriteUnprotectReadOnlyPage (
  OUT BOOLEAN  *WriteProtect
  );

/**
  Write protect read-only pages.

  @param[in]  WriteProtect      If Cr0.Bits.WP should be enabled.

**/
VOID
SmmWriteProtectReadOnlyPage (
  IN  BOOLEAN  WriteProtect
  );

///
/// Define macros to encapsulate the write unprotect/protect
/// read-only pages.
/// Below pieces of logic are defined as macros and not functions
/// because "CET" feature disable & enable must be in the same
/// function to avoid shadow stack and normal SMI stack mismatch,
/// thus WRITE_UNPROTECT_RO_PAGES () must be called pair with
/// WRITE_PROTECT_RO_PAGES () in same function.
///
/// @param[in,out] Wp   A BOOLEAN variable local to the containing
///                     function, carrying write protection status from
///                     WRITE_UNPROTECT_RO_PAGES() to
///                     WRITE_PROTECT_RO_PAGES().
///
/// @param[in,out] Cet  A BOOLEAN variable local to the containing
///                     function, carrying control flow integrity
///                     enforcement status from
///                     WRITE_UNPROTECT_RO_PAGES() to
///                     WRITE_PROTECT_RO_PAGES().
///
#define WRITE_UNPROTECT_RO_PAGES(Wp, Cet) \
  do { \
    Cet = ((AsmReadCr4 () & CR4_CET_ENABLE) != 0); \
    if (Cet) { \
      DisableCet (); \
    } \
    SmmWriteUnprotectReadOnlyPage (&Wp); \
  } while (FALSE)

#define WRITE_PROTECT_RO_PAGES(Wp, Cet) \
  do { \
    SmmWriteProtectReadOnlyPage (Wp); \
    if (Cet) { \
      EnableCet (); \
    } \
  } while (FALSE)

/**
  Get the maximum number of logical processors supported by the system.

  @retval The maximum number of logical processors supported by the system
          is indicated by the return value.
**/
UINTN
GetSupportedMaxLogicalProcessorNumber (
  VOID
  );

/**
  Extract NumberOfCpus, MaxNumberOfCpus and EFI_PROCESSOR_INFORMATION for all CPU from gEfiMpServiceProtocolGuid.

  @param[out] NumberOfCpus           Pointer to NumberOfCpus.
  @param[out] MaxNumberOfCpus        Pointer to MaxNumberOfCpus.

  @retval ProcessorInfo              Pointer to EFI_PROCESSOR_INFORMATION buffer.
**/
EFI_PROCESSOR_INFORMATION *
GetMpInformationFromMpServices (
  OUT UINTN  *NumberOfCpus,
  OUT UINTN  *MaxNumberOfCpus
  );

/**
  The common Entry Point of the SMM CPU driver.

  @retval EFI_SUCCESS    The common entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
PiSmmCpuEntryCommon (
  VOID
  );

#endif
