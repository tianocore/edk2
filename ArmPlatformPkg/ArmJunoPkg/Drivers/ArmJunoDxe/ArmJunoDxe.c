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

//
// Size in number of characters of the Linux boot argument
// passing the MAC address to be used by the PCI GigaByte
// Ethernet device : " sky2.mac_address=0x11,0x22,0x33,0x44,0x55,0x66"
//
#define SKY2_MAC_ADDRESS_BOOTARG_LEN  47

//
// Hardware platform identifiers
//
typedef enum {
  UNKNOWN,
  JUNO_R0,
  JUNO_R1
} JUNO_REVISION;

//
// Function prototypes
//
STATIC EFI_STATUS SetJunoR1DefaultBootEntries (
  VOID
  );

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
 * Build and Set UEFI Variable Boot####
 *
 * @param BootVariableName       Name of the UEFI Variable
 * @param Attributes             'Attributes' for the Boot#### variable as per UEFI spec
 * @param BootDescription        Description of the Boot#### variable
 * @param DevicePath             EFI Device Path of the EFI Application to boot
 * @param OptionalData           Parameters to pass to the EFI application
 * @param OptionalDataSize       Size of the parameters to pass to the EFI application
 *
 * @return EFI_OUT_OF_RESOURCES  A memory allocation failed
 * @return                       Return value of RT.SetVariable
 */
