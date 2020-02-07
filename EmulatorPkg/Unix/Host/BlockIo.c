/**@file

Copyright (c) 2004 - 2009, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Host.h"

#define EMU_BLOCK_IO_PRIVATE_SIGNATURE SIGNATURE_32 ('E', 'M', 'b', 'k')
typedef struct {
  UINTN                       Signature;

  EMU_IO_THUNK_PROTOCOL       *Thunk;

  char                        *Filename;
  UINTN                       ReadMode;
  UINTN                       Mode;

  int                         fd;

  BOOLEAN                     RemovableMedia;
  BOOLEAN                     WriteProtected;

  UINT64                      NumberOfBlocks;
  UINT32                      BlockSize;

  EMU_BLOCK_IO_PROTOCOL       EmuBlockIo;
  EFI_BLOCK_IO_MEDIA          *Media;

} EMU_BLOCK_IO_PRIVATE;

#define EMU_BLOCK_IO_PRIVATE_DATA_FROM_THIS(a) \
         CR(a, EMU_BLOCK_IO_PRIVATE, EmuBlockIo, EMU_BLOCK_IO_PRIVATE_SIGNATURE)



EFI_STATUS
EmuBlockIoReset (
  IN EMU_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ExtendedVerification
  );


/*++

This function extends the capability of SetFilePointer to accept 64 bit parameters

**/
EFI_STATUS
SetFilePointer64 (
  IN  EMU_BLOCK_IO_PRIVATE        *Private,
  IN  INT64                      DistanceToMove,
  OUT UINT64                     *NewFilePointer,
  IN  INT32                      MoveMethod
  )
{
  EFI_STATUS    Status;
  off_t         res;
  off_t         offset = DistanceToMove;

  Status = EFI_SUCCESS;
  res = lseek (Private->fd, offset, (int)MoveMethod);
  if (res == -1) {
    Status = EFI_INVALID_PARAMETER;
  }

  if (NewFilePointer != NULL) {
    *NewFilePointer = res;
  }

  return Status;
}


