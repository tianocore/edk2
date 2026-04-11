/** @file
  FUSE_OPEN wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

/**
  Send a FUSE_OPEN request to the Virtio Filesystem device, for opening a
  regular file.

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs  The Virtio Filesystem device to send the FUSE_OPEN
                           request to. On output, the FUSE request counter
                           "VirtioFs->RequestId" will have been incremented.

  @param[in] NodeId        The inode number of the regular file to open.

  @param[in] ReadWrite     If TRUE, open the regular file in read-write mode.
                           If FALSE, open the regular file in read-only mode.

  @param[out] FuseHandle   The open handle to the regular file, returned by the
                           Virtio Filesystem device.

  @retval EFI_SUCCESS  The regular file has been opened.

  @return              The "errno" value mapped to an EFI_STATUS code, if the
                       Virtio Filesystem device explicitly reported an error.

  @return              Error codes propagated from VirtioFsSgListsValidate(),
                       VirtioFsFuseNewRequest(), VirtioFsSgListsSubmit(),
                       VirtioFsFuseCheckResponse().
**/
EFI_STATUS
VirtioFsFuseOpen (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  IN     BOOLEAN    ReadWrite,
  OUT UINT64        *FuseHandle
  )
{
  VIRTIO_FS_FUSE_REQUEST         CommonReq;
  VIRTIO_FS_FUSE_OPEN_REQUEST    OpenReq;
  VIRTIO_FS_IO_VECTOR            ReqIoVec[2];
  VIRTIO_FS_SCATTER_GATHER_LIST  ReqSgList;
  VIRTIO_FS_FUSE_RESPONSE        CommonResp;
  VIRTIO_FS_FUSE_OPEN_RESPONSE   OpenResp;
  VIRTIO_FS_IO_VECTOR            RespIoVec[2];
  VIRTIO_FS_SCATTER_GATHER_LIST  RespSgList;
  EFI_STATUS                     Status;

  //
  // Set up the scatter-gather lists.
  //
  ReqIoVec[0].Buffer = &CommonReq;
  ReqIoVec[0].Size   = sizeof CommonReq;
  ReqIoVec[1].Buffer = &OpenReq;
  ReqIoVec[1].Size   = sizeof OpenReq;
  ReqSgList.IoVec    = ReqIoVec;
  ReqSgList.NumVec   = ARRAY_SIZE (ReqIoVec);

  RespIoVec[0].Buffer = &CommonResp;
  RespIoVec[0].Size   = sizeof CommonResp;
  RespIoVec[1].Buffer = &OpenResp;
  RespIoVec[1].Size   = sizeof OpenResp;
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
             VirtioFsFuseOpOpen,
             NodeId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the FUSE_OPEN-specific fields.
  //
  OpenReq.Flags = (ReadWrite ?
                   VIRTIO_FS_FUSE_OPEN_REQ_F_RDWR :
                   VIRTIO_FS_FUSE_OPEN_REQ_F_RDONLY);
  OpenReq.Unused = 0;

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
        "%a: Label=\"%s\" NodeId=%Lu ReadWrite=%d "
        "Errno=%d\n",
        __func__,
        VirtioFs->Label,
        NodeId,
        ReadWrite,
        CommonResp.Error
        ));
      Status = VirtioFsErrnoToEfiStatus (CommonResp.Error);
    }

    return Status;
  }

  //
  // Output the open handle.
  //
  *FuseHandle = OpenResp.FileHandle;
  return EFI_SUCCESS;
}
