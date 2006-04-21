/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    Uhci.h
    
Abstract: 
    

Revision History
--*/

#ifndef _UHCI_H
#define _UHCI_H

/*
 * Universal Host Controller Interface data structures and defines
 */

#include <IndustryStandard/Pci22.h>

#define EFI_D_UHCI  EFI_D_INFO

//
// stall time
//
#define STALL_1_MILLI_SECOND      1000
#define STALL_1_SECOND            1000 * STALL_1_MILLI_SECOND

#define FORCE_GLOBAL_RESUME_TIME  20 * STALL_1_MILLI_SECOND

#define ROOT_PORT_REST_TIME       50 * STALL_1_MILLI_SECOND

#define PORT_RESET_RECOVERY_TIME  10 * STALL_1_MILLI_SECOND

//
// 50 ms
//
#define INTERRUPT_POLLING_TIME  50 * 1000 * 10

//
// UHCI IO Space Address Register Register locates at
// offset 20 ~ 23h of PCI Configuration Space (UHCI spec, Revision 1.1),
// so, its BAR Index is 4.
//
#define USB_BAR_INDEX 4

//
// One memory block uses 1 page (common buffer for QH,TD use.)
//
#define NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES 1


#define bit(a)                            1 << (a)

//
// ////////////////////////////////////////////////////////////////////////
//
//          Universal Host Controller Registers Definitions
//
//////////////////////////////////////////////////////////////////////////
extern UINT16 USBBaseAddr;

/* Command register */
#define USBCMD          0       /* Command Register Offset 00-01h */
#define USBCMD_RS       bit (0) /* Run/Stop */
#define USBCMD_HCRESET  bit (1) /* Host reset */
#define USBCMD_GRESET   bit (2) /* Global reset */
#define USBCMD_EGSM     bit (3) /* Global Suspend Mode */
#define USBCMD_FGR      bit (4) /* Force Global Resume */
#define USBCMD_SWDBG    bit (5) /* SW Debug mode */
#define USBCMD_CF       bit (6) /* Config Flag (sw only) */
#define USBCMD_MAXP     bit (7) /* Max Packet (0 = 32, 1 = 64) */

/* Status register */
#define USBSTS        2       /* Status Register Offset 02-03h */
#define USBSTS_USBINT bit (0) /* Interrupt due to IOC */
#define USBSTS_ERROR  bit (1) /* Interrupt due to error */
#define USBSTS_RD     bit (2) /* Resume Detect */
#define USBSTS_HSE    bit (3) /* Host System Error*/
#define USBSTS_HCPE   bit (4) /* Host Controller Process Error*/
#define USBSTS_HCH    bit (5) /* HC Halted */

/* Interrupt enable register */
#define USBINTR         4       /* Interrupt Enable Register 04-05h */
#define USBINTR_TIMEOUT bit (0) /* Timeout/CRC error enable */
#define USBINTR_RESUME  bit (1) /* Resume interrupt enable */
#define USBINTR_IOC     bit (2) /* Interrupt On Complete enable */
#define USBINTR_SP      bit (3) /* Short packet interrupt enable */

/* Frame Number Register Offset 06-08h */
#define USBFRNUM  6

/* Frame List Base Address Register Offset 08-0Bh */
#define USBFLBASEADD  8

/* Start of Frame Modify Register Offset 0Ch */
#define USBSOF  0x0c

/* USB port status and control registers */
#define USBPORTSC1      0x10      /*Port 1 offset 10-11h */
#define USBPORTSC2      0x12      /*Port 2 offset 12-13h */

