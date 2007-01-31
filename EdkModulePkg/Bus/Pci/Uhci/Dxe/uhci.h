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

#include <IndustryStandard/pci22.h>

#define EFI_D_UHCI                EFI_D_INFO

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
#define INTERRUPT_POLLING_TIME    50 * 1000 * 10

//
// UHCI IO Space Address Register Register locates at
// offset 20 ~ 23h of PCI Configuration Space (UHCI spec, Revision 1.1),
// so, its BAR Index is 4.
//
#define USB_BAR_INDEX             4

//
// One memory block uses 1 page (common buffer for QH,TD use.)
//
#define NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES 1

#define bit(a)                            (1 << (a))

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
#define USBSTS          2       /* Status Register Offset 02-03h */
#define USBSTS_USBINT   bit (0) /* Interrupt due to IOC */
#define USBSTS_ERROR    bit (1) /* Interrupt due to error */
#define USBSTS_RD       bit (2) /* Resume Detect */
#define USBSTS_HSE      bit (3) /* Host System Error*/
#define USBSTS_HCPE     bit (4) /* Host Controller Process Error*/
#define USBSTS_HCH      bit (5) /* HC Halted */

/* Interrupt enable register */
#define USBINTR         4       /* Interrupt Enable Register 04-05h */
#define USBINTR_TIMEOUT bit (0) /* Timeout/CRC error enable */
#define USBINTR_RESUME  bit (1) /* Resume interrupt enable */
#define USBINTR_IOC     bit (2) /* Interrupt On Complete enable */
#define USBINTR_SP      bit (3) /* Short packet interrupt enable */

/* Frame Number Register Offset 06-08h */
#define USBFRNUM       6

/* Frame List Base Address Register Offset 08-0Bh */
#define USBFLBASEADD   8

/* Start of Frame Modify Register Offset 0Ch */
#define USBSOF         0x0c

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
#define CLASSC          0x09
//
// USB IO Space Base Address Register offset
//
#define USBBASE         0x20

//
// USB legacy Support
//
#define USB_EMULATION   0xc0

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
#define USB2_HC_DEV_FROM_THIS(a)  CR (a, USB_HC_DEV, Usb2Hc, USB_HC_DEV_SIGNATURE)

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
  EFI_USB2_HC_PROTOCOL      Usb2Hc;
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

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
UhciComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL     *This,
  IN  CHAR8                           *Language,
  OUT CHAR16                          **DriverName
  );

EFI_STATUS
EFIAPI
UhciComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL     *This,
  IN  EFI_HANDLE                      ControllerHandle,
  IN  EFI_HANDLE                      ChildHandle, OPTIONAL
  IN  CHAR8                           *Language,
  OUT CHAR16                          **ControllerName
  );

EFI_STATUS
WriteUHCCommandReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  CmdAddrOffset,
  IN UINT16                  UsbCmd
  )
/*++

Routine Description:

  Write UHCI Command Register

Arguments:

  PciIo         - EFI_PCI_IO_PROTOCOL
  CmdAddrOffset - Command address offset
  UsbCmd        - Data to write

Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
ReadUHCCommandReg (
  IN       EFI_PCI_IO_PROTOCOL     *PciIo,
  IN       UINT32                  CmdAddrOffset,
  IN OUT   UINT16                  *Data
  )
/*++

Routine Description:

  Read UHCI Command Register

Arguments:

  PciIo         - EFI_PCI_IO_PROTOCOL
  CmdAddrOffset - Command address offset
  Data          - Data to return

Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
WriteUHCStatusReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  StatusAddrOffset,
  IN UINT16                  UsbSts
  )
/*++

Routine Description:

  Write UHCI Staus Register

Arguments:

  PciIo            - EFI_PCI_IO_PROTOCOL
  StatusAddrOffset - Status address offset
  UsbSts           - Data to write

Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
ReadUHCStatusReg (
  IN       EFI_PCI_IO_PROTOCOL     *PciIo,
  IN       UINT32                  StatusAddrOffset,
  IN OUT   UINT16                  *Data
  )
/*++

Routine Description:

  Read UHCI Staus Register

Arguments:

  PciIo            - EFI_PCI_IO_PROTOCOL
  StatusAddrOffset - Status address offset
  UsbSts           - Data to return

Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
ClearStatusReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  StatusAddrOffset
  )
/*++

Routine Description:

  Clear the content of UHC's Status Register

Arguments:

  PciIo             - EFI_PCI_IO_PROTOCOL
  StatusAddrOffset  - Status address offset
  
Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
ReadUHCFrameNumberReg (
  IN       EFI_PCI_IO_PROTOCOL     *PciIo,
  IN       UINT32                  FrameNumAddrOffset,
  IN OUT   UINT16                  *Data
  )
/*++

Routine Description:

  Read from UHC's Frame Number Register

Arguments:

  PciIo              - EFI_PCI_IO_PROTOCOL
  FrameNumAddrOffset - Frame number register offset
  Data               - Data to return 
Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
WriteUHCFrameListBaseReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  FlBaseAddrOffset,
  IN UINT32                  UsbFrameListBaseAddr
  )
/*++

Routine Description:

  Write to UHC's Frame List Base Register

Arguments:

  PciIo                - EFI_PCI_IO_PROTOCOL
  FlBaseAddrOffset     - Frame Base address register
  UsbFrameListBaseAddr - Address to write

Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
ReadRootPortReg (
  IN       EFI_PCI_IO_PROTOCOL     *PciIo,
  IN       UINT32                  PortAddrOffset,
  IN OUT   UINT16                  *Data
  )
/*++

Routine Description:

  Read from UHC's Root Port Register

Arguments:

  PciIo           - EFI_PCI_IO_PROTOCOL
  PortAddrOffset  - Port Addrress Offset,
  Data            - Data to return
Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
WriteRootPortReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  PortAddrOffset,
  IN UINT16                  ControlBits
  )
/*++

Routine Description:

   Write to UHC's Root Port Register

Arguments:

  PciIo           - EFI_PCI_IO_PROTOCOL
  PortAddrOffset  - Port Addrress Offset,
  ControlBits     - Data to write
Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
WaitForUHCHalt (
  IN EFI_PCI_IO_PROTOCOL      *PciIo,
  IN UINT32                   StatusRegAddr,
  IN UINTN                    Timeout
  )
/*++

Routine Description:

  Wait until UHCI halt or timeout

Arguments:

  PciIo         - EFI_PCI_IO_PROTOCOL
  StatusRegAddr - Status Register Address
  Timeout       - Time out value in us

Returns:

  EFI_DEVICE_ERROR - Unable to read the status register
  EFI_TIMEOUT      - Time out
  EFI_SUCCESS      - Success

--*/
;