STATIC
EFI_STATUS
BootOptionCreate (
  IN  CHAR16                    BootVariableName[9],
  IN  UINT32                    Attributes,
  IN  CHAR16*                   BootDescription,
  IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
  IN  UINT8*                    OptionalData,
  IN  UINTN                     OptionalDataSize
  )
{
  UINTN                         VariableSize;
  UINT8                         *Variable;
  UINT8                         *VariablePtr;
  UINTN                         FilePathListLength;
  UINTN                         BootDescriptionSize;

  FilePathListLength  = GetDevicePathSize (DevicePath);
  BootDescriptionSize = StrSize (BootDescription);

  // Each Boot#### variable is built as follow:
  //   UINT32                   Attributes
  //   UINT16                   FilePathListLength
  //   CHAR16*                  Description
  //   EFI_DEVICE_PATH_PROTOCOL FilePathList[]
  //   UINT8                    OptionalData[]
  VariableSize = sizeof (UINT32) + sizeof (UINT16) +
      BootDescriptionSize + FilePathListLength + OptionalDataSize;
  Variable = AllocateZeroPool (VariableSize);
  if (Variable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // 'Attributes' field
  *(UINT32*)Variable = Attributes;
  // 'FilePathListLength' field
  VariablePtr = Variable + sizeof (UINT32);
  *(UINT16*)VariablePtr = FilePathListLength;
  // 'Description' field
  VariablePtr += sizeof (UINT16);
  CopyMem (VariablePtr, BootDescription, BootDescriptionSize);
  // 'FilePathList' field
  VariablePtr += BootDescriptionSize;
  CopyMem (VariablePtr, DevicePath, FilePathListLength);
  // 'OptionalData' field
  VariablePtr += FilePathListLength;
  CopyMem (VariablePtr, OptionalData, OptionalDataSize);

  return gRT->SetVariable (
      BootVariableName,
      &gEfiGlobalVariableGuid,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      VariableSize, Variable
      );
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
  UINT32                Midr;
  UINT32                CpuType;
  UINT32                CpuRev;
  JUNO_REVISION         JunoRevision;
  EFI_EVENT             EndOfDxeEvent;

  JunoRevision = UNKNOWN;
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

  //
  // We detect whether we are running on a Juno r0 or Juno r1 board at
  // runtime by checking the value of the MIDR register.
  //

  Midr     = ArmReadMidr ();
  CpuType  = (Midr >> ARM_CPU_TYPE_SHIFT) & ARM_CPU_TYPE_MASK;
  CpuRev   = Midr & ARM_CPU_REV_MASK;

  switch (CpuType) {
  case ARM_CPU_TYPE_A53:
    if (CpuRev == ARM_CPU_REV (0, 0)) {
      JunoRevision = JUNO_R0;
    } else if (CpuRev == ARM_CPU_REV (0, 3)) {
      JunoRevision = JUNO_R1;
    }
    break;

  case ARM_CPU_TYPE_A57:
    if (CpuRev == ARM_CPU_REV (0, 0)) {
      JunoRevision = JUNO_R0;
    } else if (CpuRev == ARM_CPU_REV (1, 1)) {
      JunoRevision = JUNO_R1;
    }
  }

  //
  // Try to install the ACPI Tables
  //
  if (JunoRevision == JUNO_R0) {
    Status = LocateAndInstallAcpiFromFvConditional (&mJunoAcpiTableFile, AcpiTableJunoR0Check);
  } else if (JunoRevision == JUNO_R1) {
    Status = LocateAndInstallAcpiFromFvConditional (&mJunoAcpiTableFile, AcpiTableJunoR1Check);
  }
  ASSERT_EFI_ERROR (Status);


  //
  // Set the R1 two boot options if not already done.
  //
  if (JunoRevision == JUNO_R1) {
    Status = SetJunoR1DefaultBootEntries ();
    if (EFI_ERROR (Status)) {
      return Status;
    }

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
  switch (JunoRevision) {
  case JUNO_R0:
    TextDevicePath = (CHAR16*)FixedPcdGetPtr (PcdJunoR0FdtDevicePath);
    break;

  case JUNO_R1:
    TextDevicePath = (CHAR16*)FixedPcdGetPtr (PcdJunoR1A57x2FdtDevicePath);
    break;

  default:
    TextDevicePath = NULL;
  }

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

/**
 * If no boot entry is currently defined, define the two default boot entries
 * for Juno R1.
 *
 * @return EFI_SUCCESS             Some boot entries were already defined or
 *                                 the default boot entries were set successfully.
 * @return EFI_OUT_OF_RESOURCES    A memory allocation failed.
 * @return EFI_DEVICE_ERROR        An UEFI variable could not be saved due to a hardware failure.
 * @return EFI_WRITE_PROTECTED     An UEFI variable is read-only.
 * @return EFI_SECURITY_VIOLATION  An UEFI variable could not be written.
 */
STATIC
EFI_STATUS
SetJunoR1DefaultBootEntries (
  VOID
  )
{
  EFI_STATUS                          Status;
  CONST CHAR16*                       ExtraBootArgument = L" dtb=r1a57a53.dtb";
  UINTN                               Size;
  EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL  *EfiDevicePathFromTextProtocol;
  EFI_DEVICE_PATH*                    BootDevicePath;
  UINT32                              SysPciGbeL;
  UINT32                              SysPciGbeH;
  CHAR16*                             DefaultBootArgument;
  CHAR16*                             DefaultBootArgument1;
  UINTN                               DefaultBootArgument1Size;
  CHAR16*                             DefaultBootArgument2;
  UINTN                               DefaultBootArgument2Size;
  UINT16                              BootOrder[2];

  BootDevicePath       = NULL;
  DefaultBootArgument1 = NULL;
  DefaultBootArgument2 = NULL;

  //
  // Because the driver has a dependency on gEfiVariable(Write)ArchProtocolGuid
  // (see [Depex] section of the INF file), we know we can safely access the
  // UEFI Variable at that stage.
  //
  Size = 0;
  Status = gRT->GetVariable (L"BootOrder", &gEfiGlobalVariableGuid, NULL, &Size, NULL);
  if (Status != EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (
                  &gEfiDevicePathFromTextProtocolGuid,
                  NULL,
                  (VOID **)&EfiDevicePathFromTextProtocol
                  );
  if (EFI_ERROR (Status)) {
    //
    // You must provide an implementation of DevicePathFromTextProtocol
    // in your firmware (eg: DevicePathDxe)
    //
    DEBUG ((EFI_D_ERROR, "Error: Require DevicePathFromTextProtocol\n"));
    return Status;
  }
  //
  // We use the same default kernel.
  //
  BootDevicePath = EfiDevicePathFromTextProtocol->ConvertTextToDevicePath (
                     (CHAR16*)PcdGetPtr (PcdDefaultBootDevicePath)
                     );
  if (BootDevicePath == NULL) {
    return EFI_UNSUPPORTED;
  }

  DefaultBootArgument = (CHAR16*)PcdGetPtr (PcdDefaultBootArgument);
  DefaultBootArgument1Size = StrSize (DefaultBootArgument) +
                             (SKY2_MAC_ADDRESS_BOOTARG_LEN * sizeof (CHAR16));
  DefaultBootArgument2Size = DefaultBootArgument1Size + StrSize (ExtraBootArgument);

  Status = EFI_OUT_OF_RESOURCES;
  DefaultBootArgument1 = AllocatePool (DefaultBootArgument1Size);
  if (DefaultBootArgument1 == NULL) {
    goto Error;
  }
  DefaultBootArgument2 = AllocatePool (DefaultBootArgument2Size);
  if (DefaultBootArgument2 == NULL) {
    goto Error;
  }

  SysPciGbeL = MmioRead32 (ARM_JUNO_SYS_PCIGBE_L);
  SysPciGbeH = MmioRead32 (ARM_JUNO_SYS_PCIGBE_H);

  UnicodeSPrint (
    DefaultBootArgument1,
    DefaultBootArgument1Size,
    L"%s sky2.mac_address=0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x",
    DefaultBootArgument,
    (SysPciGbeH >> 8 ) & 0xFF, (SysPciGbeH      ) & 0xFF,
    (SysPciGbeL >> 24) & 0xFF, (SysPciGbeL >> 16) & 0xFF,
    (SysPciGbeL >> 8 ) & 0xFF, (SysPciGbeL      ) & 0xFF
    );

  CopyMem (DefaultBootArgument2, DefaultBootArgument1, DefaultBootArgument1Size);
  CopyMem (
    (UINT8*)DefaultBootArgument2 + DefaultBootArgument1Size - sizeof (CHAR16),
    ExtraBootArgument,
    StrSize (ExtraBootArgument)
  );

  //
  // Create Boot0001 environment variable
  //
  Status = BootOptionCreate (
             L"Boot0001", LOAD_OPTION_ACTIVE | LOAD_OPTION_CATEGORY_BOOT,
             L"Linux with A57x2", BootDevicePath,
             (UINT8*)DefaultBootArgument1, DefaultBootArgument1Size
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto Error;
  }

  //
  // Create Boot0002 environment variable
  //
  Status = BootOptionCreate (
             L"Boot0002", LOAD_OPTION_ACTIVE | LOAD_OPTION_CATEGORY_BOOT,
             L"Linux with A57x2_A53x4", BootDevicePath,
             (UINT8*)DefaultBootArgument2, DefaultBootArgument2Size
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto Error;
  }

  //
  // Add the new Boot Index to the list
  //
  BootOrder[0] = 1; // Boot0001
  BootOrder[1] = 2; // Boot0002
  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_NON_VOLATILE       |
                  EFI_VARIABLE_BOOTSERVICE_ACCESS |
                  EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (BootOrder),
                  BootOrder
                  );

Error:
  if (BootDevicePath != NULL) {
    FreePool (BootDevicePath);
  }
  if (DefaultBootArgument1 != NULL) {
    FreePool (DefaultBootArgument1);
  }
  if (DefaultBootArgument2 != NULL) {
    FreePool (DefaultBootArgument2);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      EFI_D_ERROR,
      "ArmJunoDxe - The setting of the default boot entries failed - %r\n",
      Status
      ));
  }

  return Status;
}
