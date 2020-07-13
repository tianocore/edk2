/** @file
  Implementation for a generic i801 SMBus driver.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "PrimaryVideoConfigDxe.h"
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/EventGroup.h>
#include <Protocol/GraphicsOutput.h>

EFI_EVENT                 mDxeRuntimeCapsuleLibReadyToBootEvent  = NULL;

#define PCIE_SLOT1  L"PciRoot(0x0)/Pci(0x1B" //Wildcard
#define PCIE_SLOT2  L"PciRoot(0x0)/Pci(0x1,0x0)"
#define PCIE_SLOT3  L"PciRoot(0x0)/Pci(0x1C,0x0)"
#define PCIE_SLOT4  L"PciRoot(0x0)/Pci(0x1,0x2)"
#define PCIE_SLOT5  L"PciRoot(0x0)/Pci(0x1D,0x0)"
#define PCIE_SLOT6  L"PciRoot(0x0)/Pci(0x1,0x1)"
#define GRAPHICS_IGD_OUTPUT   L"PciRoot(0x0)/Pci(0x2,0x0)"
#define GRAPHICS_KVM_OUTPUT   L"PciRoot(0x0)/Pci(0x1D,0x6)"

EFI_GRAPHICS_OUTPUT_PROTOCOL  *mGop = NULL;

/**
  Notify function for event group EFI_EVENT_GROUP_READY_TO_BOOT.

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
STATIC
VOID
EFIAPI
SMBusConfigLoaderReadyToBootEventNotify (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EFI_STATUS                      Status;
  UINTN                           HandleCount;
  EFI_HANDLE                      *Handle;
  UINTN                           Index;
  EFI_DEVICE_PATH_PROTOCOL        *TempDevicePath;
  CHAR16                          *Str;
  BOARD_SETTINGS                  BoardSettings;
  UINTN                           BoardSettingsSize;

  Handle = NULL;
  Index = 0;
  BoardSettingsSize = sizeof(BOARD_SETTINGS);

  DEBUG ((EFI_D_INFO, "SMBusConfigLoaderReadyToBootEventNotify\n"));

  // Fetch Board Settings
  Status = gRT->GetVariable(BOARD_SETTINGS_NAME,
    &gEfiBoardSettingsVariableGuid,
    NULL,
    &BoardSettingsSize,
    &BoardSettings);

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Fetching Board Settings errored with %x\n", Status));
    return;
  }

  // Locate all GOPs
  Status = gBS->LocateHandleBuffer(ByProtocol, 
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handle);

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Fetching handles errored with %x\n", Status));
    return;
  }

  DEBUG ((EFI_D_INFO, "Amount of Handles: %x\n", HandleCount));

  if (HandleCount < 2)
    return;

  // Loop through all GOPs to find the primary one
  for (Index=0; Index < HandleCount; Index++) {
    // Get Device Path of GOP
    Status = gBS->HandleProtocol (Handle[Index], &gEfiDevicePathProtocolGuid, (VOID*)&TempDevicePath);
    if (EFI_ERROR (Status)) {
      continue;
    }
    Str = ConvertDevicePathToText(TempDevicePath, FALSE, TRUE);
    DEBUG ((EFI_D_INFO, "Current Device: %s\n", Str));

    // Get Protocol of GOP
    Status = gBS->HandleProtocol (Handle[Index], &gEfiGraphicsOutputProtocolGuid, (VOID**)&mGop);
    if (EFI_ERROR (Status)) {
      continue;
    }

    // Check which GOP should be primary. Reinstall all other to move them to the end of the GOP list
    if ((!StrnCmp(Str, GRAPHICS_KVM_OUTPUT, StrLen(GRAPHICS_KVM_OUTPUT))))  {
      DEBUG ((EFI_D_INFO, "Found the KVM Device.."));
      // If Primary Video not KVM - disable.
      if (BoardSettings.PrimaryVideo != PRIMARY_VIDEO_ASPEED) {
        DEBUG ((EFI_D_INFO, "moving to end of GOP list\n"));
        Status = gBS->ReinstallProtocolInterface (
                Handle[Index],
                &gEfiGraphicsOutputProtocolGuid,
                mGop,
                mGop);
        if (EFI_ERROR (Status)) {
          DEBUG((DEBUG_ERROR, "Reinstall Handle errored with %x\n", Status));
        }
      }
      DEBUG ((EFI_D_INFO, "\n"));
    } else if (!StrnCmp(Str, GRAPHICS_IGD_OUTPUT, StrLen(GRAPHICS_IGD_OUTPUT))) {
      DEBUG ((EFI_D_INFO, "Found the IGD Device.."));
      // If Primary Video not IGD - disable.
      if (BoardSettings.PrimaryVideo != PRIMARY_VIDEO_INTEL) {
        DEBUG ((EFI_D_INFO, "moving to end of GOP list\n"));
        Status = gBS->ReinstallProtocolInterface (
                Handle[Index],
                &gEfiGraphicsOutputProtocolGuid,
                mGop,
                mGop);
        if (EFI_ERROR (Status)) {
          DEBUG((DEBUG_ERROR, "Reinstall Handle errored with %x\n", Status));
        }
      }
      DEBUG ((EFI_D_INFO, "\n"));
    } else if
    (!StrnCmp(Str, PCIE_SLOT1, StrLen(PCIE_SLOT1)))
    {
      DEBUG ((EFI_D_INFO, "Found PCI SLOT1 Graphics Device.."));
      // If Primary Video not PCI - disable.
      if (BoardSettings.PrimaryVideo != PRIMARY_VIDEO_SLOT1) {
        DEBUG ((EFI_D_INFO, "moving to end of GOP list\n"));
        Status = gBS->ReinstallProtocolInterface (
                Handle[Index],
                &gEfiGraphicsOutputProtocolGuid,
                mGop,
                mGop);
        if (EFI_ERROR (Status)) {
          DEBUG((DEBUG_ERROR, "Reinstall Handle errored with %x\n", Status));
        }
      }
      DEBUG ((EFI_D_INFO, "\n"));
    } else if
    (!StrnCmp(Str, PCIE_SLOT2, StrLen(PCIE_SLOT2)))
    {
      DEBUG ((EFI_D_INFO, "Found PCI SLOT2 Graphics Device.."));
      // If Primary Video not PEG - disable.
      if (BoardSettings.PrimaryVideo != PRIMARY_VIDEO_SLOT2) {
        DEBUG ((EFI_D_INFO, "moving to end of GOP list\n"));
        Status = gBS->ReinstallProtocolInterface (
                Handle[Index],
                &gEfiGraphicsOutputProtocolGuid,
                mGop,
                mGop);
        if (EFI_ERROR (Status)) {
          DEBUG((DEBUG_ERROR, "Reinstall Handle errored with %x\n", Status));
        }
      }

      DEBUG ((EFI_D_INFO, "\n"));
    } else if
    (!StrnCmp(Str, PCIE_SLOT3, StrLen(PCIE_SLOT3)))
    {
      DEBUG ((EFI_D_INFO, "Found PCI SLOT3 Graphics Device.."));
      // If Primary Video not PEG - disable.
      if (BoardSettings.PrimaryVideo != PRIMARY_VIDEO_SLOT3) {
        DEBUG ((EFI_D_INFO, "moving to end of GOP list\n"));
        Status = gBS->ReinstallProtocolInterface (
                Handle[Index],
                &gEfiGraphicsOutputProtocolGuid,
                mGop,
                mGop);
        if (EFI_ERROR (Status)) {
          DEBUG((DEBUG_ERROR, "Reinstall Handle errored with %x\n", Status));
        }
      }

      DEBUG ((EFI_D_INFO, "\n"));
    } else if
    (!StrnCmp(Str, PCIE_SLOT4, StrLen(PCIE_SLOT4)))
    {
      DEBUG ((EFI_D_INFO, "Found PCI SLOT4 Graphics Device.."));
      // If Primary Video not PEG - disable.
      if (BoardSettings.PrimaryVideo != PRIMARY_VIDEO_SLOT4) {
        DEBUG ((EFI_D_INFO, "moving to end of GOP list\n"));
        Status = gBS->ReinstallProtocolInterface (
                Handle[Index],
                &gEfiGraphicsOutputProtocolGuid,
                mGop,
                mGop);
        if (EFI_ERROR (Status)) {
          DEBUG((DEBUG_ERROR, "Reinstall Handle errored with %x\n", Status));
        }
      }

      DEBUG ((EFI_D_INFO, "\n"));
    } else if
    (!StrnCmp(Str, PCIE_SLOT5, StrLen(PCIE_SLOT5)))
    {
      DEBUG ((EFI_D_INFO, "Found PCI SLOT5 Graphics Device.."));
      // If Primary Video not PEG - disable.
      if (BoardSettings.PrimaryVideo != PRIMARY_VIDEO_SLOT5) {
        DEBUG ((EFI_D_INFO, "moving to end of GOP list\n"));
        Status = gBS->ReinstallProtocolInterface (
                Handle[Index],
                &gEfiGraphicsOutputProtocolGuid,
                mGop,
                mGop);
        if (EFI_ERROR (Status)) {
          DEBUG((DEBUG_ERROR, "Reinstall Handle errored with %x\n", Status));
        }
      }

      DEBUG ((EFI_D_INFO, "\n"));
    } else if
    (!StrnCmp(Str, PCIE_SLOT6, StrLen(PCIE_SLOT6)))
    {
      DEBUG ((EFI_D_INFO, "Found PCI SLOT2 Graphics Device.."));
      // If Primary Video not PEG - disable.
      if (BoardSettings.PrimaryVideo != PRIMARY_VIDEO_SLOT6) {
        DEBUG ((EFI_D_INFO, "moving to end of GOP list\n"));
        Status = gBS->ReinstallProtocolInterface (
                Handle[Index],
                &gEfiGraphicsOutputProtocolGuid,
                mGop,
                mGop);
        if (EFI_ERROR (Status)) {
          DEBUG((DEBUG_ERROR, "Uninstalling Handle errored with %x\n", Status));
        }
      }

      DEBUG ((EFI_D_INFO, "\n"));
    }

  } // for loop
  
  // Locate all GOPs
  Status = gBS->LocateHandleBuffer(ByProtocol, 
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handle);

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Fetching handles errored with %x\n", Status));
    return;
  }

  DEBUG ((EFI_D_INFO, "New order of GOPs:\n"));

  // Loop through all GOPs for debugging
  for (Index=0; Index < HandleCount; Index++) {
    // Get Device Path of GOP
    Status = gBS->HandleProtocol (Handle[Index], &gEfiDevicePathProtocolGuid, (VOID*)&TempDevicePath);
    if (EFI_ERROR (Status)) {
      continue;
    }
    Str = ConvertDevicePathToText(TempDevicePath, FALSE, TRUE);
    DEBUG ((EFI_D_INFO, " Device: %s\n", Str));
  }

  return;
}

/**
  The Entry Point for PrimaryVideoConfig driver.

  It installs DriverBinding.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InstallPrimaryVideoConfigDxe (
  IN EFI_HANDLE                        ImageHandle,
  IN EFI_SYSTEM_TABLE                  *SystemTable
  )
{
  EFI_STATUS                Status;

  DEBUG ((EFI_D_INFO, "InstallPrimaryVideoConfigDxe: Installing ReadyToBoot hook\n"));

  //
  // Register notify function to change PrimaryVideo at ReadyToBoot.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SMBusConfigLoaderReadyToBootEventNotify,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &mDxeRuntimeCapsuleLibReadyToBootEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}