/** @file
  FUSE_INIT wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

/**
  Send a FUSE_INIT request to the Virtio Filesystem device, for starting the
  FUSE session.

  From virtio-v1.1-cs01-87fa6b5d8155, 5.11.5 Device Initialization: "On
  initialization the driver first discovers the device's virtqueues. The FUSE
  session is started by sending a FUSE_INIT request as defined by the FUSE
  protocol on one request virtqueue."

  The function may only be called after VirtioFsInit() returns successfully and
  before VirtioFsUninit() is called.

  @param[in,out] VirtioFs  The Virtio Filesystem device to send the FUSE_INIT
                           request to. The FUSE request counter
                           "VirtioFs->RequestId" is set to 1 on output. The
                           maximum write buffer size exposed in the FUSE_INIT
                           response is saved in "VirtioFs->MaxWrite", on
                           output.

  @retval EFI_SUCCESS      The FUSE session has been started.

  @retval EFI_UNSUPPORTED  FUSE interface version or feature negotiation
                           failed.

  @return                  The "errno" value mapped to an EFI_STATUS code, if
                           the Virtio Filesystem device explicitly reported an
                           error.

  @return                  Error codes propagated from
                           VirtioFsSgListsValidate(), VirtioFsFuseNewRequest(),
                           VirtioFsSgListsSubmit(),
                           VirtioFsFuseCheckResponse().
**/
EFI_STATUS
VirtioFsFuseInitSession (
  IN OUT VIRTIO_FS  *VirtioFs
  )
{
  VIRTIO_FS_FUSE_REQUEST         CommonReq;
  VIRTIO_FS_FUSE_INIT_REQUEST    InitReq;
  VIRTIO_FS_IO_VECTOR            ReqIoVec[2];
  VIRTIO_FS_SCATTER_GATHER_LIST  ReqSgList;
  VIRTIO_FS_FUSE_RESPONSE        CommonResp;
  VIRTIO_FS_FUSE_INIT_RESPONSE   InitResp;
  VIRTIO_FS_IO_VECTOR            RespIoVec[2];
  VIRTIO_FS_SCATTER_GATHER_LIST  RespSgList;
  EFI_STATUS                     Status;

  //
  // Initialize the FUSE request counter.
  //
  VirtioFs->RequestId = 1;

  //
  // Set up the scatter-gather lists.
  //
  ReqIoVec[0].Buffer = &CommonReq;
  ReqIoVec[0].Size   = sizeof CommonReq;
  ReqIoVec[1].Buffer = &InitReq;
  ReqIoVec[1].Size   = sizeof InitReq;
  ReqSgList.IoVec    = ReqIoVec;
  ReqSgList.NumVec   = ARRAY_SIZE (ReqIoVec);

  RespIoVec[0].Buffer = &CommonResp;
  RespIoVec[0].Size   = sizeof CommonResp;
  RespIoVec[1].Buffer = &InitResp;
  RespIoVec[1].Size   = sizeof InitResp;
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
             VirtioFsFuseOpInit,
             0
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the FUSE_INIT-specific fields.
  //
  InitReq.Major        = VIRTIO_FS_FUSE_MAJOR;
  InitReq.Minor        = VIRTIO_FS_FUSE_MINOR;
  InitReq.MaxReadahead = 0;
  InitReq.Flags        = VIRTIO_FS_FUSE_INIT_REQ_F_DO_READDIRPLUS;

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
        "%a: Label=\"%s\" Errno=%d\n",
        __func__,
        VirtioFs->Label,
        CommonResp.Error
        ));
      Status = VirtioFsErrnoToEfiStatus (CommonResp.Error);
    }

    return Status;
  }

  //
  // Check FUSE interface version / feature compatibility.
  //
  if ((InitResp.Major < InitReq.Major) ||
      ((InitResp.Major == InitReq.Major) && (InitResp.Minor < InitReq.Minor)) ||
      ((InitResp.Flags & VIRTIO_FS_FUSE_INIT_REQ_F_DO_READDIRPLUS) == 0) ||
      (InitResp.MaxWrite < SIZE_4KB))
  {
    return EFI_UNSUPPORTED;
  }

  //
  // Save the maximum write buffer size for FUSE_WRITE requests.
  //
  VirtioFs->MaxWrite = InitResp.MaxWrite;
  return EFI_SUCCESS;
}
