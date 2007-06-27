/** @file
  Public include file for the HII Library

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __HII_LIB_H__
#define __HII_LIB_H__

#include <Protocol/HiiDatabase.h>

/**
  This function allocates pool for an EFI_HII_PACKAGES structure
  with enough space for the variable argument list of package pointers.
  The allocated structure is initialized using NumberOfPackages, Guid, 
  and the variable length argument list of package pointers.

  @param  NumberOfPackages The number of HII packages to prepare.
  @param  Guid Package GUID.

  @return
  The allocated and initialized packages.

**/
EFI_HII_PACKAGE_LIST_HEADER *
EFIAPI
PreparePackages (
  IN CONST  UINTN     NumberOfPackages,
  IN CONST  EFI_GUID  *Guid OPTIONAL,
  ...
  );

#endif
