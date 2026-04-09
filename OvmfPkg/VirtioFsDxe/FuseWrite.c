/** @file
  FUSE_WRITE wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

/**
  Write a chunk to a regular file, by sending the FUSE_WRITE request to the
  Virtio Filesystem device.

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs  The Virtio Filesystem device to send the FUSE_WRITE
                           request to. On output, the FUSE request counter
                           "VirtioFs->RequestId" will have been incremented.

  @param[in] NodeId        The inode number of the regular file to write to.

  @param[in] FuseHandle    The open handle to the regular file to write to.

  @param[in] Offset        The absolute file position at which to start
                           writing.

  @param[in,out] Size      On input, the number of bytes to write. On
                           successful return, the number of bytes actually
                           written, which may be smaller than the value on
                           input.

  @param[in] Data          The buffer to write to the regular file.

  @retval EFI_SUCCESS          Write successful. The caller is responsible for
                               checking Size to learn the actual byte count
                               transferred.

  @retval EFI_BAD_BUFFER_SIZE  On input, Size is larger than
                               "VirtioFs->MaxWrite".

  @return                      The "errno" value mapped to an EFI_STATUS code,
                               if the Virtio Filesystem device explicitly
                               reported an error.

  @return                      Error codes propagated from
                               VirtioFsSgListsValidate(),
                               VirtioFsFuseNewRequest(),
                               VirtioFsSgListsSubmit(),
                               VirtioFsFuseCheckResponse().
**/
EFI_STATUS
VirtioFsFuseWrite (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  IN     UINT64     FuseHandle,
  IN     UINT64     Offset,
  IN OUT UINT32     *Size,
  IN     VOID       *Data
  )
{
  VIRTIO_FS_FUSE_REQUEST         CommonReq;
  VIRTIO_FS_FUSE_WRITE_REQUEST   WriteReq;
  VIRTIO_FS_IO_VECTOR            ReqIoVec[3];
  VIRTIO_FS_SCATTER_GATHER_LIST  ReqSgList;
  VIRTIO_FS_FUSE_RESPONSE        CommonResp;
  VIRTIO_FS_FUSE_WRITE_RESPONSE  WriteResp;
  VIRTIO_FS_IO_VECTOR            RespIoVec[2];
  VIRTIO_FS_SCATTER_GATHER_LIST  RespSgList;
  EFI_STATUS                     Status;

  //
  // Honor the write buffer size limit of the Virtio Filesystem device.
  //
  if (*Size > VirtioFs->MaxWrite) {
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // Set up the scatter-gather lists.
  //
  ReqIoVec[0].Buffer = &CommonReq;
  ReqIoVec[0].Size   = sizeof CommonReq;
  ReqIoVec[1].Buffer = &WriteReq;
  ReqIoVec[1].Size   = sizeof WriteReq;
  ReqIoVec[2].Buffer = Data;
  ReqIoVec[2].Size   = *Size;
  ReqSgList.IoVec    = ReqIoVec;
  ReqSgList.NumVec   = ARRAY_SIZE (ReqIoVec);

  RespIoVec[0].Buffer = &CommonResp;
  RespIoVec[0].Size   = sizeof CommonResp;
  RespIoVec[1].Buffer = &WriteResp;
  RespIoVec[1].Size   = sizeof WriteResp;
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
             VirtioFsFuseOpWrite,
             NodeId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the FUSE_WRITE-specific fields.
  //
  WriteReq.FileHandle = FuseHandle;
  WriteReq.Offset     = Offset;
  WriteReq.Size       = *Size;
  WriteReq.WriteFlags = 0;
  WriteReq.LockOwner  = 0;
  WriteReq.Flags      = 0;
  WriteReq.Padding    = 0;

  //
  // Submit the request.
  //
  Status = VirtioFsSgListsSubmit (VirtioFs, &ReqSgList, &RespSgList);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Verify the response (all response buffers are fixed size).
  //
  Status = VirtioFsFuseCheckResponse (&RespSgList, CommonReq.Unique, NULL);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_DEVICE_ERROR) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Label=\"%s\" NodeId=%Lu FuseHandle=%Lu "
        "Offset=0x%Lx Size=0x%x Data@%p Errno=%d\n",
        __func__,
        VirtioFs->Label,
        NodeId,
        FuseHandle,
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
  *Size = WriteResp.Size;
  return EFI_SUCCESS;
}
