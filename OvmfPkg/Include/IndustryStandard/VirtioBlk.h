/** @file

  Virtio Block Device specific type and macro definitions corresponding to the
  virtio-0.9.5 specification.

  Copyright (C) 2012, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTIO_BLK_H_
#define _VIRTIO_BLK_H_

#include <IndustryStandard/Virtio.h>


//
// virtio-0.9.5, Appendix D: Block Device
//
#pragma pack(1)
typedef struct {
  UINT8  PhysicalBlockExp; // # of logical blocks per physical block (log2)
  UINT8  AlignmentOffset;  // offset of first aligned logical block
  UINT16 MinIoSize;        // suggested minimum I/O size in blocks
  UINT32 OptIoSize;        // optimal (suggested maximum) I/O size in blocks
} VIRTIO_BLK_TOPOLOGY;

typedef struct {
  UINT64              Capacity;
  UINT32              SizeMax;
  UINT32              SegMax;
  UINT16              Cylinders;
  UINT8               Heads;
  UINT8               Sectors;
  UINT32              BlkSize;
  VIRTIO_BLK_TOPOLOGY Topology;
} VIRTIO_BLK_CONFIG;
#pragma pack()

#define OFFSET_OF_VBLK(Field) OFFSET_OF (VIRTIO_BLK_CONFIG, Field)
#define SIZE_OF_VBLK(Field)   (sizeof ((VIRTIO_BLK_CONFIG *) 0)->Field)

#define VIRTIO_BLK_F_BARRIER  BIT0
#define VIRTIO_BLK_F_SIZE_MAX BIT1
#define VIRTIO_BLK_F_SEG_MAX  BIT2
#define VIRTIO_BLK_F_GEOMETRY BIT4
#define VIRTIO_BLK_F_RO       BIT5
#define VIRTIO_BLK_F_BLK_SIZE BIT6  // treated as "logical block size" in
                                    // practice; actual host side
                                    // implementation negotiates "optimal"
                                    // block size separately, via
                                    // VIRTIO_BLK_F_TOPOLOGY
#define VIRTIO_BLK_F_SCSI     BIT7
#define VIRTIO_BLK_F_FLUSH    BIT9  // identical to "write cache enabled"
#define VIRTIO_BLK_F_TOPOLOGY BIT10 // information on optimal I/O alignment

//
// We keep the status byte separate from the rest of the virtio-blk request
// header. See description of historical scattering at the end of Appendix D:
// we're going to put the status byte in a separate VRING_DESC.
//
#pragma pack(1)
typedef struct {
  UINT32 Type;
  UINT32 IoPrio;
  UINT64 Sector;
} VIRTIO_BLK_REQ;
#pragma pack()

#define VIRTIO_BLK_T_IN           0x00000000
#define VIRTIO_BLK_T_OUT          0x00000001
#define VIRTIO_BLK_T_SCSI_CMD     0x00000002
#define VIRTIO_BLK_T_SCSI_CMD_OUT 0x00000003
#define VIRTIO_BLK_T_FLUSH        0x00000004
#define VIRTIO_BLK_T_FLUSH_OUT    0x00000005
#define VIRTIO_BLK_T_BARRIER      BIT31

#define VIRTIO_BLK_S_OK           0x00
#define VIRTIO_BLK_S_IOERR        0x01
#define VIRTIO_BLK_S_UNSUPP       0x02

#endif // _VIRTIO_BLK_H_
