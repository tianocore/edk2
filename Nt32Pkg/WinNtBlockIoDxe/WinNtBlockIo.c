/**@file

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  WinNtBlockIo.c

Abstract:

  Produce block IO abstractions for real devices on your PC using Win32 APIs.
  The configuration of what devices to mount or emulate comes from NT
  environment variables. The variables must be visible to the Microsoft*
  Developer Studio for them to work.

  <F>ixed       - Fixed disk like a hard drive.
  <R>emovable   - Removable media like a floppy or CD-ROM.
  Read <O>nly   - Write protected device.
  Read <W>rite  - Read write device.
  <block count> - Decimal number of blocks a device supports.
  <block size>  - Decimal number of bytes per block.

  NT envirnonment variable contents. '<' and '>' are not part of the variable,
  they are just used to make this help more readable. There should be no
  spaces between the ';'. Extra spaces will break the variable. A '!' is
  used to seperate multiple devices in a variable.

  EFI_WIN_NT_VIRTUAL_DISKS =
    <F | R><O | W>;<block count>;<block size>[!...]

  EFI_WIN_NT_PHYSICAL_DISKS =
    <drive letter>:<F | R><O | W>;<block count>;<block size>[!...]

  Virtual Disks: These devices use a file to emulate a hard disk or removable
                 media device.

    Thus a 20 MB emulated hard drive would look like:
    EFI_WIN_NT_VIRTUAL_DISKS=FW;40960;512

    A 1.44MB emulated floppy with a block size of 1024 would look like:
    EFI_WIN_NT_VIRTUAL_DISKS=RW;1440;1024

  Physical Disks: These devices use NT to open a real device in your system

    Thus a 120 MB floppy would look like:
    EFI_WIN_NT_PHYSICAL_DISKS=B:RW;245760;512

    Thus a standard CD-ROM floppy would look like:
    EFI_WIN_NT_PHYSICAL_DISKS=Z:RO;307200;2048


  * Other names and brands may be claimed as the property of others.

**/
#include <Uefi.h>
#include <WinNtDxe.h>
#include <Protocol/WinNtThunk.h>
#include <Protocol/WinNtIo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include "WinNtBlockIo.h"

EFI_DRIVER_BINDING_PROTOCOL gWinNtBlockIoDriverBinding = {
  WinNtBlockIoDriverBindingSupported,
  WinNtBlockIoDriverBindingStart,
  WinNtBlockIoDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  The user Entry Point for module WinNtBlockIo. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeWinNtBlockIo(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallAllDriverProtocols2 (
             ImageHandle,
             SystemTable,
             &gWinNtBlockIoDriverBinding,
             ImageHandle,
             &gWinNtBlockIoComponentName,
             &gWinNtBlockIoComponentName2,
             NULL,
             NULL,
             &gWinNtBlockIoDriverDiagnostics,
             &gWinNtBlockIoDriverDiagnostics2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}

EFI_STATUS
EFIAPI
WinNtBlockIoDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Handle - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
{
  EFI_STATUS              Status;
  EFI_WIN_NT_IO_PROTOCOL  *WinNtIo;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiWinNtIoProtocolGuid,
                  (VOID **) &WinNtIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure the WinNtThunkProtocol is valid
  //
  Status = EFI_UNSUPPORTED;
  if (WinNtIo->WinNtThunk->Signature == EFI_WIN_NT_THUNK_PROTOCOL_SIGNATURE) {

    //
    // Check the GUID to see if this is a handle type the driver supports
    //
    if (CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtVirtualDisksGuid) ||
        CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtPhysicalDisksGuid) ) {
      Status = EFI_SUCCESS;
    }
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
        Handle,
        &gEfiWinNtIoProtocolGuid,
        This->DriverBindingHandle,
        Handle
        );

  return Status;
}

