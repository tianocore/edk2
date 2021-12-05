/** @file
  FUSE_READ / FUSE_READDIRPLUS wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

/**
  Read a chunk from a regular file or a directory stream, by sending the
  FUSE_READ / FUSE_READDIRPLUS request to the Virtio Filesystem device.

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs  The Virtio Filesystem device to send the FUSE_READ
                           or FUSE_READDIRPLUS request to. On output, the FUSE
                           request counter "VirtioFs->RequestId" will have been
                           incremented.

  @param[in] NodeId        The inode number of the regular file or directory
                           stream to read from.

  @param[in] FuseHandle    The open handle to the regular file or directory
                           stream to read from.

  @param[in] IsDir         TRUE if NodeId and FuseHandle refer to a directory,
                           FALSE if NodeId and FuseHandle refer to a regular
                           file.

  @param[in] Offset        If IsDir is FALSE: the absolute file position at
                           which to start reading.

                           If IsDir is TRUE: the directory stream cookie at
                           which to start or continue reading. The zero-valued
                           cookie identifies the start of the directory stream.
                           Further positions in the directory stream can be
                           passed in from the CookieForNextEntry field of
                           VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE.

  @param[in,out] Size      On input, the number of bytes to read. On successful
                           return, the number of bytes actually read, which may
                           be smaller than the value on input.

                           When reading a regular file (i.e., when IsDir is
                           FALSE), EOF can be detected by passing in a nonzero
                           Size, and finding a zero Size on output.

                           When reading a directory stream (i.e., when IsDir is
                           TRUE), Data consists of a sequence of variably-sized
                           records (directory entries). A read operation
                           returns the maximal sequence of records that fits in
                           Size, without having to truncate a record. In order
                           to guarantee progress, call

                             VirtioFsFuseStatFs (VirtioFs, NodeId,
                               &FilesysAttr)

                           first, to learn the maximum Namelen for the
                           directory stream. Then assign Size at least

                             VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE_SIZE (
                               FilesysAttr.Namelen)

                           on input. (Remember that
                           VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE_SIZE() may return
                           0 if its Namelen argument is invalid.) EOF can be
                           detected if Size is set on input like described
                           above, and Size is zero on output.

  @param[out] Data         Buffer to read the bytes from the regular file or
                           the directory stream into. The caller is responsible
                           for providing room for (at least) as many bytes in
                           Data as Size is on input.

  @retval EFI_SUCCESS  Read successful. The caller is responsible for checking
                       Size to learn the actual byte count transferred.

  @return              The "errno" value mapped to an EFI_STATUS code, if the
                       Virtio Filesystem device explicitly reported an error.

  @return              Error codes propagated from VirtioFsSgListsValidate(),
                       VirtioFsFuseNewRequest(), VirtioFsSgListsSubmit(),
                       VirtioFsFuseCheckResponse().
**/
EFI_STATUS
VirtioFsFuseReadFileOrDir (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  IN     UINT64     FuseHandle,
  IN     BOOLEAN    IsDir,
  IN     UINT64     Offset,
  IN OUT UINT32     *Size,
  OUT VOID          *Data
  )
{
  VIRTIO_FS_FUSE_REQUEST         CommonReq;
  VIRTIO_FS_FUSE_READ_REQUEST    ReadReq;
  VIRTIO_FS_IO_VECTOR            ReqIoVec[2];
  VIRTIO_FS_SCATTER_GATHER_LIST  ReqSgList;
  VIRTIO_FS_FUSE_RESPONSE        CommonResp;
  VIRTIO_FS_IO_VECTOR            RespIoVec[2];
  VIRTIO_FS_SCATTER_GATHER_LIST  RespSgList;
  EFI_STATUS                     Status;
  UINTN                          TailBufferFill;

  //
  // Set up the scatter-gather lists.
  //
  ReqIoVec[0].Buffer = &CommonReq;
  ReqIoVec[0].Size   = sizeof CommonReq;
  ReqIoVec[1].Buffer = &ReadReq;
  ReqIoVec[1].Size   = sizeof ReadReq;
  ReqSgList.IoVec    = ReqIoVec;
  ReqSgList.NumVec   = ARRAY_SIZE (ReqIoVec);

  RespIoVec[0].Buffer = &CommonResp;
  RespIoVec[0].Size   = sizeof CommonResp;
  RespIoVec[1].Buffer = Data;
  RespIoVec[1].Size   = *Size;
  RespSgList.IoVec    = RespIoVec;
  RespSgList.NumVec   = ARRAY_SIZE (RespIoVec);

  //
  // Validate the scatter-gather lists; calculate the total transfer sizes.
  //
  Status = VirtioFsSgListsValidate (VirtioFs, &ReqSgList, &RespSgList);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the common request header.
  //
  Status = VirtioFsFuseNewRequest (
             VirtioFs,
             &CommonReq,
             ReqSgList.TotalSize,
             IsDir ? VirtioFsFuseOpReadDirPlus : VirtioFsFuseOpRead,
             NodeId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the FUSE_READ- / FUSE_READDIRPLUS-specific fields.
  //
  ReadReq.FileHandle = FuseHandle;
  ReadReq.Offset     = Offset;
  ReadReq.Size       = *Size;
  ReadReq.ReadFlags  = 0;
  ReadReq.LockOwner  = 0;
  ReadReq.Flags      = 0;
  ReadReq.Padding    = 0;

  //
  // Submit the request.
  //
  Status = VirtioFsSgListsSubmit (VirtioFs, &ReqSgList, &RespSgList);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Verify the response. Note that TailBufferFill is variable.
  //
  Status = VirtioFsFuseCheckResponse (
             &RespSgList,
             CommonReq.Unique,
             &TailBufferFill
             );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_DEVICE_ERROR) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Label=\"%s\" NodeId=%Lu FuseHandle=%Lu "
        "IsDir=%d Offset=0x%Lx Size=0x%x Data@%p Errno=%d\n",
        __FUNCTION__,
        VirtioFs->Label,
        NodeId,
        FuseHandle,
        IsDir,
        Offset,
        *Size,
        Data,
        CommonResp.Error
        ));
      Status = VirtioFsErrnoToEfiStatus (CommonResp.Error);
    }

    return Status;
  }

  //
  // Report the actual transfer size.
  //
  // Integer overflow in the (UINT32) cast below is not possible; the
  // VIRTIO_FS_SCATTER_GATHER_LIST functions would have caught that.
  //
  *Size = (UINT32)TailBufferFill;
  return EFI_SUCCESS;
}
