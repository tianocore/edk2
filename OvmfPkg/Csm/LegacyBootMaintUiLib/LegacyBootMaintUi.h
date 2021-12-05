/** @file
  Legacy boot maintenance Ui definition.

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_LEGACY_BOOT_OPTION_H_
#define _EFI_LEGACY_BOOT_OPTION_H_

#include <PiDxe.h>

#include <Guid/GlobalVariable.h>
#include <Guid/LegacyDevOrder.h>
#include <Guid/MdeModuleHii.h>

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>

#include <Protocol/HiiDatabase.h>
#include <Protocol/LegacyBios.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>

#include "LegacyBootMaintUiVfr.h"

#define CONFIG_OPTION_OFFSET  0x1200

//
// VarOffset that will be used to create question
// all these values are computed from the structure
// defined below
//
#define VAR_OFFSET(Field)  ((UINT16) ((UINTN) &(((LEGACY_BOOT_NV_DATA *) 0)->Field)))

//
// Question Id of Zero is invalid, so add an offset to it
//
#define QUESTION_ID(Field)  (VAR_OFFSET (Field) + CONFIG_OPTION_OFFSET)

#define LEGACY_FD_QUESTION_ID   QUESTION_ID (LegacyFD)
#define LEGACY_HD_QUESTION_ID   QUESTION_ID (LegacyHD)
#define LEGACY_CD_QUESTION_ID   QUESTION_ID (LegacyCD)
#define LEGACY_NET_QUESTION_ID  QUESTION_ID (LegacyNET)
#define LEGACY_BEV_QUESTION_ID  QUESTION_ID (LegacyBEV)

//
// String Constant
//
#define STR_FLOPPY    L"Floppy Drive #%02x"
#define STR_HARDDISK  L"HardDisk Drive #%02x"
#define STR_CDROM     L"ATAPI CDROM Drive #%02x"
#define STR_NET       L"NET Drive #%02x"
#define STR_BEV       L"BEV Drive #%02x"

#define STR_FLOPPY_HELP    L"Select Floppy Drive #%02x"
#define STR_HARDDISK_HELP  L"Select HardDisk Drive #%02x"
#define STR_CDROM_HELP     L"Select ATAPI CDROM Drive #%02x"
#define STR_NET_HELP       L"NET Drive #%02x"
#define STR_BEV_HELP       L"BEV Drive #%02x"

#define STR_FLOPPY_TITLE    L"Set Legacy Floppy Drive Order"
#define STR_HARDDISK_TITLE  L"Set Legacy HardDisk Drive Order"
#define STR_CDROM_TITLE     L"Set Legacy CDROM Drive Order"
#define STR_NET_TITLE       L"Set Legacy NET Drive Order"
#define STR_BEV_TITLE       L"Set Legacy BEV Drive Order"

//
// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8  LegacyBootMaintUiVfrBin[];

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

//
// Variable created with this flag will be "Efi:...."
//
#define VAR_FLAG  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE

#define LEGACY_BOOT_OPTION_CALLBACK_DATA_SIGNATURE  SIGNATURE_32 ('L', 'G', 'C', 'B')

typedef struct {
  UINTN                             Signature;

  //
  // HII relative handles
  //
  EFI_HII_HANDLE                    HiiHandle;
  EFI_HANDLE                        DriverHandle;

  //
  // Produced protocols
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;

  //
  // Maintain the data.
  //
  LEGACY_BOOT_MAINTAIN_DATA         *MaintainMapData;
} LEGACY_BOOT_OPTION_CALLBACK_DATA;

//
// All of the signatures that will be used in list structure
//
#define LEGACY_MENU_OPTION_SIGNATURE  SIGNATURE_32 ('m', 'e', 'n', 'u')
#define LEGACY_MENU_ENTRY_SIGNATURE   SIGNATURE_32 ('e', 'n', 't', 'r')

#define LEGACY_LEGACY_DEV_CONTEXT_SELECT  0x9

typedef struct {
  UINTN         Signature;
  LIST_ENTRY    Head;
  UINTN         MenuNumber;
} LEGACY_MENU_OPTION;

typedef struct {
  UINT16    BbsIndex;
  CHAR16    *Description;
} LEGACY_DEVICE_CONTEXT;

typedef struct {
  UINTN            Signature;
  LIST_ENTRY       Link;
  UINTN            OptionNumber;
  UINT16           *DisplayString;
  UINT16           *HelpString;
  EFI_STRING_ID    DisplayStringToken;
  EFI_STRING_ID    HelpStringToken;
  VOID             *VariableContext;
} LEGACY_MENU_ENTRY;

typedef struct {
  UINT16    BbsIndex;
} LEGACY_BOOT_OPTION_BBS_DATA;

#pragma pack()

/**
  This call back function is registered with Boot Manager formset.
  When user selects a boot option, this call back function will
  be triggered. The boot option is saved for later processing.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
LegacyBootOptionCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  EFI_BROWSER_ACTION                    Action,
  IN  EFI_QUESTION_ID                       QuestionId,
  IN  UINT8                                 Type,
  IN  EFI_IFR_TYPE_VALUE                    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  );

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         - A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        - On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         - A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
LegacyBootOptionExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  );

/**
  This function processes the results of changes in configuration.


  @param This            - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   - A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        - A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
LegacyBootOptionRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  );

#endif
