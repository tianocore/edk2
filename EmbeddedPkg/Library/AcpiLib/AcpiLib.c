/** @file
*
*  Copyright (c) 2014-2015, ARM Limited. All rights reserved.
*  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>

#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/FirmwareVolume2.h>

#include <IndustryStandard/Acpi.h>

/**
  Locate and Install the ACPI tables from the Firmware Volume if it verifies
  the function condition.

  @param  AcpiFile                Guid of the ACPI file into the Firmware Volume
  @param  CheckAcpiTableFunction  Function that checks if the ACPI table should be installed

  @return EFI_SUCCESS             The function completed successfully.
  @return EFI_NOT_FOUND           The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES    There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateAndInstallAcpiFromFvConditional (
  IN CONST EFI_GUID         *AcpiFile,
  IN EFI_LOCATE_ACPI_CHECK  CheckAcpiTableFunction
  )
{
  EFI_STATUS                     Status;
  EFI_ACPI_TABLE_PROTOCOL        *AcpiProtocol;
  EFI_HANDLE                     *HandleBuffer;
  UINTN                          NumberOfHandles;
  UINT32                         FvStatus;
  UINTN                          Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FvInstance;
  INTN                           SectionInstance;
  UINTN                          SectionSize;
  EFI_ACPI_COMMON_HEADER         *AcpiTable;
  UINTN                          AcpiTableSize;
  UINTN                          AcpiTableKey;
  BOOLEAN                        Valid;

  // Ensure the ACPI Table is present
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FvStatus        = 0;
  SectionInstance = 0;

  // Locate all the Firmware Volume protocols.
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Looking for FV with ACPI storage file
  for (Index = 0; Index < NumberOfHandles; Index++) {
    //
    // Get the protocol on this handle
    // This should not fail because of LocateHandleBuffer
    //
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **)&FvInstance
                    );
    if (EFI_ERROR (Status)) {
      goto FREE_HANDLE_BUFFER;
    }

    while (Status == EFI_SUCCESS) {
      // AcpiTable must be allocated by ReadSection (ie: AcpiTable == NULL)
      AcpiTable = NULL;

      // See if it has the ACPI storage file
      Status = FvInstance->ReadSection (
                             FvInstance,
                             AcpiFile,
                             EFI_SECTION_RAW,
                             SectionInstance,
                             (VOID **)&AcpiTable,
                             &SectionSize,
                             &FvStatus
                             );
      if (!EFI_ERROR (Status)) {
        AcpiTableKey  = 0;
        AcpiTableSize = ((EFI_ACPI_DESCRIPTION_HEADER *)AcpiTable)->Length;
        ASSERT (SectionSize >= AcpiTableSize);

        DEBUG ((
          DEBUG_ERROR,
          "- Found '%c%c%c%c' ACPI Table\n",
          (((EFI_ACPI_DESCRIPTION_HEADER *)AcpiTable)->Signature & 0xFF),
          ((((EFI_ACPI_DESCRIPTION_HEADER *)AcpiTable)->Signature >> 8) & 0xFF),
          ((((EFI_ACPI_DESCRIPTION_HEADER *)AcpiTable)->Signature >> 16) & 0xFF),
          ((((EFI_ACPI_DESCRIPTION_HEADER *)AcpiTable)->Signature >> 24) & 0xFF)
          ));

        // Is the ACPI table valid?
        if (CheckAcpiTableFunction) {
          Valid = CheckAcpiTableFunction ((EFI_ACPI_DESCRIPTION_HEADER *)AcpiTable);
        } else {
          Valid = TRUE;
        }

        // Install the ACPI Table
        if (Valid) {
          Status = AcpiProtocol->InstallAcpiTable (
                                   AcpiProtocol,
                                   AcpiTable,
                                   AcpiTableSize,
                                   &AcpiTableKey
                                   );
        }

        // Free memory allocated by ReadSection
        gBS->FreePool (AcpiTable);

        if (EFI_ERROR (Status)) {
          break;
        }

        // Increment the section instance
        SectionInstance++;
      }
    }
  }

FREE_HANDLE_BUFFER:
  //
  // Free any allocated buffers
  //
  gBS->FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

/**
  Locate and Install the ACPI tables from the Firmware Volume

  @param  AcpiFile              Guid of the ACPI file into the Firmware Volume

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateAndInstallAcpiFromFv (
  IN CONST EFI_GUID  *AcpiFile
  )
{
  return LocateAndInstallAcpiFromFvConditional (AcpiFile, NULL);
}

/**
  This function calculates and updates a UINT8 checksum
  in an ACPI description table header.

  @param  Buffer          Pointer to buffer to checksum
  @param  Size            Number of bytes to checksum

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.

**/
EFI_STATUS
EFIAPI
AcpiUpdateChecksum (
  IN OUT  UINT8      *Buffer,
  IN      UINTN      Size
  )
{
  UINTN ChecksumOffset;

  if (Buffer == NULL || Size == 0) {
    return EFI_INVALID_PARAMETER;
  }

  ChecksumOffset = OFFSET_OF (EFI_ACPI_DESCRIPTION_HEADER, Checksum);

  //
  // Set checksum to 0 first
  //
  Buffer[ChecksumOffset] = 0;

  //
  // Update checksum value
  //
  Buffer[ChecksumOffset] = CalculateCheckSum8 (Buffer, Size);

  return EFI_SUCCESS;
}

