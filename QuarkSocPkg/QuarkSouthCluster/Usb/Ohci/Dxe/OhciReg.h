/** @file
This file contains the definination for host controller
register operation routines.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/



#ifndef   _OHCI_REG_H
#define   _OHCI_REG_H

#define   HC_STATE_RESET         0x0
#define   HC_STATE_RESUME        0x1
#define   HC_STATE_OPERATIONAL   0x2
#define   HC_STATE_SUSPEND       0x3

#define   PERIODIC_ENABLE               0x01
#define   ISOCHRONOUS_ENABLE            0x02
#define   CONTROL_ENABLE                0x04
#define   BULK_ENABLE                   0x08
#define   CONTROL_BULK_RATIO            0x10

#define   HC_FUNCTIONAL_STATE    0x20
#define   INTERRUPT_ROUTING      0x40

#define   HC_RESET               0x01
#define   CONTROL_LIST_FILLED    0x02
#define   BULK_LIST_FILLED       0x04
#define   CHANGE_OWNER_REQUEST   0x08

#define   SCHEDULE_OVERRUN_COUNT 0x10

#define   SCHEDULE_OVERRUN       0x00001
#define   WRITEBACK_DONE_HEAD    0x00002
#define   START_OF_FRAME         0x00004
#define   RESUME_DETECT          0x00008
#define   UNRECOVERABLE_ERROR    0x00010
#define   FRAME_NUMBER_OVERFLOW  0x00020
#define   ROOTHUB_STATUS_CHANGE  0x00040
#define   OWNERSHIP_CHANGE       0x00080

#define   MASTER_INTERRUPT       0x00400

#define   CONTROL_HEAD           0x001
#define   BULK_HEAD              0x002
#define   DONE_HEAD              0x004

#define   Hc_HCCA                0x001
#define   Hc_PERIODIC_CURRENT    0x002
#define   Hc_CONTOL_HEAD         0x004
#define   Hc_CONTROL_CURRENT_PTR 0x008
#define   Hc_BULK_HEAD           0x010
#define   Hc_BULK_CURRENT_PTR    0x020
#define   Hc_DONE_HEAD           0x040

#define   FRAME_INTERVAL         0x008
#define   FS_LARGEST_DATA_PACKET 0x010
#define   FRMINT_TOGGLE          0x020
#define   FRAME_REMAINING        0x040
#define   FRAME_REMAIN_TOGGLE    0x080

#define   RH_DESC_A              0x00001
#define   RH_DESC_B              0x00002
#define   RH_NUM_DS_PORTS        0x00004
#define   RH_NO_PSWITCH          0x00008
#define   RH_PSWITCH_MODE        0x00010
#define   RH_DEVICE_TYPE         0x00020
#define   RH_OC_PROT_MODE        0x00040
#define   RH_NOC_PROT            0x00080
#define   RH_POTPGT              0x00100
#define   RH_NO_POTPGT           0x00200
#define   RH_DEV_REMOVABLE       0x00400
#define   RH_PORT_PWR_CTRL_MASK  0x00800

#define   RH_LOCAL_PSTAT         0x00001
#define   RH_OC_ID               0x00002
#define   RH_REMOTE_WK_ENABLE    0x00004
#define   RH_LOCAL_PSTAT_CHANGE  0x00008
#define   RH_OC_ID_CHANGE        0x00010
#define   RH_CLR_RMT_WK_ENABLE   0x00020

#define   RH_CLEAR_PORT_ENABLE        0x0001
#define   RH_SET_PORT_ENABLE          0x0002
#define   RH_SET_PORT_SUSPEND         0x0004
#define   RH_CLEAR_SUSPEND_STATUS     0x0008
#define   RH_SET_PORT_RESET           0x0010
#define   RH_SET_PORT_POWER           0x0020
#define   RH_CLEAR_PORT_POWER         0x0040
#define   RH_CONNECT_STATUS_CHANGE    0x10000
#define   RH_PORT_ENABLE_STAT_CHANGE  0x20000
#define   RH_PORT_SUSPEND_STAT_CHANGE 0x40000
#define   RH_OC_INDICATOR_CHANGE      0x80000
#define   RH_PORT_RESET_STAT_CHANGE   0x100000

#define   RH_CURR_CONNECT_STAT        0x0001
#define   RH_PORT_ENABLE_STAT         0x0002
#define   RH_PORT_SUSPEND_STAT        0x0004
#define   RH_PORT_OC_INDICATOR        0x0008
#define   RH_PORT_RESET_STAT          0x0010
#define   RH_PORT_POWER_STAT          0x0020
#define   RH_LSDEVICE_ATTACHED        0x0040

#define   RESET_SYSTEM_BUS            (1 << 0)
#define   RESET_HOST_CONTROLLER       (1 << 1)
#define   RESET_CLOCK_GENERATION      (1 << 2)
#define   RESET_SSE_GLOBAL            (1 << 5)
#define   RESET_PSPL                  (1 << 6)
#define   RESET_PCPL                  (1 << 7)
#define   RESET_SSEP1                 (1 << 9)
#define   RESET_SSEP2                 (1 << 10)
#define   RESET_SSEP3                 (1 << 11)

#define ONE_SECOND                      1000000
#define ONE_MILLI_SEC                   1000
#define MAX_BYTES_PER_TD                0x1000
#define MAX_RETRY_TIMES                 100
#define PORT_NUMBER_ON_MAINSTONE2       1


//
// Operational Register Offsets
//

//
// Command & Status Registers Offsets
//
#define HC_REVISION             0x00
#define HC_CONTROL              0x04
#define HC_COMMAND_STATUS       0x08
#define HC_INTERRUPT_STATUS     0x0C
#define HC_INTERRUPT_ENABLE     0x10
#define HC_INTERRUPT_DISABLE    0x14

//
// Memory Pointer Offsets
//
#define HC_HCCA                 0x18
#define HC_PERIODIC_CURRENT     0x1C
#define HC_CONTROL_HEAD         0x20
#define HC_CONTROL_CURRENT_PTR  0x24
#define HC_BULK_HEAD            0x28
#define HC_BULK_CURRENT_PTR     0x2C
#define HC_DONE_HEAD            0x30

//
// Frame Register Offsets
//
#define HC_FRM_INTERVAL         0x34
#define HC_FRM_REMAINING        0x38
#define HC_FRM_NUMBER           0x3C
#define HC_PERIODIC_START       0x40
#define HC_LS_THREASHOLD        0x44

//
// Root Hub Register Offsets
//
#define HC_RH_DESC_A            0x48
#define HC_RH_DESC_B            0x4C
#define HC_RH_STATUS            0x50
#define HC_RH_PORT_STATUS       0x54

#define USBHOST_OFFSET_UHCHR         0x64         // Usb Host reset register

#define OHC_BAR_INDEX               0

//
// Usb Host controller register offset
//
#define USBHOST_OFFSET_UHCREV        0x0          // Usb Host revision register
#define USBHOST_OFFSET_UHCHCON       0x4          // Usb Host control register
#define USBHOST_OFFSET_UHCCOMS       0x8          // Usb Host Command Status register
#define USBHOST_OFFSET_UHCINTS       0xC          // Usb Host Interrupt Status register
#define USBHOST_OFFSET_UHCINTE       0x10         // Usb Host Interrupt Enable register
#define USBHOST_OFFSET_UHCINTD       0x14         // Usb Host Interrupt Disable register
#define USBHOST_OFFSET_UHCHCCA       0x18         // Usb Host Controller Communication Area
#define USBHOST_OFFSET_UHCPCED       0x1C         // Usb Host Period Current Endpoint Descriptor
#define USBHOST_OFFSET_UHCCHED       0x20         // Usb Host Control Head Endpoint Descriptor
#define USBHOST_OFFSET_UHCCCED       0x24         // Usb Host Control Current Endpoint Descriptor
#define USBHOST_OFFSET_UHCBHED       0x28         // Usb Host Bulk Head Endpoint Descriptor
#define USBHOST_OFFSET_UHCBCED       0x2C         // Usb Host Bulk Current Endpoint Descriptor
#define USBHOST_OFFSET_UHCDHEAD      0x30         // Usb Host Done Head register
#define USBHOST_OFFSET_UHCFMI        0x34         // Usb Host Frame Interval register
#define USBHOST_OFFSET_UHCFMR        0x38         // Usb Host Frame Remaining register
#define USBHOST_OFFSET_UHCFMN        0x3C         // Usb Host Frame Number register
#define USBHOST_OFFSET_UHCPERS       0x40         // Usb Host Periodic Start register
#define USBHOST_OFFSET_UHCLST        0x44         // Usb Host Low-Speed Threshold register
#define USBHOST_OFFSET_UHCRHDA       0x48         // Usb Host Root Hub Descriptor A register
#define USBHOST_OFFSET_UHCRHDB       0x4C         // Usb Host Root Hub Descriptor B register
#define USBHOST_OFFSET_UHCRHS        0x50         // Usb Host Root Hub Status register
#define USBHOST_OFFSET_UHCRHPS1      0x54         // Usb Host Root Hub Port Status 1 register

//
// Usb Host controller register bit fields
//
#pragma pack(1)

typedef struct {
  UINT8                   ProgInterface;
  UINT8                   SubClassCode;
  UINT8                   BaseCode;
} USB_CLASSC;

typedef struct {
    UINT32 Revision:8;
    UINT32 Rsvd:24;
} HcREVISION;

typedef struct {
    UINT32 ControlBulkRatio:2;
    UINT32 PeriodicEnable:1;
    UINT32 IsochronousEnable:1;
    UINT32 ControlEnable:1;
    UINT32 BulkEnable:1;
    UINT32 FunctionalState:2;
    UINT32 InterruptRouting:1;
    UINT32 RemoteWakeup:1;
    UINT32 RemoteWakeupEnable:1;
    UINT32 Reserved:21;
} HcCONTROL;

typedef struct {
    UINT32 HcReset:1;
    UINT32 ControlListFilled:1;
    UINT32 BulkListFilled:1;
    UINT32 ChangeOwnerRequest:1;
    UINT32 Reserved1:12;
    UINT32 ScheduleOverrunCount:2;
    UINT32 Reserved:14;
} HcCOMMAND_STATUS;

typedef struct {
    UINT32 SchedulingOverrun:1;
    UINT32 WriteBackDone:1;
    UINT32 Sof:1;
    UINT32 ResumeDetected:1;
    UINT32 UnrecoverableError:1;
    UINT32 FrameNumOverflow:1;
    UINT32 RHStatusChange:1;
    UINT32 Reserved1:23;
    UINT32 OwnerChange:1;
    UINT32 Reserved2:1;
} HcINTERRUPT_STATUS;

typedef struct {
    UINT32 SchedulingOverrunInt:1;
    UINT32 WriteBackDoneInt:1;
    UINT32 SofInt:1;
    UINT32 ResumeDetectedInt:1;
    UINT32 UnrecoverableErrorInt:1;
    UINT32 FrameNumOverflowInt:1;
    UINT32 RHStatusChangeInt:1;
    UINT32 Reserved:23;
    UINT32 OwnerChangedInt:1;
    UINT32 MasterInterruptEnable:1;
} HcINTERRUPT_CONTROL;

typedef struct {
    UINT32 Rerserved:8;
    UINT32 Hcca:24;
} HcHCCA;

typedef struct {
    UINT32 Reserved:4;
    UINT32 MemoryPtr:28;
} HcMEMORY_PTR;

typedef struct {
    UINT32 FrameInterval:14;
    UINT32 Reserved:2;
    UINT32 FSMaxDataPacket:15;
    UINT32 FrmIntervalToggle:1;
} HcFRM_INTERVAL;

typedef struct {
    UINT32 FrameRemaining:14;
    UINT32 Reserved:17;
    UINT32 FrameRemainingToggle:1;
} HcFRAME_REMAINING;

typedef struct {
    UINT32 FrameNumber:16;
    UINT32 Reserved:16;
} HcFRAME_NUMBER;

typedef struct {
    UINT32 PeriodicStart:14;
    UINT32 Reserved:18;
} HcPERIODIC_START;

typedef struct {
    UINT32 LsThreshold:12;
    UINT32 Reserved:20;
} HcLS_THRESHOLD;

typedef struct {
    UINT32 NumDownStrmPorts:8;
    UINT32 PowerSwitchMode:1;
    UINT32 NoPowerSwitch:1;
    UINT32 DeviceType:1;
    UINT32 OverCurrentProtMode:1;
    UINT32 NoOverCurrentProtMode:1;
    UINT32 Reserved:11;
    UINT32 PowerOnToPowerGoodTime:8;
} HcRH_DESC_A;

typedef struct {
    UINT32 DeviceRemovable:16;
    UINT32 PortPowerControlMask:16;
} HcRH_DESC_B;

typedef struct {
    UINT32 LocalPowerStat:1;
    UINT32 OverCurrentIndicator:1;
    UINT32 Reserved1:13;
    UINT32 DevRemoteWakeupEnable:1;
    UINT32 LocalPowerStatChange:1;
    UINT32 OverCurrentIndicatorChange:1;
    UINT32 Reserved2:13;
    UINT32 ClearRemoteWakeupEnable:1;
} HcRH_STATUS;

typedef struct {
    UINT32 CurrentConnectStat:1;
    UINT32 EnableStat:1;
    UINT32 SuspendStat:1;
    UINT32 OCIndicator:1;
    UINT32 ResetStat:1;
    UINT32 Reserved1:3;
    UINT32 PowerStat:1;
    UINT32 LsDeviceAttached:1;
    UINT32 Reserved2:6;
    UINT32 ConnectStatChange:1;
    UINT32 EnableStatChange:1;
    UINT32 SuspendStatChange:1;
    UINT32 OCIndicatorChange:1;
    UINT32 ResetStatChange:1;
    UINT32 Reserved3:11;
} HcRHPORT_STATUS;

typedef struct {
    UINT32 FSBIR:1;
    UINT32 FHR:1;
    UINT32 CGR:1;
    UINT32 SSDC:1;
    UINT32 UIT:1;
    UINT32 SSE:1;
    UINT32 PSPL:1;
    UINT32 PCPL:1;
    UINT32 Reserved0:1;
    UINT32 SSEP1:1;
    UINT32 SSEP2:1;
    UINT32 SSEP3:1;
    UINT32 Reserved1:20;
} HcRESET;


#pragma pack()

//
// Func List
//


/**

  Get OHCI operational reg value

  @param  PciIo                 PciIo protocol instance
  @param  Offset                Offset of the operational reg

  @retval                       Value of the register

**/
UINT32
OhciGetOperationalReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset
  );

