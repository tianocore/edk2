/** @file
  Debug Port Library implementation based on usb3 debug port.

  Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HobLib.h>
#include <Protocol/PciIo.h>
#include <Protocol/IoMmu.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include "DebugCommunicationLibUsb3Internal.h"

GUID                        gUsb3DbgGuid =  USB3_DBG_GUID;

USB3_DEBUG_PORT_HANDLE      mUsb3Instance = {USB3DBG_UNINITIALIZED};
EFI_PHYSICAL_ADDRESS        mUsb3InstanceAddr = 0;
EFI_PHYSICAL_ADDRESS        *mUsb3InstanceAddrPtr = NULL;
EFI_PCI_IO_PROTOCOL         *mUsb3PciIo = NULL;

/**
  Creates a named event that can be signaled.

  This function creates an event using NotifyTpl, NoifyFunction.
  If Name is NULL, then ASSERT().
  If NotifyTpl is not a legal TPL value, then ASSERT().
  If NotifyFunction is NULL, then ASSERT().

  @param  Name                  Supplies the GUID name of the event.
  @param  NotifyTpl             Supplies the task priority level of the event notifications.
  @param  NotifyFunction        Supplies the function to notify when the event is signaled.
  @param  Event                 A pointer to the event created.

  @retval EFI_SUCCESS           A named event was created.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resource to create the named event.

**/
EFI_STATUS
EFIAPI
Usb3NamedEventListen (
  IN CONST EFI_GUID    *Name,
  IN EFI_TPL           NotifyTpl,
  IN EFI_EVENT_NOTIFY  NotifyFunction,
  IN EFI_EVENT         *Event
  )
{
  EFI_STATUS  Status;
  VOID        *RegistrationLocal;

  ASSERT (Name != NULL);
  ASSERT (NotifyFunction != NULL);
  ASSERT (NotifyTpl <= TPL_HIGH_LEVEL);

  //
  // Create event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  NotifyTpl,
                  NotifyFunction,
                  NULL,
                  Event
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for an installation of protocol interface
  //
  Status = gBS->RegisterProtocolNotify (
                  (EFI_GUID *) Name,
                  *Event,
                  &RegistrationLocal
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  USB3 map one DMA buffer.

  @param PciIo          Pointer to PciIo for USB3 debug port.
  @param Address        DMA buffer address to be mapped.
  @param NumberOfBytes  Number of bytes to be mapped.

**/
VOID
Usb3MapOneDmaBuffer (
  IN EFI_PCI_IO_PROTOCOL        *PciIo,
  IN EFI_PHYSICAL_ADDRESS       Address,
  IN UINTN                      NumberOfBytes
  )
{
  EFI_STATUS                    Status;
  VOID                          *HostAddress;
  EFI_PHYSICAL_ADDRESS          DeviceAddress;
  VOID                          *Mapping;

  HostAddress = (VOID *) (UINTN) Address;
  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    HostAddress,
                    &NumberOfBytes,
                    &DeviceAddress,
                    &Mapping
                    );
  ASSERT_EFI_ERROR (Status);
  ASSERT (DeviceAddress == ((EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress));
}

/**
  USB3 map DMA buffers.

  @param Instance       Pointer to USB3 debug port instance.
  @param PciIo          Pointer to PciIo for USB3 debug port.

**/
VOID
Usb3MapDmaBuffers (
  IN USB3_DEBUG_PORT_HANDLE     *Instance,
  IN EFI_PCI_IO_PROTOCOL        *PciIo
  )
{
  Usb3MapOneDmaBuffer (
    PciIo,
    Instance->UrbIn.Data,
    XHCI_DEBUG_DEVICE_MAX_PACKET_SIZE * 2 + USB3_DEBUG_PORT_WRITE_MAX_PACKET_SIZE
    );

  Usb3MapOneDmaBuffer (
    PciIo,
    Instance->TransferRingIn.RingSeg0,
    sizeof (TRB_TEMPLATE) * TR_RING_TRB_NUMBER
    );

  Usb3MapOneDmaBuffer (
    PciIo,
    Instance->TransferRingOut.RingSeg0,
    sizeof (TRB_TEMPLATE) * TR_RING_TRB_NUMBER
    );

  Usb3MapOneDmaBuffer (
    PciIo,
    Instance->EventRing.EventRingSeg0,
    sizeof (TRB_TEMPLATE) * EVENT_RING_TRB_NUMBER
    );

  Usb3MapOneDmaBuffer (
    PciIo,
    Instance->EventRing.ERSTBase,
    sizeof (EVENT_RING_SEG_TABLE_ENTRY) * ERST_NUMBER
    );

  Usb3MapOneDmaBuffer (
    PciIo,
    Instance->DebugCapabilityContext,
    sizeof (XHC_DC_CONTEXT)
    );

  Usb3MapOneDmaBuffer (
    PciIo,
    ((XHC_DC_CONTEXT *) (UINTN) Instance->DebugCapabilityContext)->DbcInfoContext.String0DescAddress,
    STRING0_DESC_LEN + MANU_DESC_LEN + PRODUCT_DESC_LEN + SERIAL_DESC_LEN
    );
}

/**
  Invoke a notification event

  @param[in] Event              Event whose notification function is being invoked.
  @param[in] Context            The pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
Usb3DxeSmmReadyToLockNotify (
  IN EFI_EVENT                  Event,
  IN VOID                       *Context
  )
{
  USB3_DEBUG_PORT_HANDLE        *Instance;

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  Instance = GetUsb3DebugPortInstance ();
  ASSERT (Instance != NULL);

  Instance->InNotify = TRUE;

  //
  // For the case that the USB3 debug port instance and DMA buffers are
  // from PEI HOB with IOMMU enabled.
  // Reinitialize USB3 debug port with granted DXE DMA buffer accessible
  // by SMM environment.
  //
  InitializeUsbDebugHardware (Instance);

  //
  // Wait some time for host to be ready after re-initialization.
  //
  MicroSecondDelay (1000000);

  Instance->InNotify = FALSE;
  gBS->CloseEvent (Event);
}

/**
  USB3 get IOMMU protocol.

  @return Pointer to IOMMU protocol.

**/
EDKII_IOMMU_PROTOCOL *
Usb3GetIoMmu (
  VOID
  )
{
  EFI_STATUS                Status;
  EDKII_IOMMU_PROTOCOL      *IoMmu;

  IoMmu = NULL;
  Status = gBS->LocateProtocol (
             &gEdkiiIoMmuProtocolGuid,
             NULL,
             (VOID **) &IoMmu
             );
  if (!EFI_ERROR (Status) && (IoMmu != NULL)) {
    return IoMmu;
  }

  return NULL;
}

/**
  Invoke a notification event

  @param[in] Event              Event whose notification function is being invoked.
  @param[in] Context            The pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
Usb3PciIoNotify (
  IN  EFI_EVENT                 Event,
  IN  VOID                      *Context
  )
{
  EFI_STATUS                    Status;
  UINTN                         PciIoHandleCount;
  EFI_HANDLE                    *PciIoHandleBuffer;
  UINTN                         Index;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  UINTN                         PciSegment;
  UINTN                         PciBusNumber;
  UINTN                         PciDeviceNumber;
  UINTN                         PciFunctionNumber;
  UINT32                        PciAddress;
  USB3_DEBUG_PORT_HANDLE        *Instance;
  EFI_EVENT                     SmmReadyToLockEvent;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &PciIoHandleCount,
                  &PciIoHandleBuffer
                  );
  if (!EFI_ERROR (Status) &&
      (PciIoHandleBuffer != NULL) &&
      (PciIoHandleCount != 0)) {
    for (Index = 0; Index < PciIoHandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      PciIoHandleBuffer[Index],
                      &gEfiPciIoProtocolGuid,
                      (VOID **) &PciIo
                      );
      ASSERT_EFI_ERROR (Status);
      Status = PciIo->GetLocation (PciIo, &PciSegment, &PciBusNumber, &PciDeviceNumber, &PciFunctionNumber);
      ASSERT_EFI_ERROR (Status);
      PciAddress = (UINT32) ((PciBusNumber << 20) | (PciDeviceNumber << 15) | (PciFunctionNumber << 12));
      if (PciAddress == PcdGet32(PcdUsbXhciPciAddress)) {
        //
        // Found the PciIo for USB3 debug port.
        //
        DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));
        if (Usb3GetIoMmu () != NULL) {
          Instance = GetUsb3DebugPortInstance ();
          ASSERT (Instance != NULL);
          if (Instance->Ready) {
            Instance->InNotify = TRUE;
            Usb3MapDmaBuffers (Instance, PciIo);
            Instance->InNotify = FALSE;

            if (Instance->FromHob) {
              mUsb3PciIo = PciIo;
              Usb3NamedEventListen (
                &gEfiDxeSmmReadyToLockProtocolGuid,
                TPL_NOTIFY,
                Usb3DxeSmmReadyToLockNotify,
                &SmmReadyToLockEvent
                );
            }
          }
        }
        gBS->CloseEvent (Event);
        break;
      }
    }

    gBS->FreePool (PciIoHandleBuffer);
  }
}

/**
  Return USB3 debug instance address pointer.

**/
EFI_PHYSICAL_ADDRESS *
GetUsb3DebugPortInstanceAddrPtr (
  VOID
  )
{
  if (mUsb3InstanceAddrPtr == NULL) {
    //
    // Use the local variables temporarily.
    //
    mUsb3InstanceAddr = (EFI_PHYSICAL_ADDRESS) (UINTN) &mUsb3Instance;
    mUsb3InstanceAddrPtr = &mUsb3InstanceAddr;
  }
  return mUsb3InstanceAddrPtr;
}

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param PciIo                  Pointer to PciIo for USB3 debug port.
  @param Pages                  The number of pages to allocate.
  @param Address                A pointer to store the base system memory address of the
                                allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
Usb3AllocateDmaBuffer (
  IN EFI_PCI_IO_PROTOCOL    *PciIo,
  IN UINTN                  Pages,
  OUT VOID                  **Address
  )
{
  EFI_STATUS            Status;

  *Address = NULL;
  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiRuntimeServicesData,
                    Pages,
                    Address,
                    0
                    );
  if (!EFI_ERROR (Status)) {
    Usb3MapOneDmaBuffer (
      PciIo,
      (EFI_PHYSICAL_ADDRESS) (UINTN) *Address,
      EFI_PAGES_TO_SIZE (Pages)
      );
  }
  return Status;
}

/**
  Allocate aligned memory for XHC's usage.

  @param  BufferSize      The size, in bytes, of the Buffer.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID*
AllocateAlignBuffer (
  IN UINTN                    BufferSize
  )
{
  EFI_PHYSICAL_ADDRESS    TmpAddr;
  EFI_STATUS              Status;
  VOID                    *Buf;

  Buf = NULL;

  if (gBS != NULL) {
    if (mUsb3PciIo != NULL) {
      Usb3AllocateDmaBuffer (
        mUsb3PciIo,
        EFI_SIZE_TO_PAGES (BufferSize),
        &Buf
        );
    } else {
      TmpAddr = 0xFFFFFFFF;
      Status = gBS->AllocatePages (
                      AllocateMaxAddress,
                      EfiACPIMemoryNVS,
                      EFI_SIZE_TO_PAGES (BufferSize),
                      &TmpAddr
                      );
      if (!EFI_ERROR (Status)) {
        Buf = (VOID *) (UINTN) TmpAddr;
      }
    }
  }

  return Buf;
}

/**
  The constructor function initialize USB3 debug port.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DebugCommunicationUsb3DxeConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_PHYSICAL_ADDRESS          *AddrPtr;
  USB3_DEBUG_PORT_HANDLE        *Instance;
  EFI_PHYSICAL_ADDRESS          Address;
  EFI_STATUS                    Status;
  EFI_EVENT                     Event;

  Status = EfiGetSystemConfigurationTable (&gUsb3DbgGuid, (VOID **) &AddrPtr);
  if (EFI_ERROR (Status)) {
    //
    // Instead of using local variables, install system configuration table for
    // the local instance and the buffer to save instance address pointer.
    //
    Address = SIZE_4GB;
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiACPIMemoryNVS,
                    EFI_SIZE_TO_PAGES (sizeof (EFI_PHYSICAL_ADDRESS) + sizeof (USB3_DEBUG_PORT_HANDLE)),
                    &Address
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    AddrPtr = (EFI_PHYSICAL_ADDRESS *) (UINTN) Address;
    ZeroMem (AddrPtr, sizeof (EFI_PHYSICAL_ADDRESS) + sizeof (USB3_DEBUG_PORT_HANDLE));
    Instance = (USB3_DEBUG_PORT_HANDLE *) (AddrPtr + 1);
    CopyMem (Instance, &mUsb3Instance, sizeof (USB3_DEBUG_PORT_HANDLE));
    *AddrPtr = (EFI_PHYSICAL_ADDRESS) (UINTN) Instance;

    Status = gBS->InstallConfigurationTable (&gUsb3DbgGuid, AddrPtr);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (mUsb3InstanceAddrPtr != NULL) {
    *AddrPtr = *mUsb3InstanceAddrPtr;
  }
  mUsb3InstanceAddrPtr = AddrPtr;

  Instance = GetUsb3DebugPortInstance ();
  ASSERT (Instance != NULL);

  if (Instance->PciIoEvent == 0) {
    Status = Usb3NamedEventListen (
               &gEfiPciIoProtocolGuid,
               TPL_NOTIFY,
               Usb3PciIoNotify,
               &Event
               );
    if (!EFI_ERROR (Status)) {
      Instance->PciIoEvent = (EFI_PHYSICAL_ADDRESS) (UINTN) Event;
    }
  }

  return EFI_SUCCESS;
}

/**
  The destructor function.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DebugCommunicationUsb3DxeDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  USB3_DEBUG_PORT_HANDLE        *Instance;

  Instance = GetUsb3DebugPortInstance ();
  ASSERT (Instance != NULL);

  if (Instance->PciIoEvent != 0) {
    //
    // Close the event created.
    //
    gBS->CloseEvent ((EFI_EVENT) (UINTN) Instance->PciIoEvent);
    Instance->PciIoEvent = 0;
  }
  return EFI_SUCCESS;
}

