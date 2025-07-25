/** @file
  RISC-V backtrace helper functions for SEC.

  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Backtrace.h"

/**
  Use the EFI Debug Image Table to lookup the FaultAddress and find which PE/COFF image
  it came from. As long as the PE/COFF image contains a debug directory entry a
  string can be returned. For ELF and Mach-O images the string points to the Mach-O or ELF
  image. Microsoft tools contain a pointer to the PDB file that contains the debug information.

  @param  FaultAddress         Address to find PE/COFF image for.
  @param  ImageBase            Return load address of found image
  @param  PeCoffSizeOfHeaders  Return the size of the PE/COFF header for the image that was found

  @retval NULL                 FaultAddress not in a loaded PE/COFF image.
  @retval                      Path and file name of PE/COFF image.

**/
CHAR8 *
EFIAPI
GetImageName (
  IN  UINTN  FaultAddress,
  OUT UINTN  *ImageBase,
  OUT UINTN  *PeCoffSizeOfHeaders
  )
{
  //
  // This function is not implemented in SEC phase.
  // It should be implemented in DXE phase.
  //
  *ImageBase           = 0;
  *PeCoffSizeOfHeaders = 0;

  return NULL;
}
