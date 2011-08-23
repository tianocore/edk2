/** @file

  This file contains the definition for XHCI host controller schedule routines.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_XHCI_SCHED_H_
#define _EFI_XHCI_SCHED_H_

#define XHC_URB_SIG      SIGNATURE_32 ('U', 'S', 'B', 'R')

//
// Transfer types, used in URB to identify the transfer type
//
#define XHC_CTRL_TRANSFER                     0x01
#define XHC_BULK_TRANSFER                     0x02
#define XHC_INT_TRANSFER_SYNC                 0x04
#define XHC_INT_TRANSFER_ASYNC                0x08
#define XHC_INT_ONLY_TRANSFER_ASYNC           0x10

//
// 6.4.6 TRB Types
//
#define TRB_TYPE_NORMAL                       1
#define TRB_TYPE_SETUP_STAGE                  2
#define TRB_TYPE_DATA_STAGE                   3
#define TRB_TYPE_STATUS_STAGE                 4
#define TRB_TYPE_ISOCH                        5
#define TRB_TYPE_LINK                         6
#define TRB_TYPE_EVENT_DATA                   7
#define TRB_TYPE_NO_OP                        8
#define TRB_TYPE_EN_SLOT                      9
#define TRB_TYPE_DIS_SLOT                     10
#define TRB_TYPE_ADDRESS_DEV                  11
#define TRB_TYPE_CON_ENDPOINT                 12
#define TRB_TYPE_EVALU_CONTXT                 13
#define TRB_TYPE_RESET_ENDPOINT               14
#define TRB_TYPE_STOP_ENDPOINT                15
#define TRB_TYPE_SET_TR_DEQUE                 16
#define TRB_TYPE_RESET_DEV                    17
#define TRB_TYPE_GET_PORT_BANW                21
#define TRB_TYPE_FORCE_HEADER                 22
#define TRB_TYPE_NO_OP_COMMAND                23
#define TRB_TYPE_TRANS_EVENT                  32
#define TRB_TYPE_COMMAND_COMPLT_EVENT         33
#define TRB_TYPE_PORT_STATUS_CHANGE_EVENT     34
#define TRB_TYPE_HOST_CONTROLLER_EVENT        37
#define TRB_TYPE_DEVICE_NOTIFI_EVENT          38
#define TRB_TYPE_MFINDEX_WRAP_EVENT           39

//
// Endpoint Type (EP Type).
//
#define ED_NOT_VALID                          0
#define ED_ISOCH_OUT                          1
#define ED_BULK_OUT                           2
#define ED_INTERRUPT_OUT                      3
#define ED_CONTROL_BIDIR                      4
#define ED_ISOCH_IN                           5
#define ED_BULK_IN                            6
#define ED_INTERRUPT_IN                       7

//
// 6.4.5 TRB Completion Codes
//
#define TRB_COMPLETION_INVALID                0
#define TRB_COMPLETION_SUCCESS                1
#define TRB_COMPLETION_DATA_BUFFER_ERROR      2
#define TRB_COMPLETION_BABBLE_ERROR           3
#define TRB_COMPLETION_USB_TRANSACTION_ERROR  4
#define TRB_COMPLETION_TRB_ERROR              5
#define TRB_COMPLETION_STALL_ERROR            6
#define TRB_COMPLETION_SHORT_PACKET           13

//
// USB device RouteChart record
//
typedef union _USB_DEV_TOPOLOGY {
  UINT32 Dword;
  struct {
    UINT32 RouteString:20; ///< The tier concatenation of down stream port
    UINT32 RootPortNum:8;  ///< The root port number of the chain
    UINT32 TierNum:4;      ///< The Tier the device reside
  } Field;
} USB_DEV_ROUTE;

//
// Endpoint address and its capabilities
//
typedef struct _USB_ENDPOINT {
  UINT8                     DevAddr;
  UINT8                     EpAddr;
  EFI_USB_DATA_DIRECTION    Direction;
  UINT8                     DevSpeed;
  UINTN                     MaxPacket;
  UINTN                     Type;
} USB_ENDPOINT;

//
// Command TRB
//
typedef struct _TRB {
  UINT32                    Dword1;
  UINT32                    Dword2;
  UINT32                    Dword3;
  UINT32                    CycleBit:1;
  UINT32                    RsvdZ1:9;
  UINT32                    Type:6;
  UINT32                    RsvdZ2:16;
} TRB;

typedef struct _TRANSFER_RING {
  VOID                      *RingSeg0;
  UINTN                     TrbNumber;
  TRB                       *RingEnqueue;
  TRB                       *RingDequeue;
  UINT32                    RingPCS;
} TRANSFER_RING;

typedef struct _EVENT_RING {
  UINT32                    EventInterrupter;
  VOID                      *ERSTBase;
  VOID                      *EventRingSeg0;
  UINTN                     TrbNumber;
  TRB                       *EventRingEnqueue;
  TRB                       *EventRingDequeue;
  UINT32                    EventRingCCS;
} EVENT_RING;

//
// URB (Usb Request Block) contains information for all kinds of
// usb requests.
//
typedef struct _URB {
  UINT32                          Signature;
  LIST_ENTRY                      UrbList;
  //
  // Usb Device URB related information
  //
  USB_ENDPOINT                    Ep;
  EFI_USB_DEVICE_REQUEST          *Request;
  VOID                            *Data;
  UINTN                           DataLen;
  EFI_ASYNC_USB_TRANSFER_CALLBACK Callback;
  VOID                            *Context;
  //
  // Execute result
  //
  UINT32                          Result;
  //
  // completed data length
  //
  UINTN                           Completed;
  //
  // Command/Tranfer Ring info
  //
  TRANSFER_RING                   *Ring;
  TRB                             *TrbStart;
  TRB                             *TrbEnd;
  UINTN                           TrbNum;
  EVENT_RING                      *EvtRing;
  TRB                             *EvtTrbStart;
} URB;

//
// 5.5.2 Interrupter Register Set
//
typedef struct _INTERRUPTER_REGISTER_SET {
  UINT32                    InterrupterManagement;
  UINT32                    InterrupterModeration;
  UINT32                    RingSegTableSize:16;
  UINT32                    RsvdZ1:16;
  UINT32                    RsvdZ2;
  UINT32                    BasePtrLo;
  UINT32                    BasePtrHi;
  UINT32                    DequeLo;
  UINT32                    DequeHi;
} INTERRUPTER_REGISTER_SET;

//
// Host Controller Runtime Registers
//
typedef struct _HC_RUNTIME_REGS {
  UINT32                    MicroframeIndex;
  UINT32                    RsvdZ1;
  UINT64                    RsvdZ2;
  UINT64                    RsvdZ3;
  UINT64                    RsvdZ4;
  INTERRUPTER_REGISTER_SET  IR[1];
} HC_RUNTIME_REGS;

//
// 6.5 Event Ring Segment Table
// The Event Ring Segment Table is used to define multi-segment Event Rings and to enable runtime
// expansion and shrinking of the Event Ring. The location of the Event Ring Segment Table is defined by the
// Event Ring Segment Table Base Address Register (5.5.2.3.2). The size of the Event Ring Segment Table
// is defined by the Event Ring Segment Table Base Size Register (5.5.2.3.1).
//
typedef struct _EVENT_RING_SEG_TABLE_ENTRY {
  UINT32                  PtrLo;
  UINT32                  PtrHi;
  UINT32                  RingTrbSize:16;
  UINT32                  RsvdZ1:16;
  UINT32                  RsvdZ2;
} EVENT_RING_SEG_TABLE_ENTRY;

//
// 6.4.1.1 Normal TRB
// A Normal TRB is used in several ways; exclusively on Bulk and Interrupt Transfer Rings for normal and
// Scatter/Gather operations, to define additional data buffers for Scatter/Gather operations on Isoch Transfer
// Rings, and to define the Data stage information for Control Transfer Rings.
//
typedef struct _TRANSFER_TRB_NORMAL {
  UINT32                  TRBPtrLo;
  UINT32                  TRBPtrHi;
  UINT32                  Lenth:17;
  UINT32                  TDSize:5;
  UINT32                  IntTarget:10;
  UINT32                  CycleBit:1;
  UINT32                  ENT:1;
  UINT32                  ISP:1;
  UINT32                  NS:1;
  UINT32                  CH:1;
  UINT32                  IOC:1;
  UINT32                  IDT:1;
  UINT32                  RsvdZ1:2;
  UINT32                  BEI:1;
  UINT32                  Type:6;
  UINT32                  RsvdZ2:16;
} TRANSFER_TRB_NORMAL;

//
// 6.4.1.2.1 Setup Stage TRB
// A Setup Stage TRB is created by system software to initiate a USB Setup packet on a control endpoint.
//
typedef struct _TRANSFER_TRB_CONTROL_SETUP{
  UINT32                  bmRequestType:8;
  UINT32                  bRequest:8;
  UINT32                  wValue:16;

  UINT32                  wIndex:16;
  UINT32                  wLength:16;

  UINT32                  Lenth:17;
  UINT32                  RsvdZ1:5;
  UINT32                  IntTarget:10;

  UINT32                  CycleBit:1;
  UINT32                  RsvdZ2:4;
  UINT32                  IOC:1;
  UINT32                  IDT:1;
  UINT32                  RsvdZ3:3;
  UINT32                  Type:6;
  UINT32                  TRT:2;
  UINT32                  RsvdZ4:14;
} TRANSFER_TRB_CONTROL_SETUP;

//
// 6.4.1.2.2 Data Stage TRB
// A Data Stage TRB is used generate the Data stage transaction of a USB Control transfer.
//
typedef struct _TRANSFER_TRB_CONTROL_DATA {
  UINT32                  TRBPtrLo;
  UINT32                  TRBPtrHi;
  UINT32                  Lenth:17;
  UINT32                  TDSize:5;
  UINT32                  IntTarget:10;
  UINT32                  CycleBit:1;
  UINT32                  ENT:1;
  UINT32                  ISP:1;
  UINT32                  NS:1;
  UINT32                  CH:1;
  UINT32                  IOC:1;
  UINT32                  IDT:1;
  UINT32                  RsvdZ1:3;
  UINT32                  Type:6;
  UINT32                  DIR:1;
  UINT32                  RsvdZ2:15;
} TRANSFER_TRB_CONTROL_DATA;

//
// 6.4.1.2.2 Data Stage TRB
// A Data Stage TRB is used generate the Data stage transaction of a USB Control transfer.
//
typedef struct _TRANSFER_TRB_CONTROL_STATUS {
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  RsvdZ3:22;
  UINT32                  IntTarget:10;
  UINT32                  CycleBit:1;
  UINT32                  ENT:1;
  UINT32                  RsvdZ4:2;
  UINT32                  CH:1;
  UINT32                  IOC:1;
  UINT32                  RsvdZ5:4;
  UINT32                  Type:6;
  UINT32                  DIR:1;
  UINT32                  RsvdZ6:15;
} TRANSFER_TRB_CONTROL_STATUS;

//
// 6.4.2.1 Transfer Event TRB
// A Transfer Event provides the completion status associated with a Transfer TRB. Refer to section 4.11.3.1
// for more information on the use and operation of Transfer Events.
//
typedef struct _EVT_TRB_TRANSFER {
  UINT32                  TRBPtrLo;
  UINT32                  TRBPtrHi;
  UINT32                  Lenth:24;
  UINT32                  Completcode:8;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ1:1;
  UINT32                  ED:1;
  UINT32                  RsvdZ2:7;
  UINT32                  Type:6;
  UINT32                  EndpointID:5;
  UINT32                  RsvdZ3:3;
  UINT32                  SlotId:8;
} EVT_TRB_TRANSFER;

//
// 6.4.2.2 Command Completion Event TRB
// A Command Completion Event TRB shall be generated by the xHC when a command completes on the
// Command Ring. Refer to section 4.11.4 for more information on the use of Command Completion Events.
//
typedef struct _EVT_TRB_COMMAND {
  UINT32                  TRBPtrLo;
  UINT32                  TRBPtrHi;
  UINT32                  RsvdZ2:24;
  UINT32                  Completcode:8;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:9;
  UINT32                  Type:6;
  UINT32                  VFID:8;
  UINT32                  SlotId:8;
} EVT_TRB_COMMAND;

//
// 6.4.2.3 Port Status Change Event TRB
//
typedef struct _EVT_TRB_PORT {
  UINT32                  RsvdZ1:24;
  UINT32                  PortID:8;
  UINT32                  RsvdZ2;
  UINT32                  RsvdZ3:24;
  UINT32                  Completcode:8;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ4:9;
  UINT32                  Type:6;
  UINT32                  RsvdZ5:16;
} EVT_TRB_PORT;

//
// 6.4.3.1 No Op Command TRB
// The No Op Command TRB provides a simple means for verifying the operation of the Command Ring
//  mechanisms offered by the xHCI.
//
typedef struct _CMD_TRB_NO_OP {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:9;
  UINT32                  Type:6;
  UINT32                  RsvdZ4:16;
} CMD_TRB_NO_OP;

//
// 6.4.3.2 Enable Slot Command TRB
// The Enable Slot Command TRB causes the xHC to select an available Device Slot and return the ID of the
// selected slot to the host in a Command Completion Event.
//
typedef struct _CMD_TRB_EN_SLOT {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:9;
  UINT32                  Type:6;
  UINT32                  RsvdZ4:16;
} CMD_TRB_EN_SLOT;

//
// 6.4.3.3 Disable Slot Command TRB
// The Disable Slot Command TRB releases any bandwidth assigned to the disabled slot and frees any
// internal xHC resources assigned to the slot.
//
typedef struct _CMD_TRB_DIS_SLOT {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:9;
  UINT32                  Type:6;
  UINT32                  RsvdZ4:8;
  UINT32                  SlotId:8;
} CMD_TRB_DIS_SLOT;

typedef struct _CMD_TRB_RESET_PORT {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:8;
  UINT32                  Tsp:1;
  UINT32                  Type:6;
  UINT32                  Endpoint:5;
  UINT32                  RsvdZ4:3;
  UINT32                  SlotId:8;
} CMD_TRB_RESET_PORT;

//
// 6.4.3.4 Address Device Command TRB
// The Address Device Command TRB transitions the selected Device Context from the Default to the
// Addressed state and causes the xHC to select an address for the USB device in the Default State and
// issue a SET_ADDRESS request to the USB device.
//
typedef struct _CMD_TRB_ADDR_DEV {
  UINT32                  PtrLo;
  UINT32                  PtrHi;
  UINT32                  RsvdZ1;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ2:8;
  UINT32                  BSR:1;
  UINT32                  Type:6;
  UINT32                  RsvdZ3:8;
  UINT32                  SlotId:8;
} CMD_TRB_ADDR_DEV;

//
// 6.4.3.5 Configure Endpoint Command TRB
// The Configure Endpoint Command TRB evaluates the bandwidth and resource requirements of the
// endpoints selected by the command.
//
typedef struct _CMD_CFG_ED {
  UINT32                  PtrLo;
  UINT32                  PtrHi;
  UINT32                  RsvdZ1;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ2:8;
  UINT32                  DC:1;
  UINT32                  Type:6;
  UINT32                  RsvdZ3:8;
  UINT32                  SlotId:8;
} CMD_CFG_ED;

//
// 6.4.3.6 Evaluate Context Command TRB
// The Evaluate Context Command TRB is used by system software to inform the xHC that the selected
// Context data structures in the Device Context have been modified by system software and that the xHC
// shall evaluate any changes
//
typedef struct _CMD_TRB_EVALU_CONTX {
  UINT32                  PtrLo;
  UINT32                  PtrHi;
  UINT32                  RsvdZ1;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ2:9;
  UINT32                  Type:6;
  UINT32                  RsvdZ3:8;
  UINT32                  SlotId:8;
} CMD_TRB_EVALU_CONTX;

//
// 6.4.3.7 Reset Endpoint Command TRB
// The Reset Endpoint Command TRB is used by system software to reset a specified Transfer Ring
//
typedef struct _CMD_TRB_RESET_ED {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:8;
  UINT32                  TSP:1;
  UINT32                  Type:6;
  UINT32                  EDID:5;
  UINT32                  RsvdZ4:3;
  UINT32                  SlotId:8;
} CMD_TRB_RESET_ED;

//
// 6.4.3.8 Stop Endpoint Command TRB
// The Stop Endpoint Command TRB command allows software to stop the xHC execution of the TDs on a
// Transfer Ring and temporarily take ownership of TDs that had previously been passed to the xHC.
//
typedef struct _CMD_TRB_STOP_ED {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:9;
  UINT32                  Type:6;
  UINT32                  EDID:5;
  UINT32                  RsvdZ4:2;
  UINT32                  SP:1;
  UINT32                  SlotId:8;
} CMD_TRB_STOP_ED;

//
// 6.4.3.9 Set TR Dequeue Pointer Command TRB
// The Set TR Dequeue Pointer Command TRB is used by system software to modify the TR Dequeue
// Pointer and DCS fields of an Endpoint or Stream Context.
//
typedef struct _CMD_SET_TR_DEQ {
  UINT32                  PtrLo;
  UINT32                  PtrHi;
  UINT32                  RsvdZ1:16;
  UINT32                  StreamID:16;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ2:9;
  UINT32                  Type:6;
  UINT32                  Endpoint:5;
  UINT32                  RsvdZ3:3;
  UINT32                  SlotId:8;
} CMD_SET_TR_DEQ;

//
// A Link TRB provides support for non-contiguous TRB Rings.
//
typedef struct _LNK_TRB {
  UINT32                  PtrLo;
  UINT32                  PtrHi;
  UINT32                  RsvdZ1:22;
  UINT32                  InterTarget:10;
  UINT32                  CycleBit:1;
  UINT32                  TC:1;
  UINT32                  RsvdZ2:2;
  UINT32                  CH:1;
  UINT32                  IOC:1;
  UINT32                  RsvdZ3:4;
  UINT32                  Type:6;
  UINT32                  RsvdZ4:16;
} LNK_TRB;

//
// A Link TRB provides support for non-contiguous TRB Rings.
//
typedef struct _NO_OP_TRB {
  UINT32                  RsvdZ0;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  CycleBit:1;
  UINT32                  RsvdZ3:9;
  UINT32                  Type:6;
  UINT32                  RsvdZ4:16;
} CMD_NO_OP_TRB;

//
// 6.2.2 Slot Context
//
typedef struct _SLOT_CONTEXT {
  UINT32                  RouteStr:20;
  UINT32                  Speed:4;
  UINT32                  RsvdZ1:1;
  UINT32                  MTT:1;
  UINT32                  Hub:1;
  UINT32                  ContextEntries:5;

  UINT32                  MaxExitLatency:16;
  UINT32                  RootHubPortNum:8;
  UINT32                  PortNum:8;

  UINT32                  TTHubSlotId:8;
  UINT32                  TTPortNum:8;
  UINT32                  TTT:2;
  UINT32                  RsvdZ2:4;
  UINT32                  InterTarget:10;

  UINT32                  DeviceAddress:8;
  UINT32                  RsvdZ3:19;
  UINT32                  SlotState:5;

  UINT32                  RsvdZ4;
  UINT32                  RsvdZ5;
  UINT32                  RsvdZ6;
  UINT32                  RsvdZ7;
} SLOT_CONTEXT;

//
// 6.2.3 Endpoint Context
//
typedef struct _ENDPOINT_CONTEXT {
  UINT32                  EPState:3;
  UINT32                  RsvdZ1:5;
  UINT32                  Mult:2;
  UINT32                  MaxPStreams:5;
  UINT32                  LSA:1;
  UINT32                  Interval:8;
  UINT32                  RsvdZ2:8;

  UINT32                  RsvdZ3:1;
  UINT32                  CErr:2;
  UINT32                  EPType:3;
  UINT32                  RsvdZ4:1;
  UINT32                  HID:1;
  UINT32                  MaxBurstSize:8;
  UINT32                  MaxPacketSize:16;

  UINT32                  PtrLo;

  UINT32                  PtrHi;

  UINT32                  AverageTRBLength:16;
  UINT32                  MaxESITPayload:16;

  UINT32                  RsvdZ5;
  UINT32                  RsvdZ6;
  UINT32                  RsvdZ7;
} ENDPOINT_CONTEXT;

//
// 6.2.5.1 Input Control Context
//
typedef struct _INPUT_CONTRL_CONTEXT {
  UINT32                  Dword1;
  UINT32                  Dword2;
  UINT32                  RsvdZ1;
  UINT32                  RsvdZ2;
  UINT32                  RsvdZ3;
  UINT32                  RsvdZ4;
  UINT32                  RsvdZ5;
  UINT32                  RsvdZ6;
} INPUT_CONTRL_CONTEXT;

//
// 6.2.1 Device Context
//
typedef struct _DEVICE_CONTEXT {
  SLOT_CONTEXT            Slot;
  ENDPOINT_CONTEXT        EP[31];
} DEVICE_CONTEXT;

//
// 6.2.5 Input Context
//
typedef struct _INPUT_CONTEXT {
  INPUT_CONTRL_CONTEXT    InputControlContext;
  SLOT_CONTEXT            Slot;
  ENDPOINT_CONTEXT        EP[31];
} INPUT_CONTEXT;

/**
  Initialize the XHCI host controller for schedule.

  @param  Xhc        The XHCI device to be initialized.

**/
VOID
XhcInitSched (
  IN USB_XHCI_DEV         *Xhc
  );