EFI_STATUS
EFIAPI
WinNtBlockIoDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Handle - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
{
  EFI_STATUS                  Status;
  EFI_WIN_NT_IO_PROTOCOL      *WinNtIo;
  WIN_NT_RAW_DISK_DEVICE_TYPE DiskType;
  UINT16                      Buffer[FILENAME_BUFFER_SIZE];
  CHAR16                      *Str;
  BOOLEAN                     RemovableMedia;
  BOOLEAN                     WriteProtected;
  UINTN                       NumberOfBlocks;
  UINTN                       BlockSize;

  //
  // Grab the protocols we need
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiWinNtIoProtocolGuid,
                  (VOID **) &WinNtIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set DiskType
  //
  if (CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtVirtualDisksGuid)) {
    DiskType = EfiWinNtVirtualDisks;
  } else if (CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtPhysicalDisksGuid)) {
    DiskType = EfiWinNtPhysicalDisks;
  } else {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status  = EFI_NOT_FOUND;
  Str     = WinNtIo->EnvString;
  if (DiskType == EfiWinNtVirtualDisks) {
    WinNtIo->WinNtThunk->SPrintf (
                          Buffer,
                          sizeof (Buffer),
                          L"Diskfile%d",
                          WinNtIo->InstanceNumber
                          );
  } else {
    if (*Str >= 'A' && *Str <= 'Z' || *Str >= 'a' && *Str <= 'z') {
      WinNtIo->WinNtThunk->SPrintf (Buffer, sizeof (Buffer), L"\\\\.\\%c:", *Str);
    } else {
      WinNtIo->WinNtThunk->SPrintf (Buffer, sizeof (Buffer), L"\\\\.\\PHYSICALDRIVE%c", *Str);
    }

    Str++;
    if (*Str != ':') {
      Status = EFI_NOT_FOUND;
      goto Done;
    }

    Str++;
  }

  if (*Str == 'R' || *Str == 'F') {
    RemovableMedia = (BOOLEAN) (*Str == 'R');
    Str++;
    if (*Str == 'O' || *Str == 'W') {
      WriteProtected  = (BOOLEAN) (*Str == 'O');
      Str             = GetNextElementPastTerminator (Str, ';');

      NumberOfBlocks  = StrDecimalToUintn (Str);
      if (NumberOfBlocks != 0) {
        Str       = GetNextElementPastTerminator (Str, ';');
        BlockSize = StrDecimalToUintn (Str);
        if (BlockSize != 0) {
          //
          // If we get here the variable is valid so do the work.
          //
          Status = WinNtBlockIoCreateMapping (
                    WinNtIo,
                    Handle,
                    Buffer,
                    WriteProtected,
                    RemovableMedia,
                    NumberOfBlocks,
                    BlockSize,
                    DiskType
                    );

        }
      }
    }
  }

Done:
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Handle,
          &gEfiWinNtIoProtocolGuid,
          This->DriverBindingHandle,
          Handle
          );
  }

  return Status;
}

EFI_STATUS
EFIAPI
WinNtBlockIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  Handle            - TODO: add argument description
  NumberOfChildren  - TODO: add argument description
  ChildHandleBuffer - TODO: add argument description

Returns:

  EFI_UNSUPPORTED - TODO: Add description for return value

--*/
{
  EFI_BLOCK_IO_PROTOCOL   *BlockIo;
  EFI_STATUS              Status;
  WIN_NT_BLOCK_IO_PRIVATE *Private;

  //
  // Get our context back
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &BlockIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Private = WIN_NT_BLOCK_IO_PRIVATE_DATA_FROM_THIS (BlockIo);

  //
  // BugBug: If we need to kick people off, we need to make Uninstall Close the handles.
  //         We could pass in our image handle or FLAG our open to be closed via
  //         Unistall (== to saying any CloseProtocol will close our open)
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Private->EfiHandle,
                  &gEfiBlockIoProtocolGuid,
                  &Private->BlockIo,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {

    Status = gBS->CloseProtocol (
                    Handle,
                    &gEfiWinNtIoProtocolGuid,
                    This->DriverBindingHandle,
                    Handle
                    );

    //
    // Shut down our device
    //
    Private->WinNtThunk->CloseHandle (Private->NtHandle);

    //
    // Free our instance data
    //
    FreeUnicodeStringTable (Private->ControllerNameTable);

    FreePool (Private);
  }

  return Status;
}