#define USBPORTSC_CCS   bit (0)   /* Current Connect Status*/
#define USBPORTSC_CSC   bit (1)   /* Connect Status Change */
#define USBPORTSC_PED   bit (2)   /* Port Enable / Disable */
#define USBPORTSC_PEDC  bit (3)   /* Port Enable / Disable Change */
#define USBPORTSC_LSL   bit (4)   /* Line Status Low bit*/
#define USBPORTSC_LSH   bit (5)   /* Line Status High bit*/
#define USBPORTSC_RD    bit (6)   /* Resume Detect */
#define USBPORTSC_LSDA  bit (8)   /* Low Speed Device Attached */
#define USBPORTSC_PR    bit (9)   /* Port Reset */
#define USBPORTSC_SUSP  bit (12)  /* Suspend */

/* PCI Configuration Registers for USB */

//
// Class Code Register offset
//
#define CLASSC  0x09
//
// USB IO Space Base Address Register offset
//
#define USBBASE 0x20

//
// USB legacy Support
//
#define USB_EMULATION 0xc0

//
// USB Base Class Code,Sub-Class Code and Programming Interface.
//
#define PCI_CLASSC_PI_UHCI  0x00

#define SETUP_PACKET_ID     0x2D
#define INPUT_PACKET_ID     0x69
#define OUTPUT_PACKET_ID    0xE1
#define ERROR_PACKET_ID     0x55

//
// ////////////////////////////////////////////////////////////////////////
//
//          USB Transfer Mechanism Data Structures
//
//////////////////////////////////////////////////////////////////////////
#pragma pack(1)
//
// USB Class Code structure
//
typedef struct {
  UINT8 PI;
  UINT8 SubClassCode;
  UINT8 BaseCode;
} USB_CLASSC;

typedef struct {
  UINT32  QHHorizontalTerminate : 1;
  UINT32  QHHorizontalQSelect : 1;
  UINT32  QHHorizontalRsvd : 2;
  UINT32  QHHorizontalPtr : 28;
  UINT32  QHVerticalTerminate : 1;
  UINT32  QHVerticalQSelect : 1;
  UINT32  QHVerticalRsvd : 2;
  UINT32  QHVerticalPtr : 28;
} QUEUE_HEAD;

typedef struct {
  UINT32  TDLinkPtrTerminate : 1;
  UINT32  TDLinkPtrQSelect : 1;
  UINT32  TDLinkPtrDepthSelect : 1;
  UINT32  TDLinkPtrRsvd : 1;
  UINT32  TDLinkPtr : 28;
  UINT32  TDStatusActualLength : 11;
  UINT32  TDStatusRsvd : 5;
  UINT32  TDStatus : 8;
  UINT32  TDStatusIOC : 1;
  UINT32  TDStatusIOS : 1;
  UINT32  TDStatusLS : 1;
  UINT32  TDStatusErr : 2;
  UINT32  TDStatusSPD : 1;
  UINT32  TDStatusRsvd2 : 2;
  UINT32  TDTokenPID : 8;
  UINT32  TDTokenDevAddr : 7;
  UINT32  TDTokenEndPt : 4;
  UINT32  TDTokenDataToggle : 1;
  UINT32  TDTokenRsvd : 1;
  UINT32  TDTokenMaxLen : 11;
  UINT32  TDBufferPtr;
} TD;

#pragma pack()

typedef struct {
  QUEUE_HEAD  QH;
  VOID        *ptrNext;
  VOID        *ptrDown;
  VOID        *ptrNextIntQH;  // for interrupt transfer's special use
  VOID        *LoopPtr;
} QH_STRUCT;

typedef struct {
  TD      TDData;
  UINT8   *pTDBuffer;
  VOID    *ptrNextTD;
  VOID    *ptrNextQH;
  UINT16  TDBufferLength;
  UINT16  reserved;
} TD_STRUCT;

//
// ////////////////////////////////////////////////////////////////////////
//
//          Universal Host Controller Device Data Structure
//
//////////////////////////////////////////////////////////////////////////
#define USB_HC_DEV_FROM_THIS(a)   CR (a, USB_HC_DEV, UsbHc, USB_HC_DEV_SIGNATURE)

