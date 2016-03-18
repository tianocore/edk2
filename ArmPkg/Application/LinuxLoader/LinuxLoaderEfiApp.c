/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "LinuxLoader.h"

/**
  Extract the next item from the command line.

  The items are separated by spaces. Quotation marks (") are used for argument
  grouping and the escaping character is "^" as for the EFI Shell command lines.

  @param[in out]  CommandLine  Command line pointer.
  @param[out]     Item         Pointer to the allocated buffer where the
                               item is stored.

  @retval  EFI_SUCCESS           The token was found and extracted.
  @retval  EFI_NOT_FOUND         No item found.
  @retval  EFI_OUT_OF_RESOURCES  The memory allocation failed.

**/
STATIC
EFI_STATUS
ExtractNextItem (
  IN OUT CONST CHAR16  **CommandLine,
  OUT CHAR16           **Item
  )
{
  CONST CHAR16  *Walker;
  VOID          *Buffer;
  CHAR16        *WritePtr;
  BOOLEAN       InQuotedString;
  BOOLEAN       Interpret;

  for (Walker = *CommandLine; *Walker == L' '; Walker++) {
    ;
  }

  Buffer = AllocatePool (StrSize (Walker));
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (WritePtr = Buffer, Interpret = TRUE, InQuotedString = FALSE;
       ((*Walker != L' ') || InQuotedString) && (*Walker != L'\0');
       Walker++
       ) {
    if (Interpret) {
      if (*Walker == L'^') {
        Interpret = FALSE;
        continue;
      }
      if (*Walker == L'"') {
        InQuotedString = !InQuotedString;
        continue;
      }
    } else {
      Interpret = TRUE;
    }
    *(WritePtr++) = *Walker;
  }

  if (WritePtr == Buffer) {
    FreePool (Buffer);
    return EFI_NOT_FOUND;
  }

  *WritePtr = L'\0';
  *CommandLine = Walker;
  *Item        = Buffer;

  return EFI_SUCCESS;
}

/**
  Check if an item of the command line is a flag or not.

  @param[in]  Item  Command line item.

  @retval  TRUE   The item is a flag.
  @retval  FALSE  The item is not a flag.

**/
STATIC
BOOLEAN
IsFlag (
  IN CONST CHAR16 *Item
  )
{
  return ((Item[0] == L'-') && (Item[2] == L'\0'));
}

