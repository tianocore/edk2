/** @file
  This module updates S3 Resume Performance Record in ACPI Firmware Performance
  Data Table in S3 resume boot mode.

  This module register report status code listener to collect performance data
  for S3 Resume Performance Record on S3 resume boot path.

  Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Ppi/ReportStatusCodeHandler.h>
#include <Ppi/ReadOnlyVariable2.h>

#include <Guid/FirmwarePerformance.h>
#include <Guid/Performance.h>
#include <Guid/ExtendedFirmwarePerformance.h>

#include <Library/PeiServicesLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/LockBoxLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>

/**
  Report status code listener for PEI. This is used to record the performance
  data for S3 FullResume in FPDT.

  @param[in]  PeiServices         An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in]  CodeType            Indicates the type of status code being reported.
  @param[in]  Value               Describes the current status of a hardware or software entity.
                                  This included information about the class and subclass that is used to
                                  classify the entity as well as an operation.
  @param[in]  Instance            The enumeration of a hardware or software entity within
                                  the system. Valid instance numbers start with 1.
  @param[in]  CallerId            This optional parameter may be used to identify the caller.
                                  This parameter allows the status code driver to apply different rules to
                                  different callers.
  @param[in]  Data                This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS             Status code is what we expected.
  @retval EFI_UNSUPPORTED         Status code not supported.

**/
EFI_STATUS
EFIAPI
FpdtStatusCodeListenerPei (
  IN CONST  EFI_PEI_SERVICES        **PeiServices,
  IN        EFI_STATUS_CODE_TYPE    CodeType,
  IN        EFI_STATUS_CODE_VALUE   Value,
  IN        UINT32                  Instance,
  IN CONST  EFI_GUID                *CallerId,
  IN CONST  EFI_STATUS_CODE_DATA    *Data
  )
{
  EFI_STATUS                           Status;
  UINT64                               CurrentTime;
  UINTN                                VarSize;
  EFI_PHYSICAL_ADDRESS                 S3PerformanceTablePointer;
  S3_PERFORMANCE_TABLE                 *AcpiS3PerformanceTable;
  EFI_ACPI_5_0_FPDT_S3_RESUME_RECORD   *AcpiS3ResumeRecord;
  UINT64                               S3ResumeTotal;
  EFI_ACPI_5_0_FPDT_S3_SUSPEND_RECORD  S3SuspendRecord;
  EFI_ACPI_5_0_FPDT_S3_SUSPEND_RECORD  *AcpiS3SuspendRecord;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI      *VariableServices;
  UINT8                                *BootPerformanceTable;
  FIRMWARE_PERFORMANCE_VARIABLE        PerformanceVariable;
  EFI_HOB_GUID_TYPE                    *GuidHob;
  FPDT_PEI_EXT_PERF_HEADER             *PeiPerformanceLogHeader;
  UINT8                                *FirmwarePerformanceData;
  UINT8                                *FirmwarePerformanceTablePtr;

  //
  // Check whether status code is what we are interested in.
  //
  if (((CodeType & EFI_STATUS_CODE_TYPE_MASK) != EFI_PROGRESS_CODE) ||
      (Value != (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_PC_OS_WAKE))) {
    return EFI_UNSUPPORTED;
  }

  //
  // Retrieve current time as early as possible.
  //
  CurrentTime = GetTimeInNanoSecond (GetPerformanceCounter ());

  //
  // Update S3 Resume Performance Record.
  //
  S3PerformanceTablePointer = 0;
  VarSize = sizeof (EFI_PHYSICAL_ADDRESS);
  Status = RestoreLockBox (&gFirmwarePerformanceS3PointerGuid, &S3PerformanceTablePointer, &VarSize);
  ASSERT_EFI_ERROR (Status);

  AcpiS3PerformanceTable = (S3_PERFORMANCE_TABLE *) (UINTN) S3PerformanceTablePointer;
  ASSERT (AcpiS3PerformanceTable != NULL);
  if (AcpiS3PerformanceTable->Header.Signature != EFI_ACPI_5_0_FPDT_S3_PERFORMANCE_TABLE_SIGNATURE) {
    DEBUG ((EFI_D_ERROR, "FPDT S3 performance data in ACPI memory get corrupted\n"));
    return EFI_ABORTED;
  }
  AcpiS3ResumeRecord = &AcpiS3PerformanceTable->S3Resume;
  AcpiS3ResumeRecord->FullResume = CurrentTime;
  //
  // Calculate average S3 resume time.
  //
  S3ResumeTotal = MultU64x32 (AcpiS3ResumeRecord->AverageResume, AcpiS3ResumeRecord->ResumeCount);
  AcpiS3ResumeRecord->ResumeCount++;
  AcpiS3ResumeRecord->AverageResume = DivU64x32 (S3ResumeTotal + AcpiS3ResumeRecord->FullResume, AcpiS3ResumeRecord->ResumeCount);

  DEBUG ((EFI_D_INFO, "FPDT: S3 Resume Performance - ResumeCount   = %d\n", AcpiS3ResumeRecord->ResumeCount));
  DEBUG ((EFI_D_INFO, "FPDT: S3 Resume Performance - FullResume    = %ld\n", AcpiS3ResumeRecord->FullResume));
  DEBUG ((EFI_D_INFO, "FPDT: S3 Resume Performance - AverageResume = %ld\n", AcpiS3ResumeRecord->AverageResume));

  //
  // Update S3 Suspend Performance Record.
  //
  AcpiS3SuspendRecord = &AcpiS3PerformanceTable->S3Suspend;
  VarSize = sizeof (EFI_ACPI_5_0_FPDT_S3_SUSPEND_RECORD);
  ZeroMem (&S3SuspendRecord, sizeof (EFI_ACPI_5_0_FPDT_S3_SUSPEND_RECORD));
  Status = RestoreLockBox (
             &gEfiFirmwarePerformanceGuid,
             &S3SuspendRecord,
             &VarSize
             );
  ASSERT_EFI_ERROR (Status);

  AcpiS3SuspendRecord->SuspendStart = S3SuspendRecord.SuspendStart;
  AcpiS3SuspendRecord->SuspendEnd   = S3SuspendRecord.SuspendEnd;

  DEBUG ((EFI_D_INFO, "FPDT: S3 Suspend Performance - SuspendStart = %ld\n", AcpiS3SuspendRecord->SuspendStart));
  DEBUG ((EFI_D_INFO, "FPDT: S3 Suspend Performance - SuspendEnd   = %ld\n", AcpiS3SuspendRecord->SuspendEnd));

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **) &VariableServices
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Update S3 boot records into the basic boot performance table.
  //
  VarSize = sizeof (PerformanceVariable);
  Status = VariableServices->GetVariable (
                               VariableServices,
                               EFI_FIRMWARE_PERFORMANCE_VARIABLE_NAME,
                               &gEfiFirmwarePerformanceGuid,
                               NULL,
                               &VarSize,
                               &PerformanceVariable
                               );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  BootPerformanceTable = (UINT8*) (UINTN) PerformanceVariable.BootPerformanceTablePointer;

  //
  // Dump PEI boot records
  //
  FirmwarePerformanceTablePtr = (BootPerformanceTable + sizeof (BOOT_PERFORMANCE_TABLE));
  GuidHob   = GetFirstGuidHob (&gEdkiiFpdtExtendedFirmwarePerformanceGuid);
  while (GuidHob != NULL) {
    FirmwarePerformanceData = GET_GUID_HOB_DATA (GuidHob);
    PeiPerformanceLogHeader = (FPDT_PEI_EXT_PERF_HEADER *) FirmwarePerformanceData;

    CopyMem (FirmwarePerformanceTablePtr, FirmwarePerformanceData + sizeof (FPDT_PEI_EXT_PERF_HEADER), (UINTN)(PeiPerformanceLogHeader->SizeOfAllEntries));

    GuidHob = GetNextGuidHob (&gEdkiiFpdtExtendedFirmwarePerformanceGuid, GET_NEXT_HOB (GuidHob));

    FirmwarePerformanceTablePtr += (UINTN)(PeiPerformanceLogHeader->SizeOfAllEntries);
  }

  //
  // Update Table length.
  //
  ((BOOT_PERFORMANCE_TABLE *) BootPerformanceTable)->Header.Length = (UINT32)((UINTN)FirmwarePerformanceTablePtr - (UINTN)BootPerformanceTable);

  return EFI_SUCCESS;
}

/**
  Main entry for Firmware Performance Data Table PEIM.

  This routine is to register report status code listener for FPDT.

  @param[in]  FileHandle              Handle of the file being invoked.
  @param[in]  PeiServices             Pointer to PEI Services table.

  @retval EFI_SUCCESS Report status code listener is registered successfully.

**/
EFI_STATUS
EFIAPI
FirmwarePerformancePeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS               Status;
  EFI_PEI_RSC_HANDLER_PPI  *RscHandler;

  if (FeaturePcdGet (PcdFirmwarePerformanceDataTableS3Support)) {
    //
    // S3 resume - register status code listener for OS wake vector.
    //
    Status = PeiServicesLocatePpi (
               &gEfiPeiRscHandlerPpiGuid,
               0,
               NULL,
               (VOID **) &RscHandler
               );
    ASSERT_EFI_ERROR (Status);

    Status = RscHandler->Register (FpdtStatusCodeListenerPei);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
