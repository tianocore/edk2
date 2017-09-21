/** @file
  This driver implements EFI_PCI_HOT_PLUG_INIT_PROTOCOL, providing the PCI bus
  driver with resource padding information, for PCIe hotplug purposes.

  Copyright (C) 2016, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <IndustryStandard/Acpi10.h>

#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/PciHotPlugInit.h>
#include <Protocol/PciRootBridgeIo.h>

//
// The protocol interface this driver produces.
//
// Refer to 12.6 "PCI Hot Plug PCI Initialization Protocol" in the Platform
// Init 1.4a Spec, Volume 5.
//
STATIC EFI_PCI_HOT_PLUG_INIT_PROTOCOL mPciHotPlugInit;


//
// Resource padding template for the GetResourcePadding() protocol member
// function.
//
// Refer to Table 8 "ACPI 2.0 & 3.0 QWORD Address Space Descriptor Usage" in
// the Platform Init 1.4a Spec, Volume 5.
//
// This structure is interpreted by the ApplyResourcePadding() function in the
// edk2 PCI Bus UEFI_DRIVER.
//
#pragma pack (1)
typedef struct {
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR MmioPadding;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR IoPadding;
  EFI_ACPI_END_TAG_DESCRIPTOR       EndDesc;
} RESOURCE_PADDING;
#pragma pack ()

STATIC CONST RESOURCE_PADDING mPadding = {
  //
  // MmioPadding
  //
  {
    ACPI_ADDRESS_SPACE_DESCRIPTOR,                 // Desc
    (UINT16)(                                      // Len
      sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) -
      OFFSET_OF (
        EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR,
        ResType
        )
      ),
    ACPI_ADDRESS_SPACE_TYPE_MEM, // ResType
    0,                           // GenFlag:
                                 //   ignored
    0,                           // SpecificFlag:
                                 //   non-prefetchable
    32,                          // AddrSpaceGranularity:
                                 //   reserve 32-bit aperture
    0,                           // AddrRangeMin:
                                 //   ignored
    SIZE_2MB - 1,                // AddrRangeMax:
                                 //   align at 2MB
    0,                           // AddrTranslationOffset:
                                 //   ignored
    SIZE_2MB                     // AddrLen:
                                 //   2MB padding
  },

  //
  // IoPadding
  //
  {
    ACPI_ADDRESS_SPACE_DESCRIPTOR,                 // Desc
    (UINT16)(                                      // Len
      sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) -
      OFFSET_OF (
        EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR,
        ResType
        )
      ),
    ACPI_ADDRESS_SPACE_TYPE_IO,// ResType
    0,                          // GenFlag:
                                //   ignored
    0,                          // SpecificFlag:
                                //   ignored
    0,                          // AddrSpaceGranularity:
                                //   ignored
    0,                          // AddrRangeMin:
                                //   ignored
    512 - 1,                    // AddrRangeMax:
                                //   align at 512 IO ports
    0,                          // AddrTranslationOffset:
                                //   ignored
    512                         // AddrLen:
                                //   512 IO ports
  },

  //
  // EndDesc
  //
  {
    ACPI_END_TAG_DESCRIPTOR, // Desc
    0                        // Checksum: to be ignored
  }
};


/**
  Returns a list of root Hot Plug Controllers (HPCs) that require
  initialization during the boot process.

  This procedure returns a list of root HPCs. The PCI bus driver must
  initialize  these controllers during the boot process. The PCI bus driver may
  or may not be  able to detect these HPCs. If the platform includes a
  PCI-to-CardBus bridge, it  can be included in this list if it requires
  initialization.  The HpcList must be  self consistent. An HPC cannot control
  any of its parent buses. Only one HPC can  control a PCI bus. Because this
  list includes only root HPCs, no HPC in the list  can be a child of another
  HPC. This policy must be enforced by the  EFI_PCI_HOT_PLUG_INIT_PROTOCOL.
  The PCI bus driver may not check for such  invalid conditions.  The callee
  allocates the buffer HpcList

  @param[in]  This       Pointer to the EFI_PCI_HOT_PLUG_INIT_PROTOCOL
                         instance.
  @param[out] HpcCount   The number of root HPCs that were returned.
  @param[out] HpcList    The list of root HPCs. HpcCount defines the number of
                         elements in this list.

  @retval EFI_SUCCESS             HpcList was returned.
  @retval EFI_OUT_OF_RESOURCES    HpcList was not returned due to insufficient
                                  resources.
  @retval EFI_INVALID_PARAMETER   HpcCount is NULL or HpcList is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
GetRootHpcList (
  IN  EFI_PCI_HOT_PLUG_INIT_PROTOCOL *This,
  OUT UINTN                          *HpcCount,
  OUT EFI_HPC_LOCATION               **HpcList
  )
{
  if (HpcCount == NULL || HpcList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // There are no top-level (i.e., un-enumerable) hot-plug controllers in QEMU
  // that would require special initialization.
  //
  *HpcCount = 0;
  *HpcList = NULL;
  return EFI_SUCCESS;
}


/**
  Initializes one root Hot Plug Controller (HPC). This process may causes
  initialization of its subordinate buses.

  This function initializes the specified HPC. At the end of initialization,
  the hot-plug slots or sockets (controlled by this HPC) are powered and are
  connected to the bus. All the necessary registers in the HPC are set up. For
  a Standard (PCI) Hot Plug Controller (SHPC), the registers that must be set
  up are defined in the PCI Standard Hot Plug Controller and Subsystem
  Specification.

  @param[in]  This            Pointer to the EFI_PCI_HOT_PLUG_INIT_PROTOCOL
                              instance.
  @param[in]  HpcDevicePath   The device path to the HPC that is being
                              initialized.
  @param[in]  HpcPciAddress   The address of the HPC function on the PCI bus.
  @param[in]  Event           The event that should be signaled when the HPC
                              initialization is complete.  Set to NULL if the
                              caller wants to wait until the entire
                              initialization  process is complete.
  @param[out] HpcState        The state of the HPC hardware. The state is
                              EFI_HPC_STATE_INITIALIZED or
                              EFI_HPC_STATE_ENABLED.

  @retval EFI_SUCCESS             If Event is NULL, the specific HPC was
                                  successfully initialized. If Event is not
                                  NULL, Event will be  signaled at a later time
                                  when initialization is complete.
  @retval EFI_UNSUPPORTED         This instance of
                                  EFI_PCI_HOT_PLUG_INIT_PROTOCOL does not
                                  support the specified HPC.
  @retval EFI_OUT_OF_RESOURCES    Initialization failed due to insufficient
                                  resources.
  @retval EFI_INVALID_PARAMETER   HpcState is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
InitializeRootHpc (
  IN  EFI_PCI_HOT_PLUG_INIT_PROTOCOL *This,
  IN  EFI_DEVICE_PATH_PROTOCOL       *HpcDevicePath,
  IN  UINT64                         HpcPciAddress,
  IN  EFI_EVENT                      Event, OPTIONAL
  OUT EFI_HPC_STATE                  *HpcState
  )
{
  //
  // This function should never be called, due to the information returned by
  // GetRootHpcList().
  //
  ASSERT (FALSE);

  if (HpcState == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_UNSUPPORTED;
}


/**
  Returns the resource padding that is required by the PCI bus that is
  controlled by the specified Hot Plug Controller (HPC).

  This function returns the resource padding that is required by the PCI bus
  that is controlled by the specified HPC. This member function is called for
  all the  root HPCs and nonroot HPCs that are detected by the PCI bus
  enumerator. This  function will be called before PCI resource allocation is
  completed. This function  must be called after all the root HPCs, with the
  possible exception of a  PCI-to-CardBus bridge, have completed
  initialization.

  @param[in]  This            Pointer to the EFI_PCI_HOT_PLUG_INIT_PROTOCOL
                              instance.
  @param[in]  HpcDevicePath   The device path to the HPC.
  @param[in]  HpcPciAddress   The address of the HPC function on the PCI bus.
  @param[in]  HpcState        The state of the HPC hardware.
  @param[out] Padding         The amount of resource padding that is required
                              by the PCI bus under the control of the specified
                              HPC.
  @param[out] Attributes      Describes how padding is accounted for. The
                              padding is returned in the form of ACPI 2.0
                              resource descriptors.

  @retval EFI_SUCCESS             The resource padding was successfully
                                  returned.
  @retval EFI_UNSUPPORTED         This instance of the
                                  EFI_PCI_HOT_PLUG_INIT_PROTOCOL does not
                                  support the specified HPC.
  @retval EFI_NOT_READY           This function was called before HPC
                                  initialization is complete.
  @retval EFI_INVALID_PARAMETER   HpcState or Padding or Attributes is NULL.
  @retval EFI_OUT_OF_RESOURCES    ACPI 2.0 resource descriptors for Padding
                                  cannot be allocated due to insufficient
                                  resources.
**/
STATIC
EFI_STATUS
EFIAPI
GetResourcePadding (
  IN  EFI_PCI_HOT_PLUG_INIT_PROTOCOL *This,
  IN  EFI_DEVICE_PATH_PROTOCOL       *HpcDevicePath,
  IN  UINT64                         HpcPciAddress,
  OUT EFI_HPC_STATE                  *HpcState,
  OUT VOID                           **Padding,
  OUT EFI_HPC_PADDING_ATTRIBUTES     *Attributes
  )
{
  DEBUG_CODE (
    EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *Address;
    CHAR16                                      *DevicePathString;

    Address = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *)&HpcPciAddress;
    DevicePathString = ConvertDevicePathToText (HpcDevicePath, FALSE, FALSE);

    DEBUG ((EFI_D_VERBOSE, "%a: Address=%02x:%02x.%x DevicePath=%s\n",
      __FUNCTION__, Address->Bus, Address->Device, Address->Function,
      (DevicePathString == NULL) ? L"<unavailable>" : DevicePathString));

    if (DevicePathString != NULL) {
      FreePool (DevicePathString);
    }
    );

  if (HpcState == NULL || Padding == NULL || Attributes == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Padding = AllocateCopyPool (sizeof mPadding, &mPadding);
  if (*Padding == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Resource padding is required.
  //
  *HpcState = EFI_HPC_STATE_INITIALIZED | EFI_HPC_STATE_ENABLED;

  //
  // The padding should be applied at PCI bus level, and considered by upstream
  // bridges, recursively.
  //
  *Attributes = EfiPaddingPciBus;
  return EFI_SUCCESS;
}


/**
  Entry point for this driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to SystemTable.

  @retval EFI_SUCESS       Driver has loaded successfully.
  @return                  Error codes from lower level functions.

**/
EFI_STATUS
EFIAPI
DriverInitialize (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;

  mPciHotPlugInit.GetRootHpcList = GetRootHpcList;
  mPciHotPlugInit.InitializeRootHpc = InitializeRootHpc;
  mPciHotPlugInit.GetResourcePadding = GetResourcePadding;
  Status = gBS->InstallMultipleProtocolInterfaces (&ImageHandle,
                  &gEfiPciHotPlugInitProtocolGuid, &mPciHotPlugInit, NULL);
  return Status;
}
