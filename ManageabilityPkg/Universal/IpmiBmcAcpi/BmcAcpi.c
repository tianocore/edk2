/** @file
  IPMI BMC ACPI.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Statements that include other header files
//
#include <PiDxe.h>

#include <IndustryStandard/Acpi.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/AcpiTable.h>

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>

#include <Library/ManageabilityTransportHelperLib.h>

#ifndef EFI_ACPI_CREATOR_ID
#define EFI_ACPI_CREATOR_ID  SIGNATURE_32 ('M', 'S', 'F', 'T')
#endif
#ifndef EFI_ACPI_CREATOR_REVISION
#define EFI_ACPI_CREATOR_REVISION  0x0100000D
#endif

/**

  Locate the first instance of a protocol.  If the protocol requested is an
  FV protocol, then it will return the first FV that contains the ACPI table
  storage file.

  @param [in] Protocol  The protocol to find.
  @param [in] Instance  Return pointer to the first instance of the protocol.
  @param [in] Type      The type of protocol to locate.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The protocol could not be located.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateSupportProtocol (
  IN   EFI_GUID  *Protocol,
  OUT  VOID      **Instance,
  IN   UINT32    Type
  )
{
  EFI_STATUS              Status;
  EFI_HANDLE              *HandleBuffer;
  UINTN                   NumberOfHandles;
  EFI_FV_FILETYPE         FileType;
  UINT32                  FvStatus = 0;
  EFI_FV_FILE_ATTRIBUTES  Attributes;
  UINTN                   Size;
  UINTN                   Index;

  Status = gBS->LocateHandleBuffer (ByProtocol, Protocol, NULL, &NumberOfHandles, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Looking for FV with ACPI storage file
  //
  for (Index = 0; Index < NumberOfHandles; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], Protocol, Instance);
    ASSERT (!EFI_ERROR (Status));

    if (!Type) {
      //
      // Not looking for the FV protocol, so find the first instance of the
      // protocol.  There should not be any errors because our handle buffer
      // should always contain at least one or LocateHandleBuffer would have
      // returned not found.
      //
      break;
    }

    //
    // See if it has the ACPI storage file
    //
    Status = ((EFI_FIRMWARE_VOLUME2_PROTOCOL *)(*Instance))->ReadFile (
                                                               *Instance,
                                                               &gEfiCallerIdGuid,
                                                               NULL,
                                                               &Size,
                                                               &FileType,
                                                               &Attributes,
                                                               &FvStatus
                                                               );

    //
    // If we found it, then we are done
    //
    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  gBS->FreePool (HandleBuffer);
  return Status;
}

/**
  Update ACPI SSDT for BMC IPMI KCS device

  @param [in] Table  Pointer to ACPI SSDT

  @retval EFI_SUCCESS  SSDT is updated according to PCD settings
**/
EFI_STATUS
UpdateDeviceSsdtTable (
  IN OUT EFI_ACPI_COMMON_HEADER  *Table
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  *TableHeader = NULL;
  UINT64                       TempOemTableId;
  UINT8                        *DataPtr;
  EFI_ACPI_IO_PORT_DESCRIPTOR  *IoRsc;

  TableHeader = (EFI_ACPI_DESCRIPTION_HEADER *)Table;

  //
  // Update the OEMID and OEM Table ID.
  //
  CopyMem (&TableHeader->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (TableHeader->OemId));
  TempOemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (&TableHeader->OemTableId, &TempOemTableId, sizeof (UINT64));
  TableHeader->CreatorId       = EFI_ACPI_CREATOR_ID;
  TableHeader->CreatorRevision = EFI_ACPI_CREATOR_REVISION;

  //
  // Update IO(Decode16, 0xCA2, 0xCA2, 0, 2)
  //
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "UpdateDeviceSsdtTable - IPMI\n"));
  for (DataPtr = (UINT8 *)(Table + 1);
       DataPtr < (UINT8 *)((UINT8 *)Table + Table->Length - 4);
       DataPtr++)
  {
    if (CompareMem (DataPtr, "_CRS", 4) == 0) {
      DataPtr += 4; // Skip _CRS
      ASSERT (*DataPtr == AML_BUFFER_OP);
      DataPtr++;  // Skip AML_BUFFER_OP
      ASSERT ((*DataPtr & (BIT7|BIT6)) == 0);
      DataPtr++;  // Skip PkgLength - 0xD
      ASSERT ((*DataPtr) == AML_BYTE_PREFIX);
      DataPtr++;  // Skip BufferSize OpCode
      DataPtr++;  // Skip BufferSize - 0xA
      IoRsc = (VOID *)DataPtr;
      ASSERT (IoRsc->Header.Bits.Type == ACPI_SMALL_ITEM_FLAG);
      ASSERT (IoRsc->Header.Bits.Name == ACPI_SMALL_IO_PORT_DESCRIPTOR_NAME);
      ASSERT (IoRsc->Header.Bits.Length == sizeof (EFI_ACPI_IO_PORT_DESCRIPTOR) - sizeof (ACPI_SMALL_RESOURCE_HEADER));
      DEBUG ((DEBUG_MANAGEABILITY_INFO, "IPMI IO Base in ASL update - 0x%04x <= 0x%04x\n", IoRsc->BaseAddressMin, PcdGet16 (PcdIpmiKcsIoBaseAddress)));
      IoRsc->BaseAddressMin = PcdGet16 (PcdIpmiKcsIoBaseAddress);
      IoRsc->BaseAddressMax = PcdGet16 (PcdIpmiKcsIoBaseAddress);
    }
  }

  return EFI_SUCCESS;
}

