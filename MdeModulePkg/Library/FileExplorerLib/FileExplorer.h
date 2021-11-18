/** @file
   File explorer lib.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _FILE_EXPLORER_H_
#define _FILE_EXPLORER_H_

#include <PiDxe.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Guid/FileInfo.h>
#include <Guid/MdeModuleHii.h>

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/FormBrowser2.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/FileExplorerLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>

#include "FormGuid.h"

#define FILE_EXPLORER_CALLBACK_DATA_SIGNATURE  SIGNATURE_32 ('f', 'e', 'c', 'k')

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

typedef struct {
  EFI_HANDLE                  DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  EFI_FILE_HANDLE             FileHandle;
  UINT16                      *FileName;

  BOOLEAN                     IsRoot;
  BOOLEAN                     IsDir;
} FILE_CONTEXT;

typedef struct {
  UINTN            Signature;
  LIST_ENTRY       Link;
  UINT16           *DisplayString;
  UINT16           *HelpString;
  EFI_STRING_ID    DisplayStringToken;
  EFI_STRING_ID    HelpStringToken;
  VOID             *VariableContext;
} MENU_ENTRY;

typedef struct {
  UINTN         Signature;
  LIST_ENTRY    Head;
  UINTN         MenuNumber;
  BOOLEAN       Used;
} MENU_OPTION;

typedef struct {
  //
  // Shared callback data.
  //
  UINTN                             Signature;

  //
  // File explorer formset callback data.
  //
  EFI_HII_HANDLE                    FeHiiHandle;
  EFI_HANDLE                        FeDriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    FeConfigAccess;
  EFI_FORM_BROWSER2_PROTOCOL        *FormBrowser2;
  MENU_OPTION                       *FsOptionMenu;
  CHAR16                            *FileType;
  CHOOSE_HANDLER                    ChooseHandler;
  EFI_DEVICE_PATH_PROTOCOL          *RetDevicePath;
} FILE_EXPLORER_CALLBACK_DATA;

#define FILE_EXPLORER_PRIVATE_FROM_THIS(a)  CR (a, FILE_EXPLORER_CALLBACK_DATA, FeConfigAccess, FILE_EXPLORER_CALLBACK_DATA_SIGNATURE)

extern UINT8  FileExplorerVfrBin[];

#define MENU_OPTION_SIGNATURE  SIGNATURE_32 ('m', 'e', 'n', 'u')
#define MENU_ENTRY_SIGNATURE   SIGNATURE_32 ('e', 'n', 't', 'r')

///
/// Define the maximum characters that will be accepted.
///
#define MAX_CHAR                     480
#define FILE_OPTION_OFFSET           0x8000
#define FILE_OPTION_MASK             0x7FFF
#define QUESTION_ID_UPDATE_STEP      200
#define MAX_FILE_NAME_LEN            20
#define MAX_FOLDER_NAME_LEN          20
#define NEW_FILE_QUESTION_ID_BASE    0x5000;
#define NEW_FOLDER_QUESTION_ID_BASE  0x6000;

/**
  This function processes the results of changes in configuration.
  When user select a interactive opcode, this callback will be triggered.
  Based on the Question(QuestionId) that triggers the callback, the corresponding
  actions is performed. It handles:

  1) the addition of boot option.
  2) the addition of driver option.
  3) exit from file browser
  4) update of file content if a dir is selected.
  5) boot the file if a file is selected in "boot from file"


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval  EFI_DEVICE_ERROR      The variable could not be saved.
  @retval  EFI_UNSUPPORTED       The specified Action is not supported by the callback.

**/
EFI_STATUS
EFIAPI
LibCallback (
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
LibExtractConfig (
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
LibRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  );

/**
  Update the file explower page with the refershed file system.

  @param KeyValue        Key value to identify the type of data to expect.

  @retval  EFI_SUCCESS   Update the file explorer form success.
  @retval  other errors  Error occur when parse one directory.

**/
EFI_STATUS
LibUpdateFileExplorer (
  IN UINT16  KeyValue
  );

/**
  Get the device path info saved in the menu structure.

  @param KeyValue        Key value to identify the type of data to expect.

**/
VOID
LibGetDevicePath (
  IN UINT16  KeyValue
  );

#endif