#define USB_HC_DEV_SIGNATURE      EFI_SIGNATURE_32 ('u', 'h', 'c', 'i')
#define INTERRUPT_LIST_SIGNATURE  EFI_SIGNATURE_32 ('i', 'n', 't', 's')
typedef struct {
  UINTN                           Signature;

  LIST_ENTRY                      Link;
  UINT8                           DevAddr;
  UINT8                           EndPoint;
  UINT8                           DataToggle;
  UINT8                           Reserved[5];
  TD_STRUCT                       *PtrFirstTD;
  QH_STRUCT                       *PtrQH;
  UINTN                           DataLen;
  UINTN                           PollInterval;
  VOID                            *Mapping;
  UINT8                           *DataBuffer;  // allocated host memory, not mapped memory
  EFI_ASYNC_USB_TRANSFER_CALLBACK InterruptCallBack;
  VOID                            *InterruptContext;
} INTERRUPT_LIST;

#define INTERRUPT_LIST_FROM_LINK(a) CR (a, INTERRUPT_LIST, Link, INTERRUPT_LIST_SIGNATURE)

typedef struct {
  UINT32  FrameListPtrTerminate : 1;
  UINT32  FrameListPtrQSelect : 1;
  UINT32  FrameListRsvd : 2;
  UINT32  FrameListPtr : 28;

} FRAMELIST_ENTRY;

typedef struct _MEMORY_MANAGE_HEADER {
  UINT8                         *BitArrayPtr;
  UINTN                         BitArraySizeInBytes;
  UINT8                         *MemoryBlockPtr;
  UINTN                         MemoryBlockSizeInBytes;
  VOID                          *Mapping;
  struct _MEMORY_MANAGE_HEADER  *Next;
} MEMORY_MANAGE_HEADER;

typedef struct {
  UINTN                     Signature;
  EFI_USB_HC_PROTOCOL       UsbHc;
  EFI_PCI_IO_PROTOCOL       *PciIo;

  //
  // local data
  //
  LIST_ENTRY                InterruptListHead;
  FRAMELIST_ENTRY           *FrameListEntry;
  VOID                      *FrameListMapping;
  MEMORY_MANAGE_HEADER      *MemoryHeader;
  EFI_EVENT                 InterruptTransTimer;
  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;

} USB_HC_DEV;

extern EFI_DRIVER_BINDING_PROTOCOL  gUhciDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gUhciComponentName;

EFI_STATUS
WriteUHCCommandReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  CmdAddrOffset,
  IN UINT16                  UsbCmd
  );

EFI_STATUS
ReadUHCCommandReg (
  IN       EFI_PCI_IO_PROTOCOL     *PciIo,
  IN       UINT32                  CmdAddrOffset,
  IN OUT   UINT16                  *Data
  );

EFI_STATUS
WriteUHCStatusReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  StatusAddrOffset,
  IN UINT16                  UsbSts
  );

EFI_STATUS
ReadUHCStatusReg (
  IN       EFI_PCI_IO_PROTOCOL     *PciIo,
  IN       UINT32                  StatusAddrOffset,
  IN OUT   UINT16                  *Data
  );

EFI_STATUS
ClearStatusReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  StatusAddrOffset
  );

EFI_STATUS
ReadUHCFrameNumberReg (
  IN       EFI_PCI_IO_PROTOCOL     *PciIo,
  IN       UINT32                  FrameNumAddrOffset,
  IN OUT   UINT16                  *Data
  );

EFI_STATUS
WriteUHCFrameListBaseReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  FlBaseAddrOffset,
  IN UINT32                  UsbFrameListBaseAddr
  );

EFI_STATUS
ReadRootPortReg (
  IN       EFI_PCI_IO_PROTOCOL     *PciIo,
  IN       UINT32                  PortAddrOffset,
  IN OUT   UINT16                  *Data
  );

EFI_STATUS
WriteRootPortReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  PortAddrOffset,
  IN UINT16                  ControlBits
  );

