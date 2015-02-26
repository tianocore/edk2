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

#include <Protocol/DevicePathFromText.h>

#include <Guid/GlobalVariable.h>

#include <Library/ArmShellCmdLib.h>
#include <Library/AcpiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

// This GUID must match the FILE_GUID in ArmPlatformPkg/ArmJunoPkg/AcpiTables/AcpiTables.inf
STATIC CONST EFI_GUID mJunoAcpiTableFile = { 0xa1dd808e, 0x1e95, 0x4399, { 0xab, 0xc0, 0x65, 0x3c, 0x82, 0xe8, 0x53, 0x0c } };

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
  BOOLEAN               IsJunoR1;

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

  // Install dynamic Shell command to run baremetal binaries.
  Status = ShellDynCmdRunAxfInstall (ImageHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ArmJunoDxe: Failed to install ShellDynCmdRunAxf\n"));
  }

  //
  // Set up the device path to the FDT.
  // We detect whether we are running on a Juno r0 or Juno r1 board at
  // runtime by checking the value of the MIDR register.
  //

  Midr           = ArmReadMidr ();
  CpuType        = (Midr >> ARM_CPU_TYPE_SHIFT) & ARM_CPU_TYPE_MASK;
  CpuRev         = Midr & ARM_CPU_REV_MASK;
  TextDevicePath = NULL;
  IsJunoR1       = FALSE;

  switch (CpuType) {
  case ARM_CPU_TYPE_A53:
    if (CpuRev == ARM_CPU_REV (0, 0)) {
      TextDevicePath = (CHAR16*)FixedPcdGetPtr (PcdJunoR0FdtDevicePath);
    } else if (CpuRev == ARM_CPU_REV (0, 3)) {
      TextDevicePath = (CHAR16*)FixedPcdGetPtr (PcdJunoR1A57x2FdtDevicePath);
      IsJunoR1       = TRUE;
    }
    break;

  case ARM_CPU_TYPE_A57:
    if (CpuRev == ARM_CPU_REV (0, 0)) {
      TextDevicePath = (CHAR16*)FixedPcdGetPtr (PcdJunoR0FdtDevicePath);
    } else if (CpuRev == ARM_CPU_REV (1, 1)) {
      TextDevicePath = (CHAR16*)FixedPcdGetPtr (PcdJunoR1A57x2FdtDevicePath);
      IsJunoR1       = TRUE;
    }
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

  // Try to install the ACPI Tables
  Status = LocateAndInstallAcpiFromFv (&mJunoAcpiTableFile);

  //
  // If Juno R1 and it is the first boot then default boot entries will be created
  //
  if (IsJunoR1) {
    CONST CHAR16*                       ExtraBootArgument = L" dtb=juno-r1-ca57x2_ca53x4.dtb";
    UINTN                               Size;
    EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL* EfiDevicePathFromTextProtocol;
    EFI_DEVICE_PATH*                    BootDevicePath;
    CHAR16*                             DefaultBootArgument;
    UINTN                               DefaultBootArgumentSize;
    CHAR16*                             DefaultBootArgument2;
    UINTN                               DefaultBootArgument2Size;
    UINT16                              BootOrder[2];

    // Because the driver has a dependency on gEfiVariable(Write)ArchProtocolGuid (see [Depex]
    // section of the INF file), we know we can safely access the UEFI Variable at that stage.
    Size = 0;
    Status = gRT->GetVariable (L"BootOrder", &gEfiGlobalVariableGuid, NULL, &Size, NULL);
    if (Status == EFI_NOT_FOUND) {
      Status = gBS->LocateProtocol (&gEfiDevicePathFromTextProtocolGuid, NULL, (VOID **)&EfiDevicePathFromTextProtocol);
      if (EFI_ERROR (Status)) {
        // You must provide an implementation of DevicePathFromTextProtocol in your firmware (eg: DevicePathDxe)
        DEBUG ((EFI_D_ERROR, "Error: Require DevicePathFromTextProtocol\n"));
        return Status;
      }
      // We use the same default kernel
      BootDevicePath = EfiDevicePathFromTextProtocol->ConvertTextToDevicePath ((CHAR16*)PcdGetPtr (PcdDefaultBootDevicePath));

      // Create the entry if the Default values are correct
      if (BootDevicePath != NULL) {
        DefaultBootArgument = (CHAR16*)PcdGetPtr (PcdDefaultBootArgument);
        DefaultBootArgumentSize = StrSize (DefaultBootArgument);
        DefaultBootArgument2Size = DefaultBootArgumentSize + StrSize (ExtraBootArgument);

        DefaultBootArgument2 = AllocatePool (DefaultBootArgument2Size);
        if (DefaultBootArgument2 == NULL) {
          FreePool (BootDevicePath);
          return EFI_OUT_OF_RESOURCES;
        }
        CopyMem (DefaultBootArgument2, DefaultBootArgument, DefaultBootArgumentSize);
        CopyMem ((UINT8*)DefaultBootArgument2 + (StrLen (DefaultBootArgument2) * sizeof (CHAR16)), ExtraBootArgument, StrSize (ExtraBootArgument));

        // Create Boot0001 environment variable
        Status = BootOptionCreate (L"Boot0001", LOAD_OPTION_ACTIVE | LOAD_OPTION_CATEGORY_BOOT,
            L"Linux with A57x2", BootDevicePath,
            (UINT8*)DefaultBootArgument, DefaultBootArgumentSize);
        ASSERT_EFI_ERROR (Status);

        // Create Boot0002 environment variable
        Status = BootOptionCreate (L"Boot0002", LOAD_OPTION_ACTIVE | LOAD_OPTION_CATEGORY_BOOT,
            L"Linux with A57x2_A53x4", BootDevicePath,
            (UINT8*)DefaultBootArgument2, DefaultBootArgument2Size);
        ASSERT_EFI_ERROR (Status);

        // Add the new Boot Index to the list
        BootOrder[0] = 1; // Boot0001
        BootOrder[1] = 2; // Boot0002
        Status = gRT->SetVariable (
            L"BootOrder",
            &gEfiGlobalVariableGuid,
            EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
            sizeof (BootOrder),
            BootOrder
            );

        FreePool (BootDevicePath);
        FreePool (DefaultBootArgument2);
      } else {
        Status = EFI_UNSUPPORTED;
      }
    }
  }

  return Status;
}
