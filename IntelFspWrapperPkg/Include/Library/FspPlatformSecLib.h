/** @file
  Provide FSP wrapper platform sec related function.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FSP_PLATFORM_SEC_LIB_H__
#define __FSP_PLATFORM_SEC_LIB_H__

/**
  A developer supplied function to perform platform specific operations.

  It's a developer supplied function to perform any operations appropriate to a
  given platform. It's invoked just before passing control to PEI core by SEC
  core. Platform developer may modify the SecCoreData passed to PEI Core.
  It returns a platform specific PPI list that platform wishes to pass to PEI core.
  The Generic SEC core module will merge this list to join the final list passed to
  PEI core.

  @param[in,out] SecCoreData           The same parameter as passing to PEI core. It
                                       could be overridden by this function.

  @return The platform specific PPI list to be passed to PEI core or
          NULL if there is no need of such platform specific PPI list.

**/
EFI_PEI_PPI_DESCRIPTOR *
EFIAPI
SecPlatformMain (
  IN OUT   EFI_SEC_PEI_HAND_OFF        *SecCoreData
  );

/**
  Call PEI core entry point with new temporary RAM.

  @param[in] FspHobList   HobList produced by FSP.
  @param[in] StartOfRange Start of temporary RAM.
  @param[in] EndOfRange   End of temporary RAM.
**/
VOID
EFIAPI
CallPeiCoreEntryPoint (
  IN VOID                 *FspHobList,
  IN VOID                 *StartOfRange,
  IN VOID                 *EndOfRange
  );

/**
  Save SEC context before call FspInit.

  @param[in] PeiServices  Pointer to PEI Services Table.
**/
VOID
EFIAPI
SaveSecContext (
  IN CONST EFI_PEI_SERVICES                     **PeiServices
  );

#endif