EFI_STATUS
WaitForUHCHalt (
  IN EFI_PCI_IO_PROTOCOL      *PciIo,
  IN UINT32                   StatusRegAddr,
  IN UINTN                    Timeout
  );

BOOLEAN
IsStatusOK (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  StatusRegAddr
  );

BOOLEAN
IsHostSysOrProcessErr (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  StatusRegAddr
  );

//
// This routine programs the USB frame number register. We assume that the
// HC schedule execution is stopped.
//
EFI_STATUS
SetFrameNumberReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  FRNUMAddr,
  IN UINT16                  Index
  );

UINT16
GetCurrentFrameNumber (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  FRNUMAddr
  );

EFI_STATUS
SetFrameListBaseAddress (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  FLBASEADDRReg,
  IN UINT32                  Addr
  );

UINT32
GetFrameListBaseAddress (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  FLBAddr
  );

EFI_STATUS
CreateFrameList (
  IN USB_HC_DEV     *HcDev,
  IN UINT32         FLBASEADDRReg
  );

EFI_STATUS
FreeFrameListEntry (
  IN USB_HC_DEV     *UhcDev
  );

VOID
InitFrameList (
  IN USB_HC_DEV     *HcDev
  );


EFI_STATUS
CreateQH (
  IN  USB_HC_DEV     *HcDev,
  OUT QH_STRUCT      **pptrQH
  );

VOID
SetQHHorizontalLinkPtr (
  IN QH_STRUCT     *ptrQH,
  IN VOID          *ptrNext
  );

VOID *
GetQHHorizontalLinkPtr (
  IN QH_STRUCT     *ptrQH
  );

VOID
SetQHHorizontalQHorTDSelect (
  IN QH_STRUCT     *ptrQH,
  IN BOOLEAN       bQH
  );

VOID
SetQHHorizontalValidorInvalid (
  IN QH_STRUCT     *ptrQH,
  IN BOOLEAN       bValid
  );

VOID
SetQHVerticalLinkPtr (
  IN QH_STRUCT     *ptrQH,
  IN VOID          *ptrNext
  );

VOID * 
GetQHVerticalLinkPtr (
  IN QH_STRUCT     *ptrQH
  );

VOID
SetQHVerticalQHorTDSelect (
  IN QH_STRUCT     *ptrQH,
  IN BOOLEAN       bQH
  );

BOOLEAN
IsQHHorizontalQHSelect (
  IN QH_STRUCT     *ptrQH
  );

VOID
SetQHVerticalValidorInvalid (
  IN QH_STRUCT     *ptrQH,
  IN BOOLEAN       bValid
  );

BOOLEAN
GetQHVerticalValidorInvalid (
  IN QH_STRUCT     *ptrQH
  );

EFI_STATUS
AllocateTDStruct (
  IN  USB_HC_DEV     *HcDev,
  OUT TD_STRUCT      **ppTDStruct
  );
/*++

Routine Description:

  Allocate TD Struct

Arguments:

  HcDev       - USB_HC_DEV
  ppTDStruct  - place to store TD_STRUCT pointer
Returns:

  EFI_SUCCESS

--*/

EFI_STATUS
CreateTD (
  IN  USB_HC_DEV     *HcDev,
  OUT TD_STRUCT      **pptrTD
  );
/*++

Routine Description:

  Create TD

Arguments:

  HcDev   - USB_HC_DEV
  pptrTD  - TD_STRUCT pointer to store

Returns:

  EFI_OUT_OF_RESOURCES - Can't allocate resources
  EFI_SUCCESS          - Success

--*/


EFI_STATUS
GenSetupStageTD (
  IN  USB_HC_DEV     *HcDev,
  IN  UINT8          DevAddr,
  IN  UINT8          Endpoint,
  IN  BOOLEAN        bSlow,
  IN  UINT8          *pDevReq,
  IN  UINT8          RequestLen,
  OUT TD_STRUCT      **ppTD
  );
