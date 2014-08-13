/** @file
  File System Access for NvVarsFileLib

  Copyright (c) 2004 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "NvVarsFileLib.h"

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>


/**
  Open the NvVars file for reading or writing

  @param[in]  FsHandle - Handle for a gEfiSimpleFileSystemProtocolGuid instance
  @param[in]  ReadingFile - TRUE: open the file for reading.  FALSE: writing
  @param[out] NvVarsFile - If EFI_SUCCESS is returned, then this is updated
                           with the opened NvVars file.

  @return     EFI_SUCCESS if the file was opened

**/
EFI_STATUS
GetNvVarsFile (
  IN  EFI_HANDLE            FsHandle,
  IN  BOOLEAN               ReadingFile,
  OUT EFI_FILE_HANDLE       *NvVarsFile
  )
{
  EFI_STATUS                            Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL       *Fs;
  EFI_FILE_HANDLE                       Root;

  //
  // Get the FileSystem protocol on that handle
  //
  Status = gBS->HandleProtocol (
                  FsHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&Fs
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the volume (the root directory)
  //
  Status = Fs->OpenVolume (Fs, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Attempt to open the NvVars file in the root directory
  //
  Status = Root->Open (
                   Root,
                   NvVarsFile,
                   L"NvVars",
                   ReadingFile ?
                     EFI_FILE_MODE_READ :
                     (
                       EFI_FILE_MODE_CREATE |
                       EFI_FILE_MODE_READ |
                       EFI_FILE_MODE_WRITE
                     ),
                   0
                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}


/**
  Open the NvVars file for reading or writing

  @param[in]  File - The file to inspect
  @param[out] Exists - Returns whether the file exists
  @param[out] Size - Returns the size of the file
                     (0 if the file does not exist)

**/
VOID
NvVarsFileReadCheckup (
  IN  EFI_FILE_HANDLE        File,
  OUT BOOLEAN                *Exists,
  OUT UINTN                  *Size
  )
{
  EFI_FILE_INFO               *FileInfo;

  *Exists = FALSE;
  *Size = 0;

  FileInfo = FileHandleGetInfo (File);
  if (FileInfo == NULL) {
    return;
  }

  if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
    FreePool (FileInfo);
    return;
  }

  *Exists = TRUE;
  *Size = (UINTN) FileInfo->FileSize;

  FreePool (FileInfo);
}


/**
  Open the NvVars file for reading or writing

  @param[in]  File - The file to inspect
  @param[out] Exists - Returns whether the file exists
  @param[out] Size - Returns the size of the file
                     (0 if the file does not exist)

**/
EFI_STATUS
FileHandleEmpty (
  IN  EFI_FILE_HANDLE        File
  )
{
  EFI_STATUS                  Status;
  EFI_FILE_INFO               *FileInfo;

  //
  // Retrieve the FileInfo structure
  //
  FileInfo = FileHandleGetInfo (File);
  if (FileInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If the path is a directory, then return an error
  //
  if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
    FreePool (FileInfo);
    return EFI_INVALID_PARAMETER;
  }

  //
  // If the file size is already 0, then it is empty, so
  // we can return success.
  //
  if (FileInfo->FileSize == 0) {
    FreePool (FileInfo);
    return EFI_SUCCESS;
  }

  //
  // Set the file size to 0.
  //
  FileInfo->FileSize = 0;
  Status = FileHandleSetInfo (File, FileInfo);

  FreePool (FileInfo);

  return Status;
}


/**
  Reads a file to a newly allocated buffer

  @param[in]  File - The file to read
  @param[in]  ReadSize - The size of data to read from the file

  @return     Pointer to buffer allocated to hold the file
              contents.  NULL if an error occured.

**/
VOID*
FileHandleReadToNewBuffer (
  IN EFI_FILE_HANDLE            FileHandle,
  IN UINTN                      ReadSize
  )
{
  EFI_STATUS                  Status;
  UINTN                       ActualReadSize;
  VOID                        *FileContents;

  ActualReadSize = ReadSize;
  FileContents = AllocatePool (ReadSize);
  if (FileContents != NULL) {
    Status = FileHandleRead (
               FileHandle,
               &ReadSize,
               FileContents
               );
    if (EFI_ERROR (Status) || (ActualReadSize != ReadSize)) {
      FreePool (FileContents);
      return NULL;
    }
  }

  return FileContents;
}


/**
  Reads the contents of the NvVars file on the file system

  @param[in]  FsHandle - Handle for a gEfiSimpleFileSystemProtocolGuid instance

  @return     EFI_STATUS based on the success or failure of the file read

**/
EFI_STATUS
ReadNvVarsFile (
  IN  EFI_HANDLE            FsHandle
  )
{
  EFI_STATUS                  Status;
  EFI_FILE_HANDLE             File;
  UINTN                       FileSize;
  BOOLEAN                     FileExists;
  VOID                        *FileContents;
  EFI_HANDLE                  SerializedVariables;

  Status = GetNvVarsFile (FsHandle, TRUE, &File);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "FsAccess.c: Could not open NV Variables file on this file system\n"));
    return Status;
  }

  NvVarsFileReadCheckup (File, &FileExists, &FileSize);
  if (FileSize == 0) {
    FileHandleClose (File);
    return EFI_UNSUPPORTED;
  }

  FileContents = FileHandleReadToNewBuffer (File, FileSize);
  if (FileContents == NULL) {
    FileHandleClose (File);
    return EFI_UNSUPPORTED;
  }

  DEBUG ((
    EFI_D_INFO,
    "FsAccess.c: Read %d bytes from NV Variables file\n",
    FileSize
    ));

  Status = SerializeVariablesNewInstanceFromBuffer (
             &SerializedVariables,
             FileContents,
             FileSize
             );
  if (!RETURN_ERROR (Status)) {
    Status = SerializeVariablesSetSerializedVariables (SerializedVariables);
  }

  FreePool (FileContents);
  FileHandleClose (File);

  return Status;
}


