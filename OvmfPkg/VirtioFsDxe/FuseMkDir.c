/** @file
  FUSE_MKDIR wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h> // AsciiStrSize()

#include "VirtioFsDxe.h"

/**
  Send a FUSE_MKDIR request to the Virtio Filesystem device, for creating a
  directory.

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs  The Virtio Filesystem device to send the FUSE_MKDIR
                           request to. On output, the FUSE request counter
                           "VirtioFs->RequestId" will have been incremented.

  @param[in] ParentNodeId  The inode number of the direct parent directory of
                           the directory to create.

  @param[in] Name          The single-component filename of the directory to
                           create, under the parent directory identified by
                           ParentNodeId.

  @param[out] NodeId       The inode number of the new directory.

  @retval EFI_SUCCESS  The directory has been created.

  @return              The "errno" value mapped to an EFI_STATUS code, if the
                       Virtio Filesystem device explicitly reported an error.

  @return              Error codes propagated from VirtioFsSgListsValidate(),
                       VirtioFsFuseNewRequest(), VirtioFsSgListsSubmit(),
                       VirtioFsFuseCheckResponse().
**/
EFI_STATUS
VirtioFsFuseMkDir (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     ParentNodeId,
  IN     CHAR8      *Name,
  OUT UINT64        *NodeId
  )
{
  VIRTIO_FS_FUSE_REQUEST              CommonReq;
  VIRTIO_FS_FUSE_MKDIR_REQUEST        MkDirReq;
  VIRTIO_FS_IO_VECTOR                 ReqIoVec[3];
  VIRTIO_FS_SCATTER_GATHER_LIST       ReqSgList;
  VIRTIO_FS_FUSE_RESPONSE             CommonResp;
  VIRTIO_FS_FUSE_NODE_RESPONSE        NodeResp;
  VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE  AttrResp;
  VIRTIO_FS_IO_VECTOR                 RespIoVec[3];
  VIRTIO_FS_SCATTER_GATHER_LIST       RespSgList;
  EFI_STATUS                          Status;

  //
  // Set up the scatter-gather lists.
  //
  ReqIoVec[0].Buffer = &CommonReq;
  ReqIoVec[0].Size   = sizeof CommonReq;
  ReqIoVec[1].Buffer = &MkDirReq;
  ReqIoVec[1].Size   = sizeof MkDirReq;
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
             VirtioFsFuseOpMkDir,
             ParentNodeId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the FUSE_MKDIR-specific fields.
  //
  MkDirReq.Mode = (VIRTIO_FS_FUSE_MODE_PERM_RWXU |
                   VIRTIO_FS_FUSE_MODE_PERM_RWXG |
                   VIRTIO_FS_FUSE_MODE_PERM_RWXO);
  MkDirReq.Umask = 0;

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
        "%a: Label=\"%s\" ParentNodeId=%Lu Name=\"%a\" "
        "Errno=%d\n",
        __FUNCTION__,
        VirtioFs->Label,
        ParentNodeId,
        Name,
        CommonResp.Error
        ));
      Status = VirtioFsErrnoToEfiStatus (CommonResp.Error);
    }

    return Status;
  }

  //
  // Output the NodeId of the new directory.
  //
  *NodeId = NodeResp.NodeId;
  return EFI_SUCCESS;
}
