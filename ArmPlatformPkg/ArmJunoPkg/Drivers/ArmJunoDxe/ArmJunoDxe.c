/** @file
*
*  Copyright (c) 2013-2015, ARM Limited. All rights reserved.
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

#include "ArmJunoDxeInternal.h"
#include <ArmPlatform.h>

#include <IndustryStandard/Pci.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>

#include <Guid/EventGroup.h>
#include <Guid/GlobalVariable.h>

#include <Library/ArmShellCmdLib.h>
#include <Library/AcpiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>


// This GUID must match the FILE_GUID in ArmPlatformPkg/ArmJunoPkg/AcpiTables/AcpiTables.inf
STATIC CONST EFI_GUID mJunoAcpiTableFile = { 0xa1dd808e, 0x1e95, 0x4399, { 0xab, 0xc0, 0x65, 0x3c, 0x82, 0xe8, 0x53, 0x0c } };

typedef struct {
  ACPI_HID_DEVICE_PATH      AcpiDevicePath;
  PCI_DEVICE_PATH           PciDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;

STATIC CONST EFI_PCI_ROOT_BRIDGE_DEVICE_PATH mPciRootComplexDevicePath = {
    {
      { ACPI_DEVICE_PATH,
        ACPI_DP,
        { (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),
          (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8) }
      },
      EISA_PNP_ID (0x0A03),
      0
    },
    {
      { HARDWARE_DEVICE_PATH,
        HW_PCI_DP,
        { (UINT8) (sizeof (PCI_DEVICE_PATH)),
          (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8) }
      },
      0,
      0
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { END_DEVICE_PATH_LENGTH, 0 }
    }
};

EFI_EVENT mAcpiRegistration = NULL;

/**
  This function reads PCI ID of the controller.

  @param[in]  PciIo   PCI IO protocol handle
  @param[in]  PciId   Looking for specified PCI ID Vendor/Device
**/
STATIC
EFI_STATUS
ReadMarvellYoukonPciId (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               PciId
  )
{
  UINT32      DevicePciId;
  EFI_STATUS  Status;

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        PCI_VENDOR_ID_OFFSET,
                        1,
                        &DevicePciId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DevicePciId != PciId) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  This function searches for Marvell Yukon NIC on the Juno
  platform and returns PCI IO protocol handle for the controller.

  @param[out]  PciIo   PCI IO protocol handle
**/
STATIC
EFI_STATUS
GetMarvellYukonPciIoProtocol (
  OUT EFI_PCI_IO_PROTOCOL  **PciIo
  )
{
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       HIndex;
  EFI_STATUS  Status;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return (Status);
  }

  for (HIndex = 0; HIndex < HandleCount; ++HIndex) {
    // If PciIo opened with EFI_OPEN_PROTOCOL_GET_PROTOCOL, the CloseProtocol() is not required
    Status = gBS->OpenProtocol (
                    HandleBuffer[HIndex],
                    &gEfiPciIoProtocolGuid,
                    (VOID **) PciIo,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = ReadMarvellYoukonPciId (*PciIo, JUNO_MARVELL_YUKON_ID);
    if (EFI_ERROR (Status)) {
      continue;
    } else {
      break;
    }
  }

  gBS->FreePool (HandleBuffer);

  return Status;
}

/**
 This function restore the original controller attributes

   @param[in]   PciIo               PCI IO protocol handle
   @param[in]   PciAttr             PCI controller attributes.
   @param[in]   AcpiResDescriptor   ACPI 2.0 resource descriptors for the BAR
**/
STATIC
VOID
RestorePciDev (
  IN EFI_PCI_IO_PROTOCOL                *PciIo,
  IN UINT64                             PciAttr
  )
{
  PciIo->Attributes (
           PciIo,
           EfiPciIoAttributeOperationSet,
           PciAttr,
           NULL
           );
}