/**
  Free the resouce allocated at initializing schedule.

  @param  Xhc        The XHCI device.

**/
VOID
XhcFreeSched (
  IN USB_XHCI_DEV         *Xhc
  );

/**
  Ring the door bell to notify XHCI there is a transaction to be executed through URB.

  @param  Xhc           The XHCI device.
  @param  Urb           The URB to be rung.

  @retval EFI_SUCCESS   Successfully ring the door bell.

**/
EFI_STATUS
RingIntTransferDoorBell (
  IN  USB_XHCI_DEV        *Xhc,
  IN  URB                 *Urb
  );

/**
  Execute the transfer by polling the URB. This is a synchronous operation.

  @param  Xhc               The XHCI device.
  @param  CmdTransfer       The executed URB is for cmd transfer or not.
  @param  Urb               The URB to execute.
  @param  TimeOut           The time to wait before abort, in millisecond.

  @return EFI_DEVICE_ERROR  The transfer failed due to transfer error.
  @return EFI_TIMEOUT       The transfer failed due to time out.
  @return EFI_SUCCESS       The transfer finished OK.

**/
EFI_STATUS
XhcExecTransfer (
  IN  USB_XHCI_DEV        *Xhc,
  IN  BOOLEAN             CmdTransfer,
  IN  URB                 *Urb,
  IN  UINTN               TimeOut
  );

