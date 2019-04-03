/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:


  PciDevice.c

Abstract:

  Platform Initialization Driver.

Revision History

--*/

#include "PlatformDxe.h"
#include "Library/DxeServicesTableLib.h"
#include "PciBus.h"
#include "Guid/PciLanInfo.h"

extern  VOID    *mPciLanInfo;
extern  UINTN   mPciLanCount;

extern  EFI_HANDLE  mImageHandle;
extern  SYSTEM_CONFIGURATION    mSystemConfiguration;


VOID       *mPciRegistration;
#define NCR_VENDOR_ID  0x1000
#define ATI_VENDOR_ID  0x1002
#define INTEL_VENDOR_ID 0x8086
#define ATI_RV423_ID   0x5548
#define ATI_RV423_ID2  0x5d57
#define ATI_RV380_ID   0x3e50
#define ATI_RV370_ID   0x5b60
#define SI_VENDOR_ID   0x1095
#define SI_SISATA_ID   0x3114
#define SI_SIRAID_PCIUNL 0x40
#define INTEL_82573E_IDER 0x108D

typedef struct {
  UINT8               ClassCode;
  UINT8               SubClassCode;
  UINT16              VendorId;
  UINT16              DeviceId;
} BAD_DEVICE_TABLE;

BAD_DEVICE_TABLE BadDeviceTable[] = {
                    {(UINT8)PCI_CLASS_MASS_STORAGE,(UINT8)PCI_CLASS_MASS_STORAGE_SCSI,(UINT16)NCR_VENDOR_ID, (UINT16)0xffff}, // Any NCR cards
                    {(UINT8)PCI_CLASS_MASS_STORAGE,(UINT8)PCI_CLASS_MASS_STORAGE_IDE,(UINT16)INTEL_VENDOR_ID, (UINT16)INTEL_82573E_IDER},  // Intel i82573E Tekoa GBit Lan IDE-R
                    {(UINT8)0xff,(UINT8)0xff,(UINT16)0xffff,(UINT16)0xffff}
                  };

EFI_STATUS
PciBusDriverHook (
  )
{
  EFI_STATUS                Status;
  EFI_EVENT                 FilterEvent;

  //
  // Register for callback to PCI I/O protocol
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  PciBusEvent,
                  NULL,
                  &FilterEvent
                  );
  ASSERT_EFI_ERROR(Status);

  //
  // Register for protocol notifications on this event
  //
  Status = gBS->RegisterProtocolNotify (
                  &gEfiPciIoProtocolGuid,
                  FilterEvent,
                  &mPciRegistration
                  );
  ASSERT_EFI_ERROR (Status);

  return  EFI_SUCCESS;
}

VOID
InitBadBars(
  IN    EFI_PCI_IO_PROTOCOL           *PciIo,
  IN    UINT16                        VendorId,
  IN    UINT16                        DeviceId
  )
{

  UINT64                              BaseAddress = 0;
  UINT64                              TempBaseAddress = 0;
  UINT8                               RevId = 0;
  UINT32                              Bar;
  UINT64                              IoSize;
  UINT64                              MemSize;
  UINTN                               MemSizeBits;

  switch ( VendorId) {
    case ATI_VENDOR_ID:
      //
      //  ATI fix-ups. At this time all ATI cards in BadDeviceTable
      //  have same problem in that OPROM BAR needs to be increased.
      //
      Bar = 0x30 ;
      //
      // Get original BAR address
      //
      PciIo->Pci.Read (
                   PciIo,
                   EfiPciIoWidthUint32,
                   Bar,
                   1,
                       (VOID *) &BaseAddress
                   );
      //
      // Find BAR size
      //
      TempBaseAddress = 0xffffffff;
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   Bar,
                   1,
                        (VOID *) &TempBaseAddress
                   );
      PciIo->Pci.Read (
                   PciIo,
                   EfiPciIoWidthUint32,
                   Bar,
                   1,
                       (VOID *) &TempBaseAddress
                   );
      TempBaseAddress &= 0xfffffffe;
      MemSize = 1;
      while ((TempBaseAddress & 0x01) == 0) {
        TempBaseAddress = TempBaseAddress >> 1;
        MemSize = MemSize << 1;
      }

      //
      // Free up allocated memory memory and re-allocate with increased size.
      //
      gDS->FreeMemorySpace (
             BaseAddress,
             MemSize
             );
      //
      // Force new alignment
      //
      MemSize = 0x8000000;
      MemSizeBits = 28;

      gDS->AllocateMemorySpace (
             EfiGcdAllocateAnySearchBottomUp,
             EfiGcdMemoryTypeMemoryMappedIo,
             MemSizeBits,           // Alignment
             MemSize,
             &BaseAddress,
             mImageHandle,
             NULL
             );
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   Bar,
                   1,
                        (VOID *) &BaseAddress
                   );

      break;
    case    NCR_VENDOR_ID:
