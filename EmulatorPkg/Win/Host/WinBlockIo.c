/**@file

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "WinHost.h"

#define WIN_NT_BLOCK_IO_PRIVATE_SIGNATURE SIGNATURE_32 ('N', 'T', 'b', 'k')
typedef struct {
  UINTN                       Signature;

  EMU_IO_THUNK_PROTOCOL       *Thunk;

  CHAR16                      *FileName;
  BOOLEAN                     Removable;
  BOOLEAN                     Readonly;

  HANDLE                      NtHandle;
  UINT32                      BlockSize;

  EFI_BLOCK_IO_MEDIA          *Media;
  EMU_BLOCK_IO_PROTOCOL       EmuBlockIo;
} WIN_NT_BLOCK_IO_PRIVATE;

#define WIN_NT_BLOCK_IO_PRIVATE_DATA_FROM_THIS(a) \
         CR(a, WIN_NT_BLOCK_IO_PRIVATE, EmuBlockIo, WIN_NT_BLOCK_IO_PRIVATE_SIGNATURE)


EFI_STATUS
WinNtBlockIoReset (
  IN EMU_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ExtendedVerification
  );




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
{
  EFI_STATUS    Status;
  LARGE_INTEGER LargeInt;

  LargeInt.QuadPart = DistanceToMove;
  Status = EFI_SUCCESS;

  LargeInt.LowPart = SetFilePointer (
    Private->NtHandle,
    LargeInt.LowPart,
    &LargeInt.HighPart,
    MoveMethod
  );

  if (LargeInt.LowPart == -1 && GetLastError () != NO_ERROR) {
    Status = EFI_INVALID_PARAMETER;
  }

  if (NewFilePointer != NULL) {
    *NewFilePointer = LargeInt.QuadPart;
  }

  return Status;
}



EFI_STATUS
WinNtBlockIoOpenDevice (
  IN WIN_NT_BLOCK_IO_PRIVATE   *Private,
  IN EFI_BLOCK_IO_MEDIA        *Media
  )
{
  EFI_STATUS            Status;
  UINT64                FileSize;

  //
  // If the device is already opened, close it
  //
  if (Private->NtHandle != INVALID_HANDLE_VALUE) {
    WinNtBlockIoReset (&Private->EmuBlockIo, FALSE);
  }

  //
  // Open the device
  //
  Private->NtHandle = CreateFile (
    Private->FileName,
    GENERIC_READ | (Private->Readonly ? 0 : GENERIC_WRITE),
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    OPEN_ALWAYS, // Create if it doesn't exist
    0,
    NULL
  );

  if (Private->NtHandle == INVALID_HANDLE_VALUE) {
    DEBUG ((EFI_D_INFO, "OpenBlock: Could not open %S, %x\n", Private->FileName, GetLastError ()));
    Media->MediaPresent = FALSE;
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  //
  // get the size of the file
  //
  Status = SetFilePointer64 (Private, 0, &FileSize, FILE_END);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OpenBlock: Could not get filesize of %s\n", Private->FileName));
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Media->LastBlock = DivU64x32 (FileSize, (UINT32)Private->BlockSize) - 1;

  DEBUG ((EFI_D_INIT, "OpenBlock: opened %S\n", Private->FileName));
  Status = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    if (Private->NtHandle != INVALID_HANDLE_VALUE) {
      WinNtBlockIoReset (&Private->EmuBlockIo, FALSE);
    }
  }

  return Status;
}


EFI_STATUS
EFIAPI
WinNtBlockIoCreateMapping (
  IN     EMU_BLOCK_IO_PROTOCOL    *This,
  IN     EFI_BLOCK_IO_MEDIA       *Media
  )
{
  WIN_NT_BLOCK_IO_PRIVATE    *Private;

  Private = WIN_NT_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  Media->MediaId          = 0;
  Media->RemovableMedia   = Private->Removable;
  Media->MediaPresent     = TRUE;
  Media->LogicalPartition = FALSE;
  Media->ReadOnly         = Private->Readonly;
  Media->WriteCaching     = FALSE;
  Media->IoAlign          = 1;
  Media->LastBlock        = 0; // Filled in by OpenDevice
  Media->BlockSize        = Private->BlockSize;

  // EFI_BLOCK_IO_PROTOCOL_REVISION2
  Media->LowestAlignedLba              = 0;
  Media->LogicalBlocksPerPhysicalBlock = 0;


  // EFI_BLOCK_IO_PROTOCOL_REVISION3
  Media->OptimalTransferLengthGranularity = 0;

  //
  // Remember the Media pointer.
  //
  Private->Media = Media;
  return WinNtBlockIoOpenDevice (Private, Media);
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
  EFI_BLOCK_IO_MEDIA    *Media;
  EFI_STATUS            Status;

  Media = Private->Media;

  switch (GetLastError ()) {

  case ERROR_NOT_READY:
    Media->ReadOnly = FALSE;
    Media->MediaPresent = FALSE;
    Status = EFI_NO_MEDIA;
    break;

  case ERROR_WRONG_DISK:
    Media->ReadOnly = FALSE;
    Media->MediaPresent = TRUE;
    Media->MediaId++;
    Status = EFI_MEDIA_CHANGED;
    break;

  case ERROR_WRITE_PROTECT:
    Media->ReadOnly = TRUE;
    Status = EFI_WRITE_PROTECTED;
    break;

  default:
    Status = EFI_DEVICE_ERROR;
    break;
  }

  if (Status == EFI_NO_MEDIA || Status == EFI_MEDIA_CHANGED) {
    WinNtBlockIoReset (&Private->EmuBlockIo, FALSE);
  }

  return Status;
}


EFI_STATUS
WinNtSignalToken (
  IN OUT EFI_BLOCK_IO2_TOKEN      *Token,
  IN     EFI_STATUS               Status
)
{
  if (Token != NULL) {
    if (Token->Event != NULL) {
      // Caller is responcible for signaling EFI Event
      Token->TransactionStatus = Status;
      return EFI_SUCCESS;
    }
  }
  return Status;
}

/**
  Read BufferSize bytes from Lba into Buffer.

  This function reads the requested number of blocks from the device. All the
  blocks are read, or an error is returned.
  If EFI_DEVICE_ERROR, EFI_NO_MEDIA,_or EFI_MEDIA_CHANGED is returned and
  non-blocking I/O is being used, the Event associated with this request will
  not be signaled.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    Id of the media, changes every time the media is
                              replaced.
  @param[in]       Lba        The starting Logical Block Address to read from.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[out]      Buffer     A pointer to the destination buffer for the data. The
                              caller is responsible for either having implicit or
                              explicit ownership of the buffer.

  @retval EFI_SUCCESS           The read request was queued if Token->Event is
                                not NULL.The data was read correctly from the
                                device if the Token->Event is NULL.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing
                                the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of the
                                intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack
                                of resources.
**/
EFI_STATUS
WinNtBlockIoReadBlocks (
  IN     EMU_BLOCK_IO_PROTOCOL  *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
     OUT VOID                   *Buffer
  )
{
  WIN_NT_BLOCK_IO_PRIVATE *Private;
  BOOL                    Flag;
  EFI_STATUS              Status;
  DWORD                   BytesRead;
  UINT64                  DistanceToMove;
  UINT64                  DistanceMoved;

  Private = WIN_NT_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  //
  // Seek to proper position
  //
  DistanceToMove = MultU64x32 (Lba, (UINT32)Private->BlockSize);
  Status = SetFilePointer64 (Private, DistanceToMove, &DistanceMoved, FILE_BEGIN);

  if (EFI_ERROR (Status) || (DistanceToMove != DistanceMoved)) {
    DEBUG ((EFI_D_INIT, "ReadBlocks: SetFilePointer failed\n"));
    return WinNtBlockIoError (Private->Media);
  }

  Flag = ReadFile (Private->NtHandle, Buffer, (DWORD)BufferSize, (LPDWORD)&BytesRead, NULL);
  if (!Flag || (BytesRead != BufferSize)) {
    return WinNtBlockIoError (Private->Media);
  }

  Private->Media->MediaPresent = TRUE;
  return WinNtSignalToken (Token, EFI_SUCCESS);
}


