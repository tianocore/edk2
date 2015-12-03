/** @file
  Update SSDT table to ACPI table.

  Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials 
  are licensed and made available under the terms and conditions of the BSD License 
  which accompanies this distribution.  The full text of the license may be found at 
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SsdtUpdate.h"

FV_INPUT_DATA mInputData = {0};

/**
  Read file data from given file name.

  @param[in]  FileName    Pointer the readed given file name.
  @param[out] Buffer      The buffer which read the given file name's data.
  @param[out] BufferSize  The buffer size which read the given file name's data.

  @retval EFI_SUCCESS     The file data is successfully readed.
  @retval EFI_ERROR       The file data is unsuccessfully readed.

**/
STATIC
EFI_STATUS
ReadFileData (
  IN  CHAR16   *FileName,
  OUT UINT8    **Buffer,
  OUT UINT32   *BufferSize
  )
{
  EFI_STATUS             Status;
  SHELL_FILE_HANDLE      FileHandle;
  UINT64                 Size;
  VOID                   *NewBuffer;
  UINTN                  ReadSize;

  FileHandle = NULL;
  NewBuffer = NULL;
  Size = 0;

  Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = FileHandleIsDirectory (FileHandle);
  if (!EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  Status = FileHandleGetSize (FileHandle, &Size);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  NewBuffer = AllocatePool ((UINTN) Size);

  ReadSize = (UINTN) Size;
  Status = FileHandleRead (FileHandle, &ReadSize, NewBuffer);
  if (EFI_ERROR (Status)) {
    goto Done;
  } else if (ReadSize != (UINTN) Size) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

Done:
  if (FileHandle != NULL) {
    ShellCloseFile (&FileHandle);
  }

  if (EFI_ERROR (Status)) {
    if (NewBuffer != NULL) {
      FreePool (NewBuffer);
    }
  } else {
    *Buffer = NewBuffer;
    *BufferSize = (UINT32) Size;
  }

  return Status;
}

/**
  Initialize and publish device in ACPI table.

  @param[in] Table          The pointer to the ACPI table which will be published. 
  @param[in] TableSize      The size of ACPI table which will be published.

  @retval   EFI_SUCCESS     The ACPI table is published successfully.
  @retval   Others          The ACPI table is not published.

**/
EFI_STATUS
PublishAcpiTable (
  IN UINT8       *Table,
  IN UINT32      TableSize
  )
{
  EFI_STATUS                     Status;
  EFI_ACPI_TABLE_PROTOCOL        *AcpiTable;
  UINTN                          TableKey;

  //
  // Basic check ::TODO: Add check here!!!!!!!!!!!!!!!!!
  //
  //ASSERT (Table->OemTableId == SIGNATURE_64 ('E', 'v', 'e', 'r', 'T', 'a', 'b', 'l'));

  //
  // Publish the  ACPI table
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **) &AcpiTable);
  DEBUG((EFI_D_INFO, " Publish ACPI Table-3\n"));
  ASSERT_EFI_ERROR (Status);

  TableKey = 0;
  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_DESCRIPTION_HEADER*) Table,
                        TableSize,
                        &TableKey
                        );
  DEBUG((EFI_D_INFO, " Publish ACPI Table-4\n"));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Init device 
  
  @retval EFI_SUCCESS     Init Devices successfully
  @retval Others          Some error occurs 

**/
EFI_STATUS
InitDevice (
  )
{
  //
  // Add device Init here if needed
  //
  return EFI_SUCCESS;
}

/**
  UEFI application entry point which has an interface similar to a
  standard C main function.

  The ShellCEntryLib library instance wrappers the actual UEFI application
  entry point and calls this ShellAppMain function.

  @param[in] Argc        The number of items in Argv.
  @param[in] Argv        Array of pointers to strings.

  @retval
**/
INTN
EFIAPI
 ShellAppMain (
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{
  EFI_STATUS                     Status;
  UINT8                          *FileBuffer;
  UINT32                         TableSize;
  
  TableSize         = 0;
  FileBuffer        = NULL;

  //
  // Necessary device Initialization
  //
  Status = InitDevice();
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  StrCpy (mInputData.FileName, Argv[1]);
  Status = ReadFileData (mInputData.FileName, &FileBuffer, &TableSize);
  
  //
  // Update and publish ACPI table
  //
  Status = PublishAcpiTable (FileBuffer, TableSize);
  ASSERT_EFI_ERROR (Status);
  
  return Status;
  }

