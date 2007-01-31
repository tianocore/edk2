/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    Ehci.h

Abstract:


Revision History
--*/

#ifndef _EHCI_H
#define _EHCI_H

//
// Universal Host Controller Interface data structures and defines
//
#include <IndustryStandard/pci22.h>


extern UINTN  gEHCDebugLevel;
extern UINTN  gEHCErrorLevel;


#define STALL_1_MACRO_SECOND              1
#define STALL_1_MILLI_SECOND              1000 * STALL_1_MACRO_SECOND
#define STALL_1_SECOND                    1000 * STALL_1_MILLI_SECOND

#define MEM_UNIT_SIZE                     128


#define SETUP_PACKET_PID_CODE             0x02
#define INPUT_PACKET_PID_CODE             0x01
#define OUTPUT_PACKET_PID_CODE            0x0

#define ITD_SELECT_TYPE                   0x0
#define QH_SELECT_TYPE                    0x01
#define SITD_SELECT_TYPE                  0x02
#define FSTN_SELECT_TYPE                  0x03

#define EHCI_SET_PORT_RESET_RECOVERY_TIME     50 * STALL_1_MILLI_SECOND
#define EHCI_CLEAR_PORT_RESET_RECOVERY_TIME   STALL_1_MILLI_SECOND
#define EHCI_GENERIC_TIMEOUT                  50 * STALL_1_MILLI_SECOND
#define EHCI_GENERIC_RECOVERY_TIME            50 * STALL_1_MACRO_SECOND
#define EHCI_SYNC_REQUEST_POLLING_TIME        50 * STALL_1_MACRO_SECOND
#define EHCI_ASYNC_REQUEST_POLLING_TIME       50 * STALL_1_MILLI_SECOND

#define USB_BAR_INDEX                     0 /* how many bytes away from USB_BASE to 0x10 */

#define NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES 16

#define EHCI_MIN_PACKET_SIZE              8
#define EHCI_MAX_PACKET_SIZE              1024
#define EHCI_MAX_FRAME_LIST_LENGTH        1024
#define EHCI_BLOCK_SIZE_WITH_TT           64
#define EHCI_BLOCK_SIZE                   512
#define EHCI_MAX_QTD_CAPACITY             (EFI_PAGE_SIZE * 5)

#define NAK_COUNT_RELOAD                  3
#define QTD_ERROR_COUNTER                 3
#define HIGH_BANDWIDTH_PIPE_MULTIPLIER    1

#define QTD_STATUS_ACTIVE                 0x80
#define QTD_STATUS_HALTED                 0x40
#define QTD_STATUS_BUFFER_ERR             0x20
#define QTD_STATUS_BABBLE_ERR             0x10
#define QTD_STATUS_TRANSACTION_ERR        0x08
#define QTD_STATUS_DO_STOP_SPLIT          0x02
#define QTD_STATUS_DO_START_SPLIT         0
#define QTD_STATUS_DO_PING                0x01
#define QTD_STATUS_DO_OUT                 0

#define DATA0                             0
#define DATA1                             1

#define MICRO_FRAME_0_CHANNEL             0x01
#define MICRO_FRAME_1_CHANNEL             0x02
#define MICRO_FRAME_2_CHANNEL             0x04
#define MICRO_FRAME_3_CHANNEL             0x08
#define MICRO_FRAME_4_CHANNEL             0x10
#define MICRO_FRAME_5_CHANNEL             0x20
#define MICRO_FRAME_6_CHANNEL             0x40
#define MICRO_FRAME_7_CHANNEL             0x80

#define CONTROL_TRANSFER                  0x01
#define BULK_TRANSFER                     0x02
#define SYNC_INTERRUPT_TRANSFER           0x04
#define ASYNC_INTERRUPT_TRANSFER          0x08
#define SYNC_ISOCHRONOUS_TRANSFER         0x10
#define ASYNC_ISOCHRONOUS_TRANSFER        0x20


//
// Enhanced Host Controller Registers definitions
//
extern EFI_DRIVER_BINDING_PROTOCOL  gEhciDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gEhciComponentName;

#define USBCMD            0x0     /* Command Register Offset 00-03h */
#define USBCMD_RS         0x01    /* Run / Stop */
#define USBCMD_HCRESET    0x02    /* Host controller reset */
#define USBCMD_FLS_512    0x04    /* 512 elements (2048bytes) in Frame List */
#define USBCMD_FLS_256    0x08    /* 256 elements (1024bytes) in Frame List */
#define USBCMD_PSE        0x10    /* Periodic schedule enable */
#define USBCMD_ASE        0x20    /* Asynchronous schedule enable */
#define USBCMD_IAAD       0x40    /* Interrupt on async advance doorbell */

#define USBSTS            0x04    /* Statue Register Offset 04-07h */
#define USBSTS_HSE        0x10    /* Host system error */
#define USBSTS_IAA        0x20    /* Interrupt on async advance */
#define USBSTS_HCH        0x1000  /* Host controller halted */
#define USBSTS_PSS        0x4000  /* Periodic schedule status */
#define USBSTS_ASS        0x8000  /* Asynchronous schedule status */

#define USBINTR           0x08    /* Command Register Offset 08-0bh */

#define FRINDEX           0x0c    /* Frame Index Offset 0c-0fh */

#define CTRLDSSGMENT      0x10    /* 4G Segment Selector Offset 10-13h */

#define PERIODICLISTBASE  0x14    /* Frame List Base Address Offset 14-17h */

#define ASYNCLISTADDR     0x18    /* Next Asynchronous List Address Offset 18-1bh */

#define CONFIGFLAG        0x40    /* Configured Flag Register Offset 40-43h */
#define CONFIGFLAG_CF     0x01    /* Configure Flag */

#define PORTSC            0x44    /* Port Status/Control Offset 44-47h */
#define PORTSC_CCS        0x01    /* Current Connect Status*/
#define PORTSC_CSC        0x02    /* Connect Status Change */
#define PORTSC_PED        0x04    /* Port Enable / Disable */
#define PORTSC_PEDC       0x08    /* Port Enable / Disable Change */
#define PORTSC_OCA        0x10    /* Over current Active */
#define PORTSC_OCC        0x20    /* Over current Change */
#define PORTSC_FPR        0x40    /* Force Port Resume */
#define PORTSC_SUSP       0x80    /* Port Suspend State */
#define PORTSC_PR         0x100   /* Port Reset */
#define PORTSC_LS_KSTATE  0x400   /* Line Status K-state */
#define PORTSC_LS_JSTATE  0x800   /* Line Status J-state */
#define PORTSC_PP         0x1000  /* Port Power */
#define PORTSC_PO         0x2000  /* Port Owner */

