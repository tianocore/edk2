/** @file
  FUSE_FORGET wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

/**
  Make the Virtio Filesysem device drop one reference count from a NodeId that
  the driver looked up by filename.

  Send the FUSE_FORGET request to the Virtio Filesysem device for this. Unlike
  most other FUSE requests, FUSE_FORGET doesn't elicit a response, not even the
  common VIRTIO_FS_FUSE_RESPONSE header.

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs  The Virtio Filesystem device to send the FUSE_FORGET
                           request to. On output, the FUSE request counter
                           "VirtioFs->RequestId" will have been incremented.

  @param[in] NodeId        The inode number that the client learned by way of
                           lookup, and that the server should now un-reference
                           exactly once.

  @retval EFI_SUCCESS  The FUSE_FORGET request has been submitted.

  @return              Error codes propagated from VirtioFsSgListsValidate(),
                       VirtioFsFuseNewRequest(), VirtioFsSgListsSubmit().
**/
EFI_STATUS
VirtioFsFuseForget (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId
  )
{
  VIRTIO_FS_FUSE_REQUEST         CommonReq;
  VIRTIO_FS_FUSE_FORGET_REQUEST  ForgetReq;
  VIRTIO_FS_IO_VECTOR            ReqIoVec[2];
  VIRTIO_FS_SCATTER_GATHER_LIST  ReqSgList;
  EFI_STATUS                     Status;

  //
  // Set up the scatter-gather list (note: only request).
  //
  ReqIoVec[0].Buffer = &CommonReq;
  ReqIoVec[0].Size   = sizeof CommonReq;
  ReqIoVec[1].Buffer = &ForgetReq;
  ReqIoVec[1].Size   = sizeof ForgetReq;
  ReqSgList.IoVec    = ReqIoVec;
  ReqSgList.NumVec   = ARRAY_SIZE (ReqIoVec);

  //
  // Validate the scatter-gather list (request only); calculate the total
  // transfer size.
  //
  Status = VirtioFsSgListsValidate (VirtioFs, &ReqSgList, NULL);
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
             VirtioFsFuseOpForget,
             NodeId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the FUSE_FORGET-specific fields.
  //
  ForgetReq.NumberOfLookups = 1;

  //
  // Submit the request. There's not going to be a response.
  //
  Status = VirtioFsSgListsSubmit (VirtioFs, &ReqSgList, NULL);
  return Status;
}