/**

  Set OHCI operational reg value

  @param  PciIo                  PCI Bus Io protocol instance
  @param  Offset                 Offset of the operational reg
  @param  Value                  Value to set

  @retval EFI_SUCCESS            Value set to the reg

**/


EFI_STATUS
OhciSetOperationalReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN VOID                 *Value
  );


/**

  Get HcRevision reg value

  @param  PciIo                 PCI Bus Io protocol instance

  @retval                       Value of the register

**/


UINT32
OhciGetHcRevision (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  );

/**

  Set HcReset reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to set
  @param  Value                 Value to set

  @retval EFI_SUCCESS           Value set

**/

EFI_STATUS
OhciSetHcReset (
  IN USB_OHCI_HC_DEV            *Ohc,
  IN UINT32                     Field,
  IN UINT32                     Value
  );
/**

  Get specific field of HcReset reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetHcReset (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Field
  );
/**

  Set HcControl reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to set
  @param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetHcControl (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field,
  IN UINT32               Value
  );


/**

  Get specific field of HcControl reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/


UINT32
OhciGetHcControl (
  IN USB_OHCI_HC_DEV   *Ohc,
  IN UINTN             Field
  );


/**

  Set HcCommand reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to set
  @param  Value                 Value to set

  @retval EFI_SUCCESS           Value set

**/