/**
  Writes a variable to indicate that the NV variables
  have been loaded from the file system.

**/
STATIC
VOID
SetNvVarsVariable (
  VOID
  )
{
  BOOLEAN                        VarData;
  UINTN                          Size;

  //
  // Write a variable to indicate we've already loaded the
  // variable data.  If it is found, we skip the loading on
  // subsequent attempts.
  //
  Size = sizeof (VarData);
  VarData = TRUE;
  gRT->SetVariable (
         L"NvVars",
         &gEfiSimpleFileSystemProtocolGuid,
         EFI_VARIABLE_NON_VOLATILE |
           EFI_VARIABLE_BOOTSERVICE_ACCESS |
           EFI_VARIABLE_RUNTIME_ACCESS,
         Size,
         (VOID*) &VarData
         );
}


/**
  Loads the non-volatile variables from the NvVars file on the
  given file system.

  @param[in]  FsHandle - Handle for a gEfiSimpleFileSystemProtocolGuid instance

  @return     EFI_STATUS based on the success or failure of load operation

**/
EFI_STATUS
LoadNvVarsFromFs (
  EFI_HANDLE                            FsHandle
  )
{
  EFI_STATUS                     Status;
  BOOLEAN                        VarData;
  UINTN                          Size;

  DEBUG ((EFI_D_INFO, "FsAccess.c: LoadNvVarsFromFs\n"));

  //
  // We write a variable to indicate we've already loaded the
  // variable data.  If it is found, we skip the loading.
  //
  // This is relevent if the non-volatile variable have been
  // able to survive a reboot operation.  In that case, we don't
  // want to re-load the file as it would overwrite newer changes
  // made to the variables.
  //
  Size = sizeof (VarData);
  VarData = TRUE;
  Status = gRT->GetVariable (
                  L"NvVars",
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &Size,
                  (VOID*) &VarData
                  );
  if (Status == EFI_SUCCESS) {
    DEBUG ((EFI_D_INFO, "NV Variables were already loaded\n"));
    return EFI_ALREADY_STARTED;
  }

  //
  // Attempt to restore the variables from the NvVars file.
  //
  Status = ReadNvVarsFile (FsHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "Error while restoring NV variable data\n"));
    return Status;
  }

  //
  // Write a variable to indicate we've already loaded the
  // variable data.  If it is found, we skip the loading on
  // subsequent attempts.
  //
  SetNvVarsVariable();

  DEBUG ((
    EFI_D_INFO,
    "FsAccess.c: Read NV Variables file (size=%d)\n",
    Size
    ));

  return Status;
}


STATIC
RETURN_STATUS
EFIAPI
IterateVariablesCallbackAddAllNvVariables (
  IN  VOID                         *Context,
  IN  CHAR16                       *VariableName,
  IN  EFI_GUID                     *VendorGuid,
  IN  UINT32                       Attributes,
  IN  UINTN                        DataSize,
  IN  VOID                         *Data
  )
{
  EFI_HANDLE  Instance;

  Instance = (EFI_HANDLE) Context;

  //
  // Only save non-volatile variables
  //
  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
    return RETURN_SUCCESS;
  }

  return SerializeVariablesAddVariable (
           Instance,
           VariableName,
           VendorGuid,
           Attributes,
           DataSize,
           Data
           );
}


/**
  Saves the non-volatile variables into the NvVars file on the
  given file system.

  @param[in]  FsHandle - Handle for a gEfiSimpleFileSystemProtocolGuid instance

  @return     EFI_STATUS based on the success or failure of load operation

**/
EFI_STATUS
SaveNvVarsToFs (
  EFI_HANDLE                            FsHandle
  )
{
  EFI_STATUS                  Status;
  EFI_FILE_HANDLE             File;
  UINTN                       WriteSize;
  UINTN                       VariableDataSize;
  VOID                        *VariableData;
  EFI_HANDLE                  SerializedVariables;

  SerializedVariables = NULL;

  Status = SerializeVariablesNewInstance (&SerializedVariables);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SerializeVariablesIterateSystemVariables (
             IterateVariablesCallbackAddAllNvVariables,
             (VOID*) SerializedVariables
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  VariableData = NULL;
  VariableDataSize = 0;
  Status = SerializeVariablesToBuffer (
             SerializedVariables,
             NULL,
             &VariableDataSize
             );
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    VariableData = AllocatePool (VariableDataSize);
    if (VariableData == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      Status = SerializeVariablesToBuffer (
                 SerializedVariables,
                 VariableData,
                 &VariableDataSize
                 );
    }
  }

  SerializeVariablesFreeInstance (SerializedVariables);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the NvVars file for writing.
  //
  Status = GetNvVarsFile (FsHandle, FALSE, &File);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "FsAccess.c: Unable to open file to saved NV Variables\n"));
    return Status;
  }

  //
  // Empty the starting file contents.
  //
  Status = FileHandleEmpty (File);
  if (EFI_ERROR (Status)) {
    FileHandleClose (File);
    return Status;
  }

  WriteSize = VariableDataSize;
  Status = FileHandleWrite (File, &WriteSize, VariableData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FileHandleClose (File);

  if (!EFI_ERROR (Status)) {
    //
    // Write a variable to indicate we've already loaded the
    // variable data.  If it is found, we skip the loading on
    // subsequent attempts.
    //
    SetNvVarsVariable();

    DEBUG ((EFI_D_INFO, "Saved NV Variables to NvVars file\n"));
  }

  return Status;

}