/*++

Routine Description:

  Generate Setup Stage TD

Arguments:

  HcDev       - USB_HC_DEV
  DevAddr     - Device address
  Endpoint    - Endpoint number 
  bSlow       - Full speed or low speed
  pDevReq     - Device request
  RequestLen  - Request length
  ppTD        - TD_STRUCT to return
Returns:

  EFI_OUT_OF_RESOURCES - Can't allocate memory
  EFI_SUCCESS          - Success

--*/

EFI_STATUS
GenDataTD (
  IN  USB_HC_DEV     *HcDev,
  IN  UINT8          DevAddr,
  IN  UINT8          Endpoint,
  IN  UINT8          *pData,
  IN  UINT8          Len,
  IN  UINT8          PktID,
  IN  UINT8          Toggle,
  IN  BOOLEAN        bSlow,
  OUT TD_STRUCT      **ppTD
  );
/*++

Routine Description:

  Generate Data Stage TD

Arguments:

  HcDev       - USB_HC_DEV
  DevAddr     - Device address
  Endpoint    - Endpoint number 
  pData       - Data buffer 
  Len         - Data length
  PktID       - Packet ID
  Toggle      - Data toggle value
  bSlow       - Full speed or low speed
  ppTD        - TD_STRUCT to return
Returns:

  EFI_OUT_OF_RESOURCES - Can't allocate memory
  EFI_SUCCESS          - Success

--*/

EFI_STATUS
CreateStatusTD (
  IN  USB_HC_DEV     *HcDev,
  IN  UINT8          DevAddr,
  IN  UINT8          Endpoint,
  IN  UINT8          PktID,
  IN  BOOLEAN        bSlow,
  OUT TD_STRUCT      **ppTD
  );
/*++

Routine Description:

  Generate Setup Stage TD

Arguments:

  HcDev       - USB_HC_DEV
  DevAddr     - Device address
  Endpoint    - Endpoint number 
  bSlow       - Full speed or low speed
  pDevReq     - Device request
  RequestLen  - Request length
  ppTD        - TD_STRUCT to return
Returns:

  EFI_OUT_OF_RESOURCES - Can't allocate memory
  EFI_SUCCESS          - Success

--*/

VOID
SetTDLinkPtrValidorInvalid (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bValid
  );

VOID
SetTDLinkPtrQHorTDSelect (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bQH
  );

VOID
SetTDLinkPtrDepthorBreadth (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bDepth
  );

VOID
SetTDLinkPtr (
  IN TD_STRUCT     *ptrTDStruct,
  IN VOID          *ptrNext
  );

VOID *
GetTDLinkPtr (
  IN TD_STRUCT   *ptrTDStruct
  );

VOID
EnableorDisableTDShortPacket (
  IN TD_STRUCT   *ptrTDStruct,
  IN BOOLEAN     bEnable
  );

VOID
SetTDControlErrorCounter (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINT8         nMaxErrors
  );

VOID
SetTDLoworFullSpeedDevice (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bLowSpeedDevice
  );

VOID
SetTDControlIsochronousorNot (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bIsochronous
  );

VOID
SetorClearTDControlIOC (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bSet
  );

VOID
SetTDStatusActiveorInactive (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bActive
  );

UINT16
SetTDTokenMaxLength (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINT16        nMaxLen
  );

VOID
SetTDTokenDataToggle1 (
  IN TD_STRUCT    *ptrTDStruct
  );

VOID
SetTDTokenDataToggle0 (
  IN TD_STRUCT    *ptrTDStruct
  );

UINT8
GetTDTokenDataToggle (
  IN TD_STRUCT     *ptrTDStruct
  );

VOID
SetTDTokenEndPoint (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINTN         nEndPoint
  );

VOID
SetTDTokenDeviceAddress (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINTN         nDevAddr
  );

VOID
SetTDTokenPacketID (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINT8         nPID
  );