/**
 This function returns PCI MMIO base address for a controller

   @param[in]   PciIo               PCI IO protocol handle
   @param[out]  PciRegBase          PCI base MMIO address
**/
STATIC
EFI_STATUS
BarIsDeviceMemory (
  IN   EFI_PCI_IO_PROTOCOL *PciIo,
  OUT  UINT32              *PciRegBase
  )
{
  EFI_STATUS                         Status;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *AcpiResDescriptor;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *AcpiCurrentDescriptor;

  // Marvell Yukon's Bar0 provides base memory address for control registers
  Status = PciIo->GetBarAttributes (PciIo, PCI_BAR_IDX0, NULL, (VOID**)&AcpiResDescriptor);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  AcpiCurrentDescriptor = AcpiResDescriptor;

  // Search for a memory type descriptor
  while (AcpiCurrentDescriptor->Desc != ACPI_END_TAG_DESCRIPTOR) {

    // Check if Bar is memory type one and fetch a base address
    if (AcpiCurrentDescriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR &&
        AcpiCurrentDescriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM &&
        !(AcpiCurrentDescriptor->SpecificFlag & ACPI_SPECFLAG_PREFETCHABLE)) {
      *PciRegBase = AcpiCurrentDescriptor->AddrRangeMin;
      break;
    } else {
      Status = EFI_UNSUPPORTED;
    }

    AcpiCurrentDescriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) (AcpiCurrentDescriptor + 1);
  }

  gBS->FreePool (AcpiResDescriptor);

  return Status;
}

/**
 This function provides PCI MMIO base address, old PCI controller attributes.

   @param[in]   PciIo               PCI IO protocol handle
   @param[out]  PciRegBase          PCI base MMIO address
   @param[out]  OldPciAttr          Old PCI controller attributes.
**/
STATIC
EFI_STATUS
InitPciDev (
  IN EFI_PCI_IO_PROTOCOL                 *PciIo,
  OUT UINT32                             *PciRegBase,
  OUT UINT64                             *OldPciAttr
  )
{
  UINT64      AttrSupports;
  EFI_STATUS  Status;

  // Get controller's current attributes
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationGet,
                    0,
                    OldPciAttr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Fetch supported attributes
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &AttrSupports);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Enable EFI_PCI_IO_ATTRIBUTE_IO, EFI_PCI_IO_ATTRIBUTE_MEMORY and
  // EFI_PCI_IO_ATTRIBUTE_BUS_MASTER bits in the PCI Config Header
  AttrSupports &= EFI_PCI_DEVICE_ENABLE;
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationEnable,
                    AttrSupports,
                    NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = BarIsDeviceMemory (PciIo, PciRegBase);
  if (EFI_ERROR (Status)) {
    RestorePciDev (PciIo, *OldPciAttr);
  }

  return Status;
}

/**
 This function reads MAC address from IOFPGA and writes it to Marvell Yukon NIC

   @param[in]   PciRegBase   PCI base MMIO address
**/
STATIC
EFI_STATUS
WriteMacAddress (
  IN UINT32  PciRegBase
  )
{
  UINT32  MacHigh;
  UINT32  MacLow;

  // Read MAC address from IOFPGA
  MacHigh= MmioRead32 (ARM_JUNO_SYS_PCIGBE_H);
  MacLow = MmioRead32 (ARM_JUNO_SYS_PCIGBE_L);

  // Set software reset control register to protect from deactivation
  // the config write state
  MmioWrite16 (PciRegBase + R_CONTROL_STATUS, CS_RESET_CLR);

  // Convert to Marvell MAC Address register format
  MacHigh = SwapBytes32 ((MacHigh & 0xFFFF) << 16 |
                         (MacLow & 0xFFFF0000) >> 16);
  MacLow = SwapBytes32 (MacLow) >> 16;

  // Set MAC Address
  MmioWrite8 (PciRegBase + R_TST_CTRL_1, TST_CFG_WRITE_ENABLE);
  MmioWrite32 (PciRegBase + R_MAC, MacHigh);
  MmioWrite32 (PciRegBase + R_MAC_MAINT, MacHigh);
  MmioWrite32 (PciRegBase + R_MAC + R_MAC_LOW, MacLow);
  MmioWrite32 (PciRegBase + R_MAC_MAINT + R_MAC_LOW, MacLow);
  MmioWrite8 (PciRegBase + R_TST_CTRL_1, TST_CFG_WRITE_DISABLE);

  // Initiate device reset
  MmioWrite16 (PciRegBase + R_CONTROL_STATUS, CS_RESET_SET);
  MmioWrite16 (PciRegBase + R_CONTROL_STATUS, CS_RESET_CLR);

  return EFI_SUCCESS;
}

