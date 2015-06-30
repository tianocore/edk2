/** @file
  This module produces the EFI_PEI_S3_RESUME2_PPI.
  This module works with StandAloneBootScriptExecutor to S3 resume to OS.
  This module will excute the boot script saved during last boot and after that,
  control is passed to OS waking up handler.

  Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Guid/AcpiS3Context.h>
#include <Guid/BootScriptExecutorVariable.h>
#include <Guid/Performance.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/S3Resume2.h>
#include <Ppi/SmmAccess.h>
#include <Ppi/PostBootScriptTable.h>
#include <Ppi/EndOfPeiPhase.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/LocalApicLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PrintLib.h>
#include <Library/HobLib.h>
#include <Library/LockBoxLib.h>
#include <IndustryStandard/Acpi.h>

/**
  This macro aligns the address of a variable with auto storage
  duration down to CPU_STACK_ALIGNMENT.

  Since the stack grows downward, the result preserves more of the
  stack than the original address (or the same amount), not less.
**/
#define STACK_ALIGN_DOWN(Ptr) \
          ((UINTN)(Ptr) & ~(UINTN)(CPU_STACK_ALIGNMENT - 1))

#pragma pack(1)
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
// Page-Map Level-4 Offset (PML4) and
// Page-Directory-Pointer Offset (PDPE) entries 4K & 2MB
//
typedef union {
  struct {
    UINT64  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT64  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT64  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT64  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT64  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT64  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64  Reserved:1;               // Reserved
    UINT64  MustBeZero:2;             // Must Be Zero
    UINT64  Available:3;              // Available for use by system software
    UINT64  PageTableBaseAddress:40;  // Page Table Base Address
    UINT64  AvabilableHigh:11;        // Available for use by system software
    UINT64  Nx:1;                     // No Execute bit
  } Bits;
  UINT64    Uint64;
} PAGE_MAP_AND_DIRECTORY_POINTER;

