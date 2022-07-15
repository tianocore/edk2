/** @file
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleTextIn.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "SdBootConfig.h"

extern UINT8  SdBootConfigHiiBin[];

typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

STATIC HII_VENDOR_DEVICE_PATH  mSdBootConfigVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    SD_BOOT_CONFIG_FORMSET_GUID,
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

STATIC
EFI_STATUS
SdBootConfigInstallHiiPages (
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
                        &mSdBootConfigVendorDevicePath,
                        NULL
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiHandle = HiiAddPackages (
                &gSdBootConfigFormSetGuid,
                DriverHandle,
                SdBootConfigStrings,
                SdBootConfigHiiBin,
                NULL
                );

  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mSdBootConfigVendorDevicePath,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SdBootConfigCreateReadVariable (
  SD_BOOT_CONFIG_DATA  *Config
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;

  BufferSize = sizeof (*Config);
  Status     = gRT->GetVariable (
                      L"SdBootConfig",
                      &gSdBootConfigFormSetGuid,
                      NULL,
                      &BufferSize,
                      Config
                      );
  if (Status == EFI_SUCCESS) {
    // variable exists, good
    return Status;
  }

  Config->AddEntry = 0;
  Status           = gRT->SetVariable (
                            L"SdBootConfig",
                            &gSdBootConfigFormSetGuid,
                            EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                            sizeof (*Config),
                            Config
                            );
  return Status;
}

EFI_STATUS
EFIAPI
SdBootConfigFvDevicePath (
  EFI_GUID                  *FileGuid,
  EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  )
{
  EFI_STATUS                         Status;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *DevicePath = DevicePathFromHandle (LoadedImage->DeviceHandle);
  ASSERT (*DevicePath != NULL);

  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);
  *DevicePath = AppendDevicePathNode (
                  *DevicePath,
                  (EFI_DEVICE_PATH_PROTOCOL *)&FileNode
                  );
  ASSERT (*DevicePath != NULL);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SdBootConfigBootConfig (
  SD_BOOT_CONFIG_DATA  *Config
  )
{
  EFI_STATUS                    Status;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_BOOT_MANAGER_LOAD_OPTION  SdBootOption;
  INTN                          SdBootIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         BootOptionCount;

  Status = SdBootConfigFvDevicePath (&gSdBootFileGuid, &DevicePath);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EfiBootManagerInitializeLoadOption (
             &SdBootOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             LOAD_OPTION_ACTIVE,
             L"systemd boot manager",
             DevicePath,
             NULL,
             0
             );
  ASSERT_EFI_ERROR (Status);
  FreePool (DevicePath);

  BootOptions = EfiBootManagerGetLoadOptions (
                  &BootOptionCount,
                  LoadOptionTypeBoot
                  );

  SdBootIndex = EfiBootManagerFindLoadOption (
                  &SdBootOption,
                  BootOptions,
                  BootOptionCount
                  );

  DEBUG ((
    DEBUG_INFO,
    "%a: index is %d, entry is %a\n",
    __func__,
    SdBootIndex,
    Config->AddEntry ? "enabled" : "disabled"
    ));
  if ((SdBootIndex == -1) && Config->AddEntry) {
    DEBUG ((DEBUG_INFO, "%a: adding entry\n", __func__, SdBootIndex));
    Status = EfiBootManagerAddLoadOptionVariable (&SdBootOption, 0);
    ASSERT_EFI_ERROR (Status);
  }

  if ((SdBootIndex != -1) && !Config->AddEntry) {
    DEBUG ((DEBUG_INFO, "%a: removing entry\n", __func__, SdBootIndex));
    Status = EfiBootManagerDeleteLoadOptionVariable (
               BootOptions[SdBootIndex].OptionNumber,
               LoadOptionTypeBoot
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (Config->AddEntry) {
    UINT16  OptionNumber = ((SdBootIndex == -1)
                           ? SdBootOption.OptionNumber
                           : BootOptions[SdBootIndex].OptionNumber);
    EFI_INPUT_KEY  HotKey = {
      .ScanCode    = SCAN_F5,
      .UnicodeChar = CHAR_NULL,
    };
    Status = EfiBootManagerAddKeyOptionVariable (
               NULL,
               OptionNumber,
               0,
               &HotKey,
               NULL
               );
  }

  EfiBootManagerFreeLoadOption (&SdBootOption);
  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SdBootConfigEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;
  SD_BOOT_CONFIG_DATA  Config;

  Status = SdBootConfigCreateReadVariable (&Config);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // TODO: allow host override (via fw_cfg?) for SdBootConfig

  SdBootConfigBootConfig (&Config);

  Status = SdBootConfigInstallHiiPages ();
  return Status;
}