/**
  Process the application command line.

  @param[out]  KernelTextDevicePath  A pointer to the buffer where the device
                                     path to the Linux kernel is stored. The
                                     address of the buffer is NULL in case of
                                     an error. Otherwise, the returned address
                                     is the address of a buffer allocated with
                                     a call to AllocatePool() that has to be
                                     freed by the caller.
  @param[out]  FdtTextDevicePath     A pointer to the buffer where the device
                                     path to the FDT is stored. The address of
                                     the buffer is NULL in case of an error or
                                     if the device path to the FDT is not
                                     defined. Otherwise, the returned address
                                     is the address of a buffer allocated with
                                     a call to AllocatePool() that has to be
                                     freed by the caller.
  @param[out]  InitrdTextDevicePath  A pointer to the buffer where the device
                                     path to the RAM root file system is stored.
                                     The address of the buffer is NULL in case
                                     of an error or if the device path to the
                                     RAM root file system is not defined.
                                     Otherwise, the returned address is the
                                     address of a buffer allocated with a call
                                     to AllocatePool() that has to be freed by
                                     the caller.
  @param[out]  LinuxCommandLine      A pointer to the buffer where the Linux
                                     kernel command line is stored. The address
                                     of the buffer is NULL in case of an error
                                     or if the Linux command line is not
                                     defined. Otherwise, the returned address
                                     is the address of a buffer allocated with
                                     a call to AllocatePool() that has to be
                                     freed by the caller.

  @param[out]  AtagMachineType       Value of the ARM Machine Type

  @retval  EFI_SUCCESS            The processing was successfull.
  @retval  EFI_NOT_FOUND          EFI_LOADED_IMAGE_PROTOCOL not found.
  @retval  EFI_NOT_FOUND          Path to the Linux kernel not found.
  @retval  EFI_INVALID_PARAMETER  At least one parameter is not valid or there is a
                                  conflict between two parameters.
  @retval  EFI_OUT_OF_RESOURCES   A memory allocation failed.

**/
EFI_STATUS
ProcessAppCommandLine (
  OUT  CHAR16   **KernelTextDevicePath,
  OUT  CHAR16   **FdtTextDevicePath,
  OUT  CHAR16   **InitrdTextDevicePath,
  OUT  CHAR16   **LinuxCommandLine,
  OUT  UINTN    *AtagMachineType
  )
{
  EFI_STATUS                 Status;
  EFI_STATUS                 Status2;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  CONST CHAR16               *Walker;
  CHAR16                     *Item;
  CHAR16                     Flag;
  BOOLEAN                    HasAtagSupport;
  BOOLEAN                    HasFdtSupport;

  *KernelTextDevicePath = NULL;
  *FdtTextDevicePath    = NULL;
  *InitrdTextDevicePath = NULL;
  *LinuxCommandLine     = NULL;
  *AtagMachineType      = ARM_FDT_MACHINE_TYPE;

  HasAtagSupport        = FALSE;
  HasFdtSupport         = FALSE;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID**)&LoadedImage
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Walker = (CHAR16*)LoadedImage->LoadOptions;
  if (Walker == NULL) {
    PrintHelp (NULL);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the device path to the Linux kernel.
  //

  Status = ExtractNextItem (&Walker, &Item);
  if (!EFI_ERROR (Status)) {
    if (!IsFlag (Item)) {
      *KernelTextDevicePath = Item;
    } else {
      PrintHii (NULL, STRING_TOKEN (STR_MISSING_KERNEL_PATH));
      FreePool (Item);
      return EFI_NOT_FOUND;
    }
  } else {
    if (Status != EFI_NOT_FOUND) {
      return Status;
    }
    PrintHelp (NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_INVALID_PARAMETER;
  while (*Walker != L'\0') {
    Status2 = ExtractNextItem (&Walker, &Item);
    if (Status2 == EFI_NOT_FOUND) {
      break;
    }
    if (EFI_ERROR (Status2)) {
      Status = Status2;
      goto Error;
    }

    if (!IsFlag (Item)) {
      PrintHii (NULL, STRING_TOKEN (STR_INVALID_FLAG), Item[0], Item[1]);
      FreePool (Item);
      goto Error;
    }
    Flag = Item[1];
    FreePool (Item);

    Status2 = ExtractNextItem (&Walker, &Item);
    if (Status2 == EFI_NOT_FOUND) {
      PrintHii (NULL, STRING_TOKEN (STR_MISSING_VALUE), Flag);
      goto Error;
    }
    if (EFI_ERROR (Status2)) {
      Status = Status2;
      goto Error;
    }
    if (IsFlag (Item)) {
      PrintHii (NULL, STRING_TOKEN (STR_MISSING_VALUE), Flag);
      FreePool (Item);
      goto Error;
    }

    switch (Flag) {
    case  L'a':
      if (HasFdtSupport) {
        PrintHii (NULL, STRING_TOKEN (STR_ATAG_FDT_CONFLICT));
        goto Error;
      }
      *AtagMachineType = StrDecimalToUintn (Item);
      HasAtagSupport = TRUE;
      break;
    case L'd':
      *FdtTextDevicePath = Item;
      if (HasAtagSupport) {
        PrintHii (NULL, STRING_TOKEN (STR_ATAG_FDT_CONFLICT));
        goto Error;
      }
      HasFdtSupport = TRUE;
      break;

    case L'c':
      *LinuxCommandLine = Item;
      break;

    case L'f':
      *InitrdTextDevicePath = Item;
      break;

    default:
      PrintHii (NULL, STRING_TOKEN (STR_INVALID_FLAG), L'-', Flag);
      FreePool (Item);
      goto Error;
    }
  }

  Status = EFI_SUCCESS;

Error:
  if (EFI_ERROR (Status)) {
    if (*KernelTextDevicePath != NULL) {
      FreePool (*KernelTextDevicePath);
      *KernelTextDevicePath = NULL;
    }
    if (*FdtTextDevicePath != NULL) {
      FreePool (*FdtTextDevicePath);
      *FdtTextDevicePath = NULL;
    }
    if (*InitrdTextDevicePath != NULL) {
      FreePool (*InitrdTextDevicePath);
      *InitrdTextDevicePath = NULL;
    }
    if (*LinuxCommandLine != NULL) {
      FreePool (*LinuxCommandLine);
      *LinuxCommandLine = NULL;
    }
  }

  return Status;
}
