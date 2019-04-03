/** @file
  Provide FSP hob process related function.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FSP_HOB_PROCESS_LIB_H__
#define __FSP_HOB_PROCESS_LIB_H__

/**
  BIOS process FspBobList.

  @param[in] FspHobList  Pointer to the HOB data structure produced by FSP.

  @return If platform process the FSP hob list successfully.
**/
EFI_STATUS
EFIAPI
FspHobProcess (
  IN VOID                 *FspHobList
  );

/**
  BIOS process FspBobList for Memory Resource Descriptor.

  @param[in] FspHobList  Pointer to the HOB data structure produced by FSP.

  @return If platform process the FSP hob list successfully.
**/
EFI_STATUS
EFIAPI
FspHobProcessForMemoryResource (
  IN VOID                 *FspHobList
  );

/**
  BIOS process FspBobList for other data (not Memory Resource Descriptor).

  @param[in] FspHobList  Pointer to the HOB data structure produced by FSP.

  @return If platform process the FSP hob list successfully.
**/
EFI_STATUS
EFIAPI
FspHobProcessForOtherData (
  IN VOID                 *FspHobList
  );

#endif