#define MIN_NCR_IO_SIZE  0x800
#define NCR_GRAN  11  // 2**11 = 0x800
  //
  // NCR SCSI cards like 8250S lie about IO needed. Assign as least 0x80.
  //
  for (Bar = 0x10; Bar < 0x28; Bar+= 4) {

    PciIo->Pci.Read (
                 PciIo,
                 EfiPciIoWidthUint32,
                 Bar,
                 1,
                     (VOID *) &BaseAddress
                 );
    if (BaseAddress && 0x01) {
      TempBaseAddress = 0xffffffff;
      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   Bar,
                   1,
                        (VOID *) &TempBaseAddress
                   );
      TempBaseAddress &= 0xfffffffc;
      IoSize = 1;
      while ((TempBaseAddress & 0x01) == 0) {
        TempBaseAddress = TempBaseAddress >> 1;
        IoSize = IoSize << 1;
      }
      if (IoSize < MIN_NCR_IO_SIZE) {
        gDS->FreeIoSpace (
               BaseAddress,
               IoSize
               );

        gDS->AllocateIoSpace (
               EfiGcdAllocateAnySearchTopDown,
               EfiGcdIoTypeIo,
               NCR_GRAN,           // Alignment
               MIN_NCR_IO_SIZE,
               &BaseAddress,
               mImageHandle,
               NULL
               );
        TempBaseAddress = BaseAddress + 1;
        PciIo->Pci.Write (
                     PciIo,
                     EfiPciIoWidthUint32,
                     Bar,
                     1,
                          (VOID *) &TempBaseAddress
                     );
      }
    }
  }

      break;

    case INTEL_VENDOR_ID:
      if (DeviceId == INTEL_82573E_IDER) {
        //
        //  Tekoa i82573E IDE-R fix-ups. At this time A2 step and earlier parts do not
        //  support any BARs except BAR0. Other BARS will actualy map to BAR0 so disable
        //  them all for Control Blocks and Bus mastering ops as well as Secondary IDE
        //  Controller.
        //  All Tekoa A2 or earlier step chips for now.
        //
        PciIo->Pci.Read (
                     PciIo,
                     EfiPciIoWidthUint8,
                     PCI_REVISION_ID_OFFSET,
                     1,
                     &RevId
                     );
        if (RevId <= 0x02) {
          for (Bar = 0x14; Bar < 0x24; Bar+= 4) {
            //
            // Maybe want to clean this up a bit later but for now just clear out the secondary
            // Bars don't worry aboyut freeing up thge allocs.
            //
            TempBaseAddress = 0x0;
            PciIo->Pci.Write (
                         PciIo,
                         EfiPciIoWidthUint32,
                         Bar,
                         1,
                              (VOID *) &TempBaseAddress
                         );
          } // end for
        }
        else
        {
        	//
          //Tekoa A3 or above:
          //Clear bus master base address (PCI register 0x20)
          //since Tekoa does not fully support IDE Bus Mastering
          //
          TempBaseAddress = 0x0;
          PciIo->Pci.Write (
                       PciIo,
                       EfiPciIoWidthUint32,
                       0x20,
                       1,
                            (VOID *) &TempBaseAddress
                       );
        }
      }
      break;

    default:
      break;
  }
  return;
}

VOID
ProgramPciLatency(
  IN    EFI_PCI_IO_PROTOCOL           *PciIo
  )
{
  //
  // Program Master Latency Timer
  //
  if (mSystemConfiguration.PciLatency != 0) {
     PciIo->Pci.Write (
                  PciIo,
                  EfiPciIoWidthUint8,
                  PCI_LATENCY_TIMER_OFFSET,
                  1,
                  &mSystemConfiguration.PciLatency
                  );
  }
  return;
}