/**
  Delete a single asynchronous interrupt transfer for
  the device and endpoint.

  @param  Xhc                   The XHCI device.
  @param  DevAddr               The address of the target device.
  @param  EpNum                 The endpoint of the target.

  @retval EFI_SUCCESS           An asynchronous transfer is removed.
  @retval EFI_NOT_FOUND         No transfer for the device is found.

**/
EFI_STATUS
XhciDelAsyncIntTransfer (
  IN  USB_XHCI_DEV        *Xhc,
  IN  UINT8               DevAddr,
  IN  UINT8               EpNum
  );

/**
  Remove all the asynchronous interrupt transfers.

  @param  Xhc                   The XHCI device.

**/
VOID
XhciDelAllAsyncIntTransfers (
  IN USB_XHCI_DEV         *Xhc
  );

/**
  Set Bios Ownership

  @param  Xhc          The XHCI device.

**/
VOID
XhcSetBiosOwnership (
  IN USB_XHCI_DEV         *Xhc
  );

/**
  Clear Bios Ownership

  @param  Xhc       The XHCI device.

**/
VOID
XhcClearBiosOwnership (
  IN USB_XHCI_DEV         *Xhc
  );

/**
  Find out the slot id according to device address assigned by XHCI's Address_Device cmd.

  @param  DevAddr         The device address of the target device.

  @return The slot id used by the device.

**/
UINT8
XhcDevAddrToSlotId (
  IN  UINT8               DevAddr
  );

