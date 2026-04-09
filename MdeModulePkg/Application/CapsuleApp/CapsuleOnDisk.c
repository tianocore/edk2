/** @file
  Process Capsule On Disk.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CapsuleApp.h"

EFI_GUID  mCapsuleOnDiskBootOptionGuid = {
  0x4CC29BB7, 0x2413, 0x40A2, { 0xB0, 0x6D, 0x25, 0x3E, 0x37, 0x10, 0xF5, 0x32 }
};

/**
  Get file name from file path.

  @param  FilePath    File path.

  @return Pointer to file name.

**/
CHAR16 *
GetFileNameFromPath (
  CHAR16  *FilePath
  )
{
  EFI_STATUS          Status;
  EFI_SHELL_PROTOCOL  *ShellProtocol;
  SHELL_FILE_HANDLE   Handle;
  EFI_FILE_INFO       *FileInfo;

  ShellProtocol = GetShellProtocol ();
  if (ShellProtocol == NULL) {
    return NULL;
  }

  //
  // Open file by FileName.
  //
  Status = ShellProtocol->OpenFileByName (
                            FilePath,
                            &Handle,
                            EFI_FILE_MODE_READ
                            );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get file name from EFI_FILE_INFO.
  //
  FileInfo = ShellProtocol->GetFileInfo (Handle);
  ShellProtocol->CloseFile (Handle);
  if (FileInfo == NULL) {
    return NULL;
  }

  return FileInfo->FileName;
}

/**
  Check if the device path is EFI system Partition.

  @param  DevicePath    The ESP device path.

  @retval TRUE    DevicePath is a device path for ESP.
  @retval FALSE   DevicePath is not a device path for ESP.

**/
BOOLEAN
IsEfiSysPartitionDevicePath (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  HARDDRIVE_DEVICE_PATH     *Hd;
  EFI_HANDLE                Handle;

  //
  // Check if the device path contains GPT node
  //
  TempDevicePath = DevicePath;

  while (!IsDevicePathEnd (TempDevicePath)) {
    if ((DevicePathType (TempDevicePath) == MEDIA_DEVICE_PATH) &&
        (DevicePathSubType (TempDevicePath) == MEDIA_HARDDRIVE_DP))
    {
      Hd = (HARDDRIVE_DEVICE_PATH *)TempDevicePath;
      if (Hd->MBRType == MBR_TYPE_EFI_PARTITION_TABLE_HEADER) {
        break;
      }
    }

    TempDevicePath = NextDevicePathNode (TempDevicePath);
  }

  if (!IsDevicePathEnd (TempDevicePath)) {
    //
    // Search for EFI system partition protocol on full device path in Boot Option
    //
    Status = gBS->LocateDevicePath (&gEfiPartTypeSystemPartGuid, &DevicePath, &Handle);
    return EFI_ERROR (Status) ? FALSE : TRUE;
  } else {
    return FALSE;
  }
}

/**
  Dump all EFI System Partition.

**/
VOID
DumpAllEfiSysPartition (
  VOID
  )
{
  EFI_HANDLE                *SimpleFileSystemHandles;
  UINTN                     NumberSimpleFileSystemHandles;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     NumberEfiSystemPartitions;
  EFI_SHELL_PROTOCOL        *ShellProtocol;

  NumberEfiSystemPartitions = 0;

  ShellProtocol = GetShellProtocol ();
  if (ShellProtocol == NULL) {
    Print (L"Get Shell Protocol Fail\n");
    return;
  }

  Print (L"EFI System Partition list:\n");

  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiSimpleFileSystemProtocolGuid,
         NULL,
         &NumberSimpleFileSystemHandles,
         &SimpleFileSystemHandles
         );

  for (Index = 0; Index < NumberSimpleFileSystemHandles; Index++) {
    DevicePath = DevicePathFromHandle (SimpleFileSystemHandles[Index]);
    if (IsEfiSysPartitionDevicePath (DevicePath)) {
      NumberEfiSystemPartitions++;
      Print (L"    %s\n        %s\n", ShellProtocol->GetMapFromDevicePath (&DevicePath), ConvertDevicePathToText (DevicePath, TRUE, TRUE));
    }
  }

  if (NumberEfiSystemPartitions == 0) {
    Print (L"    No ESP found.\n");
  }
}

