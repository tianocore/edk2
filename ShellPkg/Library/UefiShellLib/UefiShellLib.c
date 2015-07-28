/** @file
  Provides interface to shell functionality for shell commands and applications.

  Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLib.h"
#include <ShellBase.h>
#include <Library/SortLib.h>
#include <Library/BaseLib.h>

#define FIND_XXXXX_FILE_BUFFER_SIZE (SIZE_OF_EFI_FILE_INFO + MAX_FILE_NAME_LEN)

//
// globals...
//
SHELL_PARAM_ITEM EmptyParamList[] = {
  {NULL, TypeMax}
  };
SHELL_PARAM_ITEM SfoParamList[] = {
  {L"-sfo", TypeFlag},
  {NULL, TypeMax}
  };
EFI_SHELL_ENVIRONMENT2        *mEfiShellEnvironment2;
EFI_SHELL_INTERFACE           *mEfiShellInterface;
EFI_SHELL_PROTOCOL            *gEfiShellProtocol;
EFI_SHELL_PARAMETERS_PROTOCOL *gEfiShellParametersProtocol;
EFI_HANDLE                    mEfiShellEnvironment2Handle;
FILE_HANDLE_FUNCTION_MAP      FileFunctionMap;

/**
  Check if a Unicode character is a hexadecimal character.

  This internal function checks if a Unicode character is a
  numeric character.  The valid hexadecimal characters are
  L'0' to L'9', L'a' to L'f', or L'A' to L'F'.

  @param  Char  The character to check against.

  @retval TRUE  If the Char is a hexadecmial character.
  @retval FALSE If the Char is not a hexadecmial character.

**/
BOOLEAN
EFIAPI
ShellIsHexaDecimalDigitCharacter (
  IN      CHAR16                    Char
  )
{
  return (BOOLEAN) ((Char >= L'0' && Char <= L'9') || (Char >= L'A' && Char <= L'F') || (Char >= L'a' && Char <= L'f'));
}

/**
  Check if a Unicode character is a decimal character.

  This internal function checks if a Unicode character is a
  decimal character.  The valid characters are
  L'0' to L'9'.


  @param  Char  The character to check against.

  @retval TRUE  If the Char is a hexadecmial character.
  @retval FALSE If the Char is not a hexadecmial character.

**/
BOOLEAN
EFIAPI
ShellIsDecimalDigitCharacter (
  IN      CHAR16                    Char
  )
{
  return (BOOLEAN) (Char >= L'0' && Char <= L'9');
}

/**
  Helper function to find ShellEnvironment2 for constructor.

  @param[in] ImageHandle    A copy of the calling image's handle.

  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
EFI_STATUS
EFIAPI
ShellFindSE2 (
  IN EFI_HANDLE        ImageHandle
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *Buffer;
  UINTN       BufferSize;
  UINTN       HandleIndex;

  BufferSize = 0;
  Buffer = NULL;
  Status = gBS->OpenProtocol(ImageHandle,
                             &gEfiShellEnvironment2Guid,
                             (VOID **)&mEfiShellEnvironment2,
                             ImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_GET_PROTOCOL
                            );
  //
  // look for the mEfiShellEnvironment2 protocol at a higher level
  //
  if (EFI_ERROR (Status) || !(CompareGuid (&mEfiShellEnvironment2->SESGuid, &gEfiShellEnvironment2ExtGuid))){
    //
    // figure out how big of a buffer we need.
    //
    Status = gBS->LocateHandle (ByProtocol,
                                &gEfiShellEnvironment2Guid,
                                NULL, // ignored for ByProtocol
                                &BufferSize,
                                Buffer
                               );
    //
    // maybe it's not there???
    //
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Buffer = (EFI_HANDLE*)AllocateZeroPool(BufferSize);
      if (Buffer == NULL) {
        return (EFI_OUT_OF_RESOURCES);
      }
      Status = gBS->LocateHandle (ByProtocol,
                                  &gEfiShellEnvironment2Guid,
                                  NULL, // ignored for ByProtocol
                                  &BufferSize,
                                  Buffer
                                 );
    }
    if (!EFI_ERROR (Status) && Buffer != NULL) {
      //
      // now parse the list of returned handles
      //
      Status = EFI_NOT_FOUND;
      for (HandleIndex = 0; HandleIndex < (BufferSize/sizeof(Buffer[0])); HandleIndex++) {
        Status = gBS->OpenProtocol(Buffer[HandleIndex],
                                   &gEfiShellEnvironment2Guid,
                                   (VOID **)&mEfiShellEnvironment2,
                                   ImageHandle,
                                   NULL,
                                   EFI_OPEN_PROTOCOL_GET_PROTOCOL
                                  );
         if (CompareGuid (&mEfiShellEnvironment2->SESGuid, &gEfiShellEnvironment2ExtGuid)) {
          mEfiShellEnvironment2Handle = Buffer[HandleIndex];
          Status = EFI_SUCCESS;
          break;
        }
      }
    }
  }
  if (Buffer != NULL) {
    FreePool (Buffer);
  }
  return (Status);
}

/**
  Function to do most of the work of the constructor.  Allows for calling
  multiple times without complete re-initialization.

  @param[in] ImageHandle  A copy of the ImageHandle.
  @param[in] SystemTable  A pointer to the SystemTable for the application.

  @retval EFI_SUCCESS   The operationw as successful.
**/
EFI_STATUS
EFIAPI
ShellLibConstructorWorker (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // UEFI 2.0 shell interfaces (used preferentially)
  //
  Status = gBS->OpenProtocol(
    ImageHandle,
    &gEfiShellProtocolGuid,
    (VOID **)&gEfiShellProtocol,
    ImageHandle,
    NULL,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
   );
  if (EFI_ERROR(Status)) {
    //
    // Search for the shell protocol
    //
    Status = gBS->LocateProtocol(
      &gEfiShellProtocolGuid,
      NULL,
      (VOID **)&gEfiShellProtocol
     );
    if (EFI_ERROR(Status)) {
      gEfiShellProtocol = NULL;
    }
  }
  Status = gBS->OpenProtocol(
    ImageHandle,
    &gEfiShellParametersProtocolGuid,
    (VOID **)&gEfiShellParametersProtocol,
    ImageHandle,
    NULL,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
   );
  if (EFI_ERROR(Status)) {
    gEfiShellParametersProtocol = NULL;
  }

  if (gEfiShellParametersProtocol == NULL || gEfiShellProtocol == NULL) {
    //
    // Moved to seperate function due to complexity
    //
    Status = ShellFindSE2(ImageHandle);

    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "Status: 0x%08x\r\n", Status));
      mEfiShellEnvironment2 = NULL;
    }
    Status = gBS->OpenProtocol(ImageHandle,
                               &gEfiShellInterfaceGuid,
                               (VOID **)&mEfiShellInterface,
                               ImageHandle,
                               NULL,
                               EFI_OPEN_PROTOCOL_GET_PROTOCOL
                              );
    if (EFI_ERROR(Status)) {
      mEfiShellInterface = NULL;
    }
  }

  //
  // only success getting 2 of either the old or new, but no 1/2 and 1/2
  //
  if ((mEfiShellEnvironment2 != NULL && mEfiShellInterface          != NULL) ||
      (gEfiShellProtocol     != NULL && gEfiShellParametersProtocol != NULL)   ) {
    if (gEfiShellProtocol != NULL) {
      FileFunctionMap.GetFileInfo     = gEfiShellProtocol->GetFileInfo;
      FileFunctionMap.SetFileInfo     = gEfiShellProtocol->SetFileInfo;
      FileFunctionMap.ReadFile        = gEfiShellProtocol->ReadFile;
      FileFunctionMap.WriteFile       = gEfiShellProtocol->WriteFile;
      FileFunctionMap.CloseFile       = gEfiShellProtocol->CloseFile;
      FileFunctionMap.DeleteFile      = gEfiShellProtocol->DeleteFile;
      FileFunctionMap.GetFilePosition = gEfiShellProtocol->GetFilePosition;
      FileFunctionMap.SetFilePosition = gEfiShellProtocol->SetFilePosition;
      FileFunctionMap.FlushFile       = gEfiShellProtocol->FlushFile;
      FileFunctionMap.GetFileSize     = gEfiShellProtocol->GetFileSize;
    } else {
      FileFunctionMap.GetFileInfo     = (EFI_SHELL_GET_FILE_INFO)FileHandleGetInfo;
      FileFunctionMap.SetFileInfo     = (EFI_SHELL_SET_FILE_INFO)FileHandleSetInfo;
      FileFunctionMap.ReadFile        = (EFI_SHELL_READ_FILE)FileHandleRead;
      FileFunctionMap.WriteFile       = (EFI_SHELL_WRITE_FILE)FileHandleWrite;
      FileFunctionMap.CloseFile       = (EFI_SHELL_CLOSE_FILE)FileHandleClose;
      FileFunctionMap.DeleteFile      = (EFI_SHELL_DELETE_FILE)FileHandleDelete;
      FileFunctionMap.GetFilePosition = (EFI_SHELL_GET_FILE_POSITION)FileHandleGetPosition;
      FileFunctionMap.SetFilePosition = (EFI_SHELL_SET_FILE_POSITION)FileHandleSetPosition;
      FileFunctionMap.FlushFile       = (EFI_SHELL_FLUSH_FILE)FileHandleFlush;
      FileFunctionMap.GetFileSize     = (EFI_SHELL_GET_FILE_SIZE)FileHandleGetSize;
    }
    return (EFI_SUCCESS);
  }
  return (EFI_NOT_FOUND);
}
/**
  Constructor for the Shell library.

  Initialize the library and determine if the underlying is a UEFI Shell 2.0 or an EFI shell.

  @param ImageHandle    the image handle of the process
  @param SystemTable    the EFI System Table pointer

  @retval EFI_SUCCESS   the initialization was complete sucessfully
  @return others        an error ocurred during initialization
**/
EFI_STATUS
EFIAPI
ShellLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mEfiShellEnvironment2       = NULL;
  gEfiShellProtocol           = NULL;
  gEfiShellParametersProtocol = NULL;
  mEfiShellInterface          = NULL;
  mEfiShellEnvironment2Handle = NULL;

  //
  // verify that auto initialize is not set false
  //
  if (PcdGetBool(PcdShellLibAutoInitialize) == 0) {
    return (EFI_SUCCESS);
  }

  return (ShellLibConstructorWorker(ImageHandle, SystemTable));
}

/**
  Destructor for the library.  free any resources.

  @param[in] ImageHandle  A copy of the ImageHandle.
  @param[in] SystemTable  A pointer to the SystemTable for the application.

  @retval EFI_SUCCESS   The operation was successful.
  @return               An error from the CloseProtocol function.
**/
EFI_STATUS
EFIAPI
ShellLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (mEfiShellEnvironment2 != NULL) {
    gBS->CloseProtocol(mEfiShellEnvironment2Handle==NULL?ImageHandle:mEfiShellEnvironment2Handle,
                       &gEfiShellEnvironment2Guid,
                       ImageHandle,
                       NULL);
    mEfiShellEnvironment2 = NULL;
  }
  if (mEfiShellInterface != NULL) {
    gBS->CloseProtocol(ImageHandle,
                       &gEfiShellInterfaceGuid,
                       ImageHandle,
                       NULL);
    mEfiShellInterface = NULL;
  }
  if (gEfiShellProtocol != NULL) {
    gBS->CloseProtocol(ImageHandle,
                       &gEfiShellProtocolGuid,
                       ImageHandle,
                       NULL);
    gEfiShellProtocol = NULL;
  }
  if (gEfiShellParametersProtocol != NULL) {
    gBS->CloseProtocol(ImageHandle,
                       &gEfiShellParametersProtocolGuid,
                       ImageHandle,
                       NULL);
    gEfiShellParametersProtocol = NULL;
  }
  mEfiShellEnvironment2Handle = NULL;

  return (EFI_SUCCESS);
}

/**
  This function causes the shell library to initialize itself.  If the shell library
  is already initialized it will de-initialize all the current protocol poitners and
  re-populate them again.

  When the library is used with PcdShellLibAutoInitialize set to true this function
  will return EFI_SUCCESS and perform no actions.

  This function is intended for internal access for shell commands only.

  @retval EFI_SUCCESS   the initialization was complete sucessfully

**/
EFI_STATUS
EFIAPI
ShellInitialize (
  )
{
  //
  // if auto initialize is not false then skip
  //
  if (PcdGetBool(PcdShellLibAutoInitialize) != 0) {
    return (EFI_SUCCESS);
  }

  //
  // deinit the current stuff
  //
  ASSERT_EFI_ERROR(ShellLibDestructor(gImageHandle, gST));

  //
  // init the new stuff
  //
  return (ShellLibConstructorWorker(gImageHandle, gST));
}

/**
  This function will retrieve the information about the file for the handle
  specified and store it in allocated pool memory.

  This function allocates a buffer to store the file's information. It is the
  caller's responsibility to free the buffer

  @param  FileHandle  The file handle of the file for which information is
  being requested.

  @retval NULL information could not be retrieved.

  @return the information about the file
**/
EFI_FILE_INFO*
EFIAPI
ShellGetFileInfo (
  IN SHELL_FILE_HANDLE                     FileHandle
  )
{
  return (FileFunctionMap.GetFileInfo(FileHandle));
}