/**
  Find out the slot id according to the device's route string.

  @param  RouteString      The route string described the device location.

  @return The slot id used by the device.

**/
UINT8
EFIAPI
XhcRouteStringToSlotId (
  IN  USB_DEV_ROUTE     RouteString
  );

/**
  Calculate the device context index by endpoint address and direction.

  @param  EpAddr              The target endpoint number.
  @param  Direction           The direction of the target endpoint.

  @return The device context index of endpoint.

**/
UINT8
XhcEndpointToDci (
  IN  UINT8                   EpAddr,
  IN  UINT8                   Direction
  );

/**
  Ring the door bell to notify XHCI there is a transaction to be executed.

  @param  Xhc           The XHCI device.
  @param  SlotId        The slot id of the target device.
  @param  Dci           The device context index of the target slot or endpoint.

  @retval EFI_SUCCESS   Successfully ring the door bell.

**/
EFI_STATUS
EFIAPI
XhcRingDoorBell (
  IN USB_XHCI_DEV         *Xhc,
  IN UINT8                SlotId,
  IN UINT8                Dci
  );

/**
  Interrupt transfer periodic check handler.

  @param  Event                 Interrupt event.
  @param  Context               Pointer to USB_XHCI_DEV.

**/
VOID
EFIAPI
XhcMonitorAsyncRequests (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  );

