/** @file
  This module install ACPI Firmware Performance Data Table (FPDT).

  This module register report status code listener to collect performance data
  for Firmware Basic Boot Performance Record and other boot performance records, 
  and install FPDT to ACPI table.

  Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Protocol/ReportStatusCodeHandler.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/LockBox.h>
#include <Protocol/Variable.h>

#include <Guid/Acpi.h>
#include <Guid/FirmwarePerformance.h>
#include <Guid/EventGroup.h>
#include <Guid/EventLegacyBios.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/LockBoxLib.h>
#include <Library/UefiLib.h>

#define SMM_BOOT_RECORD_COMM_SIZE (OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data) + sizeof(SMM_BOOT_RECORD_COMMUNICATE))

EFI_RSC_HANDLER_PROTOCOL    *mRscHandlerProtocol = NULL;

BOOLEAN                     mLockBoxReady = FALSE;
EFI_EVENT                   mReadyToBootEvent;
EFI_EVENT                   mLegacyBootEvent;
EFI_EVENT                   mExitBootServicesEvent;
UINTN                       mFirmwarePerformanceTableTemplateKey  = 0;
BOOLEAN                     mDxeCoreReportStatusCodeEnable = FALSE;

BOOT_PERFORMANCE_TABLE                      *mAcpiBootPerformanceTable = NULL;
BOOT_PERFORMANCE_TABLE                      *mReceivedAcpiBootPerformanceTable = NULL;
S3_PERFORMANCE_TABLE                        *mAcpiS3PerformanceTable   = NULL;

FIRMWARE_PERFORMANCE_TABLE  mFirmwarePerformanceTableTemplate = {
  {
    EFI_ACPI_5_0_FIRMWARE_PERFORMANCE_DATA_TABLE_SIGNATURE,
    sizeof (FIRMWARE_PERFORMANCE_TABLE),
    EFI_ACPI_5_0_FIRMWARE_PERFORMANCE_DATA_TABLE_REVISION,    // Revision
    0x00, // Checksum will be updated at runtime
    //
    // It is expected that these values will be updated at EntryPoint.
    //
    {0x00},     // OEM ID is a 6 bytes long field
    0x00,       // OEM Table ID(8 bytes long)
    0x00,       // OEM Revision
    0x00,       // Creator ID
    0x00,       // Creator Revision
  },
  //
  // Firmware Basic Boot Performance Table Pointer Record.
  //
  {
    {
      EFI_ACPI_5_0_FPDT_RECORD_TYPE_FIRMWARE_BASIC_BOOT_POINTER ,       // Type
      sizeof (EFI_ACPI_5_0_FPDT_BOOT_PERFORMANCE_TABLE_POINTER_RECORD), // Length
      EFI_ACPI_5_0_FPDT_RECORD_REVISION_FIRMWARE_BASIC_BOOT_POINTER     // Revision
    },
    0,  // Reserved
    0   // BootPerformanceTablePointer will be updated at runtime.
  },
  //
  // S3 Performance Table Pointer Record.
  //
  {
    {
      EFI_ACPI_5_0_FPDT_RECORD_TYPE_S3_PERFORMANCE_TABLE_POINTER,     // Type
      sizeof (EFI_ACPI_5_0_FPDT_S3_PERFORMANCE_TABLE_POINTER_RECORD), // Length
      EFI_ACPI_5_0_FPDT_RECORD_REVISION_S3_PERFORMANCE_TABLE_POINTER  // Revision
    },
    0,  // Reserved
    0   // S3PerformanceTablePointer will be updated at runtime.
  }
};

BOOT_PERFORMANCE_TABLE mBootPerformanceTableTemplate = {
  {
    EFI_ACPI_5_0_FPDT_BOOT_PERFORMANCE_TABLE_SIGNATURE,
    sizeof (BOOT_PERFORMANCE_TABLE)
  },
  {
    {
      EFI_ACPI_5_0_FPDT_RUNTIME_RECORD_TYPE_FIRMWARE_BASIC_BOOT,    // Type
      sizeof (EFI_ACPI_5_0_FPDT_FIRMWARE_BASIC_BOOT_RECORD),        // Length
      EFI_ACPI_5_0_FPDT_RUNTIME_RECORD_REVISION_FIRMWARE_BASIC_BOOT // Revision
    },
    0,  // Reserved
    //
    // These values will be updated at runtime.
    //
    0,  // ResetEnd
    0,  // OsLoaderLoadImageStart
    0,  // OsLoaderStartImageStart
    0,  // ExitBootServicesEntry
    0   // ExitBootServicesExit
  }
};

S3_PERFORMANCE_TABLE        mS3PerformanceTableTemplate = {
  {
    EFI_ACPI_5_0_FPDT_S3_PERFORMANCE_TABLE_SIGNATURE,
    sizeof (S3_PERFORMANCE_TABLE)
  },
  {
    {
      EFI_ACPI_5_0_FPDT_RUNTIME_RECORD_TYPE_S3_RESUME,     // Type
      sizeof (EFI_ACPI_5_0_FPDT_S3_RESUME_RECORD),         // Length
      EFI_ACPI_5_0_FPDT_RUNTIME_RECORD_REVISION_S3_RESUME  // Revision
    },
    //
    // These values will be updated by Firmware Performance PEIM.
    //
    0,  // ResumeCount
    0,  // FullResume
    0   // AverageResume
  },
  {
    {
      EFI_ACPI_5_0_FPDT_RUNTIME_RECORD_TYPE_S3_SUSPEND,    // Type
      sizeof (EFI_ACPI_5_0_FPDT_S3_SUSPEND_RECORD),        // Length
      EFI_ACPI_5_0_FPDT_RUNTIME_RECORD_REVISION_S3_SUSPEND // Revision
    },
    //
    // These values will be updated bye Firmware Performance SMM driver.
    //
    0,  // SuspendStart
    0   // SuspendEnd
  }
};

/**
  This function calculates and updates an UINT8 checksum.

  @param[in]  Buffer          Pointer to buffer to checksum
  @param[in]  Size            Number of bytes to checksum

**/
VOID
FpdtAcpiTableChecksum (
  IN UINT8      *Buffer,
  IN UINTN      Size
  )
{
  UINTN ChecksumOffset;

  ChecksumOffset = OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Checksum);

  //
  // Set checksum to 0 first.
  //
  Buffer[ChecksumOffset] = 0;

  //
  // Update checksum value.
  //
  Buffer[ChecksumOffset] = CalculateCheckSum8 (Buffer, Size);
}