BOOLEAN
IsStatusOK (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  StatusRegAddr
  )
/*++

Routine Description:

  Judge whether the host controller operates well

Arguments:

  PciIo         - EFI_PCI_IO_PROTOCOL
  StatusRegAddr - Status register address

Returns:

   TRUE  -  Status is good
   FALSE -  Status is bad

--*/
;

BOOLEAN
IsHostSysOrProcessErr (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  StatusRegAddr
  )
/*++

Routine Description:

  Judge the status is HostSys,ProcessErr error or good

Arguments:

  PciIo         - EFI_PCI_IO_PROTOCOL
  StatusRegAddr - Status register address

Returns:

   TRUE  -  Status is good
   FALSE -  Status is bad

--*/
;

UINT16
GetCurrentFrameNumber (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  FrameNumAddrOffset
  )
/*++

Routine Description:

  Get Current Frame Number

Arguments:

  PciIo               - EFI_PCI_IO_PROTOCOL
  FrameNumAddrOffset  - FrameNum register AddrOffset

Returns:

  Frame number 

--*/
;

EFI_STATUS
SetFrameListBaseAddress (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  FLBASEADDRReg,
  IN UINT32                  Addr
  )
/*++

Routine Description:

  Set FrameListBase Address

Arguments:

  PciIo         - EFI_PCI_IO_PROTOCOL
  FlBaseAddrReg - FrameListBase register
  Addr          - Address to set

Returns:

  EFI_SUCCESS

--*/
;

UINT32
GetFrameListBaseAddress (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  FLBAddr
  )
/*++

Routine Description:

  Get Current Frame Number

Arguments:

  PciIo               - EFI_PCI_IO_PROTOCOL
  FrameNumAddrOffset  - FrameNum register AddrOffset

Returns:

  Frame number 

--*/
;

EFI_STATUS
CreateFrameList (
  IN USB_HC_DEV     *HcDev,
  IN UINT32         FLBASEADDRReg
  )
/*++

Routine Description:

  CreateFrameList

Arguments:

  HcDev         - USB_HC_DEV
  FlBaseAddrReg - Frame List register

Returns:

  EFI_OUT_OF_RESOURCES - Can't allocate memory resources
  EFI_UNSUPPORTED      - Map memory fail
  EFI_SUCCESS          - Success

--*/
;

EFI_STATUS
FreeFrameListEntry (
  IN USB_HC_DEV     *UhcDev
  )
/*++

Routine Description:

  Free FrameList buffer

Arguments:

  HcDev - USB_HC_DEV

Returns:

  EFI_SUCCESS - success

--*/
;

