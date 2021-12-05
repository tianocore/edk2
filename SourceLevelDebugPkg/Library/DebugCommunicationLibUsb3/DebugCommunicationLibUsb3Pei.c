/** @file
  Debug Port Library implementation based on usb3 debug port.

  Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/IoMmu.h>
#include "DebugCommunicationLibUsb3Internal.h"

GUID  gUsb3DbgGuid = USB3_DBG_GUID;

/**
  USB3 IOMMU PPI notify.

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
Usb3IoMmuPpiNotify (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN VOID                       *Ppi
  )
{
  USB3_DEBUG_PORT_HANDLE  *Instance;

  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));

  Instance = GetUsb3DebugPortInstance ();
  ASSERT (Instance != NULL);
  if (!Instance->Ready) {
    return EFI_SUCCESS;
  }

  Instance->InNotify = TRUE;

  //
  // Reinitialize USB3 debug port with granted DMA buffer from IOMMU PPI.
  //
  InitializeUsbDebugHardware (Instance);

  //
  // Wait some time for host to be ready after re-initialization.
  //
  MicroSecondDelay (1000000);

  Instance->InNotify = FALSE;

  return EFI_SUCCESS;
}

EFI_PEI_NOTIFY_DESCRIPTOR  mUsb3IoMmuPpiNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiIoMmuPpiGuid,
  Usb3IoMmuPpiNotify
};

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param IoMmu                  Pointer to IOMMU PPI.
  @param Pages                  The number of pages to allocate.
  @param HostAddress            A pointer to store the base system memory address of the
                                allocated range.
  @param DeviceAddress          The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param Mapping                A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
IoMmuAllocateBuffer (
  IN EDKII_IOMMU_PPI        *IoMmu,
  IN UINTN                  Pages,
  OUT VOID                  **HostAddress,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  )
{
  EFI_STATUS  Status;
  UINTN       NumberOfBytes;

  *HostAddress   = NULL;
  *DeviceAddress = 0;
  *Mapping       = NULL;

  Status = IoMmu->AllocateBuffer (
                    IoMmu,
                    EfiRuntimeServicesData,
                    Pages,
                    HostAddress,
                    0
                    );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  NumberOfBytes = EFI_PAGES_TO_SIZE (Pages);
  Status        = IoMmu->Map (
                           IoMmu,
                           EdkiiIoMmuOperationBusMasterCommonBuffer,
                           *HostAddress,
                           &NumberOfBytes,
                           DeviceAddress,
                           Mapping
                           );
  if (EFI_ERROR (Status)) {
    IoMmu->FreeBuffer (IoMmu, Pages, *HostAddress);
    *HostAddress = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  Status = IoMmu->SetAttribute (
                    IoMmu,
                    *Mapping,
                    EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE
                    );
  if (EFI_ERROR (Status)) {
    IoMmu->Unmap (IoMmu, *Mapping);
    IoMmu->FreeBuffer (IoMmu, Pages, *HostAddress);
    *Mapping     = NULL;
    *HostAddress = NULL;
    return Status;
  }

  return Status;
}

/**
  USB3 get IOMMU PPI.

  @return Pointer to IOMMU PPI.

**/
EDKII_IOMMU_PPI *
Usb3GetIoMmu (
  VOID
  )
{
  EFI_STATUS       Status;
  EDKII_IOMMU_PPI  *IoMmu;

  IoMmu  = NULL;
  Status = PeiServicesLocatePpi (
             &gEdkiiIoMmuPpiGuid,
             0,
             NULL,
             (VOID **)&IoMmu
             );
  if (!EFI_ERROR (Status) && (IoMmu != NULL)) {
    return IoMmu;
  }

  return NULL;
}

/**
  Return USB3 debug instance address pointer.

**/
EFI_PHYSICAL_ADDRESS *
GetUsb3DebugPortInstanceAddrPtr (
  VOID
  )
{
  USB3_DEBUG_PORT_HANDLE  *Instance;
  EFI_PHYSICAL_ADDRESS    *AddrPtr;
  EFI_PEI_HOB_POINTERS    Hob;
  EFI_STATUS              Status;

  Hob.Raw = GetFirstGuidHob (&gUsb3DbgGuid);
  if (Hob.Raw == NULL) {
    //
    // Build HOB for the local instance and the buffer to save instance address pointer.
    // Use the local instance in HOB temporarily.
    //
    AddrPtr = BuildGuidHob (
                &gUsb3DbgGuid,
                sizeof (EFI_PHYSICAL_ADDRESS) + sizeof (USB3_DEBUG_PORT_HANDLE)
                );
    ASSERT (AddrPtr != NULL);
    ZeroMem (AddrPtr, sizeof (EFI_PHYSICAL_ADDRESS) + sizeof (USB3_DEBUG_PORT_HANDLE));
    Instance              = (USB3_DEBUG_PORT_HANDLE *)(AddrPtr + 1);
    *AddrPtr              = (EFI_PHYSICAL_ADDRESS)(UINTN)Instance;
    Instance->FromHob     = TRUE;
    Instance->Initialized = USB3DBG_UNINITIALIZED;
    if (Usb3GetIoMmu () == NULL) {
      Status = PeiServicesNotifyPpi (&mUsb3IoMmuPpiNotifyDesc);
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    AddrPtr = GET_GUID_HOB_DATA (Hob.Guid);
  }

  return AddrPtr;
}

/**
  Allocate aligned memory for XHC's usage.

  @param BufferSize     The size, in bytes, of the Buffer.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
AllocateAlignBuffer (
  IN UINTN  BufferSize
  )
{
  VOID                  *Buf;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID                  *MemoryDiscoveredPpi;
  EDKII_IOMMU_PPI       *IoMmu;
  VOID                  *HostAddress;
  VOID                  *Mapping;

  Buf = NULL;

  //
  // Make sure the allocated memory is physical memory.
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiMemoryDiscoveredPpiGuid,
             0,
             NULL,
             (VOID **)&MemoryDiscoveredPpi
             );
  if (!EFI_ERROR (Status)) {
    IoMmu = Usb3GetIoMmu ();
    if (IoMmu != NULL) {
      Status = IoMmuAllocateBuffer (
                 IoMmu,
                 EFI_SIZE_TO_PAGES (BufferSize),
                 &HostAddress,
                 &Address,
                 &Mapping
                 );
      if (!EFI_ERROR (Status)) {
        ASSERT (Address == ((EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress));
        Buf = (VOID *)(UINTN)Address;
      }
    } else {
      Status = PeiServicesAllocatePages (
                 EfiACPIMemoryNVS,
                 EFI_SIZE_TO_PAGES (BufferSize),
                 &Address
                 );
      if (!EFI_ERROR (Status)) {
        Buf = (VOID *)(UINTN)Address;
      }
    }
  }

  return Buf;
}