/**
  This function uses the ACPI SDT protocol to search an ACPI table
  with a given signature.

  @param  AcpiTableSdtProtocol    Pointer to ACPI SDT protocol.
  @param  TableSignature          ACPI table signature.
  @param  Index                   The zero-based index of the table where to search the table.
                                  The index will be updated to the next instance if the table
                                  is found with the matched TableSignature.
  @param  Table                   Pointer to the table.
  @param  TableKey                Pointer to the table key.

  @return EFI_SUCCESS             The function completed successfully.
  @return EFI_INVALID_PARAMETER   At least one of parameters is invalid.
  @retval EFI_NOT_FOUND           The requested index is too large and a table was not found.

**/
EFI_STATUS
EFIAPI
AcpiLocateTableBySignature (
  IN      EFI_ACPI_SDT_PROTOCOL           *AcpiSdtProtocol,
  IN      UINT32                          TableSignature,
  IN OUT  UINTN                           *Index,
  OUT     EFI_ACPI_DESCRIPTION_HEADER     **Table,
  OUT     UINTN                           *TableKey
  )
{
  EFI_STATUS              Status;
  EFI_ACPI_SDT_HEADER     *TempTable;
  EFI_ACPI_TABLE_VERSION  TableVersion;
  UINTN                   TableIndex;

  if (AcpiSdtProtocol == NULL
      || Table == NULL
      || TableKey == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;

  //
  // Search for ACPI Table with matching signature
  //
  TableVersion = 0;
  TableIndex = *Index;
  while (!EFI_ERROR (Status)) {
    Status = AcpiSdtProtocol->GetAcpiTable (
                                TableIndex,
                                &TempTable,
                                &TableVersion,
                                TableKey
                                );
    if (!EFI_ERROR (Status)) {
      TableIndex++;

      if (((EFI_ACPI_DESCRIPTION_HEADER *)TempTable)->Signature == TableSignature) {
        *Table = (EFI_ACPI_DESCRIPTION_HEADER *)TempTable;
        *Index = TableIndex;
        break;
      }
    }
  }

  return Status;
}

/**
  This function updates the integer value of an AML Object.

  @param  AcpiTableSdtProtocol    Pointer to ACPI SDT protocol.
  @param  TableHandle             Points to the table representing the starting point
                                  for the object path search.
  @param  AsciiObjectPath         Pointer to the ACPI path of the object being updated.
  @param  Value                   New value to write to the object.

  @return EFI_SUCCESS             The function completed successfully.
  @return EFI_INVALID_PARAMETER   At least one of parameters is invalid or the data type
                                  of the ACPI object is not an integer value.
  @retval EFI_NOT_FOUND           The object is not found with the given path.

**/
EFI_STATUS
EFIAPI
AcpiAmlObjectUpdateInteger (
  IN  EFI_ACPI_SDT_PROTOCOL           *AcpiSdtProtocol,
  IN  EFI_ACPI_HANDLE                 TableHandle,
  IN  CHAR8                           *AsciiObjectPath,
  IN  UINTN                           Value
  )
{
  EFI_STATUS            Status;
  EFI_ACPI_HANDLE       ObjectHandle;
  EFI_ACPI_HANDLE       DataHandle;
  EFI_ACPI_DATA_TYPE    DataType;
  UINT8                 *Buffer;
  UINTN                 BufferSize;
  UINTN                 DataSize;

  if (AcpiSdtProtocol == NULL || AsciiObjectPath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ObjectHandle = NULL;
  DataHandle = NULL;

  Status = AcpiSdtProtocol->FindPath (TableHandle, AsciiObjectPath, &ObjectHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AcpiSdtProtocol->GetOption (ObjectHandle, 0, &DataType, (VOID *)&Buffer, &BufferSize);
  if (EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_OPCODE);
  ASSERT (Buffer != NULL);

  if (Buffer[0] != AML_NAME_OP) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  //
  // Get handle of data object
  //
  Status = AcpiSdtProtocol->GetChild (ObjectHandle, &DataHandle);
  ASSERT_EFI_ERROR (Status);

  Status = AcpiSdtProtocol->GetOption (DataHandle, 0, &DataType, (VOID *)&Buffer, &BufferSize);
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_OPCODE);
  ASSERT (Buffer != NULL);

  if (Buffer[0] == AML_ZERO_OP || Buffer[0] == AML_ONE_OP) {
    Status = AcpiSdtProtocol->SetOption (DataHandle, 0, (VOID *)&Value, sizeof (UINT8));
    ASSERT_EFI_ERROR (Status);
  } else {
    //
    // Check the size of data object
    //
    switch (Buffer[0]) {
    case AML_BYTE_PREFIX:
      DataSize = sizeof (UINT8);
      break;

    case AML_WORD_PREFIX:
      DataSize = sizeof (UINT16);
      break;

    case AML_DWORD_PREFIX:
      DataSize = sizeof (UINT32);
      break;

    case AML_QWORD_PREFIX:
      DataSize = sizeof (UINT64);
      break;

    default:
      // The data type of the ACPI object is not an integer
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    Status = AcpiSdtProtocol->SetOption (DataHandle, 1, (VOID *)&Value, DataSize);
    ASSERT_EFI_ERROR (Status);
  }

Exit:
  AcpiSdtProtocol->Close (DataHandle);
  AcpiSdtProtocol->Close (ObjectHandle);

  return Status;
}
