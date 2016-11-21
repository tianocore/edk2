/** @file
  Private functions and sturctures used by the Opal UEFI Driver.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _OPAL_HII_PRIVATE_H_
#define _OPAL_HII_PRIVATE_H_



#include <Library/OpalPasswordSupportLib.h>
#include <Protocol/HiiConfigAccess.h>

#include "OpalHii.h"
#include "OpalHiiFormValues.h"


#define  OPAL_PASSWORD_CONFIG_GUID \
  { \
    0x0d510a4f, 0xa81b, 0x473f, { 0x87, 0x07, 0xb7, 0xfd, 0xfb, 0xc0, 0x45, 0xba } \
  }

#pragma pack(1)

typedef struct {
  UINT16 Id: HII_KEY_ID_BITS;
  UINT16 Index: HII_KEY_INDEX_BITS;
  UINT16 Flag: HII_KEY_FLAG_BITS;
} KEY_BITS;

typedef union {
    UINT16    Raw;
    KEY_BITS  KeyBits;
} HII_KEY;

typedef struct {
    VENDOR_DEVICE_PATH             VendorDevicePath;
    EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

/**
* Opal PSID Authority utilized for PSID revert
*
* The type indicates the structure of the PSID authority
*/
typedef struct {
    UINT8 Psid[PSID_CHARACTER_LENGTH];
} TCG_PSID;

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration          A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param  Progress               A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
RouteConfig(
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  CONST EFI_STRING                        Configuration,
  EFI_STRING                              *Progress
  );

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request                A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param  Progress               On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param  Results                A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
ExtractConfig(
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL    *This,
  CONST EFI_STRING                        Request,
  EFI_STRING                              *Progress,
  EFI_STRING                              *Results
  );

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS
EFIAPI
DriverCallback(
  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL*   This,
  EFI_BROWSER_ACTION                      Action,
  EFI_QUESTION_ID                         QuestionId,
  UINT8                                   Type,
  EFI_IFR_TYPE_VALUE*                     Value,
  EFI_BROWSER_ACTION_REQUEST*             ActionRequest
  );

/**

  Pass the current system state to the bios via the hii_G_Configuration.

**/
VOID
OpalHiiSetBrowserData (
  VOID
  );

/**

  Populate the hii_g_Configuraton with the browser Data.

**/
VOID
OpalHiiGetBrowserData (
  VOID
  );

/**
  Draws the disk info form.

  @retval  EFI_SUCCESS       Draw the disk info success.

**/
EFI_STATUS
HiiPopulateDiskInfoForm(
  VOID
  );

/**
  Update the global Disk index info.

  @param   Index             The input disk index info.

  @retval  EFI_SUCCESS       Update the disk index info success.

**/
EFI_STATUS
HiiSelectDisk(
  UINT8 Index
  );

/**
  Use the input password to do the specified action.

  @param      Str            The input password saved in.

  @retval  EFI_SUCCESS       Do the required action success.
  @retval  Others            Other error occur.

**/
EFI_STATUS
HiiPasswordEntered(
  EFI_STRING_ID            Str
  );

/**
  Update block sid info.

  @param      Enable         Enable/disable BlockSid.

  @retval  EFI_SUCCESS       Do the required action success.
  @retval  Others            Other error occur.

**/
EFI_STATUS
HiiSetBlockSid (
  BOOLEAN          Enable
  );

/**
  Reverts the Opal disk to factory default.

  @param   PsidStringId      The string id for the PSID info.

  @retval  EFI_SUCCESS       Do the required action success.

**/
EFI_STATUS
HiiPsidRevert(
  EFI_STRING_ID         PsidStringId
  );

/**
  Get disk name string id.

  @param   DiskIndex             The input disk index info.

  @retval  The disk name string id.

**/
EFI_STRING_ID
GetDiskNameStringId(
  UINT8 DiskIndex
  );

/**
  Update the device info.

  @param OpalDisk                The Opal device.

  @retval EFI_SUCESS             Initialize the device success.
  @retval EFI_DEVICE_ERROR       Get info from device failed.
  @retval EFI_INVALID_PARAMETER  Not get Msid info before get ownership info.

**/
EFI_STATUS
OpalDiskUpdateStatus (
  OPAL_DISK        *OpalDisk
  );

#pragma pack()

#endif // _HII_P_H_