CHAR16 *
GetNextElementPastTerminator (
  IN  CHAR16  *EnvironmentVariable,
  IN  CHAR16  Terminator
  )
/*++

Routine Description:

  Worker function to parse environment variables.

Arguments:
  EnvironmentVariable - Envirnment variable to parse.

  Terminator          - Terminator to parse for.

Returns:

  Pointer to next eliment past the first occurence of Terminator or the '\0'
  at the end of the string.

--*/
{
  CHAR16  *Ptr;

  for (Ptr = EnvironmentVariable; *Ptr != '\0'; Ptr++) {
    if (*Ptr == Terminator) {
      Ptr++;
      break;
    }
  }

  return Ptr;
}

EFI_STATUS
WinNtBlockIoCreateMapping (
  IN EFI_WIN_NT_IO_PROTOCOL             *WinNtIo,
  IN EFI_HANDLE                         EfiDeviceHandle,
  IN CHAR16                             *Filename,
  IN BOOLEAN                            ReadOnly,
  IN BOOLEAN                            RemovableMedia,
  IN UINTN                              NumberOfBlocks,
  IN UINTN                              BlockSize,
  IN WIN_NT_RAW_DISK_DEVICE_TYPE        DeviceType
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  WinNtIo         - TODO: add argument description
  EfiDeviceHandle - TODO: add argument description
  Filename        - TODO: add argument description
  ReadOnly        - TODO: add argument description
  RemovableMedia  - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description
  BlockSize       - TODO: add argument description
  DeviceType      - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  EFI_STATUS              Status;
  EFI_BLOCK_IO_PROTOCOL   *BlockIo;
  WIN_NT_BLOCK_IO_PRIVATE *Private;
  UINTN                   Index;

  WinNtIo->WinNtThunk->SetErrorMode (SEM_FAILCRITICALERRORS);

  Private = AllocatePool (sizeof (WIN_NT_BLOCK_IO_PRIVATE));
  ASSERT (Private != NULL);

  EfiInitializeLock (&Private->Lock, TPL_NOTIFY);

  Private->WinNtThunk = WinNtIo->WinNtThunk;

  Private->Signature  = WIN_NT_BLOCK_IO_PRIVATE_SIGNATURE;
  Private->LastBlock  = NumberOfBlocks - 1;
  Private->BlockSize  = BlockSize;

  for (Index = 0; Filename[Index] != 0; Index++) {
    Private->Filename[Index] = Filename[Index];
  }

  Private->Filename[Index]      = 0;

  Private->ReadMode             = GENERIC_READ | (ReadOnly ? 0 : GENERIC_WRITE);
  Private->ShareMode            = FILE_SHARE_READ | FILE_SHARE_WRITE;

  Private->NumberOfBlocks       = NumberOfBlocks;
  Private->DeviceType           = DeviceType;
  Private->NtHandle             = INVALID_HANDLE_VALUE;

  Private->ControllerNameTable  = NULL;

  AddUnicodeString2 (
    "eng",
    gWinNtBlockIoComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    Private->Filename,
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gWinNtBlockIoComponentName2.SupportedLanguages,
    &Private->ControllerNameTable,
    Private->Filename,
    FALSE
    );


  BlockIo = &Private->BlockIo;
  BlockIo->Revision = EFI_BLOCK_IO_PROTOCOL_REVISION;
  BlockIo->Media = &Private->Media;
  BlockIo->Media->BlockSize = (UINT32)Private->BlockSize;
  BlockIo->Media->LastBlock = Private->NumberOfBlocks - 1;
  BlockIo->Media->MediaId = 0;;

  BlockIo->Reset = WinNtBlockIoResetBlock;
  BlockIo->ReadBlocks = WinNtBlockIoReadBlocks;
  BlockIo->WriteBlocks = WinNtBlockIoWriteBlocks;
  BlockIo->FlushBlocks = WinNtBlockIoFlushBlocks;

  BlockIo->Media->ReadOnly = ReadOnly;
  BlockIo->Media->RemovableMedia = RemovableMedia;
  BlockIo->Media->LogicalPartition = FALSE;
  BlockIo->Media->MediaPresent = TRUE;
  BlockIo->Media->WriteCaching = FALSE;

  if (DeviceType == EfiWinNtVirtualDisks) {
    BlockIo->Media->IoAlign = 1;

    //
    // Create a file to use for a virtual disk even if it does not exist.
    //
    Private->OpenMode = OPEN_ALWAYS;
  } else if (DeviceType == EfiWinNtPhysicalDisks) {
    //
    // Physical disk and floppy devices require 4 byte alignment.
    //
    BlockIo->Media->IoAlign = 4;

    //
    // You can only open a physical device if it exists.
    //
    Private->OpenMode = OPEN_EXISTING;
  } else {
    ASSERT (FALSE);
  }

  Private->EfiHandle  = EfiDeviceHandle;
  Status              = WinNtBlockIoOpenDevice (Private);
  if (!EFI_ERROR (Status)) {

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Private->EfiHandle,
                    &gEfiBlockIoProtocolGuid,
                    &Private->BlockIo,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      FreeUnicodeStringTable (Private->ControllerNameTable);
      FreePool (Private);
    }

    DEBUG ((EFI_D_INIT, "BlockDevice added: %s\n", Filename));
  }

  return Status;
}

