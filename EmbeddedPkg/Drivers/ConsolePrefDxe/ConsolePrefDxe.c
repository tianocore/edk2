/** @file
*
*  Copyright (c) 2017, Linaro, Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <IndustryStandard/Acpi.h>
#include <libfdt.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/AcpiSystemDescriptionTable.h>

#include "ConsolePrefDxe.h"

#define SPCR_SIG  EFI_ACPI_2_0_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE

extern  UINT8  ConsolePrefHiiBin[];
extern  UINT8  ConsolePrefDxeStrings[];

typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

STATIC HII_VENDOR_DEVICE_PATH  mConsolePrefDxeVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    CONSOLE_PREF_FORMSET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

STATIC EFI_EVENT  mReadyToBootEvent;

STATIC
EFI_STATUS
InstallHiiPages (
  VOID
  )
{
  EFI_STATUS      Status;
  EFI_HII_HANDLE  HiiHandle;
  EFI_HANDLE      DriverHandle;

  DriverHandle = NULL;
  Status       = gBS->InstallMultipleProtocolInterfaces (
                        &DriverHandle,
                        &gEfiDevicePathProtocolGuid,
                        &mConsolePrefDxeVendorDevicePath,
                        NULL
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiHandle = HiiAddPackages (
                &gConsolePrefFormSetGuid,
                DriverHandle,
                ConsolePrefDxeStrings,
                ConsolePrefHiiBin,
                NULL
                );

  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mConsolePrefDxeVendorDevicePath,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

STATIC
VOID
RemoveDtStdoutPath (
  VOID
  )
{
  VOID        *Dtb;
  INT32       Node;
  INT32       Error;
  EFI_STATUS  Status;

  Status = EfiGetSystemConfigurationTable (&gFdtTableGuid, &Dtb);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: could not retrieve DT blob - %r\n",
      __func__,
      Status
      ));
    return;
  }

  Node = fdt_path_offset (Dtb, "/chosen");
  if (Node < 0) {
    return;
  }

  Error = fdt_delprop (Dtb, Node, "stdout-path");
  if (Error) {
    DEBUG ((
      DEBUG_INFO,
      "%a: Failed to delete 'stdout-path' property: %a\n",
      __func__,
      fdt_strerror (Error)
      ));
  }
}

STATIC
VOID
RemoveSpcrTable (
  VOID
  )
{
  EFI_ACPI_SDT_PROTOCOL    *Sdt;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;
  EFI_STATUS               Status;
  UINTN                    TableIndex;
  EFI_ACPI_SDT_HEADER      *TableHeader;
  EFI_ACPI_TABLE_VERSION   TableVersion;
  UINTN                    TableKey;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTable
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&Sdt);
  if (EFI_ERROR (Status)) {
    return;
  }

  TableIndex  = 0;
  TableKey    = 0;
  TableHeader = NULL;

  do {
    Status = Sdt->GetAcpiTable (
                    TableIndex++,
                    &TableHeader,
                    &TableVersion,
                    &TableKey
                    );
    if (EFI_ERROR (Status)) {
      break;
    }

    if (TableHeader->Signature != SPCR_SIG) {
      continue;
    }

    Status = AcpiTable->UninstallAcpiTable (AcpiTable, TableKey);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "%a: failed to uninstall SPCR table - %r\n",
        __func__,
        Status
        ));
    }

    break;
  } while (TRUE);
}

STATIC
VOID
EFIAPI
OnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  CONSOLE_PREF_VARSTORE_DATA  ConsolePref;
  UINTN                       BufferSize;
  EFI_STATUS                  Status;
  VOID                        *Gop;

  BufferSize = sizeof (ConsolePref);
  Status     = gRT->GetVariable (
                      CONSOLE_PREF_VARIABLE_NAME,
                      &gConsolePrefFormSetGuid,
                      NULL,
                      &BufferSize,
                      &ConsolePref
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: variable '%s' could not be read - bailing!\n",
      __func__,
      CONSOLE_PREF_VARIABLE_NAME
      ));
    return;
  }

  if (ConsolePref.Console == CONSOLE_PREF_SERIAL) {
    DEBUG ((
      DEBUG_INFO,
      "%a: serial console preferred - doing nothing\n",
      __func__
      ));
    return;
  }

  //
  // Check if any GOP instances exist: if so, disable stdout-path and SPCR
  //
  Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, &Gop);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: no GOP instances found - doing nothing (%r)\n",
      __func__,
      Status
      ));
    return;
  }

  RemoveDtStdoutPath ();
  RemoveSpcrTable ();
}

/**
  The entry point for ConsolePrefDxe driver.

  @param[in] ImageHandle     The image handle of the driver.
  @param[in] SystemTable     The system table.

  @retval EFI_ALREADY_STARTED     The driver already exists in system.
  @retval EFI_OUT_OF_RESOURCES    Fail to execute entry point due to lack of
                                  resources.
  @retval EFI_SUCCESS             All the related protocols are installed on
                                  the driver.

**/
EFI_STATUS
EFIAPI
ConsolePrefDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  CONSOLE_PREF_VARSTORE_DATA  ConsolePref;
  UINTN                       BufferSize;

  //
  // Get the current console preference from the ConsolePref variable.
  //
  BufferSize = sizeof (ConsolePref);
  Status     = gRT->GetVariable (
                      CONSOLE_PREF_VARIABLE_NAME,
                      &gConsolePrefFormSetGuid,
                      NULL,
                      &BufferSize,
                      &ConsolePref
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: no console preference found, defaulting to graphical\n",
      __func__
      ));
    ConsolePref.Console = CONSOLE_PREF_GRAPHICAL;
  }

  if (!EFI_ERROR (Status) &&
      (ConsolePref.Console != CONSOLE_PREF_GRAPHICAL) &&
      (ConsolePref.Console != CONSOLE_PREF_SERIAL))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: invalid value for %s, defaulting to graphical\n",
      __func__,
      CONSOLE_PREF_VARIABLE_NAME
      ));
    ConsolePref.Console = CONSOLE_PREF_GRAPHICAL;
    Status              = EFI_INVALID_PARAMETER; // trigger setvar below
  }

  //
  // Write the newly selected value back to the variable store.
  //
  if (EFI_ERROR (Status)) {
    ZeroMem (&ConsolePref.Reserved, sizeof (ConsolePref.Reserved));
    Status = gRT->SetVariable (
                    CONSOLE_PREF_VARIABLE_NAME,
                    &gConsolePrefFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (ConsolePref),
                    &ConsolePref
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: gRT->SetVariable () failed - %r\n",
        __func__,
        Status
        ));
      return Status;
    }
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnReadyToBoot,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &mReadyToBootEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return InstallHiiPages ();
}
