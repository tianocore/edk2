/** @file
  A Setup Menu for configuring boot options defined by bootloader CFR.
  This file implements the HII Config Access protocol.

  Copyright (c) 2023, 9elements GmbH.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SetupMenu.h"
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Guid/VariableFormat.h>

SETUP_MENU_CALLBACK_DATA  mSetupMenuPrivate = {
  SETUP_MENU_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    SetupMenuExtractConfig,
    SetupMenuRouteConfig,
    SetupMenuCallback
  }
};

EFI_GUID  mSetupMenuFormsetGuid = SETUP_MENU_FORMSET_GUID;

HII_VENDOR_DEVICE_PATH  mSetupMenuHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    SETUP_MENU_FORMSET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

/**
  Parses a HII config string for the variable name.
  1. Find offset of "NAME=" value.
  2. Convert string value in Unicode-encoded hex to ASCII.
  3. Fixup Unicode string.

  It's assumed that "NAME=" has a value, hopefully tolerated.
  It's assumed that ConfigRouting generates endian-specific strings,
    so `SwapBytes16` is always required on `StrHexToBytes` byte-array.

  Caller to free VariableString pool.

**/
STATIC
CHAR16 *
EFIAPI
ConvertHiiConfigStringToVariableString (
  IN  EFI_STRING  HiiConfigString
  )
{
  EFI_STRING  ConfigStringNameStart;
  EFI_STRING  ConfigStringNameEnd;
  UINTN       ConfigStringNameLengthChars;
  UINTN       ConvertStringLengthBytes;
  UINTN       VariableStringLengthChars;
  CHAR16      *VariableString;
  UINTN       Index;
  EFI_STATUS  Status;

  ConfigStringNameStart = HiiConfigString;

  //
  // Find the start and length of variable name in stringified hex
  //
  ConfigStringNameStart = StrStr (ConfigStringNameStart, L"&NAME=");
  ASSERT (ConfigStringNameStart != NULL);
  ConfigStringNameStart += StrLen (L"&NAME=");

  ConfigStringNameEnd = StrStr (ConfigStringNameStart, L"&");
  ASSERT (ConfigStringNameEnd != NULL);

  ConfigStringNameLengthChars = ConfigStringNameEnd - ConfigStringNameStart;

  //
  // Convert stringified hex to bytes, then fixup to endian-correct string
  //
  ConvertStringLengthBytes = ConfigStringNameLengthChars / 2;
  VariableStringLengthChars = ConvertStringLengthBytes / 2;
  VariableString = AllocatePool (ConvertStringLengthBytes + sizeof (CHAR16));
  Status = StrHexToBytes (
             ConfigStringNameStart,
             ConfigStringNameLengthChars,
             (UINT8 *)VariableString,
             ConvertStringLengthBytes
             );
  ASSERT_EFI_ERROR (Status);

  VariableString[VariableStringLengthChars] = 0;

  for (Index = 0; Index < VariableStringLengthChars; Index++) {
    VariableString[Index] = SwapBytes16 (VariableString[Index]);
  }

  return VariableString;
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
SetupMenuExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  CHAR16      *VariableName;
  VOID        *VariableOption;
  UINTN       DataSize;
  EFI_STATUS  Status;

  if ((Request == NULL) || (Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;

  // Get variable
  VariableName = ConvertHiiConfigStringToVariableString (Request);
  Status = GetVariable2 (
             VariableName,
             &gEficorebootNvDataGuid,
             &VariableOption,
             &DataSize
             );
  ASSERT_EFI_ERROR (Status);

  // Use HII helper to convert variable data to config
  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                Request,
                                VariableOption,
                                DataSize,
                                Results,
                                Progress
                                );
  ASSERT_EFI_ERROR (Status);

  FreePool (VariableName);
  if (VariableOption != NULL) {
    FreePool (VariableOption);
  }

  return Status;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
SetupMenuRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  CHAR16      *VariableName;
  VOID        *VariableOption;
  UINTN       DataSize;
  UINT32      Attributes;
  EFI_STATUS  Status;
  UINTN       TempDataSize;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  // Get variable
  VariableName = ConvertHiiConfigStringToVariableString (Configuration);
  Status = GetVariable3 (
             VariableName,
             &gEficorebootNvDataGuid,
             &VariableOption,
             &DataSize,
             &Attributes
             );
  ASSERT_EFI_ERROR (Status);

  // Use HII helper to convert updated config to variable data
  TempDataSize = DataSize;
  Status = gHiiConfigRouting->ConfigToBlock (
                                gHiiConfigRouting,
                                Configuration,
                                VariableOption,
                                &TempDataSize,
                                Progress
                                );
  ASSERT_EFI_ERROR (Status);

  // Set variable
  Status = gRT->SetVariable (
                  VariableName,
                  &gEficorebootNvDataGuid,
                  Attributes,
                  DataSize,
                  VariableOption
                  );
  if (Status == EFI_WRITE_PROTECTED) {
    Status = EFI_SUCCESS;
  }

  ASSERT_EFI_ERROR (Status);

  FreePool (VariableName);
  if (VariableOption != NULL) {
    FreePool (VariableOption);
  }

  return Status;
}

/**
  This function is invoked if user selected a interactive opcode from Setup Menu
  Formset. If user set VBIOS, the new value is saved to EFI variable.

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
SetupMenuCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  return EFI_SUCCESS;
}