VOID
InitFrameList (
  IN USB_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Initialize FrameList

Arguments:

  HcDev - USB_HC_DEV

Returns:
   VOID

--*/
;

EFI_STATUS
CreateQH (
  IN  USB_HC_DEV     *HcDev,
  OUT QH_STRUCT      **pptrQH
  )
/*++

Routine Description:

  CreateQH

Arguments:

  HcDev       - USB_HC_DEV
  pptrQH      - QH_STRUCT content to return
Returns:

  EFI_SUCCESS          - Success
  EFI_OUT_OF_RESOURCES - Can't allocate memory
  
--*/
;

VOID
SetQHHorizontalLinkPtr (
  IN QH_STRUCT     *ptrQH,
  IN VOID          *ptrNext
  )
/*++

Routine Description:

  Set QH Horizontal Link Pointer

Arguments:

  PtrQH   - QH_STRUCT
  ptrNext - Data to write 

Returns:

  VOID

--*/
;

VOID                                *
GetQHHorizontalLinkPtr (
  IN QH_STRUCT     *ptrQH
  )
/*++

Routine Description:

  Get QH Horizontal Link Pointer

Arguments:

  PtrQH   - QH_STRUCT
  

Returns:

  Data to return 

--*/
;

VOID
SetQHHorizontalQHorTDSelect (
  IN QH_STRUCT     *ptrQH,
  IN BOOLEAN       bQH
  )
/*++

Routine Description:

  Set QH Horizontal QH or TD 

Arguments:

  PtrQH - QH_STRUCT
  bQH   - TRUE is QH FALSE is TD

Returns:
  VOID

--*/
;

VOID
SetQHHorizontalValidorInvalid (
  IN QH_STRUCT     *ptrQH,
  IN BOOLEAN       bValid
  )
/*++

Routine Description:

  Set QH Horizontal Valid or Invalid

Arguments:

  PtrQH  - QH_STRUCT
  bValid - TRUE is Valid FALSE is Invalid

Returns:
  VOID

--*/
;

VOID
SetQHVerticalLinkPtr (
  IN QH_STRUCT     *ptrQH,
  IN VOID          *ptrNext
  )
/*++

Routine Description:

  Set QH Vertical Link Pointer
  
Arguments:

  PtrQH   - QH_STRUCT
  ptrNext - Data to write
Returns:

  VOID

--*/
;

VOID                                *
GetQHVerticalLinkPtr (
  IN QH_STRUCT     *ptrQH
  )
/*++

Routine Description:

  Get QH Vertical Link Pointer
  
Arguments:

  PtrQH   - QH_STRUCT
 
Returns:

   Data to return

--*/
;

VOID
SetQHVerticalQHorTDSelect (
  IN QH_STRUCT     *ptrQH,
  IN BOOLEAN       bQH
  )
/*++

Routine Description:

  Set QH Vertical QH or TD

Arguments:

  PtrQH - QH_STRUCT
  bQH   - TRUE is QH FALSE is TD

Returns:

  VOID

--*/
;

BOOLEAN
IsQHHorizontalQHSelect (
  IN QH_STRUCT     *ptrQH
  )
/*++

Routine Description:

  Is QH Horizontal QH Select

Arguments:

  PtrQH - QH_STRUCT
 
Returns:

  TRUE  - QH
  FALSE - TD

--*/
;

VOID
SetQHVerticalValidorInvalid (
  IN QH_STRUCT     *ptrQH,
  IN BOOLEAN       bValid
  )
/*++

Routine Description:

  Set QH Vertical Valid or Invalid

Arguments:

  PtrQH   - QH_STRUCT
  IsValid - TRUE is valid FALSE is invalid

Returns:

  VOID

--*/
;

BOOLEAN
GetQHVerticalValidorInvalid (
  IN QH_STRUCT     *ptrQH
  )
/*++

Routine Description:

  Get QH Vertical Valid or Invalid

Arguments:

  PtrQH - QH_STRUCT

Returns:

  TRUE  - Valid
  FALSE - Invalid

--*/
;

EFI_STATUS
AllocateTDStruct (
  IN  USB_HC_DEV     *HcDev,
  OUT TD_STRUCT      **ppTDStruct
  )
/*++

Routine Description:

  Allocate TD Struct

Arguments:

  HcDev       - USB_HC_DEV
  ppTDStruct  - place to store TD_STRUCT pointer
Returns:

  EFI_SUCCESS

--*/
;

EFI_STATUS
CreateTD (
  IN  USB_HC_DEV     *HcDev,
  OUT TD_STRUCT      **pptrTD
  )
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
;


EFI_STATUS
GenSetupStageTD (
  IN  USB_HC_DEV     *HcDev,
  IN  UINT8          DevAddr,
  IN  UINT8          Endpoint,
  IN  BOOLEAN        bSlow,
  IN  UINT8          *pDevReq,
  IN  UINT8          RequestLen,
  OUT TD_STRUCT      **ppTD
  )
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
;

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
  )
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
;

EFI_STATUS
CreateStatusTD (
  IN  USB_HC_DEV     *HcDev,
  IN  UINT8          DevAddr,
  IN  UINT8          Endpoint,
  IN  UINT8          PktID,
  IN  BOOLEAN        bSlow,
  OUT TD_STRUCT      **ppTD
  )
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
;

VOID
SetTDLinkPtrValidorInvalid (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bValid
  )
/*++

Routine Description:

  Set TD Link Pointer Valid or Invalid

Arguments:

  ptrTDStruct - TD_STRUCT
  bValid      - TRUE is valid FALSE is invalid

Returns:

  VOID

--*/
;

VOID
SetTDLinkPtrQHorTDSelect (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bQH
  )
/*++

Routine Description:

  Set TD Link Pointer QH or TD Select

Arguments:

  ptrTDStruct - TD_STRUCT
  bQH         -  TRUE is QH FALSE is TD
  
Returns:

  VOID

--*/
;

VOID
SetTDLinkPtrDepthorBreadth (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bDepth
  )
/*++

Routine Description:

  Set TD Link Pointer depth or bread priority

Arguments:

  ptrTDStruct - TD_STRUCT
  bDepth      -  TRUE is Depth  FALSE is Breadth
  
Returns:

  VOID

--*/
;

VOID
SetTDLinkPtr (
  IN TD_STRUCT     *ptrTDStruct,
  IN VOID          *ptrNext
  )