/**
  This function sets the information about the file for the opened handle
  specified.

  @param[in]  FileHandle        The file handle of the file for which information
                                is being set.

  @param[in]  FileInfo          The information to set.

  @retval EFI_SUCCESS           The information was set.
  @retval EFI_INVALID_PARAMETER A parameter was out of range or invalid.
  @retval EFI_UNSUPPORTED       The FileHandle does not support FileInfo.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write protected.
  @retval EFI_ACCESS_DENIED     The file was opened read only.
  @retval EFI_VOLUME_FULL       The volume is full.
**/
EFI_STATUS
EFIAPI
ShellSetFileInfo (
  IN SHELL_FILE_HANDLE                    FileHandle,
  IN EFI_FILE_INFO              *FileInfo
  )
{
  return (FileFunctionMap.SetFileInfo(FileHandle, FileInfo));
}

  /**
  This function will open a file or directory referenced by DevicePath.

  This function opens a file with the open mode according to the file path. The
  Attributes is valid only for EFI_FILE_MODE_CREATE.

  @param  FilePath        on input the device path to the file.  On output
                          the remaining device path.
  @param  DeviceHandle    pointer to the system device handle.
  @param  FileHandle      pointer to the file handle.
  @param  OpenMode        the mode to open the file with.
  @param  Attributes      the file's file attributes.

  @retval EFI_SUCCESS           The information was set.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       Could not open the file path.
  @retval EFI_NOT_FOUND         The specified file could not be found on the
                                device or the file system could not be found on
                                the device.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_MEDIA_CHANGED     The device has a different medium in it or the
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write protected.
  @retval EFI_ACCESS_DENIED     The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES  Not enough resources were available to open the
                                file.
  @retval EFI_VOLUME_FULL       The volume is full.
**/
EFI_STATUS
EFIAPI
ShellOpenFileByDevicePath(
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **FilePath,
  OUT EFI_HANDLE                      *DeviceHandle,
  OUT SHELL_FILE_HANDLE               *FileHandle,
  IN UINT64                           OpenMode,
  IN UINT64                           Attributes
  )
{
  CHAR16                          *FileName;
  EFI_STATUS                      Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *EfiSimpleFileSystemProtocol;
  EFI_FILE_PROTOCOL               *Handle1;
  EFI_FILE_PROTOCOL               *Handle2;
  CHAR16                          *FnafPathName;
  UINTN                           PathLen;

  if (FilePath == NULL || FileHandle == NULL || DeviceHandle == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // which shell interface should we use
  //
  if (gEfiShellProtocol != NULL) {
    //
    // use UEFI Shell 2.0 method.
    //
    FileName = gEfiShellProtocol->GetFilePathFromDevicePath(*FilePath);
    if (FileName == NULL) {
      return (EFI_INVALID_PARAMETER);
    }
    Status = ShellOpenFileByName(FileName, FileHandle, OpenMode, Attributes);
    FreePool(FileName);
    return (Status);
  }


  //
  // use old shell method.
  //
  Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid,
                                  FilePath,
                                  DeviceHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = gBS->OpenProtocol(*DeviceHandle,
                             &gEfiSimpleFileSystemProtocolGuid,
                             (VOID**)&EfiSimpleFileSystemProtocol,
                             gImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = EfiSimpleFileSystemProtocol->OpenVolume(EfiSimpleFileSystemProtocol, &Handle1);
  if (EFI_ERROR (Status)) {
    FileHandle = NULL;
    return Status;
  }

  //
  // go down directories one node at a time.
  //
  while (!IsDevicePathEnd (*FilePath)) {
    //
    // For file system access each node should be a file path component
    //
    if (DevicePathType    (*FilePath) != MEDIA_DEVICE_PATH ||
        DevicePathSubType (*FilePath) != MEDIA_FILEPATH_DP
       ) {
      FileHandle = NULL;
      return (EFI_INVALID_PARAMETER);
    }
    //
    // Open this file path node
    //
    Handle2  = Handle1;
    Handle1 = NULL;

    //
    // File Name Alignment Fix (FNAF)
    // Handle2->Open may be incapable of handling a unaligned CHAR16 data.
    // The structure pointed to by FilePath may be not CHAR16 aligned.
    // This code copies the potentially unaligned PathName data from the
    // FilePath structure to the aligned FnafPathName for use in the
    // calls to Handl2->Open.
    //

    //
    // Determine length of PathName, in bytes.
    //
    PathLen = DevicePathNodeLength (*FilePath) - SIZE_OF_FILEPATH_DEVICE_PATH;

    //
    // Allocate memory for the aligned copy of the string Extra allocation is to allow for forced alignment
    // Copy bytes from possibly unaligned location to aligned location
    //
    FnafPathName = AllocateCopyPool(PathLen, (UINT8 *)((FILEPATH_DEVICE_PATH*)*FilePath)->PathName);
    if (FnafPathName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Try to test opening an existing file
    //
    Status = Handle2->Open (
                          Handle2,
                          &Handle1,
                          FnafPathName,
                          OpenMode &~EFI_FILE_MODE_CREATE,
                          0
                         );

    //
    // see if the error was that it needs to be created
    //
    if ((EFI_ERROR (Status)) && (OpenMode != (OpenMode &~EFI_FILE_MODE_CREATE))) {
      Status = Handle2->Open (
                            Handle2,
                            &Handle1,
                            FnafPathName,
                            OpenMode,
                            Attributes
                           );
    }

    //
    // Free the alignment buffer
    //
    FreePool(FnafPathName);

    //
    // Close the last node
    //
    Handle2->Close (Handle2);

    if (EFI_ERROR(Status)) {
      return (Status);
    }

    //
    // Get the next node
    //
    *FilePath = NextDevicePathNode (*FilePath);
  }

  //
  // This is a weak spot since if the undefined SHELL_FILE_HANDLE format changes this must change also!
  //
  *FileHandle = (VOID*)Handle1;
  return (EFI_SUCCESS);
}

/**
  This function will open a file or directory referenced by filename.

  If return is EFI_SUCCESS, the Filehandle is the opened file's handle;
  otherwise, the Filehandle is NULL. The Attributes is valid only for
  EFI_FILE_MODE_CREATE.

  if FileName is NULL then ASSERT()

  @param  FileName      pointer to file name
  @param  FileHandle    pointer to the file handle.
  @param  OpenMode      the mode to open the file with.
  @param  Attributes    the file's file attributes.

  @retval EFI_SUCCESS           The information was set.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       Could not open the file path.
  @retval EFI_NOT_FOUND         The specified file could not be found on the
                                device or the file system could not be found
                                on the device.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_MEDIA_CHANGED     The device has a different medium in it or the
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write protected.
  @retval EFI_ACCESS_DENIED     The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES  Not enough resources were available to open the
                                file.
  @retval EFI_VOLUME_FULL       The volume is full.
**/
EFI_STATUS
EFIAPI
ShellOpenFileByName(
  IN CONST CHAR16               *FileName,
  OUT SHELL_FILE_HANDLE         *FileHandle,
  IN UINT64                     OpenMode,
  IN UINT64                     Attributes
  )
{
  EFI_HANDLE                    DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL      *FilePath;
  EFI_STATUS                    Status;
  EFI_FILE_INFO                 *FileInfo;
  CHAR16                        *FileNameCopy;

  //
  // ASSERT if FileName is NULL
  //
  ASSERT(FileName != NULL);

  if (FileName == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  if (gEfiShellProtocol != NULL) {
    if ((OpenMode & EFI_FILE_MODE_CREATE) == EFI_FILE_MODE_CREATE) {

      //
      // Create only a directory
      //
      if ((Attributes & EFI_FILE_DIRECTORY) == EFI_FILE_DIRECTORY) {
        return ShellCreateDirectory(FileName, FileHandle);
      }

      //
      // Create the directory to create the file in
      //
      FileNameCopy = AllocateCopyPool (StrSize (FileName), FileName);
      if (FileName == NULL) {
        return (EFI_OUT_OF_RESOURCES);
      }
      PathCleanUpDirectories (FileNameCopy);
      if (PathRemoveLastItem (FileNameCopy)) {
        if (!EFI_ERROR(ShellCreateDirectory (FileNameCopy, FileHandle))) {
          ShellCloseFile (FileHandle);
        }
      }
      SHELL_FREE_NON_NULL (FileNameCopy);
    }

    //
    // Use UEFI Shell 2.0 method to create the file
    //
    Status = gEfiShellProtocol->OpenFileByName(FileName,
                                               FileHandle,
                                               OpenMode);
    if (StrCmp(FileName, L"NUL") != 0 && !EFI_ERROR(Status) && ((OpenMode & EFI_FILE_MODE_CREATE) != 0)){
      FileInfo = FileFunctionMap.GetFileInfo(*FileHandle);
      ASSERT(FileInfo != NULL);
      FileInfo->Attribute = Attributes;
      Status = FileFunctionMap.SetFileInfo(*FileHandle, FileInfo);
      FreePool(FileInfo);
    }
    return (Status);
  }
  //
  // Using EFI Shell version
  // this means convert name to path and call that function
  // since this will use EFI method again that will open it.
  //
  ASSERT(mEfiShellEnvironment2 != NULL);
  FilePath = mEfiShellEnvironment2->NameToPath ((CHAR16*)FileName);
  if (FilePath != NULL) {
    return (ShellOpenFileByDevicePath(&FilePath,
                                      &DeviceHandle,
                                      FileHandle,
                                      OpenMode,
                                      Attributes));
  }
  return (EFI_DEVICE_ERROR);
}
/**
  This function create a directory

  If return is EFI_SUCCESS, the Filehandle is the opened directory's handle;
  otherwise, the Filehandle is NULL. If the directory already existed, this
  function opens the existing directory.

  @param  DirectoryName   pointer to directory name
  @param  FileHandle      pointer to the file handle.

  @retval EFI_SUCCESS           The information was set.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       Could not open the file path.
  @retval EFI_NOT_FOUND         The specified file could not be found on the
                                device or the file system could not be found
                                on the device.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_MEDIA_CHANGED     The device has a different medium in it or the
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write protected.
  @retval EFI_ACCESS_DENIED     The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES  Not enough resources were available to open the
                                file.
  @retval EFI_VOLUME_FULL       The volume is full.
  @sa ShellOpenFileByName
**/
EFI_STATUS
EFIAPI
ShellCreateDirectory(
  IN CONST CHAR16             *DirectoryName,
  OUT SHELL_FILE_HANDLE                  *FileHandle
  )
{
  if (gEfiShellProtocol != NULL) {
    //
    // Use UEFI Shell 2.0 method
    //
    return (gEfiShellProtocol->CreateFile(DirectoryName,
                          EFI_FILE_DIRECTORY,
                          FileHandle
                         ));
  } else {
    return (ShellOpenFileByName(DirectoryName,
                                FileHandle,
                                EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                                EFI_FILE_DIRECTORY
                               ));
  }
}

/**
  This function reads information from an opened file.

  If FileHandle is not a directory, the function reads the requested number of
  bytes from the file at the file's current position and returns them in Buffer.
  If the read goes beyond the end of the file, the read length is truncated to the
  end of the file. The file's current position is increased by the number of bytes
  returned.  If FileHandle is a directory, the function reads the directory entry
  at the file's current position and returns the entry in Buffer. If the Buffer
  is not large enough to hold the current directory entry, then
  EFI_BUFFER_TOO_SMALL is returned and the current file position is not updated.
  BufferSize is set to be the size of the buffer needed to read the entry. On
  success, the current position is updated to the next directory entry. If there
  are no more directory entries, the read returns a zero-length buffer.
  EFI_FILE_INFO is the structure returned as the directory entry.

  @param FileHandle             the opened file handle
  @param BufferSize             on input the size of buffer in bytes.  on return
                                the number of bytes written.
  @param Buffer                 the buffer to put read data into.

  @retval EFI_SUCCESS           Data was read.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR  The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_BUFFER_TO_SMALL Buffer is too small. ReadSize contains required
                                size.

**/
EFI_STATUS
EFIAPI
ShellReadFile(
  IN SHELL_FILE_HANDLE                     FileHandle,
  IN OUT UINTN                  *BufferSize,
  OUT VOID                      *Buffer
  )
{
  return (FileFunctionMap.ReadFile(FileHandle, BufferSize, Buffer));
}


/**
  Write data to a file.

  This function writes the specified number of bytes to the file at the current
  file position. The current file position is advanced the actual number of bytes
  written, which is returned in BufferSize. Partial writes only occur when there
  has been a data error during the write attempt (such as "volume space full").
  The file is automatically grown to hold the data if required. Direct writes to
  opened directories are not supported.

  @param FileHandle           The opened file for writing
  @param BufferSize           on input the number of bytes in Buffer.  On output
                              the number of bytes written.
  @param Buffer               the buffer containing data to write is stored.

 @retval EFI_SUCCESS          Data was written.
 @retval EFI_UNSUPPORTED      Writes to an open directory are not supported.
 @retval EFI_NO_MEDIA         The device has no media.
 @retval EFI_DEVICE_ERROR     The device reported an error.
 @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
 @retval EFI_WRITE_PROTECTED  The device is write-protected.
 @retval EFI_ACCESS_DENIED    The file was open for read only.
 @retval EFI_VOLUME_FULL      The volume is full.
**/
EFI_STATUS
EFIAPI
ShellWriteFile(
  IN SHELL_FILE_HANDLE          FileHandle,
  IN OUT UINTN                  *BufferSize,
  IN VOID                       *Buffer
  )
{
  return (FileFunctionMap.WriteFile(FileHandle, BufferSize, Buffer));
}

/**
  Close an open file handle.

  This function closes a specified file handle. All "dirty" cached file data is
  flushed to the device, and the file is closed. In all cases the handle is
  closed.

@param FileHandle               the file handle to close.

@retval EFI_SUCCESS             the file handle was closed sucessfully.
**/
EFI_STATUS
EFIAPI
ShellCloseFile (
  IN SHELL_FILE_HANDLE                     *FileHandle
  )
{
  return (FileFunctionMap.CloseFile(*FileHandle));
}

/**
  Delete a file and close the handle

  This function closes and deletes a file. In all cases the file handle is closed.
  If the file cannot be deleted, the warning code EFI_WARN_DELETE_FAILURE is
  returned, but the handle is still closed.

  @param FileHandle             the file handle to delete

  @retval EFI_SUCCESS           the file was closed sucessfully
  @retval EFI_WARN_DELETE_FAILURE the handle was closed, but the file was not
                                deleted
  @retval INVALID_PARAMETER     One of the parameters has an invalid value.
**/
EFI_STATUS
EFIAPI
ShellDeleteFile (
  IN SHELL_FILE_HANDLE            *FileHandle
  )
{
  return (FileFunctionMap.DeleteFile(*FileHandle));
}

/**
  Set the current position in a file.

  This function sets the current file position for the handle to the position
  supplied. With the exception of seeking to position 0xFFFFFFFFFFFFFFFF, only
  absolute positioning is supported, and seeking past the end of the file is
  allowed (a subsequent write would grow the file). Seeking to position
  0xFFFFFFFFFFFFFFFF causes the current position to be set to the end of the file.
  If FileHandle is a directory, the only position that may be set is zero. This
  has the effect of starting the read process of the directory entries over.

  @param FileHandle             The file handle on which the position is being set
  @param Position               Byte position from begining of file

  @retval EFI_SUCCESS           Operation completed sucessfully.
  @retval EFI_UNSUPPORTED       the seek request for non-zero is not valid on
                                directories.
  @retval INVALID_PARAMETER     One of the parameters has an invalid value.
**/
EFI_STATUS
EFIAPI
ShellSetFilePosition (
  IN SHELL_FILE_HANDLE              FileHandle,
  IN UINT64             Position
  )
{
  return (FileFunctionMap.SetFilePosition(FileHandle, Position));
}

/**
  Gets a file's current position

  This function retrieves the current file position for the file handle. For
  directories, the current file position has no meaning outside of the file
  system driver and as such the operation is not supported. An error is returned
  if FileHandle is a directory.

  @param FileHandle             The open file handle on which to get the position.
  @param Position               Byte position from begining of file.

  @retval EFI_SUCCESS           the operation completed sucessfully.
  @retval INVALID_PARAMETER     One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       the request is not valid on directories.
**/
EFI_STATUS
EFIAPI
ShellGetFilePosition (
  IN SHELL_FILE_HANDLE                     FileHandle,
  OUT UINT64                    *Position
  )
{
  return (FileFunctionMap.GetFilePosition(FileHandle, Position));
}
/**
  Flushes data on a file

  This function flushes all modified data associated with a file to a device.

  @param FileHandle             The file handle on which to flush data

  @retval EFI_SUCCESS           The data was flushed.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write protected.
  @retval EFI_ACCESS_DENIED     The file was opened for read only.
**/
EFI_STATUS
EFIAPI
ShellFlushFile (
  IN SHELL_FILE_HANDLE                     FileHandle
  )
{
  return (FileFunctionMap.FlushFile(FileHandle));
}

/** Retrieve first entry from a directory.

  This function takes an open directory handle and gets information from the
  first entry in the directory.  A buffer is allocated to contain
  the information and a pointer to the buffer is returned in *Buffer.  The
  caller can use ShellFindNextFile() to get subsequent directory entries.

  The buffer will be freed by ShellFindNextFile() when the last directory
  entry is read.  Otherwise, the caller must free the buffer, using FreePool,
  when finished with it.

  @param[in]  DirHandle         The file handle of the directory to search.
  @param[out] Buffer            The pointer to the buffer for the file's information.

  @retval EFI_SUCCESS           Found the first file.
  @retval EFI_NOT_FOUND         Cannot find the directory.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @return Others                status of ShellGetFileInfo, ShellSetFilePosition,
                                or ShellReadFile
**/
EFI_STATUS
EFIAPI
ShellFindFirstFile (
  IN SHELL_FILE_HANDLE                     DirHandle,
  OUT EFI_FILE_INFO             **Buffer
  )
{
  //
  // pass to file handle lib
  //
  return (FileHandleFindFirstFile(DirHandle, Buffer));
}
/** Retrieve next entries from a directory.

  To use this function, the caller must first call the ShellFindFirstFile()
  function to get the first directory entry.  Subsequent directory entries are
  retrieved by using the ShellFindNextFile() function.  This function can
  be called several times to get each entry from the directory.  If the call of
  ShellFindNextFile() retrieved the last directory entry, the next call of
  this function will set *NoFile to TRUE and free the buffer.

  @param[in]  DirHandle         The file handle of the directory.
  @param[out] Buffer            The pointer to buffer for file's information.
  @param[out] NoFile            The pointer to boolean when last file is found.

  @retval EFI_SUCCESS           Found the next file, or reached last file
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
**/
EFI_STATUS
EFIAPI
ShellFindNextFile(
  IN SHELL_FILE_HANDLE                      DirHandle,
  OUT EFI_FILE_INFO              *Buffer,
  OUT BOOLEAN                    *NoFile
  )
{
  //
  // pass to file handle lib
  //
  return (FileHandleFindNextFile(DirHandle, Buffer, NoFile));
}
/**
  Retrieve the size of a file.

  if FileHandle is NULL then ASSERT()
  if Size is NULL then ASSERT()

  This function extracts the file size info from the FileHandle's EFI_FILE_INFO
  data.

  @param FileHandle             file handle from which size is retrieved
  @param Size                   pointer to size

  @retval EFI_SUCCESS           operation was completed sucessfully
  @retval EFI_DEVICE_ERROR      cannot access the file
**/
EFI_STATUS
EFIAPI
ShellGetFileSize (
  IN SHELL_FILE_HANDLE                     FileHandle,
  OUT UINT64                    *Size
  )
{
  return (FileFunctionMap.GetFileSize(FileHandle, Size));
}
/**
  Retrieves the status of the break execution flag

  this function is useful to check whether the application is being asked to halt by the shell.

  @retval TRUE                  the execution break is enabled
  @retval FALSE                 the execution break is not enabled
**/
BOOLEAN
EFIAPI
ShellGetExecutionBreakFlag(
  VOID
  )
{
  //
  // Check for UEFI Shell 2.0 protocols
  //
  if (gEfiShellProtocol != NULL) {

    //
    // We are using UEFI Shell 2.0; see if the event has been triggered
    //
    if (gBS->CheckEvent(gEfiShellProtocol->ExecutionBreak) != EFI_SUCCESS) {
      return (FALSE);
    }
    return (TRUE);
  }

  //
  // using EFI Shell; call the function to check
  //
  if (mEfiShellEnvironment2 != NULL) {
    return (mEfiShellEnvironment2->GetExecutionBreak());
  }

  return (FALSE);
}
/**
  return the value of an environment variable

  this function gets the value of the environment variable set by the
  ShellSetEnvironmentVariable function

  @param EnvKey                 The key name of the environment variable.

  @retval NULL                  the named environment variable does not exist.
  @return != NULL               pointer to the value of the environment variable
**/
CONST CHAR16*
EFIAPI
ShellGetEnvironmentVariable (
  IN CONST CHAR16                *EnvKey
  )
{
  //
  // Check for UEFI Shell 2.0 protocols
  //
  if (gEfiShellProtocol != NULL) {
    return (gEfiShellProtocol->GetEnv(EnvKey));
  }

  //
  // Check for EFI shell
  //
  if (mEfiShellEnvironment2 != NULL) {
    return (mEfiShellEnvironment2->GetEnv((CHAR16*)EnvKey));
  }

  return NULL;
}
/**
  set the value of an environment variable

This function changes the current value of the specified environment variable. If the
environment variable exists and the Value is an empty string, then the environment
variable is deleted. If the environment variable exists and the Value is not an empty
string, then the value of the environment variable is changed. If the environment
variable does not exist and the Value is an empty string, there is no action. If the
environment variable does not exist and the Value is a non-empty string, then the
environment variable is created and assigned the specified value.

  This is not supported pre-UEFI Shell 2.0.

  @param EnvKey                 The key name of the environment variable.
  @param EnvVal                 The Value of the environment variable
  @param Volatile               Indicates whether the variable is non-volatile (FALSE) or volatile (TRUE).

  @retval EFI_SUCCESS           the operation was completed sucessfully
  @retval EFI_UNSUPPORTED       This operation is not allowed in pre UEFI 2.0 Shell environments
**/
EFI_STATUS
EFIAPI
ShellSetEnvironmentVariable (
  IN CONST CHAR16               *EnvKey,
  IN CONST CHAR16               *EnvVal,
  IN BOOLEAN                    Volatile
  )
{
  //
  // Check for UEFI Shell 2.0 protocols
  //
  if (gEfiShellProtocol != NULL) {
    return (gEfiShellProtocol->SetEnv(EnvKey, EnvVal, Volatile));
  }

  //
  // This feature does not exist under EFI shell
  //
  return (EFI_UNSUPPORTED);
}

/**
  Cause the shell to parse and execute a command line.

  This function creates a nested instance of the shell and executes the specified
  command (CommandLine) with the specified environment (Environment). Upon return,
  the status code returned by the specified command is placed in StatusCode.
  If Environment is NULL, then the current environment is used and all changes made
  by the commands executed will be reflected in the current environment. If the
  Environment is non-NULL, then the changes made will be discarded.
  The CommandLine is executed from the current working directory on the current
  device.

  The EnvironmentVariables pararemeter is ignored in a pre-UEFI Shell 2.0
  environment.  The values pointed to by the parameters will be unchanged by the
  ShellExecute() function.  The Output parameter has no effect in a
  UEFI Shell 2.0 environment.

  @param[in] ParentHandle         The parent image starting the operation.
  @param[in] CommandLine          The pointer to a NULL terminated command line.
  @param[in] Output               True to display debug output.  False to hide it.
  @param[in] EnvironmentVariables Optional pointer to array of environment variables
                                  in the form "x=y".  If NULL, the current set is used.
  @param[out] Status              The status of the run command line.

  @retval EFI_SUCCESS             The operation completed sucessfully.  Status
                                  contains the status code returned.
  @retval EFI_INVALID_PARAMETER   A parameter contains an invalid value.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
  @retval EFI_UNSUPPORTED         The operation is not allowed.
**/
EFI_STATUS
EFIAPI
ShellExecute (
  IN EFI_HANDLE                 *ParentHandle,
  IN CHAR16                     *CommandLine OPTIONAL,
  IN BOOLEAN                    Output OPTIONAL,
  IN CHAR16                     **EnvironmentVariables OPTIONAL,
  OUT EFI_STATUS                *Status OPTIONAL
  )
{
  EFI_STATUS                CmdStatus;
  //
  // Check for UEFI Shell 2.0 protocols
  //
  if (gEfiShellProtocol != NULL) {
    //
    // Call UEFI Shell 2.0 version (not using Output parameter)
    //
    return (gEfiShellProtocol->Execute(ParentHandle,
                                      CommandLine,
                                      EnvironmentVariables,
                                      Status));
  }

  //
  // Check for EFI shell
  //
  if (mEfiShellEnvironment2 != NULL) {
    //
    // Call EFI Shell version.
    // Due to oddity in the EFI shell we want to dereference the ParentHandle here
    //
    CmdStatus = (mEfiShellEnvironment2->Execute(*ParentHandle,
                                          CommandLine,
                                          Output));
    //
    // No Status output parameter so just use the returned status
    //
    if (Status != NULL) {
      *Status = CmdStatus;
    }
    //
    // If there was an error, we can't tell if it was from the command or from
    // the Execute() function, so we'll just assume the shell ran successfully
    // and the error came from the command.
    //
    return EFI_SUCCESS;
  }

  return (EFI_UNSUPPORTED);
}

/**
  Retreives the current directory path

  If the DeviceName is NULL, it returns the current device's current directory
  name. If the DeviceName is not NULL, it returns the current directory name
  on specified drive.

  @param DeviceName             the name of the drive to get directory on

  @retval NULL                  the directory does not exist
  @return != NULL               the directory
**/
CONST CHAR16*
EFIAPI
ShellGetCurrentDir (
  IN CHAR16                     * CONST DeviceName OPTIONAL
  )
{
  //
  // Check for UEFI Shell 2.0 protocols
  //
  if (gEfiShellProtocol != NULL) {
    return (gEfiShellProtocol->GetCurDir(DeviceName));
  }

  //
  // Check for EFI shell
  //
  if (mEfiShellEnvironment2 != NULL) {
    return (mEfiShellEnvironment2->CurDir(DeviceName));
  }

  return (NULL);
}
/**
  sets (enabled or disabled) the page break mode

  when page break mode is enabled the screen will stop scrolling
  and wait for operator input before scrolling a subsequent screen.

  @param CurrentState           TRUE to enable and FALSE to disable
**/
VOID
EFIAPI
ShellSetPageBreakMode (
  IN BOOLEAN                    CurrentState
  )
{
  //
  // check for enabling
  //
  if (CurrentState != 0x00) {
    //
    // check for UEFI Shell 2.0
    //
    if (gEfiShellProtocol != NULL) {
      //
      // Enable with UEFI 2.0 Shell
      //
      gEfiShellProtocol->EnablePageBreak();
      return;
    } else {
      //
      // Check for EFI shell
      //
      if (mEfiShellEnvironment2 != NULL) {
        //
        // Enable with EFI Shell
        //
        mEfiShellEnvironment2->EnablePageBreak (DEFAULT_INIT_ROW, DEFAULT_AUTO_LF);
        return;
      }
    }
  } else {
    //
    // check for UEFI Shell 2.0
    //
    if (gEfiShellProtocol != NULL) {
      //
      // Disable with UEFI 2.0 Shell
      //
      gEfiShellProtocol->DisablePageBreak();
      return;
    } else {
      //
      // Check for EFI shell
      //
      if (mEfiShellEnvironment2 != NULL) {
        //
        // Disable with EFI Shell
        //
        mEfiShellEnvironment2->DisablePageBreak ();
        return;
      }
    }
  }
}

///
/// version of EFI_SHELL_FILE_INFO struct, except has no CONST pointers.
/// This allows for the struct to be populated.
///
typedef struct {
  LIST_ENTRY Link;
  EFI_STATUS Status;
  CHAR16 *FullName;
  CHAR16 *FileName;
  SHELL_FILE_HANDLE          Handle;
  EFI_FILE_INFO *Info;
} EFI_SHELL_FILE_INFO_NO_CONST;

/**
  Converts a EFI shell list of structures to the coresponding UEFI Shell 2.0 type of list.

  if OldStyleFileList is NULL then ASSERT()

  this function will convert a SHELL_FILE_ARG based list into a callee allocated
  EFI_SHELL_FILE_INFO based list.  it is up to the caller to free the memory via
  the ShellCloseFileMetaArg function.

  @param[in] FileList           the EFI shell list type
  @param[in, out] ListHead      the list to add to

  @retval the resultant head of the double linked new format list;
**/
LIST_ENTRY*
EFIAPI
InternalShellConvertFileListType (
  IN LIST_ENTRY                 *FileList,
  IN OUT LIST_ENTRY             *ListHead
  )
{
  SHELL_FILE_ARG                *OldInfo;
  LIST_ENTRY                    *Link;
  EFI_SHELL_FILE_INFO_NO_CONST  *NewInfo;

  //
  // ASSERTs
  //
  ASSERT(FileList  != NULL);
  ASSERT(ListHead  != NULL);

  //
  // enumerate through each member of the old list and copy
  //
  for (Link = FileList->ForwardLink; Link != FileList; Link = Link->ForwardLink) {
    OldInfo = CR (Link, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
    ASSERT(OldInfo           != NULL);

    //
    // Skip ones that failed to open...
    //
    if (OldInfo->Status != EFI_SUCCESS) {
      continue;
    }

    //
    // make sure the old list was valid
    //
    ASSERT(OldInfo->Info     != NULL);
    ASSERT(OldInfo->FullName != NULL);
    ASSERT(OldInfo->FileName != NULL);

    //
    // allocate a new EFI_SHELL_FILE_INFO object
    //
    NewInfo               = AllocateZeroPool(sizeof(EFI_SHELL_FILE_INFO));
    if (NewInfo == NULL) {
      ShellCloseFileMetaArg((EFI_SHELL_FILE_INFO**)(&ListHead));
      ListHead = NULL;
      break;
    }

    //
    // copy the simple items
    //
    NewInfo->Handle       = OldInfo->Handle;
    NewInfo->Status       = OldInfo->Status;

    // old shell checks for 0 not NULL
    OldInfo->Handle = 0;

    //
    // allocate new space to copy strings and structure
    //
    NewInfo->FullName     = AllocateCopyPool(StrSize(OldInfo->FullName), OldInfo->FullName);
    NewInfo->FileName     = AllocateCopyPool(StrSize(OldInfo->FileName), OldInfo->FileName);
    NewInfo->Info         = AllocateCopyPool((UINTN)OldInfo->Info->Size, OldInfo->Info);

    //
    // make sure all the memory allocations were sucessful
    //
    if (NULL == NewInfo->FullName || NewInfo->FileName == NULL || NewInfo->Info == NULL) {
      //
      // Free the partially allocated new node
      //
      SHELL_FREE_NON_NULL(NewInfo->FullName);
      SHELL_FREE_NON_NULL(NewInfo->FileName);
      SHELL_FREE_NON_NULL(NewInfo->Info);
      SHELL_FREE_NON_NULL(NewInfo);

      //
      // Free the previously converted stuff
      //
      ShellCloseFileMetaArg((EFI_SHELL_FILE_INFO**)(&ListHead));
      ListHead = NULL;
      break;
    }

    //
    // add that to the list
    //
    InsertTailList(ListHead, &NewInfo->Link);
  }
  return (ListHead);
}
/**
  Opens a group of files based on a path.

  This function uses the Arg to open all the matching files. Each matched
  file has a SHELL_FILE_INFO structure to record the file information. These
  structures are placed on the list ListHead. Users can get the SHELL_FILE_INFO
  structures from ListHead to access each file. This function supports wildcards
  and will process '?' and '*' as such.  the list must be freed with a call to
  ShellCloseFileMetaArg().

  If you are NOT appending to an existing list *ListHead must be NULL.  If
  *ListHead is NULL then it must be callee freed.

  @param Arg                    pointer to path string
  @param OpenMode               mode to open files with
  @param ListHead               head of linked list of results

  @retval EFI_SUCCESS           the operation was sucessful and the list head
                                contains the list of opened files
  @return != EFI_SUCCESS        the operation failed

  @sa InternalShellConvertFileListType
**/
EFI_STATUS
EFIAPI
ShellOpenFileMetaArg (
  IN CHAR16                     *Arg,
  IN UINT64                     OpenMode,
  IN OUT EFI_SHELL_FILE_INFO    **ListHead
  )
{
  EFI_STATUS                    Status;
  LIST_ENTRY                    mOldStyleFileList;
  CHAR16                        *CleanFilePathStr;

  //
  // ASSERT that Arg and ListHead are not NULL
  //
  ASSERT(Arg      != NULL);
  ASSERT(ListHead != NULL);

  CleanFilePathStr = NULL;

  Status = InternalShellStripQuotes (Arg, &CleanFilePathStr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check for UEFI Shell 2.0 protocols
  //
  if (gEfiShellProtocol != NULL) {
    if (*ListHead == NULL) {
      *ListHead = (EFI_SHELL_FILE_INFO*)AllocateZeroPool(sizeof(EFI_SHELL_FILE_INFO));
      if (*ListHead == NULL) {
        FreePool(CleanFilePathStr);
        return (EFI_OUT_OF_RESOURCES);
      }
      InitializeListHead(&((*ListHead)->Link));
    }
    Status = gEfiShellProtocol->OpenFileList(CleanFilePathStr,
                                           OpenMode,
                                           ListHead);
    if (EFI_ERROR(Status)) {
      gEfiShellProtocol->RemoveDupInFileList(ListHead);
    } else {
      Status = gEfiShellProtocol->RemoveDupInFileList(ListHead);
    }
    if (*ListHead != NULL && IsListEmpty(&(*ListHead)->Link)) {
      FreePool(*ListHead);
      FreePool(CleanFilePathStr);
      *ListHead = NULL;
      return (EFI_NOT_FOUND);
    }
    FreePool(CleanFilePathStr);
    return (Status);
  }

  //
  // Check for EFI shell
  //
  if (mEfiShellEnvironment2 != NULL) {
    //
    // make sure the list head is initialized
    //
    InitializeListHead(&mOldStyleFileList);

    //
    // Get the EFI Shell list of files
    //
    Status = mEfiShellEnvironment2->FileMetaArg(CleanFilePathStr, &mOldStyleFileList);
    if (EFI_ERROR(Status)) {
      *ListHead = NULL;
      FreePool(CleanFilePathStr);
      return (Status);
    }

    if (*ListHead == NULL) {
      *ListHead = (EFI_SHELL_FILE_INFO    *)AllocateZeroPool(sizeof(EFI_SHELL_FILE_INFO));
      if (*ListHead == NULL) {
        FreePool(CleanFilePathStr);
        return (EFI_OUT_OF_RESOURCES);
      }
      InitializeListHead(&((*ListHead)->Link));
    }

    //
    // Convert that to equivalent of UEFI Shell 2.0 structure
    //
    InternalShellConvertFileListType(&mOldStyleFileList, &(*ListHead)->Link);

    //
    // Free the EFI Shell version that was converted.
    //
    mEfiShellEnvironment2->FreeFileList(&mOldStyleFileList);

    if ((*ListHead)->Link.ForwardLink == (*ListHead)->Link.BackLink && (*ListHead)->Link.BackLink == &((*ListHead)->Link)) {
      FreePool(*ListHead);
      *ListHead = NULL;
      Status = EFI_NOT_FOUND;
    }
    FreePool(CleanFilePathStr);
    return (Status);
  }

  FreePool(CleanFilePathStr);
  return (EFI_UNSUPPORTED);
}
/**
  Free the linked list returned from ShellOpenFileMetaArg.

  if ListHead is NULL then ASSERT().

  @param ListHead               the pointer to free.

  @retval EFI_SUCCESS           the operation was sucessful.
**/
EFI_STATUS
EFIAPI
ShellCloseFileMetaArg (
  IN OUT EFI_SHELL_FILE_INFO    **ListHead
  )
{
  LIST_ENTRY                    *Node;

  //
  // ASSERT that ListHead is not NULL
  //
  ASSERT(ListHead != NULL);

  //
  // Check for UEFI Shell 2.0 protocols
  //
  if (gEfiShellProtocol != NULL) {
    return (gEfiShellProtocol->FreeFileList(ListHead));
  } else if (mEfiShellEnvironment2 != NULL) {
    //
    // Since this is EFI Shell version we need to free our internally made copy
    // of the list
    //
    for ( Node = GetFirstNode(&(*ListHead)->Link)
        ; *ListHead != NULL && !IsListEmpty(&(*ListHead)->Link)
        ; Node = GetFirstNode(&(*ListHead)->Link)) {
      RemoveEntryList(Node);
      ((EFI_FILE_PROTOCOL*)((EFI_SHELL_FILE_INFO_NO_CONST*)Node)->Handle)->Close(((EFI_SHELL_FILE_INFO_NO_CONST*)Node)->Handle);
      FreePool(((EFI_SHELL_FILE_INFO_NO_CONST*)Node)->FullName);
      FreePool(((EFI_SHELL_FILE_INFO_NO_CONST*)Node)->FileName);
      FreePool(((EFI_SHELL_FILE_INFO_NO_CONST*)Node)->Info);
      FreePool((EFI_SHELL_FILE_INFO_NO_CONST*)Node);
    }
    SHELL_FREE_NON_NULL(*ListHead);
    return EFI_SUCCESS;
  }

  return (EFI_UNSUPPORTED);
}

/**
  Find a file by searching the CWD and then the path.

  If FileName is NULL then ASSERT.

  If the return value is not NULL then the memory must be caller freed.

  @param FileName               Filename string.

  @retval NULL                  the file was not found
  @return !NULL                 the full path to the file.
**/
CHAR16 *
EFIAPI
ShellFindFilePath (
  IN CONST CHAR16 *FileName
  )
{
  CONST CHAR16      *Path;
  SHELL_FILE_HANDLE Handle;
  EFI_STATUS        Status;
  CHAR16            *RetVal;
  CHAR16            *TestPath;
  CONST CHAR16      *Walker;
  UINTN             Size;
  CHAR16            *TempChar;

  RetVal = NULL;

  //
  // First make sure its not an absolute path.
  //
  Status = ShellOpenFileByName(FileName, &Handle, EFI_FILE_MODE_READ, 0);
  if (!EFI_ERROR(Status)){
    if (FileHandleIsDirectory(Handle) != EFI_SUCCESS) {
      ASSERT(RetVal == NULL);
      RetVal = StrnCatGrow(&RetVal, NULL, FileName, 0);
      ShellCloseFile(&Handle);
      return (RetVal);
    } else {
      ShellCloseFile(&Handle);
    }
  }

  Path = ShellGetEnvironmentVariable(L"cwd");
  if (Path != NULL) {
    Size = StrSize(Path);
    Size += StrSize(FileName);
    TestPath = AllocateZeroPool(Size);
    if (TestPath == NULL) {
      return (NULL);
    }
    StrCpyS(TestPath, Size/sizeof(CHAR16), Path);
    StrCatS(TestPath, Size/sizeof(CHAR16), FileName);
    Status = ShellOpenFileByName(TestPath, &Handle, EFI_FILE_MODE_READ, 0);
    if (!EFI_ERROR(Status)){
      if (FileHandleIsDirectory(Handle) != EFI_SUCCESS) {
        ASSERT(RetVal == NULL);
        RetVal = StrnCatGrow(&RetVal, NULL, TestPath, 0);
        ShellCloseFile(&Handle);
        FreePool(TestPath);
        return (RetVal);
      } else {
        ShellCloseFile(&Handle);
      }
    }
    FreePool(TestPath);
  }
  Path = ShellGetEnvironmentVariable(L"path");
  if (Path != NULL) {
    Size = StrSize(Path)+sizeof(CHAR16);
    Size += StrSize(FileName);
    TestPath = AllocateZeroPool(Size);
    if (TestPath == NULL) {
      return (NULL);
    }
    Walker = (CHAR16*)Path;
    do {
      CopyMem(TestPath, Walker, StrSize(Walker));
      if (TestPath != NULL) {
        TempChar = StrStr(TestPath, L";");
        if (TempChar != NULL) {
          *TempChar = CHAR_NULL;
        }
        if (TestPath[StrLen(TestPath)-1] != L'\\') {
          StrCatS(TestPath, Size/sizeof(CHAR16), L"\\");
        }
        if (FileName[0] == L'\\') {
          FileName++;
        }
        StrCatS(TestPath, Size/sizeof(CHAR16), FileName);
        if (StrStr(Walker, L";") != NULL) {
          Walker = StrStr(Walker, L";") + 1;
        } else {
          Walker = NULL;
        }
        Status = ShellOpenFileByName(TestPath, &Handle, EFI_FILE_MODE_READ, 0);
        if (!EFI_ERROR(Status)){
          if (FileHandleIsDirectory(Handle) != EFI_SUCCESS) {
            ASSERT(RetVal == NULL);
            RetVal = StrnCatGrow(&RetVal, NULL, TestPath, 0);
            ShellCloseFile(&Handle);
            break;
          } else {
            ShellCloseFile(&Handle);
          }
        }
      }
    } while (Walker != NULL && Walker[0] != CHAR_NULL);
    FreePool(TestPath);
  }
  return (RetVal);
}

/**
  Find a file by searching the CWD and then the path with a variable set of file
  extensions.  If the file is not found it will append each extension in the list
  in the order provided and return the first one that is successful.

  If FileName is NULL, then ASSERT.
  If FileExtension is NULL, then behavior is identical to ShellFindFilePath.

  If the return value is not NULL then the memory must be caller freed.

  @param[in] FileName           Filename string.
  @param[in] FileExtension      Semi-colon delimeted list of possible extensions.

  @retval NULL                  The file was not found.
  @retval !NULL                 The path to the file.
**/
CHAR16 *
EFIAPI
ShellFindFilePathEx (
  IN CONST CHAR16 *FileName,
  IN CONST CHAR16 *FileExtension
  )
{
  CHAR16            *TestPath;
  CHAR16            *RetVal;
  CONST CHAR16      *ExtensionWalker;
  UINTN             Size;
  CHAR16            *TempChar;
  CHAR16            *TempChar2;

  ASSERT(FileName != NULL);
  if (FileExtension == NULL) {
    return (ShellFindFilePath(FileName));
  }
  RetVal = ShellFindFilePath(FileName);
  if (RetVal != NULL) {
    return (RetVal);
  }
  Size =  StrSize(FileName);
  Size += StrSize(FileExtension);
  TestPath = AllocateZeroPool(Size);
  if (TestPath == NULL) {
    return (NULL);
  }
  for (ExtensionWalker = FileExtension, TempChar2 = (CHAR16*)FileExtension;  TempChar2 != NULL ; ExtensionWalker = TempChar2 + 1){
    StrCpyS(TestPath, Size/sizeof(CHAR16), FileName);
    if (ExtensionWalker != NULL) {
      StrCatS(TestPath, Size/sizeof(CHAR16), ExtensionWalker);
    }
    TempChar = StrStr(TestPath, L";");
    if (TempChar != NULL) {
      *TempChar = CHAR_NULL;
    }
    RetVal = ShellFindFilePath(TestPath);
    if (RetVal != NULL) {
      break;
    }
    ASSERT(ExtensionWalker != NULL);
    TempChar2 = StrStr(ExtensionWalker, L";");
  }
  FreePool(TestPath);
  return (RetVal);
}

typedef struct {
  LIST_ENTRY     Link;
  CHAR16         *Name;
  SHELL_PARAM_TYPE      Type;
  CHAR16         *Value;
  UINTN          OriginalPosition;
} SHELL_PARAM_PACKAGE;

/**
  Checks the list of valid arguments and returns TRUE if the item was found.  If the
  return value is TRUE then the type parameter is set also.

  if CheckList is NULL then ASSERT();
  if Name is NULL then ASSERT();
  if Type is NULL then ASSERT();

  @param Name                   pointer to Name of parameter found
  @param CheckList              List to check against
  @param Type                   pointer to type of parameter if it was found

  @retval TRUE                  the Parameter was found.  Type is valid.
  @retval FALSE                 the Parameter was not found.  Type is not valid.
**/
BOOLEAN
EFIAPI
InternalIsOnCheckList (
  IN CONST CHAR16               *Name,
  IN CONST SHELL_PARAM_ITEM     *CheckList,
  OUT SHELL_PARAM_TYPE          *Type
  )
{
  SHELL_PARAM_ITEM              *TempListItem;
  CHAR16                        *TempString;

  //
  // ASSERT that all 3 pointer parameters aren't NULL
  //
  ASSERT(CheckList  != NULL);
  ASSERT(Type       != NULL);
  ASSERT(Name       != NULL);

  //
  // question mark and page break mode are always supported
  //
  if ((StrCmp(Name, L"-?") == 0) ||
      (StrCmp(Name, L"-b") == 0)
     ) {
     *Type = TypeFlag;
     return (TRUE);
  }

  //
  // Enumerate through the list
  //
  for (TempListItem = (SHELL_PARAM_ITEM*)CheckList ; TempListItem->Name != NULL ; TempListItem++) {
    //
    // If the Type is TypeStart only check the first characters of the passed in param
    // If it matches set the type and return TRUE
    //
    if (TempListItem->Type == TypeStart) {
      if (StrnCmp(Name, TempListItem->Name, StrLen(TempListItem->Name)) == 0) {
        *Type = TempListItem->Type;
        return (TRUE);
      }
      TempString = NULL;
      TempString = StrnCatGrow(&TempString, NULL, Name, StrLen(TempListItem->Name));
      if (TempString != NULL) {
        if (StringNoCaseCompare(&TempString, &TempListItem->Name) == 0) {
          *Type = TempListItem->Type;
          FreePool(TempString);
          return (TRUE);
        }
        FreePool(TempString);
      }
    } else if (StringNoCaseCompare(&Name, &TempListItem->Name) == 0) {
      *Type = TempListItem->Type;
      return (TRUE);
    }
  }

  return (FALSE);
}
/**
  Checks the string for indicators of "flag" status.  this is a leading '/', '-', or '+'

  @param[in] Name               pointer to Name of parameter found
  @param[in] AlwaysAllowNumbers TRUE to allow numbers, FALSE to not.
  @param[in] TimeNumbers        TRUE to allow numbers with ":", FALSE otherwise.

  @retval TRUE                  the Parameter is a flag.
  @retval FALSE                 the Parameter not a flag.
**/
BOOLEAN
EFIAPI
InternalIsFlag (
  IN CONST CHAR16               *Name,
  IN CONST BOOLEAN              AlwaysAllowNumbers,
  IN CONST BOOLEAN              TimeNumbers
  )
{
  //
  // ASSERT that Name isn't NULL
  //
  ASSERT(Name != NULL);

  //
  // If we accept numbers then dont return TRUE. (they will be values)
  //
  if (((Name[0] == L'-' || Name[0] == L'+') && InternalShellIsHexOrDecimalNumber(Name+1, FALSE, FALSE, TimeNumbers)) && AlwaysAllowNumbers) {
    return (FALSE);
  }

  //
  // If the Name has a /, +, or - as the first character return TRUE
  //
  if ((Name[0] == L'/') ||
      (Name[0] == L'-') ||
      (Name[0] == L'+')
     ) {
      return (TRUE);
  }
  return (FALSE);
}

/**
  Checks the command line arguments passed against the list of valid ones.

  If no initialization is required, then return RETURN_SUCCESS.

  @param[in] CheckList          pointer to list of parameters to check
  @param[out] CheckPackage      pointer to pointer to list checked values
  @param[out] ProblemParam      optional pointer to pointer to unicode string for
                                the paramater that caused failure.  If used then the
                                caller is responsible for freeing the memory.
  @param[in] AutoPageBreak      will automatically set PageBreakEnabled for "b" parameter
  @param[in] Argv               pointer to array of parameters
  @param[in] Argc               Count of parameters in Argv
  @param[in] AlwaysAllowNumbers TRUE to allow numbers always, FALSE otherwise.

  @retval EFI_SUCCESS           The operation completed sucessfully.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed
  @retval EFI_INVALID_PARAMETER A parameter was invalid
  @retval EFI_VOLUME_CORRUPTED  the command line was corrupt.  an argument was
                                duplicated.  the duplicated command line argument
                                was returned in ProblemParam if provided.
  @retval EFI_NOT_FOUND         a argument required a value that was missing.
                                the invalid command line argument was returned in
                                ProblemParam if provided.
**/
EFI_STATUS
EFIAPI
InternalCommandLineParse (
  IN CONST SHELL_PARAM_ITEM     *CheckList,
  OUT LIST_ENTRY                **CheckPackage,
  OUT CHAR16                    **ProblemParam OPTIONAL,
  IN BOOLEAN                    AutoPageBreak,
  IN CONST CHAR16               **Argv,
  IN UINTN                      Argc,
  IN BOOLEAN                    AlwaysAllowNumbers
  )
{
  UINTN                         LoopCounter;
  SHELL_PARAM_TYPE              CurrentItemType;
  SHELL_PARAM_PACKAGE           *CurrentItemPackage;
  UINTN                         GetItemValue;
  UINTN                         ValueSize;
  UINTN                         Count;
  CONST CHAR16                  *TempPointer;
  UINTN                         CurrentValueSize;

  CurrentItemPackage = NULL;
  GetItemValue = 0;
  ValueSize = 0;
  Count = 0;

  //
  // If there is only 1 item we dont need to do anything
  //
  if (Argc < 1) {
    *CheckPackage = NULL;
    return (EFI_SUCCESS);
  }

  //
  // ASSERTs
  //
  ASSERT(CheckList  != NULL);
  ASSERT(Argv       != NULL);

  //
  // initialize the linked list
  //
  *CheckPackage = (LIST_ENTRY*)AllocateZeroPool(sizeof(LIST_ENTRY));
  if (*CheckPackage == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  InitializeListHead(*CheckPackage);

  //
  // loop through each of the arguments
  //
  for (LoopCounter = 0 ; LoopCounter < Argc ; ++LoopCounter) {
    if (Argv[LoopCounter] == NULL) {
      //
      // do nothing for NULL argv
      //
    } else if (InternalIsOnCheckList(Argv[LoopCounter], CheckList, &CurrentItemType)) {
      //
      // We might have leftover if last parameter didnt have optional value
      //
      if (GetItemValue != 0) {
        GetItemValue = 0;
        InsertHeadList(*CheckPackage, &CurrentItemPackage->Link);
      }
      //
      // this is a flag
      //
      CurrentItemPackage = AllocateZeroPool(sizeof(SHELL_PARAM_PACKAGE));
      if (CurrentItemPackage == NULL) {
        ShellCommandLineFreeVarList(*CheckPackage);
        *CheckPackage = NULL;
        return (EFI_OUT_OF_RESOURCES);
      }
      CurrentItemPackage->Name  = AllocateCopyPool(StrSize(Argv[LoopCounter]), Argv[LoopCounter]);
      if (CurrentItemPackage->Name == NULL) {
        ShellCommandLineFreeVarList(*CheckPackage);
        *CheckPackage = NULL;
        return (EFI_OUT_OF_RESOURCES);
      }
      CurrentItemPackage->Type  = CurrentItemType;
      CurrentItemPackage->OriginalPosition = (UINTN)(-1);
      CurrentItemPackage->Value = NULL;

      //
      // Does this flag require a value
      //
      switch (CurrentItemPackage->Type) {
        //
        // possibly trigger the next loop(s) to populate the value of this item
        //
        case TypeValue:
        case TypeTimeValue:
          GetItemValue = 1;
          ValueSize = 0;
          break;
        case TypeDoubleValue:
          GetItemValue = 2;
          ValueSize = 0;
          break;
        case TypeMaxValue:
          GetItemValue = (UINTN)(-1);
          ValueSize = 0;
          break;
        default:
          //
          // this item has no value expected; we are done
          //
          InsertHeadList(*CheckPackage, &CurrentItemPackage->Link);
          ASSERT(GetItemValue == 0);
          break;
      }
    } else if (GetItemValue != 0 && CurrentItemPackage != NULL && !InternalIsFlag(Argv[LoopCounter], AlwaysAllowNumbers, (BOOLEAN)(CurrentItemPackage->Type == TypeTimeValue))) {
      //
      // get the item VALUE for a previous flag
      //
      CurrentValueSize = ValueSize + StrSize(Argv[LoopCounter]) + sizeof(CHAR16);
      CurrentItemPackage->Value = ReallocatePool(ValueSize, CurrentValueSize, CurrentItemPackage->Value);
      ASSERT(CurrentItemPackage->Value != NULL);
      if (ValueSize == 0) {
        StrCpyS( CurrentItemPackage->Value, 
                  CurrentValueSize/sizeof(CHAR16), 
                  Argv[LoopCounter]
                  );
      } else {
        StrCatS( CurrentItemPackage->Value, 
                  CurrentValueSize/sizeof(CHAR16), 
                  L" "
                  );
        StrCatS( CurrentItemPackage->Value, 
                  CurrentValueSize/sizeof(CHAR16), 
                  Argv[LoopCounter]
                  );
      }
      ValueSize += StrSize(Argv[LoopCounter]) + sizeof(CHAR16);
      
      GetItemValue--;
      if (GetItemValue == 0) {
        InsertHeadList(*CheckPackage, &CurrentItemPackage->Link);
      }
    } else if (!InternalIsFlag(Argv[LoopCounter], AlwaysAllowNumbers, FALSE)){
      //
      // add this one as a non-flag
      //

      TempPointer = Argv[LoopCounter];
      if ((*TempPointer == L'^' && *(TempPointer+1) == L'-')
       || (*TempPointer == L'^' && *(TempPointer+1) == L'/')
       || (*TempPointer == L'^' && *(TempPointer+1) == L'+')
      ){
        TempPointer++;
      }
      CurrentItemPackage = AllocateZeroPool(sizeof(SHELL_PARAM_PACKAGE));
      if (CurrentItemPackage == NULL) {
        ShellCommandLineFreeVarList(*CheckPackage);
        *CheckPackage = NULL;
        return (EFI_OUT_OF_RESOURCES);
      }
      CurrentItemPackage->Name  = NULL;
      CurrentItemPackage->Type  = TypePosition;
      CurrentItemPackage->Value = AllocateCopyPool(StrSize(TempPointer), TempPointer);
      if (CurrentItemPackage->Value == NULL) {
        ShellCommandLineFreeVarList(*CheckPackage);
        *CheckPackage = NULL;
        return (EFI_OUT_OF_RESOURCES);
      }
      CurrentItemPackage->OriginalPosition = Count++;
      InsertHeadList(*CheckPackage, &CurrentItemPackage->Link);
    } else {
      //
      // this was a non-recognised flag... error!
      //
      if (ProblemParam != NULL) {
        *ProblemParam = AllocateCopyPool(StrSize(Argv[LoopCounter]), Argv[LoopCounter]);
      }
      ShellCommandLineFreeVarList(*CheckPackage);
      *CheckPackage = NULL;
      return (EFI_VOLUME_CORRUPTED);
    }
  }
  if (GetItemValue != 0) {
    GetItemValue = 0;
    InsertHeadList(*CheckPackage, &CurrentItemPackage->Link);
  }
  //
  // support for AutoPageBreak
  //
  if (AutoPageBreak && ShellCommandLineGetFlag(*CheckPackage, L"-b")) {
    ShellSetPageBreakMode(TRUE);
  }
  return (EFI_SUCCESS);
}

/**
  Checks the command line arguments passed against the list of valid ones.
  Optionally removes NULL values first.

  If no initialization is required, then return RETURN_SUCCESS.

  @param[in] CheckList          The pointer to list of parameters to check.
  @param[out] CheckPackage      The package of checked values.
  @param[out] ProblemParam      Optional pointer to pointer to unicode string for
                                the paramater that caused failure.
  @param[in] AutoPageBreak      Will automatically set PageBreakEnabled.
  @param[in] AlwaysAllowNumbers Will never fail for number based flags.

  @retval EFI_SUCCESS           The operation completed sucessfully.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.
  @retval EFI_VOLUME_CORRUPTED  The command line was corrupt.
  @retval EFI_DEVICE_ERROR      The commands contained 2 opposing arguments.  One
                                of the command line arguments was returned in
                                ProblemParam if provided.
  @retval EFI_NOT_FOUND         A argument required a value that was missing.
                                The invalid command line argument was returned in
                                ProblemParam if provided.
**/
EFI_STATUS
EFIAPI
ShellCommandLineParseEx (
  IN CONST SHELL_PARAM_ITEM     *CheckList,
  OUT LIST_ENTRY                **CheckPackage,
  OUT CHAR16                    **ProblemParam OPTIONAL,
  IN BOOLEAN                    AutoPageBreak,
  IN BOOLEAN                    AlwaysAllowNumbers
  )
{
  //
  // ASSERT that CheckList and CheckPackage aren't NULL
  //
  ASSERT(CheckList    != NULL);
  ASSERT(CheckPackage != NULL);

  //
  // Check for UEFI Shell 2.0 protocols
  //
  if (gEfiShellParametersProtocol != NULL) {
    return (InternalCommandLineParse(CheckList,
                                     CheckPackage,
                                     ProblemParam,
                                     AutoPageBreak,
                                     (CONST CHAR16**) gEfiShellParametersProtocol->Argv,
                                     gEfiShellParametersProtocol->Argc,
                                     AlwaysAllowNumbers));
  }

  //
  // ASSERT That EFI Shell is not required
  //
  ASSERT (mEfiShellInterface != NULL);
  return (InternalCommandLineParse(CheckList,
                                   CheckPackage,
                                   ProblemParam,
                                   AutoPageBreak,
                                   (CONST CHAR16**) mEfiShellInterface->Argv,
                                   mEfiShellInterface->Argc,
                                   AlwaysAllowNumbers));
}

/**
  Frees shell variable list that was returned from ShellCommandLineParse.

  This function will free all the memory that was used for the CheckPackage
  list of postprocessed shell arguments.

  this function has no return value.

  if CheckPackage is NULL, then return

  @param CheckPackage           the list to de-allocate
  **/
VOID
EFIAPI
ShellCommandLineFreeVarList (
  IN LIST_ENTRY                 *CheckPackage
  )
{
  LIST_ENTRY                    *Node;

  //
  // check for CheckPackage == NULL
  //
  if (CheckPackage == NULL) {
    return;
  }

  //
  // for each node in the list
  //
  for ( Node = GetFirstNode(CheckPackage)
      ; !IsListEmpty(CheckPackage)
      ; Node = GetFirstNode(CheckPackage)
     ){
    //
    // Remove it from the list
    //
    RemoveEntryList(Node);

    //
    // if it has a name free the name
    //
    if (((SHELL_PARAM_PACKAGE*)Node)->Name != NULL) {
      FreePool(((SHELL_PARAM_PACKAGE*)Node)->Name);
    }

    //
    // if it has a value free the value
    //
    if (((SHELL_PARAM_PACKAGE*)Node)->Value != NULL) {
      FreePool(((SHELL_PARAM_PACKAGE*)Node)->Value);
    }

    //
    // free the node structure
    //
    FreePool((SHELL_PARAM_PACKAGE*)Node);
  }
  //
  // free the list head node
  //
  FreePool(CheckPackage);
}
/**
  Checks for presence of a flag parameter

  flag arguments are in the form of "-<Key>" or "/<Key>", but do not have a value following the key

  if CheckPackage is NULL then return FALSE.
  if KeyString is NULL then ASSERT()

  @param CheckPackage           The package of parsed command line arguments
  @param KeyString              the Key of the command line argument to check for

  @retval TRUE                  the flag is on the command line
  @retval FALSE                 the flag is not on the command line
  **/
BOOLEAN
EFIAPI
ShellCommandLineGetFlag (
  IN CONST LIST_ENTRY         * CONST CheckPackage,
  IN CONST CHAR16             * CONST KeyString
  )
{
  LIST_ENTRY                    *Node;
  CHAR16                        *TempString;

  //
  // return FALSE for no package or KeyString is NULL
  //
  if (CheckPackage == NULL || KeyString == NULL) {
    return (FALSE);
  }

  //
  // enumerate through the list of parametrs
  //
  for ( Node = GetFirstNode(CheckPackage)
      ; !IsNull (CheckPackage, Node)
      ; Node = GetNextNode(CheckPackage, Node)
      ){
    //
    // If the Name matches, return TRUE (and there may be NULL name)
    //
    if (((SHELL_PARAM_PACKAGE*)Node)->Name != NULL) {
      //
      // If Type is TypeStart then only compare the begining of the strings
      //
      if (((SHELL_PARAM_PACKAGE*)Node)->Type == TypeStart) {
        if (StrnCmp(KeyString, ((SHELL_PARAM_PACKAGE*)Node)->Name, StrLen(KeyString)) == 0) {
          return (TRUE);
        }
        TempString = NULL;
        TempString = StrnCatGrow(&TempString, NULL, KeyString, StrLen(((SHELL_PARAM_PACKAGE*)Node)->Name));
        if (TempString != NULL) {
          if (StringNoCaseCompare(&KeyString, &((SHELL_PARAM_PACKAGE*)Node)->Name) == 0) {
            FreePool(TempString);
            return (TRUE);
          }
          FreePool(TempString);
        }
      } else if (StringNoCaseCompare(&KeyString, &((SHELL_PARAM_PACKAGE*)Node)->Name) == 0) {
        return (TRUE);
      }
    }
  }
  return (FALSE);
}
/**
  Returns value from command line argument.

  Value parameters are in the form of "-<Key> value" or "/<Key> value".

  If CheckPackage is NULL, then return NULL.

  @param[in] CheckPackage       The package of parsed command line arguments.
  @param[in] KeyString          The Key of the command line argument to check for.

  @retval NULL                  The flag is not on the command line.
  @retval !=NULL                The pointer to unicode string of the value.
**/
CONST CHAR16*
EFIAPI
ShellCommandLineGetValue (
  IN CONST LIST_ENTRY           *CheckPackage,
  IN CHAR16                     *KeyString
  )
{
  LIST_ENTRY                    *Node;
  CHAR16                        *TempString;

  //
  // return NULL for no package or KeyString is NULL
  //
  if (CheckPackage == NULL || KeyString == NULL) {
    return (NULL);
  }

  //
  // enumerate through the list of parametrs
  //
  for ( Node = GetFirstNode(CheckPackage)
      ; !IsNull (CheckPackage, Node)
      ; Node = GetNextNode(CheckPackage, Node)
      ){
    //
    // If the Name matches, return TRUE (and there may be NULL name)
    //
    if (((SHELL_PARAM_PACKAGE*)Node)->Name != NULL) {
      //
      // If Type is TypeStart then only compare the begining of the strings
      //
      if (((SHELL_PARAM_PACKAGE*)Node)->Type == TypeStart) {
        if (StrnCmp(KeyString, ((SHELL_PARAM_PACKAGE*)Node)->Name, StrLen(KeyString)) == 0) {
          return (((SHELL_PARAM_PACKAGE*)Node)->Name + StrLen(KeyString));
        }
        TempString = NULL;
        TempString = StrnCatGrow(&TempString, NULL, KeyString, StrLen(((SHELL_PARAM_PACKAGE*)Node)->Name));
        if (TempString != NULL) {
          if (StringNoCaseCompare(&KeyString, &((SHELL_PARAM_PACKAGE*)Node)->Name) == 0) {
            FreePool(TempString);
            return (((SHELL_PARAM_PACKAGE*)Node)->Name + StrLen(KeyString));
          }
          FreePool(TempString);
        }
      } else if (StringNoCaseCompare(&KeyString, &((SHELL_PARAM_PACKAGE*)Node)->Name) == 0) {
        return (((SHELL_PARAM_PACKAGE*)Node)->Value);
      }
    }
  }
  return (NULL);
}

/**
  Returns raw value from command line argument.

  Raw value parameters are in the form of "value" in a specific position in the list.

  If CheckPackage is NULL, then return NULL.

  @param[in] CheckPackage       The package of parsed command line arguments.
  @param[in] Position           The position of the value.

  @retval NULL                  The flag is not on the command line.
  @retval !=NULL                The pointer to unicode string of the value.
  **/
CONST CHAR16*
EFIAPI
ShellCommandLineGetRawValue (
  IN CONST LIST_ENTRY           * CONST CheckPackage,
  IN UINTN                      Position
  )
{
  LIST_ENTRY                    *Node;

  //
  // check for CheckPackage == NULL
  //
  if (CheckPackage == NULL) {
    return (NULL);
  }

  //
  // enumerate through the list of parametrs
  //
  for ( Node = GetFirstNode(CheckPackage)
      ; !IsNull (CheckPackage, Node)
      ; Node = GetNextNode(CheckPackage, Node)
     ){
    //
    // If the position matches, return the value
    //
    if (((SHELL_PARAM_PACKAGE*)Node)->OriginalPosition == Position) {
      return (((SHELL_PARAM_PACKAGE*)Node)->Value);
    }
  }
  return (NULL);
}

/**
  returns the number of command line value parameters that were parsed.

  this will not include flags.

  @param[in] CheckPackage       The package of parsed command line arguments.

  @retval (UINTN)-1     No parsing has ocurred
  @return other         The number of value parameters found
**/
UINTN
EFIAPI
ShellCommandLineGetCount(
  IN CONST LIST_ENTRY              *CheckPackage
  )
{
  LIST_ENTRY  *Node1;
  UINTN       Count;

  if (CheckPackage == NULL) {
    return (0);
  }
  for ( Node1 = GetFirstNode(CheckPackage), Count = 0
      ; !IsNull (CheckPackage, Node1)
      ; Node1 = GetNextNode(CheckPackage, Node1)
     ){
    if (((SHELL_PARAM_PACKAGE*)Node1)->Name == NULL) {
      Count++;
    }
  }
  return (Count);
}

/**
  Determines if a parameter is duplicated.

  If Param is not NULL then it will point to a callee allocated string buffer
  with the parameter value if a duplicate is found.

  If CheckPackage is NULL, then ASSERT.

  @param[in] CheckPackage       The package of parsed command line arguments.
  @param[out] Param             Upon finding one, a pointer to the duplicated parameter.

  @retval EFI_SUCCESS           No parameters were duplicated.
  @retval EFI_DEVICE_ERROR      A duplicate was found.
  **/
EFI_STATUS
EFIAPI
ShellCommandLineCheckDuplicate (
  IN CONST LIST_ENTRY              *CheckPackage,
  OUT CHAR16                       **Param
  )
{
  LIST_ENTRY                    *Node1;
  LIST_ENTRY                    *Node2;

  ASSERT(CheckPackage != NULL);

  for ( Node1 = GetFirstNode(CheckPackage)
      ; !IsNull (CheckPackage, Node1)
      ; Node1 = GetNextNode(CheckPackage, Node1)
     ){
    for ( Node2 = GetNextNode(CheckPackage, Node1)
        ; !IsNull (CheckPackage, Node2)
        ; Node2 = GetNextNode(CheckPackage, Node2)
       ){
      if ((((SHELL_PARAM_PACKAGE*)Node1)->Name != NULL) && (((SHELL_PARAM_PACKAGE*)Node2)->Name != NULL) && StrCmp(((SHELL_PARAM_PACKAGE*)Node1)->Name, ((SHELL_PARAM_PACKAGE*)Node2)->Name) == 0) {
        if (Param != NULL) {
          *Param = NULL;
          *Param = StrnCatGrow(Param, NULL, ((SHELL_PARAM_PACKAGE*)Node1)->Name, 0);
        }
        return (EFI_DEVICE_ERROR);
      }
    }
  }
  return (EFI_SUCCESS);
}

/**
  This is a find and replace function.  Upon successful return the NewString is a copy of
  SourceString with each instance of FindTarget replaced with ReplaceWith.

  If SourceString and NewString overlap the behavior is undefined.

  If the string would grow bigger than NewSize it will halt and return error.

  @param[in] SourceString              The string with source buffer.
  @param[in, out] NewString            The string with resultant buffer.
  @param[in] NewSize                   The size in bytes of NewString.
  @param[in] FindTarget                The string to look for.
  @param[in] ReplaceWith               The string to replace FindTarget with.
  @param[in] SkipPreCarrot             If TRUE will skip a FindTarget that has a '^'
                                       immediately before it.
  @param[in] ParameterReplacing        If TRUE will add "" around items with spaces.

  @retval EFI_INVALID_PARAMETER       SourceString was NULL.
  @retval EFI_INVALID_PARAMETER       NewString was NULL.
  @retval EFI_INVALID_PARAMETER       FindTarget was NULL.
  @retval EFI_INVALID_PARAMETER       ReplaceWith was NULL.
  @retval EFI_INVALID_PARAMETER       FindTarget had length < 1.
  @retval EFI_INVALID_PARAMETER       SourceString had length < 1.
  @retval EFI_BUFFER_TOO_SMALL        NewSize was less than the minimum size to hold
                                      the new string (truncation occurred).
  @retval EFI_SUCCESS                 The string was successfully copied with replacement.
**/
EFI_STATUS
EFIAPI
ShellCopySearchAndReplace(
  IN CHAR16 CONST                     *SourceString,
  IN OUT CHAR16                       *NewString,
  IN UINTN                            NewSize,
  IN CONST CHAR16                     *FindTarget,
  IN CONST CHAR16                     *ReplaceWith,
  IN CONST BOOLEAN                    SkipPreCarrot,
  IN CONST BOOLEAN                    ParameterReplacing
  )
{
  UINTN Size;
  CHAR16 *Replace;

  if ( (SourceString == NULL)
    || (NewString    == NULL)
    || (FindTarget   == NULL)
    || (ReplaceWith  == NULL)
    || (StrLen(FindTarget) < 1)
    || (StrLen(SourceString) < 1)
   ){
    return (EFI_INVALID_PARAMETER);
  }
  Replace = NULL;
  if (StrStr(ReplaceWith, L" ") == NULL || !ParameterReplacing) {
    Replace = StrnCatGrow(&Replace, NULL, ReplaceWith, 0);
  } else {
    Replace = AllocateZeroPool(StrSize(ReplaceWith) + 2*sizeof(CHAR16));
    if (Replace != NULL) {
      UnicodeSPrint(Replace, StrSize(ReplaceWith) + 2*sizeof(CHAR16), L"\"%s\"", ReplaceWith);
    }
  }
  if (Replace == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }
  NewString = ZeroMem(NewString, NewSize);
  while (*SourceString != CHAR_NULL) {
    //
    // if we find the FindTarget and either Skip == FALSE or Skip  and we
    // dont have a carrot do a replace...
    //
    if (StrnCmp(SourceString, FindTarget, StrLen(FindTarget)) == 0
      && ((SkipPreCarrot && *(SourceString-1) != L'^') || !SkipPreCarrot)
     ){
      SourceString += StrLen(FindTarget);
      Size = StrSize(NewString);
      if ((Size + (StrLen(Replace)*sizeof(CHAR16))) > NewSize) {
        FreePool(Replace);
        return (EFI_BUFFER_TOO_SMALL);
      }
      StrCatS(NewString, NewSize/sizeof(CHAR16), Replace);
    } else {
      Size = StrSize(NewString);
      if (Size + sizeof(CHAR16) > NewSize) {
        FreePool(Replace);
        return (EFI_BUFFER_TOO_SMALL);
      }
      StrnCatS(NewString, NewSize/sizeof(CHAR16), SourceString, 1);
      SourceString++;
    }
  }
  FreePool(Replace);
  return (EFI_SUCCESS);
}

/**
  Internal worker function to output a string.

  This function will output a string to the correct StdOut.

  @param[in] String       The string to print out.

  @retval EFI_SUCCESS     The operation was sucessful.
  @retval !EFI_SUCCESS    The operation failed.
**/
EFI_STATUS
EFIAPI
InternalPrintTo (
  IN CONST CHAR16 *String
  )
{
  UINTN Size;
  Size = StrSize(String) - sizeof(CHAR16);
  if (Size == 0) {
    return (EFI_SUCCESS);
  }
  if (gEfiShellParametersProtocol != NULL) {
    return (gEfiShellProtocol->WriteFile(gEfiShellParametersProtocol->StdOut, &Size, (VOID*)String));
  }
  if (mEfiShellInterface          != NULL) {
    if (mEfiShellInterface->RedirArgc == 0) { 
    //
    // Divide in half for old shell.  Must be string length not size.
      // 
      Size /=2;  // Divide in half only when no redirection.
    }
    return (mEfiShellInterface->StdOut->Write(mEfiShellInterface->StdOut,          &Size, (VOID*)String));
  }
  ASSERT(FALSE);
  return (EFI_UNSUPPORTED);
}

/**
  Print at a specific location on the screen.

  This function will move the cursor to a given screen location and print the specified string

  If -1 is specified for either the Row or Col the current screen location for BOTH
  will be used.

  if either Row or Col is out of range for the current console, then ASSERT
  if Format is NULL, then ASSERT

  In addition to the standard %-based flags as supported by UefiLib Print() this supports
  the following additional flags:
    %N       -   Set output attribute to normal
    %H       -   Set output attribute to highlight
    %E       -   Set output attribute to error
    %B       -   Set output attribute to blue color
    %V       -   Set output attribute to green color

  Note: The background color is controlled by the shell command cls.

  @param[in] Col        the column to print at
  @param[in] Row        the row to print at
  @param[in] Format     the format string
  @param[in] Marker     the marker for the variable argument list

  @return EFI_SUCCESS           The operation was successful.
  @return EFI_DEVICE_ERROR      The console device reported an error.
**/
EFI_STATUS
EFIAPI
InternalShellPrintWorker(
  IN INT32                Col OPTIONAL,
  IN INT32                Row OPTIONAL,
  IN CONST CHAR16         *Format,
  IN VA_LIST              Marker
  )
{
  EFI_STATUS        Status;
  CHAR16            *ResumeLocation;
  CHAR16            *FormatWalker;
  UINTN             OriginalAttribute;
  CHAR16            *mPostReplaceFormat;
  CHAR16            *mPostReplaceFormat2;

  mPostReplaceFormat = AllocateZeroPool (PcdGet16 (PcdShellPrintBufferSize));
  mPostReplaceFormat2 = AllocateZeroPool (PcdGet16 (PcdShellPrintBufferSize));

  if (mPostReplaceFormat == NULL || mPostReplaceFormat2 == NULL) {
    SHELL_FREE_NON_NULL(mPostReplaceFormat);
    SHELL_FREE_NON_NULL(mPostReplaceFormat2);
    return (EFI_OUT_OF_RESOURCES);
  }

  Status            = EFI_SUCCESS;
  OriginalAttribute = gST->ConOut->Mode->Attribute;

  //
  // Back and forth each time fixing up 1 of our flags...
  //
  Status = ShellCopySearchAndReplace(Format,             mPostReplaceFormat,  PcdGet16 (PcdShellPrintBufferSize), L"%N", L"%%N", FALSE, FALSE);
  ASSERT_EFI_ERROR(Status);
  Status = ShellCopySearchAndReplace(mPostReplaceFormat,  mPostReplaceFormat2, PcdGet16 (PcdShellPrintBufferSize), L"%E", L"%%E", FALSE, FALSE);
  ASSERT_EFI_ERROR(Status);
  Status = ShellCopySearchAndReplace(mPostReplaceFormat2, mPostReplaceFormat,  PcdGet16 (PcdShellPrintBufferSize), L"%H", L"%%H", FALSE, FALSE);
  ASSERT_EFI_ERROR(Status);
  Status = ShellCopySearchAndReplace(mPostReplaceFormat,  mPostReplaceFormat2, PcdGet16 (PcdShellPrintBufferSize), L"%B", L"%%B", FALSE, FALSE);
  ASSERT_EFI_ERROR(Status);
  Status = ShellCopySearchAndReplace(mPostReplaceFormat2, mPostReplaceFormat,  PcdGet16 (PcdShellPrintBufferSize), L"%V", L"%%V", FALSE, FALSE);
  ASSERT_EFI_ERROR(Status);

  //
  // Use the last buffer from replacing to print from...
  //
  UnicodeVSPrint (mPostReplaceFormat2, PcdGet16 (PcdShellPrintBufferSize), mPostReplaceFormat, Marker);

  if (Col != -1 && Row != -1) {
    Status = gST->ConOut->SetCursorPosition(gST->ConOut, Col, Row);
  }

  FormatWalker = mPostReplaceFormat2;
  while (*FormatWalker != CHAR_NULL) {
    //
    // Find the next attribute change request
    //
    ResumeLocation = StrStr(FormatWalker, L"%");
    if (ResumeLocation != NULL) {
      *ResumeLocation = CHAR_NULL;
    }
    //
    // print the current FormatWalker string
    //
    if (StrLen(FormatWalker)>0) {
      Status = InternalPrintTo(FormatWalker);
      if (EFI_ERROR(Status)) {
        break;
      }
    }

    //
    // update the attribute
    //
    if (ResumeLocation != NULL) {
      if (*(ResumeLocation-1) == L'^') {
        //
        // Move cursor back 1 position to overwrite the ^
        //
        gST->ConOut->SetCursorPosition(gST->ConOut, gST->ConOut->Mode->CursorColumn - 1, gST->ConOut->Mode->CursorRow);

        //
        // Print a simple '%' symbol
        //
        Status = InternalPrintTo(L"%");
        ResumeLocation = ResumeLocation - 1;
      } else {
        switch (*(ResumeLocation+1)) {
          case (L'N'):
            gST->ConOut->SetAttribute(gST->ConOut, OriginalAttribute);
            break;
          case (L'E'):
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_YELLOW, ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)));
            break;
          case (L'H'):
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)));
            break;
          case (L'B'):
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_BLUE, ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)));
            break;
          case (L'V'):
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)));
            break;
          default:
            //
            // Print a simple '%' symbol
            //
            Status = InternalPrintTo(L"%");
            if (EFI_ERROR(Status)) {
              break;
            }
            ResumeLocation = ResumeLocation - 1;
            break;
        }
      }
    } else {
      //
      // reset to normal now...
      //
      break;
    }

    //
    // update FormatWalker to Resume + 2 (skip the % and the indicator)
    //
    FormatWalker = ResumeLocation + 2;
  }

  gST->ConOut->SetAttribute(gST->ConOut, OriginalAttribute);

  SHELL_FREE_NON_NULL(mPostReplaceFormat);
  SHELL_FREE_NON_NULL(mPostReplaceFormat2);
  return (Status);
}