/**
  Monitor the port status change. Enable/Disable device slot if there is a device attached/detached.

  @param  Xhc                   The XHCI device.
  @param  ParentRouteChart      The route string pointed to the parent device if it exists.
  @param  Port                  The port to be polled.
  @param  PortState             The port state.

  @retval EFI_SUCCESS           Successfully enable/disable device slot according to port state.
  @retval Others                Should not appear.

**/
EFI_STATUS
EFIAPI
XhcPollPortStatusChange (
  IN  USB_XHCI_DEV*         Xhc,
  IN  USB_DEV_ROUTE         ParentRouteChart,
  IN  UINT8                 Port,
  IN  EFI_USB_PORT_STATUS   *PortState
  );

/**
  Evaluate the slot context for hub device through XHCI's Configure_Endpoint cmd.

  @param  Xhc           The XHCI device.
  @param  SlotId        The slot id to be configured.
  @param  PortNum       The total number of downstream port supported by the hub.
  @param  TTT           The TT think time of the hub device.
  @param  MTT           The multi-TT of the hub device.

  @retval EFI_SUCCESS   Successfully configure the hub device's slot context.

**/
EFI_STATUS
XhcConfigHubContext (
  IN USB_XHCI_DEV             *Xhc,
  IN UINT8                    SlotId,
  IN UINT8                    PortNum,
  IN UINT8                    TTT,
  IN UINT8                    MTT
  );