EFI_STATUS
OhciSetHcCommandStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field,
  IN UINT32               Value
  );

/**

  Get specific field of HcCommand reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetHcCommandStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  );

/**

  Clear specific fields of Interrupt Status

  @param  Ohc                   UHC private data
  @param  Field                 Field to clear

  @retval EFI_SUCCESS           Fields cleared

**/

EFI_STATUS
OhciClearInterruptStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  );

/**

  Get fields of HcInterrupt reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetHcInterruptStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  );

/**

  Set Interrupt Control reg value

  @param  Ohc                   UHC private data
  @param  StatEnable            Enable or Disable
  @param  Field                 Field to set
  @param  Value                 Value to set

  @retval EFI_SUCCESS           Value set

**/

EFI_STATUS
OhciSetInterruptControl (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN BOOLEAN              StatEnable,
  IN UINTN                Field,
  IN UINT32               Value
  );

/**

  Get field of HcInterruptControl reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetHcInterruptControl (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  );


/**

  Set memory pointer of specific type

  @param  Ohc                   UHC private data
  @param  PointerType           Type of the pointer to set
  @param  Value                 Value to set

  @retval EFI_SUCCESS           Memory pointer set

**/

EFI_STATUS
OhciSetMemoryPointer(
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               PointerType,
  IN VOID                 *Value
  );

