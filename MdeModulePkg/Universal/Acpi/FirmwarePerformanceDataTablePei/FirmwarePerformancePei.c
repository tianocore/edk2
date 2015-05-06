/** @file
  This module updates S3 Resume Performance Record in ACPI Firmware Performance
  Data Table in S3 resume boot mode. In normal boot mode, this module consumes
  SecPerformance PPI produced by SEC phase and build Hob to convey the SEC
  performance data to DXE phase.

  This module register report status code listener to collect performance data
  for S3 Resume Performance Record on S3 resume boot path.

  Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Ppi/ReportStatusCodeHandler.h>
#include <Ppi/SecPerformance.h>

#include <Guid/FirmwarePerformance.h>

#include <Library/PeiServicesLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/LockBoxLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

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
  PEI_SEC_PERFORMANCE_PPI  *SecPerf;
  FIRMWARE_SEC_PERFORMANCE Performance;

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

  //
  // Normal boot - build Hob for SEC performance data.
  //
  Status = PeiServicesLocatePpi (
             &gPeiSecPerformancePpiGuid,
             0,
             NULL,
             (VOID **) &SecPerf
             );
  if (!EFI_ERROR (Status)) {
    Status = SecPerf->GetPerformance (PeiServices, SecPerf, &Performance);
  }
  if (!EFI_ERROR (Status)) {
    BuildGuidDataHob (
      &gEfiFirmwarePerformanceGuid,
      &Performance,
      sizeof (FIRMWARE_SEC_PERFORMANCE)
    );
    DEBUG ((EFI_D_INFO, "FPDT: SEC Performance Hob ResetEnd = %ld\n", Performance.ResetEnd));
  } else {
    //
    // SEC performance PPI is not installed or fail to get performance data
    // from SEC Performance PPI.
    //
    DEBUG ((EFI_D_ERROR, "FPDT: WARNING: SEC Performance PPI not installed or failed!\n"));
  }

  return EFI_SUCCESS;
}