/**
  Allocate EfiReservedMemoryType below 4G memory address.

  This function allocates EfiReservedMemoryType below 4G memory address.

  @param[in]  Size   Size of memory to allocate.

  @return Allocated address for output.

**/
VOID *
FpdtAllocateReservedMemoryBelow4G (
  IN UINTN       Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID                  *Buffer;

  Buffer  = NULL;
  Pages   = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiReservedMemoryType,
                  Pages,
                  &Address
                  );
  ASSERT_EFI_ERROR (Status);

  if (!EFI_ERROR (Status)) {
    Buffer = (VOID *) (UINTN) Address;
    ZeroMem (Buffer, Size);
  }

  return Buffer;
}

/**
  Callback function upon VariableArchProtocol and LockBoxProtocol
  to allocate S3 performance table memory and save the pointer to LockBox.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
FpdtAllocateS3PerformanceTableMemory (
  IN  EFI_EVENT                             Event,
  IN  VOID                                  *Context
  )
{
  EFI_STATUS                    Status;
  VOID                          *Interface;
  FIRMWARE_PERFORMANCE_VARIABLE PerformanceVariable;
  UINTN                         Size;
  EFI_PHYSICAL_ADDRESS          S3PerformanceTablePointer;

  if (mLockBoxReady && (mAcpiS3PerformanceTable != NULL)) {
    //
    // The memory for S3 performance table should have been ready,
    // and the pointer should have been saved to LockBox, just return.
    //
    return;
  }

  if (!mLockBoxReady) {
    Status = gBS->LocateProtocol (&gEfiLockBoxProtocolGuid, NULL, &Interface);
    if (!EFI_ERROR (Status)) {
      //
      // LockBox services has been ready.
      //
      mLockBoxReady = TRUE;
    }
  }

  if (mAcpiS3PerformanceTable == NULL) {
    Status = gBS->LocateProtocol (&gEfiVariableArchProtocolGuid, NULL, &Interface);
    if (!EFI_ERROR (Status)) {
      //
      // Try to allocate the same runtime buffer as last time boot.
      //
      ZeroMem (&PerformanceVariable, sizeof (PerformanceVariable));
      Size = sizeof (PerformanceVariable);
      Status = gRT->GetVariable (
                      EFI_FIRMWARE_PERFORMANCE_VARIABLE_NAME,
                      &gEfiFirmwarePerformanceGuid,
                      NULL,
                      &Size,
                      &PerformanceVariable
                      );
      if (!EFI_ERROR (Status)) {
        Status = gBS->AllocatePages (
                        AllocateAddress,
                        EfiReservedMemoryType,
                        EFI_SIZE_TO_PAGES (sizeof (S3_PERFORMANCE_TABLE)),
                        &PerformanceVariable.S3PerformanceTablePointer
                        );
        if (!EFI_ERROR (Status)) {
          mAcpiS3PerformanceTable = (S3_PERFORMANCE_TABLE *) (UINTN) PerformanceVariable.S3PerformanceTablePointer;
        }
      }
      if (mAcpiS3PerformanceTable == NULL) {
        //
        // Fail to allocate at specified address, continue to allocate at any address.
        //
        mAcpiS3PerformanceTable = (S3_PERFORMANCE_TABLE *) FpdtAllocateReservedMemoryBelow4G (sizeof (S3_PERFORMANCE_TABLE));
      }
      DEBUG ((EFI_D_INFO, "FPDT: ACPI S3 Performance Table address = 0x%x\n", mAcpiS3PerformanceTable));
      if (mAcpiS3PerformanceTable != NULL) {
        CopyMem (mAcpiS3PerformanceTable, &mS3PerformanceTableTemplate, sizeof (mS3PerformanceTableTemplate));
      }
    }
  }

  if (mLockBoxReady && (mAcpiS3PerformanceTable != NULL)) {
    //
    // If LockBox services has been ready and memory for FPDT S3 performance table has been allocated,
    // save the pointer to LockBox for use in S3 resume.
    //
    S3PerformanceTablePointer = (EFI_PHYSICAL_ADDRESS) (UINTN) mAcpiS3PerformanceTable;
    Status = SaveLockBox (
               &gFirmwarePerformanceS3PointerGuid,
               &S3PerformanceTablePointer,
               sizeof (EFI_PHYSICAL_ADDRESS)
               );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Install ACPI Firmware Performance Data Table (FPDT).

  @return Status code.

**/
EFI_STATUS
InstallFirmwarePerformanceDataTable (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_ACPI_TABLE_PROTOCOL       *AcpiTableProtocol;
  UINTN                         BootPerformanceDataSize;
  FIRMWARE_PERFORMANCE_VARIABLE PerformanceVariable;
  UINTN                         Size;

  //
  // Get AcpiTable Protocol.
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **) &AcpiTableProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (mReceivedAcpiBootPerformanceTable != NULL) {
    mAcpiBootPerformanceTable = mReceivedAcpiBootPerformanceTable;
    mAcpiBootPerformanceTable->BasicBoot.ResetEnd = mBootPerformanceTableTemplate.BasicBoot.ResetEnd;
  } else {
    //
    // Try to allocate the same runtime buffer as last time boot.
    //
    BootPerformanceDataSize = sizeof (BOOT_PERFORMANCE_TABLE);
    ZeroMem (&PerformanceVariable, sizeof (PerformanceVariable));
    Size = sizeof (PerformanceVariable);
    Status = gRT->GetVariable (
                    EFI_FIRMWARE_PERFORMANCE_VARIABLE_NAME,
                    &gEfiFirmwarePerformanceGuid,
                    NULL,
                    &Size,
                    &PerformanceVariable
                    );
    if (!EFI_ERROR (Status)) {
      Status = gBS->AllocatePages (
                      AllocateAddress,
                      EfiReservedMemoryType,
                      EFI_SIZE_TO_PAGES (BootPerformanceDataSize),
                      &PerformanceVariable.BootPerformanceTablePointer
                      );
      if (!EFI_ERROR (Status)) {
         mAcpiBootPerformanceTable = (BOOT_PERFORMANCE_TABLE *) (UINTN) PerformanceVariable.BootPerformanceTablePointer;
      }
    }
    if (mAcpiBootPerformanceTable == NULL) {
      //
      // Fail to allocate at specified address, continue to allocate at any address.
      //
      mAcpiBootPerformanceTable = (BOOT_PERFORMANCE_TABLE *) FpdtAllocateReservedMemoryBelow4G (BootPerformanceDataSize);
    }
    DEBUG ((DEBUG_INFO, "FPDT: ACPI Boot Performance Table address = 0x%x\n", mAcpiBootPerformanceTable));
    if (mAcpiBootPerformanceTable == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Fill Basic Boot record to Boot Performance Table.
    //
    CopyMem (mAcpiBootPerformanceTable, &mBootPerformanceTableTemplate, sizeof (mBootPerformanceTableTemplate));
  }
  BootPerformanceDataSize   = mAcpiBootPerformanceTable->Header.Length;

  //
  // Save Boot Performance Table address to Variable for use in S4 resume.
  //
  PerformanceVariable.BootPerformanceTablePointer = (EFI_PHYSICAL_ADDRESS) (UINTN) mAcpiBootPerformanceTable;
  //
  // Update Boot Performance Table Pointer in template.
  //
  mFirmwarePerformanceTableTemplate.BootPointerRecord.BootPerformanceTablePointer = (UINT64) (UINTN) mAcpiBootPerformanceTable;

  //
  // Save S3 Performance Table address to Variable for use in S4 resume.
  //
  PerformanceVariable.S3PerformanceTablePointer = (EFI_PHYSICAL_ADDRESS) (UINTN) mAcpiS3PerformanceTable;
  //
  // Update S3 Performance Table Pointer in template.
  //
  mFirmwarePerformanceTableTemplate.S3PointerRecord.S3PerformanceTablePointer = (UINT64) (UINTN) mAcpiS3PerformanceTable;
  //
  // Save Runtime Performance Table pointers to Variable.
  // Don't check SetVariable return status. It doesn't impact FPDT table generation.
  //
  gRT->SetVariable (
        EFI_FIRMWARE_PERFORMANCE_VARIABLE_NAME,
        &gEfiFirmwarePerformanceGuid,
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
        sizeof (PerformanceVariable),
        &PerformanceVariable
        );

  //
  // Publish Firmware Performance Data Table.
  //
  FpdtAcpiTableChecksum ((UINT8 *) &mFirmwarePerformanceTableTemplate, mFirmwarePerformanceTableTemplate.Header.Length);
  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                &mFirmwarePerformanceTableTemplate,
                                mFirmwarePerformanceTableTemplate.Header.Length,
                                &mFirmwarePerformanceTableTemplateKey
                                );
  if (EFI_ERROR (Status)) {
    if (mAcpiBootPerformanceTable != NULL) {
      FreePages (mAcpiBootPerformanceTable, EFI_SIZE_TO_PAGES (BootPerformanceDataSize));
    }
    if (mAcpiS3PerformanceTable != NULL) {
      FreePages (mAcpiS3PerformanceTable, EFI_SIZE_TO_PAGES (sizeof (S3_PERFORMANCE_TABLE)));
    }
    mAcpiBootPerformanceTable = NULL;
    mAcpiS3PerformanceTable = NULL;
    return Status;
  }
  return EFI_SUCCESS;
}

