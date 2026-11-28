/** @file

  The QemuPlatformDxe performs platform specific initialization.

  Copyright (c) 2018 - 2026, Arm Limited. All rights reserved.
  Copyright (c) 2026, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/VariableFormat.h>
#include <IndustryStandard/Acpi.h>
#include <Library/AcpiRtcPl031Lib.h>
#include <Library/ArmCcaLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/FdtClient.h>

STATIC UINT64  mPl031RtcBase;
STATIC VOID    *mAcpiTableEventRegistration;

/**
  Build and install the PL031 RTC SSDT when the ACPI table protocol is available.

  @param [in] Event    The event that is being processed.
  @param [in] Context  The event context.
**/
STATIC
VOID
EFIAPI
InstallPl031RtcSsdt (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_TABLE_PROTOCOL      *AcpiTable;
  EFI_ACPI_DESCRIPTION_HEADER  *Ssdt;
  UINTN                        TableKey;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTable
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  gBS->CloseEvent (Event);

  Status = BuildSsdtPl031Table (
             mPl031RtcBase,
             &Ssdt
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to build PL031 SSDT: %r\n",
      __func__,
      Status
      ));
    return;
  }

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        Ssdt,
                        Ssdt->Length,
                        &TableKey
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to install PL031 SSDT: %r\n",
      __func__,
      Status
      ));
  }

  FreePool (Ssdt);
}

/**
  Locate the PL031 RTC in the FDT and register a callback to install its SSDT
  if ACPI is enabled.

  @param [in] FdtClient  Pointer to the FDT client protocol.
**/
STATIC
VOID
RegisterPl031RtcSsdt (
  IN FDT_CLIENT_PROTOCOL  *FdtClient
  )
{
  EFI_STATUS  Status;
  CONST VOID  *Reg;
  UINTN       AddressCells;
  UINTN       SizeCells;
  UINT32      RegSize;
  CONST VOID  *Size;
  UINT64      RegionSize;

  Status = FdtClient->FindCompatibleNodeReg (
                        FdtClient,
                        "arm,pl031",
                        &Reg,
                        &AddressCells,
                        &SizeCells,
                        &RegSize
                        );
  if (EFI_ERROR (Status)) {
    return;
  }

  if (((AddressCells != 1) && (AddressCells != 2)) ||
      ((SizeCells != 1) && (SizeCells != 2)))
  {
    DEBUG ((DEBUG_WARN, "%a: Unsupported PL031 reg cell counts\n", __func__));
    return;
  }

  if (RegSize < (AddressCells + SizeCells) * sizeof (UINT32)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: Incomplete 'reg' property on 'arm,pl031' node\n",
      __func__
      ));
    return;
  }

  if (AddressCells == 2) {
    mPl031RtcBase = SwapBytes64 (ReadUnaligned64 ((CONST UINT64 *)Reg));
  } else {
    mPl031RtcBase = SwapBytes32 (ReadUnaligned32 ((CONST UINT32 *)Reg));
  }

  Size = (CONST UINT8 *)Reg + AddressCells * sizeof (UINT32);
  if (SizeCells == 2) {
    RegionSize = SwapBytes64 (ReadUnaligned64 ((CONST UINT64 *)Size));
  } else {
    RegionSize = SwapBytes32 (ReadUnaligned32 ((CONST UINT32 *)Size));
  }

  if (RegionSize < SIZE_4KB) {
    DEBUG ((
      DEBUG_WARN,
      "%a: PL031 reg region is smaller than the 4 KiB SSDT region\n",
      __func__
      ));
    return;
  }

  EfiCreateProtocolNotifyEvent (
    &gEfiAcpiTableProtocolGuid,
    TPL_CALLBACK,
    InstallPl031RtcSsdt,
    NULL,
    &mAcpiTableEventRegistration
    );
}

/** Entrypoint for QemuPlatformDxeEntryPoint

  @param [in] ImageHandle  The image handle.
  @param [in] SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
EFIAPI
QemuPlatformDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  INT32                Node;
  EFI_STATUS           Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = FdtClient->FindCompatibleNode (
                        FdtClient,
                        "cfi-flash",
                        &Node
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "No Flash device found, falling back to Runtime Variable Emulation\n"
      ));

    Status = PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Failed to set PcdEmuVariableNvModeEnable, Status = %r\n",
        Status
        ));
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    // The driver implementing the variable service can now be dispatched.
    Status = gBS->InstallProtocolInterface (
                    &gImageHandle,
                    &gEdkiiNvVarStoreFormattedGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  } else if (ArmCcaIsRealm ()) {
    // In a Confidential VM the CFI flash emulated by the VMM is outside
    // the trust boundary of the Realm. The VMM must not expose a CFI
    // flash device to a CCA Realm VM, as this is an invalid configuration.
    Status = EFI_SECURITY_VIOLATION;
    DEBUG ((
      DEBUG_ERROR,
      "Error: A Realm VM must not have a CFI Flash device, Status = %r.\n",
      Status
      ));
    // We must not proceed with the Realm Boot.
    CpuDeadLoop ();
  }

  RegisterPl031RtcSsdt (FdtClient);

  return Status;
}
