/*++

Copyright (c) 2004 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixBlockIo.c

Abstract:

  Produce block IO abstractions for real devices on your PC using Posix APIs.
  The configuration of what devices to mount or emulate comes from UNIX 
  environment variables. The variables must be visible to the Microsoft* 
  Developer Studio for them to work.

  <F>ixed       - Fixed disk like a hard drive.
  <R>emovable   - Removable media like a floppy or CD-ROM.
  Read <O>nly   - Write protected device.
  Read <W>rite  - Read write device.
  <block count> - Decimal number of blocks a device supports.
  <block size>  - Decimal number of bytes per block.

  UNIX envirnonment variable contents. '<' and '>' are not part of the variable, 
  they are just used to make this help more readable. There should be no 
  spaces between the ';'. Extra spaces will break the variable. A '!' is 
  used to seperate multiple devices in a variable.

  EFI_UNIX_VIRTUAL_DISKS = 
    <F | R><O | W>;<block count>;<block size>[!...]

  EFI_UNIX_PHYSICAL_DISKS =
    <drive letter>:<F | R><O | W>;<block count>;<block size>[!...]

  Virtual Disks: These devices use a file to emulate a hard disk or removable
                 media device. 
                 
    Thus a 20 MB emulated hard drive would look like:
    EFI_UNIX_VIRTUAL_DISKS=FW;40960;512

    A 1.44MB emulated floppy with a block size of 1024 would look like:
    EFI_UNIX_VIRTUAL_DISKS=RW;1440;1024

  Physical Disks: These devices use UNIX to open a real device in your system

    Thus a 120 MB floppy would look like:
    EFI_UNIX_PHYSICAL_DISKS=B:RW;245760;512

    Thus a standard CD-ROM floppy would look like:
    EFI_UNIX_PHYSICAL_DISKS=Z:RO;307200;2048


  * Other names and brands may be claimed as the property of others.

--*/

#include <fcntl.h>
#include <unistd.h>
#include "UnixBlockIo.h"

//
// Block IO protocol member functions
//
STATIC
EFI_STATUS
EFIAPI
UnixBlockIoReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  OUT VOID                  *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  MediaId     - TODO: add argument description
  Lba         - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

STATIC
EFI_STATUS
EFIAPI
UnixBlockIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  MediaId     - TODO: add argument description
  Lba         - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

STATIC
EFI_STATUS
EFIAPI
UnixBlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

