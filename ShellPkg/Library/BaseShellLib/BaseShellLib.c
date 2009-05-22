/** @file
  Provides interface to shell functionality for shell commands and applications.

Copyright (c) 2006 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/FileHandleLib.h>
#include <Protocol/EfiShellEnvironment2.h>
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellParameters.h>
#include <Protocol/SimpleFileSystem.h>

#include "BaseShellLib.h"

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

/**
  helper function to find ShellEnvironment2 for constructor
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
  if (EFI_ERROR (Status) || !(CompareGuid (&mEfiShellEnvironment2->SESGuid, &gEfiShellEnvironment2ExtGuid) != FALSE &&
     (mEfiShellEnvironment2->MajorVersion > EFI_SHELL_MAJOR_VER ||
     (mEfiShellEnvironment2->MajorVersion == EFI_SHELL_MAJOR_VER && mEfiShellEnvironment2->MinorVersion >= EFI_SHELL_MINOR_VER)))) {
    //
    // figure out how big of a buffer we need.
    //
    Status = gBS->LocateHandle (ByProtocol,
                                &gEfiShellEnvironment2Guid,
                                NULL, // ignored for ByProtocol
                                &BufferSize,
                                Buffer
                                );
    ASSERT(Status == EFI_BUFFER_TOO_SMALL);
    Buffer = (EFI_HANDLE*)AllocatePool(BufferSize);
    ASSERT(Buffer != NULL);
    Status = gBS->LocateHandle (ByProtocol,
                                &gEfiShellEnvironment2Guid,
                                NULL, // ignored for ByProtocol
                                &BufferSize,
                                Buffer
                                );
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
         if (CompareGuid (&mEfiShellEnvironment2->SESGuid, &gEfiShellEnvironment2ExtGuid) != FALSE &&
          (mEfiShellEnvironment2->MajorVersion > EFI_SHELL_MAJOR_VER ||
          (mEfiShellEnvironment2->MajorVersion == EFI_SHELL_MAJOR_VER && mEfiShellEnvironment2->MinorVersion >= EFI_SHELL_MINOR_VER))) {
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
){
  EFI_STATUS Status;

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
  )
{


  mEfiShellEnvironment2       = NULL;
  mEfiShellProtocol           = NULL;
  mEfiShellParametersProtocol = NULL;
  mEfiShellInterface          = NULL;
  mEfiShellEnvironment2Handle = NULL;

  ///@todo make a worker constructor so initialize function works
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
  )
{
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
  )
{
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
  )
{
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
                             &EfiSimpleFileSystemProtocol,
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
  IN CHAR16		                  *FileName,
  OUT EFI_FILE_HANDLE           *FileHandle,
  IN UINT64                     OpenMode,
  IN UINT64                    	Attributes
  )
{
  EFI_HANDLE                    DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL      *FilePath;

  //
  // ASSERT if FileName is NULL
  //
  ASSERT(FileName != NULL);

  if (mEfiShellProtocol != NULL) {
    //
    // Use UEFI Shell 2.0 method
    //
    return (mEfiShellProtocol->OpenFileByName(FileName,
                                             FileHandle,
                                             OpenMode));

    ///@todo add the attributes
  } 
  //
  // Using EFI Shell version
  // this means convert name to path and call that function
  // since this will use EFI method again that will open it.
  //
  ASSERT(mEfiShellEnvironment2 != NULL);
  FilePath = mEfiShellEnvironment2->NameToPath (FileName);
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
  IN CHAR16                   *DirectoryName,
  OUT EFI_FILE_HANDLE         *FileHandle
  )
{
  //
  // this is a pass thru to the open file function with sepcific open mode and attributes
  //
  return (ShellOpenFileByName(DirectoryName,
                              FileHandle,
                              EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                              EFI_FILE_DIRECTORY
                              ));
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
  IN EFI_FILE_HANDLE            *FileHandle
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
  @retval INVALID_PARAMETER    	One of the parameters has an invalid value.
**/
EFI_STATUS
EFIAPI
ShellDeleteFile (
  IN EFI_FILE_HANDLE		*FileHandle
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
  IN EFI_FILE_HANDLE   	FileHandle,
  IN UINT64           	Position
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
  IN EFI_FILE_HANDLE            FileHandle,
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
  IN EFI_FILE_HANDLE            FileHandle
  )
{
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
  )
{
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
  IN EFI_FILE_HANDLE            FileHandle,
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
  IN CHAR16                     *EnvKey
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
  return (mEfiShellEnvironment2->GetEnv(EnvKey));
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
  @param CommandLine            pointer to null terminated command line.
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

  @param FileList               the EFI shell list type

  @retval the resultant head of the double linked new format list;
**/
LIST_ENTRY*
EFIAPI
InternalShellConvertFileListType (
  LIST_ENTRY                *FileList
  )
{
  LIST_ENTRY                    *ListHead;
  SHELL_FILE_ARG                *OldInfo;
  LIST_ENTRY                *Link;
  EFI_SHELL_FILE_INFO_NO_CONST  *NewInfo;

  //
  // ASSERT that FileList is not NULL
  //
  ASSERT(FileList != NULL);

  //
  // Allocate our list head and initialize the list
  //
  ListHead = AllocateZeroPool(sizeof(LIST_ENTRY));
  ASSERT (ListHead != NULL);
  ListHead = InitializeListHead (ListHead);

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
    InsertTailList(ListHead, (LIST_ENTRY*)NewInfo);
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

  This function will fail if called sequentially without freeing the list in the middle.

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
  LIST_ENTRY                    *EmptyNode;
  LIST_ENTRY                    *mOldStyleFileList;
  
  //
  // ASSERT that Arg and ListHead are not NULL
  //
  ASSERT(Arg      != NULL);
  ASSERT(ListHead != NULL);

  // 
  // Check for UEFI Shell 2.0 protocols
  //
  if (mEfiShellProtocol != NULL) {
    return (mEfiShellProtocol->OpenFileList(Arg, 
                                           OpenMode, 
                                           ListHead));
  } 

  //
  // ASSERT that we must have EFI shell
  //
  ASSERT(mEfiShellEnvironment2 != NULL);

  //
  // allocate memory for old list head
  //
  mOldStyleFileList = (LIST_ENTRY*)AllocatePool(sizeof(LIST_ENTRY));
  ASSERT(mOldStyleFileList != NULL);

  //
  // make sure the list head is initialized
  //
  InitializeListHead((LIST_ENTRY*)mOldStyleFileList);

  //
  // Get the EFI Shell list of files
  //
  Status = mEfiShellEnvironment2->FileMetaArg(Arg, mOldStyleFileList);
  if (EFI_ERROR(Status)) {
    *ListHead = NULL;
    return (Status);
  }

  //
  // Convert that to equivalent of UEFI Shell 2.0 structure
  //
  EmptyNode = InternalShellConvertFileListType(mOldStyleFileList);

  //
  // Free the EFI Shell version that was converted.
  //
  ASSERT_EFI_ERROR(mEfiShellEnvironment2->FreeFileList(mOldStyleFileList));
  FreePool(mOldStyleFileList);
  mOldStyleFileList = NULL;

  //
  // remove the empty head of the list
  //
  *ListHead = (EFI_SHELL_FILE_INFO*)RemoveEntryList(EmptyNode);
  FreePool(EmptyNode);  

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
    for (Node = GetFirstNode((LIST_ENTRY*)*ListHead) ; IsListEmpty((LIST_ENTRY*)*ListHead) == FALSE ; Node = GetFirstNode((LIST_ENTRY*)*ListHead)) {
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

typedef struct {
  LIST_ENTRY List;
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
  )
{
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
    // If the Name matches set the type and return TRUE
    //
    if (StrCmp(Name, TempListItem->Name) == 0) {
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
  IN CONST CHAR16               *Name
  )
{
  //
  // ASSERT that Name isn't NULL
  //
  ASSERT(Name != NULL);

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
EFI_STATUS
EFIAPI
InternalCommandLineParse (
  IN CONST SHELL_PARAM_ITEM     *CheckList,
  OUT LIST_ENTRY                **CheckPackage,
  OUT CHAR16                    **ProblemParam OPTIONAL,
  IN BOOLEAN                    AutoPageBreak,
  IN CONST CHAR16               **Argv,
  IN UINTN                      Argc
  )
{
  UINTN                         LoopCounter;
  UINTN                         Count;
  ParamType                     CurrentItemType;
  SHELL_PARAM_PACKAGE           *CurrentItemPackage;
  BOOLEAN                       GetItemValue;

  CurrentItemPackage = NULL;

  //
  // ASSERTs
  //
  ASSERT(CheckList  != NULL);
  ASSERT(Argv       != NULL);

  Count = 0;
  GetItemValue = FALSE;

  //
  // If there is only 1 item we dont need to do anything
  //
  if (Argc <= 1) {
    *CheckPackage = NULL;
    return (EFI_SUCCESS);
  }

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
    } else if (GetItemValue == TRUE) {
      ASSERT(CurrentItemPackage != NULL);
      //
      // get the item VALUE for the previous flag
      //
      GetItemValue = FALSE;
      CurrentItemPackage->Value = AllocateZeroPool(StrSize(Argv[LoopCounter]));
      ASSERT(CurrentItemPackage->Value != NULL);
      StrCpy(CurrentItemPackage->Value, Argv[LoopCounter]);
      InsertTailList(*CheckPackage, (LIST_ENTRY*)CurrentItemPackage);
    } else if (InternalIsFlag(Argv[LoopCounter]) == FALSE) {
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
      CurrentItemPackage->OriginalPosition = Count++;
      InsertTailList(*CheckPackage, (LIST_ENTRY*)CurrentItemPackage);
    } else if (InternalIsOnCheckList(Argv[LoopCounter], CheckList, &CurrentItemType) == TRUE) {
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

      //
      // Does this flag require a value
      //
      if (CurrentItemPackage->Type == TypeValue) {
        //
        // trigger the next loop to populate the value of this item
        //
        GetItemValue = TRUE; 
      } else {
        //
        // this item has no value expected; we are done
        //
        CurrentItemPackage->Value = NULL;
        InsertTailList(*CheckPackage, (LIST_ENTRY*)CurrentItemPackage);
      }
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
ShellCommandLineParse (
  IN CONST SHELL_PARAM_ITEM     *CheckList,
  OUT LIST_ENTRY                **CheckPackage,
  OUT CHAR16                    **ProblemParam OPTIONAL,
  IN BOOLEAN                    AutoPageBreak
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
  if (mEfiShellParametersProtocol != NULL) {
    return (InternalCommandLineParse(CheckList, 
                                     CheckPackage, 
                                     ProblemParam, 
                                     AutoPageBreak, 
                                     mEfiShellParametersProtocol->Argv,
                                     mEfiShellParametersProtocol->Argc ));
  }

  // 
  // ASSERT That EFI Shell is not required
  //
  ASSERT (mEfiShellInterface != NULL);
  return (InternalCommandLineParse(CheckList, 
                                   CheckPackage, 
                                   ProblemParam, 
                                   AutoPageBreak, 
                                   mEfiShellInterface->Argv,
                                   mEfiShellInterface->Argc ));
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
  for (Node = GetFirstNode(CheckPackage); Node != CheckPackage ; Node = GetFirstNode(CheckPackage)) {
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
  )
{
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
  for (Node = GetFirstNode(CheckPackage) ; !IsNull (CheckPackage, Node) ; Node = GetNextNode(CheckPackage, Node) ) {
    //
    // If the Name matches, return TRUE (and there may be NULL name)
    //
    if (((SHELL_PARAM_PACKAGE*)Node)->Name != NULL) {
      if (StrCmp(KeyString, ((SHELL_PARAM_PACKAGE*)Node)->Name) == 0) {
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
  for (Node = GetFirstNode(CheckPackage) ; !IsNull (CheckPackage, Node) ; Node = GetNextNode(CheckPackage, Node) ) {
    //
    // If the Name matches, return the value (name can be NULL)
    //
    if (((SHELL_PARAM_PACKAGE*)Node)->Name != NULL) {
      if (StrCmp(KeyString, ((SHELL_PARAM_PACKAGE*)Node)->Name) == 0) {
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
  for (Node = GetFirstNode(CheckPackage) ; !IsNull (CheckPackage, Node) ; Node = GetNextNode(CheckPackage, Node) ) {
    //
    // If the position matches, return the value
    //
    if (((SHELL_PARAM_PACKAGE*)Node)->OriginalPosition == Position) {
      return (((SHELL_PARAM_PACKAGE*)Node)->Value);
    }
  }
  return (NULL);
}