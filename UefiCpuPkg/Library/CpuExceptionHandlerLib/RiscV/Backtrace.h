/** @file

  RISC-V backtrace definition file.

  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BACKTRACE_H_
#define BACKTRACE_H_

#include <PiPei.h>
#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/UefiLib.h>
#include <Guid/DebugImageInfoTable.h>
#include "ExceptionHandler.h"

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
  );

/**
  Display a backtrace.

  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
DumpCpuBacktrace (
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

#endif // BACKTRACE_H_