/**
During S5 shutdown, we need to program PME in all LAN devices.
Here we identify LAN devices and save their bus/dev/func.

**/
VOID
SavePciLanAddress(
  IN EFI_PCI_IO_PROTOCOL    *PciIo
  )
{
  EFI_STATUS        Status;
  UINTN             PciSegment,
                    PciBus,
                    PciDevice,
                    PciFunction;
  VOID              *NewBuffer;
  PCI_LAN_INFO      *x;

  Status = PciIo->GetLocation (
                    PciIo,
                    &PciSegment,
                    &PciBus,
                    &PciDevice,
                    &PciFunction
                    );
  if (EFI_ERROR (Status)) {
    return;
  }

  mPciLanCount ++;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  mPciLanCount * sizeof(PCI_LAN_INFO),
                  &NewBuffer
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  if (mPciLanCount > 1) {
    //
    // copy old data into new, larger buffer
    //
    gBS->CopyMem (
           NewBuffer,
           mPciLanInfo,
           (mPciLanCount - 1) * sizeof(PCI_LAN_INFO)
           );

    //
    // free the old memory buffer
    //
    gBS->FreePool (mPciLanInfo);

  }

  //
  // init the new entry
  //
  x = (PCI_LAN_INFO *)NewBuffer + (mPciLanCount - 1);
  x->PciBus = (UINT8)PciBus;
  x->PciDevice = (UINT8)PciDevice;
  x->PciFunction = (UINT8)PciFunction;

  mPciLanInfo = NewBuffer;

  return;
}

/**
  @param  Event          the event that is signaled.
  @param Context        not used here.


**/
VOID
EFIAPI
PciBusEvent (
  IN EFI_EVENT    Event,
  IN VOID*        Context
  )
{

  EFI_STATUS                    Status;
  UINTN                         BufferSize;
  EFI_HANDLE                    Handle;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  PCI_IO_DEVICE                 *PciIoDevice;
  UINT64                        Supports;
  UINTN                         Index;
  UINT8                         mCacheLineSize = 0x10;

  while (TRUE) {
    BufferSize = sizeof (EFI_HANDLE);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    NULL,
                    mPciRegistration,
                    &BufferSize,
                    &Handle
                    );
    if (EFI_ERROR (Status)) {
      //
      // If no more notification events exist
      //
      return;
    }

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiPciIoProtocolGuid,
                    (void **)&PciIo
                    );

    PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (PciIo);

    //
    // Enable I/O for bridge so port 0x80 codes will come out
    //
    if (PciIoDevice->Pci.Hdr.VendorId == V_PCH_INTEL_VENDOR_ID)
    {
      Status = PciIo->Attributes(
                        PciIo,
                        EfiPciIoAttributeOperationSupported,
                        0,
                        &Supports
                        );
      Supports &= EFI_PCI_DEVICE_ENABLE;
      Status = PciIo->Attributes (
                        PciIo,
                        EfiPciIoAttributeOperationEnable,
                        Supports,
                        NULL
                        );
      break;
    }

    //
    // Program PCI Latency Timer
    //
    ProgramPciLatency(PciIo);

    //
    // Program Cache Line Size to 64 bytes (0x10 DWORDs)
    //
    Status = PciIo->Pci.Write (
                          PciIo,
                          EfiPciIoWidthUint8,
                          PCI_CACHELINE_SIZE_OFFSET,
                          1,
                          &mCacheLineSize
                          );

    //
    // If PCI LAN device, save bus/dev/func info
    // so we can program PME during S5 shutdown
    //
    if (PciIoDevice->Pci.Hdr.ClassCode[2] == PCI_CLASS_NETWORK) {
      SavePciLanAddress(PciIo);
      break;
    }

    //
    // Workaround for cards with bad BARs
    //
    Index = 0;
    while (BadDeviceTable[Index].ClassCode != 0xff) {
      if (BadDeviceTable[Index].DeviceId == 0xffff) {
        if ((PciIoDevice->Pci.Hdr.ClassCode[2] == BadDeviceTable[Index].ClassCode) &&
            (PciIoDevice->Pci.Hdr.ClassCode[1] == BadDeviceTable[Index].SubClassCode) &&
            (PciIoDevice->Pci.Hdr.VendorId == BadDeviceTable[Index].VendorId)) {
          InitBadBars(PciIo,BadDeviceTable[Index].VendorId,BadDeviceTable[Index].DeviceId);
        }
      } else {
        if ((PciIoDevice->Pci.Hdr.ClassCode[2] == BadDeviceTable[Index].ClassCode) &&
            (PciIoDevice->Pci.Hdr.ClassCode[1] == BadDeviceTable[Index].SubClassCode) &&
            (PciIoDevice->Pci.Hdr.VendorId == BadDeviceTable[Index].VendorId) &&
            (PciIoDevice->Pci.Hdr.DeviceId == BadDeviceTable[Index].DeviceId)) {

          InitBadBars(PciIo,BadDeviceTable[Index].VendorId,BadDeviceTable[Index].DeviceId);
        }
      }
      ++Index;
    }
      break;
    }

  return;
}