/**
  Print at a specific location on the screen.

  This function will move the cursor to a given screen location and print the specified string.

  If -1 is specified for either the Row or Col the current screen location for BOTH
  will be used.

  If either Row or Col is out of range for the current console, then ASSERT.
  If Format is NULL, then ASSERT.

  In addition to the standard %-based flags as supported by UefiLib Print() this supports
  the following additional flags:
    %N       -   Set output attribute to normal
    %H       -   Set output attribute to highlight
    %E       -   Set output attribute to error
    %B       -   Set output attribute to blue color
    %V       -   Set output attribute to green color

  Note: The background color is controlled by the shell command cls.

  @param[in] Col        the column to print at
  @param[in] Row        the row to print at
  @param[in] Format     the format string
  @param[in] ...        The variable argument list.

  @return EFI_SUCCESS           The printing was successful.
  @return EFI_DEVICE_ERROR      The console device reported an error.
**/
EFI_STATUS
EFIAPI
ShellPrintEx(
  IN INT32                Col OPTIONAL,
  IN INT32                Row OPTIONAL,
  IN CONST CHAR16         *Format,
  ...
  )
{
  VA_LIST           Marker;
  EFI_STATUS        RetVal;
  if (Format == NULL) {
    return (EFI_INVALID_PARAMETER);
  }
  VA_START (Marker, Format);
  RetVal = InternalShellPrintWorker(Col, Row, Format, Marker);
  VA_END(Marker);
  return(RetVal);
}