/**

  Get memory pointer of specific type

  @param  Ohc                   UHC private data
  @param  PointerType           Type of pointer

  @retval                       Memory pointer of the specific type

**/

VOID *
OhciGetMemoryPointer (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               PointerType
  );

/**

  Set Frame Interval value

  @param  Ohc                   UHC private data
  @param  Field                 Field to set
  @param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetFrameInterval (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field,
  IN UINT32               Value
  );


/**

  Get field of frame interval reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetFrameInterval (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  );


/**

  Set Frame Remaining reg value

  @param  Ohc                   UHC private data
  @param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetFrameRemaining (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Value
  );

/**

  Get value of frame remaining reg

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of frame remaining reg

**/
UINT32
OhciGetFrameRemaining (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  );

/**

  Set frame number reg value

  @param  Ohc                   UHC private data
  @param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetFrameNumber(
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Value
  );

/**

  Get frame number reg value

  @param  Ohc                   UHC private data

  @retval                       Value of frame number reg

**/

UINT32
OhciGetFrameNumber (
  IN USB_OHCI_HC_DEV      *Ohc
  );


/**

  Set period start reg value

  @param  Ohc                   UHC private data
  @param  Value                 Value to set

  @retval EFI_SUCCESS           Value set

**/

EFI_STATUS
OhciSetPeriodicStart (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Value
  );


