/** @file
  FUSE_SETATTR wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

/**
  Send the FUSE_SETATTR request to the Virtio Filesystem device, for changing
  the attributes of an inode.

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs  The Virtio Filesystem device to send the
                           FUSE_SETATTR request to. On output, the FUSE request
                           counter "VirtioFs->RequestId" will have been
                           incremented.

  @param[in] NodeId        The inode number representing the regular file or
                           directory whose attributes should be changed.

  @param[in] Size          The new size to set for the regular file. If NULL,
                           then the file size will not be changed. If NodeId
                           refers to a directory, then the caller is
                           responsible for passing NULL as Size.

  @param[in] Atime         The new last access time to set for the regular file
                           or directory (seconds since the Epoch). If NULL,
                           then the last access time is not changed.

  @param[in] Mtime         The new last modification time to set for the
                           regular file or directory (seconds since the Epoch).
                           If NULL, then the last modification time is not
                           changed.

  @param[in] Mode          The new file mode bits to set for the regular file
                           or directory. If NULL, then the file mode bits are
                           not changed.

  @retval EFI_SUCCESS  The attributes have been updated.

  @return              The "errno" value mapped to an EFI_STATUS code, if the
                       Virtio Filesystem device explicitly reported an error.

  @return              Error codes propagated from VirtioFsSgListsValidate(),
                       VirtioFsFuseNewRequest(), VirtioFsSgListsSubmit(),
                       VirtioFsFuseCheckResponse().
**/
EFI_STATUS
VirtioFsFuseSetAttr (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     NodeId,
  IN     UINT64     *Size      OPTIONAL,
  IN     UINT64     *Atime     OPTIONAL,
  IN     UINT64     *Mtime     OPTIONAL,
  IN     UINT32     *Mode      OPTIONAL
  )
{
  VIRTIO_FS_FUSE_REQUEST              CommonReq;
  VIRTIO_FS_FUSE_SETATTR_REQUEST      AttrReq;
  VIRTIO_FS_IO_VECTOR                 ReqIoVec[2];
  VIRTIO_FS_SCATTER_GATHER_LIST       ReqSgList;
  VIRTIO_FS_FUSE_RESPONSE             CommonResp;
  VIRTIO_FS_FUSE_GETATTR_RESPONSE     GetAttrResp;
  VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE  AttrResp;
  VIRTIO_FS_IO_VECTOR                 RespIoVec[3];
  VIRTIO_FS_SCATTER_GATHER_LIST       RespSgList;
  EFI_STATUS                          Status;

  //
  // Set up the scatter-gather lists.
  //
  ReqIoVec[0].Buffer = &CommonReq;
  ReqIoVec[0].Size   = sizeof CommonReq;
  ReqIoVec[1].Buffer = &AttrReq;
  ReqIoVec[1].Size   = sizeof AttrReq;
  ReqSgList.IoVec    = ReqIoVec;
  ReqSgList.NumVec   = ARRAY_SIZE (ReqIoVec);

  RespIoVec[0].Buffer = &CommonResp;
  RespIoVec[0].Size   = sizeof CommonResp;
  RespIoVec[1].Buffer = &GetAttrResp;
  RespIoVec[1].Size   = sizeof GetAttrResp;
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
             VirtioFsFuseOpSetAttr,
             NodeId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the FUSE_SETATTR-specific fields.
  //
  AttrReq.Valid      = 0;
  AttrReq.Padding    = 0;
  AttrReq.FileHandle = 0;
  AttrReq.Size       = (Size == NULL) ? 0 : *Size;
  AttrReq.LockOwner  = 0;
  AttrReq.Atime      = (Atime == NULL) ? 0 : *Atime;
  AttrReq.Mtime      = (Mtime == NULL) ? 0 : *Mtime;
  AttrReq.Ctime      = 0;
  AttrReq.AtimeNsec  = 0;
  AttrReq.MtimeNsec  = 0;
  AttrReq.CtimeNsec  = 0;
  AttrReq.Mode       = (Mode == NULL) ? 0 : *Mode;
  AttrReq.Unused4    = 0;
  AttrReq.Uid        = 0;
  AttrReq.Gid        = 0;
  AttrReq.Unused5    = 0;

  if (Size != NULL) {
    AttrReq.Valid |= VIRTIO_FS_FUSE_SETATTR_REQ_F_SIZE;
  }

  if (Atime != NULL) {
    AttrReq.Valid |= VIRTIO_FS_FUSE_SETATTR_REQ_F_ATIME;
  }

  if (Mtime != NULL) {
    AttrReq.Valid |= VIRTIO_FS_FUSE_SETATTR_REQ_F_MTIME;
  }

  if (Mode != NULL) {
    AttrReq.Valid |= VIRTIO_FS_FUSE_SETATTR_REQ_F_MODE;
  }

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
      "%a: Label=\"%s\" NodeId=%Lu",
      __FUNCTION__,
      VirtioFs->Label,
      NodeId
      ));
    if (Size != NULL) {
      DEBUG ((DEBUG_ERROR, " Size=0x%Lx", *Size));
    }

    if (Atime != NULL) {
      DEBUG ((DEBUG_ERROR, " Atime=%Lu", *Atime));
    }

    if (Mtime != NULL) {
      DEBUG ((DEBUG_ERROR, " Mtime=%Lu", *Mtime));
    }

    if (Mode != NULL) {
      DEBUG ((DEBUG_ERROR, " Mode=0x%x", *Mode)); // no support for octal :/
    }

    DEBUG ((DEBUG_ERROR, " Errno=%d\n", CommonResp.Error));
    Status = VirtioFsErrnoToEfiStatus (CommonResp.Error);
  }

  return Status;
}
