/** @file

  VMware PVSCSI Device specific type and macro definitions.

  Copyright (C) 2020, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PVSCSI_H_
#define __PVSCSI_H_

//
// Device offsets and constants
//

#define PCI_VENDOR_ID_VMWARE            (0x15ad)
#define PCI_DEVICE_ID_VMWARE_PVSCSI     (0x07c0)

//
// CDB (Command Descriptor Block) with size above this constant
// should be considered out-of-band
//
#define PVSCSI_CDB_MAX_SIZE         (16)

typedef enum {
  PvScsiRegOffsetCommand           =    0x0,
  PvScsiRegOffsetCommandData       =    0x4,
  PvScsiRegOffsetCommandStatus     =    0x8,
  PvScsiRegOffsetLastSts0          =  0x100,
  PvScsiRegOffsetLastSts1          =  0x104,
  PvScsiRegOffsetLastSts2          =  0x108,
  PvScsiRegOffsetLastSts3          =  0x10c,
  PvScsiRegOffsetIntrStatus        = 0x100c,
  PvScsiRegOffsetIntrMask          = 0x2010,
  PvScsiRegOffsetKickNonRwIo       = 0x3014,
  PvScsiRegOffsetDebug             = 0x3018,
  PvScsiRegOffsetKickRwIo          = 0x4018,
} PVSCSI_BAR0_OFFSETS;

//
// Define Interrupt-Status register flags
//
#define PVSCSI_INTR_CMPL_0      BIT0
#define PVSCSI_INTR_CMPL_1      BIT1
#define PVSCSI_INTR_CMPL_MASK   (PVSCSI_INTR_CMPL_0 | PVSCSI_INTR_CMPL_1)

typedef enum {
  PvScsiCmdFirst               = 0,
  PvScsiCmdAdapterReset        = 1,
  PvScsiCmdIssueScsi           = 2,
  PvScsiCmdSetupRings          = 3,
  PvScsiCmdResetBus            = 4,
  PvScsiCmdResetDevice         = 5,
  PvScsiCmdAbortCmd            = 6,
  PvScsiCmdConfig              = 7,
  PvScsiCmdSetupMsgRing        = 8,
  PvScsiCmdDeviceUnplug        = 9,
  PvScsiCmdLast                = 10
} PVSCSI_COMMANDS;

#define PVSCSI_SETUP_RINGS_MAX_NUM_PAGES    (32)

#pragma pack (1)
typedef struct {
  UINT32 ReqRingNumPages;
  UINT32 CmpRingNumPages;
  UINT64 RingsStatePPN;
  UINT64 ReqRingPPNs[PVSCSI_SETUP_RINGS_MAX_NUM_PAGES];
  UINT64 CmpRingPPNs[PVSCSI_SETUP_RINGS_MAX_NUM_PAGES];
} PVSCSI_CMD_DESC_SETUP_RINGS;
#pragma pack ()

#define PVSCSI_MAX_CMD_DATA_WORDS   \
  (sizeof (PVSCSI_CMD_DESC_SETUP_RINGS) / sizeof (UINT32))

#pragma pack (1)
typedef struct {
  UINT32 ReqProdIdx;
  UINT32 ReqConsIdx;
  UINT32 ReqNumEntriesLog2;

  UINT32 CmpProdIdx;
  UINT32 CmpConsIdx;
  UINT32 CmpNumEntriesLog2;

  UINT8  Pad[104];

  UINT32 MsgProdIdx;
  UINT32 MsgConsIdx;
  UINT32 MsgNumEntriesLog2;
} PVSCSI_RINGS_STATE;
#pragma pack ()

//
// Define PVSCSI request descriptor tags
//
#define PVSCSI_SIMPLE_QUEUE_TAG            (0x20)

//
// Define PVSCSI request descriptor flags
//
#define PVSCSI_FLAG_CMD_WITH_SG_LIST       BIT0
#define PVSCSI_FLAG_CMD_OUT_OF_BAND_CDB    BIT1
#define PVSCSI_FLAG_CMD_DIR_NONE           BIT2
#define PVSCSI_FLAG_CMD_DIR_TOHOST         BIT3
#define PVSCSI_FLAG_CMD_DIR_TODEVICE       BIT4

#pragma pack (1)
typedef struct {
  UINT64 Context;
  UINT64 DataAddr;
  UINT64 DataLen;
  UINT64 SenseAddr;
  UINT32 SenseLen;
  UINT32 Flags;
  UINT8  Cdb[16];
  UINT8  CdbLen;
  UINT8  Lun[8];
  UINT8  Tag;
  UINT8  Bus;
  UINT8  Target;
  UINT8  VcpuHint;
  UINT8  Unused[59];
} PVSCSI_RING_REQ_DESC;
#pragma pack ()

//
// Host adapter status/error codes
//
typedef enum {
  PvScsiBtStatSuccess       = 0x00,  // CCB complete normally with no errors
  PvScsiBtStatLinkedCommandCompleted         = 0x0a,
  PvScsiBtStatLinkedCommandCompletedWithFlag = 0x0b,
  PvScsiBtStatDataUnderrun  = 0x0c,
  PvScsiBtStatSelTimeout    = 0x11,  // SCSI selection timeout
  PvScsiBtStatDatarun       = 0x12,  // Data overrun/underrun
  PvScsiBtStatBusFree       = 0x13,  // Unexpected bus free
  PvScsiBtStatInvPhase      = 0x14,  //
                                     // Invalid bus phase or sequence requested
                                     // by target
                                     //
  PvScsiBtStatLunMismatch   = 0x17,  //
                                     // Linked CCB has different LUN from first
                                     // CCB
                                     //
  PvScsiBtStatSensFailed    = 0x1b,  // Auto request sense failed
  PvScsiBtStatTagReject     = 0x1c,  //
                                     // SCSI II tagged queueing message rejected
                                     // by target
                                     //
  PvScsiBtStatBadMsg        = 0x1d,  //
                                     // Unsupported message received by the host
                                     // adapter
                                     //
  PvScsiBtStatHaHardware    = 0x20,  // Host adapter hardware failed
  PvScsiBtStatNoResponse    = 0x21,  //
                                     // Target did not respond to SCSI ATN sent
                                     // a SCSI RST
                                     //
  PvScsiBtStatSentRst       = 0x22,  // Host adapter asserted a SCSI RST
  PvScsiBtStatRecvRst       = 0x23,  // Other SCSI devices asserted a SCSI RST
  PvScsiBtStatDisconnect    = 0x24,  //
                                     // Target device reconnected improperly
                                     // (w/o tag)
                                     //
  PvScsiBtStatBusReset      = 0x25,  // Host adapter issued BUS device reset
  PvScsiBtStatAbortQueue    = 0x26,  // Abort queue generated
  PvScsiBtStatHaSoftware    = 0x27,  // Host adapter software error
  PvScsiBtStatHaTimeout     = 0x30,  // Host adapter hardware timeout error
  PvScsiBtStatScsiParity    = 0x34,  // SCSI parity error detected
} PVSCSI_HOST_BUS_ADAPTER_STATUS;

#pragma pack (1)
typedef struct {
  UINT64 Context;
  UINT64 DataLen;
  UINT32 SenseLen;
  UINT16 HostStatus;
  UINT16 ScsiStatus;
  UINT32 Pad[2];
} PVSCSI_RING_CMP_DESC;
#pragma pack ()

#endif // __PVSCSI_H_
