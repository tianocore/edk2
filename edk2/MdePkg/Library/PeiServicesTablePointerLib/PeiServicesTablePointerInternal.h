/** @file
  Internal Header file for PEI Services Table Pointer Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_SERVICE_TABLE_POINTER_INTERNAL_H_
#define _PEI_SERVICE_TABLE_POINTER_INTERNAL_H_
/**
  The constructor function caches the pointer to PEI services.
  
  The constructor function caches the pointer to PEI services.
  It will always return EFI_SUCCESS.

  @param  FfsHeader   Pointer to FFS header the loaded driver.
  @param  PeiServices Pointer to the PEI services.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PeiServicesTablePointerLibConstructor (
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  IN EFI_PEI_SERVICES     **PeiServices
  )
;
#endif
