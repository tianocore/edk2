/** @file
  FUSE_UNLINK / FUSE_RMDIR wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h> // AsciiStrSize()

#include "VirtioFsDxe.h"

/**
  Remove a regular file or a directory, by sending the FUSE_UNLINK or
  FUSE_RMDIR request to the Virtio Filesystem device.

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs  The Virtio Filesystem device to send the FUSE_UNLINK
                           / FUSE_RMDIR request to. On output, the FUSE request
                           counter "VirtioFs->RequestId" will have been
                           incremented.

  @param[in] ParentNodeId  The inode number of the directory in which Name
                           should be removed.

  @param[in] Name          The single-component filename to remove in the
                           directory identified by ParentNodeId.

  @param[in] IsDir         TRUE if Name refers to a directory, FALSE otherwise.

  @retval EFI_SUCCESS  The file or directory has been removed.

  @return              The "errno" value mapped to an EFI_STATUS code, if the
                       Virtio Filesystem device explicitly reported an error.

  @return              Error codes propagated from VirtioFsSgListsValidate(),
                       VirtioFsFuseNewRequest(), VirtioFsSgListsSubmit(),
                       VirtioFsFuseCheckResponse().
**/
EFI_STATUS
VirtioFsFuseRemoveFileOrDir (
  IN OUT VIRTIO_FS  *VirtioFs,
  IN     UINT64     ParentNodeId,
  IN     CHAR8      *Name,
  IN     BOOLEAN    IsDir
  )
{
  VIRTIO_FS_FUSE_REQUEST         CommonReq;
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
  ReqIoVec[1].Buffer = Name;
  ReqIoVec[1].Size   = AsciiStrSize (Name);
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
             IsDir ? VirtioFsFuseOpRmDir : VirtioFsFuseOpUnlink,
             ParentNodeId
             );
  if (EFI_ERROR (Status)) {
    return Status;
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
      "%a: Label=\"%s\" ParentNodeId=%Lu Name=\"%a\" "
      "IsDir=%d Errno=%d\n",
      __FUNCTION__,
      VirtioFs->Label,
      ParentNodeId,
      Name,
      IsDir,
      CommonResp.Error
      ));
    Status = VirtioFsErrnoToEfiStatus (CommonResp.Error);
  }

  return Status;
}
