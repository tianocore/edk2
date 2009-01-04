/** @file
  Library class name: FrameworkHiiLib.
  
  FrameworkHiiLib is designed for produce interfaces to access
  framework HII things. It firstly also produce all interfaces define
  in HiiLib library class in MdePkg/Include/HiiLib.h. It also produce
  framework's old interface PreparePackages to keep compatiblity.
     
  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __FRAMEWORK_HII_LIB_H__
#define __FRAMEWORK_HII_LIB_H__

#include <FrameworkDxe.h>

#include <Protocol/FrameworkHii.h>
//
// FrameworkHiiLib will produce HiiLib library class too.
//
#include <Library/HiiLib.h>

/**
  This function allocates pool for an EFI_HII_PACKAGES structure
  with enough space for the variable argument list of package pointers.
  The allocated structure is initialized using NumberOfPackages, Guid,
  and the variable length argument list of package pointers.

  @param  NumberOfPackages  The number of HII packages to prepare.
  @param  Guid              Package GUID.
  @param  ...               The variable argument list of package pointers.

  @return                   The allocated and initialized packages.
**/
EFI_HII_PACKAGES *
EFIAPI
PreparePackages (
  IN UINTN           NumberOfPackages,
  IN CONST EFI_GUID  *Guid OPTIONAL,
  ...
  );

#endif