/**
  Print at a specific location on the screen.

  This function will move the cursor to a given screen location and print the specified string.

  If -1 is specified for either the Row or Col the current screen location for BOTH
  will be used.

  If either Row or Col is out of range for the current console, then ASSERT.
  If Format is NULL, then ASSERT.

  In addition to the standard %-based flags as supported by UefiLib Print() this supports
  the following additional flags:
    %N       -   Set output attribute to normal.
    %H       -   Set output attribute to highlight.
    %E       -   Set output attribute to error.
    %B       -   Set output attribute to blue color.
    %V       -   Set output attribute to green color.

  Note: The background color is controlled by the shell command cls.

  @param[in] Col                The column to print at.
  @param[in] Row                The row to print at.
  @param[in] Language           The language of the string to retrieve.  If this parameter
                                is NULL, then the current platform language is used.
  @param[in] HiiFormatStringId  The format string Id for getting from Hii.
  @param[in] HiiFormatHandle    The format string Handle for getting from Hii.
  @param[in] ...                The variable argument list.

  @return EFI_SUCCESS           The printing was successful.
  @return EFI_DEVICE_ERROR      The console device reported an error.
**/
EFI_STATUS
EFIAPI
ShellPrintHiiEx(
  IN INT32                Col OPTIONAL,
  IN INT32                Row OPTIONAL,
  IN CONST CHAR8          *Language OPTIONAL,
  IN CONST EFI_STRING_ID  HiiFormatStringId,
  IN CONST EFI_HANDLE     HiiFormatHandle,
  ...
  )
{
  VA_LIST           Marker;
  CHAR16            *HiiFormatString;
  EFI_STATUS        RetVal;

  VA_START (Marker, HiiFormatHandle);
  HiiFormatString = HiiGetString(HiiFormatHandle, HiiFormatStringId, Language);
  ASSERT(HiiFormatString != NULL);

  RetVal = InternalShellPrintWorker(Col, Row, HiiFormatString, Marker);

  SHELL_FREE_NON_NULL(HiiFormatString);
  VA_END(Marker);

  return (RetVal);
}