#define CAPLENGTH         0       /* Capability Register Length 00h */

#define HCIVERSION        0x02    /* Interface Version Number  02-03h */

#define HCSPARAMS         0x04    /* Structural Parameters 04-07h */
#define HCSP_NPORTS       0x0f    /* Number of physical downstream ports on host controller */

#define HCCPARAMS         0x08    /* Capability Parameters 08-0bh */
#define HCCP_64BIT        0x01    /* 64-bit Addressing Capability */
#define HCCP_PFLF         0x02    /* Programmable Frame List Flag */
#define HCCP_EECP         0xff00  /* EHCI Extemded Capabilities Pointer */

#define HCSPPORTROUTE     0x0c    /* Companion Port Route Description 60b */

#define CLASSC            0x09    /* Class Code 09-0bh */

#define USBBASE           0x10    /* Base Address to Memory-mapped Host Controller Register Space 10-13h */

#define SBRN              0x60    /* Serial Bus Release Number 60h */

#define FLADJ             0x61    /* Frame Length Adjustment Register 61h */

#define PORTWAKECAP       0x62    /* Port wake capablilities register(OPIONAL)  61-62h */

//
// PCI Configuration Registers
//
#define EHCI_PCI_CLASSC         0x09
#define EHCI_PCI_MEMORY_BASE    0x10

//
// Memory Offset Registers
//
#define EHCI_MEMORY_CAPLENGTH   0x0
#define EHCI_MEMORY_CONFIGFLAG  0x40

//
// USB Base Class Code,Sub-Class Code and Programming Interface
//
#define PCI_CLASSC_PI_EHCI      PCI_IF_EHCI

#define SETUP_PACKET_ID         0x2D
#define INPUT_PACKET_ID         0x69
#define OUTPUT_PACKET_ID        0xE1
#define ERROR_PACKET_ID         0x55

#define bit(a)                  (1 << (a))

#define GET_0B_TO_31B(Addr)     (((UINTN) Addr) & (0xffffffff))
#define GET_32B_TO_63B(Addr)    ((UINTN)RShiftU64((UINTN) Addr, 32) & (0xffffffff))


//
// Ehci Data and Ctrl Structures
//
#pragma pack(1)

typedef struct {
  UINT8 PI;
  UINT8 SubClassCode;
  UINT8 BaseCode;
} USB_CLASSC;

//
//32 Bytes Aligned
//
typedef struct {
  UINT32  NextQtdTerminate : 1;
  UINT32  Rsvd1 : 4;
  UINT32  NextQtdPointer : 27;

  UINT32  AltNextQtdTerminate : 1;
  UINT32  Rsvd2 : 4;
  UINT32  AltNextQtdPointer : 27;

  UINT32  Status : 8;
  UINT32  PidCode : 2;
  UINT32  ErrorCount : 2;
  UINT32  CurrentPage : 3;
  UINT32  InterruptOnComplete : 1;
  UINT32  TotalBytes : 15;
  UINT32  DataToggle : 1;

  UINT32  CurrentOffset : 12;
  UINT32  BufferPointer0 : 20;

  UINT32  Rsvd3 : 12;
  UINT32  BufferPointer1 : 20;

  UINT32  Rsvd4 : 12;
  UINT32  BufferPointer2 : 20;

  UINT32  Rsvd5 : 12;
  UINT32  BufferPointer3 : 20;

  UINT32  Rsvd6 : 12;
  UINT32  BufferPointer4 : 20;

  UINT32  PAD[5];
} EHCI_QTD_HW;

//
//32 Bytes Aligned
//
typedef struct {
  UINT32  QhTerminate : 1;
  UINT32  SelectType : 2;
  UINT32  Rsvd1 : 2;
  UINT32  QhHorizontalPointer : 27;

  UINT32  DeviceAddr : 7;
  UINT32  Inactive : 1;
  UINT32  EndpointNum : 4;
  UINT32  EndpointSpeed : 2;
  UINT32  DataToggleControl : 1;
  UINT32  HeadReclamationFlag : 1;
  UINT32  MaxPacketLen : 11;
  UINT32  ControlEndpointFlag : 1;
  UINT32  NakCountReload : 4;

  UINT32  InerruptScheduleMask : 8;
  UINT32  SplitComletionMask : 8;
  UINT32  HubAddr : 7;
  UINT32  PortNum : 7;
  UINT32  Multiplier : 2;

  UINT32  Rsvd2 : 5;
  UINT32  CurrentQtdPointer : 27;

  UINT32  NextQtdTerminate : 1;
  UINT32  Rsvd3 : 4;
  UINT32  NextQtdPointer : 27;

  UINT32  AltNextQtdTerminate : 1;
  UINT32  NakCount : 4;
  UINT32  AltNextQtdPointer : 27;

  UINT32  Status : 8;
  UINT32  PidCode : 2;
  UINT32  ErrorCount : 2;
  UINT32  CurrentPage : 3;
  UINT32  InterruptOnComplete : 1;
  UINT32  TotalBytes : 15;
  UINT32  DataToggle : 1;

  UINT32  CurrentOffset : 12;
  UINT32  BufferPointer0 : 20;

  UINT32  CompleteSplitMask : 8;
  UINT32  Rsvd4 : 4;
  UINT32  BufferPointer1 : 20;

  UINT32  FrameTag : 5;
  UINT32  SplitBytes : 7;
  UINT32  BufferPointer2 : 20;

  UINT32  Rsvd5 : 12;
  UINT32  BufferPointer3 : 20;

  UINT32  Rsvd6 : 12;
  UINT32  BufferPointer4 : 20;

  UINT32  Pad[5];
} EHCI_QH_HW;

typedef struct {
  UINT32  LinkTerminate : 1;
  UINT32  SelectType : 2;
  UINT32  Rsvd : 2;
  UINT32  LinkPointer : 27;
} FRAME_LIST_ENTRY;

#pragma pack()

