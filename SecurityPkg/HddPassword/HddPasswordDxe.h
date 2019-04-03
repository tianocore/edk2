/** @file

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HDD_PASSWORD_DXE_H_
#define _HDD_PASSWORD_DXE_H_

#include <Uefi.h>

#include <IndustryStandard/Atapi.h>
#include <IndustryStandard/Pci.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/PciIo.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/VariableLock.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/EventGroup.h>
#include <Guid/S3StorageDeviceInitList.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/LockBoxLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/PciLib.h>
#include <Library/BaseCryptLib.h>

#include "HddPasswordCommon.h"
#include "HddPasswordHiiDataStruc.h"

//
// This is the generated IFR binary data for each formset defined in VFR.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  HddPasswordBin[];

//
// This is the generated String package data for all .UNI files.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  HddPasswordDxeStrings[];

#define HDD_PASSWORD_DXE_PRIVATE_SIGNATURE SIGNATURE_32 ('H', 'D', 'D', 'P')

typedef struct _HDD_PASSWORD_CONFIG_FORM_ENTRY {
  LIST_ENTRY                    Link;
  EFI_HANDLE                    Controller;
  UINTN                         Bus;
  UINTN                         Device;
  UINTN                         Function;
  UINT16                        Port;
  UINT16                        PortMultiplierPort;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  CHAR16                        HddString[64];
  CHAR8                         Password[HDD_PASSWORD_MAX_LENGTH];
  EFI_STRING_ID                 TitleToken;
  EFI_STRING_ID                 TitleHelpToken;

  HDD_PASSWORD_CONFIG           IfrData;
  EFI_ATA_PASS_THRU_PROTOCOL    *AtaPassThru;
} HDD_PASSWORD_CONFIG_FORM_ENTRY;

typedef struct _HDD_PASSWORD_DXE_PRIVATE_DATA {
  UINTN                            Signature;
  EFI_HANDLE                       DriverHandle;
  EFI_HII_HANDLE                   HiiHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL   ConfigAccess;
  HDD_PASSWORD_CONFIG_FORM_ENTRY   *Current;
} HDD_PASSWORD_DXE_PRIVATE_DATA;

#define HDD_PASSWORD_DXE_PRIVATE_FROM_THIS(a)  CR (a, HDD_PASSWORD_DXE_PRIVATE_DATA, ConfigAccess, HDD_PASSWORD_DXE_PRIVATE_SIGNATURE)

//
//Iterate through the doule linked list. NOT delete safe
//
#define EFI_LIST_FOR_EACH(Entry, ListHead)    \
  for (Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

#define PASSWORD_SALT_SIZE                  32

#define HDD_PASSWORD_REQUEST_VARIABLE_NAME  L"HddPasswordRequest"

//
// It needs to be locked before EndOfDxe.
//
#define HDD_PASSWORD_VARIABLE_NAME          L"HddPassword"

#pragma pack(1)

typedef struct {
  HDD_PASSWORD_DEVICE   Device;
  HDD_PASSWORD_REQUEST  Request;
} HDD_PASSWORD_REQUEST_VARIABLE;

//
// It will be used to validate HDD password when the device is at frozen state.
//
typedef struct {
  HDD_PASSWORD_DEVICE   Device;
  UINT8                 PasswordHash[SHA256_DIGEST_SIZE];
  UINT8                 PasswordSalt[PASSWORD_SALT_SIZE];
} HDD_PASSWORD_VARIABLE;

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH           VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL     End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

//
// Time out value for ATA pass through protocol
//
#define ATA_TIMEOUT        EFI_TIMER_PERIOD_SECONDS (3)

typedef struct {
  UINT32                   Address;
  S3_BOOT_SCRIPT_LIB_WIDTH Width;
} HDD_HC_PCI_REGISTER_SAVE;

#endif