/**
  Function to determine if a given filename represents a file or a directory.

  @param[in] DirName      Path to directory to test.

  @retval EFI_SUCCESS             The Path represents a directory
  @retval EFI_NOT_FOUND           The Path does not represent a directory
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
  @return                         The path failed to open
**/
EFI_STATUS
EFIAPI
ShellIsDirectory(
  IN CONST CHAR16 *DirName
  )
{
  EFI_STATUS        Status;
  SHELL_FILE_HANDLE Handle;
  CHAR16            *TempLocation;
  CHAR16            *TempLocation2;

  ASSERT(DirName != NULL);

  Handle        = NULL;
  TempLocation  = NULL;

  Status = ShellOpenFileByName(DirName, &Handle, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status)) {
    //
    // try good logic first.
    //
    if (gEfiShellProtocol != NULL) {
      TempLocation  = StrnCatGrow(&TempLocation, NULL, DirName, 0);
      if (TempLocation == NULL) {
        ShellCloseFile(&Handle);
        return (EFI_OUT_OF_RESOURCES);
      }
      TempLocation2 = StrStr(TempLocation, L":");
      if (TempLocation2 != NULL && StrLen(StrStr(TempLocation, L":")) == 2) {
        *(TempLocation2+1) = CHAR_NULL;
      }
      if (gEfiShellProtocol->GetDevicePathFromMap(TempLocation) != NULL) {
        FreePool(TempLocation);
        return (EFI_SUCCESS);
      }
      FreePool(TempLocation);
    } else {
      //
      // probably a map name?!?!!?
      //
      TempLocation = StrStr(DirName, L"\\");
      if (TempLocation != NULL && *(TempLocation+1) == CHAR_NULL) {
        return (EFI_SUCCESS);
      }
    }
    return (Status);
  }

  if (FileHandleIsDirectory(Handle) == EFI_SUCCESS) {
    ShellCloseFile(&Handle);
    return (EFI_SUCCESS);
  }
  ShellCloseFile(&Handle);
  return (EFI_NOT_FOUND);
}

