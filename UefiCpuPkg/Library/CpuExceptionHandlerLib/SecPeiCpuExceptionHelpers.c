/** @file
  Helper functions for SEC/PEI exception handling.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/PeCoffGetEntryPointLib.h>

/**
  Extract the base file name from a full PDB path string.

  Scans the string for the last occurrence of '\\' or '/'
  and returns a pointer to the character immediately after it.

  @param[in]  Str   Full PDB path

  @return  Pointer to the base file name within Str.
**/
STATIC CHAR8 *
PdbBaseName (
  IN CHAR8  *Str
  )
{
  CHAR8  *LastSlash;

  LastSlash = Str;
  while (*Str != '\0') {
    if ((*Str == '\\') || (*Str == '/')) {
      LastSlash = Str + 1;
    }

    Str++;
  }

  return LastSlash;
}

/**
  Return the image name for the given fault address.

  @param[in]  FaultAddress          The address that caused the fault.
  @param[out] ImageBase             On return, the base address of the PE/COFF
                                    image containing FaultAddress, or 0 if not
                                    found or validation failed.
  @param[out] PeCoffSizeOfHeaders   Set to 0 in SEC/PEI phase.

  @retval NULL   The image base could not be determined.
  @retval Other  Pointer to a null-terminated ASCII string with the image
                 base name extracted from the PDB path.
**/
CHAR8 *
GetImageName (
  IN  UINTN  FaultAddress,
  OUT UINTN  *ImageBase,
  OUT UINTN  *PeCoffSizeOfHeaders
  )
{
  VOID  *PdbPointer;

  *PeCoffSizeOfHeaders = 0;
  *ImageBase           = PeCoffSearchImageBase (FaultAddress);

  if (*ImageBase == 0) {
    return NULL;
  }

  PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)*ImageBase);
  if (PdbPointer == NULL) {
    return NULL;
  }

  return PdbBaseName ((CHAR8 *)PdbPointer);
}

/**
  This function is a stub implementation for the SEC and PEI phases where
  console output is not supported.

  @param[in]  Buffer    Pointer to the null-terminated CHAR16 string to output.

**/
VOID
LogToConsole (
  IN CHAR16  *Buffer
  )
{
  // Console not supported here
  return;
}
