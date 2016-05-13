/** @file
  Provide FSP wrapper hob process related function.

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FSP_WRAPPER_HOB_PROCESS_LIB_H__
#define __FSP_WRAPPER_HOB_PROCESS_LIB_H__

/**
  Post FSP-M HOB process for Memory Resource Descriptor.

  @param[in] FspHobList  Pointer to the HOB data structure produced by FSP.

  @return If platform process the FSP hob list successfully.
**/
EFI_STATUS
EFIAPI
PostFspmHobProcess (
  IN VOID                 *FspHobList
  );

/**
  Post FSP-S HOB process (not Memory Resource Descriptor).

  @param[in] FspHobList  Pointer to the HOB data structure produced by FSP.

  @return If platform process the FSP hob list successfully.
**/
EFI_STATUS
EFIAPI
PostFspsHobProcess (
  IN VOID                 *FspHobList
  );

#endif