VOID
SetTDDataBuffer (
  IN TD_STRUCT     *ptrTDStruct
  );

BOOLEAN
IsTDStatusActive (
  IN TD_STRUCT     *ptrTDStruct
  );

BOOLEAN
IsTDStatusStalled (
  IN TD_STRUCT     *ptrTDStruct
  );

BOOLEAN
IsTDStatusBufferError (
  IN TD_STRUCT     *ptrTDStruct
  );

BOOLEAN
IsTDStatusBabbleError (
  IN TD_STRUCT     *ptrTDStruct
  );

BOOLEAN
IsTDStatusNAKReceived (
  IN TD_STRUCT     *ptrTDStruct
  );

BOOLEAN
IsTDStatusCRCTimeOutError (
  IN TD_STRUCT     *ptrTDStruct
  );

BOOLEAN
IsTDStatusBitStuffError (
  IN TD_STRUCT     *ptrTDStruct
  );

UINT16
GetTDStatusActualLength (
  IN TD_STRUCT     *ptrTDStruct
  );

UINT16
GetTDTokenMaxLength (
  IN TD_STRUCT     *ptrTDStruct
  );

UINT8
GetTDTokenEndPoint (
  IN TD_STRUCT     *ptrTDStruct
  );

UINT8
GetTDTokenDeviceAddress (
  IN TD_STRUCT     *ptrTDStruct
  );

UINT8
GetTDTokenPacketID (
  IN TD_STRUCT     *ptrTDStruct
  );

UINT8 *
GetTDDataBuffer (
  IN TD_STRUCT     *ptrTDStruct
  );

BOOLEAN
GetTDLinkPtrValidorInvalid (
  IN TD_STRUCT     *ptrTDStruct
  );

UINTN
CountTDsNumber (
  IN TD_STRUCT     *ptrFirstTD
  );

VOID
LinkTDToQH (
  IN QH_STRUCT     *ptrQH,
  IN TD_STRUCT     *ptrTD
  );

VOID
LinkTDToTD (
  IN TD_STRUCT     *ptrPreTD,
  IN TD_STRUCT     *ptrTD
  );

VOID
SetorClearCurFrameListTerminate (
  IN FRAMELIST_ENTRY     *pCurEntry,
  IN BOOLEAN             bSet
  );

VOID
SetCurFrameListQHorTD (
  IN FRAMELIST_ENTRY        *pCurEntry,
  IN BOOLEAN                bQH
  );

BOOLEAN
GetCurFrameListTerminate (
  IN FRAMELIST_ENTRY     *pCurEntry
  );

VOID
SetCurFrameListPointer (
  IN FRAMELIST_ENTRY     *pCurEntry,
  IN UINT8               *ptr
  );

VOID *
GetCurFrameListPointer (
  IN FRAMELIST_ENTRY     *pCurEntry
  );

VOID
LinkQHToFrameList (
  IN FRAMELIST_ENTRY   *pEntry,
  IN UINT16            FrameListIndex,
  IN QH_STRUCT         *ptrQH
  );
/*++

Routine Description:

  Link QH To Frame List

Arguments:

  pEntry           - FRAMELIST_ENTRY
  FrameListIndex   - Frame List Index
  PtrQH            - QH to link 
Returns:

  VOID

--*/
VOID
DeleteQHTDs (
  IN FRAMELIST_ENTRY *pEntry,
  IN QH_STRUCT       *ptrQH,
  IN TD_STRUCT       *ptrFirstTD,
  IN UINT16          FrameListIndex,
  IN BOOLEAN         SearchOther
  );

VOID
DelLinkSingleQH (
  IN USB_HC_DEV      *HcDev,
  IN QH_STRUCT       *ptrQH,
  IN UINT16          FrameListIndex,
  IN BOOLEAN         SearchOther,
  IN BOOLEAN         Delete
  );