/**
  Check if capsule is provisioned.

  @retval TRUE    Capsule is provisioned previously.
  @retval FALSE   No capsule is provisioned.

**/
BOOLEAN
IsCapsuleProvisioned (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT64      OsIndication;
  UINTN       DataSize;

  OsIndication = 0;
  DataSize     = sizeof (UINT64);
  Status       = gRT->GetVariable (
                        L"OsIndications",
                        &gEfiGlobalVariableGuid,
                        NULL,
                        &DataSize,
                        &OsIndication
                        );
  if (!EFI_ERROR (Status) &&
      ((OsIndication & EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED) != 0))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Get one active Efi System Partition.

  @param[out] FsDevicePath   The device path of Fs
  @param[out] Fs             The file system within EfiSysPartition

  @retval EFI_SUCCESS     Get file system successfully
  @retval EFI_NOT_FOUND   No valid file system found

**/
EFI_STATUS
GetEfiSysPartition (
  OUT EFI_DEVICE_PATH_PROTOCOL         **FsDevicePath,
  OUT EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  **Fs
  )
{
  EFI_HANDLE                *SimpleFileSystemHandles;
  UINTN                     NumberSimpleFileSystemHandles;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_STATUS                Status;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NumberSimpleFileSystemHandles,
                  &SimpleFileSystemHandles
                  );

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < NumberSimpleFileSystemHandles; Index++) {
    DevicePath = DevicePathFromHandle (SimpleFileSystemHandles[Index]);
    if (IsEfiSysPartitionDevicePath (DevicePath)) {
      Status = gBS->HandleProtocol (SimpleFileSystemHandles[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID **)Fs);
      if (!EFI_ERROR (Status)) {
        *FsDevicePath = DevicePath;
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Check if Active Efi System Partition within GPT is in the device path.

  @param[in]  DevicePath     The device path
  @param[out] FsDevicePath   The device path of Fs
  @param[out] Fs             The file system within EfiSysPartition

  @retval EFI_SUCCESS    Get file system successfully
  @retval EFI_NOT_FOUND  No valid file system found
  @retval others         Get file system failed

**/
EFI_STATUS
GetEfiSysPartitionFromDevPath (
  IN  EFI_DEVICE_PATH_PROTOCOL         *DevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL         **FsDevicePath,
  OUT EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  **Fs
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  HARDDRIVE_DEVICE_PATH     *Hd;
  EFI_HANDLE                Handle;

  //
  // Check if the device path contains GPT node
  //
  TempDevicePath = DevicePath;
  while (!IsDevicePathEnd (TempDevicePath)) {
    if ((DevicePathType (TempDevicePath) == MEDIA_DEVICE_PATH) &&
        (DevicePathSubType (TempDevicePath) == MEDIA_HARDDRIVE_DP))
    {
      Hd = (HARDDRIVE_DEVICE_PATH *)TempDevicePath;
      if (Hd->MBRType == MBR_TYPE_EFI_PARTITION_TABLE_HEADER) {
        break;
      }
    }

    TempDevicePath = NextDevicePathNode (TempDevicePath);
  }

  if (!IsDevicePathEnd (TempDevicePath)) {
    //
    // Search for EFI system partition protocol on full device path in Boot Option
    //
    Status = gBS->LocateDevicePath (&gEfiPartTypeSystemPartGuid, &DevicePath, &Handle);

    //
    // Search for simple file system on this handler
    //
    if (!EFI_ERROR (Status)) {
      Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)Fs);
      if (!EFI_ERROR (Status)) {
        *FsDevicePath = DevicePathFromHandle (Handle);
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Get SimpleFileSystem from boot option file path.

  @param[in]  DevicePath     The file path of boot option
  @param[out] FullPath       The full device path of boot device
  @param[out] Fs             The file system within EfiSysPartition

  @retval EFI_SUCCESS    Get file system successfully
  @retval EFI_NOT_FOUND  No valid file system found
  @retval others         Get file system failed

**/
EFI_STATUS
GetEfiSysPartitionFromBootOptionFilePath (
  IN  EFI_DEVICE_PATH_PROTOCOL         *DevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL         **FullPath,
  OUT EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  **Fs
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *CurFullPath;
  EFI_DEVICE_PATH_PROTOCOL  *PreFullPath;
  EFI_DEVICE_PATH_PROTOCOL  *FsFullPath;

  CurFullPath = NULL;
  FsFullPath  = NULL;
  //
  // Try every full device Path generated from bootoption
  //
  do {
    PreFullPath = CurFullPath;
    CurFullPath = EfiBootManagerGetNextLoadOptionDevicePath (DevicePath, CurFullPath);

    if (PreFullPath != NULL) {
      FreePool (PreFullPath);
    }

    if (CurFullPath == NULL) {
      //
      // No Active EFI system partition is found in BootOption device path
      //
      Status = EFI_NOT_FOUND;
      break;
    }

    DEBUG_CODE_BEGIN ();
    CHAR16  *DevicePathStr;

    DevicePathStr = ConvertDevicePathToText (CurFullPath, TRUE, TRUE);
    if (DevicePathStr != NULL) {
      DEBUG ((DEBUG_INFO, "Full device path %s\n", DevicePathStr));
      FreePool (DevicePathStr);
    }

    DEBUG_CODE_END ();

    Status = GetEfiSysPartitionFromDevPath (CurFullPath, &FsFullPath, Fs);
  } while (EFI_ERROR (Status));

  if (*Fs != NULL) {
    *FullPath = FsFullPath;
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**
  Get a valid SimpleFileSystem within EFI system partition.

  @param[in]  Map             The FS mapping capsule write to
  @param[out] BootNext        The value of BootNext Variable
  @param[out] Fs              The file system within EfiSysPartition
  @param[out] UpdateBootNext  The flag to indicate whether update BootNext Variable

  @retval EFI_SUCCESS    Get FS successfully
  @retval EFI_NOT_FOUND  No valid FS found
  @retval others         Get FS failed

**/
EFI_STATUS
GetUpdateFileSystem (
  IN  CHAR16                           *Map,
  OUT UINT16                           *BootNext,
  OUT EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  **Fs,
  OUT BOOLEAN                          *UpdateBootNext
  )
{
  EFI_STATUS                      Status;
  CHAR16                          BootOptionName[20];
  UINTN                           Index;
  CONST EFI_DEVICE_PATH_PROTOCOL  *MappedDevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *FullPath;
  UINT16                          *BootNextData;
  EFI_BOOT_MANAGER_LOAD_OPTION    BootNextOption;
  EFI_BOOT_MANAGER_LOAD_OPTION    *BootOptionBuffer;
  UINTN                           BootOptionCount;
  EFI_SHELL_PROTOCOL              *ShellProtocol;
  EFI_BOOT_MANAGER_LOAD_OPTION    NewOption;

  MappedDevicePath = NULL;
  BootOptionBuffer = NULL;

  ShellProtocol = GetShellProtocol ();
  if (ShellProtocol == NULL) {
    Print (L"Get Shell Protocol Fail\n");
    return EFI_NOT_FOUND;
  }

  //
  // 1. If Fs is not assigned and there are capsule provisioned before,
  // Get EFI system partition from BootNext.
  //
  if (IsCapsuleProvisioned () && (Map == NULL)) {
    Status = GetVariable2 (
               L"BootNext",
               &gEfiGlobalVariableGuid,
               (VOID **)&BootNextData,
               NULL
               );
    if (EFI_ERROR (Status) || (BootNextData == NULL)) {
      Print (L"Get Boot Next Data Fail. Status = %r\n", Status);
      return EFI_NOT_FOUND;
    } else {
      UnicodeSPrint (BootOptionName, sizeof (BootOptionName), L"Boot%04x", *BootNextData);
      Status = EfiBootManagerVariableToLoadOption (BootOptionName, &BootNextOption);
      if (!EFI_ERROR (Status)) {
        DevicePath = BootNextOption.FilePath;
        Status     = GetEfiSysPartitionFromBootOptionFilePath (DevicePath, &FullPath, Fs);
        if (!EFI_ERROR (Status)) {
          *UpdateBootNext = FALSE;
          Print (L"Get EFI system partition from BootNext : %s\n", BootNextOption.Description);
          Print (L"%s %s\n", ShellProtocol->GetMapFromDevicePath (&FullPath), ConvertDevicePathToText (FullPath, TRUE, TRUE));
          return EFI_SUCCESS;
        }
      }
    }
  }

  //
  // Check if Map is valid.
  //
  if (Map != NULL) {
    MappedDevicePath = ShellProtocol->GetDevicePathFromMap (Map);
    if (MappedDevicePath == NULL) {
      Print (L"'%s' is not a valid mapping.\n", Map);
      return EFI_INVALID_PARAMETER;
    } else if (!IsEfiSysPartitionDevicePath (DuplicateDevicePath (MappedDevicePath))) {
      Print (L"'%s' is not a EFI System Partition.\n", Map);
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // 2. Get EFI system partition form boot options.
  //
  BootOptionBuffer = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
  if ((BootOptionBuffer == NULL) ||
      ((BootOptionCount == 0) && (Map == NULL))
      )
  {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < BootOptionCount; Index++) {
    //
    // Get the boot option from the link list
    //
    DevicePath = BootOptionBuffer[Index].FilePath;

    //
    // Skip inactive or legacy boot options
    //
    if (((BootOptionBuffer[Index].Attributes & LOAD_OPTION_ACTIVE) == 0) ||
        (DevicePathType (DevicePath) == BBS_DEVICE_PATH))
    {
      continue;
    }

    DEBUG_CODE_BEGIN ();
    CHAR16  *DevicePathStr;

    DevicePathStr = ConvertDevicePathToText (DevicePath, TRUE, TRUE);
    if (DevicePathStr != NULL) {
      DEBUG ((DEBUG_INFO, "Try BootOption %s\n", DevicePathStr));
      FreePool (DevicePathStr);
    } else {
      DEBUG ((DEBUG_INFO, "DevicePathToStr failed\n"));
    }

    DEBUG_CODE_END ();

    Status = GetEfiSysPartitionFromBootOptionFilePath (DevicePath, &FullPath, Fs);
    if (!EFI_ERROR (Status)) {
      if (Map == NULL) {
        *BootNext       = (UINT16)BootOptionBuffer[Index].OptionNumber;
        *UpdateBootNext = TRUE;
        Print (L"Found EFI system partition on Boot%04x: %s\n", *BootNext, BootOptionBuffer[Index].Description);
        Print (L"%s %s\n", ShellProtocol->GetMapFromDevicePath (&FullPath), ConvertDevicePathToText (FullPath, TRUE, TRUE));
        return EFI_SUCCESS;
      }

      if (StrnCmp (Map, ShellProtocol->GetMapFromDevicePath (&FullPath), StrLen (Map)) == 0) {
        *BootNext       = (UINT16)BootOptionBuffer[Index].OptionNumber;
        *UpdateBootNext = TRUE;
        Print (L"Found Boot Option on %s : %s\n", Map, BootOptionBuffer[Index].Description);
        return EFI_SUCCESS;
      }
    }
  }

  //
  // 3. If no ESP is found on boot option, try to find a ESP and create boot option for it.
  //
  if (Map != NULL) {
    //
    // If map is assigned, try to get ESP from mapped Fs.
    //
    DevicePath = DuplicateDevicePath (MappedDevicePath);
    Status     = GetEfiSysPartitionFromDevPath (DevicePath, &FullPath, Fs);
    if (EFI_ERROR (Status)) {
      Print (L"Error: Cannot get EFI system partition from '%s' - %r\n", Map, Status);
      return EFI_NOT_FOUND;
    }

    Print (L"Warning: Cannot find Boot Option on '%s'!\n", Map);
  } else {
    Status = GetEfiSysPartition (&DevicePath, Fs);
    if (EFI_ERROR (Status)) {
      Print (L"Error: Cannot find a EFI system partition!\n");
      return EFI_NOT_FOUND;
    }
  }

  Print (L"Create Boot option for capsule on disk:\n");
  Status = EfiBootManagerInitializeLoadOption (
             &NewOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             LOAD_OPTION_ACTIVE,
             L"UEFI Capsule On Disk",
             DevicePath,
             (UINT8 *)&mCapsuleOnDiskBootOptionGuid,
             sizeof (EFI_GUID)
             );
  if (!EFI_ERROR (Status)) {
    Status = EfiBootManagerAddLoadOptionVariable (&NewOption, (UINTN)-1);
    {
      if (!EFI_ERROR (Status)) {
        *UpdateBootNext = TRUE;
        *BootNext       = (UINT16)NewOption.OptionNumber;
        Print (L"  Boot%04x: %s\n", *BootNext, ConvertDevicePathToText (DevicePath, TRUE, TRUE));
        return EFI_SUCCESS;
      }
    }
  }

  Print (L"ERROR: Cannot create boot option! - %r\n", Status);

  return EFI_NOT_FOUND;
}

/**
  Write files to a given SimpleFileSystem.

  @param[in] Buffer          The buffer array
  @param[in] BufferSize      The buffer size array
  @param[in] FileName        The file name array
  @param[in] BufferNum       The buffer number
  @param[in] Fs              The SimpleFileSystem handle to be written

  @retval EFI_SUCCESS    Write file successfully
  @retval EFI_NOT_FOUND  SFS protocol not found
  @retval others         Write file failed

**/
EFI_STATUS
WriteUpdateFile (
  IN  VOID                             **Buffer,
  IN  UINTN                            *BufferSize,
  IN  CHAR16                           **FileName,
  IN  UINTN                            BufferNum,
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs
  )
{
  EFI_STATUS         Status;
  EFI_FILE           *Root;
  EFI_FILE           *FileHandle;
  EFI_FILE_PROTOCOL  *DirHandle;
  UINT64             FileInfo;
  VOID               *Filebuffer;
  UINTN              FileSize;
  UINTN              Index;

  DirHandle  = NULL;
  FileHandle = NULL;
  Index      = 0;

  //
  // Open Root from SFS
  //
  Status = Fs->OpenVolume (Fs, &Root);
  if (EFI_ERROR (Status)) {
    Print (L"Cannot open volume. Status = %r\n", Status);
    return EFI_NOT_FOUND;
  }

  //
  // Ensure that efi and updatecapsule directories exist
  //
  Status = Root->Open (Root, &DirHandle, L"\\EFI", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
  if (EFI_ERROR (Status)) {
    Status = Root->Open (Root, &DirHandle, L"\\EFI", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, EFI_FILE_DIRECTORY);
    if (EFI_ERROR (Status)) {
      Print (L"Unable to create %s directory\n", L"\\EFI");
      return EFI_NOT_FOUND;
    }
  }

  Status = Root->Open (Root, &DirHandle, EFI_CAPSULE_FILE_DIRECTORY, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
  if (EFI_ERROR (Status)) {
    Status = Root->Open (Root, &DirHandle, EFI_CAPSULE_FILE_DIRECTORY, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, EFI_FILE_DIRECTORY);
    if (EFI_ERROR (Status)) {
      Print (L"Unable to create %s directory\n", EFI_CAPSULE_FILE_DIRECTORY);
      return EFI_NOT_FOUND;
    }
  }

  for (Index = 0; Index < BufferNum; Index++) {
    FileHandle = NULL;

    //
    // Open UpdateCapsule file
    //
    Status = DirHandle->Open (DirHandle, &FileHandle, FileName[Index], EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR (Status)) {
      Print (L"Unable to create %s file\n", FileName[Index]);
      return EFI_NOT_FOUND;
    }

    //
    // Empty the file contents
    //
    Status = FileHandleGetSize (FileHandle, &FileInfo);
    if (EFI_ERROR (Status)) {
      FileHandleClose (FileHandle);
      Print (L"Error Reading %s\n", FileName[Index]);
      return EFI_DEVICE_ERROR;
    }

    //
    // If the file size is already 0, then it has been empty.
    //
    if (FileInfo != 0) {
      //
      // Set the file size to 0.
      //
      FileInfo = 0;
      Status   = FileHandleSetSize (FileHandle, FileInfo);
      if (EFI_ERROR (Status)) {
        Print (L"Error Deleting %s\n", FileName[Index]);
        FileHandleClose (FileHandle);
        return Status;
      }
    }

    //
    // Write Filebuffer to file
    //
    Filebuffer = Buffer[Index];
    FileSize   = BufferSize[Index];
    Status     = FileHandleWrite (FileHandle, &FileSize, Filebuffer);
    if (EFI_ERROR (Status)) {
      Print (L"Unable to write Capsule Update to %s, Status = %r\n", FileName[Index], Status);
      return EFI_NOT_FOUND;
    }

    Print (L"Succeed to write %s\n", FileName[Index]);
    FileHandleClose (FileHandle);
  }

  return EFI_SUCCESS;
}

/**
  Set capsule status variable.

  @param[in] SetCap     Set or clear the capsule flag.

  @retval EFI_SUCCESS   Succeed to set SetCap variable.
  @retval others        Fail to set the variable.

**/
EFI_STATUS
SetCapsuleStatusVariable (
  BOOLEAN  SetCap
  )
{
  EFI_STATUS  Status;
  UINT64      OsIndication;
  UINTN       DataSize;

  OsIndication = 0;
  DataSize     = sizeof (UINT64);
  Status       = gRT->GetVariable (
                        L"OsIndications",
                        &gEfiGlobalVariableGuid,
                        NULL,
                        &DataSize,
                        &OsIndication
                        );
  if (EFI_ERROR (Status)) {
    OsIndication = 0;
  }

  if (SetCap) {
    OsIndication |= ((UINT64)EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED);
  } else {
    OsIndication &= ~((UINT64)EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED);
  }

  Status = gRT->SetVariable (
                  L"OsIndications",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  sizeof (UINT64),
                  &OsIndication
                  );

  return Status;
}

/**
  Check if Capsule On Disk is supported.

  @retval TRUE              Capsule On Disk is supported.
  @retval FALSE             Capsule On Disk is not supported.

**/
BOOLEAN
IsCapsuleOnDiskSupported (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT64      OsIndicationsSupported;
  UINTN       DataSize;

  DataSize = sizeof (UINT64);
  Status   = gRT->GetVariable (
                    L"OsIndicationsSupported",
                    &gEfiGlobalVariableGuid,
                    NULL,
                    &DataSize,
                    &OsIndicationsSupported
                    );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((OsIndicationsSupported & EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED) != 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  Process Capsule On Disk.

  @param[in]  CapsuleBuffer       An array of pointer to capsule images
  @param[in]  CapsuleBufferSize   An array of UINTN to capsule images size
  @param[in]  FilePath            An array of capsule images file path
  @param[in]  Map                 File system mapping string
  @param[in]  CapsuleNum          The count of capsule images

  @retval EFI_SUCCESS       Capsule on disk success.
  @retval others            Capsule on disk fail.

**/
EFI_STATUS
ProcessCapsuleOnDisk (
  IN VOID    **CapsuleBuffer,
  IN UINTN   *CapsuleBufferSize,
  IN CHAR16  **FilePath,
  IN CHAR16  *Map,
  IN UINTN   CapsuleNum
  )
{
  EFI_STATUS                       Status;
  UINT16                           BootNext;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;
  BOOLEAN                          UpdateBootNext;
  CHAR16                           *FileName[MAX_CAPSULE_NUM];
  UINTN                            Index;

  //
  // Check if Capsule On Disk is supported
  //
  if (!IsCapsuleOnDiskSupported ()) {
    Print (L"CapsuleApp: Capsule On Disk is not supported.\n");
    return EFI_UNSUPPORTED;
  }

  //
  // Get a valid file system from boot path
  //
  Fs = NULL;

  Status = GetUpdateFileSystem (Map, &BootNext, &Fs, &UpdateBootNext);
  if (EFI_ERROR (Status)) {
    Print (L"CapsuleApp: cannot find a valid file system on boot devices. Status = %r\n", Status);
    return Status;
  }

  //
  // Get file name from file path
  //
  for (Index = 0; Index < CapsuleNum; Index++) {
    FileName[Index] = GetFileNameFromPath (FilePath[Index]);
  }

  //
  // Copy capsule image to '\efi\UpdateCapsule\'
  //
  Status = WriteUpdateFile (CapsuleBuffer, CapsuleBufferSize, FileName, CapsuleNum, Fs);
  if (EFI_ERROR (Status)) {
    Print (L"CapsuleApp: capsule image could not be copied for update.\n");
    return Status;
  }

  //
  // Set variable then reset
  //
  Status = SetCapsuleStatusVariable (TRUE);
  if (EFI_ERROR (Status)) {
    Print (L"CapsuleApp: unable to set OSIndication variable.\n");
    return Status;
  }

  if (UpdateBootNext) {
    Status = gRT->SetVariable (
                    L"BootNext",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof (UINT16),
                    &BootNext
                    );
    if (EFI_ERROR (Status)) {
      Print (L"CapsuleApp: unable to set BootNext variable.\n");
      return Status;
    }
  }

  return EFI_SUCCESS;
}