//
// Page Table Entry 2MB
//
typedef union {
  struct {
    UINT64  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT64  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT64  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT64  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT64  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT64  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64  Dirty:1;                  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64  MustBe1:1;                // Must be 1 
    UINT64  Global:1;                 // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64  Available:3;              // Available for use by system software
    UINT64  PAT:1;                    //
    UINT64  MustBeZero:8;             // Must be zero;
    UINT64  PageTableBaseAddress:31;  // Page Table Base Address
    UINT64  AvabilableHigh:11;        // Available for use by system software
    UINT64  Nx:1;                     // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_ENTRY;

//
// Page Table Entry 1GB
//
typedef union {
  struct {
    UINT64  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT64  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT64  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT64  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT64  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT64  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64  Dirty:1;                  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64  MustBe1:1;                // Must be 1 
    UINT64  Global:1;                 // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64  Available:3;              // Available for use by system software
    UINT64  PAT:1;                    //
    UINT64  MustBeZero:17;            // Must be zero;
    UINT64  PageTableBaseAddress:22;  // Page Table Base Address
    UINT64  AvabilableHigh:11;        // Available for use by system software
    UINT64  Nx:1;                     // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} PAGE_TABLE_1G_ENTRY;

#pragma pack()

//
// Function prototypes
//
/**
  a ASM function to transfer control to OS.
  
  @param  S3WakingVector  The S3 waking up vector saved in ACPI Facs table
  @param  AcpiLowMemoryBase a buffer under 1M which could be used during the transfer             
**/
typedef
VOID
(EFIAPI *ASM_TRANSFER_CONTROL) (
  IN   UINT32           S3WakingVector,
  IN   UINT32           AcpiLowMemoryBase
  );

/**
  Restores the platform to its preboot configuration for an S3 resume and
  jumps to the OS waking vector.

  This function will restore the platform to its pre-boot configuration that was
  pre-stored in the boot script table and transfer control to OS waking vector.
  Upon invocation, this function is responsible for locating the following
  information before jumping to OS waking vector:
    - ACPI tables
    - boot script table
    - any other information that it needs

  The S3RestoreConfig() function then executes the pre-stored boot script table
  and transitions the platform to the pre-boot state. The boot script is recorded
  during regular boot using the EFI_S3_SAVE_STATE_PROTOCOL.Write() and
  EFI_S3_SMM_SAVE_STATE_PROTOCOL.Write() functions.  Finally, this function
  transfers control to the OS waking vector. If the OS supports only a real-mode
  waking vector, this function will switch from flat mode to real mode before
  jumping to the waking vector.  If all platform pre-boot configurations are
  successfully restored and all other necessary information is ready, this
  function will never return and instead will directly jump to the OS waking
  vector. If this function returns, it indicates that the attempt to resume
  from the ACPI S3 sleep state failed.

  @param[in] This         Pointer to this instance of the PEI_S3_RESUME_PPI

  @retval EFI_ABORTED     Execution of the S3 resume boot script table failed.
  @retval EFI_NOT_FOUND   Some necessary information that is used for the S3
                          resume boot path could not be located.

**/
EFI_STATUS
EFIAPI
S3RestoreConfig2 (
  IN EFI_PEI_S3_RESUME2_PPI  *This
  );

/**
  Set data segment selectors value including DS/ES/FS/GS/SS.

  @param[in]  SelectorValue      Segment selector value to be set.

**/
VOID
EFIAPI
AsmSetDataSelectors (
  IN UINT16   SelectorValue
  );

//
// Globals
//
EFI_PEI_S3_RESUME2_PPI      mS3ResumePpi = { S3RestoreConfig2 };

EFI_PEI_PPI_DESCRIPTOR mPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiS3Resume2PpiGuid,
  &mS3ResumePpi
};

EFI_PEI_PPI_DESCRIPTOR mPpiListPostScriptTable = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiPostScriptTablePpiGuid,
  0
};

EFI_PEI_PPI_DESCRIPTOR mPpiListEndOfPeiTable = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  0
};

//
// Global Descriptor Table (GDT)
//
GLOBAL_REMOVE_IF_UNREFERENCED IA32_GDT mGdtEntries[] = {
/* selector { Global Segment Descriptor                              } */
/* 0x00 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}},
/* 0x08 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}},
/* 0x10 */  {{0xFFFF, 0,  0,  0xB,  1,  0,  1,  0xF,  0,  0, 1,  1,  0}},
/* 0x18 */  {{0xFFFF, 0,  0,  0x3,  1,  0,  1,  0xF,  0,  0, 1,  1,  0}},
/* 0x20 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}},
/* 0x28 */  {{0xFFFF, 0,  0,  0xB,  1,  0,  1,  0xF,  0,  0, 0,  1,  0}},
/* 0x30 */  {{0xFFFF, 0,  0,  0x3,  1,  0,  1,  0xF,  0,  0, 0,  1,  0}},
/* 0x38 */  {{0xFFFF, 0,  0,  0xB,  1,  0,  1,  0xF,  0,  1, 0,  1,  0}},
/* 0x40 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}},
};

#define DATA_SEGEMENT_SELECTOR        0x18

//
// IA32 Gdt register
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST IA32_DESCRIPTOR mGdt = {
  sizeof (mGdtEntries) - 1,
  (UINTN) mGdtEntries
  };

/**
  Performance measure function to get S3 detailed performance data.

  This function will getS3 detailed performance data and saved in pre-reserved ACPI memory.
**/
VOID
WriteToOsS3PerformanceData (
  VOID
  )
{
  EFI_STATUS                                    Status;
  EFI_PHYSICAL_ADDRESS                          mAcpiLowMemoryBase;
  PERF_HEADER                                   *PerfHeader;
  PERF_DATA                                     *PerfData;
  UINT64                                        Ticker;
  UINTN                                         Index;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI               *VariableServices;
  UINTN                                         VarSize;
  UINTN                                         LogEntryKey;
  CONST VOID                                    *Handle;
  CONST CHAR8                                   *Token;
  CONST CHAR8                                   *Module;
  UINT64                                        StartTicker;
  UINT64                                        EndTicker;
  UINT64                                        StartValue;
  UINT64                                        EndValue;
  BOOLEAN                                       CountUp;
  UINT64                                        Freq;

  //
  // Retrive time stamp count as early as possilbe
  //
  Ticker = GetPerformanceCounter ();

  Freq   = GetPerformanceCounterProperties (&StartValue, &EndValue);

  Freq   = DivU64x32 (Freq, 1000);

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **) &VariableServices
             );
  if (EFI_ERROR (Status)) {
    return;
  }

  VarSize   = sizeof (EFI_PHYSICAL_ADDRESS);
  Status = VariableServices->GetVariable (
                               VariableServices,
                               L"PerfDataMemAddr",
                               &gPerformanceProtocolGuid,
                               NULL,
                               &VarSize,
                               &mAcpiLowMemoryBase
                               );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Fail to retrieve variable to log S3 performance data \n"));
    return;
  }

  PerfHeader = (PERF_HEADER *) (UINTN) mAcpiLowMemoryBase;

  if (PerfHeader->Signiture != PERFORMANCE_SIGNATURE) {
    DEBUG ((EFI_D_ERROR, "Performance data in ACPI memory get corrupted! \n"));
    return;
  }

  //
  // Record total S3 resume time.
  //
  if (EndValue >= StartValue) {
    PerfHeader->S3Resume = Ticker - StartValue;
    CountUp              = TRUE;
  } else {
    PerfHeader->S3Resume = StartValue - Ticker;
    CountUp              = FALSE;
  }

  //
  // Get S3 detailed performance data
  //
  Index = 0;
  LogEntryKey = 0;
  while ((LogEntryKey = GetPerformanceMeasurement (
                          LogEntryKey,
                          &Handle,
                          &Token,
                          &Module,
                          &StartTicker,
                          &EndTicker)) != 0) {
    if (EndTicker != 0) {
      PerfData = &PerfHeader->S3Entry[Index];

      //
      // Use File Handle to specify the different performance log for PEIM.
      // File Handle is the base address of PEIM FFS file.
      //
      if ((AsciiStrnCmp (Token, "PEIM", PEI_PERFORMANCE_STRING_SIZE) == 0) && (Handle != NULL)) {
        AsciiSPrint (PerfData->Token, PERF_TOKEN_LENGTH, "0x%11p", Handle);
      } else {
        AsciiStrCpyS (PerfData->Token, PERF_TOKEN_SIZE, Token);
      }
      if (StartTicker == 1) {
        StartTicker = StartValue;
      }
      if (EndTicker == 1) {
        EndTicker = StartValue;
      }
      Ticker = CountUp? (EndTicker - StartTicker) : (StartTicker - EndTicker);
      PerfData->Duration = (UINT32) DivU64x32 (Ticker, (UINT32) Freq);

      //
      // Only Record > 1ms performance data so that more big performance can be recorded.
      //
      if ((Ticker > Freq) && (++Index >= PERF_PEI_ENTRY_MAX_NUM)) {
        //
        // Reach the maximum number of PEI performance log entries.
        //
        break;
      }
    }
  }
  PerfHeader->S3EntryNum = (UINT32) Index;
}

