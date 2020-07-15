/** @file
  Public Header file of HII library used by Opal UEFI Driver.
  Defines required callbacks of Opal HII library.

Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _OPAL_HII_H_
#define _OPAL_HII_H_

#include <Protocol/HiiConfigAccess.h>

#include "OpalDriver.h"
#include "OpalHiiFormValues.h"

#define  OPAL_PASSWORD_CONFIG_GUID \
  { \
    0x0d510a4f, 0xa81b, 0x473f, { 0x87, 0x07, 0xb7, 0xfd, 0xfb, 0xc0, 0x45, 0xba } \
  }

#define OPAL_REQUEST_VARIABLE_NAME     L"OpalRequest"

#pragma pack(1)

typedef struct {
  UINT32                   Length;
  OPAL_REQUEST             OpalRequest;
  //EFI_DEVICE_PATH_PROTOCOL OpalDevicePath;
} OPAL_REQUEST_VARIABLE;

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

#pragma pack()

extern const EFI_GUID gHiiSetupVariableGuid;

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

  Populate the hii_g_Configuration with the browser Data.

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

  @param      PpRequest      Input the Pp Request.

  @retval  EFI_SUCCESS       Do the required action success.
  @retval  Others            Other error occur.

**/
EFI_STATUS
HiiSetBlockSidAction (
  UINT32          PpRequest
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

  @retval EFI_SUCCESS            Initialize the device success.
  @retval EFI_DEVICE_ERROR       Get info from device failed.
  @retval EFI_INVALID_PARAMETER  Not get Msid info before get ownership info.

**/
EFI_STATUS
OpalDiskUpdateStatus (
  OPAL_DISK        *OpalDisk
  );

/**
  Get the driver image handle.

  @retval  the driver image handle.

**/
EFI_HANDLE
HiiGetDriverImageHandleCB(
  VOID
  );

/**
  Install the HII form and string packages.

  @retval  EFI_SUCCESS           Install all the resources success.
  @retval  EFI_OUT_OF_RESOURCES  Out of resource error.
**/
EFI_STATUS
OpalHiiAddPackages(
  VOID
  );

/**
  Returns the opaque pointer to a physical disk context.

  @param  DiskIndex       Input the disk index.

  @retval The device pointer.

**/
OPAL_DISK*
HiiGetOpalDiskCB(
  UINT8 DiskIndex
  );

/**
  Returns the disk name.

  @param  DiskIndex       Input the disk index.

  @retval Returns the disk name.

**/
CHAR8*
HiiDiskGetNameCB(
  UINT8 DiskIndex
  );

/**
  Set a string Value in a form.

  @param      DestStringId   The stringid which need to update.
  @param      SrcAsciiStr    The string nned to update.

  @retval  EFI_SUCCESS       Do the required action success.
  @retval  Others            Other error occur.

**/
EFI_STATUS
HiiSetFormString(
  EFI_STRING_ID       DestStringId,
  CHAR8               *SrcAsciiStr
  );

/**
  Install the HII related resources.

  @retval  EFI_SUCCESS        Install all the resources success.
  @retval  other              Error occur when install the resources.
**/
EFI_STATUS
HiiInstall(
  VOID
  );

/**
  Uninstall the HII capability.

  @retval  EFI_SUCCESS           Uninstall all the resources success.
  @retval  others                Other errors occur when unistall the hii resource.
**/
EFI_STATUS
HiiUninstall(
  VOID
  );

/**
  Initialize the Opal disk base on the hardware info get from device.

  @param Dev                  The Opal device.

  @retval EFI_SUCCESS         Initialize the device success.
  @retval EFI_DEVICE_ERROR    Get info from device failed.

**/
EFI_STATUS
OpalDiskInitialize (
  IN OPAL_DRIVER_DEVICE          *Dev
  );

/**
  Update the device ownership

  @param OpalDisk                The Opal device.

  @retval EFI_SUCCESS            Get ownership success.
  @retval EFI_ACCESS_DENIED      Has send BlockSID command, can't change ownership.
  @retval EFI_INVALID_PARAMETER  Not get Msid info before get ownership info.

**/
EFI_STATUS
OpalDiskUpdateOwnerShip (
  OPAL_DISK        *OpalDisk
  );

#endif // _HII_H_
