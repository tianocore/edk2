/** @file
  FUSE_GETATTR wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

/**
  Send a FUSE_GETATTR request to the Virtio Filesystem device, for fetching the
  attributes of an inode.

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs  The Virtio Filesystem device to send the
                           FUSE_GETATTR request to. On output, the FUSE request
                           counter "VirtioFs->RequestId" will have been
                           incremented.

  @param[in] NodeId        The inode number for which the attributes should be
                           retrieved.

  @param[out] FuseAttr     The VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE object
                           describing the properties of the inode.

  @retval EFI_SUCCESS  FuseAttr has been filled in.

  @return              The "errno" value mapped to an EFI_STATUS code, if the
                       Virtio Filesystem device explicitly reported an error.

  @return              Error codes propagated from VirtioFsSgListsValidate(),
                       VirtioFsFuseNewRequest(), VirtioFsSgListsSubmit(),
                       VirtioFsFuseCheckResponse().
**/
EFI_STATUS
VirtioFsFuseGetAttr (
  IN OUT VIRTIO_FS                        *VirtioFs,
  IN     UINT64                           NodeId,
  OUT VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE  *FuseAttr
  )
{
  VIRTIO_FS_FUSE_REQUEST           CommonReq;
  VIRTIO_FS_FUSE_GETATTR_REQUEST   GetAttrReq;
  VIRTIO_FS_IO_VECTOR              ReqIoVec[2];
  VIRTIO_FS_SCATTER_GATHER_LIST    ReqSgList;
  VIRTIO_FS_FUSE_RESPONSE          CommonResp;
  VIRTIO_FS_FUSE_GETATTR_RESPONSE  GetAttrResp;
  VIRTIO_FS_IO_VECTOR              RespIoVec[3];
  VIRTIO_FS_SCATTER_GATHER_LIST    RespSgList;
  EFI_STATUS                       Status;

  //
  // Set up the scatter-gather lists.
  //
  ReqIoVec[0].Buffer = &CommonReq;
  ReqIoVec[0].Size   = sizeof CommonReq;
  ReqIoVec[1].Buffer = &GetAttrReq;
  ReqIoVec[1].Size   = sizeof GetAttrReq;
  ReqSgList.IoVec    = ReqIoVec;
  ReqSgList.NumVec   = ARRAY_SIZE (ReqIoVec);

  RespIoVec[0].Buffer = &CommonResp;
  RespIoVec[0].Size   = sizeof CommonResp;
  RespIoVec[1].Buffer = &GetAttrResp;
  RespIoVec[1].Size   = sizeof GetAttrResp;
  RespIoVec[2].Buffer = FuseAttr;
  RespIoVec[2].Size   = sizeof *FuseAttr;
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
             VirtioFsFuseOpGetAttr,
             NodeId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the FUSE_GETATTR-specific fields.
  //
  GetAttrReq.GetAttrFlags = 0;
  GetAttrReq.Dummy        = 0;
  GetAttrReq.FileHandle   = 0;

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
      "%a: Label=\"%s\" NodeId=%Lu Errno=%d\n",
      __func__,
      VirtioFs->Label,
      NodeId,
      CommonResp.Error
      ));
    Status = VirtioFsErrnoToEfiStatus (CommonResp.Error);
  }

  return Status;
}
