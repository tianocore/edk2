/** @file
  Main file for Unload shell Driver1 function.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"

/**
  Function to translate the EFI_MEMORY_TYPE into a string.

  @param[in] Memory     The memory type.

  @retval               A string representation of the type allocated from BS Pool.
**/
CHAR16*
EFIAPI
ConvertMemoryType (
  IN CONST EFI_MEMORY_TYPE Memory
  )
{
  CHAR16 *RetVal;
  RetVal = NULL;

  switch (Memory) {
  case EfiReservedMemoryType:       StrnCatGrow(&RetVal, NULL, L"EfiReservedMemoryType", 0);        break;
  case EfiLoaderCode:               StrnCatGrow(&RetVal, NULL, L"EfiLoaderCode", 0);                break;
  case EfiLoaderData:               StrnCatGrow(&RetVal, NULL, L"EfiLoaderData", 0);                break;
  case EfiBootServicesCode:         StrnCatGrow(&RetVal, NULL, L"EfiBootServicesCode", 0);          break;
  case EfiBootServicesData:         StrnCatGrow(&RetVal, NULL, L"EfiBootServicesData", 0);          break;
  case EfiRuntimeServicesCode:      StrnCatGrow(&RetVal, NULL, L"EfiRuntimeServicesCode", 0);       break;
  case EfiRuntimeServicesData:      StrnCatGrow(&RetVal, NULL, L"EfiRuntimeServicesData", 0);       break;
  case EfiConventionalMemory:       StrnCatGrow(&RetVal, NULL, L"EfiConventionalMemory", 0);        break;
  case EfiUnusableMemory:           StrnCatGrow(&RetVal, NULL, L"EfiUnusableMemory", 0);            break;
  case EfiACPIReclaimMemory:        StrnCatGrow(&RetVal, NULL, L"EfiACPIReclaimMemory", 0);         break;
  case EfiACPIMemoryNVS:            StrnCatGrow(&RetVal, NULL, L"EfiACPIMemoryNVS", 0);             break;
  case EfiMemoryMappedIO:           StrnCatGrow(&RetVal, NULL, L"EfiMemoryMappedIO", 0);            break;
  case EfiMemoryMappedIOPortSpace:  StrnCatGrow(&RetVal, NULL, L"EfiMemoryMappedIOPortSpace", 0);   break;
  case EfiPalCode:                  StrnCatGrow(&RetVal, NULL, L"EfiPalCode", 0);                   break;
  case EfiMaxMemoryType:            StrnCatGrow(&RetVal, NULL, L"EfiMaxMemoryType", 0);             break;
  default: ASSERT(FALSE);
  }
  return (RetVal);
}

/**
  Function to dump LoadedImage info from TheHandle.

  @param[in] TheHandle              The handle to dump info from.

  @retval EFI_SUCCESS               The info was dumped.
  @retval EFI_INVALID_PARAMETER     The handle did not have LoadedImage
**/
EFI_STATUS
EFIAPI
DumpLoadedImageProtocolInfo (
  IN EFI_HANDLE   TheHandle
  )
{
  EFI_LOADED_IMAGE_PROTOCOL         *Image;
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL  *DevicePathToText;
  CHAR16                            *DevicePathText;
  CHAR16                            *CodeTypeText;
  CHAR16                            *DataTypeText;
  CHAR8                             *PdbPointer;

  Image = NULL;

  Status = gBS->OpenProtocol(TheHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&Image, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (EFI_ERROR(Status)) {
    return (EFI_INVALID_PARAMETER);
  }

  Status = gBS->LocateProtocol(
    &gEfiDevicePathToTextProtocolGuid,
    NULL,
    (VOID**)&DevicePathToText);
  //
  // we now have the device path to text protocol
  //
  if (!EFI_ERROR(Status)) {
    DevicePathText = DevicePathToText->ConvertDevicePathToText(Image->FilePath, TRUE, TRUE);
  } else {
    DevicePathText = NULL;
  }

  CodeTypeText = ConvertMemoryType(Image->ImageCodeType);
  DataTypeText = ConvertMemoryType(Image->ImageDataType);
  PdbPointer = (CHAR8*)PeCoffLoaderGetPdbPointer(Image->ImageBase);
  ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_UNLOAD_VERBOSE), gShellDriver1HiiHandle,
    ConvertHandleToHandleIndex(TheHandle),
    TheHandle,
    Image,
    Image->ParentHandle,
    Image->SystemTable,
    Image->DeviceHandle,
    DevicePathText,
    PdbPointer,
    Image->ImageBase,
    Image->ImageSize,
    CodeTypeText,
    DataTypeText
    );

  SHELL_FREE_NON_NULL(DevicePathText);
  SHELL_FREE_NON_NULL(CodeTypeText);
  SHELL_FREE_NON_NULL(DataTypeText);

  return (EFI_SUCCESS);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-n", TypeFlag},
  {L"-v", TypeFlag},
  {L"-verbose", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'unload' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunUnload (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  LIST_ENTRY            *Package;
  CHAR16                *ProblemParam;
  SHELL_STATUS          ShellStatus;
  EFI_HANDLE            TheHandle;
  CONST CHAR16          *Param1;
  SHELL_PROMPT_RESPONSE *Resp;
  UINT64                Value;

  ShellStatus         = SHELL_SUCCESS;
  Package             = NULL;
  Resp                = NULL;
  Value               = 0;
  TheHandle           = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 2){
      //
      // error for too many parameters
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount(Package) < 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDriver1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Param1    = ShellCommandLineGetRawValue(Package, 1);
      if (Param1 != NULL) {
        Status    = ShellConvertStringToUint64(Param1, &Value, TRUE, FALSE);
        TheHandle = ConvertHandleIndexToHandle((UINTN)Value);
      }

      if (EFI_ERROR(Status) || Param1 == NULL || TheHandle == NULL){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Param1);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        ASSERT(TheHandle != NULL);
        if (ShellCommandLineGetFlag(Package, L"-v") || ShellCommandLineGetFlag(Package, L"-verbose")) {
          DumpLoadedImageProtocolInfo(TheHandle);
        }
        
        if (!ShellCommandLineGetFlag(Package, L"-n")) {
          Status = ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_UNLOAD_CONF), gShellDriver1HiiHandle, (UINTN)TheHandle);
          Status = ShellPromptForResponse(ShellPromptResponseTypeYesNo, NULL, (VOID**)&Resp);
        }
        if (ShellCommandLineGetFlag(Package, L"-n") || (Resp != NULL && *Resp == ShellPromptResponseYes)) {
          Status = gBS->UnloadImage(TheHandle);
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HANDLE_RESULT), gShellDriver1HiiHandle, L"Unload", (UINTN)TheHandle, Status);
        }
        SHELL_FREE_NON_NULL(Resp);
      }
    }
  }
  if (ShellStatus == SHELL_SUCCESS) {
    if (Status == EFI_SECURITY_VIOLATION) {
      ShellStatus = SHELL_SECURITY_VIOLATION;
    } else if (Status == EFI_INVALID_PARAMETER) {
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (EFI_ERROR(Status)) {
      ShellStatus = SHELL_NOT_FOUND;
    }
  }

  if (Package != NULL) {
    ShellCommandLineFreeVarList(Package);
  }

  return (ShellStatus);
}
