/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    UhcHlp.c
    
Abstract: 
    

Revision History
--*/

#include "uhci.h"

STATIC
EFI_STATUS
USBReadPortW (
  IN       EFI_PCI_IO_PROTOCOL     *PciIo,
  IN       UINT32                  PortOffset,
  IN OUT   UINT16                  *Data
  )
/*++

Routine Description:

  USBReadPort Word

Arguments:

  PciIo       - EFI_PCI_IO_PROTOCOL
  PortOffset  - Port offset
  Data        - Data to reutrn

Returns:

  EFI_SUCCESS

--*/
{
  //
  // Perform 16bit Read in PCI IO Space
  //
  return PciIo->Io.Read (
                     PciIo,
                     EfiPciIoWidthUint16,
                     USB_BAR_INDEX,
                     (UINT64) PortOffset,
                     1,
                     Data
                     );
}

STATIC
EFI_STATUS
USBWritePortW (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  PortOffset,
  IN UINT16                  Data
  )
/*++

Routine Description:

  USB Write Port Word

Arguments:

  PciIo       - EFI_PCI_IO_PROTOCOL
  PortOffset  - Port offset
  Data        - Data to write

Returns:

  EFI_SUCCESS

--*/
{
  //
  // Perform 16bit Write in PCI IO Space
  //
  return PciIo->Io.Write (
                     PciIo,
                     EfiPciIoWidthUint16,
                     USB_BAR_INDEX,
                     (UINT64) PortOffset,
                     1,
                     &Data
                     );
}

STATIC
EFI_STATUS
USBWritePortDW (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  PortOffset,
  IN UINT32                  Data
  )
/*++

Routine Description:

  USB Write Port DWord

Arguments:

  PciIo       - EFI_PCI_IO_PROTOCOL
  PortOffset  - Port offset
  Data        - Data to write

Returns:

  EFI_SUCCESS

--*/
{
  //
  // Perform 32bit Write in PCI IO Space
  //
  return PciIo->Io.Write (
                     PciIo,
                     EfiPciIoWidthUint32,
                     USB_BAR_INDEX,
                     (UINT64) PortOffset,
                     1,
                     &Data
                     );
}
//
//  USB register-base helper functions
//
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
{
  //
  // Write to UHC's Command Register
  //
  return USBWritePortW (PciIo, CmdAddrOffset, UsbCmd);
}

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
{
  //
  // Read from UHC's Command Register
  //
  return USBReadPortW (PciIo, CmdAddrOffset, Data);
}

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
{
  //
  // Write to UHC's Status Register
  //
  return USBWritePortW (PciIo, StatusAddrOffset, UsbSts);
}