VOID
DeleteQueuedTDs (
  IN USB_HC_DEV      *HcDev,
  IN TD_STRUCT       *ptrFirstTD
  );

VOID
InsertQHTDToINTList (
  IN USB_HC_DEV                        *HcDev,
  IN QH_STRUCT                         *ptrQH,
  IN TD_STRUCT                         *ptrFirstTD,
  IN UINT8                             DeviceAddress,
  IN UINT8                             EndPointAddress,
  IN UINT8                             DataToggle,
  IN UINTN                             DataLength,
  IN UINTN                             PollingInterval,
  IN VOID                              *Mapping,
  IN UINT8                             *DataBuffer,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK   CallBackFunction,
  IN VOID                              *Context
  );
/*++
Routine Description:
  Insert QH and TD To Interrupt List
Arguments:

  HcDev           - USB_HC_DEV
  PtrQH           - QH_STRUCT
  PtrFirstTD      - First TD_STRUCT
  DeviceAddress   - Device Address
  EndPointAddress - EndPoint Address
  DataToggle      - Data Toggle
  DataLength      - Data length 
  PollingInterval - Polling Interval when inserted to frame list
  Mapping         - Mapping alue  
  DataBuffer      - Data buffer
  CallBackFunction- CallBackFunction after interrupt transfeer
  Context         - CallBackFunction Context passed as function parameter
Returns:
  EFI_SUCCESS            - Sucess
  EFI_INVALID_PARAMETER  - Paremeter is error 

--*/

EFI_STATUS
DeleteAsyncINTQHTDs (
  IN  USB_HC_DEV  *HcDev,
  IN  UINT8       DeviceAddress,
  IN  UINT8       EndPointAddress,
  OUT UINT8       *DataToggle
  );
/*++
Routine Description:

  Delete Async INT QH and TDs
Arguments:

  HcDev           - USB_HC_DEV
  DeviceAddress   - Device Address
  EndPointAddress - EndPoint Address
  DataToggle      - Data Toggle

Returns:
  EFI_SUCCESS            - Sucess
  EFI_INVALID_PARAMETER  - Paremeter is error 

--*/
BOOLEAN
CheckTDsResults (
  IN  TD_STRUCT               *ptrTD,
  IN  UINTN                   RequiredLen,
  OUT UINT32                  *Result,
  OUT UINTN                   *ErrTDPos,
  OUT UINTN                   *ActualTransferSize
  );
/*++

Routine Description:

  Check TDs Results

Arguments:

  PtrTD               - TD_STRUCT to check
  RequiredLen         - Required Len
  Result              - Transfer result
  ErrTDPos            - Error TD Position
  ActualTransferSize  - Actual Transfer Size

Returns:

  TRUE  - Sucess
  FALSE - Fail

--*/
VOID
ExecuteAsyncINTTDs (
  IN  USB_HC_DEV      *HcDev,
  IN  INTERRUPT_LIST  *ptrList,
  OUT UINT32          *Result,
  OUT UINTN           *ErrTDPos,
  OUT UINTN           *ActualLen
  ) ;
/*++

Routine Description:

  Execute Async Interrupt TDs

Arguments:

  HcDev     - USB_HC_DEV
  PtrList   - INTERRUPT_LIST
  Result    - Transfer result
  ErrTDPos  - Error TD Position
  ActualTransferSize  - Actual Transfer Size
  
Returns:

  VOID

--*/
VOID
UpdateAsyncINTQHTDs (
  IN INTERRUPT_LIST  *ptrList,
  IN UINT32          Result,
  IN UINT32          ErrTDPos
  );
/*++

Routine Description:

  Update Async Interrupt QH and TDs

Arguments:

  PtrList   - INTERRUPT_LIST
  Result    - Transfer reslut
  ErrTDPos  - Error TD Position

Returns:

  VOID

--*/
VOID
ReleaseInterruptList (
  IN USB_HC_DEV      *HcDev,
  IN LIST_ENTRY      *ListHead
  );
