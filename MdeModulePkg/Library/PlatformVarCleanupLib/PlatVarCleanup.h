/** @file
  Include file for platform variable cleanup.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PLAT_VAR_CLEANUP_
#define _PLAT_VAR_CLEANUP_

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/PlatformVarCleanupLib.h>

#include <Protocol/Variable.h>
#include <Protocol/VarCheck.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/DevicePath.h>

#include <Guid/EventGroup.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/VarErrorFlag.h>

#include "PlatVarCleanupHii.h"

//
// This is the generated IFR binary data for each formset defined in VFR.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8 PlatVarCleanupBin[];

//
// This is the generated String package data for all .UNI files.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8 PlatformVarCleanupLibStrings[];

#define USER_VARIABLE_NODE_SIGNATURE SIGNATURE_32 ('U', 'V', 'N', 'S')

typedef struct {
  UINTN             Signature;
  LIST_ENTRY        Link;
  EFI_GUID          Guid;
  CHAR16            *PromptString;
  LIST_ENTRY        NameLink;
} USER_VARIABLE_NODE;

#define USER_VARIABLE_FROM_LINK(a) CR (a, USER_VARIABLE_NODE, Link, USER_VARIABLE_NODE_SIGNATURE)

#define USER_VARIABLE_NAME_NODE_SIGNATURE SIGNATURE_32 ('U', 'V', 'N', 'N')

typedef struct {
  UINTN             Signature;
  LIST_ENTRY        Link;
  CHAR16            *Name;
  UINTN             DataSize;
  UINT32            Attributes;
  UINT16            Index;
  EFI_QUESTION_ID   QuestionId;
  CHAR16            *PromptString;
  CHAR16            *HelpString;
  BOOLEAN           Deleted;
} USER_VARIABLE_NAME_NODE;

#define USER_VARIABLE_NAME_FROM_LINK(a) CR (a, USER_VARIABLE_NAME_NODE, Link, USER_VARIABLE_NAME_NODE_SIGNATURE)

#pragma pack(1)
//
// HII specific Vendor Device Path definition.
//
typedef struct {
  VENDOR_DEVICE_PATH            VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      End;
} HII_VENDOR_DEVICE_PATH;
#pragma pack()

#define VARIABLE_CLEANUP_HII_PRIVATE_SIGNATURE SIGNATURE_32 ('V', 'C', 'H', 'P')

typedef struct {
  UINTN                             Signature;
  EFI_HANDLE                        DriverHandle;
  EFI_HII_HANDLE                    HiiHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  EFI_HII_CONFIG_ROUTING_PROTOCOL   *ConfigRouting;
  VARIABLE_CLEANUP_DATA             VariableCleanupData;
} VARIABLE_CLEANUP_HII_PRIVATE_DATA;

#define VARIABLE_CLEANUP_HII_PRIVATE_FROM_THIS(a) CR (a, VARIABLE_CLEANUP_HII_PRIVATE_DATA, ConfigAccess, VARIABLE_CLEANUP_HII_PRIVATE_SIGNATURE)

#endif
