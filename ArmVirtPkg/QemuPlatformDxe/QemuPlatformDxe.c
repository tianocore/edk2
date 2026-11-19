/** @file

  The QemuPlatformDxe performs platform specific initialization.

  Copyright (c) 2018 - 2023, Arm Limited. All rights reserved.
  Copyright (c) 2026, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Guid/VariableFormat.h>
#include <IndustryStandard/Acpi.h>
#include <Library/ArmCcaLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/FdtClient.h>

#define RTC_SSDT_OEM_TABLE_ID           SIGNATURE_64 ('P', 'L', '0', '3', '1', 'R', 'T', 'C')
#define RTC_OP_REGION_BASE_PLACEHOLDER  0xDEADCAFE00000000ULL

#pragma pack (1)
typedef struct {
  UINT8     ExtOpPrefix;
  UINT8     OpRegionOp;
  UINT32    NameString;
  UINT8     RegionSpace;
  UINT8     QWordPrefix;
  UINT64    RegionOffset;
  UINT8     WordPrefix;
  UINT16    RegionLen;
} AML_OP_REGION_64_16;
#pragma pack ()

STATIC UINT64  mPl031RtcBase;
STATIC VOID    *mAcpiTableEventRegistration;

/**
  EFI_ACPI_TABLE_PROTOCOL notification callback that patches the PL031 base
  address into the RTC SSDT and installs it.

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
  UINTN                        Index;
  EFI_ACPI_DESCRIPTION_HEADER  *Ssdt;
  UINTN                        SsdtSize;
  UINT8                        *Ptr;
  AML_OP_REGION_64_16          *OpRegion;
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

  for (Index = 0; ; Index++) {
    Status = GetSectionFromFv (
               &gEfiCallerIdGuid,
               EFI_SECTION_RAW,
               Index,
               (VOID **)&Ssdt,
               &SsdtSize
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: PL031 SSDT not found in FV: %r\n",
        __func__,
        Status
        ));
      return;
    }

    if (Ssdt->OemTableId == RTC_SSDT_OEM_TABLE_ID) {
      break;
    }

    FreePool (Ssdt);
  }

  for (Ptr = (UINT8 *)(Ssdt + 1);
       Ptr <= (UINT8 *)Ssdt + SsdtSize - sizeof (*OpRegion);
       Ptr++)
  {
    OpRegion = (AML_OP_REGION_64_16 *)Ptr;
    if ((OpRegion->ExtOpPrefix  == AML_EXT_OP) &&
        (OpRegion->OpRegionOp   == AML_EXT_REGION_OP) &&
        (OpRegion->NameString   == SIGNATURE_32 ('R', 'T', 'C', 'R')) &&
        (OpRegion->RegionSpace  == EFI_ACPI_3_0_SYSTEM_MEMORY) &&
        (OpRegion->QWordPrefix  == AML_QWORD_PREFIX) &&
        (OpRegion->RegionOffset == RTC_OP_REGION_BASE_PLACEHOLDER) &&
        (OpRegion->WordPrefix   == AML_WORD_PREFIX) &&
        (OpRegion->RegionLen    == SIZE_4KB))
    {
      OpRegion->RegionOffset = mPl031RtcBase;

      Status = AcpiTable->InstallAcpiTable (
                            AcpiTable,
                            Ssdt,
                            SsdtSize,
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
      return;
    }
  }

  DEBUG ((DEBUG_ERROR, "%a: Failed to patch PL031 OperationRegion\n", __func__));
  ASSERT (FALSE);
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

  if (RegSize < (AddressCells + SizeCells) * sizeof (UINT32)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: Incomplete 'reg' property on 'arm,pl031' node\n",
      __func__
      ));
    return;
  }

  if (AddressCells == 2) {
    mPl031RtcBase = SwapBytes64 (*(CONST UINT64 *)Reg);
  } else if (AddressCells == 1) {
    mPl031RtcBase = SwapBytes32 (*(CONST UINT32 *)Reg);
  } else {
    DEBUG ((
      DEBUG_WARN,
      "%a: Unsupported #address-cells (%u) on 'arm,pl031' node\n",
      __func__,
      (UINT32)AddressCells
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
