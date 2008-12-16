/** @file

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PlatOverMngr.h

Abstract:

  Function prototype for platform driver override manager driver

**/

#ifndef _PLAT_OVER_MNGR_H_
#define _PLAT_OVER_MNGR_H_

#include <PiDxe.h>

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/PciIo.h>
#include <Protocol/BusSpecificDriverOverride.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DevicePathToText.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PlatDriOverLib.h>
#include <Library/IfrSupportLib.h>
#include <Library/ExtendedIfrSupportLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/ExtendedHiiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>

#define MIN_ALIGNMENT_SIZE  4
#define ALIGN_SIZE(a)       ((a % MIN_ALIGNMENT_SIZE) ? MIN_ALIGNMENT_SIZE - (a % MIN_ALIGNMENT_SIZE) : 0)


#define EFI_CALLBACK_INFO_SIGNATURE SIGNATURE_32 ('C', 'l', 'b', 'k')
#define EFI_CALLBACK_INFO_FROM_THIS(a)  CR (a, EFI_CALLBACK_INFO, ConfigAccess, EFI_CALLBACK_INFO_SIGNATURE)
#define MAX_CHOICE_NUM    0x100
#define UPDATE_DATA_SIZE  0x1000


extern UINT8  VfrBin[];

extern UINT8  PlatOverMngrStrings[];

//
// Following definition is the same as in vfr file
//
#define PLAT_OVER_MNGR_GUID \
  { \
    0x8614567d, 0x35be, 0x4415, {0x8d, 0x88, 0xbd, 0x7d, 0xc, 0x9c, 0x70, 0xc0} \
  }

typedef struct {
  UINT8   DriSelection[100];
  UINT8   DriOrder[100];
  UINT8   PciDeviceFilter;
} PLAT_OVER_MNGR_DATA;

#define FORM_ID_DEVICE                 0x1234
#define FORM_ID_DRIVER                 0x1200
#define FORM_ID_ORDER                  0x1500

#define KEY_VALUE_DEVICE_OFFSET        0x0100
#define KEY_VALUE_DEVICE_MAX           0x04ff

#define QUESTION_ID_OFFSET             0x0500

#define KEY_VALUE_DEVICE_REFRESH       0x1234
#define KEY_VALUE_DEVICE_FILTER        0x1235
#define KEY_VALUE_DEVICE_CLEAR         0x1236

#define KEY_VALUE_DRIVER_GOTO_PREVIOUS 0x1300
#define KEY_VALUE_DRIVER_GOTO_ORDER    0x1301

#define KEY_VALUE_ORDER_GOTO_PREVIOUS  0x2000
#define KEY_VALUE_ORDER_SAVE_AND_EXIT  0x1800

#define VARSTORE_ID_PLAT_OVER_MNGR     0x1000

//
// Question Id start from 1, so define an offset for it
//
#define VAR_OFFSET(Field)              ((UINTN) &(((PLAT_OVER_MNGR_DATA *) 0)->Field))

#define DRIVER_SELECTION_VAR_OFFSET     (VAR_OFFSET (DriSelection))
#define DRIVER_ORDER_VAR_OFFSET         (VAR_OFFSET (DriOrder))

#define DRIVER_SELECTION_QUESTION_ID    (VAR_OFFSET (DriSelection) + QUESTION_ID_OFFSET)
#define DRIVER_ORDER_QUESTION_ID        (VAR_OFFSET (DriOrder) + QUESTION_ID_OFFSET)

typedef struct {
  UINTN                           Signature;

  EFI_HANDLE                      DriverHandle;
  EFI_HII_HANDLE                  RegisteredHandle;
  PLAT_OVER_MNGR_DATA             FakeNvData;

  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;

  EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;
} EFI_CALLBACK_INFO;

typedef struct {
  EFI_DRIVER_CONFIGURATION_PROTOCOL         *DriverConfiguration;
  EFI_HANDLE                                DriverImageHandle;
  EFI_HANDLE                                ControllerHandle;
  EFI_HANDLE                                ChildControllerHandle;
  //
  // To avoid created string leak in Hii database, use this token to reuse every token created by the driver
  //
  EFI_STRING_ID                             DescriptionToken;
} CFG_PROTOCOL_INVOKER_CHOICE;

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param  This         Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request      A null-terminated Unicode string in <ConfigRequest> format.
  @param  Progress     On return, points to a character in the Request string.
                       Points to the string's null terminator if request was successful.
                       Points to the most recent '&' before the first failing name/value
                       pair (or the beginning of the string if the failure is in the
                       first name/value pair) if the request was not successful.
  @param  Results      A null-terminated Unicode string in <ConfigAltResp> format which
                       has all values filled in for the names in the Request string.
                       String to be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
PlatOverMngrExtractConfig (
 IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  );

/**
  This function processes the results of changes in configuration.

  @param  This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration   A null-terminated Unicode string in <ConfigRequest> format.
  @param  Progress        A pointer to a string filled in with the offset of the most
                          recent '&' before the first failing name/value pair (or the
                          beginning of the string if the failure is in the first
                          name/value pair) or the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
PlatOverMngrRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  );

/**
  This is the function that is called to provide results data to the driver.  This data
  consists of a unique key which is used to identify what data is either being passed back
  or being asked for.

  @param  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action         A null-terminated Unicode string in <ConfigRequest> format.
  @param  KeyValue       A unique Goto OpCode callback value which record user's selection.
                         0x100 <= KeyValue <0x500 : user select a controller item in the first page;
                         KeyValue == 0x1234       : user select 'Refresh' in first page, or user select 'Go to Previous Menu' in second page
                         KeyValue == 0x1235       : user select 'Pci device filter' in first page
                         KeyValue == 0x1500       : user select 'order ... priority' item in second page
                         KeyValue == 0x1800       : user select 'commint changes' in third page
                         KeyValue == 0x2000       : user select 'Go to Previous Menu' in third page
  @param  Type           The type of value for the question.
  @param  Value          A pointer to the data being sent to the original exporting driver.
  @param  ActionRequest  On return, points to the action requested by the callback function.

  @retval EFI_SUCCESS    Always returned.

**/
EFI_STATUS
EFIAPI
PlatOverMngrCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        KeyValue,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );

/**
  Get the image name

  @param Image            Image to search.

  @retval !NULL           Pointer into the image name if the image name is found,
  @retval NULL            Pointer to NULL if the image name is not found.

**/
CHAR16 *
GetImageName (
  IN EFI_LOADED_IMAGE_PROTOCOL *Image
  );

/**
  Get the description string by device path.

  @param  DevPath     The input device path.

  @retval !NULL       The description string retured.
  @retval  NULL       The description string cannot be found.

**/
CHAR16 *
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  );

#endif
