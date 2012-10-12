/** @file

  Virtio Block Device specific type and macro definitions corresponding to the
  virtio-0.9.5 specification.

  Copyright (C) 2012, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VIRTIO_BLK_H_
#define _VIRTIO_BLK_H_

#include <IndustryStandard/Virtio.h>


//
// virtio-0.9.5, Appendix D: Block Device
//
#pragma pack(1)
typedef struct {
  VIRTIO_HDR Generic;
  UINT64     VhdrCapacity;
  UINT32     VhdrSizeMax;
  UINT32     VhdrSegMax;
  UINT16     VhdrCylinders;
  UINT8      VhdrHeads;
  UINT8      VhdrSectors;
  UINT32     VhdrBlkSize;
} VBLK_HDR;
#pragma pack()

#define OFFSET_OF_VBLK(Field) OFFSET_OF (VBLK_HDR, Field)
#define SIZE_OF_VBLK(Field)   (sizeof ((VBLK_HDR *) 0)->Field)

#define VIRTIO_BLK_F_BARRIER  BIT0
#define VIRTIO_BLK_F_SIZE_MAX BIT1
#define VIRTIO_BLK_F_SEG_MAX  BIT2
#define VIRTIO_BLK_F_GEOMETRY BIT4
#define VIRTIO_BLK_F_RO       BIT5
#define VIRTIO_BLK_F_BLK_SIZE BIT6 // treated as "logical block size" in
                                   // practice; actual host side implementation
                                   // negotiates "optimal" block size
                                   // separately
#define VIRTIO_BLK_F_SCSI     BIT7
#define VIRTIO_BLK_F_FLUSH    BIT9 // identical to "write cache enabled"

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
