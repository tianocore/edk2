/** @file
  OVMF ACPI Platform Driver

  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiPlatform.h"

EFI_STATUS
EFIAPI
InstallAcpiTable (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol,
  IN   VOID                          *AcpiTableBuffer,
  IN   UINTN                         AcpiTableBufferSize,
  OUT  UINTN                         *TableKey
  )
{
  return AcpiProtocol->InstallAcpiTable (
                         AcpiProtocol,
                         AcpiTableBuffer,
                         AcpiTableBufferSize,
                         TableKey
                         );
}


/**
  Locate the first instance of a protocol.  If the protocol requested is an
  FV protocol, then it will return the first FV that contains the ACPI table
  storage file.

  @param  Instance      Return pointer to the first instance of the protocol

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateFvInstanceWithTables (
  OUT EFI_FIRMWARE_VOLUME2_PROTOCOL **Instance
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         NumberOfHandles;
  EFI_FV_FILETYPE               FileType;
  UINT32                        FvStatus;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINTN                         Size;
  UINTN                         Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FvInstance;

  FvStatus = 0;

  //
  // Locate protocol.
  //
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiFirmwareVolume2ProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );
  if (EFI_ERROR (Status)) {
    //
    // Defined errors at this time are not found and out of resources.
    //
    return Status;
  }

  //
  // Looking for FV with ACPI storage file
  //
  for (Index = 0; Index < NumberOfHandles; Index++) {
    //
    // Get the protocol on this handle
    // This should not fail because of LocateHandleBuffer
    //
    Status = gBS->HandleProtocol (
                     HandleBuffer[Index],
                     &gEfiFirmwareVolume2ProtocolGuid,
                     (VOID**) &FvInstance
                     );
    ASSERT_EFI_ERROR (Status);

    //
    // See if it has the ACPI storage file
    //
    Status = FvInstance->ReadFile (
                           FvInstance,
                           (EFI_GUID*)PcdGetPtr (PcdAcpiTableStorageFile),
                           NULL,
                           &Size,
                           &FileType,
                           &Attributes,
                           &FvStatus
                           );

    //
    // If we found it, then we are done
    //
    if (Status == EFI_SUCCESS) {
      *Instance = FvInstance;
      break;
    }
  }

  //
  // Our exit status is determined by the success of the previous operations
  // If the protocol was found, Instance already points to it.
  //

  //
  // Free any allocated buffers
  //
  gBS->FreePool (HandleBuffer);

  return Status;
}


/**
  Find ACPI tables in an FV and install them.

  This is now a fall-back path. Normally, we will search for tables provided
  by the VMM first.

  If that fails, we use this function to load the ACPI tables from an FV. The
  sources for the FV based tables is located under OvmfPkg/AcpiTables.

  @param  AcpiTable     Protocol instance pointer

**/
EFI_STATUS
EFIAPI
InstallOvmfFvTables (
  IN  EFI_ACPI_TABLE_PROTOCOL     *AcpiTable
  )
{
  EFI_STATUS                           Status;
  EFI_FIRMWARE_VOLUME2_PROTOCOL        *FwVol;
  INTN                                 Instance;
  EFI_ACPI_COMMON_HEADER               *CurrentTable;
  UINTN                                TableHandle;
  UINT32                               FvStatus;
  UINTN                                TableSize;
  UINTN                                Size;
  EFI_ACPI_TABLE_INSTALL_ACPI_TABLE    TableInstallFunction;

  Instance     = 0;
  CurrentTable = NULL;
  TableHandle  = 0;

  if (QemuDetected ()) {
    TableInstallFunction = QemuInstallAcpiTable;
  } else {
    TableInstallFunction = InstallAcpiTable;
  }

  //
  // set FwVol (and use an ASSERT() below) to suppress incorrect
  // compiler/analyzer warnings
  //
  FwVol = NULL;
  //
  // Locate the firmware volume protocol
  //
  Status = LocateFvInstanceWithTables (&FwVol);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  ASSERT (FwVol != NULL);

  //
  // Read tables from the storage file.
  //
  while (Status == EFI_SUCCESS) {

    Status = FwVol->ReadSection (
                      FwVol,
                      (EFI_GUID*)PcdGetPtr (PcdAcpiTableStorageFile),
                      EFI_SECTION_RAW,
                      Instance,
                      (VOID**) &CurrentTable,
                      &Size,
                      &FvStatus
                      );
    if (!EFI_ERROR (Status)) {
      //
      // Add the table
      //
      TableHandle = 0;

      TableSize = ((EFI_ACPI_DESCRIPTION_HEADER *) CurrentTable)->Length;
      ASSERT (Size >= TableSize);

      //
      // Install ACPI table
      //
      Status = TableInstallFunction (
                 AcpiTable,
                 CurrentTable,
                 TableSize,
                 &TableHandle
                 );

      //
      // Free memory allocated by ReadSection
      //
      gBS->FreePool (CurrentTable);

      if (EFI_ERROR (Status)) {
        return EFI_ABORTED;
      }

      //
      // Increment the instance
      //
      Instance++;
      CurrentTable = NULL;
    }
  }

  return EFI_SUCCESS;
}

/**
  Effective entrypoint of Acpi Platform driver.

  @param  ImageHandle
  @param  SystemTable

  @return EFI_SUCCESS
  @return EFI_LOAD_ERROR
  @return EFI_OUT_OF_RESOURCES

**/
EFI_STATUS
EFIAPI
InstallAcpiTables (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiTable
  )
{
  EFI_STATUS                         Status;

  if (XenDetected ()) {
    Status = InstallXenTables (AcpiTable);
  } else {
    Status = InstallQemuFwCfgTables (AcpiTable);
  }

  if (EFI_ERROR (Status)) {
    Status = InstallOvmfFvTables (AcpiTable);
  }

  return Status;
}