/**

  Entry point for Acpi platform driver.

  @param [in] ImageHandle  A handle for the image that is initializing this driver.
  @param [in] SystemTable  A pointer to the EFI system table.

  @retval EFI_SUCCESS           Driver initialized successfully.
  @retval EFI_LOAD_ERROR        Failed to Initialize or has been loaded.
  @retval EFI_OUT_OF_RESOURCES  Could not allocate needed resources.

**/
EFI_STATUS
EFIAPI
BmcAcpiEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  AcpiStatus;

  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FwVol;
  INTN                           Instance      = 0;
  EFI_ACPI_COMMON_HEADER         *CurrentTable = NULL;
  UINTN                          TableHandle   = 0;
  UINT32                         FvStatus;
  UINT32                         Size;

  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;
  UINTN                    TableSize;

  //
  // Find the AcpiTable protocol
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  //
  // Locate the firmware volume protocol
  //
  Status = LocateSupportProtocol (&gEfiFirmwareVolume2ProtocolGuid, (VOID **)&FwVol, 1);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status   = EFI_SUCCESS;
  Instance = 0;

  //
  // Read tables from the storage file.
  //
  while (!EFI_ERROR (Status)) {
    CurrentTable = NULL;

    Status = FwVol->ReadSection (
                      FwVol,
                      &gEfiCallerIdGuid,
                      EFI_SECTION_RAW,
                      Instance,
                      (VOID **)&CurrentTable,
                      (UINTN *)&Size,
                      &FvStatus
                      );
    if (!EFI_ERROR (Status)) {
      //
      // Perform any table specific updates.
      //
      AcpiStatus = UpdateDeviceSsdtTable (CurrentTable);
      if (!EFI_ERROR (AcpiStatus)) {
        TableHandle = 0;
        TableSize   = ((EFI_ACPI_DESCRIPTION_HEADER *)CurrentTable)->Length;
        ASSERT (Size >= TableSize);

        Status = AcpiTable->InstallAcpiTable (
                              AcpiTable,
                              CurrentTable,
                              TableSize,
                              &TableHandle
                              );

        ASSERT_EFI_ERROR (Status);
      }

      //
      // Increment the instance
      //
      Instance++;
    }
  }

  return EFI_SUCCESS;
}