STATIC
EFI_STATUS
EFIAPI
UnixBlockIoResetBlock (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                  - TODO: add argument description
  ExtendedVerification  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
// Private Worker functions
//
STATIC
EFI_STATUS
UnixBlockIoCreateMapping (
  IN EFI_UNIX_IO_PROTOCOL             *UnixIo,
  IN EFI_HANDLE                         EfiDeviceHandle,
  IN CHAR16                             *Filename,
  IN BOOLEAN                            ReadOnly,
  IN BOOLEAN                            RemovableMedia,
  IN UINTN                              NumberOfBlocks,
  IN UINTN                              BlockSize
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  UnixIo         - TODO: add argument description
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
;

STATIC
EFI_STATUS
UnixBlockIoReadWriteCommon (
  IN  UNIX_BLOCK_IO_PRIVATE *Private,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer,
  IN CHAR8                    *CallerName
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

  TODO: add return values

--*/
;

STATIC
EFI_STATUS
UnixBlockIoError (
  IN UNIX_BLOCK_IO_PRIVATE      *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

STATIC
EFI_STATUS
UnixBlockIoOpenDevice (
  UNIX_BLOCK_IO_PRIVATE         *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

STATIC
CHAR16                                    *
GetNextElementPastTerminator (
  IN  CHAR16  *EnvironmentVariable,
  IN  CHAR16  Terminator
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  EnvironmentVariable - TODO: add argument description
  Terminator          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;
EFI_DRIVER_BINDING_PROTOCOL gUnixBlockIoDriverBinding = {
  UnixBlockIoDriverBindingSupported,
  UnixBlockIoDriverBindingStart,
  UnixBlockIoDriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
UnixBlockIoDriverBindingSupported (
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
  EFI_UNIX_IO_PROTOCOL  *UnixIo;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiUnixIoProtocolGuid,
                  (VOID **)&UnixIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure the UnixThunkProtocol is valid
  //
  Status = EFI_UNSUPPORTED;
  if (UnixIo->UnixThunk->Signature == EFI_UNIX_THUNK_PROTOCOL_SIGNATURE) {

    //
    // Check the GUID to see if this is a handle type the driver supports
    //
    if (CompareGuid (UnixIo->TypeGuid, &gEfiUnixVirtualDisksGuid) ) {
      Status = EFI_SUCCESS;
    }
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
        Handle,
        &gEfiUnixIoProtocolGuid,
        This->DriverBindingHandle,
        Handle
        );
  return Status;
}

EFI_STATUS
EFIAPI
UnixBlockIoDriverBindingStart (
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
  EFI_UNIX_IO_PROTOCOL       *UnixIo;
  CHAR16                      Buffer[FILENAME_BUFFER_SIZE];
  CHAR16                      *Str;
  BOOLEAN                     RemovableMedia;
  BOOLEAN                     WriteProtected;
  UINTN                       NumberOfBlocks;
  UINTN                       BlockSize;
  INTN	                      i;

  //
  // Grab the protocols we need
  //
  
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiUnixIoProtocolGuid,
                  (void *)&UnixIo,
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
  if (!CompareGuid (UnixIo->TypeGuid, &gEfiUnixVirtualDisksGuid)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status  = EFI_NOT_FOUND;
  //  Extract filename.
  Str     = UnixIo->EnvString;
  i = 0;
  while (*Str && *Str != ':')
    Buffer[i++] = *Str++;
  Buffer[i] = 0;
  if (*Str != ':') {
    goto Done;
  }

  Str++;

  RemovableMedia = FALSE;
  WriteProtected = TRUE;
  NumberOfBlocks = 0;
  BlockSize = 512;
  do {
    if (*Str == 'R' || *Str == 'F') {
      RemovableMedia = (BOOLEAN) (*Str == 'R');
      Str++;
    }
    if (*Str == 'O' || *Str == 'W') {
      WriteProtected  = (BOOLEAN) (*Str == 'O');
      Str++;
    }
    if (*Str == 0)
      break;
    if (*Str != ';')
      goto Done;
    Str++;

    NumberOfBlocks  = Atoi (Str);
    Str       = GetNextElementPastTerminator (Str, ';');
    if (NumberOfBlocks == 0)
      break;

    BlockSize = Atoi (Str);
    if (BlockSize != 0)
      Str       = GetNextElementPastTerminator (Str, ';');
  } while (0);

  //
  // If we get here the variable is valid so do the work.
  //
  Status = UnixBlockIoCreateMapping (
              UnixIo,
              Handle,
              Buffer,
              WriteProtected,
              RemovableMedia,
              NumberOfBlocks,
              BlockSize
              );

Done:
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Handle,
          &gEfiUnixIoProtocolGuid,
          This->DriverBindingHandle,
          Handle
          );
  }

  return Status;
}

EFI_STATUS
EFIAPI
UnixBlockIoDriverBindingStop (
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
  UNIX_BLOCK_IO_PRIVATE *Private;

  //
  // Get our context back
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiBlockIoProtocolGuid,
                  (void *)&BlockIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Private = UNIX_BLOCK_IO_PRIVATE_DATA_FROM_THIS (BlockIo);

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
                    &gEfiUnixIoProtocolGuid,
                    This->DriverBindingHandle,
                    Handle
                    );

    //
    // Shut down our device
    //
    Private->UnixThunk->Close (Private->fd);

    //
    // Free our instance data
    //
    FreeUnicodeStringTable (Private->ControllerNameTable);

    gBS->FreePool (Private);
  }

  return Status;
}

STATIC
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

STATIC
EFI_STATUS
UnixBlockIoCreateMapping (
  IN EFI_UNIX_IO_PROTOCOL             *UnixIo,
  IN EFI_HANDLE                         EfiDeviceHandle,
  IN CHAR16                             *Filename,
  IN BOOLEAN                            ReadOnly,
  IN BOOLEAN                            RemovableMedia,
  IN UINTN                              NumberOfBlocks,
  IN UINTN                              BlockSize
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  UnixIo         - TODO: add argument description
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
  UNIX_BLOCK_IO_PRIVATE *Private;
  UINTN                   Index;

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (UNIX_BLOCK_IO_PRIVATE),
                  (void *)&Private
                  );
  ASSERT_EFI_ERROR (Status);

  EfiInitializeLock (&Private->Lock, TPL_NOTIFY);

  Private->UnixThunk = UnixIo->UnixThunk;

  Private->Signature  = UNIX_BLOCK_IO_PRIVATE_SIGNATURE;
  Private->LastBlock  = NumberOfBlocks - 1;
  Private->BlockSize  = BlockSize;

  for (Index = 0; Filename[Index] != 0; Index++) {
    Private->Filename[Index] = Filename[Index];
  }

  Private->Filename[Index]      = 0;

  Private->Mode                 = (ReadOnly ? O_RDONLY : O_RDWR);

  Private->NumberOfBlocks       = NumberOfBlocks;
  Private->fd                   = -1;

  Private->ControllerNameTable  = NULL;

  AddUnicodeString (
    "eng",
    gUnixBlockIoComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    Filename
    );

  BlockIo = &Private->BlockIo;
  BlockIo->Revision = EFI_BLOCK_IO_PROTOCOL_REVISION;
  BlockIo->Media = &Private->Media;
  BlockIo->Media->BlockSize = Private->BlockSize;
  BlockIo->Media->LastBlock = Private->NumberOfBlocks - 1;
  BlockIo->Media->MediaId = 0;;

  BlockIo->Reset = UnixBlockIoResetBlock;
  BlockIo->ReadBlocks = UnixBlockIoReadBlocks;
  BlockIo->WriteBlocks = UnixBlockIoWriteBlocks;
  BlockIo->FlushBlocks = UnixBlockIoFlushBlocks;

  BlockIo->Media->ReadOnly = ReadOnly;
  BlockIo->Media->RemovableMedia = RemovableMedia;
  BlockIo->Media->LogicalPartition = FALSE;
  BlockIo->Media->MediaPresent = TRUE;
  BlockIo->Media->WriteCaching = FALSE;

  BlockIo->Media->IoAlign = 1;

  Private->EfiHandle  = EfiDeviceHandle;
  Status              = UnixBlockIoOpenDevice (Private);
  if (!EFI_ERROR (Status)) {

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Private->EfiHandle,
                    &gEfiBlockIoProtocolGuid,
                    &Private->BlockIo,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      FreeUnicodeStringTable (Private->ControllerNameTable);
      gBS->FreePool (Private);
    }

    DEBUG ((EFI_D_ERROR, "BlockDevice added: %s\n", Filename));
  }

  return Status;
}

STATIC
EFI_STATUS
UnixBlockIoOpenDevice (
  UNIX_BLOCK_IO_PRIVATE                 *Private
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
  if (Private->fd >= 0) {
    BlockIo->Reset (BlockIo, FALSE);
  }

  //
  // Open the device
  //
  Private->fd = Private->UnixThunk->Open
    (Private->Filename, Private->Mode, 0644);

  if (Private->fd < 0) {
    DEBUG ((EFI_D_INFO, "PlOpenBlock: Could not open %s\n",
	    Private->Filename));
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
  Status = SetFilePointer64 (Private, 0, &FileSize, SEEK_END);

  if (EFI_ERROR (Status)) {
    FileSize = MultU64x32 (Private->NumberOfBlocks, Private->BlockSize);
    DEBUG ((EFI_D_ERROR, "PlOpenBlock: Could not get filesize of %s\n", Private->Filename));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  if (Private->NumberOfBlocks == 0) {
    Private->NumberOfBlocks = DivU64x32 (FileSize, Private->BlockSize);
  }

  EndOfFile = MultU64x32 (Private->NumberOfBlocks, Private->BlockSize);

  if (FileSize != EndOfFile) {
    //
    // file is not the proper size, change it
    //
    DEBUG ((EFI_D_INIT, "PlOpenBlock: Initializing block device: %a\n", Private->Filename));

    //
    // first set it to 0
    //
    Private->UnixThunk->FTruncate (Private->fd, 0);

    //
    // then set it to the needed file size (OS will zero fill it)
    //
    Private->UnixThunk->FTruncate (Private->fd, EndOfFile);
  }

  DEBUG ((EFI_D_INIT, "%HPlOpenBlock: opened %s%N\n", Private->Filename));
  Status = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    if (Private->fd >= 0) {
      BlockIo->Reset (BlockIo, FALSE);
    }
  }

  EfiReleaseLock (&Private->Lock);
  return Status;
}

STATIC
EFI_STATUS
UnixBlockIoError (
  IN UNIX_BLOCK_IO_PRIVATE      *Private
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
  return EFI_DEVICE_ERROR;

#if 0
  EFI_BLOCK_IO_PROTOCOL *BlockIo;
  EFI_STATUS            Status;
  BOOLEAN               ReinstallBlockIoFlag;


  BlockIo = &Private->BlockIo;

  switch (Private->UnixThunk->GetLastError ()) {

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
#endif
}

STATIC
EFI_STATUS
UnixBlockIoReadWriteCommon (
  IN  UNIX_BLOCK_IO_PRIVATE     *Private,
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

  if (Private->fd < 0) {
    Status = UnixBlockIoOpenDevice (Private);
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

  if ((UINT32) Buffer % Private->Media.IoAlign != 0) {
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
  DistanceToMove = MultU64x32 (Lba, BlockSize);
  Status = SetFilePointer64 (Private, DistanceToMove, &DistanceMoved, SEEK_SET);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INIT, "WriteBlocks: SetFilePointer failed\n"));
    return UnixBlockIoError (Private);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UnixBlockIoReadBlocks (
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
  UNIX_BLOCK_IO_PRIVATE *Private;
  ssize_t                 len;
  EFI_STATUS              Status;
  EFI_TPL                 OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Private = UNIX_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  Status  = UnixBlockIoReadWriteCommon (Private, MediaId, Lba, BufferSize, Buffer, "UnixReadBlocks");
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  len = Private->UnixThunk->Read (Private->fd, Buffer, BufferSize);
  if (len != BufferSize) {
    DEBUG ((EFI_D_INIT, "ReadBlocks: ReadFile failed.\n"));
    Status = UnixBlockIoError (Private);
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

STATIC
EFI_STATUS
EFIAPI
UnixBlockIoWriteBlocks (
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
  UNIX_BLOCK_IO_PRIVATE *Private;
  ssize_t                 len;
  EFI_STATUS              Status;
  EFI_TPL                 OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Private = UNIX_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  Status  = UnixBlockIoReadWriteCommon (Private, MediaId, Lba, BufferSize, Buffer, "UnixWriteBlocks");
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  len = Private->UnixThunk->Write (Private->fd, Buffer, BufferSize);
  if (len != BufferSize) {
    DEBUG ((EFI_D_INIT, "ReadBlocks: WriteFile failed.\n"));
    Status = UnixBlockIoError (Private);
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

STATIC
EFI_STATUS
EFIAPI
UnixBlockIoFlushBlocks (
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

STATIC
EFI_STATUS
EFIAPI
UnixBlockIoResetBlock (
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
  UNIX_BLOCK_IO_PRIVATE *Private;
  EFI_TPL               OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  
  Private = UNIX_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Private->fd >= 0) {
    Private->UnixThunk->Close (Private->fd);
    Private->fd = -1;
  }

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

UINTN
Atoi (
  CHAR16  *String
  )
/*++

Routine Description:

  Convert a unicode string to a UINTN

Arguments:

  String - Unicode string.

Returns: 

  UINTN of the number represented by String.  

--*/
{
  UINTN   Number;
  CHAR16  *Str;

  //
  // skip preceeding white space
  //
  Str = String;
  while ((*Str) && (*Str == ' ')) {
    Str++;
  }
  //
  // Convert ot a Number
  //
  Number = 0;
  while (*Str != '\0') {
    if ((*Str >= '0') && (*Str <= '9')) {
      Number = (Number * 10) +*Str - '0';
    } else {
      break;
    }

    Str++;
  }

  return Number;
}

EFI_STATUS
SetFilePointer64 (
  IN  UNIX_BLOCK_IO_PRIVATE    *Private,
  IN  INT64                      DistanceToMove,
  OUT UINT64                     *NewFilePointer,
  IN  INTN                      MoveMethod
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
  off_t         res;

  res = Private->UnixThunk->Lseek(Private->fd, DistanceToMove, MoveMethod);
  if (res == -1) {
    Status = EFI_INVALID_PARAMETER;
  }

  if (NewFilePointer != NULL) {
    *NewFilePointer = res;
  }

  return Status;
}
