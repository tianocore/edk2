/** @file
  Provides interface to shell functionality for shell commands and applications.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLib.h"

#define MAX_FILE_NAME_LEN 522 // (20 * (6+5+2))+1) unicode characters from EFI FAT spec (doubled for bytes)
#define FIND_XXXXX_FILE_BUFFER_SIZE (SIZE_OF_EFI_FILE_INFO + MAX_FILE_NAME_LEN)

//
// This is not static since it's extern in the .h file
//
SHELL_PARAM_ITEM EmptyParamList[] = {
  {NULL, TypeMax}
  };

//
// Static file globals for the shell library
//
STATIC EFI_SHELL_ENVIRONMENT2        *mEfiShellEnvironment2;
STATIC EFI_SHELL_INTERFACE           *mEfiShellInterface;
STATIC EFI_SHELL_PROTOCOL            *mEfiShellProtocol;
STATIC EFI_SHELL_PARAMETERS_PROTOCOL *mEfiShellParametersProtocol;
STATIC EFI_HANDLE                    mEfiShellEnvironment2Handle;
STATIC FILE_HANDLE_FUNCTION_MAP      FileFunctionMap;
STATIC UINTN                         mTotalParameterCount;
STATIC CHAR16                        *mPostReplaceFormat;
STATIC CHAR16                        *mPostReplaceFormat2;

/**
  Check if a Unicode character is a hexadecimal character.

  This internal function checks if a Unicode character is a
  decimal character.  The valid hexadecimal character is
  L'0' to L'9', L'a' to L'f', or L'A' to L'F'.


  @param  Char  The character to check against.

  @retval TRUE  If the Char is a hexadecmial character.
  @retval FALSE If the Char is not a hexadecmial character.

**/
BOOLEAN
EFIAPI
ShellIsHexaDecimalDigitCharacter (
  IN      CHAR16                    Char
  ) {
  return (BOOLEAN) ((Char >= L'0' && Char <= L'9') || (Char >= L'A' && Char <= L'F') || (Char >= L'a' && Char <= L'f'));
}