typedef struct _EHCI_QTD_ENTITY     EHCI_QTD_ENTITY;
typedef struct _EHCI_QH_ENTITY      EHCI_QH_ENTITY;
typedef struct _EHCI_ASYNC_REQUEST  EHCI_ASYNC_REQUEST;
//
//Aligan On 32 Bytes
//
struct _EHCI_QTD_ENTITY {
  EHCI_QTD_HW     Qtd;
  UINT32          TotalBytes;
  UINT32          StaticTotalBytes;
  UINT32          StaticCurrentOffset;
  EHCI_QTD_ENTITY *Prev;
  EHCI_QTD_ENTITY *Next;
  EHCI_QTD_ENTITY *AltNext;
  EHCI_QH_ENTITY  *SelfQh;
};
//
//Aligan On 32 Bytes
//
struct _EHCI_QH_ENTITY {
  EHCI_QH_HW      Qh;
  EHCI_QH_ENTITY  *Next;
  EHCI_QH_ENTITY  *Prev;
  EHCI_QTD_ENTITY *FirstQtdPtr;
  EHCI_QTD_ENTITY *LastQtdPtr;
  EHCI_QTD_ENTITY *AltQtdPtr;
  UINTN           Interval;
  UINT8           TransferType;
};

#define GET_QH_ENTITY_ADDR(a)   ((EHCI_QH_ENTITY *) a)
#define GET_QTD_ENTITY_ADDR(a)  ((EHCI_QTD_ENTITY *) a)


//
// Ehci Managment Structures
//
#define USB2_HC_DEV_FROM_THIS(a)  CR (a, USB2_HC_DEV, Usb2Hc, USB2_HC_DEV_SIGNATURE)

#define USB2_HC_DEV_SIGNATURE     EFI_SIGNATURE_32 ('e', 'h', 'c', 'i')

struct _EHCI_ASYNC_REQUEST {
  UINT8                           TransferType;
  EFI_ASYNC_USB_TRANSFER_CALLBACK CallBackFunc;
  VOID                            *Context;
  EHCI_ASYNC_REQUEST              *Prev;
  EHCI_ASYNC_REQUEST              *Next;
  EHCI_QH_ENTITY                  *QhPtr;
};

typedef struct _MEMORY_MANAGE_HEADER {
  UINT8                         *BitArrayPtr;
  UINTN                         BitArraySizeInBytes;
  UINT8                         *MemoryBlockPtr;
  UINTN                         MemoryBlockSizeInBytes;
  VOID                          *Mapping;
  struct _MEMORY_MANAGE_HEADER  *Next;
} MEMORY_MANAGE_HEADER;

typedef struct _USB2_HC_DEV {
  UINTN                     Signature;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;
  UINTN                     PeriodicFrameListLength;
  VOID                      *PeriodicFrameListBuffer;
  VOID                      *PeriodicFrameListMap;
  VOID                      *AsyncList;
  EHCI_ASYNC_REQUEST        *AsyncRequestList;
  EFI_EVENT                 AsyncRequestEvent;
  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;
  MEMORY_MANAGE_HEADER      *MemoryHeader;
  UINT8                     Is64BitCapable;
  UINT32                    High32BitAddr;
  EHCI_QH_ENTITY            *NULLQH;
  UINT32                    UsbCapabilityLen;
  UINT16                    DeviceSpeed[16];
} USB2_HC_DEV;


//
// Prototypes
// Driver model protocol interface
//

EFI_STATUS
EFIAPI
EhciDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
EhciDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
EhciDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  );

//
// Ehci protocol interface
//
EFI_STATUS
EFIAPI
EhciGetCapability (
  IN  EFI_USB2_HC_PROTOCOL   *This,
  OUT UINT8                  *MaxSpeed,
  OUT UINT8                  *PortNumber,
  OUT UINT8                  *Is64BitCapable
  );

EFI_STATUS
EFIAPI
EhciReset (
  IN  EFI_USB2_HC_PROTOCOL     *This,
  IN  UINT16                   Attributes
  );

EFI_STATUS
EFIAPI
EhciGetState (
  IN  EFI_USB2_HC_PROTOCOL     *This,
  OUT EFI_USB_HC_STATE         *State
  );

EFI_STATUS
EFIAPI
EhciSetState (
  IN  EFI_USB2_HC_PROTOCOL     *This,
  IN  EFI_USB_HC_STATE         State
  );

EFI_STATUS
EFIAPI
EhciControlTransfer (
  IN  EFI_USB2_HC_PROTOCOL                 *This,
  IN  UINT8                                DeviceAddress,
  IN  UINT8                                DeviceSpeed,
  IN  UINTN                                MaximumPacketLength,
  IN  EFI_USB_DEVICE_REQUEST               *Request,
  IN  EFI_USB_DATA_DIRECTION               TransferDirection,
  IN  OUT VOID                             *Data,
  IN  OUT UINTN                            *DataLength,
  IN  UINTN                                TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR   *Translator,
  OUT UINT32                               *TransferResult
  );

EFI_STATUS
EFIAPI
EhciBulkTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN  OUT VOID                            *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN  OUT UINTN                           *DataLength,
  IN  OUT UINT8                           *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  );

EFI_STATUS
EFIAPI
EhciAsyncInterruptTransfer (
  IN  EFI_USB2_HC_PROTOCOL                  * This,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaxiumPacketLength,
  IN  BOOLEAN                               IsNewTransfer,
  IN  OUT UINT8                             *DataToggle,
  IN  UINTN                                 PollingInterval,
  IN  UINTN                                 DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR    *Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK       CallBackFunction,
  IN  VOID                                  *Context OPTIONAL
  );

EFI_STATUS
EFIAPI
EhciSyncInterruptTransfer (
  IN  EFI_USB2_HC_PROTOCOL                  *This,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaximumPacketLength,
  IN  OUT VOID                              *Data,
  IN  OUT UINTN                             *DataLength,
  IN  OUT UINT8                             *DataToggle,
  IN  UINTN                                 TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR    *Translator,
  OUT UINT32                                *TransferResult
  );

EFI_STATUS
EFIAPI
EhciIsochronousTransfer (
  IN  EFI_USB2_HC_PROTOCOL                  *This,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaximumPacketLength,
  IN  UINT8                                 DataBuffersNumber,
  IN  OUT VOID                              *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                                 DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR    *Translator,
  OUT UINT32                                *TransferResult
  );

EFI_STATUS
EFIAPI
EhciAsyncIsochronousTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN  OUT VOID                            *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN  VOID                                *Context
  );

EFI_STATUS
EFIAPI
EhciGetRootHubPortStatus (
  IN  EFI_USB2_HC_PROTOCOL     *This,
  IN  UINT8                    PortNumber,
  OUT EFI_USB_PORT_STATUS      *PortStatus
  );

EFI_STATUS
EFIAPI
EhciSetRootHubPortFeature (
  IN  EFI_USB2_HC_PROTOCOL     *This,
  IN  UINT8                    PortNumber,
  IN  EFI_USB_PORT_FEATURE     PortFeature
  );

