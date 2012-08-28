/*++

Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
    PcatPciRootBridgeIo.c
    
Abstract:

    EFI PC AT PCI Root Bridge Io Protocol

Revision History

--*/

#include "PcatPciRootBridge.h"
#include <IndustryStandard/Pci.h>
#include "SalProc.h"

#include EFI_GUID_DEFINITION (SalSystemTable)

//
// Might be good to put this in an include file, but people may start
//  using it! They should always access the EFI abstraction that is
//  contained in this file. Just a little information hiding.
//
#define PORT_TO_MEM(_Port) ( ((_Port) & 0xffffffffffff0000) | (((_Port) & 0xfffc) << 10) | ((_Port) & 0x0fff) )
                                                                           
//                                                                  
// Macro's with casts make this much easier to use and read.
//
#define PORT_TO_MEM8(_Port)     (*(UINT8  *)(PORT_TO_MEM(_Port)))
#define PORT_TO_MEM16(_Port)    (*(UINT16 *)(PORT_TO_MEM(_Port)))
#define PORT_TO_MEM32(_Port)    (*(UINT32 *)(PORT_TO_MEM(_Port)))

#define EFI_PCI_ADDRESS_IA64(_seg, _bus,_dev,_func,_reg) \
    ( (UINT64) ( (((UINTN)_seg) << 24) + (((UINTN)_bus) << 16) + (((UINTN)_dev) << 11) + (((UINTN)_func) << 8) + ((UINTN)_reg)) )

//
// Local variables for performing SAL Proc calls
//
PLABEL         mSalProcPlabel;
CALL_SAL_PROC  mGlobalSalProc;

EFI_STATUS
PcatRootBridgeIoIoRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *UserBuffer
  )
{
  PCAT_PCI_ROOT_BRIDGE_INSTANCE *PrivateData;
  UINTN                         InStride;
  UINTN                         OutStride;
  UINTN                         AlignMask;
  UINTN                         Address;
  PTR                           Buffer;
  UINT16                        Data16;
  UINT32                        Data32;
  
 
  if ( UserBuffer == NULL ) {
    return EFI_INVALID_PARAMETER;
  }
  
  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);

  Address    = (UINTN)  UserAddress;
  Buffer.buf = (UINT8 *)UserBuffer;

  if ( Address < PrivateData->IoBase || Address > PrivateData->IoLimit ) {
    return EFI_INVALID_PARAMETER;
  }
    
  if ((UINT32)Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Width & 0x03) == EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  AlignMask = (1 << (Width & 0x03)) - 1;
  if ( Address & AlignMask ) {
    return EFI_INVALID_PARAMETER;
  }

  InStride  = 1 << (Width & 0x03);
  OutStride = InStride;
  if (Width >=EfiPciWidthFifoUint8 && Width <= EfiPciWidthFifoUint64) {
    InStride = 0;
  }
  if (Width >=EfiPciWidthFillUint8 && Width <= EfiPciWidthFillUint64) {
    OutStride = 0;
  }
  Width = Width & 0x03;

  Address += PrivateData->PhysicalIoBase;

  //
  // Loop for each iteration and move the data
  //

  switch (Width) {
  case EfiPciWidthUint8:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      MEMORY_FENCE();
      *Buffer.ui8 = PORT_TO_MEM8(Address);
      MEMORY_FENCE();
    }
    break;

  case EfiPciWidthUint16:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      MEMORY_FENCE();
      if (Buffer.ui & 0x1) {
        Data16 = PORT_TO_MEM16(Address);
        *Buffer.ui8     = (UINT8)(Data16 & 0xff);
        *(Buffer.ui8+1) = (UINT8)((Data16 >> 8) & 0xff);
      } else {
        *Buffer.ui16 = PORT_TO_MEM16(Address);
      }
      MEMORY_FENCE();
    }
    break;

  case EfiPciWidthUint32:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      MEMORY_FENCE();
      if (Buffer.ui & 0x3) {
        Data32 = PORT_TO_MEM32(Address);
        *Buffer.ui8     = (UINT8)(Data32 & 0xff);
        *(Buffer.ui8+1) = (UINT8)((Data32 >> 8) & 0xff);
        *(Buffer.ui8+2) = (UINT8)((Data32 >> 16) & 0xff);
        *(Buffer.ui8+3) = (UINT8)((Data32 >> 24) & 0xff);
      } else {
        *Buffer.ui32 = PORT_TO_MEM32(Address);
      }
      MEMORY_FENCE();
    }
    break;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PcatRootBridgeIoIoWrite (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 UserAddress,
  IN UINTN                                  Count,
  IN OUT VOID                               *UserBuffer
  )
{
  PCAT_PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;
  UINTN                          InStride;
  UINTN                          OutStride;
  UINTN                          AlignMask;
  UINTN                          Address;
  PTR                            Buffer;
  UINT16                         Data16;
  UINT32                         Data32;

  if ( UserBuffer == NULL ) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);

  Address    = (UINTN)  UserAddress;
  Buffer.buf = (UINT8 *)UserBuffer;

  if ( Address < PrivateData->IoBase || Address > PrivateData->IoLimit ) {
    return EFI_INVALID_PARAMETER;
  }
    
  if (Width < 0 || Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Width & 0x03) == EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  AlignMask = (1 << (Width & 0x03)) - 1;
  if ( Address & AlignMask ) {
    return EFI_INVALID_PARAMETER;
  }

  InStride  = 1 << (Width & 0x03);
  OutStride = InStride;
  if (Width >=EfiPciWidthFifoUint8 && Width <= EfiPciWidthFifoUint64) {
    InStride = 0;
  }
  if (Width >=EfiPciWidthFillUint8 && Width <= EfiPciWidthFillUint64) {
    OutStride = 0;
  }
  Width = Width & 0x03;

  Address += PrivateData->PhysicalIoBase;

  //
  // Loop for each iteration and move the data
  //

  switch (Width) {
  case EfiPciWidthUint8:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      MEMORY_FENCE();
      PORT_TO_MEM8(Address) = *Buffer.ui8;
      MEMORY_FENCE();
    }
    break;

  case EfiPciWidthUint16:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      MEMORY_FENCE();
      if (Buffer.ui & 0x1) {
        Data16 = *Buffer.ui8;
        Data16 = Data16 | (*(Buffer.ui8+1) << 8);
        PORT_TO_MEM16(Address) = Data16;
      } else {
        PORT_TO_MEM16(Address) = *Buffer.ui16;
      }
      MEMORY_FENCE();
    }
    break;
  case EfiPciWidthUint32:
    for (; Count > 0; Count--, Buffer.buf += OutStride, Address += InStride) {
      MEMORY_FENCE();
      if (Buffer.ui & 0x3) {
        Data32 = *Buffer.ui8;
        Data32 = Data32 | (*(Buffer.ui8+1) << 8);
        Data32 = Data32 | (*(Buffer.ui8+2) << 16);
        Data32 = Data32 | (*(Buffer.ui8+3) << 24);
        PORT_TO_MEM32(Address) = Data32;
      } else {
        PORT_TO_MEM32(Address) = *Buffer.ui32;
      }
      MEMORY_FENCE();
    }
    break;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PcatRootBridgeIoGetIoPortMapping (
  OUT EFI_PHYSICAL_ADDRESS  *IoPortMapping,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryPortMapping
  )