/**
  Configure all the device endpoints through XHCI's Configure_Endpoint cmd.

  @param  Xhc           The XHCI device.
  @param  SlotId        The slot id to be configured.
  @param  DeviceSpeed   The device's speed.
  @param  ConfigDesc    The pointer to the usb device configuration descriptor.

  @retval EFI_SUCCESS   Successfully configure all the device endpoints.

**/
EFI_STATUS
EFIAPI
XhcSetConfigCmd (
  IN USB_XHCI_DEV             *Xhc,
  IN UINT8                    SlotId,
  IN UINT8                    DeviceSpeed,
  IN USB_CONFIG_DESCRIPTOR    *ConfigDesc
  );

/**
  Find out the actual device address according to the requested device address from UsbBus.

  @param  BusDevAddr       The requested device address by UsbBus upper driver.

  @return The actual device address assigned to the device.

**/
UINT8
EFIAPI
XhcBusDevAddrToSlotId (
  IN  UINT8       BusDevAddr
  );

/**
  Assign and initialize the device slot for a new device.

  @param  Xhc                 The XHCI device.
  @param  ParentRouteChart    The route string pointed to the parent device.
  @param  ParentPort          The port at which the device is located.
  @param  RouteChart          The route string pointed to the device.
  @param  DeviceSpeed         The device speed.

  @retval EFI_SUCCESS   Successfully assign a slot to the device and assign an address to it.

**/
EFI_STATUS
EFIAPI
XhcInitializeDeviceSlot (
  IN  USB_XHCI_DEV              *Xhc,
  IN  USB_DEV_ROUTE             ParentRouteChart,
  IN  UINT16                    ParentPort,
  IN  USB_DEV_ROUTE             RouteChart,
  IN  UINT8                     DeviceSpeed
  );

