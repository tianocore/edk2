/** @file

  Macros and type definitions for LSI Fusion MPT SCSI devices.

  Copyright (C) 2020, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FUSION_MPT_SCSI_H__
#define __FUSION_MPT_SCSI_H__

//
// Device offsets and constants
//

#define LSI_LOGIC_PCI_VENDOR_ID     0x1000
#define LSI_53C1030_PCI_DEVICE_ID   0x0030
#define LSI_SAS1068_PCI_DEVICE_ID   0x0054
#define LSI_SAS1068E_PCI_DEVICE_ID  0x0058

#define MPT_REG_DOORBELL   0x00
#define MPT_REG_WRITE_SEQ  0x04
#define MPT_REG_HOST_DIAG  0x08
#define MPT_REG_TEST       0x0c
#define MPT_REG_DIAG_DATA  0x10
#define MPT_REG_DIAG_ADDR  0x14
#define MPT_REG_ISTATUS    0x30
#define MPT_REG_IMASK      0x34
#define MPT_REG_REQ_Q      0x40
#define MPT_REG_REP_Q      0x44

#define MPT_DOORBELL_RESET      0x40
#define MPT_DOORBELL_HANDSHAKE  0x42

#define MPT_IMASK_DOORBELL  0x01
#define MPT_IMASK_REPLY     0x08

#define MPT_MESSAGE_HDR_FUNCTION_SCSI_IO_REQUEST  0x00
#define MPT_MESSAGE_HDR_FUNCTION_IOC_INIT         0x02

#define MPT_SG_ENTRY_TYPE_SIMPLE  0x01

#define MPT_IOC_WHOINIT_ROM_BIOS  0x02

#define MPT_SCSIIO_REQUEST_CONTROL_TXDIR_NONE   (0x00 << 24)
#define MPT_SCSIIO_REQUEST_CONTROL_TXDIR_WRITE  (0x01 << 24)
#define MPT_SCSIIO_REQUEST_CONTROL_TXDIR_READ   (0x02 << 24)

#define MPT_SCSI_IOCSTATUS_SUCCESS           0x0000
#define MPT_SCSI_IOCSTATUS_DEVICE_NOT_THERE  0x0043
#define MPT_SCSI_IOCSTATUS_DATA_OVERRUN      0x0044
#define MPT_SCSI_IOCSTATUS_DATA_UNDERRUN     0x0045

//
// Device structures
//

#pragma pack (1)
typedef struct {
  UINT8     WhoInit;
  UINT8     Reserved1;
  UINT8     ChainOffset;
  UINT8     Function;
  UINT8     Flags;
  UINT8     MaxDevices;
  UINT8     MaxBuses;
  UINT8     MessageFlags;
  UINT32    MessageContext;
  UINT16    ReplyFrameSize;
  UINT16    Reserved2;
  UINT32    HostMfaHighAddr;
  UINT32    SenseBufferHighAddr;
} MPT_IO_CONTROLLER_INIT_REQUEST;

typedef struct {
  UINT8     WhoInit;
  UINT8     Reserved1;
  UINT8     MessageLength;
  UINT8     Function;
  UINT8     Flags;
  UINT8     MaxDevices;
  UINT8     MaxBuses;
  UINT8     MessageFlags;
  UINT32    MessageContext;
  UINT16    Reserved2;
  UINT16    IocStatus;
  UINT32    IocLogInfo;
} MPT_IO_CONTROLLER_INIT_REPLY;

typedef struct {
  UINT8     TargetId;
  UINT8     Bus;
  UINT8     ChainOffset;
  UINT8     Function;
  UINT8     CdbLength;
  UINT8     SenseBufferLength;
  UINT8     Reserved;
  UINT8     MessageFlags;
  UINT32    MessageContext;
  UINT8     Lun[8];
  UINT32    Control;
  UINT8     Cdb[16];
  UINT32    DataLength;
  UINT32    SenseBufferLowAddress;
} MPT_SCSI_IO_REQUEST;

typedef struct {
  UINT32    Length             :             24;
  UINT32    EndOfList          :          1;
  UINT32    Is64BitAddress     :     1;
  //
  // True when the buffer contains data to be transfered. Otherwise it's the
  // destination buffer
  //
  UINT32    BufferContainsData : 1;
  UINT32    LocalAddress       :       1;
  UINT32    ElementType        :        2;
  UINT32    EndOfBuffer        :        1;
  UINT32    LastElement        :        1;
  UINT64    DataBufferAddress;
} MPT_SG_ENTRY_SIMPLE;

typedef struct {
  UINT8     TargetId;
  UINT8     Bus;
  UINT8     MessageLength;
  UINT8     Function;
  UINT8     CdbLength;
  UINT8     SenseBufferLength;
  UINT8     Reserved;
  UINT8     MessageFlags;
  UINT32    MessageContext;
  UINT8     ScsiStatus;
  UINT8     ScsiState;
  UINT16    IocStatus;
  UINT32    IocLogInfo;
  UINT32    TransferCount;
  UINT32    SenseCount;
  UINT32    ResponseInfo;
} MPT_SCSI_IO_REPLY;

typedef struct {
  MPT_SCSI_IO_REQUEST    Header;
  MPT_SG_ENTRY_SIMPLE    Sg;
} MPT_SCSI_REQUEST_WITH_SG;
#pragma pack ()

typedef union {
  MPT_SCSI_IO_REPLY    Data;
  UINT64               Uint64;     // 8 byte alignment required by HW
} MPT_SCSI_IO_REPLY_ALIGNED;

typedef union {
  MPT_SCSI_REQUEST_WITH_SG    Data;
  UINT64                      Uint64; // 8 byte alignment required by HW
} MPT_SCSI_REQUEST_ALIGNED;

#endif // __FUSION_MPT_SCSI_H__