/*++

  Get the IO Port Map from the SAL System Table.
  
--*/
{
  SAL_SYSTEM_TABLE_ASCENDING_ORDER    *SalSystemTable;
  SAL_ST_MEMORY_DESCRIPTOR_ENTRY      *SalMemDesc;
  EFI_STATUS                          Status;

  //
  // On all Itanium architectures, bit 63 is the I/O bit for performming Memory Mapped I/O operations
  //
  *MemoryPortMapping = 0x8000000000000000;

  Status = EfiLibGetSystemConfigurationTable(&gEfiSalSystemTableGuid, &SalSystemTable);
  if (EFI_ERROR(Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // BugBug: Add code to test checksum on the Sal System Table
  //
  if (SalSystemTable->Entry0.Type != 0) {
    return EFI_UNSUPPORTED;
  }

  mSalProcPlabel.ProcEntryPoint = SalSystemTable->Entry0.SalProcEntry; 
  mSalProcPlabel.GP             = SalSystemTable->Entry0.GlobalDataPointer;
  mGlobalSalProc                = (CALL_SAL_PROC)&mSalProcPlabel.ProcEntryPoint;

  //
  // The SalSystemTable pointer includes the Type 0 entry.
  //  The SalMemDesc is Type 1 so it comes next.
  //
  SalMemDesc = (SAL_ST_MEMORY_DESCRIPTOR_ENTRY *)(SalSystemTable + 1);
  while (SalMemDesc->Type == SAL_ST_MEMORY_DESCRIPTOR) {
    if (SalMemDesc->MemoryType == SAL_IO_PORT_MAPPING) {
      *IoPortMapping = SalMemDesc->PhysicalMemoryAddress;
      *IoPortMapping |= 0x8000000000000000;
      return EFI_SUCCESS;
    }
    SalMemDesc++;
  }
  return EFI_UNSUPPORTED;
}

EFI_STATUS
PcatRootBridgeIoPciRW (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     BOOLEAN                                Write,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  IN OUT UINT8                                  *UserBuffer
  )
{
  PCAT_PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;
  UINTN                          AlignMask;
  UINTN                          InStride;
  UINTN                          OutStride;
  UINT64                         Address;
  DEFIO_PCI_ADDR                 *Defio;
  PTR                            Buffer;
  UINT32                         Data32;
  UINT16                         Data16;
  rArg                           Return;

  if (Width < 0 || Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Width & 0x03) == EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  AlignMask = (1 << (Width & 0x03)) - 1;
  if ( UserAddress & AlignMask ) {
    return EFI_INVALID_PARAMETER;
  }

  InStride  = 1 << (Width & 0x03);
  OutStride = InStride;
  if (Width >=EfiPciWidthFifoUint8 && Width <= EfiPciWidthFifoUint64) {
    InStride = 0;
  }
  if (Width >=EfiPciWidthFillUint8 && Width <= EfiPciWidthFillUint64) {
    OutStride = 0;
  }
  Width = Width & 0x03;

  Defio = (DEFIO_PCI_ADDR *)&UserAddress;

  if ((Defio->Function > PCI_MAX_FUNC) || (Defio->Device > PCI_MAX_DEVICE)) {
    return EFI_UNSUPPORTED;
  }
  
  Buffer.buf = (UINT8 *)UserBuffer;
  
  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);

  Address = EFI_PCI_ADDRESS_IA64(
              This->SegmentNumber, 
              Defio->Bus, 
              Defio->Device, 
              Defio->Function, 
              Defio->Register
              );

  //
  // PCI Config access are all 32-bit alligned, but by accessing the
  //  CONFIG_DATA_REGISTER (0xcfc) with different widths more cycle types
  //  are possible on PCI.
  //
  // SalProc takes care of reading the proper register depending on stride
  //

  EfiAcquireLock(&PrivateData->PciLock);

  while (Count) {

    if(Write) {

      if (Buffer.ui & 0x3) {
        Data32  = (*(Buffer.ui8+0) << 0);
        Data32 |= (*(Buffer.ui8+1) << 8);
        Data32 |= (*(Buffer.ui8+2) << 16);
        Data32 |= (*(Buffer.ui8+3) << 24);
      } else {
        Data32 = *Buffer.ui32;
      }

      Return.p0 = -3;
      Return    = mGlobalSalProc((UINT64) SAL_PCI_CONFIG_WRITE,
                                 Address, 1 << Width, Data32, 0, 0, 0, 0);
        
      if(Return.p0) {
        EfiReleaseLock(&PrivateData->PciLock);
        return EFI_UNSUPPORTED;
      }

    } else {

      Return.p0 = -3;
      Return    = mGlobalSalProc((UINT64) SAL_PCI_CONFIG_READ,
                                 Address, 1 << Width, 0, 0, 0, 0, 0);

      if(Return.p0) {
        EfiReleaseLock(&PrivateData->PciLock);
        return EFI_UNSUPPORTED;
      }

      switch (Width) {
      case EfiPciWidthUint8:
        *Buffer.ui8 = (UINT8)Return.p1;
        break;
      case EfiPciWidthUint16:
        if (Buffer.ui & 0x1) {
          Data16 = (UINT16)Return.p1;
          *(Buffer.ui8 + 0) = Data16 & 0xff;
          *(Buffer.ui8 + 1) = (Data16 >> 8) & 0xff;
        } else {
          *Buffer.ui16 = (UINT16)Return.p1;
        }
        break;
      case EfiPciWidthUint32:
        if (Buffer.ui & 0x3) {
          Data32 = (UINT32)Return.p1;
          *(Buffer.ui8 + 0) = (UINT8)(Data32 & 0xff);
          *(Buffer.ui8 + 1) = (UINT8)((Data32 >> 8) & 0xff);
          *(Buffer.ui8 + 2) = (UINT8)((Data32 >> 16) & 0xff);
          *(Buffer.ui8 + 3) = (UINT8)((Data32 >> 24) & 0xff);
        } else {
          *Buffer.ui32 = (UINT32)Return.p1;
        }
        break;
      }
    }

    Address += InStride;
    Buffer.buf += OutStride;
    Count -= 1;
  }
  
  EfiReleaseLock(&PrivateData->PciLock);

  return EFI_SUCCESS;
}

EFI_STATUS
ScanPciRootBridgeForRoms(
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *IoDev
  )
  
{
  return EFI_UNSUPPORTED;
}