/**
  Function to determine if a given filename represents a file.

  @param[in] Name         Path to file to test.

  @retval EFI_SUCCESS     The Path represents a file.
  @retval EFI_NOT_FOUND   The Path does not represent a file.
  @retval other           The path failed to open.
**/
EFI_STATUS
EFIAPI
ShellIsFile(
  IN CONST CHAR16 *Name
  )
{
  EFI_STATUS        Status;
  SHELL_FILE_HANDLE            Handle;

  ASSERT(Name != NULL);

  Handle = NULL;

  Status = ShellOpenFileByName(Name, &Handle, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status)) {
    return (Status);
  }

  if (FileHandleIsDirectory(Handle) != EFI_SUCCESS) {
    ShellCloseFile(&Handle);
    return (EFI_SUCCESS);
  }
  ShellCloseFile(&Handle);
  return (EFI_NOT_FOUND);
}

/**
  Function to determine if a given filename represents a file.

  This will search the CWD and then the Path.

  If Name is NULL, then ASSERT.

  @param[in] Name         Path to file to test.

  @retval EFI_SUCCESS     The Path represents a file.
  @retval EFI_NOT_FOUND   The Path does not represent a file.
  @retval other           The path failed to open.
**/
EFI_STATUS
EFIAPI
ShellIsFileInPath(
  IN CONST CHAR16 *Name
  )
{
  CHAR16      *NewName;
  EFI_STATUS  Status;

  if (!EFI_ERROR(ShellIsFile(Name))) {
    return (EFI_SUCCESS);
  }

  NewName = ShellFindFilePath(Name);
  if (NewName == NULL) {
    return (EFI_NOT_FOUND);
  }
  Status = ShellIsFile(NewName);
  FreePool(NewName);
  return (Status);
}

/**
  Function return the number converted from a hex representation of a number.

  Note: this function cannot be used when (UINTN)(-1), (0xFFFFFFFF) may be a valid
  result.  Use ShellConvertStringToUint64 instead.

  @param[in] String   String representation of a number.

  @return             The unsigned integer result of the conversion.
  @retval (UINTN)(-1) An error occured.
**/
UINTN
EFIAPI
ShellHexStrToUintn(
  IN CONST CHAR16 *String
  )
{
  UINT64        RetVal;

  if (!EFI_ERROR(ShellConvertStringToUint64(String, &RetVal, TRUE, TRUE))) {
    return ((UINTN)RetVal);
  }
  
  return ((UINTN)(-1));
}

/**
  Function to determine whether a string is decimal or hex representation of a number
  and return the number converted from the string.  Spaces are always skipped.

  @param[in] String   String representation of a number

  @return             the number
  @retval (UINTN)(-1) An error ocurred.
**/
UINTN
EFIAPI
ShellStrToUintn(
  IN CONST CHAR16 *String
  )
{
  UINT64        RetVal;
  BOOLEAN       Hex;

  Hex = FALSE;

  if (!InternalShellIsHexOrDecimalNumber(String, Hex, TRUE, FALSE)) {
    Hex = TRUE;
  }

  if (!EFI_ERROR(ShellConvertStringToUint64(String, &RetVal, Hex, TRUE))) {
    return ((UINTN)RetVal);
  }
  return ((UINTN)(-1));
}

