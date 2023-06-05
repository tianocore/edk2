/** @file
  HII-to-Redfish memory driver header file.

  (C) Copyright 2022 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HII_2_REDFISH_BOOT_DXE_H_
#define HII_2_REDFISH_BOOT_DXE_H_

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/RedfishEventLib.h>

#include <Protocol/HiiConfigAccess.h>

#include <Guid/VariableFormat.h>
#include <Guid/MdeModuleHii.h>

#include "Hii2RedfishBootData.h"

extern UINT8  Hii2RedfishBootVfrBin[];

#define COMPUTER_SYSTEM_SECHEMA_VERSION  "x-uefi-redfish-ComputerSystem.v1_5_0"
#define COMPUTER_SYSTEM_BOOT_BOOTORDER   L"/Systems/{1}/Boot/BootOrder"

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

#endif
