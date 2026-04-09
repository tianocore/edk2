/** @file
  BlockFront functions and types declarations.

  Copyright (C) 2014, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "XenPvBlkDxe.h"

#include <IndustryStandard/Xen/event_channel.h>
#include <IndustryStandard/Xen/io/blkif.h>

typedef struct _XEN_BLOCK_FRONT_DEVICE  XEN_BLOCK_FRONT_DEVICE;
typedef struct _XEN_BLOCK_FRONT_IO      XEN_BLOCK_FRONT_IO;

struct _XEN_BLOCK_FRONT_IO {
  XEN_BLOCK_FRONT_DEVICE    *Dev;
  UINT8                     *Buffer;
  UINTN                     Size;
  UINTN                     Sector; ///< 512 bytes sector.

  grant_ref_t               GrantRef[BLKIF_MAX_SEGMENTS_PER_REQUEST];
  INT32                     NumRef;

  EFI_STATUS                Status;
};

typedef struct {
  UINT64     Sectors;
  UINT32     SectorSize;
  UINT32     VDiskInfo;
  BOOLEAN    ReadWrite;
  BOOLEAN    CdRom;
  BOOLEAN    FeatureBarrier;
  BOOLEAN    FeatureFlushCache;
} XEN_BLOCK_FRONT_MEDIA_INFO;

#define XEN_BLOCK_FRONT_SIGNATURE  SIGNATURE_32 ('X', 'p', 'v', 'B')
struct _XEN_BLOCK_FRONT_DEVICE {
  UINT32                        Signature;
  EFI_BLOCK_IO_PROTOCOL         BlockIo;
  domid_t                       DomainId;

  blkif_front_ring_t            Ring;
  grant_ref_t                   RingRef;
  evtchn_port_t                 EventChannel;
  blkif_vdev_t                  DeviceId;

  CONST CHAR8                   *NodeName;
  XEN_BLOCK_FRONT_MEDIA_INFO    MediaInfo;

  VOID                          *StateWatchToken;

  XENBUS_PROTOCOL               *XenBusIo;
};

#define XEN_BLOCK_FRONT_FROM_BLOCK_IO(b) \
  CR (b, XEN_BLOCK_FRONT_DEVICE, BlockIo, XEN_BLOCK_FRONT_SIGNATURE)

EFI_STATUS
XenPvBlockFrontInitialization (
  IN  XENBUS_PROTOCOL         *XenBusIo,
  IN  CONST CHAR8             *NodeName,
  OUT XEN_BLOCK_FRONT_DEVICE  **DevPtr
  );

VOID
XenPvBlockFrontShutdown (
  IN XEN_BLOCK_FRONT_DEVICE  *Dev
  );

VOID
XenPvBlockAsyncIo (
  IN OUT XEN_BLOCK_FRONT_IO  *IoData,
  IN     BOOLEAN             IsWrite
  );

EFI_STATUS
XenPvBlockIo (
  IN OUT XEN_BLOCK_FRONT_IO  *IoData,
  IN     BOOLEAN             IsWrite
  );

VOID
XenPvBlockAsyncIoPoll (
  IN XEN_BLOCK_FRONT_DEVICE  *Dev
  );

VOID
XenPvBlockSync (
  IN XEN_BLOCK_FRONT_DEVICE  *Dev
  );
