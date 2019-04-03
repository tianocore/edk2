/** @file

  Virtio GPU Device specific type and macro definitions.

  At the time of this writing, the Virtio 1.0 specification has not
  incorporated the GPU device yet. The following work-in-progress specification
  is used as basis for the implementation:

  - https://lists.oasis-open.org/archives/virtio-dev/201605/msg00002.html
  - https://www.kraxel.org/virtio/

  This header file is minimal, and only defines the types and macros that are
  necessary for the OvmfPkg implementation.

  Copyright (C) 2016, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTIO_GPU_H_
#define _VIRTIO_GPU_H_

#include <IndustryStandard/Virtio.h>

//
// Queue number for sending control commands.
//
#define VIRTIO_GPU_CONTROL_QUEUE 0

//
// Command and response types.
//
typedef enum {
  //
  // Commands related to mode setup:
  //
  // - create/release a host-side 2D resource,
  //
  VirtioGpuCmdResourceCreate2d      = 0x0101,
  VirtioGpuCmdResourceUnref         = 0x0102,
  //
  // - attach/detach guest RAM to/from a host-side 2D resource,
  //
  VirtioGpuCmdResourceAttachBacking = 0x0106,
  VirtioGpuCmdResourceDetachBacking = 0x0107,
  //
  // - assign/unassign a host-side 2D resource to/from a scanout ("head").
  //
  VirtioGpuCmdSetScanout            = 0x0103,

  //
  // Commands related to drawing:
  //
  // - transfer a guest RAM update to the host-side 2D resource (does not imply
  //   host display refresh),
  //
  VirtioGpuCmdTransferToHost2d      = 0x0105,
  //
  // - trigger a host display refresh from the 2D resource.
  //
  VirtioGpuCmdResourceFlush         = 0x0104,

  //
  // Success code for all of the above commands.
  //
  VirtioGpuRespOkNodata             = 0x1100,
} VIRTIO_GPU_CONTROL_TYPE;

//
// Common request/response header.
//
#define VIRTIO_GPU_FLAG_FENCE BIT0

#pragma pack (1)
typedef struct {
  //
  // The guest sets Type to VirtioGpuCmd* in the requests. The host sets Type
  // to VirtioGpuResp* in the responses.
  //
  UINT32 Type;

  //
  // Fencing forces the host to complete the command before producing a
  // response.
  //
  UINT32 Flags;
  UINT64 FenceId;

  //
  // Unused.
  //
  UINT32 CtxId;
  UINT32 Padding;
} VIRTIO_GPU_CONTROL_HEADER;
#pragma pack ()

//
// Rectangle structure used by several operations.
//
#pragma pack (1)
typedef struct {
  UINT32 X;
  UINT32 Y;
  UINT32 Width;
  UINT32 Height;
} VIRTIO_GPU_RECTANGLE;
#pragma pack ()

//
// Request structure for VirtioGpuCmdResourceCreate2d.
//
typedef enum {
  //
  // 32-bit depth, BGRX component order, X component ignored.
  //
  VirtioGpuFormatB8G8R8X8Unorm = 2,
} VIRTIO_GPU_FORMATS;

#pragma pack (1)
typedef struct {
  VIRTIO_GPU_CONTROL_HEADER Header;
  UINT32                    ResourceId; // note: 0 is invalid
  UINT32                    Format;     // from VIRTIO_GPU_FORMATS
  UINT32                    Width;
  UINT32                    Height;
} VIRTIO_GPU_RESOURCE_CREATE_2D;
#pragma pack ()

//
// Request structure for VirtioGpuCmdResourceUnref.
//
#pragma pack (1)
typedef struct {
  VIRTIO_GPU_CONTROL_HEADER Header;
  UINT32                    ResourceId;
  UINT32                    Padding;
} VIRTIO_GPU_RESOURCE_UNREF;
#pragma pack ()

//
// Request structure for VirtioGpuCmdResourceAttachBacking.
//
// The spec allows for a scatter-gather list, but for simplicity we hard-code a
// single guest buffer.
//
#pragma pack (1)
typedef struct {
  UINT64 Addr;
  UINT32 Length;
  UINT32 Padding;
} VIRTIO_GPU_MEM_ENTRY;

typedef struct {
  VIRTIO_GPU_CONTROL_HEADER Header;
  UINT32                    ResourceId;
  UINT32                    NrEntries;  // number of entries: constant 1
  VIRTIO_GPU_MEM_ENTRY      Entry;
} VIRTIO_GPU_RESOURCE_ATTACH_BACKING;
#pragma pack ()

//
// Request structure for VirtioGpuCmdResourceDetachBacking.
//
#pragma pack (1)
typedef struct {
  VIRTIO_GPU_CONTROL_HEADER Header;
  UINT32                    ResourceId;
  UINT32                    Padding;
} VIRTIO_GPU_RESOURCE_DETACH_BACKING;
#pragma pack ()

//
// Request structure for VirtioGpuCmdSetScanout.
//
#pragma pack (1)
typedef struct {
  VIRTIO_GPU_CONTROL_HEADER Header;
  VIRTIO_GPU_RECTANGLE      Rectangle;
  UINT32                    ScanoutId;
  UINT32                    ResourceId;
} VIRTIO_GPU_SET_SCANOUT;
#pragma pack ()

//
// Request structure for VirtioGpuCmdTransferToHost2d.
//
#pragma pack (1)
typedef struct {
  VIRTIO_GPU_CONTROL_HEADER Header;
  VIRTIO_GPU_RECTANGLE      Rectangle;
  UINT64                    Offset;
  UINT32                    ResourceId;
  UINT32                    Padding;
}  VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
#pragma pack ()

//
// Request structure for VirtioGpuCmdResourceFlush.
//
#pragma pack (1)
typedef struct {
  VIRTIO_GPU_CONTROL_HEADER Header;
  VIRTIO_GPU_RECTANGLE      Rectangle;
  UINT32                    ResourceId;
  UINT32                    Padding;
} VIRTIO_GPU_RESOURCE_FLUSH;
#pragma pack ()

#endif // _VIRTIO_GPU_H_