EFI_STATUS
WinNtBlockIoOpenDevice (
  WIN_NT_BLOCK_IO_PRIVATE                 *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  EFI_STATUS            Status;
  UINT64                FileSize;
  UINT64                EndOfFile;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;

  BlockIo = &Private->BlockIo;
  EfiAcquireLock (&Private->Lock);

  //
  // If the device is already opened, close it
  //
  if (Private->NtHandle != INVALID_HANDLE_VALUE) {
    BlockIo->Reset (BlockIo, FALSE);
  }

  //
  // Open the device
  //
  Private->NtHandle = Private->WinNtThunk->CreateFile (
                                            Private->Filename,
                                            (DWORD)Private->ReadMode,
                                            (DWORD)Private->ShareMode,
                                            NULL,
                                            (DWORD)Private->OpenMode,
                                            0,
                                            NULL
                                            );

  Status = Private->WinNtThunk->GetLastError ();

  if (Private->NtHandle == INVALID_HANDLE_VALUE) {
    DEBUG ((EFI_D_INFO, "PlOpenBlock: Could not open %s, %x\n", Private->Filename, Private->WinNtThunk->GetLastError ()));
    BlockIo->Media->MediaPresent  = FALSE;
    Status                        = EFI_NO_MEDIA;
    goto Done;
  }

  if (!BlockIo->Media->MediaPresent) {
    //
    // BugBug: try to emulate if a CD appears - notify drivers to check it out
    //
    BlockIo->Media->MediaPresent = TRUE;
    EfiReleaseLock (&Private->Lock);
    EfiAcquireLock (&Private->Lock);
  }

  //
  // get the size of the file
  //
  Status = SetFilePointer64 (Private, 0, &FileSize, FILE_END);

  if (EFI_ERROR (Status)) {
    FileSize = MultU64x32 (Private->NumberOfBlocks, (UINT32)Private->BlockSize);
    if (Private->DeviceType == EfiWinNtVirtualDisks) {
      DEBUG ((EFI_D_ERROR, "PlOpenBlock: Could not get filesize of %s\n", Private->Filename));
      Status = EFI_UNSUPPORTED;
      goto Done;
    }
  }

  if (Private->NumberOfBlocks == 0) {
    Private->NumberOfBlocks = DivU64x32 (FileSize, (UINT32)Private->BlockSize);
  }

  EndOfFile = MultU64x32 (Private->NumberOfBlocks, (UINT32)Private->BlockSize);

  if (FileSize != EndOfFile) {
    //
    // file is not the proper size, change it
    //
    DEBUG ((EFI_D_INIT, "PlOpenBlock: Initializing block device: %hs\n", Private->Filename));

    //
    // first set it to 0
    //
    SetFilePointer64 (Private, 0, NULL, FILE_BEGIN);
    Private->WinNtThunk->SetEndOfFile (Private->NtHandle);

    //
    // then set it to the needed file size (OS will zero fill it)
    //
    SetFilePointer64 (Private, EndOfFile, NULL, FILE_BEGIN);
    Private->WinNtThunk->SetEndOfFile (Private->NtHandle);
  }

  DEBUG ((EFI_D_INIT, "%HPlOpenBlock: opened %s%N\n", Private->Filename));
  Status = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    if (Private->NtHandle != INVALID_HANDLE_VALUE) {
      BlockIo->Reset (BlockIo, FALSE);
    }
  }

  EfiReleaseLock (&Private->Lock);
  return Status;
}