/*++

Routine Description:

  Set TD Link Pointer

Arguments:

  ptrTDStruct - TD_STRUCT
  ptrNext     - Pointer to set
  
Returns:

  VOID

--*/
;

VOID                                *
GetTDLinkPtr (
  IN TD_STRUCT   *ptrTDStruct
  )
/*++

Routine Description:

  Get TD Link Pointer

Arguments:

  ptrTDStruct - TD_STRUCT
   
Returns:

  Pointer to get

--*/
;

VOID
EnableorDisableTDShortPacket (
  IN TD_STRUCT   *ptrTDStruct,
  IN BOOLEAN     bEnable
  )
/*++

Routine Description:

  Enable or Disable TD ShortPacket

Arguments:

  ptrTDStruct - TD_STRUCT
  bEnable     - TRUE is Enanble FALSE is Disable

Returns:

  VOID

--*/
;

VOID
SetTDControlErrorCounter (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINT8         nMaxErrors
  )
/*++

Routine Description:

  Set TD Control ErrorCounter

Arguments:

  ptrTDStruct - TD_STRUCT
  nMaxErrors  - Error counter number
  
Returns:

  VOID

--*/
;

VOID
SetTDLoworFullSpeedDevice (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bLowSpeedDevice
  )
/*++

Routine Description:

  Set TD status low speed or full speed

Arguments:

  ptrTDStruct     - A point to TD_STRUCT
  bLowSpeedDevice - Show low speed or full speed

Returns:

  VOID

--*/
;

VOID
SetTDControlIsochronousorNot (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bIsochronous
  )
/*++

Routine Description:

  Set TD status Isochronous or not
  
Arguments:

  ptrTDStruct   - A point to TD_STRUCT
  IsIsochronous - Show Isochronous or not

Returns:

  VOID

--*/
;

VOID
SetorClearTDControlIOC (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bSet
  )
/*++

Routine Description:

  Set TD status IOC IsSet

Arguments:

  ptrTDStruct - A point to TD_STRUCT
  IsSet       - Show IOC set or not

Returns:

  VOID

--*/
;

VOID
SetTDStatusActiveorInactive (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bActive
  )
/*++

Routine Description:

  Set TD status active or not
Arguments:

  ptrTDStruct - A point to TD_STRUCT
  IsActive    - Active or not

Returns:

  VOID

--*/
;

UINT16
SetTDTokenMaxLength (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINT16        nMaxLen
  )
/*++

Routine Description:

  Set TD Token maxlength

Arguments:

  ptrTDStruct   - A point to TD_STRUCT
  MaximumLength - Maximum length of TD Token

Returns:

  Real maximum length set to TD Token

--*/
;

VOID
SetTDTokenDataToggle1 (
  IN TD_STRUCT    *ptrTDStruct
  )
/*++

Routine Description:

  Set TD Token data toggle1

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  VOID

--*/
;

VOID
SetTDTokenDataToggle0 (
  IN TD_STRUCT    *ptrTDStruct
  )
/*++

Routine Description:

  Set TD Token data toggle0

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  VOID

--*/
;

