/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  
  

Module Name:

  EdkIIGlueHiiLib.h
  
Abstract: 

  Public header file for Hii Lib

--*/

#ifndef __EDKII_GLUE_HII_LIB_H__
#define __EDKII_GLUE_HII_LIB_H__

#if (EFI_SPECIFICATION_VERSION < 0x0002000A)

#define PreparePackages GluePreparePackages


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
EFI_HII_PACKAGES *
EFIAPI
GluePreparePackages (
  IN UINTN           NumberOfPackages,
  IN CONST EFI_GUID  *Guid OPTIONAL,
  ...
  );

#endif

#endif