/**
  The function will check if current waking vector is long mode.

  @param  AcpiS3Context                 a pointer to a structure of ACPI_S3_CONTEXT

  @retval TRUE   Current context need long mode waking vector.
  @retval FALSE  Current context need not long mode waking vector.
**/
BOOLEAN
IsLongModeWakingVector (
  IN ACPI_S3_CONTEXT                *AcpiS3Context
  )
{
  EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs;

  Facs = (EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *) ((UINTN) (AcpiS3Context->AcpiFacsTable));
  if ((Facs == NULL) ||
      (Facs->Signature != EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE) ||
      ((Facs->FirmwareWakingVector == 0) && (Facs->XFirmwareWakingVector == 0)) ) {
    // Something wrong with FACS
    return FALSE;
  }
  if (Facs->XFirmwareWakingVector != 0) {
    if ((Facs->Version == EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION) &&
        ((Facs->Flags & EFI_ACPI_4_0_64BIT_WAKE_SUPPORTED_F) != 0) &&
        ((Facs->Flags & EFI_ACPI_4_0_OSPM_64BIT_WAKE__F) != 0)) {
      // Both BIOS and OS wants 64bit vector
      if (FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

/**
  Jump to OS waking vector.
  The function will install boot script done PPI, report S3 resume status code, and then jump to OS waking vector.

  @param  AcpiS3Context                 a pointer to a structure of ACPI_S3_CONTEXT
  @param  PeiS3ResumeState              a pointer to a structure of PEI_S3_RESUME_STATE
**/
VOID
EFIAPI
S3ResumeBootOs (
  IN ACPI_S3_CONTEXT                *AcpiS3Context,
  IN PEI_S3_RESUME_STATE            *PeiS3ResumeState
  )
{
  EFI_STATUS                                    Status;
  EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs;
  ASM_TRANSFER_CONTROL                          AsmTransferControl;
  UINTN                                         TempStackTop;
  UINTN                                         TempStack[0x10];

  //
  // Restore IDT
  //
  AsmWriteIdtr (&PeiS3ResumeState->Idtr);

  if (PeiS3ResumeState->ReturnStatus != EFI_SUCCESS) {
    //
    // Report Status code that boot script execution is failed
    //
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_BOOT_SCRIPT_ERROR)
      );
  }

  //
  // NOTE: Because Debug Timer interrupt and system interrupts will be disabled 
  // in BootScriptExecuteDxe, the rest code in S3ResumeBootOs() cannot be halted
  // by soft debugger.
  //

  PERF_END (NULL, "ScriptExec", NULL, 0);

  //
  // Install BootScriptDonePpi
  //
  Status = PeiServicesInstallPpi (&mPpiListPostScriptTable);
  ASSERT_EFI_ERROR (Status);

  //
  // Get ACPI Table Address
  //
  Facs = (EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *) ((UINTN) (AcpiS3Context->AcpiFacsTable));

  if ((Facs == NULL) ||
      (Facs->Signature != EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE) ||
      ((Facs->FirmwareWakingVector == 0) && (Facs->XFirmwareWakingVector == 0)) ) {
    //
    // Report Status code that no valid vector is found
    //
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MAJOR,
      (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_OS_WAKE_ERROR)
      );
    CpuDeadLoop ();
    return ;
  }

  //
  // Install EndOfPeiPpi
  //
  Status = PeiServicesInstallPpi (&mPpiListEndOfPeiTable);
  ASSERT_EFI_ERROR (Status);

  //
  // report status code on S3 resume
  //
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_OS_WAKE);

  PERF_CODE (
    WriteToOsS3PerformanceData ();
    );

  AsmTransferControl = (ASM_TRANSFER_CONTROL)(UINTN)PeiS3ResumeState->AsmTransferControl;
  if (Facs->XFirmwareWakingVector != 0) {
    //
    // Switch to native waking vector
    //
    TempStackTop = (UINTN)&TempStack + sizeof(TempStack);
    if ((Facs->Version == EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION) &&
        ((Facs->Flags & EFI_ACPI_4_0_64BIT_WAKE_SUPPORTED_F) != 0) &&
        ((Facs->Flags & EFI_ACPI_4_0_OSPM_64BIT_WAKE__F) != 0)) {
      //
      // X64 long mode waking vector
      //
      DEBUG (( EFI_D_ERROR, "Transfer to 64bit OS waking vector - %x\r\n", (UINTN)Facs->XFirmwareWakingVector));
      if (FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
        AsmEnablePaging64 (
          0x38,
          Facs->XFirmwareWakingVector,
          0,
          0,
          (UINT64)(UINTN)TempStackTop
          );
      } else {
        //
        // Report Status code that no valid waking vector is found
        //
        REPORT_STATUS_CODE (
          EFI_ERROR_CODE | EFI_ERROR_MAJOR,
          (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_OS_WAKE_ERROR)
          );
        DEBUG (( EFI_D_ERROR, "Unsupported for 32bit DXE transfer to 64bit OS waking vector!\r\n"));
        ASSERT (FALSE);
        CpuDeadLoop ();
        return ;
      }
    } else {
      //
      // IA32 protected mode waking vector (Page disabled)
      //
      DEBUG (( EFI_D_ERROR, "Transfer to 32bit OS waking vector - %x\r\n", (UINTN)Facs->XFirmwareWakingVector));
      SwitchStack (
        (SWITCH_STACK_ENTRY_POINT) (UINTN) Facs->XFirmwareWakingVector,
        NULL,
        NULL,
        (VOID *)(UINTN)TempStackTop
        );
    }
  } else {
    //
    // 16bit Realmode waking vector
    //
    DEBUG (( EFI_D_ERROR, "Transfer to 16bit OS waking vector - %x\r\n", (UINTN)Facs->FirmwareWakingVector));
    AsmTransferControl (Facs->FirmwareWakingVector, 0x0);
  }

  //
  // Report Status code the failure of S3Resume
  //
  REPORT_STATUS_CODE (
    EFI_ERROR_CODE | EFI_ERROR_MAJOR,
    (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_OS_WAKE_ERROR)
    );

  //
  // Never run to here
  //
  CpuDeadLoop();
}

