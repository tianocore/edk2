/** @file
  Declaration of internal functions in PE/COFF Lib.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  BasePeCoffLibInternals.h

**/

#ifndef __PECOFF_LOADER_LIB_INTERNALS__
#define __PECOFF_LOADER_LIB_INTERNALS__

EFI_STATUS
EFIAPI
TianoPeCoffLoaderLibGetImageInfo (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

EFI_STATUS
EFIAPI
TianoPeCoffLoaderLibLoadImage (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

EFI_STATUS
EFIAPI
TianoPeCoffLoaderLibRelocateImage (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

EFI_STATUS
EFIAPI
TianoPeCoffLoaderLibUnloadimage (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL      *This,
  IN PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

#endif
