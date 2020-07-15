/** @file

  Virtio SCSI Host Device specific type and macro definitions corresponding to
  the virtio-0.9.5 specification.

  Copyright (C) 2012, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTIO_SCSI_H_
#define _VIRTIO_SCSI_H_

#include <IndustryStandard/Virtio.h>


//
// virtio-0.9.5, Appendix I: SCSI Host Device
//
#pragma pack(1)
typedef struct {
  UINT32     NumQueues;
  UINT32     SegMax;
  UINT32     MaxSectors;
  UINT32     CmdPerLun;
  UINT32     EventInfoSize;
  UINT32     SenseSize;
  UINT32     CdbSize;
  UINT16     MaxChannel;
  UINT16     MaxTarget;
  UINT32     MaxLun;
} VIRTIO_SCSI_CONFIG;
#pragma pack()

#define OFFSET_OF_VSCSI(Field) OFFSET_OF (VIRTIO_SCSI_CONFIG, Field)
#define SIZE_OF_VSCSI(Field)   (sizeof ((VIRTIO_SCSI_CONFIG *) 0)->Field)

#define VIRTIO_SCSI_F_INOUT   BIT0
#define VIRTIO_SCSI_F_HOTPLUG BIT1

//
// We expect these maximum sizes from the host. Also we force the CdbLength and
// SenseDataLength parameters of EFI_EXT_SCSI_PASS_THRU_PROTOCOL.PassThru() not
// to exceed these limits. See UEFI 2.3.1 errata C 14.7.
//
#define VIRTIO_SCSI_CDB_SIZE   32
#define VIRTIO_SCSI_SENSE_SIZE 96

//
// We pass the dynamically sized buffers ("dataout", "datain") in separate ring
// descriptors.
//
#pragma pack(1)
typedef struct {
  UINT8  Lun[8];
  UINT64 Id;
  UINT8  TaskAttr;
  UINT8  Prio;
  UINT8  Crn;
  UINT8  Cdb[VIRTIO_SCSI_CDB_SIZE];
} VIRTIO_SCSI_REQ;

typedef struct {
  UINT32 SenseLen;
  UINT32 Residual;
  UINT16 StatusQualifier;
  UINT8  Status;
  UINT8  Response;
  UINT8  Sense[VIRTIO_SCSI_SENSE_SIZE];
} VIRTIO_SCSI_RESP;
#pragma pack()

//
// selector of first virtio queue usable for request transfer
//
#define VIRTIO_SCSI_REQUEST_QUEUE 2

//
// host response codes
//
#define VIRTIO_SCSI_S_OK                0
#define VIRTIO_SCSI_S_OVERRUN           1
#define VIRTIO_SCSI_S_ABORTED           2
#define VIRTIO_SCSI_S_BAD_TARGET        3
#define VIRTIO_SCSI_S_RESET             4
#define VIRTIO_SCSI_S_BUSY              5
#define VIRTIO_SCSI_S_TRANSPORT_FAILURE 6
#define VIRTIO_SCSI_S_TARGET_FAILURE    7
#define VIRTIO_SCSI_S_NEXUS_FAILURE     8
#define VIRTIO_SCSI_S_FAILURE           9

#endif // _VIRTIO_SCSI_H_
