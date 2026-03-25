/** @file
  Provides image name lookup for SEC/PEI phase exception handling.

  Uses PeCoffSearchImageBase() to locate the faulting image, then
  validates the PE/COFF header before calling PeCoffLoaderGetPdbPointer()
  to extract the image name. Validation prevents crashes from false
  positives where random code/data accidentally matches the 2-byte
  PE/COFF signature check in PeCoffSearchImageBase().

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved. <BR>

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
  Validate that the address returned by PeCoffSearchImageBase() is
  actually a valid PE/COFF or TE image header.

  PeCoffSearchImageBase() only checks 2 bytes at each page boundary,
  which can produce false positives when code or data accidentally
  contains 'MZ', 'VZ', or 'PE' at a page boundary. This function
  performs deeper validation to reject false positives.

  @param[in]  Base  Candidate image base address from PeCoffSearchImageBase().

  @retval TRUE   The address appears to be a valid PE/COFF or TE image.
  @retval FALSE  The address is a false positive.
**/
STATIC BOOLEAN
IsValidPeCoffImage (
  IN UINTN  Base
  )
{
  EFI_IMAGE_DOS_HEADER  *DosHdr;
  UINT32                PeOffset;

  if (Base == 0) {
    return FALSE;
  }

  DosHdr = (EFI_IMAGE_DOS_HEADER *)Base;

  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS header found. Validate e_lfanew:
    //   - Must be beyond the DOS header itself
    //   - Must be within a reasonable range (< 4KB) to avoid
    //     reading from a wildly wrong address
    //
    PeOffset = DosHdr->e_lfanew;
    if ((PeOffset < sizeof (EFI_IMAGE_DOS_HEADER)) || (PeOffset > 0x1000)) {
      return FALSE;
    }

    //
    // Verify the PE signature ('PE\0\0') at the computed offset.
    // This eliminates most false positives.
    //
    if (*(UINT32 *)(Base + PeOffset) != EFI_IMAGE_NT_SIGNATURE) {
      return FALSE;
    }

    return TRUE;
  }

  //
  // TE header: only 2-byte signature, but TE images are rare and
  // the full header is small — accept as-is.
  //
  if (*(UINT16 *)Base == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    return TRUE;
  }

  return FALSE;
}

/**
  Return the image name for the given fault address.

  Locates the PE/COFF image base by scanning backwards from FaultAddress,
  validates the found header to reject false positives, then reads the
  PDB path from the image's debug directory to obtain the image name.
  All operations work in SEC/PEI phase without UEFI Boot Services.

  @param[in]  FaultAddress          The address that caused the fault.
  @param[out] ImageBase             On return, the base address of the PE/COFF
                                    image containing FaultAddress, or 0 if not
                                    found or validation failed.
  @param[out] PeCoffSizeOfHeaders   On return, the size of the PE/COFF headers.
                                    Set to 0 in SEC/PEI phase.

  @retval NULL   The image base could not be determined or validation failed.
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

  //
  // Validate the PE/COFF header before calling PeCoffLoaderGetPdbPointer().
  // PeCoffSearchImageBase() only checks 2 bytes and can return false
  // positives. Processing an invalid header in PeCoffLoaderGetPdbPointer()
  // can cause a recursive exception.
  //
  if (!IsValidPeCoffImage (*ImageBase)) {
    *ImageBase = 0;
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
  console output is not supported. It performs no operations and returns
  immediately.

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