EFI_STATUS
WinNtBlockIoError (
  IN WIN_NT_BLOCK_IO_PRIVATE      *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  EFI_BLOCK_IO_PROTOCOL *BlockIo;
  EFI_STATUS            Status;
  BOOLEAN               ReinstallBlockIoFlag;

  BlockIo = &Private->BlockIo;

  switch (Private->WinNtThunk->GetLastError ()) {

  case ERROR_NOT_READY:
    Status                        = EFI_NO_MEDIA;
    BlockIo->Media->ReadOnly      = FALSE;
    BlockIo->Media->MediaPresent  = FALSE;
    ReinstallBlockIoFlag          = FALSE;
    break;

  case ERROR_WRONG_DISK:
    BlockIo->Media->ReadOnly      = FALSE;
    BlockIo->Media->MediaPresent  = TRUE;
    BlockIo->Media->MediaId += 1;
    ReinstallBlockIoFlag  = TRUE;
    Status                = EFI_MEDIA_CHANGED;
    break;

  case ERROR_WRITE_PROTECT:
    BlockIo->Media->ReadOnly  = TRUE;
    ReinstallBlockIoFlag      = FALSE;
    Status                    = EFI_WRITE_PROTECTED;
    break;

  default:
    ReinstallBlockIoFlag  = FALSE;
    Status                = EFI_DEVICE_ERROR;
    break;
  }

  if (ReinstallBlockIoFlag) {
    BlockIo->Reset (BlockIo, FALSE);

    gBS->ReinstallProtocolInterface (
          Private->EfiHandle,
          &gEfiBlockIoProtocolGuid,
          BlockIo,
          BlockIo
          );
  }

  return Status;
}

