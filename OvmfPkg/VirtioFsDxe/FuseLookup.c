/** @file
  FUSE_LOOKUP wrapper for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h> // AsciiStrSize()

#include "VirtioFsDxe.h"

/**
  Send a FUSE_LOOKUP request to the Virtio Filesystem device, for resolving a
  filename to an inode.

  The function returns EFI_NOT_FOUND exclusively if the Virtio Filesystem
  device explicitly responds with ENOENT -- "No such file or directory".

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs  The Virtio Filesystem device to send the FUSE_LOOKUP
                           request to. On output, the FUSE request counter
                           "VirtioFs->RequestId" will have been incremented.

  @param[in] DirNodeId     The inode number of the directory in which Name
                           should be resolved to an inode.

  @param[in] Name          The single-component filename to resolve in the
                           directory identified by DirNodeId.

  @param[out] NodeId       The inode number which Name has been resolved to.

  @param[out] FuseAttr     The VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE object
                           describing the properties of the resolved inode.

  @retval EFI_SUCCESS    Filename to inode resolution successful.

  @retval EFI_NOT_FOUND  The Virtio Filesystem device explicitly reported
                         ENOENT -- "No such file or directory".

  @return                The "errno" value mapped to an EFI_STATUS code, if the
                         Virtio Filesystem device explicitly reported an error
                         different from ENOENT. If said mapping resulted in
                         EFI_NOT_FOUND, it is remapped to EFI_DEVICE_ERROR.

  @return                Error codes propagated from VirtioFsSgListsValidate(),
                         VirtioFsFuseNewRequest(), VirtioFsSgListsSubmit(),
                         VirtioFsFuseCheckResponse(). EFI_NOT_FOUND is remapped
                         to EFI_DEVICE_ERROR.
**/
EFI_STATUS
VirtioFsFuseLookup (
  IN OUT VIRTIO_FS                        *VirtioFs,
  IN     UINT64                           DirNodeId,
  IN     CHAR8                            *Name,
  OUT UINT64                              *NodeId,
  OUT VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE  *FuseAttr
  )
{
  VIRTIO_FS_FUSE_REQUEST         CommonReq;
  VIRTIO_FS_IO_VECTOR            ReqIoVec[2];
  VIRTIO_FS_SCATTER_GATHER_LIST  ReqSgList;
  VIRTIO_FS_FUSE_RESPONSE        CommonResp;
  VIRTIO_FS_FUSE_NODE_RESPONSE   NodeResp;
  VIRTIO_FS_IO_VECTOR            RespIoVec[3];
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
  RespIoVec[1].Buffer = &NodeResp;
  RespIoVec[1].Size   = sizeof NodeResp;
  RespIoVec[2].Buffer = FuseAttr;
  RespIoVec[2].Size   = sizeof *FuseAttr;
  RespSgList.IoVec    = RespIoVec;
  RespSgList.NumVec   = ARRAY_SIZE (RespIoVec);

  //
  // Validate the scatter-gather lists; calculate the total transfer sizes.
  //
  Status = VirtioFsSgListsValidate (VirtioFs, &ReqSgList, &RespSgList);
  if (EFI_ERROR (Status)) {
    goto Fail;
  }

  //
  // Populate the common request header.
  //
  Status = VirtioFsFuseNewRequest (
             VirtioFs,
             &CommonReq,
             ReqSgList.TotalSize,
             VirtioFsFuseOpLookup,
             DirNodeId
             );
  if (EFI_ERROR (Status)) {
    goto Fail;
  }

  //
  // Submit the request.
  //
  Status = VirtioFsSgListsSubmit (VirtioFs, &ReqSgList, &RespSgList);
  if (EFI_ERROR (Status)) {
    goto Fail;
  }

  //
  // Verify the response (all response buffers are fixed size).
  //
  Status = VirtioFsFuseCheckResponse (&RespSgList, CommonReq.Unique, NULL);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_DEVICE_ERROR) {
      DEBUG ((
        ((CommonResp.Error == VIRTIO_FS_FUSE_ERRNO_ENOENT) ?
         DEBUG_VERBOSE :
         DEBUG_ERROR),
        "%a: Label=\"%s\" DirNodeId=%Lu Name=\"%a\" Errno=%d\n",
        __func__,
        VirtioFs->Label,
        DirNodeId,
        Name,
        CommonResp.Error
        ));
      if (CommonResp.Error == VIRTIO_FS_FUSE_ERRNO_ENOENT) {
        return EFI_NOT_FOUND;
      }

      Status = VirtioFsErrnoToEfiStatus (CommonResp.Error);
    }

    goto Fail;
  }

  //
  // Output the NodeId to which Name has been resolved to.
  //
  *NodeId = NodeResp.NodeId;
  return EFI_SUCCESS;

Fail:
  return (Status == EFI_NOT_FOUND) ? EFI_DEVICE_ERROR : Status;
}