/**
  Safely append with automatic string resizing given length of Destination and
  desired length of copy from Source.

  append the first D characters of Source to the end of Destination, where D is
  the lesser of Count and the StrLen() of Source. If appending those D characters
  will fit within Destination (whose Size is given as CurrentSize) and
  still leave room for a NULL terminator, then those characters are appended,
  starting at the original terminating NULL of Destination, and a new terminating
  NULL is appended.

  If appending D characters onto Destination will result in a overflow of the size
  given in CurrentSize the string will be grown such that the copy can be performed
  and CurrentSize will be updated to the new size.

  If Source is NULL, there is nothing to append, just return the current buffer in
  Destination.

  if Destination is NULL, then ASSERT()
  if Destination's current length (including NULL terminator) is already more then
  CurrentSize, then ASSERT()

  @param[in, out] Destination   The String to append onto
  @param[in, out] CurrentSize   on call the number of bytes in Destination.  On
                                return possibly the new size (still in bytes).  if NULL
                                then allocate whatever is needed.
  @param[in]      Source        The String to append from
  @param[in]      Count         Maximum number of characters to append.  if 0 then
                                all are appended.

  @return Destination           return the resultant string.
**/
CHAR16*
EFIAPI
StrnCatGrow (
  IN OUT CHAR16           **Destination,
  IN OUT UINTN            *CurrentSize,
  IN     CONST CHAR16     *Source,
  IN     UINTN            Count
  )
{
  UINTN DestinationStartSize;
  UINTN NewSize;

  //
  // ASSERTs
  //
  ASSERT(Destination != NULL);

  //
  // If there's nothing to do then just return Destination
  //
  if (Source == NULL) {
    return (*Destination);
  }

  //
  // allow for un-initialized pointers, based on size being 0
  //
  if (CurrentSize != NULL && *CurrentSize == 0) {
    *Destination = NULL;
  }

  //
  // allow for NULL pointers address as Destination
  //
  if (*Destination != NULL) {
    ASSERT(CurrentSize != 0);
    DestinationStartSize = StrSize(*Destination);
    ASSERT(DestinationStartSize <= *CurrentSize);
  } else {
    DestinationStartSize = 0;
//    ASSERT(*CurrentSize == 0);
  }

  //
  // Append all of Source?
  //
  if (Count == 0) {
    Count = StrLen(Source);
  }

  //
  // Test and grow if required
  //
  if (CurrentSize != NULL) {
    NewSize = *CurrentSize;
    if (NewSize < DestinationStartSize + (Count * sizeof(CHAR16))) {
      while (NewSize < (DestinationStartSize + (Count*sizeof(CHAR16)))) {
        NewSize += 2 * Count * sizeof(CHAR16);
      }
      *Destination = ReallocatePool(*CurrentSize, NewSize, *Destination);
      *CurrentSize = NewSize;
    }
  } else {
    NewSize = (Count+1)*sizeof(CHAR16);
    *Destination = AllocateZeroPool(NewSize);
  }

  //
  // Now use standard StrnCat on a big enough buffer
  //
  if (*Destination == NULL) {
    return (NULL);
  }
  
  StrnCatS(*Destination, NewSize/sizeof(CHAR16), Source, Count);
  return *Destination;
}

/**
  Prompt the user and return the resultant answer to the requestor.

  This function will display the requested question on the shell prompt and then
  wait for an apropriate answer to be input from the console.

  if the SHELL_PROMPT_REQUEST_TYPE is SHELL_PROMPT_REQUEST_TYPE_YESNO, ShellPromptResponseTypeQuitContinue
  or SHELL_PROMPT_REQUEST_TYPE_YESNOCANCEL then *Response is of type SHELL_PROMPT_RESPONSE.

  if the SHELL_PROMPT_REQUEST_TYPE is ShellPromptResponseTypeFreeform then *Response is of type
  CHAR16*.

  In either case *Response must be callee freed if Response was not NULL;

  @param Type                     What type of question is asked.  This is used to filter the input
                                  to prevent invalid answers to question.
  @param Prompt                   Pointer to string prompt to use to request input.
  @param Response                 Pointer to Response which will be populated upon return.

  @retval EFI_SUCCESS             The operation was sucessful.
  @retval EFI_UNSUPPORTED         The operation is not supported as requested.
  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @return other                   The operation failed.
**/
EFI_STATUS
EFIAPI
ShellPromptForResponse (
  IN SHELL_PROMPT_REQUEST_TYPE   Type,
  IN CHAR16         *Prompt OPTIONAL,
  IN OUT VOID       **Response OPTIONAL
  )
{
  EFI_STATUS        Status;
  EFI_INPUT_KEY     Key;
  UINTN             EventIndex;
  SHELL_PROMPT_RESPONSE          *Resp;
  UINTN             Size;
  CHAR16            *Buffer;

  Status  = EFI_UNSUPPORTED;
  Resp    = NULL;
  Buffer  = NULL;
  Size    = 0;
  if (Type != ShellPromptResponseTypeFreeform) {
    Resp = (SHELL_PROMPT_RESPONSE*)AllocateZeroPool(sizeof(SHELL_PROMPT_RESPONSE));
    if (Resp == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
  }

  switch(Type) {
    case ShellPromptResponseTypeQuitContinue:
      if (Prompt != NULL) {
        ShellPrintEx(-1, -1, L"%s", Prompt);
      }
      //
      // wait for valid response
      //
      gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      if (EFI_ERROR(Status)) {
        break;
      }
      ShellPrintEx(-1, -1, L"%c", Key.UnicodeChar);
      if (Key.UnicodeChar == L'Q' || Key.UnicodeChar ==L'q') {
        *Resp = ShellPromptResponseQuit;
      } else {
        *Resp = ShellPromptResponseContinue;
      }
      break;
    case ShellPromptResponseTypeYesNoCancel:
       if (Prompt != NULL) {
        ShellPrintEx(-1, -1, L"%s", Prompt);
      }
      //
      // wait for valid response
      //
      *Resp = ShellPromptResponseMax;
      while (*Resp == ShellPromptResponseMax) {
        if (ShellGetExecutionBreakFlag()) {
          Status = EFI_ABORTED;
          break;
        }
        gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
        Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
        if (EFI_ERROR(Status)) {
          break;
        }
        ShellPrintEx(-1, -1, L"%c", Key.UnicodeChar);
        switch (Key.UnicodeChar) {
          case L'Y':
          case L'y':
            *Resp = ShellPromptResponseYes;
            break;
          case L'N':
          case L'n':
            *Resp = ShellPromptResponseNo;
            break;
          case L'C':
          case L'c':
            *Resp = ShellPromptResponseCancel;
            break;
        }
      }
      break;    case ShellPromptResponseTypeYesNoAllCancel:
       if (Prompt != NULL) {
        ShellPrintEx(-1, -1, L"%s", Prompt);
      }
      //
      // wait for valid response
      //
      *Resp = ShellPromptResponseMax;
      while (*Resp == ShellPromptResponseMax) {
        if (ShellGetExecutionBreakFlag()) {
          Status = EFI_ABORTED;
          break;
        }
        gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
        Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
        if (EFI_ERROR(Status)) {
          break;
        }
        ShellPrintEx(-1, -1, L"%c", Key.UnicodeChar);
        switch (Key.UnicodeChar) {
          case L'Y':
          case L'y':
            *Resp = ShellPromptResponseYes;
            break;
          case L'N':
          case L'n':
            *Resp = ShellPromptResponseNo;
            break;
          case L'A':
          case L'a':
            *Resp = ShellPromptResponseAll;
            break;
          case L'C':
          case L'c':
            *Resp = ShellPromptResponseCancel;
            break;
        }
      }
      break;
    case ShellPromptResponseTypeEnterContinue:
    case ShellPromptResponseTypeAnyKeyContinue:
      if (Prompt != NULL) {
        ShellPrintEx(-1, -1, L"%s", Prompt);
      }
      //
      // wait for valid response
      //
      *Resp = ShellPromptResponseMax;
      while (*Resp == ShellPromptResponseMax) {
        if (ShellGetExecutionBreakFlag()) {
          Status = EFI_ABORTED;
          break;
        }
        gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
        if (Type == ShellPromptResponseTypeEnterContinue) {
          Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
          if (EFI_ERROR(Status)) {
            break;
          }
          ShellPrintEx(-1, -1, L"%c", Key.UnicodeChar);
          if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            *Resp = ShellPromptResponseContinue;
            break;
          }
        }
        if (Type == ShellPromptResponseTypeAnyKeyContinue) {
          Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
          ASSERT_EFI_ERROR(Status);
          *Resp = ShellPromptResponseContinue;
          break;
        }
      }
      break;
    case ShellPromptResponseTypeYesNo:
       if (Prompt != NULL) {
        ShellPrintEx(-1, -1, L"%s", Prompt);
      }
      //
      // wait for valid response
      //
      *Resp = ShellPromptResponseMax;
      while (*Resp == ShellPromptResponseMax) {
        if (ShellGetExecutionBreakFlag()) {
          Status = EFI_ABORTED;
          break;
        }
        gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
        Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
        if (EFI_ERROR(Status)) {
          break;
        }
        ShellPrintEx(-1, -1, L"%c", Key.UnicodeChar);
        switch (Key.UnicodeChar) {
          case L'Y':
          case L'y':
            *Resp = ShellPromptResponseYes;
            break;
          case L'N':
          case L'n':
            *Resp = ShellPromptResponseNo;
            break;
        }
      }
      break;
    case ShellPromptResponseTypeFreeform:
      if (Prompt != NULL) {
        ShellPrintEx(-1, -1, L"%s", Prompt);
      }
      while(1) {
        if (ShellGetExecutionBreakFlag()) {
          Status = EFI_ABORTED;
          break;
        }
        gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
        Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
        if (EFI_ERROR(Status)) {
          break;
        }
        ShellPrintEx(-1, -1, L"%c", Key.UnicodeChar);
        if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          break;
        }
        ASSERT((Buffer == NULL && Size == 0) || (Buffer != NULL));
        StrnCatGrow(&Buffer, &Size, &Key.UnicodeChar, 1);
      }
      break;
    //
    // This is the location to add new prompt types.
    // If your new type loops remember to add ExecutionBreak support.
    //
    default:
      ASSERT(FALSE);
  }

  if (Response != NULL) {
    if (Resp != NULL) {
      *Response = Resp;
    } else if (Buffer != NULL) {
      *Response = Buffer;
    }
  } else {
    if (Resp != NULL) {
      FreePool(Resp);
    }
    if (Buffer != NULL) {
      FreePool(Buffer);
    }
  }

  ShellPrintEx(-1, -1, L"\r\n");
  return (Status);
}

/**
  Prompt the user and return the resultant answer to the requestor.

  This function is the same as ShellPromptForResponse, except that the prompt is
  automatically pulled from HII.

  @param Type     What type of question is asked.  This is used to filter the input
                  to prevent invalid answers to question.
  @param[in] HiiFormatStringId  The format string Id for getting from Hii.
  @param[in] HiiFormatHandle    The format string Handle for getting from Hii.
  @param Response               Pointer to Response which will be populated upon return.

  @retval EFI_SUCCESS the operation was sucessful.
  @return other       the operation failed.

  @sa ShellPromptForResponse
**/
EFI_STATUS
EFIAPI
ShellPromptForResponseHii (
  IN SHELL_PROMPT_REQUEST_TYPE         Type,
  IN CONST EFI_STRING_ID  HiiFormatStringId,
  IN CONST EFI_HANDLE     HiiFormatHandle,
  IN OUT VOID             **Response
  )
{
  CHAR16      *Prompt;
  EFI_STATUS  Status;

  Prompt = HiiGetString(HiiFormatHandle, HiiFormatStringId, NULL);
  Status = ShellPromptForResponse(Type, Prompt, Response);
  FreePool(Prompt);
  return (Status);
}

/**
  Function to determin if an entire string is a valid number.

  If Hex it must be preceeded with a 0x or has ForceHex, set TRUE.

  @param[in] String       The string to evaluate.
  @param[in] ForceHex     TRUE - always assume hex.
  @param[in] StopAtSpace  TRUE to halt upon finding a space, FALSE to keep going.
  @param[in] TimeNumbers        TRUE to allow numbers with ":", FALSE otherwise.

  @retval TRUE        It is all numeric (dec/hex) characters.
  @retval FALSE       There is a non-numeric character.
**/
BOOLEAN
EFIAPI
InternalShellIsHexOrDecimalNumber (
  IN CONST CHAR16   *String,
  IN CONST BOOLEAN  ForceHex,
  IN CONST BOOLEAN  StopAtSpace,
  IN CONST BOOLEAN  TimeNumbers
  )
{
  BOOLEAN Hex;

  //
  // chop off a single negative sign
  //
  if (String != NULL && *String == L'-') {
    String++;
  }

  if (String == NULL) {
    return (FALSE);
  }

  //
  // chop leading zeroes
  //
  while(String != NULL && *String == L'0'){
    String++;
  }
  //
  // allow '0x' or '0X', but not 'x' or 'X'
  //
  if (String != NULL && (*String == L'x' || *String == L'X')) {
    if (*(String-1) != L'0') {
      //
      // we got an x without a preceeding 0
      //
      return (FALSE);
    }
    String++;
    Hex = TRUE;
  } else if (ForceHex) {
    Hex = TRUE;
  } else {
    Hex = FALSE;
  }

  //
  // loop through the remaining characters and use the lib function
  //
  for ( ; String != NULL && *String != CHAR_NULL && !(StopAtSpace && *String == L' ') ; String++){
    if (TimeNumbers && (String[0] == L':')) {
      continue;
    }
    if (Hex) {
      if (!ShellIsHexaDecimalDigitCharacter(*String)) {
        return (FALSE);
      }
    } else {
      if (!ShellIsDecimalDigitCharacter(*String)) {
        return (FALSE);
      }
    }
  }

  return (TRUE);
}

/**
  Function to determine if a given filename exists.

  @param[in] Name         Path to test.

  @retval EFI_SUCCESS     The Path represents a file.
  @retval EFI_NOT_FOUND   The Path does not represent a file.
  @retval other           The path failed to open.
**/
EFI_STATUS
EFIAPI
ShellFileExists(
  IN CONST CHAR16 *Name
  )
{
  EFI_STATUS          Status;
  EFI_SHELL_FILE_INFO *List;

  ASSERT(Name != NULL);

  List = NULL;
  Status = ShellOpenFileMetaArg((CHAR16*)Name, EFI_FILE_MODE_READ, &List);
  if (EFI_ERROR(Status)) {
    return (Status);
  }

  ShellCloseFileMetaArg(&List);

  return (EFI_SUCCESS);
}

/**
  Convert a Unicode character to upper case only if
  it maps to a valid small-case ASCII character.

  This internal function only deal with Unicode character
  which maps to a valid small-case ASCII character, i.e.
  L'a' to L'z'. For other Unicode character, the input character
  is returned directly.

  @param  Char  The character to convert.

  @retval LowerCharacter   If the Char is with range L'a' to L'z'.
  @retval Unchanged        Otherwise.

**/
CHAR16
EFIAPI
InternalShellCharToUpper (
  IN      CHAR16                    Char
  )
{
  if (Char >= L'a' && Char <= L'z') {
    return (CHAR16) (Char - (L'a' - L'A'));
  }

  return Char;
}

/**
  Convert a Unicode character to numerical value.

  This internal function only deal with Unicode character
  which maps to a valid hexadecimal ASII character, i.e.
  L'0' to L'9', L'a' to L'f' or L'A' to L'F'. For other
  Unicode character, the value returned does not make sense.

  @param  Char  The character to convert.

  @return The numerical value converted.

**/
UINTN
EFIAPI
InternalShellHexCharToUintn (
  IN      CHAR16                    Char
  )
{
  if (ShellIsDecimalDigitCharacter (Char)) {
    return Char - L'0';
  }

  return (UINTN) (10 + InternalShellCharToUpper (Char) - L'A');
}