/**
  Evaluate the endpoint 0 context through XHCI's Evaluate_Context cmd.

  @param  Xhc           The XHCI device.
  @param  SlotId        The slot id to be evaluated.
  @param  MaxPacketSize The max packet size supported by the device control transfer.

  @retval EFI_SUCCESS   Successfully evaluate the device endpoint 0.

**/
EFI_STATUS
EFIAPI
XhcEvaluateContext (
  IN USB_XHCI_DEV             *Xhc,
  IN UINT8                    SlotId,
  IN UINT32                   MaxPacketSize
  );

/**
  Disable the specified device slot.

  @param  Xhc           The XHCI device.
  @param  SlotId        The slot id to be disabled.

  @retval EFI_SUCCESS   Successfully disable the device slot.

**/
EFI_STATUS
EFIAPI
XhcDisableSlotCmd (
  IN USB_XHCI_DEV             *Xhc,
  IN UINT8                    SlotId
  );

/**
  Synchronize the specified transfer ring to update the enqueue and dequeue pointer.

  @param  Xhc         The XHCI device.
  @param  TrsRing     The transfer ring to sync.

  @retval EFI_SUCCESS The transfer ring is synchronized successfully.

**/
EFI_STATUS
EFIAPI
XhcSyncTrsRing (
  IN USB_XHCI_DEV         *Xhc,
  TRANSFER_RING           *TrsRing
  );