/**
  The function reads MAC address from Juno IOFPGA registers and writes it
  into Marvell Yukon NIC.
**/
STATIC
EFI_STATUS
ArmJunoSetNicMacAddress ()
{
  UINT64                              OldPciAttr;
  EFI_PCI_IO_PROTOCOL*                PciIo;
  UINT32                              PciRegBase;
  EFI_STATUS                          Status;

  Status = GetMarvellYukonPciIoProtocol (&PciIo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InitPciDev (PciIo, &PciRegBase, &OldPciAttr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WriteMacAddress (PciRegBase);

  RestorePciDev (PciIo, OldPciAttr);

  return EFI_SUCCESS;
}

/**
  Notification function of the event defined as belonging to the
  EFI_END_OF_DXE_EVENT_GROUP_GUID event group that was created in
  the entry point of the driver.

  This function is called when an event belonging to the
  EFI_END_OF_DXE_EVENT_GROUP_GUID event group is signalled. Such an
  event is signalled once at the end of the dispatching of all
  drivers (end of the so called DXE phase).

  @param[in]  Event    Event declared in the entry point of the driver whose
                       notification function is being invoked.
  @param[in]  Context  NULL
**/
STATIC
VOID
OnEndOfDxe (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_DEVICE_PATH_PROTOCOL* PciRootComplexDevicePath;
  EFI_HANDLE                Handle;
  EFI_STATUS                Status;

  //
  // PCI Root Complex initialization
  // At the end of the DXE phase, we should get all the driver dispatched.
  // Force the PCI Root Complex to be initialized. It allows the OS to skip
  // this step.
  //
  PciRootComplexDevicePath = (EFI_DEVICE_PATH_PROTOCOL*) &mPciRootComplexDevicePath;
  Status = gBS->LocateDevicePath (&gEfiPciRootBridgeIoProtocolGuid,
                                  &PciRootComplexDevicePath,
                                  &Handle);

  Status = gBS->ConnectController (Handle, NULL, PciRootComplexDevicePath, FALSE);
  ASSERT_EFI_ERROR (Status);

  Status = ArmJunoSetNicMacAddress ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ArmJunoDxe: Failed to set Marvell Yukon NIC MAC address\n"));
  }
}

EFI_STATUS
EFIAPI
ArmJunoEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  HypBase;
  CHAR16                *TextDevicePath;
  UINTN                 TextDevicePathSize;
  VOID                  *Buffer;
  UINT32                JunoRevision;
  EFI_EVENT             EndOfDxeEvent;

  //
  // Register the OHCI and EHCI controllers as non-coherent
  // non-discoverable devices.
  //
  Status = RegisterNonDiscoverableMmioDevice (
             NonDiscoverableDeviceTypeOhci,
             NonDiscoverableDeviceDmaTypeNonCoherent,
             NULL,
             NULL,
             1,
             FixedPcdGet32 (PcdSynopsysUsbOhciBaseAddress),
             SIZE_64KB
             );
  ASSERT_EFI_ERROR (Status);

  Status = RegisterNonDiscoverableMmioDevice (
             NonDiscoverableDeviceTypeEhci,
             NonDiscoverableDeviceDmaTypeNonCoherent,
             NULL,
             NULL,
             1,
             FixedPcdGet32 (PcdSynopsysUsbEhciBaseAddress),
             SIZE_64KB
             );
  ASSERT_EFI_ERROR (Status);

  //
  // If a hypervisor has been declared then we need to make sure its region is protected at runtime
  //
  // Note: This code is only a workaround for our dummy hypervisor (ArmPkg/Extra/AArch64ToAArch32Shim/)
  //       that does not set up (yet) the stage 2 translation table to hide its own memory to EL1.
  //
  if (FixedPcdGet32 (PcdHypFvSize) != 0) {
    // Ensure the hypervisor region is strictly contained into a EFI_PAGE_SIZE-aligned region.
    // The memory must be a multiple of EFI_PAGE_SIZE to ensure we do not reserve more memory than the hypervisor itself.
    // A UEFI Runtime region size granularity cannot be smaller than EFI_PAGE_SIZE. If the hypervisor size is not rounded
    // to this size then there is a risk some non-runtime memory could be visible to the OS view.
    if (((FixedPcdGet32 (PcdHypFvSize) & EFI_PAGE_MASK) == 0) && ((FixedPcdGet32 (PcdHypFvBaseAddress) & EFI_PAGE_MASK) == 0)) {
      // The memory needs to be declared because the DXE core marked it as reserved and removed it from the memory space
      // as it contains the Firmware.
      Status = gDS->AddMemorySpace (
          EfiGcdMemoryTypeSystemMemory,
          FixedPcdGet32 (PcdHypFvBaseAddress), FixedPcdGet32 (PcdHypFvSize),
          EFI_MEMORY_WB | EFI_MEMORY_RUNTIME
          );
      if (!EFI_ERROR (Status)) {
        // We allocate the memory to ensure it is marked as runtime memory
        HypBase = FixedPcdGet32 (PcdHypFvBaseAddress);
        Status = gBS->AllocatePages (AllocateAddress, EfiRuntimeServicesCode,
                                     EFI_SIZE_TO_PAGES (FixedPcdGet32 (PcdHypFvSize)), &HypBase);
      }
    } else {
      // The hypervisor must be contained into a EFI_PAGE_SIZE-aligned region and its size must also be aligned
      // on a EFI_PAGE_SIZE boundary (ie: 4KB).
      Status = EFI_UNSUPPORTED;
      ASSERT_EFI_ERROR (Status);
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  // Install dynamic Shell command to run baremetal binaries.
  Status = ShellDynCmdRunAxfInstall (ImageHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ArmJunoDxe: Failed to install ShellDynCmdRunAxf\n"));
  }

  GetJunoRevision(JunoRevision);

  //
  // Try to install the ACPI Tables
  //
  Status = LocateAndInstallAcpiFromFv (&mJunoAcpiTableFile);
  ASSERT_EFI_ERROR (Status);

  //
  // Setup R1/R2 options if not already done.
  //
  if (JunoRevision != JUNO_REVISION_R0) {
    // Enable PCI enumeration
    PcdSetBool (PcdPciDisableBusEnumeration, FALSE);

    //
    // Create an event belonging to the "gEfiEndOfDxeEventGroupGuid" group.
    // The "OnEndOfDxe()" function is declared as the call back function.
    // It will be called at the end of the DXE phase when an event of the
    // same group is signalled to inform about the end of the DXE phase.
    // Install the INSTALL_FDT_PROTOCOL protocol.
    //
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    OnEndOfDxe,
                    NULL,
                    &gEfiEndOfDxeEventGroupGuid,
                    &EndOfDxeEvent
                    );

    // Declare the related ACPI Tables
    EfiCreateProtocolNotifyEvent (
        &gEfiAcpiTableProtocolGuid,
        TPL_CALLBACK,
        AcpiPciNotificationEvent,
        NULL,
        &mAcpiRegistration
        );
  }

  //
  // Set up the device path to the FDT.
  //
  TextDevicePath = (CHAR16*)FixedPcdGetPtr (PcdJunoFdtDevicePath);
  if (TextDevicePath != NULL) {
    TextDevicePathSize = StrSize (TextDevicePath);
    Buffer = PcdSetPtr (PcdFdtDevicePaths, &TextDevicePathSize, TextDevicePath);
    Status = (Buffer != NULL) ? EFI_SUCCESS : EFI_BUFFER_TOO_SMALL;
  } else {
    Status = EFI_NOT_FOUND;
  }

  if (EFI_ERROR (Status)) {
    DEBUG (
      (EFI_D_ERROR,
      "ArmJunoDxe: Setting of FDT device path in PcdFdtDevicePaths failed - %r\n", Status)
      );
    return Status;
  }

  return Status;
}