/**
  Report status code listener of FPDT. This is used to collect performance data
  for OsLoaderLoadImageStart and OsLoaderStartImageStart in FPDT.

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
FpdtStatusCodeListenerDxe (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data
  )
{
  EFI_STATUS  Status;

  //
  // Check whether status code is what we are interested in.
  //
  if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) != EFI_PROGRESS_CODE) {
    return EFI_UNSUPPORTED;
  }
  
  if (Value == (EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_PC_HANDOFF_TO_NEXT)) {
    //
    // DxeCore ReportStatusCode Enable so that the capability can be supported.
    //
    mDxeCoreReportStatusCodeEnable = TRUE;
  }

  Status = EFI_SUCCESS;
  if (Value == PcdGet32 (PcdProgressCodeOsLoaderLoad)) {
    //
    // Progress code for OS Loader LoadImage.
    //
    if (mAcpiBootPerformanceTable == NULL) {
      return Status;
    }

    //
    // Update OS Loader LoadImage Start for UEFI boot.
    //
    mAcpiBootPerformanceTable->BasicBoot.OsLoaderLoadImageStart = GetTimeInNanoSecond (GetPerformanceCounter ());
  } else if (Value == PcdGet32 (PcdProgressCodeOsLoaderStart)) {
    //
    // Progress code for OS Loader StartImage.
    //
    if (mAcpiBootPerformanceTable == NULL) {
      return Status;
    }

    //
    // Update OS Loader StartImage Start for UEFI boot.
    //
    mAcpiBootPerformanceTable->BasicBoot.OsLoaderStartImageStart = GetTimeInNanoSecond (GetPerformanceCounter ());
  } else if (Value == (EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_BS_PC_EXIT_BOOT_SERVICES)) {
    //
    // Unregister boot time report status code listener.
    //
    mRscHandlerProtocol->Unregister (FpdtStatusCodeListenerDxe);

    //
    // Progress code for ExitBootServices.
    //
    if (mAcpiBootPerformanceTable == NULL) {
      return Status;
    }

    //
    // Update ExitBootServicesExit for UEFI boot.
    //
    mAcpiBootPerformanceTable->BasicBoot.ExitBootServicesExit = GetTimeInNanoSecond (GetPerformanceCounter ());
  } else if (Value == (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_LEGACY_BOOT_EVENT)) {
    if (mAcpiBootPerformanceTable == NULL) {
      //
      // Firmware Performance Data Table not installed, do nothing.
      //
      return Status;
    }

    //
    // Update Firmware Basic Boot Performance Record for legacy boot.
    //
    mAcpiBootPerformanceTable->BasicBoot.OsLoaderStartImageStart = GetTimeInNanoSecond (GetPerformanceCounter ());

    //
    // Dump FPDT Boot Performance record.
    //
    DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - ResetEnd                = %ld\n", mAcpiBootPerformanceTable->BasicBoot.ResetEnd));
    DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - OsLoaderLoadImageStart  = 0\n"));
    DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - OsLoaderStartImageStart = %ld\n", mAcpiBootPerformanceTable->BasicBoot.OsLoaderStartImageStart));
    DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - ExitBootServicesEntry   = 0\n"));
    DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - ExitBootServicesExit    = 0\n"));
  } else if (Value == (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_READY_TO_BOOT_EVENT)) {
    if (mAcpiBootPerformanceTable == NULL) {
      //
      // ACPI Firmware Performance Data Table not installed yet, install it now.
      //
      InstallFirmwarePerformanceDataTable ();
    }
  } else if (Data != NULL && CompareGuid (&Data->Type, &gEdkiiFpdtExtendedFirmwarePerformanceGuid)) {
    //
    // Get the Boot performance table and then install it to ACPI table.
    //
    CopyMem (&mReceivedAcpiBootPerformanceTable, Data + 1, Data->Size);
  } else {
    //
    // Ignore else progress code.
    //
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}


/**
  Notify function for event EVT_SIGNAL_EXIT_BOOT_SERVICES. This is used to record
  performance data for ExitBootServicesEntry in FPDT.

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
VOID
EFIAPI
FpdtExitBootServicesEventNotify (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  if (!mDxeCoreReportStatusCodeEnable) {
    //
    // When DxeCore Report Status Code is disabled, 
    // Unregister boot time report status code listener at ExitBootService Event.
    //
    mRscHandlerProtocol->Unregister (FpdtStatusCodeListenerDxe);
  }

  if (mAcpiBootPerformanceTable == NULL) {
    //
    // Firmware Performance Data Table not installed, do nothing.
    //
    return ;
  }

  //
  // Update Firmware Basic Boot Performance Record for UEFI boot.
  //
  mAcpiBootPerformanceTable->BasicBoot.ExitBootServicesEntry = GetTimeInNanoSecond (GetPerformanceCounter ());

  //
  // Dump FPDT Boot Performance record.
  //
  DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - ResetEnd                = %ld\n", mAcpiBootPerformanceTable->BasicBoot.ResetEnd));
  DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - OsLoaderLoadImageStart  = %ld\n", mAcpiBootPerformanceTable->BasicBoot.OsLoaderLoadImageStart));
  DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - OsLoaderStartImageStart = %ld\n", mAcpiBootPerformanceTable->BasicBoot.OsLoaderStartImageStart));
  DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - ExitBootServicesEntry   = %ld\n", mAcpiBootPerformanceTable->BasicBoot.ExitBootServicesEntry));
  //
  // ExitBootServicesExit will be updated later, so don't dump it here.
  //
}

/**
  The module Entry Point of the Firmware Performance Data Table DXE driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
FirmwarePerformanceDxeEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS               Status;
  EFI_HOB_GUID_TYPE        *GuidHob;
  FIRMWARE_SEC_PERFORMANCE *Performance;
  VOID                     *Registration;
  UINT64                   OemTableId;

  CopyMem (
    mFirmwarePerformanceTableTemplate.Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (mFirmwarePerformanceTableTemplate.Header.OemId)
    );
  OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (&mFirmwarePerformanceTableTemplate.Header.OemTableId, &OemTableId, sizeof (UINT64));
  mFirmwarePerformanceTableTemplate.Header.OemRevision      = PcdGet32 (PcdAcpiDefaultOemRevision);
  mFirmwarePerformanceTableTemplate.Header.CreatorId        = PcdGet32 (PcdAcpiDefaultCreatorId);
  mFirmwarePerformanceTableTemplate.Header.CreatorRevision  = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  //
  // Get Report Status Code Handler Protocol.
  //
  Status = gBS->LocateProtocol (&gEfiRscHandlerProtocolGuid, NULL, (VOID **) &mRscHandlerProtocol);
  ASSERT_EFI_ERROR (Status);

  //
  // Register report status code listener for OS Loader load and start.
  //
  Status = mRscHandlerProtocol->Register (FpdtStatusCodeListenerDxe, TPL_HIGH_LEVEL);
  ASSERT_EFI_ERROR (Status);

  //
  // Register the notify function to update FPDT on ExitBootServices Event.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FpdtExitBootServicesEventNotify,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &mExitBootServicesEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Retrieve GUID HOB data that contains the ResetEnd.
  //
  GuidHob = GetFirstGuidHob (&gEfiFirmwarePerformanceGuid);
  if (GuidHob != NULL) {
    Performance = (FIRMWARE_SEC_PERFORMANCE *) GET_GUID_HOB_DATA (GuidHob);
    mBootPerformanceTableTemplate.BasicBoot.ResetEnd = Performance->ResetEnd;
  } else {
    //
    // SEC Performance Data Hob not found, ResetEnd in ACPI FPDT table will be 0.
    //
    DEBUG ((DEBUG_WARN, "FPDT: WARNING: SEC Performance Data Hob not found, ResetEnd will be set to 0!\n"));
  }

  if (FeaturePcdGet (PcdFirmwarePerformanceDataTableS3Support)) {
    //
    // Register callback function upon VariableArchProtocol and LockBoxProtocol
    // to allocate S3 performance table memory and save the pointer to LockBox.
    //
    EfiCreateProtocolNotifyEvent (
      &gEfiVariableArchProtocolGuid,
      TPL_CALLBACK,
      FpdtAllocateS3PerformanceTableMemory,
      NULL,
      &Registration
      );
    EfiCreateProtocolNotifyEvent (
      &gEfiLockBoxProtocolGuid,
      TPL_CALLBACK,
      FpdtAllocateS3PerformanceTableMemory,
      NULL,
      &Registration
      );
  } else {
    //
    // Exclude S3 Performance Table Pointer from FPDT table template.
    //
    mFirmwarePerformanceTableTemplate.Header.Length -= sizeof (EFI_ACPI_5_0_FPDT_S3_PERFORMANCE_TABLE_POINTER_RECORD);
  }

  return EFI_SUCCESS;
}
