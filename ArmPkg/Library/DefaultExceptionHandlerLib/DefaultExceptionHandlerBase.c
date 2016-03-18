/** @file

  Copyright (c) 2012, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

/**

  @param  FaultAddress         Address to find PE/COFF image for.
  @param  ImageBase            Return load address of found image
  @param  PeCoffSizeOfHeaders  Return the size of the PE/COFF header for the image that was found

  @retval NULL                 FaultAddress not in a loaded PE/COFF image.
  @retval                      Path and file name of PE/COFF image.

**/
CHAR8 *
GetImageName (
  IN  UINTN  FaultAddress,
  OUT UINTN  *ImageBase,
  OUT UINTN  *PeCoffSizeOfHeaders
  )
{
  return NULL;
}