EFI_STATUS
ReadUHCStatusReg (
  IN     EFI_PCI_IO_PROTOCOL     *PciIo,
  IN     UINT32                  StatusAddrOffset,
  IN OUT UINT16                  *Data
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
{
  //
  // Read from UHC's Status Register
  //
  return USBReadPortW (PciIo, StatusAddrOffset, Data);
}


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
{
 
  return WriteUHCStatusReg (PciIo, StatusAddrOffset, 0x003F);
}

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
{
  
  return USBReadPortW (PciIo, FrameNumAddrOffset, Data);
}

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
{
  
  return USBWritePortDW (PciIo, FlBaseAddrOffset, UsbFrameListBaseAddr);
}

EFI_STATUS
ReadRootPortReg (
  IN     EFI_PCI_IO_PROTOCOL     *PciIo,
  IN     UINT32                  PortAddrOffset,
  IN OUT UINT16                  *Data
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
{
 
  return USBReadPortW (PciIo, PortAddrOffset, Data);
}

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
{
 
  return USBWritePortW (PciIo, PortAddrOffset, ControlBits);
}



EFI_STATUS
WaitForUHCHalt (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  StatusRegAddr,
  IN UINTN                   Timeout
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
{
  UINTN       Delay;
  EFI_STATUS  Status;
  UINT16      HcStatus;

  //
  // Timeout is in us unit
  //
  Delay = (Timeout / 50) + 1;
  do {
    Status = ReadUHCStatusReg (PciIo, StatusRegAddr, &HcStatus);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    if ((HcStatus & USBSTS_HCH) == USBSTS_HCH) {
      break;
    }
    //
    // Stall for 50 us
    //
    gBS->Stall (50);

  } while (Delay--);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

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
{
  EFI_STATUS  Status;
  UINT16      HcStatus;
  //
  // Detect whether the interrupt is caused by fatal error.
  // see "UHCI Design Guid".
  //
  Status = ReadUHCStatusReg (PciIo, StatusRegAddr, &HcStatus);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (HcStatus & (USBSTS_HCPE | USBSTS_HSE | USBSTS_HCH)) {
    return FALSE;
  } else {
    return TRUE;
  }

}


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
{
  EFI_STATUS  Status;
  UINT16      HcStatus;
  //
  // Detect whether the interrupt is caused by serious error.
  // see "UHCI Design Guid".
  //
  Status = ReadUHCStatusReg (PciIo, StatusRegAddr, &HcStatus);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (HcStatus & (USBSTS_HSE | USBSTS_HCPE)) {
    return TRUE;
  } else {
    return FALSE;
  }
}


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
{
  //
  // Gets value in the USB frame number register.
  //
  UINT16  FrameNumber;

  ReadUHCFrameNumberReg (PciIo, FrameNumAddrOffset, &FrameNumber);

  return (UINT16) (FrameNumber & 0x03FF);
}

EFI_STATUS
SetFrameListBaseAddress (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  FlBaseAddrReg,
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
{
  //
  // Sets value in the USB Frame List Base Address register.
  //
  return WriteUHCFrameListBaseReg (PciIo, FlBaseAddrReg, (UINT32) (Addr & 0xFFFFF000));
}

VOID
EnableMaxPacketSize (
  IN USB_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Enable Max Packet Size

Arguments:

  HcDev - USB_HC_DEV

Returns:

  VOID

--*/
{
  UINT16      CommandContent;

  ReadUHCCommandReg (
    HcDev->PciIo,
    (UINT32) (USBCMD),
    &CommandContent
    );

  if ((CommandContent & USBCMD_MAXP) != USBCMD_MAXP) {
    CommandContent |= USBCMD_MAXP;
    WriteUHCCommandReg (
      HcDev->PciIo,
      (UINT32) (USBCMD),
      CommandContent
      );
  }

  return ;
}

EFI_STATUS
CreateFrameList (
  IN USB_HC_DEV     *HcDev,
  IN UINT32         FlBaseAddrReg
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
{
  EFI_STATUS            Status;
  VOID                  *CommonBuffer;
  EFI_PHYSICAL_ADDRESS  MappedAddress;
  VOID                  *Mapping;
  UINTN                 BufferSizeInPages;
  UINTN                 BufferSizeInBytes;

  //
  // The Frame List is a common buffer that will be
  // accessed by both the cpu and the usb bus master
  // at the same time.
  // The Frame List ocupies 4K bytes,
  // and must be aligned on 4-Kbyte boundaries.
  //
  BufferSizeInBytes = 4096;
  BufferSizeInPages = EFI_SIZE_TO_PAGES (BufferSizeInBytes);
  Status = HcDev->PciIo->AllocateBuffer (
                           HcDev->PciIo,
                           AllocateAnyPages,
                           EfiBootServicesData,
                           BufferSizeInPages,
                           &CommonBuffer,
                           0
                           );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = HcDev->PciIo->Map (
                           HcDev->PciIo,
                           EfiPciIoOperationBusMasterCommonBuffer,
                           CommonBuffer,
                           &BufferSizeInBytes,
                           &MappedAddress,
                           &Mapping
                           );
  if (EFI_ERROR (Status) || (BufferSizeInBytes != 4096)) {
    HcDev->PciIo->FreeBuffer (HcDev->PciIo, BufferSizeInPages, CommonBuffer);
    return EFI_UNSUPPORTED;
  }

  HcDev->FrameListEntry   = (FRAMELIST_ENTRY *) ((UINTN) MappedAddress);

  HcDev->FrameListMapping = Mapping;

  InitFrameList (HcDev);

  //
  // Tell the Host Controller where the Frame List lies,
  // by set the Frame List Base Address Register.
  //
  SetFrameListBaseAddress (
    HcDev->PciIo,
    FlBaseAddrReg,
    (UINT32) ((UINTN) HcDev->FrameListEntry)
    );

  return EFI_SUCCESS;
}

EFI_STATUS
FreeFrameListEntry (
  IN USB_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Free FrameList buffer

Arguments:

  HcDev - USB_HC_DEV

Returns:

  EFI_SUCCESS - success

--*/
{
  //
  // Unmap the common buffer for framelist entry,
  // and free the common buffer.
  // Uhci's frame list occupy 4k memory.
  //
  HcDev->PciIo->Unmap (HcDev->PciIo, HcDev->FrameListMapping);
  HcDev->PciIo->FreeBuffer (
                  HcDev->PciIo,
                  EFI_SIZE_TO_PAGES (4096),
                  (VOID *) (HcDev->FrameListEntry)
                  );
  return EFI_SUCCESS;
}

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
{
  FRAMELIST_ENTRY *FrameListPtr;
  UINTN           Index;

  //
  // Validate each Frame List Entry
  //
  FrameListPtr = HcDev->FrameListEntry;
  for (Index = 0; Index < 1024; Index++) {
    FrameListPtr->FrameListPtrTerminate = 1;
    FrameListPtr->FrameListPtr          = 0;
    FrameListPtr->FrameListPtrQSelect   = 0;
    FrameListPtr->FrameListRsvd         = 0;
    FrameListPtr++;
  }
}
//
// //////////////////////////////////////////////////////////////
//
// QH TD related Helper Functions
//
////////////////////////////////////////////////////////////////
//
// functions for QH
//
STATIC
EFI_STATUS
AllocateQHStruct (
  IN  USB_HC_DEV     *HcDev,
  OUT QH_STRUCT      **ppQHStruct
  )
/*++

Routine Description:

  Allocate QH Struct

Arguments:

  HcDev       - USB_HC_DEV
  ppQHStruct  - QH_STRUCT content to return
Returns:

  EFI_SUCCESS

--*/
{
  *ppQHStruct = NULL;

  //
  // QH must align on 16 bytes alignment,
  // since the memory allocated by UhciAllocatePool ()
  // is aligned on 32 bytes, it is no need to adjust
  // the allocated memory returned.
  //
  return UhciAllocatePool (HcDev, (UINT8 **) ppQHStruct, sizeof (QH_STRUCT));
}


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
  ppQHStruct  - QH_STRUCT content to return
Returns:

  EFI_SUCCESS          - Success
  EFI_OUT_OF_RESOURCES - Can't allocate memory
--*/
{
  EFI_STATUS  Status;

  //
  // allocate align memory for QH_STRUCT
  //
  Status = AllocateQHStruct (HcDev, pptrQH);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // init each field of the QH_STRUCT
  //
  //
  // Make QH ready
  //
  SetQHHorizontalValidorInvalid (*pptrQH, FALSE);
  SetQHVerticalValidorInvalid   (*pptrQH, FALSE);

  return EFI_SUCCESS;
}

VOID
SetQHHorizontalLinkPtr (
  IN QH_STRUCT     *PtrQH,
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
{
  //
  // Since the QH_STRUCT is aligned on 16-byte boundaries,
  // Only the highest 28bit of the address is valid
  // (take 32bit address as an example).
  //
  PtrQH->QH.QHHorizontalPtr = (UINT32) ((UINTN) ptrNext >> 4);
}

VOID *
GetQHHorizontalLinkPtr (
  IN QH_STRUCT     *PtrQH
  )
/*++

Routine Description:

  Get QH Horizontal Link Pointer

Arguments:

  PtrQH   - QH_STRUCT
  

Returns:

  Data to return 

--*/
{
  //
  // Restore the 28bit address to 32bit address
  // (take 32bit address as an example)
  //
  return (VOID *) ((UINTN) (PtrQH->QH.QHHorizontalPtr << 4));
}

VOID
SetQHHorizontalQHorTDSelect (
  IN QH_STRUCT     *PtrQH,
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
{
  //
  // if QH is connected, the specified bit is set,
  // if TD is connected, the specified bit is cleared.
  //
  PtrQH->QH.QHHorizontalQSelect = bQH ? 1 : 0;
}


VOID
SetQHHorizontalValidorInvalid (
  IN QH_STRUCT     *PtrQH,
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
{
  //
  // Valid means the horizontal link pointer is valid,
  // else, it's invalid.
  //
  PtrQH->QH.QHHorizontalTerminate = bValid ? 0 : 1;
}

VOID
SetQHVerticalLinkPtr (
  IN QH_STRUCT     *PtrQH,
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
{
  //
  // Since the QH_STRUCT is aligned on 16-byte boundaries,
  // Only the highest 28bit of the address is valid
  // (take 32bit address as an example).
  //
  PtrQH->QH.QHVerticalPtr = (UINT32) ((UINTN) ptrNext >> 4);
}

VOID *
GetQHVerticalLinkPtr (
  IN QH_STRUCT     *PtrQH
  )
/*++

Routine Description:

  Get QH Vertical Link Pointer
  
Arguments:

  PtrQH   - QH_STRUCT
 
Returns:

   Data to return

--*/
{
  //
  // Restore the 28bit address to 32bit address
  // (take 32bit address as an example)
  //
  return (VOID *) ((UINTN) (PtrQH->QH.QHVerticalPtr << 4));
}

VOID
SetQHVerticalQHorTDSelect (
  IN QH_STRUCT     *PtrQH,
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
{
  //
  // Set the specified bit if the Vertical Link Pointer pointing to a QH,
  // Clear the specified bit if the Vertical Link Pointer pointing to a TD.
  //
  PtrQH->QH.QHVerticalQSelect = bQH ? 1 : 0;
}

BOOLEAN
IsQHHorizontalQHSelect (
  IN QH_STRUCT     *PtrQH
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
{
  //
  // Retrieve the information about whether the Horizontal Link Pointer
  // pointing to a QH or TD.
  //
  return (BOOLEAN) (PtrQH->QH.QHHorizontalQSelect ? TRUE : FALSE);
}

VOID
SetQHVerticalValidorInvalid (
  IN QH_STRUCT     *PtrQH,
  IN BOOLEAN       IsValid
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
{
  //
  // If TRUE, indicates the Vertical Link Pointer field is valid,
  // else, the field is invalid.
  //
  PtrQH->QH.QHVerticalTerminate = IsValid ? 0 : 1;
}


BOOLEAN
GetQHVerticalValidorInvalid (
  IN QH_STRUCT     *PtrQH
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
{
  //
  // If TRUE, indicates the Vertical Link Pointer field is valid,
  // else, the field is invalid.
  //
  return (BOOLEAN) (!(PtrQH->QH.QHVerticalTerminate));
}

STATIC
BOOLEAN
GetQHHorizontalValidorInvalid (
  IN QH_STRUCT     *PtrQH
  )
/*++

Routine Description:

  Get QH Horizontal Valid or Invalid

Arguments:

  PtrQH - QH_STRUCT

Returns:

  TRUE  - Valid
  FALSE - Invalid

--*/
{
  //
  // If TRUE, meaning the Horizontal Link Pointer field is valid,
  // else, the field is invalid.
  //
  return (BOOLEAN) (!(PtrQH->QH.QHHorizontalTerminate));
}
//
// functions for TD
//
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
{
  *ppTDStruct = NULL;

  //
  // TD must align on 16 bytes alignment,
  // since the memory allocated by UhciAllocatePool ()
  // is aligned on 32 bytes, it is no need to adjust
  // the allocated memory returned.
  //
  return UhciAllocatePool (
          HcDev,
          (UINT8 **) ppTDStruct,
          sizeof (TD_STRUCT)
          );
}

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
{
  EFI_STATUS  Status;
  //
  // create memory for TD_STRUCT, and align the memory.
  //
  Status = AllocateTDStruct (HcDev, pptrTD);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Make TD ready.
  //
  SetTDLinkPtrValidorInvalid (*pptrTD, FALSE);


  return EFI_SUCCESS;
}

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
{
  EFI_STATUS  Status;
  TD_STRUCT   *pTDStruct;

  Status = CreateTD (HcDev, &pTDStruct);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetTDLinkPtr (pTDStruct, NULL);

  //
  // Depth first fashion
  //
  SetTDLinkPtrDepthorBreadth (pTDStruct, TRUE);

  //
  // initialize as the last TD in the QH context,
  // this field will be updated in the TD linkage process.
  //
  SetTDLinkPtrValidorInvalid (pTDStruct, FALSE);

  //
  // Disable Short Packet Detection by default
  //
  EnableorDisableTDShortPacket (pTDStruct, FALSE);

  //
  // Max error counter is 3, retry 3 times when error encountered.
  //
  SetTDControlErrorCounter (pTDStruct, 3);

  //
  // set device speed attribute
  // (TRUE - Slow Device; FALSE - Full Speed Device)
  //
  SetTDLoworFullSpeedDevice (pTDStruct, bSlow);

  //
  // Non isochronous transfer TD
  //
  SetTDControlIsochronousorNot (pTDStruct, FALSE);

  //
  // Interrupt On Complete bit be set to zero,
  // Disable IOC interrupt.
  //
  SetorClearTDControlIOC (pTDStruct, FALSE);

  //
  // Set TD Active bit
  //
  SetTDStatusActiveorInactive (pTDStruct, TRUE);

  SetTDTokenMaxLength (pTDStruct, RequestLen);

  SetTDTokenDataToggle0 (pTDStruct);

  SetTDTokenEndPoint (pTDStruct, Endpoint);

  SetTDTokenDeviceAddress (pTDStruct, DevAddr);

  SetTDTokenPacketID (pTDStruct, SETUP_PACKET_ID);

  pTDStruct->pTDBuffer      = (UINT8 *) pDevReq;
  pTDStruct->TDBufferLength = RequestLen;
  SetTDDataBuffer (pTDStruct);

  *ppTD = pTDStruct;

  return EFI_SUCCESS;
}

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
{
  TD_STRUCT   *pTDStruct;
  EFI_STATUS  Status;

  Status = CreateTD (HcDev, &pTDStruct);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetTDLinkPtr (pTDStruct, NULL);

  //
  // Depth first fashion
  //
  SetTDLinkPtrDepthorBreadth (pTDStruct, TRUE);

  //
  // Link pointer pointing to TD struct
  //
  SetTDLinkPtrQHorTDSelect (pTDStruct, FALSE);

  //
  // initialize as the last TD in the QH context,
  // this field will be updated in the TD linkage process.
  //
  SetTDLinkPtrValidorInvalid (pTDStruct, FALSE);

  //
  // Disable short packet detect
  //
  EnableorDisableTDShortPacket (pTDStruct, FALSE);
  //
  // Max error counter is 3
  //
  SetTDControlErrorCounter (pTDStruct, 3);

  //
  // set device speed attribute
  // (TRUE - Slow Device; FALSE - Full Speed Device)
  //
  SetTDLoworFullSpeedDevice (pTDStruct, bSlow);

  //
  // Non isochronous transfer TD
  //
  SetTDControlIsochronousorNot (pTDStruct, FALSE);

  //
  // Disable Interrupt On Complete
  // Disable IOC interrupt.
  //
  SetorClearTDControlIOC (pTDStruct, FALSE);

  //
  // Set Active bit
  //
  SetTDStatusActiveorInactive (pTDStruct, TRUE);

  SetTDTokenMaxLength (pTDStruct, Len);

  if (Toggle) {
    SetTDTokenDataToggle1 (pTDStruct);
  } else {
    SetTDTokenDataToggle0 (pTDStruct);
  }

  SetTDTokenEndPoint (pTDStruct, Endpoint);

  SetTDTokenDeviceAddress (pTDStruct, DevAddr);

  SetTDTokenPacketID (pTDStruct, PktID);

  pTDStruct->pTDBuffer      = (UINT8 *) pData;
  pTDStruct->TDBufferLength = Len;
  SetTDDataBuffer (pTDStruct);
  *ppTD = pTDStruct;

  return EFI_SUCCESS;
}


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

  Generate Status Stage TD

Arguments:

  HcDev       - USB_HC_DEV
  DevAddr     - Device address
  Endpoint    - Endpoint number 
  PktID       - Packet ID
  bSlow       - Full speed or low speed
  ppTD        - TD_STRUCT to return
Returns:

  EFI_OUT_OF_RESOURCES - Can't allocate memory
  EFI_SUCCESS          - Success

--*/
{
  TD_STRUCT   *ptrTDStruct;
  EFI_STATUS  Status;

  Status = CreateTD (HcDev, &ptrTDStruct);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetTDLinkPtr (ptrTDStruct, NULL);

  //
  // Depth first fashion
  //
  SetTDLinkPtrDepthorBreadth (ptrTDStruct, TRUE);

  //
  // initialize as the last TD in the QH context,
  // this field will be updated in the TD linkage process.
  //
  SetTDLinkPtrValidorInvalid (ptrTDStruct, FALSE);

  //
  // Disable short packet detect
  //
  EnableorDisableTDShortPacket (ptrTDStruct, FALSE);

  //
  // Max error counter is 3
  //
  SetTDControlErrorCounter (ptrTDStruct, 3);

  //
  // set device speed attribute
  // (TRUE - Slow Device; FALSE - Full Speed Device)
  //
  SetTDLoworFullSpeedDevice (ptrTDStruct, bSlow);

  //
  // Non isochronous transfer TD
  //
  SetTDControlIsochronousorNot (ptrTDStruct, FALSE);

  //
  // Disable Interrupt On Complete
  // Disable IOC interrupt.
  //
  SetorClearTDControlIOC (ptrTDStruct, FALSE);

  //
  // Set TD Active bit
  //
  SetTDStatusActiveorInactive (ptrTDStruct, TRUE);

  SetTDTokenMaxLength (ptrTDStruct, 0);

  SetTDTokenDataToggle1 (ptrTDStruct);

  SetTDTokenEndPoint (ptrTDStruct, Endpoint);

  SetTDTokenDeviceAddress (ptrTDStruct, DevAddr);

  SetTDTokenPacketID (ptrTDStruct, PktID);

  ptrTDStruct->pTDBuffer      = NULL;
  ptrTDStruct->TDBufferLength = 0;
  SetTDDataBuffer (ptrTDStruct);

  *ppTD = ptrTDStruct;

  return EFI_SUCCESS;
}


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
{
  //
  // Valid means the link pointer is valid,
  // else, it's invalid.
  //
  ptrTDStruct->TDData.TDLinkPtrTerminate = (bValid ? 0 : 1);
}

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
{
  //
  // Indicate whether the Link Pointer pointing to a QH or TD
  //
  ptrTDStruct->TDData.TDLinkPtrQSelect = (bQH ? 1 : 0);
}

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
{
  //
  // If TRUE, indicating the host controller should process in depth first
  // fashion,
  // else, the host controller should process in breadth first fashion
  //
  ptrTDStruct->TDData.TDLinkPtrDepthSelect = (bDepth ? 1 : 0);
}

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
{
  //
  // Set TD Link Pointer. Since QH,TD align on 16-byte boundaries,
  // only the highest 28 bits are valid. (if take 32bit address as an example)
  //
  ptrTDStruct->TDData.TDLinkPtr = (UINT32) ((UINTN) ptrNext >> 4);
}

VOID *
GetTDLinkPtr (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Get TD Link Pointer

Arguments:

  ptrTDStruct - TD_STRUCT
   
Returns:

  Pointer to get

--*/
{
  //
  // Get TD Link Pointer. Restore it back to 32bit
  // (if take 32bit address as an example)
  //
  return (VOID *) ((UINTN) (ptrTDStruct->TDData.TDLinkPtr << 4));
}

STATIC
BOOLEAN
IsTDLinkPtrQHOrTD (
  IN TD_STRUCT     *ptrTDStruct
  )
/*++

Routine Description:

  Is TD Link Pointer is QH Or TD

Arguments:

  ptrTDStruct - TODO: add argument description

Returns:

  TRUE  - QH
  FALSE - TD 

--*/
{
  //
  // Get the information about whether the Link Pointer field pointing to
  // a QH or a TD.
  //
  return (BOOLEAN) (ptrTDStruct->TDData.TDLinkPtrQSelect);
}

VOID
EnableorDisableTDShortPacket (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bEnable
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
{
  //
  // TRUE means enable short packet detection mechanism.
  //
  ptrTDStruct->TDData.TDStatusSPD = (bEnable ? 1 : 0);
}

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
{
  //
  // valid value of nMaxErrors is 0,1,2,3
  //
  if (nMaxErrors > 3) {
    nMaxErrors = 3;
  }

  ptrTDStruct->TDData.TDStatusErr = nMaxErrors;
}


VOID
SetTDLoworFullSpeedDevice (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       bLowSpeedDevice
  )
{
  //
  // TRUE means the TD is targeting at a Low-speed device
  //
  ptrTDStruct->TDData.TDStatusLS = (bLowSpeedDevice ? 1 : 0);
}

VOID
SetTDControlIsochronousorNot (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       IsIsochronous
  )
{
  //
  // TRUE means the TD belongs to Isochronous transfer type.
  //
  ptrTDStruct->TDData.TDStatusIOS = (IsIsochronous ? 1 : 0);
}

VOID
SetorClearTDControlIOC (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       IsSet
  )
{
  //
  // If this bit is set, it indicates that the host controller should issue
  // an interrupt on completion of the frame in which this TD is executed.
  //
  ptrTDStruct->TDData.TDStatusIOC = IsSet ? 1 : 0;
}

VOID
SetTDStatusActiveorInactive (
  IN TD_STRUCT     *ptrTDStruct,
  IN BOOLEAN       IsActive
  )
{
  //
  // If this bit is set, it indicates that the TD is active and can be
  // executed.
  //
  if (IsActive) {
    ptrTDStruct->TDData.TDStatus |= 0x80;
  } else {
    ptrTDStruct->TDData.TDStatus &= 0x7F;
  }
}

UINT16
SetTDTokenMaxLength (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINT16        MaximumLength
  )
{
  //
  // Specifies the maximum number of data bytes allowed for the transfer.
  // the legal value extent is 0 ~ 0x500.
  //
  if (MaximumLength > 0x500) {
    MaximumLength = 0x500;
  }
  ptrTDStruct->TDData.TDTokenMaxLen = MaximumLength - 1;

  return MaximumLength;
}

VOID
SetTDTokenDataToggle1 (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  //
  // Set the data toggle bit to DATA1
  //
  ptrTDStruct->TDData.TDTokenDataToggle = 1;
}

VOID
SetTDTokenDataToggle0 (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  //
  // Set the data toggle bit to DATA0
  //
  ptrTDStruct->TDData.TDTokenDataToggle = 0;
}

UINT8
GetTDTokenDataToggle (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  //
  // Get the data toggle value.
  //
  return (UINT8) (ptrTDStruct->TDData.TDTokenDataToggle);
}

VOID
SetTDTokenEndPoint (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINTN         EndPoint
  )
{
  //
  // Set EndPoint Number the TD is targeting at.
  //
  ptrTDStruct->TDData.TDTokenEndPt = (UINT8) EndPoint;
}

VOID
SetTDTokenDeviceAddress (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINTN         DeviceAddress
  )
{
  //
  // Set Device Address the TD is targeting at.
  //
  ptrTDStruct->TDData.TDTokenDevAddr = (UINT8) DeviceAddress;
}

VOID
SetTDTokenPacketID (
  IN TD_STRUCT     *ptrTDStruct,
  IN UINT8         PID
  )
{
  //
  // Set the Packet Identification to be used for this transaction.
  //
  ptrTDStruct->TDData.TDTokenPID = PID;
}

VOID
SetTDDataBuffer (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  //
  // Set the beginning address of the data buffer that will be used
  // during the transaction.
  //
  ptrTDStruct->TDData.TDBufferPtr = (UINT32) ((UINTN) (ptrTDStruct->pTDBuffer));
}

BOOLEAN
IsTDStatusActive (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether the TD is active.
  //
  TDStatus = (UINT8) (ptrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x80);
}

BOOLEAN
IsTDStatusStalled (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether the device/endpoint addressed by this TD is stalled.
  //
  TDStatus = (UINT8) (ptrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x40);
}

BOOLEAN
IsTDStatusBufferError (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  UINT8 TDStatus;
  //
  // Detect whether Data Buffer Error is happened.
  //
  TDStatus = (UINT8) (ptrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x20);
}

BOOLEAN
IsTDStatusBabbleError (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether Babble Error is happened.
  //
  TDStatus = (UINT8) (ptrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x10);
}

BOOLEAN
IsTDStatusNAKReceived (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether NAK is received.
  //
  TDStatus = (UINT8) (ptrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x08);
}

BOOLEAN
IsTDStatusCRCTimeOutError (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether CRC/Time Out Error is encountered.
  //
  TDStatus = (UINT8) (ptrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x04);
}

BOOLEAN
IsTDStatusBitStuffError (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether Bitstuff Error is received.
  //
  TDStatus = (UINT8) (ptrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x02);
}

UINT16
GetTDStatusActualLength (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  //
  // Retrieve the actual number of bytes that were tansferred.
  // the value is encoded as n-1. so return the decoded value.
  //
  return (UINT16) ((ptrTDStruct->TDData.TDStatusActualLength) + 1);
}

UINT16
GetTDTokenMaxLength (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  //
  // Retrieve the maximum number of data bytes allowed for the trnasfer.
  //
  return (UINT16) ((ptrTDStruct->TDData.TDTokenMaxLen) + 1);
}

UINT8
GetTDTokenEndPoint (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  //
  // Retrieve the endpoint number the transaction is targeting at.
  //
  return (UINT8) (ptrTDStruct->TDData.TDTokenEndPt);
}

UINT8
GetTDTokenDeviceAddress (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  //
  // Retrieve the device address the transaction is targeting at.
  //
  return (UINT8) (ptrTDStruct->TDData.TDTokenDevAddr);
}

UINT8
GetTDTokenPacketID (
  IN  TD_STRUCT *ptrTDStruct
  )
{
  //
  // Retrieve the Packet Identification information.
  //
  return (UINT8) (ptrTDStruct->TDData.TDTokenPID);
}

UINT8 *
GetTDDataBuffer (
  IN TD_STRUCT    *ptrTDStruct
  )
{
  //
  // Retrieve the beginning address of the data buffer
  // that involved in this transaction.
  //
  return ptrTDStruct->pTDBuffer;
}

BOOLEAN
GetTDLinkPtrValidorInvalid (
  IN TD_STRUCT     *ptrTDStruct
  )
{
  //
  // Retrieve the information of whether the Link Pointer field
  // is valid or not.
  //
  if (ptrTDStruct->TDData.TDLinkPtrTerminate) {
    return FALSE;
  } else {
    return TRUE;
  }

}

UINTN
CountTDsNumber (
  IN TD_STRUCT     *PtrFirstTD
  )
{
  UINTN     Number;
  TD_STRUCT *ptr;
  //
  // Count the queued TDs number.
  //
  Number  = 0;
  ptr     = PtrFirstTD;
  while (ptr) {
    ptr = (TD_STRUCT *) ptr->ptrNextTD;
    Number++;
  }

  return Number;
}



VOID
LinkTDToQH (
  IN QH_STRUCT     *PtrQH,
  IN TD_STRUCT     *PtrTD
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
{
  if (PtrQH == NULL || PtrTD == NULL) {
    return ;
  }
  //
  //  Validate QH Vertical Ptr field
  //
  SetQHVerticalValidorInvalid (PtrQH, TRUE);

  //
  //  Vertical Ptr pointing to TD structure
  //
  SetQHVerticalQHorTDSelect (PtrQH, FALSE);

  SetQHVerticalLinkPtr (PtrQH, (VOID *) PtrTD);

  PtrQH->ptrDown = (VOID *) PtrTD;
}

VOID
LinkTDToTD (
  IN TD_STRUCT     *ptrPreTD,
  IN TD_STRUCT     *PtrTD
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
{
  if (ptrPreTD == NULL || PtrTD == NULL) {
    return ;
  }
  //
  // Depth first fashion
  //
  SetTDLinkPtrDepthorBreadth (ptrPreTD, TRUE);

  //
  // Link pointer pointing to TD struct
  //
  SetTDLinkPtrQHorTDSelect (ptrPreTD, FALSE);

  //
  // Validate the link pointer valid bit
  //
  SetTDLinkPtrValidorInvalid (ptrPreTD, TRUE);

  SetTDLinkPtr (ptrPreTD, PtrTD);

  ptrPreTD->ptrNextTD = (VOID *) PtrTD;
}
//
// Transfer Schedule related Helper Functions
//
VOID
SetorClearCurFrameListTerminate (
  IN FRAMELIST_ENTRY     *pCurEntry,
  IN BOOLEAN             IsSet
  )
{
  //
  // If TRUE, empty the frame. If FALSE, indicate the Pointer field is valid.
  //
  pCurEntry->FrameListPtrTerminate = (IsSet ? 1 : 0);
}

VOID
SetCurFrameListQHorTD (
  IN FRAMELIST_ENTRY     *pCurEntry,
  IN BOOLEAN             IsQH
  )
{
  //
  // This bit indicates to the hardware whether the item referenced by the
  // link pointer is a TD or a QH.
  //
  pCurEntry->FrameListPtrQSelect = (IsQH ? 1 : 0);
}

STATIC
BOOLEAN
IsCurFrameListQHorTD (
  IN FRAMELIST_ENTRY     *pCurEntry
  )
{
  //
  // TRUE is QH
  // FALSE is TD
  //
  return (BOOLEAN) (pCurEntry->FrameListPtrQSelect);
}

BOOLEAN
GetCurFrameListTerminate (
  IN FRAMELIST_ENTRY     *pCurEntry
  )
{
  //
  // TRUE means the frame is empty,
  // FALSE means the link pointer field is valid.
  //
  return (BOOLEAN) (pCurEntry->FrameListPtrTerminate);
}

VOID
SetCurFrameListPointer (
  IN FRAMELIST_ENTRY     *pCurEntry,
  IN UINT8               *ptr
  )
{
  //
  // Set the pointer field of the frame.
  //
  pCurEntry->FrameListPtr = (UINT32) ((UINTN) ptr >> 4);
}

VOID *
GetCurFrameListPointer (
  IN FRAMELIST_ENTRY     *pCurEntry
  )
{
  //
  // Get the link pointer of the frame.
  //
  return (VOID *) ((UINTN) (pCurEntry->FrameListPtr << 4));

}

VOID
LinkQHToFrameList (
  IN FRAMELIST_ENTRY     *pEntry,
  IN UINT16              FrameListIndex,
  IN QH_STRUCT           *PtrQH
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
{
  FRAMELIST_ENTRY *pCurFrame;
  QH_STRUCT       *TempQH;
  QH_STRUCT       *NextTempQH;
  TD_STRUCT       *TempTD;
  BOOLEAN         LINK;

  //
  // Get frame list entry that the link process will begin from.
  //
  pCurFrame = pEntry + FrameListIndex;

  //
  // if current frame is empty
  // then link the specified QH directly to the Frame List.
  //
  if (GetCurFrameListTerminate (pCurFrame)) {
    
    //
    // Link new QH to the frame list entry.
    //
    SetCurFrameListQHorTD (pCurFrame, TRUE);

    SetCurFrameListPointer (pCurFrame, (UINT8 *) PtrQH);

    //
    // clear T bit in the Frame List, indicating that the frame list entry
    // is no longer empty.
    //
    SetorClearCurFrameListTerminate (pCurFrame, FALSE);

    return ;

  } else {
    //
    // current frame list has link pointer
    //
    if (!IsCurFrameListQHorTD (pCurFrame)) {
      //
      //  a TD is linked to the framelist entry
      //
      TempTD = (TD_STRUCT *) GetCurFrameListPointer (pCurFrame);

      while (GetTDLinkPtrValidorInvalid (TempTD)) {

        if (IsTDLinkPtrQHOrTD (TempTD)) {
          //
          // QH linked next to the TD
          //
          break;
        }

        TempTD = (TD_STRUCT *) GetTDLinkPtr (TempTD);
      }
      
      //
      // either no ptr linked next to the TD or QH is linked next to the TD
      //
      if (!GetTDLinkPtrValidorInvalid (TempTD)) {
        
        //
        // no ptr linked next to the TD
        //
        TempTD->ptrNextQH = PtrQH;
        SetTDLinkPtrQHorTDSelect (TempTD, TRUE);
        SetTDLinkPtr (TempTD, PtrQH);
        SetTDLinkPtrValidorInvalid (TempTD, TRUE);
        return ;

      } else {
        //
        //  QH is linked next to the TD
        //
        TempQH = (QH_STRUCT *) GetTDLinkPtr (TempTD);
      }
    } else {
      //
      // a QH is linked to the framelist entry
      //
      TempQH = (QH_STRUCT *) GetCurFrameListPointer (pCurFrame);
    }
    
    //
    // Set up Flag
    //
    LINK = TRUE;

    //
    // Avoid the same qh repeated linking in one frame entry
    //
    if (TempQH == PtrQH) {
      LINK = FALSE;
      return ;
    }
    //
    // if current QH has next QH connected
    //
    while (GetQHHorizontalValidorInvalid (TempQH)) {
      //
      // Get next QH pointer
      //
      NextTempQH = (QH_STRUCT *) GetQHHorizontalLinkPtr (TempQH);

      //
      // Bulk transfer qh may be self-linked,
      // so, the code below is to aVOID dead-loop when meeting self-linked qh
      //
      if (NextTempQH == TempQH) {
        LINK = FALSE;
        break;
      }

      TempQH = NextTempQH;

      //
      // Avoid the same qh repeated linking in one frame entry
      //
      if (TempQH == PtrQH) {
        LINK = FALSE;
      }
    }

    if (LINK) {
      TempQH->ptrNext = PtrQH;
      SetQHHorizontalQHorTDSelect (TempQH, TRUE);
      SetQHHorizontalLinkPtr (TempQH, PtrQH);
      SetQHHorizontalValidorInvalid (TempQH, TRUE);
    }

    return ;
  }
}

EFI_STATUS
ExecuteControlTransfer (
  IN  USB_HC_DEV     *HcDev,
  IN  TD_STRUCT      *PtrTD,
  IN  UINT32         wIndex,
  OUT UINTN          *ActualLen,
  IN  UINTN          TimeOut,
  OUT UINT32         *TransferResult
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
{
  UINTN   ErrTDPos;
  UINTN   Delay;
  UINTN   RequiredLen;
  BOOLEAN TransferFinished;

  ErrTDPos        = 0;
  *TransferResult = EFI_USB_NOERROR;
  RequiredLen     = *ActualLen;
  *ActualLen      = 0;

  Delay           = (TimeOut * STALL_1_MILLI_SECOND / 50) + 1;

  do {
    TransferFinished = CheckTDsResults (
                         PtrTD,
                         RequiredLen,
                         TransferResult,
                         &ErrTDPos,
                         ActualLen
                         );

    if (TransferFinished) {
      break;
    }
       
    //
    // TD is inactive, which means the control transfer is end.
    //
    if ((*TransferResult & EFI_USB_ERR_NOTEXECUTE) != EFI_USB_ERR_NOTEXECUTE) {
      break;
    } 

    gBS->Stall (50);

  } while (Delay--);

  if (*TransferResult != EFI_USB_NOERROR) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
ExecBulkorSyncInterruptTransfer (
  IN  USB_HC_DEV     *HcDev,
  IN  TD_STRUCT      *PtrTD,
  IN  UINT32         wIndex,
  OUT UINTN          *ActualLen,
  OUT UINT8          *DataToggle,
  IN  UINTN          TimeOut,
  OUT UINT32         *TransferResult
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
{
  UINTN   ErrTDPos;
  UINTN   ScrollNum;
  UINTN   Delay;
  UINTN   RequiredLen;
  BOOLEAN TransferFinished;

  ErrTDPos        = 0;
  *TransferResult = EFI_USB_NOERROR;
  RequiredLen     = *ActualLen;
  *ActualLen      = 0;

  Delay           = (TimeOut * STALL_1_MILLI_SECOND / 50) + 1;

  do {

    TransferFinished = CheckTDsResults (
                         PtrTD,
                         RequiredLen,
                         TransferResult,
                         &ErrTDPos,
                         ActualLen
                         );
                        
    if (TransferFinished) {
      break;
    }
       
    //
    // TD is inactive, which means bulk or interrupt transfer's end.
    //    
    if ((*TransferResult & EFI_USB_ERR_NOTEXECUTE) != EFI_USB_ERR_NOTEXECUTE) {
      break;
    }

    gBS->Stall (50);

  } while (Delay--);

  //
  // has error
  //
  if (*TransferResult != EFI_USB_NOERROR) {
  
    //
    // scroll the Data Toggle back to the last success TD
    //
    ScrollNum = CountTDsNumber (PtrTD) - ErrTDPos;
    if (ScrollNum & 0x1) {
      *DataToggle ^= 1;
    }

    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

VOID
DelLinkSingleQH (
  IN  USB_HC_DEV     *HcDev,
  IN  QH_STRUCT      *PtrQH,
  IN  UINT16         FrameListIndex,
  IN  BOOLEAN        SearchOther,
  IN  BOOLEAN        Delete
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
{
  FRAMELIST_ENTRY *pCurFrame;
  UINTN           Index;
  UINTN           BeginFrame;
  UINTN           EndFrame;
  QH_STRUCT       *CurrentQH;
  QH_STRUCT       *NextQH;
  TD_STRUCT       *CurrentTD;
  VOID            *PtrPreQH;
  BOOLEAN         Found;

  NextQH    = NULL;
  PtrPreQH  = NULL;
  Found     = FALSE;

  if (PtrQH == NULL) {
    return ;
  }

  if (SearchOther) {
    BeginFrame  = 0;
    EndFrame    = 1024;
  } else {
    BeginFrame  = FrameListIndex;
    EndFrame    = FrameListIndex + 1;
  }

  for (Index = BeginFrame; Index < EndFrame; Index++) {

    pCurFrame = HcDev->FrameListEntry + (Index & 0x3FF);

    if (GetCurFrameListTerminate (pCurFrame)) {
      //
      // current frame list is empty,search next frame list entry
      //
      continue;
    }

    if (!IsCurFrameListQHorTD (pCurFrame)) {
      //
      // TD linked to current framelist
      //
      CurrentTD = (TD_STRUCT *) GetCurFrameListPointer (pCurFrame);

      while (GetTDLinkPtrValidorInvalid (CurrentTD)) {

        if (IsTDLinkPtrQHOrTD (CurrentTD)) {
          //
          // QH linked next to the TD,break while ()
          //
          break;
        }

        CurrentTD = (TD_STRUCT *) GetTDLinkPtr (CurrentTD);
      }

      if (!GetTDLinkPtrValidorInvalid (CurrentTD)) {
        //
        // no QH linked next to the last TD,
        // search next frame list
        //
        continue;
      }
      
      //
      // a QH linked next to the last TD
      //
      CurrentQH = (QH_STRUCT *) GetTDLinkPtr (CurrentTD);

      PtrPreQH  = CurrentTD;

    } else {
      //
      // a QH linked to current framelist
      //
      CurrentQH = (QH_STRUCT *) GetCurFrameListPointer (pCurFrame);

      PtrPreQH  = NULL;
    }

    if (CurrentQH == PtrQH) {

      if (GetQHHorizontalValidorInvalid (PtrQH)) {
        //
        // there is QH connected after the QH found
        //
        //
        // retrieve nex qh pointer of the qh found.
        //
        NextQH = GetQHHorizontalLinkPtr (PtrQH);
      } else {
        NextQH = NULL;
      }

      if (PtrPreQH) {
        //
        // QH linked to a TD struct
        //
        CurrentTD = (TD_STRUCT *) PtrPreQH;

        SetTDLinkPtrValidorInvalid (CurrentTD, (BOOLEAN) ((NextQH == NULL) ? FALSE : TRUE));
        SetTDLinkPtr (CurrentTD, NextQH);
        CurrentTD->ptrNextQH = NextQH;

      } else {
        //
        // QH linked directly to current framelist entry
        //
        SetorClearCurFrameListTerminate (pCurFrame, (BOOLEAN) ((NextQH == NULL) ? TRUE : FALSE));
        SetCurFrameListPointer (pCurFrame, (UINT8 *) NextQH);
      }

      Found = TRUE;
      //
      // search next framelist entry
      //
      continue;
    }

    while (GetQHHorizontalValidorInvalid (CurrentQH)) {

      PtrPreQH = CurrentQH;
      //
      // Get next horizontal linked QH
      //
      CurrentQH = (QH_STRUCT *) GetQHHorizontalLinkPtr (CurrentQH);
      //
      // the qh is found
      //
      if (CurrentQH == PtrQH) {
        break;
      }
    }
    
    //
    // search next frame list entry
    //
    if (CurrentQH != PtrQH) {
      //
      // Not find the QH
      //
      continue;
    }
    //
    // find the specified qh, then delink it from
    // the horizontal QH list in the frame entry.
    //
    
    if (GetQHHorizontalValidorInvalid (PtrQH)) {
      //
      // there is QH connected after the QH found
      //
      //
      // retrieve nex qh pointer of the qh found.
      //
      NextQH = GetQHHorizontalLinkPtr (PtrQH);

    } else {
      //
      // NO QH connected after the QH found
      //
      NextQH = NULL;
      //
      // NULL the previous QH's link ptr and set Terminate field.
      //
      SetQHHorizontalValidorInvalid ((QH_STRUCT *) PtrPreQH, FALSE);
    }

    SetQHHorizontalLinkPtr ((QH_STRUCT *) PtrPreQH, NextQH);
    ((QH_STRUCT *) PtrPreQH)->ptrNext = NextQH;

    Found = TRUE;
  }

  if (Found && Delete) {
    //
    // free memory once used by the specific QH
    //
    UhciFreePool (HcDev, (UINT8 *) PtrQH, sizeof (QH_STRUCT));
  }

  return ;
}


VOID
DeleteQueuedTDs (
  IN USB_HC_DEV     *HcDev,
  IN TD_STRUCT      *PtrFirstTD
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
{
  TD_STRUCT *Tptr1;
  TD_STRUCT *Tptr2;

  Tptr1 = PtrFirstTD;
  //
  // Delete all the TDs in a queue.
  //
  while (Tptr1) {

    Tptr2 = Tptr1;

    if (!GetTDLinkPtrValidorInvalid (Tptr2)) {
      Tptr1 = NULL;
    } else {

      Tptr1 = GetTDLinkPtr (Tptr2);

      //
      // TD link to itself
      //
      if (Tptr1 == Tptr2) {
        Tptr1 = NULL;
      }
    }

    UhciFreePool (HcDev, (UINT8 *) Tptr2, sizeof (TD_STRUCT));
  }

  return ;
}

VOID
InsertQHTDToINTList (
  IN USB_HC_DEV                          *HcDev,
  IN QH_STRUCT                           *PtrQH,
  IN TD_STRUCT                           *PtrFirstTD,
  IN UINT8                               DeviceAddress,
  IN UINT8                               EndPointAddress,
  IN UINT8                               DataToggle,
  IN UINTN                               DataLength,
  IN UINTN                               PollingInterval,
  IN VOID                                *Mapping,
  IN UINT8                               *DataBuffer,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK     CallBackFunction,
  IN VOID                                *Context
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
{
  INTERRUPT_LIST  *Node;

  Node = AllocatePool (sizeof (INTERRUPT_LIST));
  if (Node == NULL) {
    return ;
  }
  
  //
  // Fill Node field
  //
  Node->Signature     = INTERRUPT_LIST_SIGNATURE;
  Node->DevAddr       = DeviceAddress;
  Node->EndPoint      = EndPointAddress;
  Node->PtrQH         = PtrQH;
  Node->PtrFirstTD    = PtrFirstTD;
  Node->DataToggle    = DataToggle;
  Node->DataLen       = DataLength;
  Node->PollInterval  = PollingInterval;
  Node->Mapping       = Mapping;
  //
  // DataBuffer is allocated host memory, not mapped memory
  //
  Node->DataBuffer        = DataBuffer;
  Node->InterruptCallBack = CallBackFunction;
  Node->InterruptContext  = Context;

  //
  // insert the new interrupt transfer to the head of the list.
  // The interrupt transfer's monitor function scans the whole list from head
  // to tail. The new interrupt transfer MUST be added to the head of the list
  // for the sake of error recovery.
  //
  InsertHeadList (&(HcDev->InterruptListHead), &(Node->Link));

  return ;
}


EFI_STATUS
DeleteAsyncINTQHTDs (
  IN  USB_HC_DEV     *HcDev,
  IN  UINT8          DeviceAddress,
  IN  UINT8          EndPointAddress,
  OUT UINT8          *DataToggle
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
{
  QH_STRUCT       *MatchQH;
  QH_STRUCT       *ptrNextQH;
  TD_STRUCT       *MatchTD;
  LIST_ENTRY      *Link;
  INTERRUPT_LIST  *MatchList;
  INTERRUPT_LIST  *PtrList;
  BOOLEAN         Found;

  UINT32          Result;
  UINTN           ErrTDPos;
  UINTN           ActualLen;

  MatchQH   = NULL;
  MatchTD   = NULL;
  MatchList = NULL;

  //
  // no interrupt transaction exists
  //
  if (IsListEmpty (&(HcDev->InterruptListHead))) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // find the correct QH-TD that need to delete
  // (by matching Device address and EndPoint number to match QH-TD )
  //
  Found = FALSE;
  Link  = &(HcDev->InterruptListHead);
  do {

    Link    = Link->ForwardLink;
    PtrList = INTERRUPT_LIST_FROM_LINK (Link);

    if ((PtrList->DevAddr == DeviceAddress) && ((PtrList->EndPoint & 0x0f) == (EndPointAddress & 0x0f))) {
      MatchList = PtrList;

      Found     = TRUE;
      break;
    }

  } while (Link->ForwardLink != &(HcDev->InterruptListHead));

  if (!Found) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // get current endpoint's data toggle bit and save.
  //
  ExecuteAsyncINTTDs (HcDev, MatchList, &Result, &ErrTDPos, &ActualLen);
  UpdateAsyncINTQHTDs (MatchList, Result, (UINT32) ErrTDPos);
  *DataToggle = MatchList->DataToggle;

  MatchTD     = MatchList->PtrFirstTD;
  MatchQH     = MatchList->PtrQH;
  //
  // find the first matching QH position in the FrameList
  //
  while (MatchQH) {

    ptrNextQH = MatchQH->ptrNextIntQH;

    //
    // Search all the entries
    //
    DelLinkSingleQH (HcDev, MatchQH, 0, TRUE, TRUE);

    MatchQH = ptrNextQH;
  }
  
  //
  // Call PciIo->Unmap() to unmap the busmaster read/write
  //
  HcDev->PciIo->Unmap (HcDev->PciIo, MatchList->Mapping);

  //
  // free host data buffer allocated,
  // mapped data buffer is freed by Unmap
  //
  if (MatchList->DataBuffer != NULL) {
    gBS->FreePool (MatchList->DataBuffer);
  }
  
  //
  // at last delete the TDs, to aVOID problems
  //
  DeleteQueuedTDs (HcDev, MatchTD);

  //
  // remove Match node from interrupt list
  //
  RemoveEntryList (&(MatchList->Link));
  gBS->FreePool (MatchList);
  return EFI_SUCCESS;
}

BOOLEAN
CheckTDsResults (
  IN  TD_STRUCT     *PtrTD,
  IN  UINTN         RequiredLen,
  OUT UINT32        *Result,
  OUT UINTN         *ErrTDPos,
  OUT UINTN         *ActualTransferSize
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
{
  UINTN Len;

  *Result   = EFI_USB_NOERROR;
  *ErrTDPos = 0;

  //
  // Init to zero.
  //
  *ActualTransferSize = 0;

  while (PtrTD) {

    if (IsTDStatusActive (PtrTD)) {
      *Result |= EFI_USB_ERR_NOTEXECUTE;
    }

    if (IsTDStatusStalled (PtrTD)) {
      *Result |= EFI_USB_ERR_STALL;
    }

    if (IsTDStatusBufferError (PtrTD)) {
      *Result |= EFI_USB_ERR_BUFFER;
    }

    if (IsTDStatusBabbleError (PtrTD)) {
      *Result |= EFI_USB_ERR_BABBLE;
    }

    if (IsTDStatusNAKReceived (PtrTD)) {
      *Result |= EFI_USB_ERR_NAK;
    }

    if (IsTDStatusCRCTimeOutError (PtrTD)) {
      *Result |= EFI_USB_ERR_TIMEOUT;
    }

    if (IsTDStatusBitStuffError (PtrTD)) {
      *Result |= EFI_USB_ERR_BITSTUFF;
    }
   
    //
    // if any error encountered, stop processing the left TDs.
    //
    if (*Result) {
      return FALSE;
    }

    Len = GetTDStatusActualLength (PtrTD) & 0x7FF;
    *ActualTransferSize += Len;

    if (*ActualTransferSize <= RequiredLen && Len < PtrTD->TDData.TDTokenMaxLen) {
      //
      // transter finished and actural length less than required length
      //
      goto Done;
    }
    //
    // Accumulate actual transferred data length in each TD.
    //
    PtrTD = (TD_STRUCT *) (PtrTD->ptrNextTD);
    //
    // Record the first Error TD's position in the queue,
    // this value is zero-based.
    //
    (*ErrTDPos)++;
  }

Done:
  return TRUE;
}


VOID
ExecuteAsyncINTTDs (
  IN  USB_HC_DEV         *HcDev,
  IN  INTERRUPT_LIST     *PtrList,
  OUT UINT32             *Result,
  OUT UINTN              *ErrTDPos,
  OUT UINTN              *ActualLen
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
{
  //
  // *ErrTDPos is zero-based value, indicating the first error TD's position
  // in the TDs' sequence.
  // *ErrTDPos value is only valid when *Result is not equal NOERROR.
  //
  UINTN RequiredLen;

  RequiredLen = *ActualLen;
  CheckTDsResults (PtrList->PtrFirstTD, RequiredLen, Result, ErrTDPos, ActualLen);

  return ;
}


VOID
UpdateAsyncINTQHTDs (
  IN INTERRUPT_LIST     *PtrList,
  IN UINT32             Result,
  IN UINT32             ErrTDPos
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
{
  QH_STRUCT *PtrFirstQH;
  QH_STRUCT *PtrQH;
  TD_STRUCT *PtrFirstTD;
  TD_STRUCT *PtrTD;
  UINT8     DataToggle;
  UINT32    Index;

  PtrFirstQH  = PtrList->PtrQH;
  PtrFirstTD  = PtrList->PtrFirstTD;

  DataToggle  = 0;

  if (Result == EFI_USB_NOERROR) {

    PtrTD = PtrFirstTD;
    while (PtrTD) {
      DataToggle  = GetTDTokenDataToggle (PtrTD);
      PtrTD       = PtrTD->ptrNextTD;
    }
    
    //
    // save current DataToggle value to interrupt list.
    // this value is used for tracing the interrupt endpoint DataToggle.
    // when this interrupt transfer is deleted, the last DataToggle is saved
    //
    PtrList->DataToggle = DataToggle;

    PtrTD               = PtrFirstTD;

    //
    // Since DataToggle bit should toggle after each success transaction,
    // the First TD's DataToggle bit will be updated to XOR of Last TD's
    // DataToggle bit. If the First TD's DataToggle bit is not equal Last
    // TD's DataToggle bit, that means it already be the XOR of Last TD's,
    // so no update is needed.
    //
    if (DataToggle == GetTDTokenDataToggle (PtrFirstTD)) {
      PtrTD = PtrFirstTD;
      while (PtrTD) {

        DataToggle ^= 1;
        if (DataToggle) {
          SetTDTokenDataToggle1 (PtrTD);
        } else {
          SetTDTokenDataToggle0 (PtrTD);
        }

        PtrTD = PtrTD->ptrNextTD;
      }
    }
    //
    // restore Link Pointer of QH to First TD
    // (because QH's Link Pointer will change during TD execution)
    //
    PtrQH = PtrFirstQH;
    while (PtrQH) {

      LinkTDToQH (PtrQH, PtrFirstTD);
      PtrQH = PtrQH->ptrNextIntQH;
    }
    
    //
    // set all the TDs active
    //
    PtrTD = PtrFirstTD;
    while (PtrTD) {
      SetTDStatusActiveorInactive (PtrTD, TRUE);
      PtrTD = PtrTD->ptrNextTD;
    }

  } else if (((Result & EFI_USB_ERR_NOTEXECUTE) == EFI_USB_ERR_NOTEXECUTE) ||
           ((Result & EFI_USB_ERR_NAK) == EFI_USB_ERR_NAK)
          ) {
    //
    // no update
    //
  } else {
    //
    // Have Errors
    //
    PtrTD = PtrFirstTD;
    //
    // not first TD error
    //
    if (ErrTDPos != 0) {
      //
      // get the last success TD
      //
      for (Index = 1; Index < ErrTDPos; Index++) {
        PtrTD = PtrTD->ptrNextTD;
      }
      //
      // update Data Toggle in the interrupt list node
      //
      PtrList->DataToggle = GetTDTokenDataToggle (PtrTD);

      //
      // get the error TD
      //
      PtrTD = PtrTD->ptrNextTD;

    } else {
      PtrList->DataToggle = GetTDTokenDataToggle (PtrTD);
    }
    //
    // do not restore the QH's vertical link pointer,
    // let the callback function do the rest of error handling.
    //
  }

  return ;
}

VOID
ReleaseInterruptList (
  IN USB_HC_DEV         *HcDev,
  IN LIST_ENTRY         *ListHead
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
{
  LIST_ENTRY      *Link;
  LIST_ENTRY      *SavedLink;
  INTERRUPT_LIST  *pNode;
  TD_STRUCT       *PtrTD;
  TD_STRUCT       *ptrNextTD;
  QH_STRUCT       *PtrQH;
  QH_STRUCT       *SavedQH;

  if (ListHead == NULL) {
    return ;
  }

  Link = ListHead;

  //
  // Free all the resources in the interrupt list
  //
  SavedLink = Link->ForwardLink;
  while (!IsListEmpty (ListHead)) {

    Link      = SavedLink;

    SavedLink = Link->ForwardLink;

    pNode     = INTERRUPT_LIST_FROM_LINK (Link);

    RemoveEntryList (&pNode->Link);

    SavedQH = pNode->PtrQH;
    for (PtrQH = SavedQH; PtrQH != NULL; PtrQH = SavedQH) {
      SavedQH = PtrQH->ptrNextIntQH;
      UhciFreePool (HcDev, (UINT8 *) PtrQH, sizeof (QH_STRUCT));
    }

    PtrTD = pNode->PtrFirstTD;
    while (PtrTD != NULL) {

      ptrNextTD = PtrTD->ptrNextTD;
      UhciFreePool (HcDev, (UINT8 *) PtrTD, sizeof (TD_STRUCT));
      PtrTD = ptrNextTD;
    }

    gBS->FreePool (pNode);
  }
}


EFI_STATUS
InitializeMemoryManagement (
  IN USB_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Initialize Memory Management

Arguments:

  HcDev - USB_HC_DEV

Returns:

  EFI_SUCCESS -  Success
--*/
{
  MEMORY_MANAGE_HEADER  *MemoryHeader;
  EFI_STATUS            Status;
  UINTN                 MemPages;

  MemPages  = NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES;
  Status    = CreateMemoryBlock (HcDev, &MemoryHeader, MemPages);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HcDev->MemoryHeader = MemoryHeader;

  return EFI_SUCCESS;
}

EFI_STATUS
CreateMemoryBlock (
  IN  USB_HC_DEV               *HcDev,
  OUT MEMORY_MANAGE_HEADER     **MemoryHeader,
  IN  UINTN                    MemoryBlockSizeInPages
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

  EFI_SUCCESS -  Success
--*/
{
  EFI_STATUS            Status;
  VOID                  *CommonBuffer;
  EFI_PHYSICAL_ADDRESS  MappedAddress;
  UINTN                 MemoryBlockSizeInBytes;
  VOID                  *Mapping;

  //
  // Allocate memory for MemoryHeader
  //
  *MemoryHeader = AllocateZeroPool (sizeof (MEMORY_MANAGE_HEADER));
  if (*MemoryHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  (*MemoryHeader)->Next = NULL;

  //
  // set Memory block size
  //
  (*MemoryHeader)->MemoryBlockSizeInBytes = EFI_PAGES_TO_SIZE (MemoryBlockSizeInPages);

  //
  // each bit in Bit Array will manage 32 bytes memory in memory block
  //
  (*MemoryHeader)->BitArraySizeInBytes = ((*MemoryHeader)->MemoryBlockSizeInBytes / 32) / 8;

  //
  // Allocate memory for BitArray
  //
  (*MemoryHeader)->BitArrayPtr = AllocateZeroPool ((*MemoryHeader)->BitArraySizeInBytes);
  if ((*MemoryHeader)->BitArrayPtr == NULL) {
    gBS->FreePool (*MemoryHeader);
    return EFI_OUT_OF_RESOURCES;
  }
  
  //
  // Memory Block uses MemoryBlockSizeInPages pages,
  // and it is allocated as common buffer use.
  //
  Status = HcDev->PciIo->AllocateBuffer (
                           HcDev->PciIo,
                           AllocateAnyPages,
                           EfiBootServicesData,
                           MemoryBlockSizeInPages,
                           &CommonBuffer,
                           0
                           );
  if (EFI_ERROR (Status)) {
    gBS->FreePool ((*MemoryHeader)->BitArrayPtr);
    gBS->FreePool (*MemoryHeader);
    return Status;
  }

  MemoryBlockSizeInBytes = EFI_PAGES_TO_SIZE (MemoryBlockSizeInPages);
  Status = HcDev->PciIo->Map (
                           HcDev->PciIo,
                           EfiPciIoOperationBusMasterCommonBuffer,
                           CommonBuffer,
                           &MemoryBlockSizeInBytes,
                           &MappedAddress,
                           &Mapping
                           );
  //
  // if returned Mapped size is less than the size we request,do not support.
  //
  if (EFI_ERROR (Status) || (MemoryBlockSizeInBytes != EFI_PAGES_TO_SIZE (MemoryBlockSizeInPages))) {
    HcDev->PciIo->FreeBuffer (HcDev->PciIo, MemoryBlockSizeInPages, CommonBuffer);
    gBS->FreePool ((*MemoryHeader)->BitArrayPtr);
    gBS->FreePool (*MemoryHeader);
    return EFI_UNSUPPORTED;
  }
  //
  // Set Memory block initial address
  //
  (*MemoryHeader)->MemoryBlockPtr = (UINT8 *) ((UINTN) MappedAddress);
  (*MemoryHeader)->Mapping        = Mapping;

  ZeroMem (
    (*MemoryHeader)->MemoryBlockPtr,
    EFI_PAGES_TO_SIZE (MemoryBlockSizeInPages)
    );

  return EFI_SUCCESS;
}

EFI_STATUS
FreeMemoryHeader (
  IN USB_HC_DEV               *HcDev,
  IN MEMORY_MANAGE_HEADER     *MemoryHeader
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
{
  if ((MemoryHeader == NULL) || (HcDev == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // unmap the common buffer used by the memory block
  //
  HcDev->PciIo->Unmap (HcDev->PciIo, MemoryHeader->Mapping);

  //
  // free common buffer
  //
  HcDev->PciIo->FreeBuffer (
                  HcDev->PciIo,
                  EFI_SIZE_TO_PAGES (MemoryHeader->MemoryBlockSizeInBytes),
                  MemoryHeader->MemoryBlockPtr
                  );
  //
  // free bit array
  //
  gBS->FreePool (MemoryHeader->BitArrayPtr);
  //
  // free memory header
  //
  gBS->FreePool (MemoryHeader);

  return EFI_SUCCESS;
}

EFI_STATUS
UhciAllocatePool (
  IN  USB_HC_DEV     *HcDev,
  OUT UINT8          **Pool,
  IN  UINTN          AllocSize
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
{
  MEMORY_MANAGE_HEADER  *MemoryHeader;
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;
  MEMORY_MANAGE_HEADER  *NewMemoryHeader;
  UINTN                 RealAllocSize;
  UINTN                 MemoryBlockSizeInPages;
  EFI_STATUS            Status;

  *Pool         = NULL;

  MemoryHeader  = HcDev->MemoryHeader;
  ASSERT (MemoryHeader != NULL);

  //
  // allocate unit is 32 bytes (align on 32 byte)
  //
  if (AllocSize & 0x1F) {
    RealAllocSize = (AllocSize / 32 + 1) * 32;
  } else {
    RealAllocSize = AllocSize;
  }
  
  //
  // There may be linked MemoryHeaders.
  // To allocate a free pool in Memory blocks,
  // must search in the MemoryHeader link list
  // until enough free pool is found.
  //
  Status = EFI_NOT_FOUND;
  for (TempHeaderPtr = MemoryHeader; TempHeaderPtr != NULL;
       TempHeaderPtr = TempHeaderPtr->Next) {

    Status = AllocMemInMemoryBlock (
               TempHeaderPtr,
               (VOID **) Pool,
               RealAllocSize / 32
               );
    if (!EFI_ERROR (Status)) {
      ZeroMem (*Pool, AllocSize);
      return EFI_SUCCESS;
    }
  }
  
  //
  // There is no enough memory,
  // Create a new Memory Block
  //
  
  //
  // if pool size is larger than NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES,
  // just allocate a large enough memory block.
  //
  if (RealAllocSize > EFI_PAGES_TO_SIZE (NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES)) {
    MemoryBlockSizeInPages = EFI_SIZE_TO_PAGES (RealAllocSize) + 1;
  } else {
    MemoryBlockSizeInPages = NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES;
  }

  Status = CreateMemoryBlock (HcDev, &NewMemoryHeader, MemoryBlockSizeInPages);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Link the new Memory Block to the Memory Header list
  //
  InsertMemoryHeaderToList (MemoryHeader, NewMemoryHeader);

  Status = AllocMemInMemoryBlock (
             NewMemoryHeader,
             (VOID **) Pool,
             RealAllocSize / 32
             );

  if (!EFI_ERROR (Status)) {
    ZeroMem (*Pool, AllocSize);
  }

  return Status;
}

VOID
UhciFreePool (
  IN USB_HC_DEV     *HcDev,
  IN UINT8          *Pool,
  IN UINTN          AllocSize
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
{
  MEMORY_MANAGE_HEADER  *MemoryHeader;
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;
  UINTN                 StartBytePos;
  UINTN                 Index;
  UINT8                 StartBitPos;
  UINT8                 Index2;
  UINTN                 Count;
  UINTN                 RealAllocSize;

  MemoryHeader = HcDev->MemoryHeader;

  //
  // allocate unit is 32 byte (align on 32 byte)
  //
  if (AllocSize & 0x1F) {
    RealAllocSize = (AllocSize / 32 + 1) * 32;
  } else {
    RealAllocSize = AllocSize;
  }
  //
  // scan the memory header linked list for
  // the asigned memory to free.
  //
  for (TempHeaderPtr = MemoryHeader;TempHeaderPtr != NULL;
       TempHeaderPtr = TempHeaderPtr->Next) {

    if ((Pool >= TempHeaderPtr->MemoryBlockPtr) &&
        ((Pool + RealAllocSize) <= (TempHeaderPtr->MemoryBlockPtr + TempHeaderPtr->MemoryBlockSizeInBytes))
        ) {
      //
      // Pool is in the Memory Block area,
      // find the start byte and bit in the bit array
      //
      StartBytePos  = ((Pool - TempHeaderPtr->MemoryBlockPtr) / 32) / 8;
      StartBitPos   = (UINT8) (((Pool - TempHeaderPtr->MemoryBlockPtr) / 32) & 0x7);

      //
      // reset associated bits in bit arry
      //
      for (Index = StartBytePos, Index2 = StartBitPos, Count = 0; Count < (RealAllocSize / 32); Count++) {

        TempHeaderPtr->BitArrayPtr[Index] = (UINT8) (TempHeaderPtr->BitArrayPtr[Index] ^ bit (Index2));
        Index2++;
        if (Index2 == 8) {
          Index += 1;
          Index2 = 0;
        }
      }
      //
      // break the loop
      //
      break;
    }
  }
  
  //
  // Release emptied memory blocks (only if the memory block is not
  // the first one in the memory header list
  //
  for (TempHeaderPtr = MemoryHeader->Next; TempHeaderPtr != NULL;) {
    //
    // Debug
    //
    ASSERT (MemoryHeader->Next != NULL);

    if (IsMemoryBlockEmptied (TempHeaderPtr)) {

      DelinkMemoryBlock (MemoryHeader, TempHeaderPtr);
      //
      // when the TempHeaderPtr is freed in FreeMemoryHeader(),
      // the TempHeaderPtr is pointing to nonsense content.
      //
      FreeMemoryHeader (HcDev, TempHeaderPtr);
      //
      // reset the TempHeaderPtr, continue search for
      // another empty memory block.
      //
      TempHeaderPtr = MemoryHeader->Next;
      continue;
    }

    TempHeaderPtr = TempHeaderPtr->Next;
  }
}

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
{
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;

  for (TempHeaderPtr = MemoryHeader; TempHeaderPtr != NULL;
       TempHeaderPtr = TempHeaderPtr->Next) {
    if (TempHeaderPtr->Next == NULL) {
      TempHeaderPtr->Next = NewMemoryHeader;
      break;
    }
  }
}

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

  EFI_NOT_FOUND  - Can't find the free memory 
  EFI_SUCCESS    - Success

--*/
{
  UINTN TempBytePos;
  UINTN FoundBytePos;
  UINT8 Index;
  UINT8 FoundBitPos;
  UINT8 ByteValue;
  UINT8 BitValue;
  UINTN NumberOfZeros;
  UINTN Count;

  FoundBytePos  = 0;
  FoundBitPos   = 0;
  ByteValue     = MemoryHeader->BitArrayPtr[0];
  NumberOfZeros = 0;
  Index         = 0;

  for (TempBytePos = 0; TempBytePos < MemoryHeader->BitArraySizeInBytes;) {
    
    //
    // Pop out BitValue from a byte in TempBytePos.
    //
    BitValue = (UINT8) (ByteValue & 0x1);
    //
    // right shift the byte
    //
    ByteValue /= 2;

    if (BitValue == 0) {
      //
      // Found a free bit, the NumberOfZeros only record the number
      // of those consecutive zeros
      //
      NumberOfZeros++;
      //
      // Found enough consecutive free space, break the loop
      //
      if (NumberOfZeros >= NumberOfMemoryUnit) {
        break;
      }
    } else {
      //
      // Encountering a '1', meant the bit is ocupied.
      //
      if (NumberOfZeros >= NumberOfMemoryUnit) {
        //
        // Found enough consecutive free space,break the loop
        //
        break;
      } else {
        //
        // the NumberOfZeros only record the number of those consecutive zeros,
        // so reset the NumberOfZeros to 0 when encountering '1' before finding
        // enough consecutive '0's
        //
        NumberOfZeros = 0;
        //
        // reset the (FoundBytePos,FoundBitPos) to the position of '1'
        //
        FoundBytePos  = TempBytePos;
        FoundBitPos   = Index;
      }
    }
    //
    // step forward a bit
    //
    Index++;
    if (Index == 8) {
      //
      // step forward a byte, getting the byte value,
      // and reset the bit pos.
      //
      TempBytePos += 1;
      ByteValue = MemoryHeader->BitArrayPtr[TempBytePos];
      Index     = 0;
    }
  }

  if (NumberOfZeros < NumberOfMemoryUnit) {
    return EFI_NOT_FOUND;
  }
  
  //
  // Found enough free space.
  //
  
  //
  // The values recorded in (FoundBytePos,FoundBitPos) have two conditions:
  //  1)(FoundBytePos,FoundBitPos) record the position
  //    of the last '1' before the consecutive '0's, it must
  //    be adjusted to the start position of the consecutive '0's.
  //  2)the start address of the consecutive '0's is just the start of
  //    the bitarray. so no need to adjust the values of
  //    (FoundBytePos,FoundBitPos).
  //
  if ((MemoryHeader->BitArrayPtr[0] & bit (0)) != 0) {
    FoundBitPos += 1;
  }
  
  //
  // Have the (FoundBytePos,FoundBitPos) make sense.
  //
  if (FoundBitPos > 7) {
    FoundBytePos += 1;
    FoundBitPos -= 8;
  }
  
  //
  // Set the memory as allocated
  //
  for (TempBytePos = FoundBytePos, Index = FoundBitPos,Count = 0;
       Count < NumberOfMemoryUnit; Count ++) {

    MemoryHeader->BitArrayPtr[TempBytePos] = (UINT8) (MemoryHeader->BitArrayPtr[TempBytePos] | bit (Index));
    Index++;
    if (Index == 8) {
      TempBytePos += 1;
      Index = 0;
    }
  }

  *Pool = MemoryHeader->MemoryBlockPtr + (FoundBytePos * 8 + FoundBitPos) * 32;

  return EFI_SUCCESS;
}

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

  TRUE  - Empty
  FALSE - Not Empty 

--*/
{
  UINTN Index;

  for (Index = 0; Index < MemoryHeaderPtr->BitArraySizeInBytes; Index++) {
    if (MemoryHeaderPtr->BitArrayPtr[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

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
{
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;

  if ((FirstMemoryHeader == NULL) || (NeedFreeMemoryHeader == NULL)) {
    return ;
  }
  for (TempHeaderPtr = FirstMemoryHeader; TempHeaderPtr != NULL;
       TempHeaderPtr = TempHeaderPtr->Next) {

    if (TempHeaderPtr->Next == NeedFreeMemoryHeader) {
      //
      // Link the before and after
      //
      TempHeaderPtr->Next = NeedFreeMemoryHeader->Next;
      break;
    }
  }
}

EFI_STATUS
DelMemoryManagement (
  IN USB_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Delete Memory Management

Arguments:

  HcDev - USB_HC_DEV

Returns:

  EFI_SUCCESS - Success

--*/
{
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;

  for (TempHeaderPtr = HcDev->MemoryHeader->Next; TempHeaderPtr != NULL;) {

    DelinkMemoryBlock (HcDev->MemoryHeader, TempHeaderPtr);
    //
    // when the TempHeaderPtr is freed in FreeMemoryHeader(),
    // the TempHeaderPtr is pointing to nonsense content.
    //
    FreeMemoryHeader (HcDev, TempHeaderPtr);
    //
    // reset the TempHeaderPtr,continue free another memory block.
    //
    TempHeaderPtr = HcDev->MemoryHeader->Next;
  }

  FreeMemoryHeader (HcDev, HcDev->MemoryHeader);

  return EFI_SUCCESS;
}


VOID
CleanUsbTransactions (
  IN USB_HC_DEV     *HcDev
  )
{
  //
  // only asynchronous interrupt transfers are always alive on the bus
  //
  ReleaseInterruptList (HcDev, &(HcDev->InterruptListHead));
}

VOID
TurnOffUSBEmulation (
  IN EFI_PCI_IO_PROTOCOL     *PciIo
  )
/*++
  
  Routine Description:
    Disable USB Emulation
  Arguments:
    PciIo  -  EFI_PCI_IO_PROTOCOL
  Returns:
    VOID
--*/
{
  UINT16  Command;

  //
  // Disable USB Emulation
  //
  Command = 0;
  PciIo->Pci.Write (
               PciIo,
               EfiPciIoWidthUint16,
               USB_EMULATION,
               1,
               &Command
               );

  return ;
}