EFI_STATUS
WinNtBlockIoReadWriteCommon (
  IN  WIN_NT_BLOCK_IO_PRIVATE     *Private,
  IN UINT32                       MediaId,
  IN EFI_LBA                      Lba,
  IN UINTN                        BufferSize,
  IN VOID                         *Buffer,
  IN CHAR8                        *CallerName
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private     - TODO: add argument description
  MediaId     - TODO: add argument description
  Lba         - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description
  CallerName  - TODO: add argument description

Returns:

  EFI_NO_MEDIA - TODO: Add description for return value
  EFI_MEDIA_CHANGED - TODO: Add description for return value
  EFI_INVALID_PARAMETER - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value
  EFI_BAD_BUFFER_SIZE - TODO: Add description for return value
  EFI_INVALID_PARAMETER - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  EFI_STATUS  Status;
  UINTN       BlockSize;
  UINT64      LastBlock;
  INT64       DistanceToMove;
  UINT64      DistanceMoved;

  if (Private->NtHandle == INVALID_HANDLE_VALUE) {
    Status = WinNtBlockIoOpenDevice (Private);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (!Private->Media.MediaPresent) {
    DEBUG ((EFI_D_INIT, "%s: No Media\n", CallerName));
    return EFI_NO_MEDIA;
  }

  if (Private->Media.MediaId != MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if ((UINTN) Buffer % Private->Media.IoAlign != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Verify buffer size
  //
  BlockSize = Private->BlockSize;
  if (BufferSize == 0) {
    DEBUG ((EFI_D_INIT, "%s: Zero length read\n", CallerName));
    return EFI_SUCCESS;
  }

  if ((BufferSize % BlockSize) != 0) {
    DEBUG ((EFI_D_INIT, "%s: Invalid read size\n", CallerName));
    return EFI_BAD_BUFFER_SIZE;
  }

  LastBlock = Lba + (BufferSize / BlockSize) - 1;
  if (LastBlock > Private->LastBlock) {
    DEBUG ((EFI_D_INIT, "ReadBlocks: Attempted to read off end of device\n"));
    return EFI_INVALID_PARAMETER;
  }
  //
  // Seek to End of File
  //
  DistanceToMove = MultU64x32 (Lba, (UINT32)BlockSize);
  Status = SetFilePointer64 (Private, DistanceToMove, &DistanceMoved, FILE_BEGIN);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INIT, "WriteBlocks: SetFilePointer failed\n"));
    return WinNtBlockIoError (Private);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtBlockIoReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  OUT VOID                  *Buffer
  )
/*++

  Routine Description:
    Read BufferSize bytes from Lba into Buffer.

  Arguments:
    This       - Protocol instance pointer.
    MediaId    - Id of the media, changes every time the media is replaced.
    Lba        - The starting Logical Block Address to read from
    BufferSize - Size of Buffer, must be a multiple of device block size.
    Buffer     - Buffer containing read data

  Returns:
    EFI_SUCCESS           - The data was read correctly from the device.
    EFI_DEVICE_ERROR      - The device reported an error while performing the read.
    EFI_NO_MEDIA          - There is no media in the device.
    EFI_MEDIA_CHANGED     - The MediaId does not matched the current device.
    EFI_BAD_BUFFER_SIZE   - The Buffer was not a multiple of the block size of the
                            device.
    EFI_INVALID_PARAMETER - The read request contains device addresses that are not
                            valid for the device.

--*/
{
  WIN_NT_BLOCK_IO_PRIVATE *Private;
  BOOL                    Flag;
  EFI_STATUS              Status;
  DWORD                   BytesRead;
  EFI_TPL                 OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Private = WIN_NT_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  Status  = WinNtBlockIoReadWriteCommon (Private, MediaId, Lba, BufferSize, Buffer, "WinNtReadBlocks");
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Flag = Private->WinNtThunk->ReadFile (Private->NtHandle, Buffer, (DWORD) BufferSize, (LPDWORD) &BytesRead, NULL);
  if (!Flag || (BytesRead != BufferSize)) {
    DEBUG ((EFI_D_INIT, "ReadBlocks: ReadFile failed. (%d)\n", Private->WinNtThunk->GetLastError ()));
    Status = WinNtBlockIoError (Private);
    goto Done;
  }

  //
  // If we wrote then media is present.
  //
  This->Media->MediaPresent = TRUE;
  Status = EFI_SUCCESS;

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

EFI_STATUS
EFIAPI
WinNtBlockIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  )
/*++

  Routine Description:
    Write BufferSize bytes from Lba into Buffer.

  Arguments:
    This       - Protocol instance pointer.
    MediaId    - Id of the media, changes every time the media is replaced.
    Lba        - The starting Logical Block Address to read from
    BufferSize - Size of Buffer, must be a multiple of device block size.
    Buffer     - Buffer containing read data

  Returns:
    EFI_SUCCESS           - The data was written correctly to the device.
    EFI_WRITE_PROTECTED   - The device can not be written to.
    EFI_DEVICE_ERROR      - The device reported an error while performing the write.
    EFI_NO_MEDIA          - There is no media in the device.
    EFI_MEDIA_CHNAGED     - The MediaId does not matched the current device.
    EFI_BAD_BUFFER_SIZE   - The Buffer was not a multiple of the block size of the
                            device.
    EFI_INVALID_PARAMETER - The write request contains a LBA that is not
                            valid for the device.

--*/
{
  WIN_NT_BLOCK_IO_PRIVATE *Private;
  UINTN                   BytesWritten;
  BOOL                    Flag;
  BOOL                    Locked;
  EFI_STATUS              Status;
  EFI_TPL                 OldTpl;
  UINTN                   BytesReturned;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Private = WIN_NT_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  Status  = WinNtBlockIoReadWriteCommon (Private, MediaId, Lba, BufferSize, Buffer, "WinNtWriteBlocks");
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // According the Windows requirement, first need to lock the volume before 
  // write to it.
  //
  if (Private->DeviceType == EfiWinNtPhysicalDisks) {
    Locked = Private->WinNtThunk->DeviceIoControl (Private->NtHandle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &BytesReturned, NULL);
    if (Locked == 0) {
      DEBUG ((EFI_D_INIT, "ReadBlocks: Lock volume failed. (%d)\n", Private->WinNtThunk->GetLastError ()));
      Status = WinNtBlockIoError (Private);
      goto Done;
    }
  } else {
    Locked = 0;
  }
  Flag = Private->WinNtThunk->WriteFile (Private->NtHandle, Buffer, (DWORD) BufferSize, (LPDWORD) &BytesWritten, NULL);
  if (Locked != 0) {
    Private->WinNtThunk->DeviceIoControl (Private->NtHandle, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &BytesReturned, NULL);
  }
  if (!Flag || (BytesWritten != BufferSize)) {
    DEBUG ((EFI_D_INIT, "ReadBlocks: WriteFile failed. (%d)\n", Private->WinNtThunk->GetLastError ()));
    Status = WinNtBlockIoError (Private);
    goto Done;
  }

  //
  // If the write succeeded, we are not write protected and media is present.
  //
  This->Media->MediaPresent = TRUE;
  This->Media->ReadOnly     = FALSE;
  Status = EFI_SUCCESS;

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;

}

EFI_STATUS
EFIAPI
WinNtBlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
/*++

  Routine Description:
    Flush the Block Device.

  Arguments:
    This             - Protocol instance pointer.

  Returns:
    EFI_SUCCESS      - All outstanding data was written to the device
    EFI_DEVICE_ERROR - The device reported an error while writting back the data
    EFI_NO_MEDIA     - There is no media in the device.

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtBlockIoResetBlock (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  )
/*++

  Routine Description:
    Reset the Block Device.

  Arguments:
    This                 - Protocol instance pointer.
    ExtendedVerification - Driver may perform diagnostics on reset.

  Returns:
    EFI_SUCCESS           - The device was reset.
    EFI_DEVICE_ERROR      - The device is not functioning properly and could
                            not be reset.

--*/
{
  WIN_NT_BLOCK_IO_PRIVATE *Private;
  EFI_TPL                 OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Private = WIN_NT_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Private->NtHandle != INVALID_HANDLE_VALUE) {
    Private->WinNtThunk->CloseHandle (Private->NtHandle);
    Private->NtHandle = INVALID_HANDLE_VALUE;
  }

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}


