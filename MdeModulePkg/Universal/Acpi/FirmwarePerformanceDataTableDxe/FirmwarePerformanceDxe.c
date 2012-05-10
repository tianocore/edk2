/** @file
  This module install ACPI Firmware Performance Data Table (FPDT).

  This module register report status code listener to collect performance data
  for Firmware Basic Boot Performance Record and other boot performance records, 
  and install FPDT to ACPI table.

  Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <IndustryStandard/Acpi50.h>

#include <Protocol/ReportStatusCodeHandler.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/SmmCommunication.h>

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
#include <Library/PcdLib.h>

//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_OEM_ID           "INTEL"
#define EFI_ACPI_OEM_TABLE_ID     0x2020204F4E414954ULL // "TIANO   "
#define EFI_ACPI_OEM_REVISION     0x00000001
#define EFI_ACPI_CREATOR_ID       0x5446534D            // TBD "MSFT"
#define EFI_ACPI_CREATOR_REVISION 0x01000013            // TBD
#define EXTENSION_RECORD_SIZE     0x10000
#define SMM_BOOT_RECORD_COMM_SIZE OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data) + sizeof(SMM_BOOT_RECORD_COMMUNICATE)

EFI_RSC_HANDLER_PROTOCOL    *mRscHandlerProtocol = NULL;

EFI_EVENT                   mReadyToBootEvent;
EFI_EVENT                   mLegacyBootEvent;
EFI_EVENT                   mExitBootServicesEvent;
UINTN                       mFirmwarePerformanceTableTemplateKey  = 0;
UINT32                      mBootRecordSize = 0;
UINT32                      mBootRecordMaxSize = 0;
UINT8                       *mBootRecordBuffer = NULL;

BOOT_PERFORMANCE_TABLE                      *mAcpiBootPerformanceTable = NULL;
S3_PERFORMANCE_TABLE                        *mAcpiS3PerformanceTable   = NULL;

FIRMWARE_PERFORMANCE_TABLE  mFirmwarePerformanceTableTemplate = {
  {
    EFI_ACPI_5_0_FIRMWARE_PERFORMANCE_DATA_TABLE_SIGNATURE,
    sizeof (FIRMWARE_PERFORMANCE_TABLE),
    EFI_ACPI_5_0_FIRMWARE_PERFORMANCE_DATA_TABLE_REVISION,    // Revision
    0x00, // Checksum will be updated at runtime
    //
    // It is expected that these values will be updated at runtime.
    //
    EFI_ACPI_OEM_ID,            // OEMID is a 6 bytes long field
    EFI_ACPI_OEM_TABLE_ID,      // OEM table identification(8 bytes long)
    EFI_ACPI_OEM_REVISION,      // OEM revision number
    EFI_ACPI_CREATOR_ID,        // ASL compiler vendor ID
    EFI_ACPI_CREATOR_REVISION,  // ASL compiler revision number
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

  Pages   = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiReservedMemoryType,
                  Pages,
                  &Address
                  );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, Size);

  return Buffer;
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
  EFI_PHYSICAL_ADDRESS          Address;
  UINTN                         Size;
  UINT8                         SmmBootRecordCommBuffer[SMM_BOOT_RECORD_COMM_SIZE];
  EFI_SMM_COMMUNICATE_HEADER    *SmmCommBufferHeader;
  SMM_BOOT_RECORD_COMMUNICATE   *SmmCommData;
  UINTN                         CommSize;
  UINTN                         PerformanceRuntimeDataSize;
  UINT8                         *PerformanceRuntimeData; 
  UINT8                         *PerformanceRuntimeDataHead; 
  EFI_SMM_COMMUNICATION_PROTOCOL  *Communication;
  FIRMWARE_PERFORMANCE_VARIABLE PerformanceVariable;

  //
  // Get AcpiTable Protocol.
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **) &AcpiTableProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Collect boot records from SMM drivers.
  //
  SmmCommData = NULL;
  Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **) &Communication);
  if (!EFI_ERROR (Status)) {
    //
    // Initialize communicate buffer 
    //
    SmmCommBufferHeader = (EFI_SMM_COMMUNICATE_HEADER*)SmmBootRecordCommBuffer;
    SmmCommData = (SMM_BOOT_RECORD_COMMUNICATE*)SmmCommBufferHeader->Data;
    ZeroMem((UINT8*)SmmCommData, sizeof(SMM_BOOT_RECORD_COMMUNICATE));

    CopyGuid (&SmmCommBufferHeader->HeaderGuid, &gEfiFirmwarePerformanceGuid);
    SmmCommBufferHeader->MessageLength = sizeof(SMM_BOOT_RECORD_COMMUNICATE);
    CommSize = SMM_BOOT_RECORD_COMM_SIZE;
  
    //
    // Get the size of boot records.
    //
    SmmCommData->Function       = SMM_FPDT_FUNCTION_GET_BOOT_RECORD_SIZE;
    SmmCommData->BootRecordData = NULL;
    Status = Communication->Communicate (Communication, SmmBootRecordCommBuffer, &CommSize);
    ASSERT_EFI_ERROR (Status);
  
    if (!EFI_ERROR (SmmCommData->ReturnStatus) && SmmCommData->BootRecordSize != 0) {
      //
      // Get all boot records
      //
      SmmCommData->Function       = SMM_FPDT_FUNCTION_GET_BOOT_RECORD_DATA;
      SmmCommData->BootRecordData = AllocateZeroPool(SmmCommData->BootRecordSize);
      ASSERT (SmmCommData->BootRecordData != NULL);
      
      Status = Communication->Communicate (Communication, SmmBootRecordCommBuffer, &CommSize);
      ASSERT_EFI_ERROR (Status);
      ASSERT_EFI_ERROR(SmmCommData->ReturnStatus);
    }
  }

  //
  // Prepare memory for runtime Performance Record. 
  // Runtime performance records includes two tables S3 performance table and Boot performance table. 
  // S3 Performance table includes S3Resume and S3Suspend records. 
  // Boot Performance table includes BasicBoot record, and one or more appended Boot Records. 
  //
  PerformanceRuntimeData = NULL;
  PerformanceRuntimeDataSize = sizeof (S3_PERFORMANCE_TABLE) + sizeof (BOOT_PERFORMANCE_TABLE) + mBootRecordSize + PcdGet32 (PcdExtFpdtBootRecordPadSize);
  if (SmmCommData != NULL) {
    PerformanceRuntimeDataSize += SmmCommData->BootRecordSize;
  }

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
    Address = PerformanceVariable.S3PerformanceTablePointer;
    Status = gBS->AllocatePages (
                    AllocateAddress,
                    EfiReservedMemoryType,
                    EFI_SIZE_TO_PAGES (PerformanceRuntimeDataSize),
                    &Address
                    );
    if (!EFI_ERROR (Status)) {
      PerformanceRuntimeData = (UINT8 *) (UINTN) Address;
    }
  }

  if (PerformanceRuntimeData == NULL) {
    //
    // Fail to allocate at specified address, continue to allocate at any address.
    //
    PerformanceRuntimeData = FpdtAllocateReservedMemoryBelow4G (PerformanceRuntimeDataSize);
  }
  DEBUG ((EFI_D_INFO, "FPDT: Performance Runtime Data address = 0x%x\n", PerformanceRuntimeData));

  if (PerformanceRuntimeData == NULL) {
    if (SmmCommData != NULL && SmmCommData->BootRecordData != NULL) {
      FreePool (SmmCommData->BootRecordData);
    }
    return EFI_OUT_OF_RESOURCES;
  }
  
  PerformanceRuntimeDataHead = PerformanceRuntimeData;

  if (FeaturePcdGet (PcdFirmwarePerformanceDataTableS3Support)) {
    //
    // Prepare S3 Performance Table.
    //
    mAcpiS3PerformanceTable = (S3_PERFORMANCE_TABLE *) PerformanceRuntimeData;
    CopyMem (mAcpiS3PerformanceTable, &mS3PerformanceTableTemplate, sizeof (mS3PerformanceTableTemplate));
    PerformanceRuntimeData  = PerformanceRuntimeData + mAcpiS3PerformanceTable->Header.Length;
    DEBUG ((EFI_D_INFO, "FPDT: ACPI S3 Performance Table address = 0x%x\n", mAcpiS3PerformanceTable));
    //
    // Save S3 Performance Table address to Variable for use in Firmware Performance PEIM.
    //
    PerformanceVariable.S3PerformanceTablePointer = (EFI_PHYSICAL_ADDRESS) (UINTN) mAcpiS3PerformanceTable;
    //
    // Update S3 Performance Table Pointer in template.
    //
    mFirmwarePerformanceTableTemplate.S3PointerRecord.S3PerformanceTablePointer = (UINT64) PerformanceVariable.S3PerformanceTablePointer;
  } else {
    //
    // Exclude S3 Performance Table Pointer from FPDT table template.
    //
    mFirmwarePerformanceTableTemplate.Header.Length -= sizeof (EFI_ACPI_5_0_FPDT_S3_PERFORMANCE_TABLE_POINTER_RECORD);
  }

  //
  // Prepare Boot Performance Table.
  //
  mAcpiBootPerformanceTable = (BOOT_PERFORMANCE_TABLE *) PerformanceRuntimeData;
  //
  // Fill Basic Boot record to Boot Performance Table.
  //
  CopyMem (PerformanceRuntimeData, &mBootPerformanceTableTemplate, sizeof (mBootPerformanceTableTemplate));
  PerformanceRuntimeData = PerformanceRuntimeData + mAcpiBootPerformanceTable->Header.Length;
  //
  // Fill Boot records from boot drivers.
  //
  CopyMem (PerformanceRuntimeData, mBootRecordBuffer, mBootRecordSize);
  mAcpiBootPerformanceTable->Header.Length += mBootRecordSize;
  PerformanceRuntimeData = PerformanceRuntimeData + mBootRecordSize;
  if (SmmCommData != NULL && SmmCommData->BootRecordData != NULL) {
    //
    // Fill Boot records from SMM drivers.
    //
    CopyMem (PerformanceRuntimeData, SmmCommData->BootRecordData, SmmCommData->BootRecordSize);
    FreePool (SmmCommData->BootRecordData);
    mAcpiBootPerformanceTable->Header.Length = (UINT32) (mAcpiBootPerformanceTable->Header.Length + SmmCommData->BootRecordSize);
    PerformanceRuntimeData = PerformanceRuntimeData + SmmCommData->BootRecordSize;
  }
  //
  // Reserve space for boot records after ReadyToBoot.
  //
  PerformanceRuntimeData = PerformanceRuntimeData + PcdGet32 (PcdExtFpdtBootRecordPadSize);
  DEBUG ((EFI_D_INFO, "FPDT: ACPI Boot Performance Table address = 0x%x\n", mAcpiBootPerformanceTable));
  //
  // Save Boot Performance Table address to Variable for use in S4 resume.
  //
  PerformanceVariable.BootPerformanceTablePointer = (EFI_PHYSICAL_ADDRESS) (UINTN) mAcpiBootPerformanceTable;
  //
  // Update Boot Performance Table Pointer in template.
  //
  mFirmwarePerformanceTableTemplate.BootPointerRecord.BootPerformanceTablePointer = (UINT64) (UINTN) mAcpiBootPerformanceTable;

  //
  // Save Runtime Performance Table pointers to Variable.
  //
  Status = gRT->SetVariable (
                  EFI_FIRMWARE_PERFORMANCE_VARIABLE_NAME,
                  &gEfiFirmwarePerformanceGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (PerformanceVariable),
                  &PerformanceVariable
                  );
  ASSERT_EFI_ERROR (Status);

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
    FreePool (PerformanceRuntimeDataHead);
    mAcpiBootPerformanceTable = NULL;
    mAcpiS3PerformanceTable = NULL;
    return Status;
  }
  
  //
  // Free temp Boot record, and update Boot Record to point to Basic Boot performance table.
  //
  if (mBootRecordBuffer != NULL) {
    FreePool (mBootRecordBuffer);
  }
  mBootRecordBuffer  = (UINT8 *) mAcpiBootPerformanceTable;
  mBootRecordSize    = mAcpiBootPerformanceTable->Header.Length;
  mBootRecordMaxSize = mBootRecordSize + PcdGet32 (PcdExtFpdtBootRecordPadSize);
  
  return EFI_SUCCESS;
}

/**
  Notify function for event group EFI_EVENT_GROUP_READY_TO_BOOT. This is used to
  install the Firmware Performance Data Table.

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
VOID
EFIAPI
FpdtReadyToBootEventNotify (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  if (mAcpiBootPerformanceTable == NULL) {
    //
    // ACPI Firmware Performance Data Table not installed yet, install it now.
    //
    InstallFirmwarePerformanceDataTable ();
  }
}

/**
  Notify function for event group EFI_EVENT_LEGACY_BOOT_GUID. This is used to
  record performance data for OsLoaderLoadImageStart in FPDT for legacy boot.

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
VOID
EFIAPI
FpdtLegacyBootEventNotify (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  if (mAcpiBootPerformanceTable == NULL) {
    //
    // Firmware Performance Data Table not installed, do nothing.
    //
    return ;
  }

  //
  // Update Firmware Basic Boot Performance Record for legacy boot.
  //
  mAcpiBootPerformanceTable->BasicBoot.OsLoaderLoadImageStart  = 0;
  mAcpiBootPerformanceTable->BasicBoot.OsLoaderStartImageStart = GetTimeInNanoSecond (GetPerformanceCounter ());
  mAcpiBootPerformanceTable->BasicBoot.ExitBootServicesEntry   = 0;
  mAcpiBootPerformanceTable->BasicBoot.ExitBootServicesExit    = 0;

  //
  // Dump FPDT Boot Performance record.
  //
  DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - ResetEnd                = %ld\n", mAcpiBootPerformanceTable->BasicBoot.ResetEnd));
  DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - OsLoaderLoadImageStart  = 0\n"));
  DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - OsLoaderStartImageStart = %ld\n", mAcpiBootPerformanceTable->BasicBoot.OsLoaderStartImageStart));
  DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - ExitBootServicesEntry   = 0\n"));
  DEBUG ((EFI_D_INFO, "FPDT: Boot Performance - ExitBootServicesExit    = 0\n"));
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
    // Progress code for ExitBootServices.
    //
    if (mAcpiBootPerformanceTable == NULL) {
      return Status;
    }

    //
    // Update ExitBootServicesExit for UEFI boot.
    //
    mAcpiBootPerformanceTable->BasicBoot.ExitBootServicesExit = GetTimeInNanoSecond (GetPerformanceCounter ());

    //
    // Unregister boot time report status code listener.
    //
    mRscHandlerProtocol->Unregister (FpdtStatusCodeListenerDxe);
  } else if (Data != NULL && CompareGuid (&Data->Type, &gEfiFirmwarePerformanceGuid)) {
    //
    // Append one or more Boot records
    //
    if (mAcpiBootPerformanceTable == NULL) {
      //
      // Append Boot records before FPDT ACPI table is installed. 
      //
      if (mBootRecordSize + Data->Size > mBootRecordMaxSize) {
        mBootRecordBuffer = ReallocatePool (mBootRecordSize, mBootRecordSize + Data->Size + EXTENSION_RECORD_SIZE, mBootRecordBuffer);
        ASSERT (mBootRecordBuffer != NULL);
        mBootRecordMaxSize = mBootRecordSize + Data->Size + EXTENSION_RECORD_SIZE;
      }
      //
      // Save boot record into the temp memory space.
      //
      CopyMem (mBootRecordBuffer + mBootRecordSize, Data + 1, Data->Size);
      mBootRecordSize += Data->Size;
    } else {
      //
      // Append Boot records after FPDT ACPI table is installed. 
      //
      if (mBootRecordSize + Data->Size > mBootRecordMaxSize) {
        //
        // No enough space to save boot record.
        //
        Status = EFI_OUT_OF_RESOURCES;
      } else {
        //
        // Save boot record into BootPerformance table
        //
        CopyMem (mBootRecordBuffer + mBootRecordSize, Data + 1, Data->Size);
        mBootRecordSize += Data->Size;
        mAcpiBootPerformanceTable->Header.Length = mBootRecordSize;
      }
    }
  } else {
    //
    // Ignore else progress code.
    //
    Status = EFI_UNSUPPORTED;
  }

  return Status;
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
  // Create ready to boot event to install ACPI FPDT table.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FpdtReadyToBootEventNotify,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &mReadyToBootEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Create legacy boot event to log OsLoaderStartImageStart for legacy boot.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FpdtLegacyBootEventNotify,
                  NULL,
                  &gEfiEventLegacyBootGuid,
                  &mLegacyBootEvent
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
    DEBUG ((EFI_D_ERROR, "FPDT: WARNING: SEC Performance Data Hob not found, ResetEnd will be set to 0!\n"));
  }

  return EFI_SUCCESS;
}