/**

  Get periodic start reg value

  @param  Ohc                   UHC private data

  @param                        Value of periodic start reg

**/

UINT32
OhciGetPeriodicStart (
  IN USB_OHCI_HC_DEV      *Ohc
  );


/**

  Set Ls Threshold reg value

  @param  Ohc                   UHC private data
  @param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetLsThreshold (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Value
  );

/**

  Get Ls Threshold reg value

  @param  Ohc                   UHC private data

  @retval                       Value of Ls Threshold reg

**/

UINT32
OhciGetLsThreshold (
  IN USB_OHCI_HC_DEV      *Ohc
  );

/**

  Set Root Hub Descriptor reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to set
  @param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/
EFI_STATUS
OhciSetRootHubDescriptor (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field,
  IN UINT32               Value
  );


/**

  Get Root Hub Descriptor reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetRootHubDescriptor (
  IN USB_OHCI_HC_DEV     *Ohc,
  IN UINTN               Field
  );

/**

  Set Root Hub Status reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetRootHubStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  );


/**

  Get Root Hub Status reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetRootHubStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  );


/**

  Set Root Hub Port Status reg value

  @param  Ohc                   UHC private data
  @param  Index                 Index of the port
  @param  Field                 Field to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetRootHubPortStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Index,
  IN UINTN                Field
  );


/**

  Get Root Hub Port Status reg value

  @param  Ohc                   UHC private data
  @param  Index                 Index of the port
  @param  Field                 Field to get

  @retval                       Value of the field and index

**/

UINT32
OhciReadRootHubPortStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Index,
  IN UINTN                Field
  );

#endif