EFI_STATUS
EmuBlockIoOpenDevice (
  IN EMU_BLOCK_IO_PRIVATE   *Private
  )
{
  EFI_STATUS            Status;
  UINT64                FileSize;
  struct statfs         buf;


  //
  // If the device is already opened, close it
  //
  if (Private->fd >= 0) {
    EmuBlockIoReset (&Private->EmuBlockIo, FALSE);
  }

  //
  // Open the device
  //
  Private->fd = open (Private->Filename, Private->Mode, 0644);
  if (Private->fd < 0) {
    printf ("EmuOpenBlock: Could not open %s: %s\n", Private->Filename, strerror(errno));
    Private->Media->MediaPresent  = FALSE;
    Status                        = EFI_NO_MEDIA;
    goto Done;
  }

  if (!Private->Media->MediaPresent) {
    //
    // BugBug: try to emulate if a CD appears - notify drivers to check it out
    //
    Private->Media->MediaPresent = TRUE;
  }

  //
  // get the size of the file
  //
  Status = SetFilePointer64 (Private, 0, &FileSize, SEEK_END);
  if (EFI_ERROR (Status)) {
    printf ("EmuOpenBlock: Could not get filesize of %s\n", Private->Filename);
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  if (FileSize == 0) {
    // lseek fails on a real device. ioctl calls are OS specific
#if __APPLE__
    {
      UINT32 BlockSize;

      if (ioctl (Private->fd, DKIOCGETBLOCKSIZE, &BlockSize) == 0) {
        Private->Media->BlockSize = BlockSize;
      }
      if (ioctl (Private->fd, DKIOCGETBLOCKCOUNT, &Private->NumberOfBlocks) == 0) {
        if ((Private->NumberOfBlocks == 0) && (BlockSize == 0x800)) {
          // A DVD is ~ 4.37 GB so make up a number
          Private->Media->LastBlock = (0x100000000ULL/0x800) - 1;
        } else {
          Private->Media->LastBlock = Private->NumberOfBlocks - 1;
        }
      }
      ioctl (Private->fd, DKIOCGETMAXBLOCKCOUNTWRITE, &Private->Media->OptimalTransferLengthGranularity);
    }
#else
    {
      size_t BlockSize;
      UINT64 DiskSize;

      if (ioctl (Private->fd, BLKSSZGET, &BlockSize) == 0) {
        Private->Media->BlockSize = BlockSize;
      }
      if (ioctl (Private->fd, BLKGETSIZE64, &DiskSize) == 0) {
        Private->NumberOfBlocks = DivU64x32 (DiskSize, (UINT32)BlockSize);
        Private->Media->LastBlock = Private->NumberOfBlocks - 1;
      }
    }
#endif

  } else {
    Private->Media->BlockSize = Private->BlockSize;
    Private->NumberOfBlocks = DivU64x32 (FileSize, Private->Media->BlockSize);
    Private->Media->LastBlock = Private->NumberOfBlocks - 1;

    if (fstatfs (Private->fd, &buf) == 0) {
#if __APPLE__
      Private->Media->OptimalTransferLengthGranularity = buf.f_iosize/buf.f_bsize;
#else
      Private->Media->OptimalTransferLengthGranularity = buf.f_bsize/buf.f_bsize;
#endif
    }
  }

  DEBUG ((EFI_D_INIT, "%HEmuOpenBlock: opened %a%N\n", Private->Filename));
  Status = EFI_SUCCESS;

Done:
  if (EFI_ERROR (Status)) {
    if (Private->fd >= 0) {
      EmuBlockIoReset (&Private->EmuBlockIo, FALSE);
    }
  }

  return Status;
}


EFI_STATUS
EmuBlockIoCreateMapping (
  IN     EMU_BLOCK_IO_PROTOCOL    *This,
  IN     EFI_BLOCK_IO_MEDIA       *Media
  )
{
  EFI_STATUS              Status;
  EMU_BLOCK_IO_PRIVATE    *Private;

  Private = EMU_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  Private->Media = Media;

  Media->MediaId          = 0;
  Media->RemovableMedia   = Private->RemovableMedia;
  Media->MediaPresent     = TRUE;
  Media->LogicalPartition = FALSE;
  Media->ReadOnly         = Private->WriteProtected;
  Media->WriteCaching     = FALSE;
  Media->IoAlign          = 1;
  Media->LastBlock        = 0; // Filled in by OpenDevice

  // EFI_BLOCK_IO_PROTOCOL_REVISION2
  Media->LowestAlignedLba              = 0;
  Media->LogicalBlocksPerPhysicalBlock = 0;


  // EFI_BLOCK_IO_PROTOCOL_REVISION3
  Media->OptimalTransferLengthGranularity = 0;

  Status = EmuBlockIoOpenDevice (Private);


  return Status;
}


EFI_STATUS
EmuBlockIoError (
  IN EMU_BLOCK_IO_PRIVATE      *Private
  )
{
  EFI_STATUS            Status;
  BOOLEAN               ReinstallBlockIoFlag;


  switch (errno) {

  case EAGAIN:
    Status                        = EFI_NO_MEDIA;
    Private->Media->ReadOnly      = FALSE;
    Private->Media->MediaPresent  = FALSE;
    ReinstallBlockIoFlag          = FALSE;
    break;

  case EACCES:
    Private->Media->ReadOnly      = FALSE;
    Private->Media->MediaPresent  = TRUE;
    Private->Media->MediaId += 1;
    ReinstallBlockIoFlag  = TRUE;
    Status                = EFI_MEDIA_CHANGED;
    break;

  case EROFS:
    Private->Media->ReadOnly  = TRUE;
    ReinstallBlockIoFlag      = FALSE;
    Status                    = EFI_WRITE_PROTECTED;
    break;

  default:
    ReinstallBlockIoFlag  = FALSE;
    Status                = EFI_DEVICE_ERROR;
    break;
  }
  return Status;
}


EFI_STATUS
EmuBlockIoReadWriteCommon (
  IN  EMU_BLOCK_IO_PRIVATE        *Private,
  IN UINT32                       MediaId,
  IN EFI_LBA                      Lba,
  IN UINTN                        BufferSize,
  IN VOID                         *Buffer,
  IN CHAR8                        *CallerName
  )
{
  EFI_STATUS  Status;
  UINTN       BlockSize;
  UINT64      LastBlock;
  INT64       DistanceToMove;
  UINT64      DistanceMoved;

  if (Private->fd < 0) {
    Status = EmuBlockIoOpenDevice (Private);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (!Private->Media->MediaPresent) {
    DEBUG ((EFI_D_INIT, "%s: No Media\n", CallerName));
    return EFI_NO_MEDIA;
  }

  if (Private->Media->MediaId != MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if ((UINTN) Buffer % Private->Media->IoAlign != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Verify buffer size
  //
  BlockSize = Private->Media->BlockSize;
  if (BufferSize == 0) {
    DEBUG ((EFI_D_INIT, "%s: Zero length read\n", CallerName));
    return EFI_SUCCESS;
  }

  if ((BufferSize % BlockSize) != 0) {
    DEBUG ((EFI_D_INIT, "%s: Invalid read size\n", CallerName));
    return EFI_BAD_BUFFER_SIZE;
  }

  LastBlock = Lba + (BufferSize / BlockSize) - 1;
  if (LastBlock > Private->Media->LastBlock) {
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
    return EmuBlockIoError (Private);
  }

  return EFI_SUCCESS;
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
EmuBlockIoReadBlocks (
  IN     EMU_BLOCK_IO_PROTOCOL  *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                LBA,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
     OUT VOID                   *Buffer
  )
{
  EFI_STATUS              Status;
  EMU_BLOCK_IO_PRIVATE    *Private;
  ssize_t                 len;

  Private = EMU_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  Status  = EmuBlockIoReadWriteCommon (Private, MediaId, LBA, BufferSize, Buffer, "UnixReadBlocks");
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  len = read (Private->fd, Buffer, BufferSize);
  if (len != BufferSize) {
    DEBUG ((EFI_D_INIT, "ReadBlocks: ReadFile failed.\n"));
    Status = EmuBlockIoError (Private);
    goto Done;
  }

  //
  // If we read then media is present.
  //
  Private->Media->MediaPresent = TRUE;
  Status = EFI_SUCCESS;

Done:
  if (Token != NULL) {
    if (Token->Event != NULL) {
      // Caller is responsible for signaling EFI Event
      Token->TransactionStatus = Status;
      return EFI_SUCCESS;
    }
  }
  return Status;
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
EmuBlockIoWriteBlocks (
  IN     EMU_BLOCK_IO_PROTOCOL  *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                LBA,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
  IN     VOID                   *Buffer
  )
{
  EMU_BLOCK_IO_PRIVATE    *Private;
  ssize_t                 len;
  EFI_STATUS              Status;


  Private = EMU_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  Status  = EmuBlockIoReadWriteCommon (Private, MediaId, LBA, BufferSize, Buffer, "UnixWriteBlocks");
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  len = write (Private->fd, Buffer, BufferSize);
  if (len != BufferSize) {
    DEBUG ((EFI_D_INIT, "ReadBlocks: WriteFile failed.\n"));
    Status = EmuBlockIoError (Private);
    goto Done;
  }

  //
  // If the write succeeded, we are not write protected and media is present.
  //
  Private->Media->MediaPresent = TRUE;
  Private->Media->ReadOnly     = FALSE;
  Status = EFI_SUCCESS;

Done:
  if (Token != NULL) {
    if (Token->Event != NULL) {
      // Caller is responsible for signaling EFI Event
      Token->TransactionStatus = Status;
      return EFI_SUCCESS;
    }
  }

  return Status;
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
EmuBlockIoFlushBlocks (
  IN     EMU_BLOCK_IO_PROTOCOL    *This,
  IN OUT EFI_BLOCK_IO2_TOKEN      *Token
  )
{
  EMU_BLOCK_IO_PRIVATE *Private;

  Private = EMU_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Private->fd >= 0) {
    fsync (Private->fd);
#if __APPLE__
    fcntl (Private->fd, F_FULLFSYNC);
#endif
  }


  if (Token != NULL) {
    if (Token->Event != NULL) {
      // Caller is responsible for signaling EFI Event
      Token->TransactionStatus = EFI_SUCCESS;
      return EFI_SUCCESS;
    }
  }

  return EFI_SUCCESS;
}


/**
  Reset the block device hardware.

  @param[in]  This                 Indicates a pointer to the calling context.
  @param[in]  ExtendedVerification Indicates that the driver may perform a more
                                   exhaustive verification operation of the device
                                   during reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EmuBlockIoReset (
  IN EMU_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ExtendedVerification
  )
{
  EMU_BLOCK_IO_PRIVATE *Private;

  Private = EMU_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  if (Private->fd >= 0) {
    close (Private->fd);
    Private->fd = -1;
  }

  return EFI_SUCCESS;
}


char *
StdDupUnicodeToAscii (
  IN  CHAR16 *Str
  )
{
  UINTN   Size;
  char    *Ascii;
  char    *Ptr;

  Size = StrLen (Str) + 1;
  Ascii = malloc (Size);
  if (Ascii == NULL) {
    return NULL;
  }

  for (Ptr = Ascii; *Str != '\0'; Ptr++, Str++) {
    *Ptr = *Str;
  }
  *Ptr = 0;

  return Ascii;
}


EMU_BLOCK_IO_PROTOCOL gEmuBlockIoProtocol = {
  GasketEmuBlockIoReset,
  GasketEmuBlockIoReadBlocks,
  GasketEmuBlockIoWriteBlocks,
  GasketEmuBlockIoFlushBlocks,
  GasketEmuBlockIoCreateMapping
};

EFI_STATUS
EmuBlockIoThunkOpen (
  IN  EMU_IO_THUNK_PROTOCOL   *This
  )
{
  EMU_BLOCK_IO_PRIVATE  *Private;
  char                  *Str;

  if (This->Private != NULL) {
    return EFI_ALREADY_STARTED;
  }

  if (!CompareGuid (This->Protocol, &gEmuBlockIoProtocolGuid)) {
    return EFI_UNSUPPORTED;
  }

  Private = malloc (sizeof (EMU_BLOCK_IO_PRIVATE));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }


  Private->Signature = EMU_BLOCK_IO_PRIVATE_SIGNATURE;
  Private->Thunk     = This;
  CopyMem (&Private->EmuBlockIo, &gEmuBlockIoProtocol, sizeof (gEmuBlockIoProtocol));
  Private->fd        = -1;
  Private->BlockSize = 512;

  Private->Filename = StdDupUnicodeToAscii (This->ConfigString);
  if (Private->Filename == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Str = strstr (Private->Filename, ":");
  if (Str == NULL) {
    Private->RemovableMedia = FALSE;
    Private->WriteProtected = FALSE;
  } else {
    for (*Str++ = '\0'; *Str != 0; Str++) {
      if (*Str == 'R' || *Str == 'F') {
        Private->RemovableMedia = (BOOLEAN) (*Str == 'R');
      }
      if (*Str == 'O' || *Str == 'W') {
        Private->WriteProtected  = (BOOLEAN) (*Str == 'O');
      }
      if (*Str == ':') {
        Private->BlockSize = strtol (++Str, NULL, 0);
        break;
      }
    }
  }

  Private->Mode = Private->WriteProtected ? O_RDONLY : O_RDWR;

  This->Interface = &Private->EmuBlockIo;
  This->Private   = Private;
  return EFI_SUCCESS;
}


EFI_STATUS
EmuBlockIoThunkClose (
  IN  EMU_IO_THUNK_PROTOCOL   *This
  )
{
  EMU_BLOCK_IO_PRIVATE  *Private;

  if (!CompareGuid (This->Protocol, &gEmuBlockIoProtocolGuid)) {
    return EFI_UNSUPPORTED;
  }

  Private = This->Private;

  if (This->Private != NULL) {
    if (Private->Filename != NULL) {
      free (Private->Filename);
    }
    free (This->Private);
    This->Private = NULL;
  }

  return EFI_SUCCESS;
}



EMU_IO_THUNK_PROTOCOL gBlockIoThunkIo = {
  &gEmuBlockIoProtocolGuid,
  NULL,
  NULL,
  0,
  GasketBlockIoThunkOpen,
  GasketBlockIoThunkClose,
  NULL
};


