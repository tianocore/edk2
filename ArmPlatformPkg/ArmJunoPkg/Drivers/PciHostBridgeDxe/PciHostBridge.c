/** @file
*  Pci Host Bridge support for the Xpress-RICH3 PCIe Root Complex
*
*  Copyright (c) 2011-2015, ARM Ltd. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "PciHostBridge.h"

#include <Guid/EventGroup.h>

/**
 * PCI Root Bridge Description
 */
typedef struct {
  UINT32  AcpiUid;
  UINT64  MemAllocAttributes;
} PCI_ROOT_BRIDGE_DESC;

PCI_ROOT_BRIDGE_DESC PciRbDescriptions = {
    0,                                  // AcpiUid
    PCI_MEMORY_ALLOCATION_ATTRIBUTES    // MemAllocAttributes
};

/**
 * Template for PCI Host Bridge Instance
 **/
STATIC CONST PCI_HOST_BRIDGE_INSTANCE
gPciHostBridgeInstanceTemplate = {
  PCI_HOST_BRIDGE_SIGNATURE,      //Signature
  NULL,                           // Handle
  NULL,                           // ImageHandle
  NULL,                           // RootBridge
  TRUE,                           // CanRestarted
  NULL,                           // CpuIo
  NULL,                           // Metronome
  {                               // ResAlloc
    PciHbRaNotifyPhase,           //   ResAlloc.NotifyPhase
    PciHbRaGetNextRootBridge,     //   ResAlloc.GetNextRootBridge
    PciHbRaGetAllocAttributes,    //   ResAlloc.GetAllocAttributes
    PciHbRaStartBusEnumeration,   //   ResAlloc.StartBusEnumeration
    PciHbRaSetBusNumbers,         //   ResAlloc.SetBusNumbers
    PciHbRaSubmitResources,       //   ResAlloc.SubmitResources
    PciHbRaGetProposedResources,  //   ResAlloc.GetProposedResources
    PciHbRaPreprocessController   //   ResAlloc.PreprocessController
  }
};
PCI_HOST_BRIDGE_INSTANCE* gpPciHostBridgeInstance;

EFI_STATUS
HostBridgeConstructor (
  IN OUT PCI_HOST_BRIDGE_INSTANCE** Instance,
  IN  EFI_HANDLE                    ImageHandle
  )
{
  EFI_STATUS                  Status;
  PCI_HOST_BRIDGE_INSTANCE*   HostBridge;

  PCI_TRACE ("HostBridgeConstructor()");

  if (Instance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HostBridge = AllocateCopyPool (sizeof (PCI_HOST_BRIDGE_INSTANCE), &gPciHostBridgeInstanceTemplate);
  if (HostBridge == NULL) {
    PCI_TRACE ("HostBridgeConstructor(): FAIL to allocate resources");
    return EFI_OUT_OF_RESOURCES;
  }

  // It will also create a device handle for the PCI Host Bridge (as HostBridge->Handle == NULL)
  Status = gBS->InstallMultipleProtocolInterfaces (
                    &HostBridge->Handle,
                    &gEfiPciHostBridgeResourceAllocationProtocolGuid, &HostBridge->ResAlloc,
                    NULL
                    );
  if (EFI_ERROR (Status)) {
    PCI_TRACE ("HostBridgeConstructor(): FAIL to install resource allocator");
    FreePool (HostBridge);
    return EFI_DEVICE_ERROR;
  } else {
    PCI_TRACE ("HostBridgeConstructor(): SUCCEED to install resource allocator");
  }

  Status = gBS->LocateProtocol (&gEfiCpuIo2ProtocolGuid, NULL, (VOID **)(&(HostBridge->CpuIo)));
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiMetronomeArchProtocolGuid, NULL, (VOID **)(&(HostBridge->Metronome)));
  ASSERT_EFI_ERROR (Status);

  HostBridge->ImageHandle = ImageHandle;

  *Instance = HostBridge;
  return EFI_SUCCESS;
}

EFI_STATUS
HostBridgeDestructor (
  IN PCI_HOST_BRIDGE_INSTANCE* HostBridge
  )
{
  EFI_STATUS Status;

  Status = gBS->UninstallMultipleProtocolInterfaces (
                      HostBridge->Handle,
                      &gEfiPciHostBridgeResourceAllocationProtocolGuid, &HostBridge->ResAlloc,
                      NULL
                      );

  if (HostBridge->RootBridge) {
    PciRbDestructor (HostBridge->RootBridge);
  }

  FreePool (HostBridge);

  return Status;
}

/**
  Entry point of this driver

  @param ImageHandle     Handle of driver image
  @param SystemTable     Point to EFI_SYSTEM_TABLE

  @retval EFI_OUT_OF_RESOURCES  Can not allocate memory resource
  @retval EFI_DEVICE_ERROR      Can not install the protocol instance
  @retval EFI_SUCCESS           Success to initialize the Pci host bridge.
**/
EFI_STATUS
EFIAPI
PciHostBridgeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;

  PCI_TRACE ("PciHostBridgeEntryPoint()");

  // Creation of the PCI Host Bridge Instance
  Status = HostBridgeConstructor (&gpPciHostBridgeInstance, ImageHandle);
  if (EFI_ERROR (Status)) {
    PCI_TRACE ("PciHostBridgeEntryPoint(): ERROR: Fail to construct PCI Host Bridge.");
    return Status;
  }

  // Creation of the PCIe Root Bridge
  Status = PciRbConstructor (gpPciHostBridgeInstance, PciRbDescriptions.AcpiUid, PciRbDescriptions.MemAllocAttributes);
  if (EFI_ERROR (Status)) {
    PCI_TRACE ("PciHostBridgeEntryPoint(): ERROR: Fail to construct PCI Root Bridge.");
    return Status;
  }
  ASSERT (gpPciHostBridgeInstance->RootBridge->Signature == PCI_ROOT_BRIDGE_SIGNATURE);

  // PCI 32bit Memory Space
  Status = gDS->AddMemorySpace (
    EfiGcdMemoryTypeMemoryMappedIo,
    PCI_MEM32_BASE,
    PCI_MEM32_SIZE,
    0
  );

  // PCI 64bit Memory Space
  Status = gDS->AddMemorySpace (
    EfiGcdMemoryTypeMemoryMappedIo,
    PCI_MEM64_BASE,
    PCI_MEM64_SIZE,
    0
  );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciHostBridgeUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS Status;

  // Free Reserved memory space in GCD
  gDS->RemoveMemorySpace (PCI_MEM32_BASE, PCI_MEM32_SIZE);
  gDS->RemoveMemorySpace (PCI_MEM64_BASE, PCI_MEM64_SIZE);

  // Free the allocated memory
  Status = HostBridgeDestructor (gpPciHostBridgeInstance);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
