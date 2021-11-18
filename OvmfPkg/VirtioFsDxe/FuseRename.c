/** @file
  FUSE_RENAME2 wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h> // AsciiStrSize()

#include "VirtioFsDxe.h"

/**
  Rename a regular file or a directory, by sending the FUSE_RENAME2 request to
  the Virtio Filesystem device. If the new filename exists, the request will
  fail.

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs     The Virtio Filesystem device to send the
                              FUSE_RENAME2 request to. On output, the FUSE
                              request counter "VirtioFs->RequestId" will have
                              been incremented.

  @param[in] OldParentNodeId  The inode number of the directory in which
                              OldName should be removed.

  @param[in] OldName          The single-component filename to remove in the
                              directory identified by OldParentNodeId.

  @param[in] NewParentNodeId  The inode number of the directory in which
                              NewName should be created, such that on
                              successful return, (NewParentNodeId, NewName)
                              refer to the same inode as (OldParentNodeId,
                              OldName) did on entry.

  @param[in] NewName          The single-component filename to create in the
                              directory identified by NewParentNodeId.

  @retval EFI_SUCCESS  The file or directory has been renamed.

  @return              The "errno" value mapped to an EFI_STATUS code, if the
                       Virtio Filesystem device explicitly reported an error.

  @return              Error codes propagated from VirtioFsSgListsValidate(),
                       VirtioFsFuseNewRequest(), VirtioFsSgListsSubmit(),
                       VirtioFsFuseCheckResponse().
**/
EFI_STATUS
VirtioFsFuseRename (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     OldParentNodeId,
  IN     CHAR8      *OldName,
  IN     UINT64     NewParentNodeId,
  IN     CHAR8      *NewName
  )
{
  VIRTIO_FS_FUSE_REQUEST          CommonReq;
  VIRTIO_FS_FUSE_RENAME2_REQUEST  Rename2Req;
  VIRTIO_FS_IO_VECTOR             ReqIoVec[4];
  VIRTIO_FS_SCATTER_GATHER_LIST   ReqSgList;
  VIRTIO_FS_FUSE_RESPONSE         CommonResp;
  VIRTIO_FS_IO_VECTOR             RespIoVec[1];
  VIRTIO_FS_SCATTER_GATHER_LIST   RespSgList;
  EFI_STATUS                      Status;

  //
  // Set up the scatter-gather lists.
  //
  ReqIoVec[0].Buffer = &CommonReq;
  ReqIoVec[0].Size   = sizeof CommonReq;
  ReqIoVec[1].Buffer = &Rename2Req;
  ReqIoVec[1].Size   = sizeof Rename2Req;
  ReqIoVec[2].Buffer = OldName;
  ReqIoVec[2].Size   = AsciiStrSize (OldName);
  ReqIoVec[3].Buffer = NewName;
  ReqIoVec[3].Size   = AsciiStrSize (NewName);
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
             VirtioFsFuseOpRename2,
             OldParentNodeId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the FUSE_RENAME2-specific fields.
  //
  Rename2Req.NewDir  = NewParentNodeId;
  Rename2Req.Flags   = VIRTIO_FS_FUSE_RENAME2_REQ_F_NOREPLACE;
  Rename2Req.Padding = 0;

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
      "%a: Label=\"%s\" OldParentNodeId=%Lu OldName=\"%a\" "
      "NewParentNodeId=%Lu NewName=\"%a\" Errno=%d\n",
      __FUNCTION__,
      VirtioFs->Label,
      OldParentNodeId,
      OldName,
      NewParentNodeId,
      NewName,
      CommonResp.Error
      ));
    Status = VirtioFsErrnoToEfiStatus (CommonResp.Error);
  }

  return Status;
}