/**
  helper function to find ShellEnvironment2 for constructor
**/
EFI_STATUS
EFIAPI
ShellFindSE2 (
  IN EFI_HANDLE        ImageHandle
  ) {
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
  if (EFI_ERROR (Status) || !(CompareGuid (&mEfiShellEnvironment2->SESGuid, &gEfiShellEnvironment2ExtGuid) != FALSE)){
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
      Buffer = (EFI_HANDLE*)AllocatePool(BufferSize);
      ASSERT(Buffer != NULL);
      Status = gBS->LocateHandle (ByProtocol,
                                  &gEfiShellEnvironment2Guid,
                                  NULL, // ignored for ByProtocol
                                  &BufferSize,
                                  Buffer
                                  );
    }
    if (!EFI_ERROR (Status)) {
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
         if (CompareGuid (&mEfiShellEnvironment2->SESGuid, &gEfiShellEnvironment2ExtGuid) != FALSE) {
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

EFI_STATUS
EFIAPI
ShellLibConstructorWorker (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  ) {
  EFI_STATUS Status;

  ASSERT(PcdGet16 (PcdShellPrintBufferSize) < PcdGet32 (PcdMaximumUnicodeStringLength));
  mPostReplaceFormat = AllocateZeroPool (PcdGet16 (PcdShellPrintBufferSize));
  ASSERT (mPostReplaceFormat != NULL);
  mPostReplaceFormat2 = AllocateZeroPool (PcdGet16 (PcdShellPrintBufferSize));
  ASSERT (mPostReplaceFormat2 != NULL);

  //
  // Set the parameter count to an invalid number
  //
  mTotalParameterCount = (UINTN)(-1);

  //
  // UEFI 2.0 shell interfaces (used preferentially)
  //
  Status = gBS->OpenProtocol(ImageHandle,
                             &gEfiShellProtocolGuid,
                             (VOID **)&mEfiShellProtocol,
                             ImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_GET_PROTOCOL
                             );
  if (EFI_ERROR(Status)) {
    mEfiShellProtocol = NULL;
  }
  Status = gBS->OpenProtocol(ImageHandle,
                             &gEfiShellParametersProtocolGuid,
                             (VOID **)&mEfiShellParametersProtocol,
                             ImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_GET_PROTOCOL
                             );
  if (EFI_ERROR(Status)) {
    mEfiShellParametersProtocol = NULL;
  }

  if (mEfiShellParametersProtocol == NULL || mEfiShellProtocol == NULL) {
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
      (mEfiShellProtocol     != NULL && mEfiShellParametersProtocol != NULL)    ) {
    if (mEfiShellProtocol != NULL) {
      FileFunctionMap.GetFileInfo     = mEfiShellProtocol->GetFileInfo;
      FileFunctionMap.SetFileInfo     = mEfiShellProtocol->SetFileInfo;
      FileFunctionMap.ReadFile        = mEfiShellProtocol->ReadFile;
      FileFunctionMap.WriteFile       = mEfiShellProtocol->WriteFile;
      FileFunctionMap.CloseFile       = mEfiShellProtocol->CloseFile;
      FileFunctionMap.DeleteFile      = mEfiShellProtocol->DeleteFile;
      FileFunctionMap.GetFilePosition = mEfiShellProtocol->GetFilePosition;
      FileFunctionMap.SetFilePosition = mEfiShellProtocol->SetFilePosition;
      FileFunctionMap.FlushFile       = mEfiShellProtocol->FlushFile;
      FileFunctionMap.GetFileSize     = mEfiShellProtocol->GetFileSize;
    } else {
      FileFunctionMap.GetFileInfo     = FileHandleGetInfo;
      FileFunctionMap.SetFileInfo     = FileHandleSetInfo;
      FileFunctionMap.ReadFile        = FileHandleRead;
      FileFunctionMap.WriteFile       = FileHandleWrite;
      FileFunctionMap.CloseFile       = FileHandleClose;
      FileFunctionMap.DeleteFile      = FileHandleDelete;
      FileFunctionMap.GetFilePosition = FileHandleGetPosition;
      FileFunctionMap.SetFilePosition = FileHandleSetPosition;
      FileFunctionMap.FlushFile       = FileHandleFlush;
      FileFunctionMap.GetFileSize     = FileHandleGetSize;
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
  ) {


  mEfiShellEnvironment2       = NULL;
  mEfiShellProtocol           = NULL;
  mEfiShellParametersProtocol = NULL;
  mEfiShellInterface          = NULL;
  mEfiShellEnvironment2Handle = NULL;
  mPostReplaceFormat          = NULL;
  mPostReplaceFormat2         = NULL;

  //
  // verify that auto initialize is not set false
  //
  if (PcdGetBool(PcdShellLibAutoInitialize) == 0) {
    return (EFI_SUCCESS);
  }

  return (ShellLibConstructorWorker(ImageHandle, SystemTable));
}

/**
  Destructory for the library.  free any resources.
**/
EFI_STATUS
EFIAPI
ShellLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  ) {
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
  if (mEfiShellProtocol != NULL) {
    gBS->CloseProtocol(ImageHandle,
                       &gEfiShellProtocolGuid,
                       ImageHandle,
                       NULL);
    mEfiShellProtocol = NULL;
  }
  if (mEfiShellParametersProtocol != NULL) {
    gBS->CloseProtocol(ImageHandle,
                       &gEfiShellParametersProtocolGuid,
                       ImageHandle,
                       NULL);
    mEfiShellParametersProtocol = NULL;
  }
  mEfiShellEnvironment2Handle = NULL;

  if (mPostReplaceFormat != NULL) {
    FreePool(mPostReplaceFormat);
  }
  if (mPostReplaceFormat2 != NULL) {
    FreePool(mPostReplaceFormat2);
  }
  mPostReplaceFormat          = NULL;
  mPostReplaceFormat2         = NULL;

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
  ) {
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
  IN EFI_FILE_HANDLE            FileHandle
  ) {
  return (FileFunctionMap.GetFileInfo(FileHandle));
}

/**
  This function will set the information about the file for the opened handle
  specified.

  @param  FileHandle            The file handle of the file for which information
  is being set

  @param  FileInfo              The infotmation to set.

  @retval EFI_SUCCESS		The information was set.
  @retval EFI_UNSUPPORTED The InformationType is not known.
  @retval EFI_NO_MEDIA		The device has no medium.
  @retval EFI_DEVICE_ERROR	The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED	The file or medium is write protected.
  @retval EFI_ACCESS_DENIED	The file was opened read only.
  @retval EFI_VOLUME_FULL	The volume is full.
**/
EFI_STATUS
EFIAPI
ShellSetFileInfo (
  IN EFI_FILE_HANDLE  	        FileHandle,
  IN EFI_FILE_INFO              *FileInfo
  ) {
  return (FileFunctionMap.SetFileInfo(FileHandle, FileInfo));
}

  /**
  This function will open a file or directory referenced by DevicePath.

  This function opens a file with the open mode according to the file path. The
  Attributes is valid only for EFI_FILE_MODE_CREATE.

  @param  FilePath 		    on input the device path to the file.  On output
                          the remaining device path.
  @param  DeviceHandle  	pointer to the system device handle.
  @param  FileHandle		  pointer to the file handle.
  @param  OpenMode		    the mode to open the file with.
  @param  Attributes		  the file's file attributes.

  @retval EFI_SUCCESS		        The information was set.
  @retval EFI_INVALID_PARAMETER	One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED	      Could not open the file path.
  @retval EFI_NOT_FOUND	        The specified file could not be found on the
                                device or the file system could not be found on
                                the device.
  @retval EFI_NO_MEDIA		      The device has no medium.
  @retval EFI_MEDIA_CHANGED	    The device has a different medium in it or the
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR	    The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED	  The file or medium is write protected.
  @retval EFI_ACCESS_DENIED	The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES	Not enough resources were available to open the
                                file.
  @retval EFI_VOLUME_FULL	      The volume is full.
**/
EFI_STATUS
EFIAPI
ShellOpenFileByDevicePath(
  IN OUT EFI_DEVICE_PATH_PROTOCOL  	  **FilePath,
  OUT EFI_HANDLE                    	*DeviceHandle,
  OUT EFI_FILE_HANDLE               	*FileHandle,
  IN UINT64                          	OpenMode,
  IN UINT64                          	Attributes
  ) {
  CHAR16      *FileName;
  EFI_STATUS  Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *EfiSimpleFileSystemProtocol;
  EFI_FILE_HANDLE LastHandle;

  //
  // ASERT for FileHandle, FilePath, and DeviceHandle being NULL
  //
  ASSERT(FilePath != NULL);
  ASSERT(FileHandle != NULL);
  ASSERT(DeviceHandle != NULL);
  //
  // which shell interface should we use
  //
  if (mEfiShellProtocol != NULL) {
    //
    // use UEFI Shell 2.0 method.
    //
    FileName = mEfiShellProtocol->GetFilePathFromDevicePath(*FilePath);
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
  Status = EfiSimpleFileSystemProtocol->OpenVolume(EfiSimpleFileSystemProtocol, FileHandle);
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
    LastHandle  = *FileHandle;
    *FileHandle = NULL;

    //
    // Try to test opening an existing file
    //
    Status = LastHandle->Open (
                          LastHandle,
                          FileHandle,
                          ((FILEPATH_DEVICE_PATH*)*FilePath)->PathName,
                          OpenMode &~EFI_FILE_MODE_CREATE,
                          0
                          );

    //
    // see if the error was that it needs to be created
    //
    if ((EFI_ERROR (Status)) && (OpenMode != (OpenMode &~EFI_FILE_MODE_CREATE))) {
      Status = LastHandle->Open (
                            LastHandle,
                            FileHandle,
                            ((FILEPATH_DEVICE_PATH*)*FilePath)->PathName,
                            OpenMode,
                            Attributes
                            );
    }
    //
    // Close the last node
    //
    LastHandle->Close (LastHandle);

    if (EFI_ERROR(Status)) {
      return (Status);
    }

    //
    // Get the next node
    //
    *FilePath = NextDevicePathNode (*FilePath);
  }
  return (EFI_SUCCESS);
}

/**
  This function will open a file or directory referenced by filename.

  If return is EFI_SUCCESS, the Filehandle is the opened file's handle;
  otherwise, the Filehandle is NULL. The Attributes is valid only for
  EFI_FILE_MODE_CREATE.

  if FileNAme is NULL then ASSERT()

  @param  FileName 		  pointer to file name
  @param  FileHandle		pointer to the file handle.
  @param  OpenMode		  the mode to open the file with.
  @param  Attributes		the file's file attributes.

  @retval EFI_SUCCESS		        The information was set.
  @retval EFI_INVALID_PARAMETER	One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED	      Could not open the file path.
  @retval EFI_NOT_FOUND	        The specified file could not be found on the
                                device or the file system could not be found
                                on the device.
  @retval EFI_NO_MEDIA		      The device has no medium.
  @retval EFI_MEDIA_CHANGED	    The device has a different medium in it or the
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR	    The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED	  The file or medium is write protected.
  @retval EFI_ACCESS_DENIED	    The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES	Not enough resources were available to open the
                                file.
  @retval EFI_VOLUME_FULL	      The volume is full.
**/
EFI_STATUS
EFIAPI
ShellOpenFileByName(
  IN CONST CHAR16		            *FileName,
  OUT EFI_FILE_HANDLE           *FileHandle,
  IN UINT64                     OpenMode,
  IN UINT64                    	Attributes
  ) {
  EFI_HANDLE                    DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL      *FilePath;
  EFI_STATUS                    Status;
  EFI_FILE_INFO                 *FileInfo;

  //
  // ASSERT if FileName is NULL
  //
  ASSERT(FileName != NULL);

  if (mEfiShellProtocol != NULL) {
    //
    // Use UEFI Shell 2.0 method
    //
    Status = mEfiShellProtocol->OpenFileByName(FileName,
                                               FileHandle,
                                               OpenMode);
    if (!EFI_ERROR(Status) && ((OpenMode & EFI_FILE_MODE_CREATE) != 0)){
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
  if (FileDevicePath != NULL) {
    return (ShellOpenFileByDevicePath(&FilePath,
                                      &DeviceHandle,
                                      FileHandle,
                                      OpenMode,
                                      Attributes ));
  }
  return (EFI_DEVICE_ERROR);
}
/**
  This function create a directory

  If return is EFI_SUCCESS, the Filehandle is the opened directory's handle;
  otherwise, the Filehandle is NULL. If the directory already existed, this
  function opens the existing directory.

  @param  DirectoryName		pointer to directory name
  @param  FileHandle		  pointer to the file handle.

  @retval EFI_SUCCESS		        The information was set.
  @retval EFI_INVALID_PARAMETER	One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED	      Could not open the file path.
  @retval EFI_NOT_FOUND	        The specified file could not be found on the
                                device or the file system could not be found
                                on the device.
  @retval EFI_NO_MEDIA		      The device has no medium.
  @retval EFI_MEDIA_CHANGED	    The device has a different medium in it or the
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR	    The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED	  The file or medium is write protected.
  @retval EFI_ACCESS_DENIED	    The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES	Not enough resources were available to open the
                                file.
  @retval EFI_VOLUME_FULL	      The volume is full.
  @sa ShellOpenFileByName
**/
EFI_STATUS
EFIAPI
ShellCreateDirectory(
  IN CONST CHAR16             *DirectoryName,
  OUT EFI_FILE_HANDLE         *FileHandle
  ) {
  if (mEfiShellProtocol != NULL) {
    //
    // Use UEFI Shell 2.0 method
    //
    return (mEfiShellProtocol->CreateFile(DirectoryName,
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

  @retval EFI_SUCCESS	          Data was read.
  @retval EFI_NO_MEDIA	        The device has no media.
  @retval EFI_DEVICE_ERROR	The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_BUFFER_TO_SMALL	Buffer is too small. ReadSize contains required
                                size.

**/
EFI_STATUS
EFIAPI
ShellReadFile(
  IN EFI_FILE_HANDLE            FileHandle,
  IN OUT UINTN                  *BufferSize,
  OUT VOID                      *Buffer
  ) {
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

 @retval EFI_SUCCESS	        Data was written.
 @retval EFI_UNSUPPORTED	    Writes to an open directory are not supported.
 @retval EFI_NO_MEDIA	        The device has no media.
 @retval EFI_DEVICE_ERROR	    The device reported an error.
 @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
 @retval EFI_WRITE_PROTECTED	The device is write-protected.
 @retval EFI_ACCESS_DENIED	  The file was open for read only.
 @retval EFI_VOLUME_FULL	    The volume is full.
**/
EFI_STATUS
EFIAPI
ShellWriteFile(
  IN EFI_FILE_HANDLE            FileHandle,
  IN OUT UINTN                  *BufferSize,
  IN VOID                       *Buffer
  ) {
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
  IN EFI_FILE_HANDLE            *FileHandle
  ) {
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
  @retval INVALID_PARAMETER    	One of the parameters has an invalid value.
**/
EFI_STATUS
EFIAPI
ShellDeleteFile (
  IN EFI_FILE_HANDLE		*FileHandle
  ) {
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
  IN EFI_FILE_HANDLE   	FileHandle,
  IN UINT64           	Position
  ) {
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
  IN EFI_FILE_HANDLE            FileHandle,
  OUT UINT64                    *Position
  ) {
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
  IN EFI_FILE_HANDLE            FileHandle
  ) {
  return (FileFunctionMap.FlushFile(FileHandle));
}

/**
  Retrieves the first file from a directory

  This function opens a directory and gets the first file's info in the
  directory. Caller can use ShellFindNextFile() to get other files.  When
  complete the caller is responsible for calling FreePool() on Buffer.

  @param DirHandle              The file handle of the directory to search
  @param Buffer                 Pointer to buffer for file's information

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
  IN EFI_FILE_HANDLE            DirHandle,
  OUT EFI_FILE_INFO             **Buffer
  ) {
  //
  // pass to file handle lib
  //
  return (FileHandleFindFirstFile(DirHandle, Buffer));
}
/**
  Retrieves the next file in a directory.

  To use this function, caller must call the LibFindFirstFile() to get the
  first file, and then use this function get other files. This function can be
  called for several times to get each file's information in the directory. If
  the call of ShellFindNextFile() got the last file in the directory, the next
  call of this function has no file to get. *NoFile will be set to TRUE and the
  Buffer memory will be automatically freed.

  @param DirHandle              the file handle of the directory
  @param Buffer			            pointer to buffer for file's information
  @param NoFile			            pointer to boolean when last file is found

  @retval EFI_SUCCESS           Found the next file, or reached last file
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
**/
EFI_STATUS
EFIAPI
ShellFindNextFile(
  IN EFI_FILE_HANDLE             DirHandle,
  OUT EFI_FILE_INFO              *Buffer,
  OUT BOOLEAN                    *NoFile
  ) {
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
  IN EFI_FILE_HANDLE            FileHandle,
  OUT UINT64                    *Size
  ) {
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
  if (mEfiShellProtocol != NULL) {

    //
    // We are using UEFI Shell 2.0; see if the event has been triggered
    //
    if (gBS->CheckEvent(mEfiShellProtocol->ExecutionBreak) != EFI_SUCCESS) {
      return (FALSE);
    }
    return (TRUE);
  }

  //
  // using EFI Shell; call the function to check
  //
  ASSERT(mEfiShellEnvironment2 != NULL);
  return (mEfiShellEnvironment2->GetExecutionBreak());
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
  if (mEfiShellProtocol != NULL) {
    return (mEfiShellProtocol->GetEnv(EnvKey));
  }

  //
  // ASSERT that we must have EFI shell
  //
  ASSERT(mEfiShellEnvironment2 != NULL);

  //
  // using EFI Shell
  //
  return (mEfiShellEnvironment2->GetEnv((CHAR16*)EnvKey));
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
  if (mEfiShellProtocol != NULL) {
    return (mEfiShellProtocol->SetEnv(EnvKey, EnvVal, Volatile));
  }

  //
  // This feature does not exist under EFI shell
  //
  return (EFI_UNSUPPORTED);
}
/**
  cause the shell to parse and execute a command line.

  This function creates a nested instance of the shell and executes the specified
command (CommandLine) with the specified environment (Environment). Upon return,
the status code returned by the specified command is placed in StatusCode.
If Environment is NULL, then the current environment is used and all changes made
by the commands executed will be reflected in the current environment. If the
Environment is non-NULL, then the changes made will be discarded.
The CommandLine is executed from the current working directory on the current
device.

EnvironmentVariables and Status are only supported for UEFI Shell 2.0.
Output is only supported for pre-UEFI Shell 2.0

  @param ImageHandle            Parent image that is starting the operation
  @param CommandLine            pointer to NULL terminated command line.
  @param Output                 true to display debug output.  false to hide it.
  @param EnvironmentVariables   optional pointer to array of environment variables
                                in the form "x=y".  if NULL current set is used.
  @param Status                 the status of the run command line.

  @retval EFI_SUCCESS           the operation completed sucessfully.  Status
                                contains the status code returned.
  @retval EFI_INVALID_PARAMETER a parameter contains an invalid value
  @retval EFI_OUT_OF_RESOURCES  out of resources
  @retval EFI_UNSUPPORTED       the operation is not allowed.
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
  //
  // Check for UEFI Shell 2.0 protocols
  //
  if (mEfiShellProtocol != NULL) {
    //
    // Call UEFI Shell 2.0 version (not using Output parameter)
    //
    return (mEfiShellProtocol->Execute(ParentHandle,
                                      CommandLine,
                                      EnvironmentVariables,
                                      Status));
  }
  //
  // ASSERT that we must have EFI shell
  //
  ASSERT(mEfiShellEnvironment2 != NULL);
  //
  // Call EFI Shell version (not using EnvironmentVariables or Status parameters)
  // Due to oddity in the EFI shell we want to dereference the ParentHandle here
  //
  return (mEfiShellEnvironment2->Execute(*ParentHandle,
                                        CommandLine,
                                        Output));
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
  IN CHAR16                     *DeviceName OPTIONAL
  )
{
  //
  // Check for UEFI Shell 2.0 protocols
  //
  if (mEfiShellProtocol != NULL) {
    return (mEfiShellProtocol->GetCurDir(DeviceName));
  }
  //
  // ASSERT that we must have EFI shell
  //
  ASSERT(mEfiShellEnvironment2 != NULL);
  return (mEfiShellEnvironment2->CurDir(DeviceName));
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
    if (mEfiShellProtocol != NULL) {
      //
      // Enable with UEFI 2.0 Shell
      //
      mEfiShellProtocol->EnablePageBreak();
      return;
    } else {
      //
      // ASSERT that must have EFI Shell
      //
      ASSERT(mEfiShellEnvironment2 != NULL);
      //
      // Enable with EFI Shell
      //
      mEfiShellEnvironment2->EnablePageBreak (DEFAULT_INIT_ROW, DEFAULT_AUTO_LF);
      return;
    }
  } else {
    //
    // check for UEFI Shell 2.0
    //
    if (mEfiShellProtocol != NULL) {
      //
      // Disable with UEFI 2.0 Shell
      //
      mEfiShellProtocol->DisablePageBreak();
      return;
    } else {
      //
      // ASSERT that must have EFI Shell
      //
      ASSERT(mEfiShellEnvironment2 != NULL);
      //
      // Disable with EFI Shell
      //
      mEfiShellEnvironment2->DisablePageBreak ();
      return;
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
  EFI_FILE_HANDLE Handle;
  EFI_FILE_INFO *Info;
} EFI_SHELL_FILE_INFO_NO_CONST;

/**
  Converts a EFI shell list of structures to the coresponding UEFI Shell 2.0 type of list.

  if OldStyleFileList is NULL then ASSERT()

  this function will convert a SHELL_FILE_ARG based list into a callee allocated
  EFI_SHELL_FILE_INFO based list.  it is up to the caller to free the memory via
  the ShellCloseFileMetaArg function.

  @param[in] FileList           the EFI shell list type
  @param[in,out] ListHead      the list to add to

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

    //
    // make sure the old list was valid
    //
    ASSERT(OldInfo           != NULL);
    ASSERT(OldInfo->Info     != NULL);
    ASSERT(OldInfo->FullName != NULL);
    ASSERT(OldInfo->FileName != NULL);

    //
    // allocate a new EFI_SHELL_FILE_INFO object
    //
    NewInfo               = AllocateZeroPool(sizeof(EFI_SHELL_FILE_INFO));

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
    NewInfo->FullName     = AllocateZeroPool(StrSize(OldInfo->FullName));
    NewInfo->FileName     = AllocateZeroPool(StrSize(OldInfo->FileName));
    NewInfo->Info         = AllocateZeroPool((UINTN)OldInfo->Info->Size);

    //
    // make sure all the memory allocations were sucessful
    //
    ASSERT(NewInfo->FullName != NULL);
    ASSERT(NewInfo->FileName != NULL);
    ASSERT(NewInfo->Info     != NULL);

    //
    // Copt the strings and structure
    //
    StrCpy(NewInfo->FullName, OldInfo->FullName);
    StrCpy(NewInfo->FileName, OldInfo->FileName);
    gBS->CopyMem (NewInfo->Info, OldInfo->Info, (UINTN)OldInfo->Info->Size);

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
  file has a SHELL_FILE_ARG structure to record the file information. These
  structures are placed on the list ListHead. Users can get the SHELL_FILE_ARG
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
  #retval EFI_UNSUPPORTED       a previous ShellOpenFileMetaArg must be closed first.
                                *ListHead is set to NULL.
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

  //
  // ASSERT that Arg and ListHead are not NULL
  //
  ASSERT(Arg      != NULL);
  ASSERT(ListHead != NULL);

  //
  // Check for UEFI Shell 2.0 protocols
  //
  if (mEfiShellProtocol != NULL) {
    if (*ListHead == NULL) {
      *ListHead = (EFI_SHELL_FILE_INFO*)AllocateZeroPool(sizeof(EFI_SHELL_FILE_INFO));
      if (*ListHead == NULL) {
        return (EFI_OUT_OF_RESOURCES);
      }
      InitializeListHead(&((*ListHead)->Link));
    }
    Status = mEfiShellProtocol->OpenFileList(Arg,
                                           OpenMode,
                                           ListHead);
    if (EFI_ERROR(Status)) {
      mEfiShellProtocol->RemoveDupInFileList(ListHead);
    } else {
      Status = mEfiShellProtocol->RemoveDupInFileList(ListHead);
    }
    return (Status);
  }

  //
  // ASSERT that we must have EFI shell
  //
  ASSERT(mEfiShellEnvironment2 != NULL);

  //
  // make sure the list head is initialized
  //
  InitializeListHead(&mOldStyleFileList);

  //
  // Get the EFI Shell list of files
  //
  Status = mEfiShellEnvironment2->FileMetaArg(Arg, &mOldStyleFileList);
  if (EFI_ERROR(Status)) {
    *ListHead = NULL;
    return (Status);
  }

  if (*ListHead == NULL) {
    *ListHead = (EFI_SHELL_FILE_INFO    *)AllocateZeroPool(sizeof(EFI_SHELL_FILE_INFO));
    if (*ListHead == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
  }

  //
  // Convert that to equivalent of UEFI Shell 2.0 structure
  //
  InternalShellConvertFileListType(&mOldStyleFileList, &(*ListHead)->Link);

  //
  // Free the EFI Shell version that was converted.
  //
  mEfiShellEnvironment2->FreeFileList(&mOldStyleFileList);

  return (Status);
}
/**
  Free the linked list returned from ShellOpenFileMetaArg

  if ListHead is NULL then ASSERT()

  @param ListHead               the pointer to free

  @retval EFI_SUCCESS           the operation was sucessful
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
  if (mEfiShellProtocol != NULL) {
    return (mEfiShellProtocol->FreeFileList(ListHead));
  } else {
    //
    // Since this is EFI Shell version we need to free our internally made copy
    // of the list
    //
    for ( Node = GetFirstNode(&(*ListHead)->Link)
        ; IsListEmpty(&(*ListHead)->Link) == FALSE
        ; Node = GetFirstNode(&(*ListHead)->Link)) {
      RemoveEntryList(Node);
      ((EFI_SHELL_FILE_INFO_NO_CONST*)Node)->Handle->Close(((EFI_SHELL_FILE_INFO_NO_CONST*)Node)->Handle);
      FreePool(((EFI_SHELL_FILE_INFO_NO_CONST*)Node)->FullName);
      FreePool(((EFI_SHELL_FILE_INFO_NO_CONST*)Node)->FileName);
      FreePool(((EFI_SHELL_FILE_INFO_NO_CONST*)Node)->Info);
      FreePool((EFI_SHELL_FILE_INFO_NO_CONST*)Node);
    }
    return EFI_SUCCESS;
  }
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
  EFI_FILE_HANDLE   Handle;
  EFI_STATUS        Status;
  CHAR16            *RetVal;
  CHAR16            *TestPath;
  CONST CHAR16      *Walker;
  UINTN             Size;

  RetVal = NULL;

  Path = ShellGetEnvironmentVariable(L"cwd");
  if (Path != NULL) {
    Size = StrSize(Path);
    Size += StrSize(FileName);
    TestPath = AllocateZeroPool(Size);
    StrCpy(TestPath, Path);
    StrCat(TestPath, FileName);
    Status = ShellOpenFileByName(TestPath, &Handle, EFI_FILE_MODE_READ, 0);
    if (!EFI_ERROR(Status)){
      RetVal = StrnCatGrow(&RetVal, NULL, TestPath, 0);
      ShellCloseFile(&Handle);
      FreePool(TestPath);
      return (RetVal);
    }
    FreePool(TestPath);
  }
  Path = ShellGetEnvironmentVariable(L"path");
  if (Path != NULL) {
    Size = StrSize(Path);
    Size += StrSize(FileName);
    TestPath = AllocateZeroPool(Size);
    Walker = (CHAR16*)Path;
    do {
      CopyMem(TestPath, Walker, StrSize(Walker));
      if (StrStr(TestPath, L";") != NULL) {
        *(StrStr(TestPath, L";")) = CHAR_NULL;
      }
      StrCat(TestPath, FileName);
      if (StrStr(Walker, L";") != NULL) {
        Walker = StrStr(Walker, L";") + 1;
      } else {
        Walker = NULL;
      }
      Status = ShellOpenFileByName(TestPath, &Handle, EFI_FILE_MODE_READ, 0);
      if (!EFI_ERROR(Status)){
        RetVal = StrnCatGrow(&RetVal, NULL, TestPath, 0);
        ShellCloseFile(&Handle);
        break;
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
  for (ExtensionWalker = FileExtension ;  ; ExtensionWalker = StrStr(ExtensionWalker, L";") + 1 ){
    StrCpy(TestPath, FileName);
    StrCat(TestPath, ExtensionWalker);
    if (StrStr(TestPath, L";") != NULL) {
      *(StrStr(TestPath, L";")) = CHAR_NULL;
    }
    RetVal = ShellFindFilePath(TestPath);
    if (RetVal != NULL) {
      break;
    }
    //
    // Must be after first loop...
    //
    if (StrStr(ExtensionWalker, L";") == NULL) {
      break;
    }
  }
  FreePool(TestPath);
  return (RetVal);
}

typedef struct {
  LIST_ENTRY     Link;
  CHAR16         *Name;
  ParamType      Type;
  CHAR16         *Value;
  UINTN          OriginalPosition;
} SHELL_PARAM_PACKAGE;

/**
  Checks the list of valid arguments and returns TRUE if the item was found.  If the
  return value is TRUE then the type parameter is set also.

  if CheckList is NULL then ASSERT();
  if Name is NULL then ASSERT();
  if Type is NULL then ASSERT();

  @param Type                   pointer to type of parameter if it was found
  @param Name                   pointer to Name of parameter found
  @param CheckList              List to check against

  @retval TRUE                  the Parameter was found.  Type is valid.
  @retval FALSE                 the Parameter was not found.  Type is not valid.
**/
BOOLEAN
EFIAPI
InternalIsOnCheckList (
  IN CONST CHAR16               *Name,
  IN CONST SHELL_PARAM_ITEM     *CheckList,
  OUT ParamType                 *Type
  ) {
  SHELL_PARAM_ITEM              *TempListItem;

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
    if (TempListItem->Type == TypeStart && StrnCmp(Name, TempListItem->Name, StrLen(TempListItem->Name)) == 0) {
      *Type = TempListItem->Type;
      return (TRUE);
    } else if (StrCmp(Name, TempListItem->Name) == 0) {
      *Type = TempListItem->Type;
      return (TRUE);
    }
  }

  return (FALSE);
}
/**
  Checks the string for indicators of "flag" status.  this is a leading '/', '-', or '+'

  @param Name                   pointer to Name of parameter found

  @retval TRUE                  the Parameter is a flag.
  @retval FALSE                 the Parameter not a flag
**/
BOOLEAN
EFIAPI
InternalIsFlag (
  IN CONST CHAR16               *Name,
  IN BOOLEAN                    AlwaysAllowNumbers
  )
{
  //
  // ASSERT that Name isn't NULL
  //
  ASSERT(Name != NULL);

  //
  // If we accept numbers then dont return TRUE. (they will be values)
  //
  if (((Name[0] == L'-' || Name[0] == L'+') && ShellIsHexaDecimalDigitCharacter(Name[1])) && AlwaysAllowNumbers != FALSE) {
    return (FALSE);
  }

  //
  // If the Name has a / or - as the first character return TRUE
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

  @param CheckList              pointer to list of parameters to check
  @param CheckPackage           pointer to pointer to list checked values
  @param ProblemParam           optional pointer to pointer to unicode string for
                                the paramater that caused failure.  If used then the
                                caller is responsible for freeing the memory.
  @param AutoPageBreak          will automatically set PageBreakEnabled for "b" parameter
  @param Argc                   Count of parameters in Argv
  @param Argv                   pointer to array of parameters

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
STATIC
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
  ) {
  UINTN                         LoopCounter;
  ParamType                     CurrentItemType;
  SHELL_PARAM_PACKAGE           *CurrentItemPackage;
  UINTN                         GetItemValue;
  UINTN                         ValueSize;

  CurrentItemPackage = NULL;
  mTotalParameterCount = 0;
  GetItemValue = 0;
  ValueSize = 0;

  //
  // If there is only 1 item we dont need to do anything
  //
  if (Argc <= 1) {
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
  InitializeListHead(*CheckPackage);

  //
  // loop through each of the arguments
  //
  for (LoopCounter = 0 ; LoopCounter < Argc ; ++LoopCounter) {
    if (Argv[LoopCounter] == NULL) {
      //
      // do nothing for NULL argv
      //
    } else if (InternalIsOnCheckList(Argv[LoopCounter], CheckList, &CurrentItemType) != FALSE) {
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
      CurrentItemPackage = AllocatePool(sizeof(SHELL_PARAM_PACKAGE));
      ASSERT(CurrentItemPackage != NULL);
      CurrentItemPackage->Name  = AllocatePool(StrSize(Argv[LoopCounter]));
      ASSERT(CurrentItemPackage->Name != NULL);
      StrCpy(CurrentItemPackage->Name,  Argv[LoopCounter]);
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
    } else if (GetItemValue != 0 && InternalIsFlag(Argv[LoopCounter], AlwaysAllowNumbers) == FALSE) {
      ASSERT(CurrentItemPackage != NULL);
      //
      // get the item VALUE for a previous flag
      //
      CurrentItemPackage->Value = ReallocatePool(ValueSize, ValueSize + StrSize(Argv[LoopCounter]) + sizeof(CHAR16), CurrentItemPackage->Value);
      ASSERT(CurrentItemPackage->Value != NULL);
      if (ValueSize == 0) {
        StrCpy(CurrentItemPackage->Value, Argv[LoopCounter]);
      } else {
        StrCat(CurrentItemPackage->Value, L" ");
        StrCat(CurrentItemPackage->Value, Argv[LoopCounter]);
      }
      ValueSize += StrSize(Argv[LoopCounter]) + sizeof(CHAR16);
      GetItemValue--;
      if (GetItemValue == 0) {
        InsertHeadList(*CheckPackage, &CurrentItemPackage->Link);
      }
    } else if (InternalIsFlag(Argv[LoopCounter], AlwaysAllowNumbers) == FALSE) {
      //
      // add this one as a non-flag
      //
      CurrentItemPackage = AllocatePool(sizeof(SHELL_PARAM_PACKAGE));
      ASSERT(CurrentItemPackage != NULL);
      CurrentItemPackage->Name  = NULL;
      CurrentItemPackage->Type  = TypePosition;
      CurrentItemPackage->Value = AllocatePool(StrSize(Argv[LoopCounter]));
      ASSERT(CurrentItemPackage->Value != NULL);
      StrCpy(CurrentItemPackage->Value, Argv[LoopCounter]);
      CurrentItemPackage->OriginalPosition = mTotalParameterCount++;
      InsertHeadList(*CheckPackage, &CurrentItemPackage->Link);
    } else if (ProblemParam) {
      //
      // this was a non-recognised flag... error!
      //
      *ProblemParam = AllocatePool(StrSize(Argv[LoopCounter]));
      ASSERT(*ProblemParam != NULL);
      StrCpy(*ProblemParam, Argv[LoopCounter]);
      ShellCommandLineFreeVarList(*CheckPackage);
      *CheckPackage = NULL;
      return (EFI_VOLUME_CORRUPTED);
    } else {
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

  @param CheckList              pointer to list of parameters to check
  @param CheckPackage           pointer to pointer to list checked values
  @param ProblemParam           optional pointer to pointer to unicode string for
                                the paramater that caused failure.
  @param AutoPageBreak          will automatically set PageBreakEnabled for "b" parameter

  @retval EFI_SUCCESS           The operation completed sucessfully.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed
  @retval EFI_INVALID_PARAMETER A parameter was invalid
  @retval EFI_VOLUME_CORRUPTED  the command line was corrupt.  an argument was
                                duplicated.  the duplicated command line argument
                                was returned in ProblemParam if provided.
  @retval EFI_DEVICE_ERROR      the commands contained 2 opposing arguments.  one
                                of the command line arguments was returned in
                                ProblemParam if provided.
  @retval EFI_NOT_FOUND         a argument required a value that was missing.
                                the invalid command line argument was returned in
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
  ) {
  //
  // ASSERT that CheckList and CheckPackage aren't NULL
  //
  ASSERT(CheckList    != NULL);
  ASSERT(CheckPackage != NULL);

  //
  // Check for UEFI Shell 2.0 protocols
  //
  if (mEfiShellParametersProtocol != NULL) {
    return (InternalCommandLineParse(CheckList,
                                     CheckPackage,
                                     ProblemParam,
                                     AutoPageBreak,
                                     (CONST CHAR16**) mEfiShellParametersProtocol->Argv,
                                     mEfiShellParametersProtocol->Argc,
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
  ) {
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
      ; IsListEmpty(CheckPackage) == FALSE
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
  IN CONST LIST_ENTRY           *CheckPackage,
  IN CHAR16                     *KeyString
  ) {
  LIST_ENTRY                    *Node;

  //
  // ASSERT that both CheckPackage and KeyString aren't NULL
  //
  ASSERT(KeyString != NULL);

  //
  // return FALSE for no package
  //
  if (CheckPackage == NULL) {
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
      if ( ((SHELL_PARAM_PACKAGE*)Node)->Type == TypeStart
        && StrnCmp(KeyString, ((SHELL_PARAM_PACKAGE*)Node)->Name, StrLen(KeyString)) == 0
        ){
        return (TRUE);
      } else if (StrCmp(KeyString, ((SHELL_PARAM_PACKAGE*)Node)->Name) == 0) {
        return (TRUE);
      }
    }
  }
  return (FALSE);
}
/**
  returns value from command line argument

  value parameters are in the form of "-<Key> value" or "/<Key> value"

  if CheckPackage is NULL, then return NULL;

  @param CheckPackage           The package of parsed command line arguments
  @param KeyString              the Key of the command line argument to check for

  @retval NULL                  the flag is not on the command line
  @return !=NULL                pointer to unicode string of the value
  **/
CONST CHAR16*
EFIAPI
ShellCommandLineGetValue (
  IN CONST LIST_ENTRY           *CheckPackage,
  IN CHAR16                     *KeyString
  ) {
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
    // If the Name matches, return the value (name can be NULL)
    //
    if (((SHELL_PARAM_PACKAGE*)Node)->Name != NULL) {
      //
      // If Type is TypeStart then only compare the begining of the strings
      //
      if ( ((SHELL_PARAM_PACKAGE*)Node)->Type == TypeStart
        && StrnCmp(KeyString, ((SHELL_PARAM_PACKAGE*)Node)->Name, StrLen(KeyString)) == 0
        ){
        //
        // return the string part after the flag
        //
        return (((SHELL_PARAM_PACKAGE*)Node)->Name + StrLen(KeyString));
      } else if (StrCmp(KeyString, ((SHELL_PARAM_PACKAGE*)Node)->Name) == 0) {
        //
        // return the value
        //
        return (((SHELL_PARAM_PACKAGE*)Node)->Value);
      }
    }
  }
  return (NULL);
}
/**
  returns raw value from command line argument

  raw value parameters are in the form of "value" in a specific position in the list

  if CheckPackage is NULL, then return NULL;

  @param CheckPackage           The package of parsed command line arguments
  @param Position               the position of the value

  @retval NULL                  the flag is not on the command line
  @return !=NULL                pointer to unicode string of the value
  **/
CONST CHAR16*
EFIAPI
ShellCommandLineGetRawValue (
  IN CONST LIST_ENTRY           *CheckPackage,
  IN UINT32                     Position
  ) {
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

  @retval (UINTN)-1     No parsing has ocurred
  @return other         The number of value parameters found
**/
UINTN
EFIAPI
ShellCommandLineGetCount(
  VOID
  )
{
  return (mTotalParameterCount);
}

/**
  Determins if a parameter is duplicated.

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
      if (StrCmp(((SHELL_PARAM_PACKAGE*)Node1)->Name, ((SHELL_PARAM_PACKAGE*)Node2)->Name) == 0) {
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

  @param[in] SourceString             String with source buffer
  @param[in,out] NewString           String with resultant buffer
  @param[in] NewSize                  Size in bytes of NewString
  @param[in] FindTarget               String to look for
  @param[in[ ReplaceWith              String to replace FindTarget with
  @param[in] SkipPreCarrot            If TRUE will skip a FindTarget that has a '^'
                                      immediately before it.

  @retval EFI_INVALID_PARAMETER       SourceString was NULL.
  @retval EFI_INVALID_PARAMETER       NewString was NULL.
  @retval EFI_INVALID_PARAMETER       FindTarget was NULL.
  @retval EFI_INVALID_PARAMETER       ReplaceWith was NULL.
  @retval EFI_INVALID_PARAMETER       FindTarget had length < 1.
  @retval EFI_INVALID_PARAMETER       SourceString had length < 1.
  @retval EFI_BUFFER_TOO_SMALL        NewSize was less than the minimum size to hold
                                      the new string (truncation occurred).
  @retval EFI_SUCCESS                 the string was sucessfully copied with replacement.
**/

EFI_STATUS
EFIAPI
ShellCopySearchAndReplace2(
  IN CHAR16 CONST                     *SourceString,
  IN CHAR16                           *NewString,
  IN UINTN                            NewSize,
  IN CONST CHAR16                     *FindTarget,
  IN CONST CHAR16                     *ReplaceWith,
  IN CONST BOOLEAN                    SkipPreCarrot
  )
{
  UINTN Size;
  if ( (SourceString == NULL)
    || (NewString    == NULL)
    || (FindTarget   == NULL)
    || (ReplaceWith  == NULL)
    || (StrLen(FindTarget) < 1)
    || (StrLen(SourceString) < 1)
    ){
    return (EFI_INVALID_PARAMETER);
  }
  NewString = SetMem16(NewString, NewSize, CHAR_NULL);
  while (*SourceString != CHAR_NULL) {
    //
    // if we find the FindTarget and either Skip == FALSE or Skip == TRUE and we
    // dont have a carrot do a replace...
    //
    if (StrnCmp(SourceString, FindTarget, StrLen(FindTarget)) == 0
      && ((SkipPreCarrot && *(SourceString-1) != L'^') || SkipPreCarrot == FALSE)
      ){
      SourceString += StrLen(FindTarget);
      Size = StrSize(NewString);
      if ((Size + (StrLen(ReplaceWith)*sizeof(CHAR16))) > NewSize) {
        return (EFI_BUFFER_TOO_SMALL);
      }
      StrCat(NewString, ReplaceWith);
    } else {
      Size = StrSize(NewString);
      if (Size + sizeof(CHAR16) > NewSize) {
        return (EFI_BUFFER_TOO_SMALL);
      }
      StrnCat(NewString, SourceString, 1);
      SourceString++;
    }
  }
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
  if (mEfiShellParametersProtocol != NULL) {
    return (mEfiShellParametersProtocol->StdOut->Write(mEfiShellParametersProtocol->StdOut, &Size, (VOID*)String));
  }
  if (mEfiShellInterface          != NULL) {
    //
    // Divide in half for old shell.  Must be string length not size.
    //
    Size /= 2;
    return (         mEfiShellInterface->StdOut->Write(mEfiShellInterface->StdOut,          &Size, (VOID*)String));
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

  @param[in] Row        the row to print at
  @param[in] Col        the column to print at
  @param[in] Format     the format string
  @param[in] Marker     the marker for the variable argument list

  @return the number of characters printed to the screen
**/

UINTN
EFIAPI
InternalShellPrintWorker(
  IN INT32                Col OPTIONAL,
  IN INT32                Row OPTIONAL,
  IN CONST CHAR16         *Format,
  VA_LIST                 Marker
  )
{
  UINTN             Return;
  EFI_STATUS        Status;
  UINTN             NormalAttribute;
  CHAR16            *ResumeLocation;
  CHAR16            *FormatWalker;

  //
  // Back and forth each time fixing up 1 of our flags...
  //
  Status = ShellLibCopySearchAndReplace(Format,             mPostReplaceFormat,  PcdGet16 (PcdShellPrintBufferSize), L"%N", L"%%N");
  ASSERT_EFI_ERROR(Status);
  Status = ShellLibCopySearchAndReplace(mPostReplaceFormat,  mPostReplaceFormat2, PcdGet16 (PcdShellPrintBufferSize), L"%E", L"%%E");
  ASSERT_EFI_ERROR(Status);
  Status = ShellLibCopySearchAndReplace(mPostReplaceFormat2, mPostReplaceFormat,  PcdGet16 (PcdShellPrintBufferSize), L"%H", L"%%H");
  ASSERT_EFI_ERROR(Status);
  Status = ShellLibCopySearchAndReplace(mPostReplaceFormat,  mPostReplaceFormat2, PcdGet16 (PcdShellPrintBufferSize), L"%B", L"%%B");
  ASSERT_EFI_ERROR(Status);
  Status = ShellLibCopySearchAndReplace(mPostReplaceFormat2, mPostReplaceFormat,  PcdGet16 (PcdShellPrintBufferSize), L"%V", L"%%V");
  ASSERT_EFI_ERROR(Status);

  //
  // Use the last buffer from replacing to print from...
  //
  Return = UnicodeVSPrint (mPostReplaceFormat2, PcdGet16 (PcdShellPrintBufferSize), mPostReplaceFormat, Marker);

  if (Col != -1 && Row != -1) {
    Status = gST->ConOut->SetCursorPosition(gST->ConOut, Col, Row);
    ASSERT_EFI_ERROR(Status);
  }

  NormalAttribute = gST->ConOut->Mode->Attribute;
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
    Status = InternalPrintTo(FormatWalker);
    ASSERT_EFI_ERROR(Status);
    //
    // update the attribute
    //
    if (ResumeLocation != NULL) {
      switch (*(ResumeLocation+1)) {
        case (L'N'):
          gST->ConOut->SetAttribute(gST->ConOut, NormalAttribute);
          break;
        case (L'E'):
          gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_YELLOW, ((NormalAttribute&(BIT4|BIT5|BIT6))>>4)));
          break;
        case (L'H'):
          gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, ((NormalAttribute&(BIT4|BIT5|BIT6))>>4)));
          break;
        case (L'B'):
          gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_BLUE, ((NormalAttribute&(BIT4|BIT5|BIT6))>>4)));
          break;
        case (L'V'):
          gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, ((NormalAttribute&(BIT4|BIT5|BIT6))>>4)));
          break;
        default:
          //
          // Print a simple '%' symbol
          //
          Status = InternalPrintTo(L"%");
          ASSERT_EFI_ERROR(Status);
          ResumeLocation = ResumeLocation - 1;
          break;
      }
    } else {
      //
      // reset to normal now...
      //
      gST->ConOut->SetAttribute(gST->ConOut, NormalAttribute);
      break;
    }

    //
    // update FormatWalker to Resume + 2 (skip the % and the indicator)
    //
    FormatWalker = ResumeLocation + 2;
  }

  return (Return);
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

  @param[in] Row        the row to print at
  @param[in] Col        the column to print at
  @param[in] Format     the format string

  @return the number of characters printed to the screen
**/

UINTN
EFIAPI
ShellPrintEx(
  IN INT32                Col OPTIONAL,
  IN INT32                Row OPTIONAL,
  IN CONST CHAR16         *Format,
  ...
  )
{
  VA_LIST           Marker;
  EFI_STATUS        Status;
  VA_START (Marker, Format);
  Status = InternalShellPrintWorker(Col, Row, Format, Marker);
  VA_END(Marker);
  return(Status);
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

  @param[in] Row                The row to print at.
  @param[in] Col                The column to print at.
  @param[in] Language           The language of the string to retrieve.  If this parameter
                                is NULL, then the current platform language is used.
  @param[in] HiiFormatStringId  The format string Id for getting from Hii.
  @param[in] HiiFormatHandle    The format string Handle for getting from Hii.

  @return the number of characters printed to the screen.
**/
UINTN
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
  UINTN             RetVal;

  VA_START (Marker, HiiFormatHandle);
  HiiFormatString = HiiGetString(HiiFormatHandle, HiiFormatStringId, Language);
  ASSERT(HiiFormatString != NULL);

  RetVal = InternalShellPrintWorker(Col, Row, HiiFormatString, Marker);

  FreePool(HiiFormatString);
  VA_END(Marker);

  return (RetVal);
}

/**
  Function to determine if a given filename represents a file or a directory.

  @param[in] DirName      Path to directory to test.

  @retval EFI_SUCCESS     The Path represents a directory
  @retval EFI_NOT_FOUND   The Path does not represent a directory
  @return other           The path failed to open
**/
EFI_STATUS
EFIAPI
ShellIsDirectory(
  IN CONST CHAR16 *DirName
  )
{
  EFI_STATUS        Status;
  EFI_FILE_HANDLE   Handle;

  ASSERT(DirName != NULL);

  Handle = NULL;

  Status = ShellOpenFileByName(DirName, &Handle, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status)) {
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
  EFI_FILE_HANDLE   Handle;

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
  ) {
  CHAR16      *NewName;
  EFI_STATUS  Status;

  if (!EFI_ERROR(ShellIsFile(Name))) {
    return (TRUE);
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
  Function to determine whether a string is decimal or hex representation of a number
  and return the number converted from the string.

  @param[in] String   String representation of a number

  @retval all         the number
**/
UINTN
EFIAPI
ShellStrToUintn(
  IN CONST CHAR16 *String
  )
{
  CONST CHAR16  *Walker;
  for (Walker = String; Walker != NULL && *Walker != CHAR_NULL && *Walker == L' '; Walker++);
  if (StrnCmp(Walker, L"0x", 2) == 0 || StrnCmp(Walker, L"0X", 2) == 0){
    return (StrHexToUintn(Walker));
  }
  return (StrDecimalToUintn(Walker));
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

  @param[in,out] Destination   The String to append onto
  @param[in,out] CurrentSize   on call the number of bytes in Destination.  On
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
    while (NewSize < (DestinationStartSize + (Count*sizeof(CHAR16)))) {
      NewSize += 2 * Count * sizeof(CHAR16);
    }
    *Destination = ReallocatePool(*CurrentSize, NewSize, *Destination);
    *CurrentSize = NewSize;
  } else {
    *Destination = AllocateZeroPool((Count+1)*sizeof(CHAR16));
  }

  //
  // Now use standard StrnCat on a big enough buffer
  //
  return StrnCat(*Destination, Source, Count);
}