EFI_STATUS
EFIAPI
EhciClearRootHubPortFeature (
  IN  EFI_USB2_HC_PROTOCOL     *This,
  IN  UINT8                    PortNumber,
  IN  EFI_USB_PORT_FEATURE     PortFeature
  );

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
EhciComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL     *This,
  IN  CHAR8                           *Language,
  OUT CHAR16                          **DriverName
  );

EFI_STATUS
EFIAPI
EhciComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL     *This,
  IN  EFI_HANDLE                      ControllerHandle,
  IN  EFI_HANDLE                      ChildHandle, OPTIONAL
  IN  CHAR8                           *Language,
  OUT CHAR16                          **ControllerName
  );

//
// Internal Functions Declaration
//

//
// EhciMem Functions
//
EFI_STATUS
CreateMemoryBlock (
  IN  USB2_HC_DEV               *HcDev,
  OUT MEMORY_MANAGE_HEADER      **MemoryHeader,
  IN  UINTN                     MemoryBlockSizeInPages
  )
/*++

Routine Description:

  Use PciIo->AllocateBuffer to allocate common buffer for the memory block,
  and use PciIo->Map to map the common buffer for Bus Master Read/Write.

Arguments:

  HcDev                  - USB2_HC_DEV
  MemoryHeader           - MEMORY_MANAGE_HEADER to output
  MemoryBlockSizeInPages - MemoryBlockSizeInPages

Returns:

  EFI_SUCCESS           Success
  EFI_OUT_OF_RESOURCES  Fail for no resources
  EFI_UNSUPPORTED       Unsupported currently

--*/
;

EFI_STATUS
FreeMemoryHeader (
  IN USB2_HC_DEV               *HcDev,
  IN MEMORY_MANAGE_HEADER      *MemoryHeader
  )
/*++

Routine Description:

  Free Memory Header

Arguments:

  HcDev         - USB2_HC_DEV
  MemoryHeader  - MemoryHeader to be freed

Returns:

  EFI_SUCCESS            Success
  EFI_INVALID_PARAMETER  Parameter is error

--*/
;

VOID
InsertMemoryHeaderToList (
  IN MEMORY_MANAGE_HEADER     *MemoryHeader,
  IN MEMORY_MANAGE_HEADER     *NewMemoryHeader
  )
/*++

Routine Description:

  Insert Memory Header To List

Arguments:

  MemoryHeader    - MEMORY_MANAGE_HEADER
  NewMemoryHeader - MEMORY_MANAGE_HEADER

Returns:

  VOID

--*/
;

EFI_STATUS
AllocMemInMemoryBlock (
  IN  MEMORY_MANAGE_HEADER     *MemoryHeader,
  OUT VOID                     **Pool,
  IN  UINTN                    NumberOfMemoryUnit
  )
/*++

Routine Description:

  Alloc Memory In MemoryBlock

Arguments:

  MemoryHeader        - MEMORY_MANAGE_HEADER
  Pool                - Place to store pointer to memory
  NumberOfMemoryUnit  - Number Of Memory Unit

Returns:

  EFI_SUCCESS    Success
  EFI_NOT_FOUND  Can't find the free memory

--*/
;

BOOLEAN
IsMemoryBlockEmptied (
  IN MEMORY_MANAGE_HEADER     *MemoryHeaderPtr
  )
/*++

Routine Description:

  Is Memory Block Emptied

Arguments:

  MemoryHeaderPtr - MEMORY_MANAGE_HEADER

Returns:

  TRUE    Empty
  FALSE   Not Empty

--*/
;

VOID
DelinkMemoryBlock (
  IN MEMORY_MANAGE_HEADER     *FirstMemoryHeader,
  IN MEMORY_MANAGE_HEADER     *NeedFreeMemoryHeader
  )
/*++

Routine Description:

  Delink Memory Block

Arguments:

  FirstMemoryHeader     - MEMORY_MANAGE_HEADER
  NeedFreeMemoryHeader  - MEMORY_MANAGE_HEADER

Returns:

  VOID

--*/
;