/**
  Write BufferSize bytes from Lba into Buffer.

  This function writes the requested number of blocks to the device. All blocks
  are written, or an error is returned.If EFI_DEVICE_ERROR, EFI_NO_MEDIA,
  EFI_WRITE_PROTECTED or EFI_MEDIA_CHANGED is returned and non-blocking I/O is
  being used, the Event associated with this request will not be signaled.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    The media ID that the write request is for.
  @param[in]       Lba        The starting logical block address to be written. The
                              caller is responsible for writing to only legitimate
                              locations.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[in]       Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The write request was queued if Event is not NULL.
                                The data was written correctly to the device if
                                the Event is NULL.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not match the current device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack
                                of resources.

**/
EFI_STATUS
WinNtBlockIoWriteBlocks (
  IN     EMU_BLOCK_IO_PROTOCOL  *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
  IN     VOID                   *Buffer
  )
{
  WIN_NT_BLOCK_IO_PRIVATE *Private;
  UINTN                   BytesWritten;
  BOOL                    Success;
  EFI_STATUS              Status;
  UINT64                  DistanceToMove;
  UINT64                  DistanceMoved;

  Private = WIN_NT_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  //
  // Seek to proper position
  //
  DistanceToMove = MultU64x32 (Lba, (UINT32)Private->BlockSize);
  Status = SetFilePointer64 (Private, DistanceToMove, &DistanceMoved, FILE_BEGIN);

  if (EFI_ERROR (Status) || (DistanceToMove != DistanceMoved)) {
    DEBUG ((EFI_D_INIT, "WriteBlocks: SetFilePointer failed\n"));
    return WinNtBlockIoError (Private->Media);
  }

  Success = WriteFile (Private->NtHandle, Buffer, (DWORD)BufferSize, (LPDWORD)&BytesWritten, NULL);
  if (!Success || (BytesWritten != BufferSize)) {
    return WinNtBlockIoError (Private->Media);
  }

  //
  // If the write succeeded, we are not write protected and media is present.
  //
  Private->Media->MediaPresent = TRUE;
  Private->Media->ReadOnly     = FALSE;
  return WinNtSignalToken (Token, EFI_SUCCESS);
}