EFI_STATUS
SetFilePointer64 (
  IN  WIN_NT_BLOCK_IO_PRIVATE    *Private,
  IN  INT64                      DistanceToMove,
  OUT UINT64                     *NewFilePointer,
  IN  DWORD                      MoveMethod
  )
/*++

This function extends the capability of SetFilePointer to accept 64 bit parameters

--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO: function comment is missing 'Returns:'
// TODO:    Private - add argument and description to function comment
// TODO:    DistanceToMove - add argument and description to function comment
// TODO:    NewFilePointer - add argument and description to function comment
// TODO:    MoveMethod - add argument and description to function comment
{
  EFI_STATUS    Status;
  LARGE_INTEGER LargeInt;

  LargeInt.QuadPart = DistanceToMove;
  Status            = EFI_SUCCESS;

  LargeInt.LowPart = Private->WinNtThunk->SetFilePointer (
                                            Private->NtHandle,
                                            LargeInt.LowPart,
                                            &LargeInt.HighPart,
                                            MoveMethod
                                            );

  if (LargeInt.LowPart == -1 && Private->WinNtThunk->GetLastError () != NO_ERROR) {
    Status = EFI_INVALID_PARAMETER;
  }

  if (NewFilePointer != NULL) {
    *NewFilePointer = LargeInt.QuadPart;
  }

  return Status;
}