EFI_STATUS
InitialMemoryManagement (
  IN USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Initialize Memory Management

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

EFI_STATUS
DeinitialMemoryManagement (
  IN USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Deinitialize Memory Management

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

EFI_STATUS
EhciAllocatePool (
  IN  USB2_HC_DEV     *HcDev,
  OUT UINT8           **Pool,
  IN  UINTN           AllocSize
  )
/*++

Routine Description:

  Ehci Allocate Pool

Arguments:

  HcDev     - USB2_HC_DEV
  Pool      - Place to store pointer to the memory buffer
  AllocSize - Alloc Size

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

VOID
EhciFreePool (
  IN USB2_HC_DEV     *HcDev,
  IN UINT8           *Pool,
  IN UINTN           AllocSize
  )
/*++

Routine Description:

  Uhci Free Pool

Arguments:

  HcDev     - USB_HC_DEV
  Pool      - Pool to free
  AllocSize - Pool size

Returns:

  VOID

--*/
;

//
// EhciReg Functions
//
EFI_STATUS
ReadEhcCapabiltiyReg (
  IN USB2_HC_DEV             *HcDev,
  IN UINT32                  CapabiltiyRegAddr,
  IN OUT UINT32              *Data
  )
/*++

Routine Description:

  Read  Ehc Capabitlity register

Arguments:

  HcDev             - USB2_HC_DEV
  CapabiltiyRegAddr - Ehc Capability register address
  Data              - A pointer to data read from register

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

EFI_STATUS
ReadEhcOperationalReg (
  IN USB2_HC_DEV             *HcDev,
  IN UINT32                  OperationalRegAddr,
  IN OUT UINT32              *Data
  )
/*++

Routine Description:

  Read  Ehc Operation register

Arguments:

  HcDev                - USB2_HC_DEV
  OperationalRegAddr   - Ehc Operation register address
  Data                 - A pointer to data read from register

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

EFI_STATUS
WriteEhcOperationalReg (
  IN USB2_HC_DEV             *HcDev,
  IN UINT32                  OperationalRegAddr,
  IN UINT32                  Data
  )
/*++

Routine Description:

  Write  Ehc Operation register

Arguments:

  HcDev                - USB2_HC_DEV
  OperationalRegAddr   - Ehc Operation register address
  Data                 - 32bit write to register

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

EFI_STATUS
SetEhcDoorbell (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Set Ehc door bell bit

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
SetFrameListLen (
  IN USB2_HC_DEV     *HcDev,
  IN UINTN           Length
  )
/*++

Routine Description:

  Set the length of Frame List

Arguments:

  HcDev    - USB2_HC_DEV
  Length   - the required length of frame list

Returns:

  EFI_SUCCESS            Success
  EFI_INVALID_PARAMETER  Invalid parameter
  EFI_DEVICE_ERROR       Fail

--*/
;

BOOLEAN
IsFrameListProgrammable (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Whether frame list is programmable

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   Programmable
  FALSE  Unprogrammable

--*/
;

BOOLEAN
IsPeriodicScheduleEnabled (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Whether periodic schedule is enabled

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   Enabled
  FALSE  Disabled

--*/
;

BOOLEAN
IsAsyncScheduleEnabled (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Whether asynchronous schedule is enabled

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   Enabled
  FALSE  Disabled

--*/
;

BOOLEAN
IsEhcPortEnabled (
  IN  USB2_HC_DEV     *HcDev,
  IN  UINT8           PortNum
  )
/*++

Routine Description:

  Whether port is enabled

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   Enabled
  FALSE  Disabled

--*/
;

BOOLEAN
IsEhcReseted (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Whether Ehc is halted

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   Reseted
  FALSE  Unreseted

--*/
;

BOOLEAN
IsEhcHalted (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Whether Ehc is halted

Arguments:

  HcDev  - USB2_HC_DEV

Returns:

  TRUE   Halted
  FALSE  Not halted

--*/
;

BOOLEAN
IsEhcSysError (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Whether Ehc is system error

Arguments:

  HcDev  - USB2_HC_DEV

Returns:

  TRUE   System error
  FALSE  No system error

--*/
;

BOOLEAN
IsHighSpeedDevice (
  IN EFI_USB2_HC_PROTOCOL *This,
  IN UINT8                PortNum
  )
/*++

Routine Description:

  Whether high speed device attached

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   High speed
  FALSE  Full speed

--*/
;

EFI_STATUS
WaitForEhcReset (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  wait for Ehc reset or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
;

EFI_STATUS
WaitForEhcHalt (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  wait for Ehc halt or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
;

EFI_STATUS
WaitForEhcNotHalt (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  wait for Ehc not halt or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
;

EFI_STATUS
WaitForEhcDoorbell (
  IN  USB2_HC_DEV            *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  Wait for periodic schedule disable or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
;

EFI_STATUS
WaitForAsyncScheduleEnable (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  Wait for Ehc asynchronous schedule enable or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
;

EFI_STATUS
WaitForAsyncScheduleDisable (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  Wait for Ehc asynchronous schedule disable or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
;

EFI_STATUS
WaitForPeriodicScheduleEnable (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  Wait for Ehc periodic schedule enable or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
;

EFI_STATUS
WaitForPeriodicScheduleDisable (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  Wait for periodic schedule disable or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
;

EFI_STATUS
GetCapabilityLen (
  IN USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Get the length of capability register

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
SetFrameListBaseAddr (
  IN USB2_HC_DEV     *HcDev,
  IN UINT32          FrameBuffer
  )
/*++

Routine Description:

  Set base address of frame list first entry

Arguments:

  HcDev       - USB2_HC_DEV
  FrameBuffer - base address of first entry of frame list

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
SetAsyncListAddr (
  IN USB2_HC_DEV        *HcDev,
  IN EHCI_QH_ENTITY     *QhPtr
  )
/*++

Routine Description:

  Set address of first Async schedule Qh

Arguments:

  HcDev    - USB2_HC_DEV
  QhPtr    - A pointer to first Qh in the Async schedule

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
SetCtrlDataStructSeg (
  IN USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Set address of first Async schedule Qh

Arguments:

  HcDev    - USB2_HC_DEV
  QhPtr    - A pointer to first Qh in the Async schedule

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
SetPortRoutingEhc (
  IN USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Set Ehc port routing bit

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
EnablePeriodicSchedule (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Enable periodic schedule

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
DisablePeriodicSchedule (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Disable periodic schedule

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
EnableAsynchronousSchedule (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Enable asynchrounous schedule

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
DisableAsynchronousSchedule (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Disable asynchrounous schedule

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
StartScheduleExecution (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Start Ehc schedule execution

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
ResetEhc (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Reset Ehc

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
ClearEhcAllStatus (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Clear Ehc all status bits

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

//
// EhciSched Functions
//
EFI_STATUS
InitialPeriodicFrameList (
  IN USB2_HC_DEV      *HcDev,
  IN UINTN            Length
  )
/*++

Routine Description:

  Initialize Periodic Schedule Frame List

Arguments:

  HcDev   - USB2_HC_DEV
  Length  - Frame List Length

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

VOID
DeinitialPeriodicFrameList (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Deinitialize Periodic Schedule Frame List

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  VOID

--*/
;

EFI_STATUS
CreatePollingTimer (
  IN  USB2_HC_DEV      *HcDev,
  IN  EFI_EVENT_NOTIFY NotifyFunction
  )
/*++

Routine Description:

  Create Async Request Polling Timer

Arguments:

  HcDev          - USB2_HC_DEV
  NotifyFunction - Timer Notify Function

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

EFI_STATUS
DestoryPollingTimer (
  IN  USB2_HC_DEV *HcDev
  )
/*++

Routine Description:

  Destory Async Request Polling Timer

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

EFI_STATUS
StartPollingTimer (
  IN  USB2_HC_DEV *HcDev
  )
/*++

Routine Description:

  Start Async Request Polling Timer

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

EFI_STATUS
StopPollingTimer (
  IN  USB2_HC_DEV *HcDev
  )
/*++

Routine Description:

  Stop Async Request Polling Timer

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

EFI_STATUS
CreateQh (
  IN  USB2_HC_DEV         *HcDev,
  IN  UINT8               DeviceAddr,
  IN  UINT8               Endpoint,
  IN  UINT8               DeviceSpeed,
  IN  UINTN               MaxPacketLen,
  OUT EHCI_QH_ENTITY      **QhPtrPtr
  )
/*++

Routine Description:

  Create Qh Structure and Pre-Initialize

Arguments:

  HcDev          - USB2_HC_DEV
  DeviceAddr     - Address of Device
  Endpoint       - Endpoint Number
  DeviceSpeed    - Device Speed
  MaxPacketLen   - Max Length of one Packet
  QhPtrPtr       - A pointer of pointer to Qh for return

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
CreateControlQh (
  IN  USB2_HC_DEV                       *HcDev,
  IN  UINT8                             DeviceAddr,
  IN  UINT8                             DeviceSpeed,
  IN UINTN                              MaxPacketLen,
  IN EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT EHCI_QH_ENTITY                    **QhPtrPtr
  )
/*++

Routine Description:

  Create Qh for Control Transfer

Arguments:

  HcDev         - USB2_HC_DEV
  DeviceAddr    - Address of Device
  DeviceSpeed   - Device Speed
  MaxPacketLen  - Max Length of one Packet
  Translator    - Translator Transaction for SplitX
  QhPtrPtr      - A pointer of pointer to Qh for return

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
CreateBulkQh (
  IN  USB2_HC_DEV                         *HcDev,
  IN  UINT8                               DeviceAddr,
  IN  UINT8                               EndPointAddr,
  IN  UINT8                               DeviceSpeed,
  IN  UINT8                               DataToggle,
  IN  UINTN                               MaxPacketLen,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT EHCI_QH_ENTITY                      **QhPtrPtr
  )
/*++

Routine Description:

  Create Qh for Bulk Transfer

Arguments:

  HcDev         - USB2_HC_DEV
  DeviceAddr    - Address of Device
  EndPointAddr  - Address of Endpoint
  DeviceSpeed   - Device Speed
  MaxPacketLen  - Max Length of one Packet
  Translator    - Translator Transaction for SplitX
  QhPtrPtr      - A pointer of pointer to Qh for return

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
CreateInterruptQh (
  IN  USB2_HC_DEV                        *HcDev,
  IN  UINT8                              DeviceAddr,
  IN  UINT8                              EndPointAddr,
  IN  UINT8                              DeviceSpeed,
  IN  UINT8                              DataToggle,
  IN  UINTN                              MaxPacketLen,
  IN  UINTN                              Interval,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT EHCI_QH_ENTITY                     **QhPtrPtr
  )
/*++

Routine Description:

  Create Qh for Control Transfer

Arguments:

  HcDev        - USB2_HC_DEV
  DeviceAddr   - Address of Device
  EndPointAddr - Address of Endpoint
  DeviceSpeed  - Device Speed
  MaxPacketLen - Max Length of one Packet
  Interval     - value of interval
  Translator   - Translator Transaction for SplitX
  QhPtrPtr     - A pointer of pointer to Qh for return

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

VOID
DestoryQh (
  IN  USB2_HC_DEV        *HcDev,
  IN EHCI_QH_ENTITY      *QhPtr
  )
/*++

Routine Description:

  Destory Qh Structure

Arguments:

  HcDev - USB2_HC_DEV
  QhPtr - A pointer to Qh

Returns:

  VOID

--*/
;

EFI_STATUS
CreateQtd (
  IN  USB2_HC_DEV          *HcDev,
  IN  UINT8                *DataPtr,
  IN  UINTN                DataLen,
  IN  UINT8                PktId,
  IN  UINT8                Toggle,
  IN  UINT8                QtdStatus,
  OUT EHCI_QTD_ENTITY      **QtdPtrPtr
  )
/*++

Routine Description:

  Create Qtd Structure and Pre-Initialize it

Arguments:

  HcDev       - USB2_HC_DEV
  DataPtr     - A pointer to user data buffer to transfer
  DataLen     - Length of user data to transfer
  PktId       - Packet Identification of this Qtd
  Toggle      - Data Toggle of this Qtd
  QtdStatus   - Default value of status of this Qtd
  QtdPtrPtr   - A pointer of pointer to Qtd for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
;

EFI_STATUS
CreateSetupQtd (
  IN  USB2_HC_DEV          *HcDev,
  IN  UINT8                *DevReqPtr,
  OUT EHCI_QTD_ENTITY      **QtdPtrPtr
  )
/*++

Routine Description:

  Create Qtd Structure for Setup

Arguments:

  HcDev      - USB2_HC_DEV
  DevReqPtr  - A pointer to Device Request Data
  QtdPtrPtr  - A pointer of pointer to Qtd for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
;

EFI_STATUS
CreateDataQtd (
  IN  USB2_HC_DEV           *HcDev,
  IN  UINT8                 *DataPtr,
  IN  UINTN                 DataLen,
  IN  UINT8                 PktId,
  IN  UINT8                 Toggle,
  OUT EHCI_QTD_ENTITY       **QtdPtrPtr
  )
/*++

Routine Description:

  Create Qtd Structure for data

Arguments:

  HcDev       - USB2_HC_DEV
  DataPtr     - A pointer to user data buffer to transfer
  DataLen     - Length of user data to transfer
  PktId       - Packet Identification of this Qtd
  Toggle      - Data Toggle of this Qtd
  QtdPtrPtr   - A pointer of pointer to Qtd for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
;

EFI_STATUS
CreateStatusQtd (
  IN  USB2_HC_DEV           *HcDev,
  IN  UINT8                 PktId,
  OUT EHCI_QTD_ENTITY       **QtdPtrPtr
  )
/*++

Routine Description:

  Create Qtd Structure for status

Arguments:

  HcDev       - USB2_HC_DEV
  PktId       - Packet Identification of this Qtd
  QtdPtrPtr   - A pointer of pointer to Qtd for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
;

EFI_STATUS
CreateAltQtd (
  IN  USB2_HC_DEV           *HcDev,
  IN  UINT8                 PktId,
  OUT EHCI_QTD_ENTITY       **QtdPtrPtr
  )
/*++

Routine Description:

  Create Qtd Structure for Alternative

Arguments:

  HcDev      - USB2_HC_DEV
  PktId      - Packet Identification of this Qtd
  QtdPtrPtr  - A pointer of pointer to Qtd for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
;

EFI_STATUS
CreateControlQtds (
  IN  USB2_HC_DEV                         *HcDev,
  IN UINT8                                DataPktId,
  IN UINT8                                *RequestCursor,
  IN UINT8                                *DataCursor,
  IN UINTN                                DataLen,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT EHCI_QTD_ENTITY                     **ControlQtdsHead
  )
/*++

Routine Description:

  Create Qtds list for Control Transfer

Arguments:

  HcDev           - USB2_HC_DEV
  DataPktId       - Packet Identification of Data Qtds
  RequestCursor   - A pointer to request structure buffer to transfer
  DataCursor      - A pointer to user data buffer to transfer
  DataLen         - Length of user data to transfer
  ControlQtdsHead - A pointer of pointer to first Qtd for control tranfer for return

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
CreateBulkOrInterruptQtds (
  IN  USB2_HC_DEV                          *HcDev,
  IN  UINT8                                PktId,
  IN  UINT8                                *DataCursor,
  IN  UINTN                                DataLen,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR   *Translator,
  OUT EHCI_QTD_ENTITY                      **QtdsHead
  )
/*++

Routine Description:

  Create Qtds list for Bulk or Interrupt Transfer

Arguments:

  HcDev        - USB2_HC_DEV
  PktId        - Packet Identification of Qtds
  DataCursor   - A pointer to user data buffer to transfer
  DataLen      - Length of user data to transfer
  DataToggle   - Data Toggle to start
  Translator   - Translator Transaction for SplitX
  QtdsHead     - A pointer of pointer to first Qtd for control tranfer for return

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

VOID
DestoryQtds (
  IN USB2_HC_DEV          *HcDev,
  IN EHCI_QTD_ENTITY      *FirstQtdPtr
  )
/*++

Routine Description:

  Destory all Qtds in the list

Arguments:

  HcDev        - USB2_HC_DEV
  FirstQtdPtr  - A pointer to first Qtd in the list

Returns:

  VOID

--*/
;

VOID
LinkQtdToQtd (
  IN EHCI_QTD_ENTITY     *PreQtdPtr,
  IN EHCI_QTD_ENTITY     *QtdPtr
  )
/*++

Routine Description:

  Link Qtds together

Arguments:

  PreQtdPtr  - A pointer to pre Qtd
  QtdPtr     - A pointer to next Qtd

Returns:

  VOID

--*/
;

VOID
LinkQtdsToAltQtd (
  IN EHCI_QTD_ENTITY     *FirstQtdPtr,
  IN EHCI_QTD_ENTITY     *AltQtdPtr
  )
/*++

Routine Description:

  Link AlterQtds together

Arguments:

  FirstQtdPtr - A pointer to first Qtd in the list
  AltQtdPtr - A pointer to alternative Qtd

Returns:
  VOID

--*/
;

VOID
LinkQtdToQh (
  IN EHCI_QH_ENTITY      *QhPtr,
  IN EHCI_QTD_ENTITY     *QtdEntryPtr
  )
/*++

Routine Description:

  Link Qtds list to Qh

Arguments:

  QhPtr   - A pointer to Qh
  QtdPtr  - A pointer to first Qtd in the list

Returns:

  VOID

--*/
;

EFI_STATUS
LinkQhToAsyncList (
  IN  USB2_HC_DEV       *HcDev,
  IN EHCI_QH_ENTITY     *QhPtr
  )
/*++

Routine Description:

  Link Qh to Async Schedule List

Arguments:

  HcDev - USB2_HC_DEV
  QhPtr - A pointer to Qh

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

EFI_STATUS
UnlinkQhFromAsyncList (
  IN USB2_HC_DEV         *HcDev,
  IN EHCI_QH_ENTITY      *QhPtr
  )
/*++

Routine Description:

  Unlink Qh from Async Schedule List

Arguments:

  HcDev   - USB2_HC_DEV
  QhPtr   - A pointer to Qh

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
;

VOID
LinkQhToPeriodicList (
  IN USB2_HC_DEV        *HcDev,
  IN EHCI_QH_ENTITY     *QhPtr
  )
/*++

Routine Description:

  Link Qh to Periodic Schedule List

Arguments:

  HcDev   - USB2_HC_DEV
  QhPtr   - A pointer to Qh

Returns:

  VOID

--*/
;

VOID
UnlinkQhFromPeriodicList (
  IN USB2_HC_DEV         *HcDev,
  IN EHCI_QH_ENTITY      *QhPtr,
  IN UINTN                Interval
  )
/*++

Routine Description:

  Unlink Qh from Periodic Schedule List

Arguments:

  HcDev     - USB2_HC_DEV
  QhPtr     - A pointer to Qh
  Interval  - Interval of this periodic transfer

Returns:

  VOID

--*/
;

VOID
LinkToAsyncReqeust (
  IN  USB2_HC_DEV        *HcDev,
  IN  EHCI_ASYNC_REQUEST *AsyncRequestPtr
  )
/*++

Routine Description:

  Llink AsyncRequest Entry to Async Request List

Arguments:

  HcDev           - USB2_HC_DEV
  AsyncRequestPtr - A pointer to Async Request Entry

Returns:

  VOID

--*/
;

VOID
UnlinkFromAsyncReqeust (
  IN  USB2_HC_DEV        *HcDev,
  IN  EHCI_ASYNC_REQUEST *AsyncRequestPtr
  )
/*++

Routine Description:

  Unlink AsyncRequest Entry from Async Request List

Arguments:

  HcDev           - USB2_HC_DEV
  AsyncRequestPtr - A pointer to Async Request Entry

Returns:

  VOID

--*/
;

UINTN
GetNumberOfQtd (
  IN EHCI_QTD_ENTITY    *FirstQtdPtr
  )
/*++

Routine Description:

  Number of Qtds in the list

Arguments:

  FirstQtdPtr - A pointer to first Qtd in the list

Returns:

  Number of Qtds in the list

--*/
;



UINTN
GetCapacityOfQtd (
  IN UINT8    *BufferCursor
  )
/*++

Routine Description:

  Get Capacity of Qtd

Arguments:

  BufferCursor  - BufferCursor of the Qtd

Returns:

  Capacity of Qtd

--*/
;

UINTN
GetApproxiOfInterval (
  IN UINTN  Interval
  )
/*++

Routine Description:

  Get the approximate value in the 2 index sequence

Arguments:

  Interval - the value of interval

Returns:

  approximate value of interval in the 2 index sequence

--*/
;

EHCI_QTD_HW *
GetQtdNextPointer (
  IN EHCI_QTD_HW *HwQtdPtr
  )
/*++

Routine Description:

  Get Qtd next pointer field

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  A pointer to next hardware Qtd structure

--*/
;

BOOLEAN
IsQtdStatusActive (
  IN EHCI_QTD_HW *HwQtdPtr
  )
/*++

Routine Description:

  Whether Qtd status is active or not

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  TRUE    Active
  FALSE   Inactive

--*/
;

BOOLEAN
IsQtdStatusHalted (
  IN EHCI_QTD_HW *HwQtdPtr
  )
/*++

Routine Description:

  Whether Qtd status is halted or not

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  TRUE    Halted
  FALSE   Not halted

--*/
;

BOOLEAN
IsQtdStatusBufferError (
  IN EHCI_QTD_HW *HwQtdPtr
  )
/*++

Routine Description:

  Whether Qtd status is buffer error or not

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  TRUE    Buffer error
  FALSE   No buffer error

--*/
;

BOOLEAN
IsQtdStatusBabbleError (
  IN EHCI_QTD_HW *HwQtdPtr
  )
/*++

Routine Description:

  Whether Qtd status is babble error or not

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  TRUE    Babble error
  FALSE   No babble error

--*/
;

BOOLEAN
IsQtdStatusTransactionError (
  IN EHCI_QTD_HW *HwQtdPtr
  )
/*++

Routine Description:

  Whether Qtd status is transaction error or not

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  TRUE    Transaction error
  FALSE   No transaction error

--*/
;

BOOLEAN
IsDataInTransfer (
  IN  UINT8     EndPointAddress
  )
/*++

Routine Description:

  Whether is a DataIn direction transfer

Arguments:

  EndPointAddress - address of the endpoint

Returns:

  TRUE    DataIn
  FALSE   DataOut

--*/
;

EFI_STATUS
MapDataBuffer (
  IN  USB2_HC_DEV             *HcDev,
  IN  EFI_USB_DATA_DIRECTION  TransferDirection,
  IN  OUT VOID                *Data,
  IN  OUT UINTN               *DataLength,
  OUT UINT8                   *PktId,
  OUT UINT8                   **DataCursor,
  OUT VOID                    **DataMap
  )
/*++

Routine Description:

  Map address of user data buffer

Arguments:

  HcDev             - USB2_HC_DEV
  TransferDirection - direction of transfer
  Data              - A pointer to user data buffer
  DataLength        - length of user data
  PktId             - Packte Identificaion
  DataCursor        - mapped address to return
  DataMap           - identificaion of this mapping to return

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

EFI_STATUS
MapRequestBuffer (
  IN  USB2_HC_DEV             *HcDev,
  IN  OUT VOID                *Request,
  OUT UINT8                   **RequestCursor,
  OUT VOID                    **RequestMap
  )
/*++

Routine Description:

  Map address of request structure buffer

Arguments:

  HcDev           - USB2_HC_DEV
  Request         - A pointer to request structure
  RequestCursor   - Mapped address of request structure to return
  RequestMap      - Identificaion of this mapping to return

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

VOID
SetQtdBufferPointer (
  IN EHCI_QTD_HW *QtdHwPtr,
  IN VOID        *DataPtr,
  IN UINTN       DataLen
  )
/*++

Routine Description:

  Set data buffer pointers in Qtd

Arguments:

  QtdHwPtr  - A pointer to Qtd hardware structure
  DataPtr   - A pointer to user data buffer
  DataLen   - Length of the user data buffer

Returns:

  VOID

--*/
;

EHCI_QTD_HW *
GetQtdAlternateNextPointer (
  IN EHCI_QTD_HW *HwQtdPtr
  )
/*++

Routine Description:

  Get Qtd alternate next pointer field

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  A pointer to hardware alternate Qtd

--*/
;

VOID
ZeroOutQhOverlay (
  IN EHCI_QH_ENTITY *QhPtr
  )
/*++

Routine Description:

  Zero out the fields in Qh structure

Arguments:

  QhPtr - A pointer to Qh structure

Returns:

  VOID

--*/
;

VOID
UpdateAsyncRequestTransfer (
  IN EHCI_ASYNC_REQUEST *AsyncRequestPtr,
  IN UINT32             TransferResult,
  IN UINTN              ErrTDPos
  )
/*++

Routine Description:

  Update asynchronous request transfer

Arguments:

  AsyncRequestPtr  - A pointer to async request
  TransferResult   - transfer result
  ErrQtdPos        - postion of error Qtd

Returns:

  VOID

--*/
;


EFI_STATUS
DeleteAsyncRequestTransfer (
  IN  USB2_HC_DEV     *HcDev,
  IN  UINT8           DeviceAddress,
  IN  UINT8           EndPointAddress,
  OUT UINT8           *DataToggle
  )
/*++

Routine Description:

  Delete all asynchronous request transfer

Arguments:

  HcDev           - USB2_HC_DEV
  DeviceAddress   - address of usb device
  EndPointAddress - address of endpoint
  DataToggle      - stored data toggle

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;

VOID
CleanUpAllAsyncRequestTransfer (
  IN  USB2_HC_DEV *HcDev
  )
/*++

Routine Description:

  Clean up all asynchronous request transfer

Arguments:

  HcDev - USB2_HC_DEV

Returns:
  VOID

--*/
;

EFI_STATUS
ExecuteTransfer (
  IN  USB2_HC_DEV         *HcDev,
  IN BOOLEAN              IsControl,
  IN  EHCI_QH_ENTITY      *QhPtr,
  IN OUT UINTN            *ActualLen,
  OUT UINT8               *DataToggle,
  IN  UINTN               TimeOut,
  OUT UINT32              *TransferResult
  )
/*++

Routine Description:

  Execute Bulk or SyncInterrupt Transfer

Arguments:

  HcDev            - USB2_HC_DEV
  IsControl        - Is control transfer or not
  QhPtr            - A pointer to Qh
  ActualLen        - Actual transfered Len
  DataToggle       - Data Toggle
  TimeOut          - TimeOut threshold
  TransferResult   - Transfer result

Returns:

  EFI_SUCCESS        Sucess
  EFI_DEVICE_ERROR   Error

--*/
;

BOOLEAN
CheckQtdsTransferResult (
  IN BOOLEAN             IsControl,
  IN  EHCI_QH_ENTITY     *QhPtr,
  OUT UINT32             *Result,
  OUT UINTN              *ErrQtdPos,
  OUT UINTN              *ActualLen
  )
/*++

Routine Description:

  Check transfer result of Qtds

Arguments:

  IsControl     - Is control transfer or not
  QhPtr         - A pointer to Qh
  Result        - Transfer result
  ErrQtdPos     - Error TD Position
  ActualLen     - Actual Transfer Size

Returns:

  TRUE    Qtds finished
  FALSE   Not finish

--*/
;

EFI_STATUS
AsyncRequestMoniter (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
/*++

Routine Description:

  Interrupt transfer periodic check handler

Arguments:

  Event     - Interrupt event
  Context   - Pointer to USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
;


EFI_STATUS
CreateNULLQH (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Create the NULL QH to make it as the Async QH header

Arguments:

  HcDev   - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
--*/
;

VOID
DestroyNULLQH (
  IN  USB2_HC_DEV     *HcDev
  );

VOID
ClearLegacySupport (
  IN USB2_HC_DEV     *HcDev
  );

VOID
HostReset (
  IN USB2_HC_DEV    *HcDev
  );


VOID
DumpEHCIPortsStatus (
  IN USB2_HC_DEV    *HcDev
  );


#endif