/**
  Restore S3 page table because we do not trust ACPINvs content.
  If BootScriptExector driver will not run in 64-bit mode, this function will do nothing. 

  @param S3NvsPageTableAddress   PageTableAddress in ACPINvs
  @param Build4GPageTableOnly    If BIOS just build 4G page table only
**/
VOID
RestoreS3PageTables (
  IN UINTN                                         S3NvsPageTableAddress,
  IN BOOLEAN                                       Build4GPageTableOnly
  )
{
  if (FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
    UINT32                                        RegEax;
    UINT32                                        RegEdx;
    UINT8                                         PhysicalAddressBits;
    EFI_PHYSICAL_ADDRESS                          PageAddress;
    UINTN                                         IndexOfPml4Entries;
    UINTN                                         IndexOfPdpEntries;
    UINTN                                         IndexOfPageDirectoryEntries;
    UINT32                                        NumberOfPml4EntriesNeeded;
    UINT32                                        NumberOfPdpEntriesNeeded;
    PAGE_MAP_AND_DIRECTORY_POINTER                *PageMapLevel4Entry;
    PAGE_MAP_AND_DIRECTORY_POINTER                *PageMap;
    PAGE_MAP_AND_DIRECTORY_POINTER                *PageDirectoryPointerEntry;
    PAGE_TABLE_ENTRY                              *PageDirectoryEntry;
    VOID                                          *Hob;
    BOOLEAN                                       Page1GSupport;
    PAGE_TABLE_1G_ENTRY                           *PageDirectory1GEntry;

    //
    // NOTE: We have to ASSUME the page table generation format, because we do not know whole page table information.
    // The whole page table is too large to be saved in SMRAM.
    //
    // The assumption is : whole page table is allocated in CONTINOUS memory and CR3 points to TOP page.
    //
    DEBUG ((EFI_D_ERROR, "S3NvsPageTableAddress - %x (%x)\n", (UINTN)S3NvsPageTableAddress, (UINTN)Build4GPageTableOnly));

    //
    // By architecture only one PageMapLevel4 exists - so lets allocate storgage for it.
    //
    PageMap = (PAGE_MAP_AND_DIRECTORY_POINTER *)S3NvsPageTableAddress;
    S3NvsPageTableAddress += SIZE_4KB;
    
    Page1GSupport = FALSE;
    if (PcdGetBool(PcdUse1GPageTable)) {
      AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
      if (RegEax >= 0x80000001) {
        AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
        if ((RegEdx & BIT26) != 0) {
          Page1GSupport = TRUE;
        }
      }
    }
    
    //
    // Get physical address bits supported.
    //
    Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
    if (Hob != NULL) {
      PhysicalAddressBits = ((EFI_HOB_CPU *) Hob)->SizeOfMemorySpace;
    } else {
      AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
      if (RegEax >= 0x80000008) {
        AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
        PhysicalAddressBits = (UINT8) RegEax;
      } else {
        PhysicalAddressBits = 36;
      }
    }
    
    //
    // IA-32e paging translates 48-bit linear addresses to 52-bit physical addresses.
    //
    ASSERT (PhysicalAddressBits <= 52);
    if (PhysicalAddressBits > 48) {
      PhysicalAddressBits = 48;
    }

    //
    // NOTE: In order to save time to create full page table, we just create 4G page table by default.
    // And let PF handler in BootScript driver to create more on request.
    //
    if (Build4GPageTableOnly) {
      PhysicalAddressBits = 32;
      ZeroMem (PageMap, EFI_PAGES_TO_SIZE(2));
    }
    //
    // Calculate the table entries needed.
    //
    if (PhysicalAddressBits <= 39) {
      NumberOfPml4EntriesNeeded = 1;
      NumberOfPdpEntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 30));
    } else {
      NumberOfPml4EntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 39));
      NumberOfPdpEntriesNeeded = 512;
    }
    
    PageMapLevel4Entry = PageMap;
    PageAddress        = 0;
    for (IndexOfPml4Entries = 0; IndexOfPml4Entries < NumberOfPml4EntriesNeeded; IndexOfPml4Entries++, PageMapLevel4Entry++) {
      //
      // Each PML4 entry points to a page of Page Directory Pointer entires.
      // So lets allocate space for them and fill them in in the IndexOfPdpEntries loop.
      //
      PageDirectoryPointerEntry = (PAGE_MAP_AND_DIRECTORY_POINTER *)S3NvsPageTableAddress;
      S3NvsPageTableAddress += SIZE_4KB;
    
      //
      // Make a PML4 Entry
      //
      PageMapLevel4Entry->Uint64 = (UINT64)(UINTN)PageDirectoryPointerEntry;
      PageMapLevel4Entry->Bits.ReadWrite = 1;
      PageMapLevel4Entry->Bits.Present = 1;

      if (Page1GSupport) {
        PageDirectory1GEntry = (VOID *) PageDirectoryPointerEntry;
    
        for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectory1GEntry++, PageAddress += SIZE_1GB) {
          //
          // Fill in the Page Directory entries
          //
          PageDirectory1GEntry->Uint64 = (UINT64)PageAddress;
          PageDirectory1GEntry->Bits.ReadWrite = 1;
          PageDirectory1GEntry->Bits.Present = 1;
          PageDirectory1GEntry->Bits.MustBe1 = 1;
        }
      } else {
        for (IndexOfPdpEntries = 0; IndexOfPdpEntries < NumberOfPdpEntriesNeeded; IndexOfPdpEntries++, PageDirectoryPointerEntry++) {
          //
          // Each Directory Pointer entries points to a page of Page Directory entires.
          // So allocate space for them and fill them in in the IndexOfPageDirectoryEntries loop.
          //       
          PageDirectoryEntry = (PAGE_TABLE_ENTRY *)S3NvsPageTableAddress;
          S3NvsPageTableAddress += SIZE_4KB;
    
          //
          // Fill in a Page Directory Pointer Entries
          //
          PageDirectoryPointerEntry->Uint64 = (UINT64)(UINTN)PageDirectoryEntry;
          PageDirectoryPointerEntry->Bits.ReadWrite = 1;
          PageDirectoryPointerEntry->Bits.Present = 1;
    
          for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectoryEntry++, PageAddress += SIZE_2MB) {
            //
            // Fill in the Page Directory entries
            //
            PageDirectoryEntry->Uint64 = (UINT64)PageAddress;
            PageDirectoryEntry->Bits.ReadWrite = 1;
            PageDirectoryEntry->Bits.Present = 1;
            PageDirectoryEntry->Bits.MustBe1 = 1;
          }
        }
      }
    }
    return ;
  } else {
  	//
  	// If DXE is running 32-bit mode, no need to establish page table.
  	//
    return ;
  }
}