/*++

Routine Description:

  Release Interrupt List
Arguments:

  HcDev     - USB_HC_DEV
  ListHead  - List head

Returns:

  VOID

--*/
EFI_STATUS
ExecuteControlTransfer (
  IN  USB_HC_DEV  *HcDev,
  IN  TD_STRUCT   *ptrTD,
  IN  UINT32      wIndex,
  OUT UINTN       *ActualLen,
  IN  UINTN       TimeOut,
  OUT UINT32      *TransferResult
  );
/*++

Routine Description:

  Execute Control Transfer

Arguments:

  HcDev            - USB_HC_DEV
  PtrTD            - TD_STRUCT
  wIndex           - No use
  ActualLen        - Actual transfered Len 
  TimeOut          - TimeOut value in milliseconds
  TransferResult   - Transfer result
Returns:

  EFI_SUCCESS      - Sucess
  EFI_DEVICE_ERROR - Error
  

--*/
EFI_STATUS
ExecBulkorSyncInterruptTransfer (
  IN  USB_HC_DEV  *HcDev,
  IN  TD_STRUCT   *ptrTD,
  IN  UINT32      wIndex,
  OUT UINTN       *ActualLen,
  OUT UINT8       *DataToggle,
  IN  UINTN       TimeOut,
  OUT UINT32      *TransferResult
  );
/*++

Routine Description:

  Execute Bulk or SyncInterrupt Transfer

Arguments:

  HcDev            - USB_HC_DEV
  PtrTD            - TD_STRUCT
  wIndex           - No use
  ActualLen        - Actual transfered Len 
  DataToggle       - Data Toggle
  TimeOut          - TimeOut value in milliseconds
  TransferResult   - Transfer result
Returns:

  EFI_SUCCESS      - Sucess
  EFI_DEVICE_ERROR - Error
--*/

EFI_STATUS
InitializeMemoryManagement (
  IN USB_HC_DEV           *HcDev
  );

EFI_STATUS
CreateMemoryBlock (
  IN USB_HC_DEV            *HcDev,
  IN MEMORY_MANAGE_HEADER  **MemoryHeader,
  IN UINTN                 MemoryBlockSizeInPages
  );

EFI_STATUS
FreeMemoryHeader (
  IN USB_HC_DEV            *HcDev,
  IN MEMORY_MANAGE_HEADER  *MemoryHeader
  );

EFI_STATUS
UhciAllocatePool (
  IN USB_HC_DEV      *UhcDev,
  IN UINT8           **Pool,
  IN UINTN           AllocSize
  );

VOID
UhciFreePool (
  IN USB_HC_DEV      *HcDev,
  IN UINT8           *Pool,
  IN UINTN           AllocSize
  );

VOID
InsertMemoryHeaderToList (
  IN MEMORY_MANAGE_HEADER  *MemoryHeader,
  IN MEMORY_MANAGE_HEADER  *NewMemoryHeader
  );

EFI_STATUS
AllocMemInMemoryBlock (
  IN MEMORY_MANAGE_HEADER  *MemoryHeader,
  IN VOID                  **Pool,
  IN UINTN                 NumberOfMemoryUnit
  );

BOOLEAN
IsMemoryBlockEmptied (
  IN MEMORY_MANAGE_HEADER  *MemoryHeaderPtr
  );

VOID
DelinkMemoryBlock (
  IN MEMORY_MANAGE_HEADER    *FirstMemoryHeader,
  IN MEMORY_MANAGE_HEADER    *FreeMemoryHeader
  );

EFI_STATUS
DelMemoryManagement (
  IN USB_HC_DEV      *HcDev
  );

VOID
EnableMaxPacketSize (
  IN USB_HC_DEV          *HcDev
  );

VOID
CleanUsbTransactions (
  IN USB_HC_DEV    *HcDev
  );

VOID
TurnOffUSBEmulation (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  );

#endif