/**
  Synchronize the specified event ring to update the enqueue and dequeue pointer.

  @param  Xhc         The XHCI device.
  @param  EvtRing     The event ring to sync.

  @retval EFI_SUCCESS The event ring is synchronized successfully.

**/
EFI_STATUS
EFIAPI
XhcSyncEventRing (
  IN USB_XHCI_DEV         *Xhc,
  EVENT_RING              *EvtRing
  );

/**
  Check if there is a new generated event.

  @param  Xhc           The XHCI device.
  @param  EvtRing       The event ring to check.
  @param  NewEvtTrb     The new event TRB found.

  @retval EFI_SUCCESS   Found a new event TRB at the event ring.
  @retval EFI_NOT_READY The event ring has no new event.

**/
EFI_STATUS
EFIAPI
XhcCheckNewEvent (
  IN  USB_XHCI_DEV            *Xhc,
  IN  EVENT_RING              *EvtRing,
  OUT TRB                     **NewEvtTrb
  );

/**
  Create XHCI transfer ring.

  @param  Xhc               The XHCI device.
  @param  TrbNum            The number of TRB in the ring.
  @param  TransferRing           The created transfer ring.

**/
VOID
CreateTransferRing (
  IN  USB_XHCI_DEV          *Xhc,
  IN  UINTN                 TrbNum,
  OUT TRANSFER_RING         *TransferRing
  );

/**
  Create XHCI event ring.

  @param  Xhc                 The XHCI device.
  @param  EventInterrupter    The interrupter of event.
  @param  EventRing           The created event ring.

**/
VOID
CreateEventRing (
  IN  USB_XHCI_DEV          *Xhc,
  IN  UINT8                 EventInterrupter,
  OUT EVENT_RING            *EventRing
  );

/**
  System software shall use a Reset Endpoint Command (section 4.11.4.7) to remove the Halted
  condition in the xHC. After the successful completion of the Reset Endpoint Command, the Endpoint
  Context is transitioned from the Halted to the Stopped state and the Transfer Ring of the endpoint is
  reenabled. The next write to the Doorbell of the Endpoint will transition the Endpoint Context from the
  Stopped to the Running state.

  @param  Xhc                   The XHCI device.
  @param  Urb                   The urb which makes the endpoint halted.

  @retval EFI_SUCCESS           The recovery is successful.
  @retval Others                Failed to recovery halted endpoint.

**/
EFI_STATUS
EFIAPI
XhcRecoverHaltedEndpoint (
  IN  USB_XHCI_DEV        *Xhc,
  IN  URB                 *Urb
  );

/**
  Create a new URB for a new transaction.

  @param  Xhc       The XHCI device
  @param  DevAddr   The device address
  @param  EpAddr    Endpoint addrress
  @param  DevSpeed  The device speed
  @param  MaxPacket The max packet length of the endpoint
  @param  Type      The transaction type
  @param  Request   The standard USB request for control transfer
  @param  Data      The user data to transfer
  @param  DataLen   The length of data buffer
  @param  Callback  The function to call when data is transferred
  @param  Context   The context to the callback

  @return Created URB or NULL

**/
URB*
XhcCreateUrb (
  IN USB_XHCI_DEV                       *Xhc,
  IN UINT8                              DevAddr,
  IN UINT8                              EpAddr,
  IN UINT8                              DevSpeed,
  IN UINTN                              MaxPacket,
  IN UINTN                              Type,
  IN EFI_USB_DEVICE_REQUEST             *Request,
  IN VOID                               *Data,
  IN UINTN                              DataLen,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK    Callback,
  IN VOID                               *Context
  );

/**
  Create a transfer TRB.

  @param  Xhc     The XHCI device
  @param  Urb     The urb used to construct the transfer TRB.

  @return Created TRB or NULL

**/
EFI_STATUS
XhcCreateTransferTrb (
  IN USB_XHCI_DEV                 *Xhc,
  IN URB                          *Urb
  );

#endif
