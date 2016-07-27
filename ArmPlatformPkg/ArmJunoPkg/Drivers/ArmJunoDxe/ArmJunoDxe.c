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

#include <Protocol/DevicePathFromText.h>
#include <Protocol/PciRootBridgeIo.h>

#include <Guid/EventGroup.h>
#include <Guid/GlobalVariable.h>

#include <Library/ArmShellCmdLib.h>
#include <Library/AcpiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
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
}

STATIC
BOOLEAN
AcpiTableJunoR0Check (
  IN  EFI_ACPI_DESCRIPTION_HEADER *AcpiHeader
  )
{
  return TRUE;
}

STATIC
BOOLEAN
AcpiTableJunoR1Check (
  IN  EFI_ACPI_DESCRIPTION_HEADER *AcpiHeader
  )
{
  return TRUE;
}

STATIC
BOOLEAN
AcpiTableJunoR2Check (
  IN  EFI_ACPI_DESCRIPTION_HEADER *AcpiHeader
  )
{
  return TRUE;
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

  Status = PciEmulationEntryPoint ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

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

  // Install dynamic Shell command to run baremetal binaries.
  Status = ShellDynCmdRunAxfInstall (ImageHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ArmJunoDxe: Failed to install ShellDynCmdRunAxf\n"));
  }

  GetJunoRevision(JunoRevision);

  //
  // Try to install the ACPI Tables
  //
  if (JunoRevision == JUNO_REVISION_R0) {
    Status = LocateAndInstallAcpiFromFvConditional (&mJunoAcpiTableFile, AcpiTableJunoR0Check);
  } else if (JunoRevision == JUNO_REVISION_R1) {
    Status = LocateAndInstallAcpiFromFvConditional (&mJunoAcpiTableFile, AcpiTableJunoR1Check);
  } else if (JunoRevision == JUNO_REVISION_R2) {
    Status = LocateAndInstallAcpiFromFvConditional (&mJunoAcpiTableFile, AcpiTableJunoR2Check);
  }

  ASSERT_EFI_ERROR (Status);

  //
  // Setup R1/R2 options if not already done.
  //
  if (JunoRevision != JUNO_REVISION_R0) {
    // Enable PCI enumeration
    PcdSetBool (PcdPciDisableBusEnumeration, FALSE);

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
