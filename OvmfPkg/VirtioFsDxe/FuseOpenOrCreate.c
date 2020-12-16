/** @file
  FUSE_CREATE wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h> // AsciiStrSize()

#include "VirtioFsDxe.h"

/**
  Send a FUSE_CREATE request to the Virtio Filesystem device, for opening a
  regular file with (O_RDWR | O_CREAT) semantics.

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs  The Virtio Filesystem device to send the FUSE_CREATE
                           request to. On output, the FUSE request counter
                           "VirtioFs->RequestId" will have been incremented.

  @param[in] ParentNodeId  The inode number of the direct parent directory of
                           the regular file to open or create.

  @param[in] Name          The single-component filename of the regular file to
                           open or create, under the parent directory
                           identified by ParentNodeId.

  @param[out] NodeId       The inode number of the regular file, returned by
                           the Virtio Filesystem device.

  @param[out] FuseHandle   The open file handle returned by the Virtio
                           Filesystem device.

  @retval EFI_SUCCESS  The regular file has been opened, and (if necessary)
                       created.

  @return              The "errno" value mapped to an EFI_STATUS code, if the
                       Virtio Filesystem device explicitly reported an error.

  @return              Error codes propagated from VirtioFsSgListsValidate(),
                       VirtioFsFuseNewRequest(), VirtioFsSgListsSubmit(),
                       VirtioFsFuseCheckResponse().
**/
EFI_STATUS
VirtioFsFuseOpenOrCreate (
  IN OUT VIRTIO_FS *VirtioFs,
  IN     UINT64    ParentNodeId,
  IN     CHAR8     *Name,
     OUT UINT64    *NodeId,
     OUT UINT64    *FuseHandle
  )
{
  VIRTIO_FS_FUSE_REQUEST             CommonReq;
  VIRTIO_FS_FUSE_CREATE_REQUEST      CreateReq;
  VIRTIO_FS_IO_VECTOR                ReqIoVec[3];
  VIRTIO_FS_SCATTER_GATHER_LIST      ReqSgList;
  VIRTIO_FS_FUSE_RESPONSE            CommonResp;
  VIRTIO_FS_FUSE_NODE_RESPONSE       NodeResp;
  VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE AttrResp;
  VIRTIO_FS_FUSE_OPEN_RESPONSE       OpenResp;
  VIRTIO_FS_IO_VECTOR                RespIoVec[4];
  VIRTIO_FS_SCATTER_GATHER_LIST      RespSgList;
  EFI_STATUS                         Status;

  //
  // Set up the scatter-gather lists.
  //
  ReqIoVec[0].Buffer = &CommonReq;
  ReqIoVec[0].Size   = sizeof CommonReq;
  ReqIoVec[1].Buffer = &CreateReq;
  ReqIoVec[1].Size   = sizeof CreateReq;
  ReqIoVec[2].Buffer = Name;
  ReqIoVec[2].Size   = AsciiStrSize (Name);
  ReqSgList.IoVec    = ReqIoVec;
  ReqSgList.NumVec   = ARRAY_SIZE (ReqIoVec);

  RespIoVec[0].Buffer = &CommonResp;
  RespIoVec[0].Size   = sizeof CommonResp;
  RespIoVec[1].Buffer = &NodeResp;
  RespIoVec[1].Size   = sizeof NodeResp;
  RespIoVec[2].Buffer = &AttrResp;
  RespIoVec[2].Size   = sizeof AttrResp;
  RespIoVec[3].Buffer = &OpenResp;
  RespIoVec[3].Size   = sizeof OpenResp;
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
  Status = VirtioFsFuseNewRequest (VirtioFs, &CommonReq, ReqSgList.TotalSize,
             VirtioFsFuseOpCreate, ParentNodeId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the FUSE_CREATE-specific fields.
  //
  // VIRTIO_FS_FUSE_OPEN_REQ_F_RDWR is why this request can never open a
  // directory (EISDIR). And VIRTIO_FS_FUSE_OPEN_REQ_F_RDWR is consistent with
  // the only OpenMode of EFI_FILE_PROTOCOL.Open() that enables filesystem
  // object creation -- that is, Create/Read/Write.
  //
  CreateReq.Flags   = VIRTIO_FS_FUSE_OPEN_REQ_F_RDWR;
  CreateReq.Mode    = (VIRTIO_FS_FUSE_MODE_PERM_RUSR |
                       VIRTIO_FS_FUSE_MODE_PERM_WUSR |
                       VIRTIO_FS_FUSE_MODE_PERM_RGRP |
                       VIRTIO_FS_FUSE_MODE_PERM_WGRP |
                       VIRTIO_FS_FUSE_MODE_PERM_ROTH |
                       VIRTIO_FS_FUSE_MODE_PERM_WOTH);
  CreateReq.Umask   = 0;
  CreateReq.Padding = 0;

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
      DEBUG ((DEBUG_ERROR, "%a: Label=\"%s\" ParentNodeId=%Lu Name=\"%a\" "
        "Errno=%d\n", __FUNCTION__, VirtioFs->Label, ParentNodeId, Name,
        CommonResp.Error));
      Status = VirtioFsErrnoToEfiStatus (CommonResp.Error);
    }
    return Status;
  }

  //
  // Output the NodeId of the (possibly new) regular file. Also output the open
  // file handle.
  //
  *NodeId     = NodeResp.NodeId;
  *FuseHandle = OpenResp.FileHandle;
  return EFI_SUCCESS;
}