/**
  Convert a Null-terminated Unicode hexadecimal string to a value of type UINT64.

  This function returns a value of type UINT64 by interpreting the contents
  of the Unicode string specified by String as a hexadecimal number.
  The format of the input Unicode string String is:

                  [spaces][zeros][x][hexadecimal digits].

  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F].
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix.
  If "x" appears in the input string, it must be prefixed with at least one 0.
  The function will ignore the pad space, which includes spaces or tab characters,
  before [zeros], [x] or [hexadecimal digit]. The running zero before [x] or
  [hexadecimal digit] will be ignored. Then, the decoding starts after [x] or the
  first valid hexadecimal digit. Then, the function stops at the first character that is
  a not a valid hexadecimal character or NULL, whichever one comes first.

  If String has only pad spaces, then zero is returned.
  If String has no leading pad spaces, leading zeros or valid hexadecimal digits,
  then zero is returned.

  @param[in]  String      A pointer to a Null-terminated Unicode string.
  @param[out] Value       Upon a successful return the value of the conversion.
  @param[in] StopAtSpace  FALSE to skip spaces.

  @retval EFI_SUCCESS             The conversion was successful.
  @retval EFI_INVALID_PARAMETER   A parameter was NULL or invalid.
  @retval EFI_DEVICE_ERROR        An overflow occured.
**/
EFI_STATUS
EFIAPI
InternalShellStrHexToUint64 (
  IN CONST CHAR16   *String,
     OUT   UINT64   *Value,
  IN CONST BOOLEAN  StopAtSpace
  )
{
  UINT64    Result;

  if (String == NULL || StrSize(String) == 0 || Value == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == L' ') || (*String == L'\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == L'0') {
    String++;
  }

  if (InternalShellCharToUpper (*String) == L'X') {
    if (*(String - 1) != L'0') {
      return 0;
    }
    //
    // Skip the 'X'
    //
    String++;
  }

  Result = 0;

  //
  // there is a space where there should't be
  //
  if (*String == L' ') {
    return (EFI_INVALID_PARAMETER);
  }

  while (ShellIsHexaDecimalDigitCharacter (*String)) {
    //
    // If the Hex Number represented by String overflows according
    // to the range defined by UINT64, then return EFI_DEVICE_ERROR.
    //
    if (!(Result <= (RShiftU64((((UINT64) ~0) - InternalShellHexCharToUintn (*String)), 4)))) {
//    if (!(Result <= ((((UINT64) ~0) - InternalShellHexCharToUintn (*String)) >> 4))) {
      return (EFI_DEVICE_ERROR);
    }

    Result = (LShiftU64(Result, 4));
    Result += InternalShellHexCharToUintn (*String);
    String++;

    //
    // stop at spaces if requested
    //
    if (StopAtSpace && *String == L' ') {
      break;
    }
  }

  *Value = Result;
  return (EFI_SUCCESS);
}

/**
  Convert a Null-terminated Unicode decimal string to a value of
  type UINT64.

  This function returns a value of type UINT64 by interpreting the contents
  of the Unicode string specified by String as a decimal number. The format
  of the input Unicode string String is:

                  [spaces] [decimal digits].

  The valid decimal digit character is in the range [0-9]. The
  function will ignore the pad space, which includes spaces or
  tab characters, before [decimal digits]. The running zero in the
  beginning of [decimal digits] will be ignored. Then, the function
  stops at the first character that is a not a valid decimal character
  or a Null-terminator, whichever one comes first.

  If String has only pad spaces, then 0 is returned.
  If String has no pad spaces or valid decimal digits,
  then 0 is returned.

  @param[in]  String      A pointer to a Null-terminated Unicode string.
  @param[out] Value       Upon a successful return the value of the conversion.
  @param[in] StopAtSpace  FALSE to skip spaces.

  @retval EFI_SUCCESS             The conversion was successful.
  @retval EFI_INVALID_PARAMETER   A parameter was NULL or invalid.
  @retval EFI_DEVICE_ERROR        An overflow occured.
**/
EFI_STATUS
EFIAPI
InternalShellStrDecimalToUint64 (
  IN CONST CHAR16 *String,
     OUT   UINT64 *Value,
  IN CONST BOOLEAN  StopAtSpace
  )
{
  UINT64     Result;

  if (String == NULL || StrSize (String) == 0 || Value == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == L' ') || (*String == L'\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == L'0') {
    String++;
  }

  Result = 0;

  //
  // Stop upon space if requested 
  // (if the whole value was 0)
  //
  if (StopAtSpace && *String == L' ') {
    *Value = Result;
    return (EFI_SUCCESS);
  }

  while (ShellIsDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according
    // to the range defined by UINT64, then return EFI_DEVICE_ERROR.
    //

    if (!(Result <= (DivU64x32((((UINT64) ~0) - (*String - L'0')),10)))) {
      return (EFI_DEVICE_ERROR);
    }

    Result = MultU64x32(Result, 10) + (*String - L'0');
    String++;

    //
    // Stop at spaces if requested
    //
    if (StopAtSpace && *String == L' ') {
      break;
    }
  }

  *Value = Result;

  return (EFI_SUCCESS);
}

/**
  Function to verify and convert a string to its numerical value.

  If Hex it must be preceeded with a 0x, 0X, or has ForceHex set TRUE.

  @param[in] String       The string to evaluate.
  @param[out] Value       Upon a successful return the value of the conversion.
  @param[in] ForceHex     TRUE - always assume hex.
  @param[in] StopAtSpace  FALSE to skip spaces.

  @retval EFI_SUCCESS             The conversion was successful.
  @retval EFI_INVALID_PARAMETER   String contained an invalid character.
  @retval EFI_NOT_FOUND           String was a number, but Value was NULL.
**/
EFI_STATUS
EFIAPI
ShellConvertStringToUint64(
  IN CONST CHAR16   *String,
     OUT   UINT64   *Value,
  IN CONST BOOLEAN  ForceHex,
  IN CONST BOOLEAN  StopAtSpace
  )
{
  UINT64        RetVal;
  CONST CHAR16  *Walker;
  EFI_STATUS    Status;
  BOOLEAN       Hex;

  Hex = ForceHex;

  if (!InternalShellIsHexOrDecimalNumber(String, Hex, StopAtSpace, FALSE)) {
    if (!Hex) {
      Hex = TRUE;
      if (!InternalShellIsHexOrDecimalNumber(String, Hex, StopAtSpace, FALSE)) {
        return (EFI_INVALID_PARAMETER);
      }
    } else {
      return (EFI_INVALID_PARAMETER);
    }
  }

  //
  // Chop off leading spaces
  //
  for (Walker = String; Walker != NULL && *Walker != CHAR_NULL && *Walker == L' '; Walker++);

  //
  // make sure we have something left that is numeric.
  //
  if (Walker == NULL || *Walker == CHAR_NULL || !InternalShellIsHexOrDecimalNumber(Walker, Hex, StopAtSpace, FALSE)) {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // do the conversion.
  //
  if (Hex || StrnCmp(Walker, L"0x", 2) == 0 || StrnCmp(Walker, L"0X", 2) == 0){
    Status = InternalShellStrHexToUint64(Walker, &RetVal, StopAtSpace);
  } else {
    Status = InternalShellStrDecimalToUint64(Walker, &RetVal, StopAtSpace);
  }

  if (Value == NULL && !EFI_ERROR(Status)) {
    return (EFI_NOT_FOUND);
  }

  if (Value != NULL) {
    *Value = RetVal;
  }

  return (Status);
}

/**
  Function to determin if an entire string is a valid number.

  If Hex it must be preceeded with a 0x or has ForceHex, set TRUE.

  @param[in] String       The string to evaluate.
  @param[in] ForceHex     TRUE - always assume hex.
  @param[in] StopAtSpace  TRUE to halt upon finding a space, FALSE to keep going.

  @retval TRUE        It is all numeric (dec/hex) characters.
  @retval FALSE       There is a non-numeric character.
**/
BOOLEAN
EFIAPI
ShellIsHexOrDecimalNumber (
  IN CONST CHAR16   *String,
  IN CONST BOOLEAN  ForceHex,
  IN CONST BOOLEAN  StopAtSpace
  )
{
  if (ShellConvertStringToUint64(String, NULL, ForceHex, StopAtSpace) == EFI_NOT_FOUND) {
    return (TRUE);
  }
  return (FALSE);
}

/**
  Function to read a single line from a SHELL_FILE_HANDLE. The \n is not included in the returned
  buffer.  The returned buffer must be callee freed.

  If the position upon start is 0, then the Ascii Boolean will be set.  This should be
  maintained and not changed for all operations with the same file.

  @param[in]       Handle        SHELL_FILE_HANDLE to read from.
  @param[in, out]  Ascii         Boolean value for indicating whether the file is
                                 Ascii (TRUE) or UCS2 (FALSE).

  @return                        The line of text from the file.
  @retval NULL                   There was not enough memory available.

  @sa ShellFileHandleReadLine
**/
CHAR16*
EFIAPI
ShellFileHandleReturnLine(
  IN SHELL_FILE_HANDLE            Handle,
  IN OUT BOOLEAN                *Ascii
  )
{
  CHAR16          *RetVal;
  UINTN           Size;
  EFI_STATUS      Status;

  Size = 0;
  RetVal = NULL;

  Status = ShellFileHandleReadLine(Handle, RetVal, &Size, FALSE, Ascii);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    RetVal = AllocateZeroPool(Size);
    if (RetVal == NULL) {
      return (NULL);
    }
    Status = ShellFileHandleReadLine(Handle, RetVal, &Size, FALSE, Ascii);

  }
  if (EFI_ERROR(Status) && (RetVal != NULL)) {
    FreePool(RetVal);
    RetVal = NULL;
  }
  return (RetVal);
}

/**
  Function to read a single line (up to but not including the \n) from a SHELL_FILE_HANDLE.

  If the position upon start is 0, then the Ascii Boolean will be set.  This should be
  maintained and not changed for all operations with the same file.

  @param[in]       Handle        SHELL_FILE_HANDLE to read from.
  @param[in, out]  Buffer        The pointer to buffer to read into.
  @param[in, out]  Size          The pointer to number of bytes in Buffer.
  @param[in]       Truncate      If the buffer is large enough, this has no effect.
                                 If the buffer is is too small and Truncate is TRUE,
                                 the line will be truncated.
                                 If the buffer is is too small and Truncate is FALSE,
                                 then no read will occur.

  @param[in, out]  Ascii         Boolean value for indicating whether the file is
                                 Ascii (TRUE) or UCS2 (FALSE).

  @retval EFI_SUCCESS           The operation was successful.  The line is stored in
                                Buffer.
  @retval EFI_INVALID_PARAMETER Handle was NULL.
  @retval EFI_INVALID_PARAMETER Size was NULL.
  @retval EFI_BUFFER_TOO_SMALL  Size was not large enough to store the line.
                                Size was updated to the minimum space required.
**/
EFI_STATUS
EFIAPI
ShellFileHandleReadLine(
  IN SHELL_FILE_HANDLE          Handle,
  IN OUT CHAR16                 *Buffer,
  IN OUT UINTN                  *Size,
  IN BOOLEAN                    Truncate,
  IN OUT BOOLEAN                *Ascii
  )
{
  EFI_STATUS  Status;
  CHAR16      CharBuffer;
  UINTN       CharSize;
  UINTN       CountSoFar;
  UINT64      OriginalFilePosition;


  if (Handle == NULL
    ||Size   == NULL
   ){
    return (EFI_INVALID_PARAMETER);
  }
  if (Buffer == NULL) {
    ASSERT(*Size == 0);
  } else {
    *Buffer = CHAR_NULL;
  }
  gEfiShellProtocol->GetFilePosition(Handle, &OriginalFilePosition);
  if (OriginalFilePosition == 0) {
    CharSize = sizeof(CHAR16);
    Status = gEfiShellProtocol->ReadFile(Handle, &CharSize, &CharBuffer);
    ASSERT_EFI_ERROR(Status);
    if (CharBuffer == gUnicodeFileTag) {
      *Ascii = FALSE;
    } else {
      *Ascii = TRUE;
      gEfiShellProtocol->SetFilePosition(Handle, OriginalFilePosition);
    }
  }

  for (CountSoFar = 0;;CountSoFar++){
    CharBuffer = 0;
    if (*Ascii) {
      CharSize = sizeof(CHAR8);
    } else {
      CharSize = sizeof(CHAR16);
    }
    Status = gEfiShellProtocol->ReadFile(Handle, &CharSize, &CharBuffer);
    if (  EFI_ERROR(Status)
       || CharSize == 0
       || (CharBuffer == L'\n' && !(*Ascii))
       || (CharBuffer ==  '\n' && *Ascii)
     ){
      break;
    }
    //
    // if we have space save it...
    //
    if ((CountSoFar+1)*sizeof(CHAR16) < *Size){
      ASSERT(Buffer != NULL);
      ((CHAR16*)Buffer)[CountSoFar] = CharBuffer;
      ((CHAR16*)Buffer)[CountSoFar+1] = CHAR_NULL;
    }
  }

  //
  // if we ran out of space tell when...
  //
  if ((CountSoFar+1)*sizeof(CHAR16) > *Size){
    *Size = (CountSoFar+1)*sizeof(CHAR16);
    if (!Truncate) {
      gEfiShellProtocol->SetFilePosition(Handle, OriginalFilePosition);
    } else {
      DEBUG((DEBUG_WARN, "The line was truncated in ShellFileHandleReadLine"));
    }
    return (EFI_BUFFER_TOO_SMALL);
  }
  while(Buffer[StrLen(Buffer)-1] == L'\r') {
    Buffer[StrLen(Buffer)-1] = CHAR_NULL;
  }

  return (Status);
}

/**
  Function to print help file / man page content in the spec from the UEFI Shell protocol GetHelpText function.

  @param[in] CommandToGetHelpOn  Pointer to a string containing the command name of help file to be printed.
  @param[in] SectionToGetHelpOn  Pointer to the section specifier(s).
  @param[in] PrintCommandText    If TRUE, prints the command followed by the help content, otherwise prints 
                                 the help content only.
  @retval EFI_DEVICE_ERROR       The help data format was incorrect.
  @retval EFI_NOT_FOUND          The help data could not be found.
  @retval EFI_SUCCESS            The operation was successful.
**/
EFI_STATUS
EFIAPI
ShellPrintHelp (
  IN CONST CHAR16     *CommandToGetHelpOn,
  IN CONST CHAR16     *SectionToGetHelpOn,
  IN BOOLEAN          PrintCommandText
  )
{
	EFI_STATUS          Status;
	CHAR16              *OutText;
	  
	OutText = NULL;
	
  //
  // Get the string to print based
  //
	Status = gEfiShellProtocol->GetHelpText (CommandToGetHelpOn, SectionToGetHelpOn, &OutText);
  
  //
  // make sure we got a valid string
  //
  if (EFI_ERROR(Status)){
    return Status;
	} 
  if (OutText == NULL || StrLen(OutText) == 0) {
    return EFI_NOT_FOUND;  
	}
  
  //
  // Chop off trailing stuff we dont need
  //
  while (OutText[StrLen(OutText)-1] == L'\r' || OutText[StrLen(OutText)-1] == L'\n' || OutText[StrLen(OutText)-1] == L' ') {
    OutText[StrLen(OutText)-1] = CHAR_NULL;
  }
  
  //
  // Print this out to the console
  //
  if (PrintCommandText) {
    ShellPrintEx(-1, -1, L"%H%-14s%N- %s\r\n", CommandToGetHelpOn, OutText);
  } else {
    ShellPrintEx(-1, -1, L"%N%s\r\n", OutText);
  }
  
  SHELL_FREE_NON_NULL(OutText);

	return EFI_SUCCESS;
}

/**
  Function to delete a file by name
  
  @param[in]       FileName       Pointer to file name to delete.
  
  @retval EFI_SUCCESS             the file was deleted sucessfully
  @retval EFI_WARN_DELETE_FAILURE the handle was closed, but the file was not
                                  deleted
  @retval EFI_INVALID_PARAMETER   One of the parameters has an invalid value.
  @retval EFI_NOT_FOUND           The specified file could not be found on the
                                  device or the file system could not be found
                                  on the device.
  @retval EFI_NO_MEDIA            The device has no medium.
  @retval EFI_MEDIA_CHANGED       The device has a different medium in it or the
                                  medium is no longer supported.
  @retval EFI_DEVICE_ERROR        The device reported an error.
  @retval EFI_VOLUME_CORRUPTED    The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED     The file or medium is write protected.
  @retval EFI_ACCESS_DENIED       The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES    Not enough resources were available to open the
                                  file.
  @retval other                   The file failed to open
**/
EFI_STATUS
EFIAPI
ShellDeleteFileByName(
  IN CONST CHAR16               *FileName
  )
{
  EFI_STATUS                Status;
  SHELL_FILE_HANDLE         FileHandle;
  
  Status = ShellFileExists(FileName);
  
  if (Status == EFI_SUCCESS){
    Status = ShellOpenFileByName(FileName, &FileHandle, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0x0);
    if (Status == EFI_SUCCESS){
      Status = ShellDeleteFile(&FileHandle);
    }
  } 

  return(Status);
  
}

/**
  Cleans off all the quotes in the string.

  @param[in]     OriginalString   pointer to the string to be cleaned.
  @param[out]   CleanString      The new string with all quotes removed. 
                                                  Memory allocated in the function and free 
                                                  by caller.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
InternalShellStripQuotes (
  IN  CONST CHAR16     *OriginalString,
  OUT CHAR16           **CleanString
  )
{
  CHAR16            *Walker;
  
  if (OriginalString == NULL || CleanString == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *CleanString = AllocateCopyPool (StrSize (OriginalString), OriginalString);
  if (*CleanString == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Walker = *CleanString; Walker != NULL && *Walker != CHAR_NULL ; Walker++) {
    if (*Walker == L'\"') {
      CopyMem(Walker, Walker+1, StrSize(Walker) - sizeof(Walker[0]));
    }
  }

  return EFI_SUCCESS;
}