UINT8
GetTDTokenDataToggle (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Get TD Token data toggle

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  data toggle value

--*/
;

VOID
SetTDTokenEndPoint (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINTN         nEndPoint
  )
/*++

Routine Description:

  Set Data Token endpoint number

Arguments:

  ptrTDStruct - A point to TD_STRUCT
  EndPoint    - End point number

Returns:

  VOID

--*/
;

VOID
SetTDTokenDeviceAddress (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINTN         nDevAddr
  )
/*++

Routine Description:

  Set TD Token device address

Arguments:

  ptrTDStruct   - A point to TD_STRUCT
  DeviceAddress - Device address

Returns:

  VOID
  
--*/
;

VOID
SetTDTokenPacketID (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINT8         nPID
  )
/*++

Routine Description:

  Set TD Token packet ID

Arguments:

  ptrTDStruct - A point to TD_STRUCT
  PID         - Packet ID

Returns:

  VOID

--*/
;

VOID
SetTDDataBuffer (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Set TD data buffer

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  VOID

--*/
;

BOOLEAN
IsTDStatusActive (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Indicate whether TD status active or not

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  TRUE  - Active
  FALSE - Inactive 

--*/
;

BOOLEAN
IsTDStatusStalled (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Indicate whether TD status stalled or not

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  TRUE  - Stalled
  FALSE - not stalled

--*/
;

BOOLEAN
IsTDStatusBufferError (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Indicate whether TD status buffer error or not

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  TRUE  - Buffer error
  FALSE - No error

--*/
;

BOOLEAN
IsTDStatusBabbleError (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Indicate whether TD status babble error or not

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  TRUE  - Babble error
  FALSE - No error

--*/
;

BOOLEAN
IsTDStatusNAKReceived (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Indicate whether TD status NAK received
Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  TRUE  - NAK received
  FALSE - NAK not received

--*/
;

BOOLEAN
IsTDStatusCRCTimeOutError (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Indicate whether TD status CRC timeout error or not

Arguments:

  ptrTDStruct - A point to TD_STRUCT
  
Returns:

  TRUE  - CRC timeout error
  FALSE - CRC timeout no error

--*/
;

BOOLEAN
IsTDStatusBitStuffError (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Indicate whether TD status bit stuff error or not

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  TRUE  - Bit stuff error
  FALSE - Bit stuff no error

--*/
;

UINT16
GetTDStatusActualLength (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Get TD status length

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  Return Td status length

--*/
;

UINT16
GetTDTokenMaxLength (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Get TD Token maximum length

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  Return TD token maximum length

--*/
;

UINT8
GetTDTokenEndPoint (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Get TD Token endpoint number

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  Return TD Token endpoint number

--*/
;

UINT8
GetTDTokenDeviceAddress (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Get TD Token device address

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  Return TD Token device address

--*/
;

UINT8
GetTDTokenPacketID (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Get TD Token packet ID

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  Return TD Token packet ID

--*/
;

UINT8                               *
GetTDDataBuffer (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Get the point to TD data buffer

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  Return a point to TD data buffer

--*/
;

BOOLEAN
GetTDLinkPtrValidorInvalid (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Get TD LinkPtr valid or not

Arguments:

  ptrTDStruct - A point to TD_STRUCT

Returns:

  TRUE  - Invalid
  FALSE - Valid

--*/
;

UINTN
CountTDsNumber (
  IN TD_STRUCT     *ptrFirstTD
  )
/*++

Routine Description:

  Get the number of TDs

Arguments:

  PtrFirstTD  - A point to the first TD_STRUCT

Returns:

  Return the number of TDs

--*/
;

VOID
LinkTDToQH (
  IN QH_STRUCT     *ptrQH,
  IN TD_STRUCT     *ptrTD
  )
/*++

Routine Description:

  Link TD To QH

Arguments:

  PtrQH - QH_STRUCT
  PtrTD - TD_STRUCT
Returns:

  VOID

--*/
;

VOID
LinkTDToTD (
  IN TD_STRUCT     *ptrPreTD,
  IN TD_STRUCT     *ptrTD
  )
/*++

Routine Description:

  Link TD To TD

Arguments:

  ptrPreTD - Previous TD_STRUCT to be linked
  PtrTD    - TD_STRUCT to link
Returns:

  VOID

--*/
;

VOID
SetorClearCurFrameListTerminate (
  IN FRAMELIST_ENTRY     *pCurEntry,
  IN BOOLEAN             bSet
  )
/*++

Routine Description:

  Set or clear current framelist terminate

Arguments:

  pCurEntry - A point to FRAMELIST_ENTITY
  IsSet     - TRUE to empty the frame and indicate the Pointer field is valid

Returns:

  VOID

--*/
;

VOID
SetCurFrameListQHorTD (
  IN FRAMELIST_ENTRY        *pCurEntry,
  IN BOOLEAN                bQH
  )
/*++

Routine Description:

  Set current framelist QH or TD

Arguments:

  pCurEntry - A point to FRAMELIST_ENTITY
  IsQH      - TRUE to set QH and FALSE to set TD

Returns:

  VOID

--*/
;

BOOLEAN
GetCurFrameListTerminate (
  IN FRAMELIST_ENTRY     *pCurEntry
  )
/*++

Routine Description:

  Get current framelist terminate

Arguments:

  pCurEntry - A point to FRAMELIST_ENTITY

Returns:

  TRUE  - Terminate
  FALSE - Not terminate

--*/
;

VOID
SetCurFrameListPointer (
  IN FRAMELIST_ENTRY     *pCurEntry,
  IN UINT8               *ptr
  )
/*++

Routine Description:

  Set current framelist pointer

Arguments:

  pCurEntry - A point to FRAMELIST_ENTITY
  ptr       - A point to FrameListPtr point to

Returns:

  VOID
  
--*/
;

VOID                                *
GetCurFrameListPointer (
  IN FRAMELIST_ENTRY     *pCurEntry
  )
/*++

Routine Description:

  Get current framelist pointer

Arguments:

  pCurEntry - A point to FRAMELIST_ENTITY

Returns:

  A point FrameListPtr point to

--*/
;

VOID
LinkQHToFrameList (
  IN FRAMELIST_ENTRY   *pEntry,
  IN UINT16            FrameListIndex,
  IN QH_STRUCT         *ptrQH
  )
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
;

VOID
DelLinkSingleQH (
  IN USB_HC_DEV      *HcDev,
  IN QH_STRUCT       *ptrQH,
  IN UINT16          FrameListIndex,
  IN BOOLEAN         SearchOther,
  IN BOOLEAN         Delete
  )
/*++

Routine Description:

  Unlink from frame list and delete single QH
  
Arguments:

  HcDev            - USB_HC_DEV
  PtrQH            - QH_STRUCT
  FrameListIndex   - Frame List Index
  SearchOther      - Search Other QH
  Delete           - TRUE is to delete the QH
  
Returns:

  VOID
  
--*/
;

VOID
DeleteQueuedTDs (
  IN USB_HC_DEV      *HcDev,
  IN TD_STRUCT       *ptrFirstTD
  )
/*++

Routine Description:

  Delete Queued TDs
  
Arguments:

  HcDev       - USB_HC_DEV
  PtrFirstTD  - TD link list head

Returns:

  VOID

--*/
;

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
  )
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
;

EFI_STATUS
DeleteAsyncINTQHTDs (
  IN  USB_HC_DEV  *HcDev,
  IN  UINT8       DeviceAddress,
  IN  UINT8       EndPointAddress,
  OUT UINT8       *DataToggle
  )
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
;

BOOLEAN
CheckTDsResults (
  IN  TD_STRUCT               *ptrTD,
  IN  UINTN                   RequiredLen,
  OUT UINT32                  *Result,
  OUT UINTN                   *ErrTDPos,
  OUT UINTN                   *ActualTransferSize
  )
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
;

VOID
ExecuteAsyncINTTDs (
  IN  USB_HC_DEV      *HcDev,
  IN  INTERRUPT_LIST  *ptrList,
  OUT UINT32          *Result,
  OUT UINTN           *ErrTDPos,
  OUT UINTN           *ActualLen
  )
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
;

VOID
UpdateAsyncINTQHTDs (
  IN INTERRUPT_LIST  *ptrList,
  IN UINT32          Result,
  IN UINT32          ErrTDPos
  )
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
;

VOID
ReleaseInterruptList (
  IN USB_HC_DEV      *HcDev,
  IN LIST_ENTRY      *ListHead
  )
/*++

Routine Description:

  Release Interrupt List
  
Arguments:

  HcDev     - USB_HC_DEV
  ListHead  - List head

Returns:

  VOID

--*/
;

EFI_STATUS
ExecuteControlTransfer (
  IN  USB_HC_DEV  *HcDev,
  IN  TD_STRUCT   *ptrTD,
  IN  UINT32      wIndex,
  OUT UINTN       *ActualLen,
  IN  UINTN       TimeOut,
  OUT UINT32      *TransferResult
  )
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
;

EFI_STATUS
ExecBulkorSyncInterruptTransfer (
  IN  USB_HC_DEV  *HcDev,
  IN  TD_STRUCT   *ptrTD,
  IN  UINT32      wIndex,
  OUT UINTN       *ActualLen,
  OUT UINT8       *DataToggle,
  IN  UINTN       TimeOut,
  OUT UINT32      *TransferResult
  )
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
;

EFI_STATUS
InitializeMemoryManagement (
  IN USB_HC_DEV           *HcDev
  )
/*++

Routine Description:

  Initialize Memory Management

Arguments:

  HcDev - USB_HC_DEV

Returns:

  EFI_SUCCESS -  Success
  
--*/
;

EFI_STATUS
CreateMemoryBlock (
  IN USB_HC_DEV            *HcDev,
  IN MEMORY_MANAGE_HEADER  **MemoryHeader,
  IN UINTN                 MemoryBlockSizeInPages
  )
/*++

Routine Description:

  Use PciIo->AllocateBuffer to allocate common buffer for the memory block,
  and use PciIo->Map to map the common buffer for Bus Master Read/Write.


Arguments:

  HcDev        - USB_HC_DEV
  MemoryHeader - MEMORY_MANAGE_HEADER to output
  MemoryBlockSizeInPages - MemoryBlockSizeInPages
  
Returns:

  EFI_SUCCESS            - Success
  EFI_OUT_OF_RESOURCES   - Out of resources
  EFI_UNSUPPORTED        - Unsupported

--*/
;

EFI_STATUS
FreeMemoryHeader (
  IN USB_HC_DEV            *HcDev,
  IN MEMORY_MANAGE_HEADER  *MemoryHeader
  )
/*++

Routine Description:

  Free Memory Header

Arguments:

  HcDev         - USB_HC_DEV
  MemoryHeader  - MemoryHeader to be freed

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success

--*/
;

EFI_STATUS
UhciAllocatePool (
  IN USB_HC_DEV      *UhcDev,
  IN UINT8           **Pool,
  IN UINTN           AllocSize
  )
/*++

Routine Description:

  Uhci Allocate Pool

Arguments:

  HcDev     - USB_HC_DEV
  Pool      - Place to store pointer to the memory buffer
  AllocSize - Alloc Size

Returns:

  EFI_SUCCESS - Success

--*/
;

VOID
UhciFreePool (
  IN USB_HC_DEV      *HcDev,
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

VOID
InsertMemoryHeaderToList (
  IN MEMORY_MANAGE_HEADER  *MemoryHeader,
  IN MEMORY_MANAGE_HEADER  *NewMemoryHeader
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
  IN MEMORY_MANAGE_HEADER  *MemoryHeader,
  IN VOID                  **Pool,
  IN UINTN                 NumberOfMemoryUnit
  )
/*++

Routine Description:

  Alloc Memory In MemoryBlock

Arguments:

  MemoryHeader        - MEMORY_MANAGE_HEADER
  Pool                - Place to store pointer to memory
  NumberOfMemoryUnit  - Number Of Memory Unit

Returns:

  EFI_NOT_FOUND  - Can't find the free memory 
  EFI_SUCCESS    - Success

--*/
;

BOOLEAN
IsMemoryBlockEmptied (
  IN MEMORY_MANAGE_HEADER  *MemoryHeaderPtr
  )
/*++

Routine Description:

  Is Memory Block Emptied

Arguments:

  MemoryHeaderPtr - MEMORY_MANAGE_HEADER

Returns:

  TRUE  - Empty
  FALSE - Not Empty 

--*/
;

VOID
DelinkMemoryBlock (
  IN MEMORY_MANAGE_HEADER    *FirstMemoryHeader,
  IN MEMORY_MANAGE_HEADER    *FreeMemoryHeader
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
DelMemoryManagement (
  IN USB_HC_DEV      *HcDev
  )
/*++

Routine Description:

  Delete Memory Management

Arguments:

  HcDev - USB_HC_DEV

Returns:

  EFI_SUCCESS - Success

--*/
;

VOID
EnableMaxPacketSize (
  IN USB_HC_DEV          *HcDev
  )
/*++

Routine Description:

  Enable Max Packet Size

Arguments:

  HcDev - USB_HC_DEV

Returns:

  VOID

--*/
;

VOID
CleanUsbTransactions (
  IN USB_HC_DEV    *HcDev
  )
/*++

Routine Description:

  Clean USB Transactions

Arguments:

  HcDev - A point to USB_HC_DEV

Returns:

  VOID

--*/
;

VOID
TurnOffUSBEmulation (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  )
/*++

Routine Description:

  Set current framelist QH or TD

Arguments:

  pCurEntry - A point to FRAMELIST_ENTITY
  IsQH      - TRUE to set QH and FALSE to set TD

Returns:

  VOID

--*/
;

//
// Prototypes
// Driver model protocol interface
//

EFI_STATUS
EFIAPI
UHCIDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UHCIDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UHCIDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  );

//
// UHCI interface functions
//

EFI_STATUS
EFIAPI
UHCIReset (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  UINT16                  Attributes
  );

EFI_STATUS
EFIAPI
UHCIGetState (
  IN  EFI_USB_HC_PROTOCOL     *This,
  OUT EFI_USB_HC_STATE        *State
  );

EFI_STATUS
EFIAPI
UHCISetState (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  EFI_USB_HC_STATE        State
  );

EFI_STATUS
EFIAPI
UHCIControlTransfer (
  IN       EFI_USB_HC_PROTOCOL        *This,
  IN       UINT8                      DeviceAddress,
  IN       BOOLEAN                    IsSlowDevice,
  IN       UINT8                      MaximumPacketLength,
  IN       EFI_USB_DEVICE_REQUEST     *Request,
  IN       EFI_USB_DATA_DIRECTION     TransferDirection,
  IN OUT   VOID                       *Data, OPTIONAL
  IN OUT   UINTN                      *DataLength, OPTIONAL
  IN       UINTN                      TimeOut,
  OUT      UINT32                     *TransferResult
  );

EFI_STATUS
EFIAPI
UHCIBulkTransfer (
  IN       EFI_USB_HC_PROTOCOL     *This,
  IN       UINT8                   DeviceAddress,
  IN       UINT8                   EndPointAddress,
  IN       UINT8                   MaximumPacketLength,
  IN OUT   VOID                    *Data,
  IN OUT   UINTN                   *DataLength,
  IN OUT   UINT8                   *DataToggle,
  IN       UINTN                   TimeOut,
  OUT      UINT32                  *TransferResult
  );

EFI_STATUS
EFIAPI
UHCIAsyncInterruptTransfer (
  IN       EFI_USB_HC_PROTOCOL                * This,
  IN       UINT8                              DeviceAddress,
  IN       UINT8                              EndPointAddress,
  IN       BOOLEAN                            IsSlowDevice,
  IN       UINT8                              MaximumPacketLength,
  IN       BOOLEAN                            IsNewTransfer,
  IN OUT   UINT8                              *DataToggle,
  IN       UINTN                              PollingInterval, OPTIONAL
  IN       UINTN                              DataLength, OPTIONAL
  IN       EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction, OPTIONAL
  IN       VOID                               *Context OPTIONAL
  );

EFI_STATUS
EFIAPI
UHCISyncInterruptTransfer (
  IN       EFI_USB_HC_PROTOCOL     *This,
  IN       UINT8                   DeviceAddress,
  IN       UINT8                   EndPointAddress,
  IN       BOOLEAN                 IsSlowDevice,
  IN       UINT8                   MaximumPacketLength,
  IN OUT   VOID                    *Data,
  IN OUT   UINTN                   *DataLength,
  IN OUT   UINT8                   *DataToggle,
  IN       UINTN                   TimeOut,
  OUT      UINT32                  *TransferResult
  );

EFI_STATUS
EFIAPI
UHCIIsochronousTransfer (
  IN       EFI_USB_HC_PROTOCOL     *This,
  IN       UINT8                   DeviceAddress,
  IN       UINT8                   EndPointAddress,
  IN       UINT8                   MaximumPacketLength,
  IN OUT   VOID                    *Data,
  IN       UINTN                   DataLength,
  OUT      UINT32                  *TransferResult
  );

EFI_STATUS
EFIAPI
UHCIAsyncIsochronousTransfer (
  IN       EFI_USB_HC_PROTOCOL                 * This,
  IN       UINT8                               DeviceAddress,
  IN       UINT8                               EndPointAddress,
  IN       UINT8                               MaximumPacketLength,
  IN OUT   VOID                                *Data,
  IN       UINTN                               DataLength,
  IN       EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN       VOID                                *Context OPTIONAL
  );

EFI_STATUS
EFIAPI
UHCIGetRootHubPortNumber (
  IN  EFI_USB_HC_PROTOCOL     *This,
  OUT UINT8                   *PortNumber
  );

EFI_STATUS
EFIAPI
UHCIGetRootHubPortStatus (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  UINT8                   PortNumber,
  OUT EFI_USB_PORT_STATUS     *PortStatus
  );

EFI_STATUS
EFIAPI
UHCISetRootHubPortFeature (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  UINT8                   PortNumber,
  IN  EFI_USB_PORT_FEATURE    PortFeature
  );

EFI_STATUS
EFIAPI
UHCIClearRootHubPortFeature (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  UINT8                   PortNumber,
  IN  EFI_USB_PORT_FEATURE    PortFeature
  );

//
// UEFI 2.0 Protocol
//

EFI_STATUS
EFIAPI
UHCI2GetCapability(
  IN  EFI_USB2_HC_PROTOCOL  * This,
  OUT UINT8                 *MaxSpeed,
  OUT UINT8                 *PortNumber,
  OUT UINT8                 *Is64BitCapable
  );

EFI_STATUS
EFIAPI
UHCI2Reset (
  IN EFI_USB2_HC_PROTOCOL   * This,
  IN UINT16                 Attributes
  );

EFI_STATUS
EFIAPI
UHCI2GetState (
  IN  EFI_USB2_HC_PROTOCOL   * This,
  OUT EFI_USB_HC_STATE       * State
  );

EFI_STATUS
EFIAPI
UHCI2SetState (
  IN EFI_USB2_HC_PROTOCOL   * This,
  IN EFI_USB_HC_STATE       State
  );

EFI_STATUS
EFIAPI
UHCI2ControlTransfer (
  IN     EFI_USB2_HC_PROTOCOL      * This,
  IN     UINT8                     DeviceAddress,
  IN     UINT8                     DeviceSpeed,
  IN     UINTN                     MaximumPacketLength,
  IN     EFI_USB_DEVICE_REQUEST    * Request,
  IN     EFI_USB_DATA_DIRECTION    TransferDirection,
  IN OUT VOID                      *Data, OPTIONAL
  IN OUT UINTN                     *DataLength, OPTIONAL
  IN     UINTN                     TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR            *Translator,
  OUT    UINT32                    *TransferResult
  );

EFI_STATUS
EFIAPI
UHCI2BulkTransfer (
  IN     EFI_USB2_HC_PROTOCOL   * This,
  IN     UINT8                  DeviceAddress,
  IN     UINT8                  EndPointAddress,
  IN     UINT8                  DeviceSpeed,
  IN     UINTN                  MaximumPacketLength,
  IN     UINT8                  DataBuffersNumber,
  IN OUT VOID                   *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN OUT UINTN                  *DataLength,
  IN OUT UINT8                  *DataToggle,
  IN     UINTN                  TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR            *Translator,
  OUT    UINT32                 *TransferResult
  );

EFI_STATUS
EFIAPI
UHCI2AsyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL   * This,
  IN     UINT8                  DeviceAddress,
  IN     UINT8                  EndPointAddress,
  IN     UINT8                  DeviceSpeed,
  IN     UINTN                  MaximumPacketLength,
  IN     BOOLEAN                IsNewTransfer,
  IN OUT UINT8                  *DataToggle,
  IN     UINTN                  PollingInterval, OPTIONAL
  IN     UINTN                  DataLength, OPTIONAL
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR            *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK               CallBackFunction, OPTIONAL
  IN     VOID                   *Context OPTIONAL
  );

EFI_STATUS
EFIAPI
UHCI2SyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL   * This,
  IN     UINT8                  DeviceAddress,
  IN     UINT8                  EndPointAddress,
  IN     UINT8                  DeviceSpeed,
  IN     UINTN                  MaximumPacketLength,
  IN OUT VOID                   *Data,
  IN OUT UINTN                  *DataLength,
  IN OUT UINT8                  *DataToggle,
  IN     UINTN                  TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR           *Translator,
  OUT    UINT32                 *TransferResult
  );

EFI_STATUS
EFIAPI
UHCI2IsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL   * This,
  IN     UINT8                  DeviceAddress,
  IN     UINT8                  EndPointAddress,
  IN     UINT8                  DeviceSpeed,
  IN     UINTN                  MaximumPacketLength,
  IN     UINT8                  DataBuffersNumber,
  IN OUT VOID                   *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                  DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR           *Translator,
  OUT    UINT32                 *TransferResult
  );

EFI_STATUS
EFIAPI
UHCI2AsyncIsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL   * This,
  IN     UINT8                  DeviceAddress,
  IN     UINT8                  EndPointAddress,
  IN     UINT8                  DeviceSpeed,
  IN     UINTN                  MaximumPacketLength,
  IN     UINT8                  DataBuffersNumber,
  IN OUT VOID                   *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                  DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR           *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK              IsochronousCallBack,
  IN     VOID                   *Context OPTIONAL
  );

EFI_STATUS
EFIAPI
UHCI2GetRootHubPortStatus (
  IN  EFI_USB2_HC_PROTOCOL   * This,
  IN  UINT8                  PortNumber,
  OUT EFI_USB_PORT_STATUS    * PortStatus
  );

EFI_STATUS
EFIAPI
UHCI2SetRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL    * This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  );

EFI_STATUS
EFIAPI
UHCI2ClearRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL    * This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  );

//
// Asynchronous interrupt transfer monitor function
//
VOID
EFIAPI
MonitorInterruptTrans (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  );

#endif