/**
  Flush the Block Device.

  If EFI_DEVICE_ERROR, EFI_NO_MEDIA,_EFI_WRITE_PROTECTED or EFI_MEDIA_CHANGED
  is returned and non-blocking I/O is being used, the Event associated with
  this request will not be signaled.

  @param[in]      This     Indicates a pointer to the calling context.
  @param[in,out]  Token    A pointer to the token associated with the transaction

  @retval EFI_SUCCESS          The flush request was queued if Event is not NULL.
                               All outstanding data was written correctly to the
                               device if the Event is NULL.
  @retval EFI_DEVICE_ERROR     The device reported an error while writing back
                               the data.
  @retval EFI_WRITE_PROTECTED  The device cannot be written to.
  @retval EFI_NO_MEDIA         There is no media in the device.
  @retval EFI_MEDIA_CHANGED    The MediaId is not for the current media.
  @retval EFI_OUT_OF_RESOURCES The request could not be completed due to a lack
                               of resources.

**/
EFI_STATUS
WinNtBlockIoFlushBlocks (
  IN     EMU_BLOCK_IO_PROTOCOL    *This,
  IN OUT EFI_BLOCK_IO2_TOKEN      *Token
  )
{
  return WinNtSignalToken (Token, EFI_SUCCESS);
}


/**
  Reset the block device hardware.

  @param[in]  This                 Indicates a pointer to the calling context.
  @param[in]  ExtendedVerification Indicates that the driver may perform a more
                                   exhausive verfication operation of the device
                                   during reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
WinNtBlockIoReset (
  IN EMU_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ExtendedVerification
  )
{
  WIN_NT_BLOCK_IO_PRIVATE *Private;

  Private = WIN_NT_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Private->NtHandle != INVALID_HANDLE_VALUE) {
    CloseHandle (Private->NtHandle);
    Private->NtHandle = INVALID_HANDLE_VALUE;
  }

  return EFI_SUCCESS;
}

EMU_BLOCK_IO_PROTOCOL gEmuBlockIoProtocol = {
  WinNtBlockIoReset,
  WinNtBlockIoReadBlocks,
  WinNtBlockIoWriteBlocks,
  WinNtBlockIoFlushBlocks,
  WinNtBlockIoCreateMapping
};

EFI_STATUS
EFIAPI
WinNtBlockIoThunkOpen (
  IN  EMU_IO_THUNK_PROTOCOL   *This
  )
{
  WIN_NT_BLOCK_IO_PRIVATE  *Private;
  CHAR16                   *Str;

  Private = AllocatePool (sizeof (*Private));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature = WIN_NT_BLOCK_IO_PRIVATE_SIGNATURE;
  Private->Thunk     = This;
  CopyMem (&Private->EmuBlockIo, &gEmuBlockIoProtocol, sizeof (gEmuBlockIoProtocol));
  Private->BlockSize = 512;
  Private->NtHandle = INVALID_HANDLE_VALUE;

  Private->FileName = AllocateCopyPool (StrSize (This->ConfigString), This->ConfigString);
  if (Private->FileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Parse ConfigString
  // <ConfigString> := <FileName> ':' [RF][OW] ':' <BlockSize>
  //
  Str = StrStr (Private->FileName, L":");
  if (Str == NULL) {
    Private->Removable = FALSE;
    Private->Readonly  = FALSE;
  } else {
    for (*Str++ = L'\0'; *Str != L'\0'; Str++) {
      if (*Str == 'R' || *Str == 'F') {
        Private->Removable = (BOOLEAN) (*Str == L'R');
      }
      if (*Str == 'O' || *Str == 'W') {
        Private->Readonly = (BOOLEAN) (*Str == L'O');
      }
      if (*Str == ':') {
        Private->BlockSize = wcstol (++Str, NULL, 0);
        break;
      }
    }
  }

  This->Interface = &Private->EmuBlockIo;
  This->Private   = Private;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
WinNtBlockIoThunkClose (
  IN  EMU_IO_THUNK_PROTOCOL   *This
  )
{
  WIN_NT_BLOCK_IO_PRIVATE  *Private;

  Private = This->Private;

  if (Private != NULL) {
    if (Private->FileName != NULL) {
      FreePool (Private->FileName);
    }
    FreePool (Private);
  }

  return EFI_SUCCESS;
}



EMU_IO_THUNK_PROTOCOL mWinNtBlockIoThunkIo = {
  &gEmuBlockIoProtocolGuid,
  NULL,
  NULL,
  0,
  WinNtBlockIoThunkOpen,
  WinNtBlockIoThunkClose,
  NULL
};