/**
  Jump to boot script executor driver.

  The function will close and lock SMRAM and then jump to boot script execute driver to executing S3 boot script table.

  @param  AcpiS3Context                 a pointer to a structure of ACPI_S3_CONTEXT
  @param  EfiBootScriptExecutorVariable The function entry to executing S3 boot Script table. This function is build in
                                        boot script execute driver
**/
VOID
EFIAPI
S3ResumeExecuteBootScript (
  IN ACPI_S3_CONTEXT                *AcpiS3Context,
  IN BOOT_SCRIPT_EXECUTOR_VARIABLE  *EfiBootScriptExecutorVariable
  )
{
  EFI_STATUS                 Status;
  PEI_SMM_ACCESS_PPI         *SmmAccess;
  UINTN                      Index;
  VOID                       *GuidHob;
  IA32_DESCRIPTOR            *IdtDescriptor;
  VOID                       *IdtBuffer;
  PEI_S3_RESUME_STATE        *PeiS3ResumeState;
  BOOLEAN                    InterruptStatus;

  DEBUG ((EFI_D_ERROR, "S3ResumeExecuteBootScript()\n"));

  //
  // Attempt to use content from SMRAM first
  //
  GuidHob = GetFirstGuidHob (&gEfiAcpiVariableGuid);
  if (GuidHob != NULL) {
    //
    // Last step for SMM - send SMI for initialization
    //

    //
    // Send SMI to APs
    //    
    SendSmiIpiAllExcludingSelf ();
    //
    // Send SMI to BSP
    //
    SendSmiIpi (GetApicId ());

    Status = PeiServicesLocatePpi (
                              &gPeiSmmAccessPpiGuid,
                              0,
                              NULL,
                              (VOID **) &SmmAccess
                              );
    if (!EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Close all SMRAM regions before executing boot script\n"));
  
      for (Index = 0, Status = EFI_SUCCESS; !EFI_ERROR (Status); Index++) {
        Status = SmmAccess->Close ((EFI_PEI_SERVICES **)GetPeiServicesTablePointer (), SmmAccess, Index);
      }

      DEBUG ((EFI_D_ERROR, "Lock all SMRAM regions before executing boot script\n"));
  
      for (Index = 0, Status = EFI_SUCCESS; !EFI_ERROR (Status); Index++) {
        Status = SmmAccess->Lock ((EFI_PEI_SERVICES **)GetPeiServicesTablePointer (), SmmAccess, Index);
      }
    }
  }

  if (FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
    AsmWriteCr3 ((UINTN)AcpiS3Context->S3NvsPageTableAddress);
  }

  if (FeaturePcdGet (PcdFrameworkCompatibilitySupport)) {
    //
    // On some platform, such as ECP, a dispatch node in boot script table may execute a 32-bit PEIM which may need PeiServices
    // pointer. So PeiServices need preserve in (IDTBase- sizeof (UINTN)). 
    //
    IdtDescriptor = (IA32_DESCRIPTOR *) (UINTN) (AcpiS3Context->IdtrProfile);
    //
    // Make sure the newly allcated IDT align with 16-bytes
    // 
    IdtBuffer = AllocatePages (EFI_SIZE_TO_PAGES((IdtDescriptor->Limit + 1) + 16));
    ASSERT (IdtBuffer != NULL);
    //
    // Additional 16 bytes allocated to save IA32 IDT descriptor and Pei Service Table Pointer
    // IA32 IDT descriptor will be used to setup IA32 IDT table for 32-bit Framework Boot Script code
    // 
    ZeroMem (IdtBuffer, 16);
    AsmReadIdtr ((IA32_DESCRIPTOR *)IdtBuffer);
    CopyMem ((VOID*)((UINT8*)IdtBuffer + 16),(VOID*)(IdtDescriptor->Base), (IdtDescriptor->Limit + 1));
    IdtDescriptor->Base = (UINTN)((UINT8*)IdtBuffer + 16);
    *(UINTN*)(IdtDescriptor->Base - sizeof(UINTN)) = (UINTN)GetPeiServicesTablePointer ();
  }

  InterruptStatus = SaveAndDisableInterrupts ();
  //
  // Need to make sure the GDT is loaded with values that support long mode and real mode.
  //
  AsmWriteGdtr (&mGdt);
  //
  // update segment selectors per the new GDT.
  //
  AsmSetDataSelectors (DATA_SEGEMENT_SELECTOR);
  //
  // Restore interrupt state.
  //
  SetInterruptState (InterruptStatus);

  //
  // Prepare data for return back
  //
  PeiS3ResumeState = AllocatePool (sizeof(*PeiS3ResumeState));
  ASSERT (PeiS3ResumeState != NULL);
  DEBUG (( EFI_D_ERROR, "PeiS3ResumeState - %x\r\n", PeiS3ResumeState));
  PeiS3ResumeState->ReturnCs           = 0x10;
  PeiS3ResumeState->ReturnEntryPoint   = (EFI_PHYSICAL_ADDRESS)(UINTN)S3ResumeBootOs;
  PeiS3ResumeState->ReturnStackPointer = (EFI_PHYSICAL_ADDRESS)STACK_ALIGN_DOWN (&Status);
  //
  // Save IDT
  //
  AsmReadIdtr (&PeiS3ResumeState->Idtr);
  
  //
  // Report Status Code to indicate S3 boot script execution
  //
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_S3_BOOT_SCRIPT);

  PERF_START (NULL, "ScriptExec", NULL, 0);

  if (FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
    //
    // X64 S3 Resume
    //
    DEBUG (( EFI_D_ERROR, "Enable X64 and transfer control to Standalone Boot Script Executor\r\n"));

    //
    // Switch to long mode to complete resume.
    //
    AsmEnablePaging64 (
      0x38,
      EfiBootScriptExecutorVariable->BootScriptExecutorEntrypoint,
      (UINT64)(UINTN)AcpiS3Context,
      (UINT64)(UINTN)PeiS3ResumeState,
      (UINT64)(UINTN)(AcpiS3Context->BootScriptStackBase + AcpiS3Context->BootScriptStackSize)
      );
  } else {
    //
    // IA32 S3 Resume
    //
    DEBUG (( EFI_D_ERROR, "transfer control to Standalone Boot Script Executor\r\n"));
    SwitchStack (
      (SWITCH_STACK_ENTRY_POINT) (UINTN) EfiBootScriptExecutorVariable->BootScriptExecutorEntrypoint,
      (VOID *)AcpiS3Context,
      (VOID *)PeiS3ResumeState,
      (VOID *)(UINTN)(AcpiS3Context->BootScriptStackBase + AcpiS3Context->BootScriptStackSize)
      );
  }

  //
  // Never run to here
  //
  CpuDeadLoop();
}
/**
  Restores the platform to its preboot configuration for an S3 resume and
  jumps to the OS waking vector.

  This function will restore the platform to its pre-boot configuration that was
  pre-stored in the boot script table and transfer control to OS waking vector.
  Upon invocation, this function is responsible for locating the following
  information before jumping to OS waking vector:
    - ACPI tables
    - boot script table
    - any other information that it needs

  The S3RestoreConfig() function then executes the pre-stored boot script table
  and transitions the platform to the pre-boot state. The boot script is recorded
  during regular boot using the EFI_S3_SAVE_STATE_PROTOCOL.Write() and
  EFI_S3_SMM_SAVE_STATE_PROTOCOL.Write() functions.  Finally, this function
  transfers control to the OS waking vector. If the OS supports only a real-mode
  waking vector, this function will switch from flat mode to real mode before
  jumping to the waking vector.  If all platform pre-boot configurations are
  successfully restored and all other necessary information is ready, this
  function will never return and instead will directly jump to the OS waking
  vector. If this function returns, it indicates that the attempt to resume
  from the ACPI S3 sleep state failed.

  @param[in] This         Pointer to this instance of the PEI_S3_RESUME_PPI

  @retval EFI_ABORTED     Execution of the S3 resume boot script table failed.
  @retval EFI_NOT_FOUND   Some necessary information that is used for the S3
                          resume boot path could not be located.

**/
EFI_STATUS
EFIAPI
S3RestoreConfig2 (
  IN EFI_PEI_S3_RESUME2_PPI  *This
  )
{
  EFI_STATUS                                    Status;
  PEI_SMM_ACCESS_PPI                            *SmmAccess;
  UINTN                                         Index;
  ACPI_S3_CONTEXT                               *AcpiS3Context;
  EFI_PHYSICAL_ADDRESS                          TempEfiBootScriptExecutorVariable;
  EFI_PHYSICAL_ADDRESS                          TempAcpiS3Context;
  BOOT_SCRIPT_EXECUTOR_VARIABLE                 *EfiBootScriptExecutorVariable;
  UINTN                                         VarSize;
  EFI_SMRAM_DESCRIPTOR                          *SmramDescriptor;
  SMM_S3_RESUME_STATE                           *SmmS3ResumeState;
  VOID                                          *GuidHob;
  BOOLEAN                                       Build4GPageTableOnly;
  BOOLEAN                                       InterruptStatus;

  TempAcpiS3Context = 0;
  TempEfiBootScriptExecutorVariable = 0;

  DEBUG ((EFI_D_ERROR, "Enter S3 PEIM\r\n"));

  VarSize = sizeof (EFI_PHYSICAL_ADDRESS);
  Status = RestoreLockBox (
             &gEfiAcpiVariableGuid,
             &TempAcpiS3Context,
             &VarSize
             );
  ASSERT_EFI_ERROR (Status);

  Status = RestoreLockBox (
             &gEfiAcpiS3ContextGuid,
             NULL,
             NULL
             );
  ASSERT_EFI_ERROR (Status);

  AcpiS3Context = (ACPI_S3_CONTEXT *)(UINTN)TempAcpiS3Context;
  ASSERT (AcpiS3Context != NULL);

  VarSize   = sizeof (EFI_PHYSICAL_ADDRESS);
  Status = RestoreLockBox (
             &gEfiBootScriptExecutorVariableGuid,
             &TempEfiBootScriptExecutorVariable,
             &VarSize
             );
  ASSERT_EFI_ERROR (Status);

  Status = RestoreLockBox (
             &gEfiBootScriptExecutorContextGuid,
             NULL,
             NULL
             );
  ASSERT_EFI_ERROR (Status);

  EfiBootScriptExecutorVariable = (BOOT_SCRIPT_EXECUTOR_VARIABLE *) (UINTN) TempEfiBootScriptExecutorVariable;
  ASSERT (EfiBootScriptExecutorVariable != NULL);

  DEBUG (( EFI_D_ERROR, "AcpiS3Context = %x\n", AcpiS3Context));
  DEBUG (( EFI_D_ERROR, "Waking Vector = %x\n", ((EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *) ((UINTN) (AcpiS3Context->AcpiFacsTable)))->FirmwareWakingVector));
  DEBUG (( EFI_D_ERROR, "AcpiS3Context->AcpiFacsTable = %x\n", AcpiS3Context->AcpiFacsTable));
  DEBUG (( EFI_D_ERROR, "AcpiS3Context->IdtrProfile = %x\n", AcpiS3Context->IdtrProfile));  
  DEBUG (( EFI_D_ERROR, "AcpiS3Context->S3NvsPageTableAddress = %x\n", AcpiS3Context->S3NvsPageTableAddress));
  DEBUG (( EFI_D_ERROR, "AcpiS3Context->S3DebugBufferAddress = %x\n", AcpiS3Context->S3DebugBufferAddress));
  DEBUG (( EFI_D_ERROR, "AcpiS3Context->BootScriptStackBase = %x\n", AcpiS3Context->BootScriptStackBase));
  DEBUG (( EFI_D_ERROR, "AcpiS3Context->BootScriptStackSize = %x\n", AcpiS3Context->BootScriptStackSize));
  DEBUG (( EFI_D_ERROR, "EfiBootScriptExecutorVariable->BootScriptExecutorEntrypoint = %x\n", EfiBootScriptExecutorVariable->BootScriptExecutorEntrypoint));

  //
  // Additional step for BootScript integrity - we only handle BootScript and BootScriptExecutor.
  // Script dispatch image and context (parameter) are handled by platform.
  // We just use restore all lock box in place, no need restore one by one.
  //
  Status = RestoreAllLockBoxInPlace ();
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    // Something wrong
    CpuDeadLoop ();
  }

  if (FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
    //
    // Need reconstruct page table here, since we do not trust ACPINvs.
    //
    if (IsLongModeWakingVector (AcpiS3Context)) {
      Build4GPageTableOnly = FALSE;
    } else {
      Build4GPageTableOnly = TRUE;
    }
    RestoreS3PageTables ((UINTN)AcpiS3Context->S3NvsPageTableAddress, Build4GPageTableOnly);
  }

  //
  // Attempt to use content from SMRAM first
  //
  GuidHob = GetFirstGuidHob (&gEfiAcpiVariableGuid);
  if (GuidHob != NULL) {
    Status = PeiServicesLocatePpi (
                              &gPeiSmmAccessPpiGuid,
                              0,
                              NULL,
                              (VOID **) &SmmAccess
                              );
    for (Index = 0; !EFI_ERROR (Status); Index++) {
      Status = SmmAccess->Open ((EFI_PEI_SERVICES **)GetPeiServicesTablePointer (), SmmAccess, Index);
    }

    SmramDescriptor = (EFI_SMRAM_DESCRIPTOR *) GET_GUID_HOB_DATA (GuidHob);
    SmmS3ResumeState = (SMM_S3_RESUME_STATE *)(UINTN)SmramDescriptor->CpuStart;

    SmmS3ResumeState->ReturnCs           = AsmReadCs ();
    SmmS3ResumeState->ReturnEntryPoint   = (EFI_PHYSICAL_ADDRESS)(UINTN)S3ResumeExecuteBootScript;
    SmmS3ResumeState->ReturnContext1     = (EFI_PHYSICAL_ADDRESS)(UINTN)AcpiS3Context;
    SmmS3ResumeState->ReturnContext2     = (EFI_PHYSICAL_ADDRESS)(UINTN)EfiBootScriptExecutorVariable;
    SmmS3ResumeState->ReturnStackPointer = (EFI_PHYSICAL_ADDRESS)STACK_ALIGN_DOWN (&Status);

    DEBUG (( EFI_D_ERROR, "SMM S3 Signature                = %x\n", SmmS3ResumeState->Signature));
    DEBUG (( EFI_D_ERROR, "SMM S3 Stack Base               = %x\n", SmmS3ResumeState->SmmS3StackBase));
    DEBUG (( EFI_D_ERROR, "SMM S3 Stack Size               = %x\n", SmmS3ResumeState->SmmS3StackSize));
    DEBUG (( EFI_D_ERROR, "SMM S3 Resume Entry Point       = %x\n", SmmS3ResumeState->SmmS3ResumeEntryPoint));
    DEBUG (( EFI_D_ERROR, "SMM S3 CR0                      = %x\n", SmmS3ResumeState->SmmS3Cr0));
    DEBUG (( EFI_D_ERROR, "SMM S3 CR3                      = %x\n", SmmS3ResumeState->SmmS3Cr3));
    DEBUG (( EFI_D_ERROR, "SMM S3 CR4                      = %x\n", SmmS3ResumeState->SmmS3Cr4));
    DEBUG (( EFI_D_ERROR, "SMM S3 Return CS                = %x\n", SmmS3ResumeState->ReturnCs));
    DEBUG (( EFI_D_ERROR, "SMM S3 Return Entry Point       = %x\n", SmmS3ResumeState->ReturnEntryPoint));
    DEBUG (( EFI_D_ERROR, "SMM S3 Return Context1          = %x\n", SmmS3ResumeState->ReturnContext1));
    DEBUG (( EFI_D_ERROR, "SMM S3 Return Context2          = %x\n", SmmS3ResumeState->ReturnContext2));
    DEBUG (( EFI_D_ERROR, "SMM S3 Return Stack Pointer     = %x\n", SmmS3ResumeState->ReturnStackPointer));
    DEBUG (( EFI_D_ERROR, "SMM S3 Smst                     = %x\n", SmmS3ResumeState->Smst));

    if (SmmS3ResumeState->Signature == SMM_S3_RESUME_SMM_32) {
      SwitchStack (
        (SWITCH_STACK_ENTRY_POINT)(UINTN)SmmS3ResumeState->SmmS3ResumeEntryPoint,
        (VOID *)AcpiS3Context,
        0,
        (VOID *)(UINTN)(SmmS3ResumeState->SmmS3StackBase + SmmS3ResumeState->SmmS3StackSize)
        );
    }
    if (SmmS3ResumeState->Signature == SMM_S3_RESUME_SMM_64) {
      //
      // Switch to long mode to complete resume.
      //

      InterruptStatus = SaveAndDisableInterrupts ();
      //
      // Need to make sure the GDT is loaded with values that support long mode and real mode.
      //
      AsmWriteGdtr (&mGdt);
      //
      // update segment selectors per the new GDT.
      //      
      AsmSetDataSelectors (DATA_SEGEMENT_SELECTOR);
      //
      // Restore interrupt state.
      //
      SetInterruptState (InterruptStatus);

      AsmWriteCr3 ((UINTN)SmmS3ResumeState->SmmS3Cr3);

      //
      // Disable interrupt of Debug timer, since IDT table cannot work in long mode.
      // NOTE: On x64 platforms, because DisablePaging64() will disable interrupts,
      // the code in S3ResumeExecuteBootScript() cannot be halted by soft debugger.
      //
      SaveAndSetDebugTimerInterrupt (FALSE);

      AsmEnablePaging64 (
        0x38,
        SmmS3ResumeState->SmmS3ResumeEntryPoint,
        (UINT64)(UINTN)AcpiS3Context,
        0,
        SmmS3ResumeState->SmmS3StackBase + SmmS3ResumeState->SmmS3StackSize
        );
    }

  }

  S3ResumeExecuteBootScript (AcpiS3Context, EfiBootScriptExecutorVariable );
  return EFI_SUCCESS;
}
/**
  Main entry for S3 Resume PEIM.

  This routine is to install EFI_PEI_S3_RESUME2_PPI.
  
  @param  FileHandle              Handle of the file being invoked.
  @param  PeiServices             Pointer to PEI Services table.

  @retval EFI_SUCCESS S3Resume Ppi is installed successfully.

**/
EFI_STATUS
EFIAPI
PeimS3ResumeEntryPoint (
  IN EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  EFI_STATUS  Status;

  //
  // Install S3 Resume Ppi
  //
  Status = (**PeiServices).InstallPpi (PeiServices, &mPpiList);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

