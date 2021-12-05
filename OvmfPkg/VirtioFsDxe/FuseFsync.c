/** @file
  FUSE_FSYNC / FUSE_FSYNCDIR wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

/**
  Synchronize the in-core state of a regular file or a directory with the
  storage device on the host, by sending the FUSE_FSYNC or FUSE_FSYNCDIR
  request to the Virtio Filesystem device.

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs  The Virtio Filesystem device to send the FUSE_FSYNC
                           / FUSE_FSYNCDIR request to. On output, the FUSE
                           request counter "VirtioFs->RequestId" will have been
                           incremented.

  @param[in] NodeId        The inode number of the file or directory to sync.

  @param[in] FuseHandle    The open handle to the file or directory to sync.

  @param[in] IsDir         TRUE if NodeId and FuseHandle refer to a directory,
                           FALSE if NodeId and FuseHandle refer to a regular
                           file.

  @retval EFI_SUCCESS  The file or directory has been synchronized.

  @return              The "errno" value mapped to an EFI_STATUS code, if the
                       Virtio Filesystem device explicitly reported an error.

  @return              Error codes propagated from VirtioFsSgListsValidate(),
                       VirtioFsFuseNewRequest(), VirtioFsSgListsSubmit(),
                       VirtioFsFuseCheckResponse().
**/
EFI_STATUS
VirtioFsFuseFsyncFileOrDir (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  IN     UINT64     FuseHandle,
  IN     BOOLEAN    IsDir
  )
{
  VIRTIO_FS_FUSE_REQUEST         CommonReq;
  VIRTIO_FS_FUSE_FSYNC_REQUEST   FsyncReq;
  VIRTIO_FS_IO_VECTOR            ReqIoVec[2];
  VIRTIO_FS_SCATTER_GATHER_LIST  ReqSgList;
  VIRTIO_FS_FUSE_RESPONSE        CommonResp;
  VIRTIO_FS_IO_VECTOR            RespIoVec[1];
  VIRTIO_FS_SCATTER_GATHER_LIST  RespSgList;
  EFI_STATUS                     Status;

  //
  // Set up the scatter-gather lists.
  //
  ReqIoVec[0].Buffer = &CommonReq;
  ReqIoVec[0].Size   = sizeof CommonReq;
  ReqIoVec[1].Buffer = &FsyncReq;
  ReqIoVec[1].Size   = sizeof FsyncReq;
  ReqSgList.IoVec    = ReqIoVec;
  ReqSgList.NumVec   = ARRAY_SIZE (ReqIoVec);

  RespIoVec[0].Buffer = &CommonResp;
  RespIoVec[0].Size   = sizeof CommonResp;
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
             IsDir ? VirtioFsFuseOpFsyncDir : VirtioFsFuseOpFsync,
             NodeId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the FUSE_FSYNC- / FUSE_FSYNCDIR-specific fields.
  //
  FsyncReq.FileHandle = FuseHandle;
  FsyncReq.FsyncFlags = 0;
  FsyncReq.Padding    = 0;

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
  if (Status == EFI_DEVICE_ERROR) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Label=\"%s\" NodeId=%Lu FuseHandle=%Lu "
      "IsDir=%d Errno=%d\n",
      __FUNCTION__,
      VirtioFs->Label,
      NodeId,
      FuseHandle,
      IsDir,
      CommonResp.Error
      ));
    Status = VirtioFsErrnoToEfiStatus (CommonResp.Error);
  }

  return Status;
}
