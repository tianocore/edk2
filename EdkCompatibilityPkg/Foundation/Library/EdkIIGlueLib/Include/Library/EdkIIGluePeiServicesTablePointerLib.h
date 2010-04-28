/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGluePeiServicesTablePointerLib.h
  
Abstract: 

  Public header file for PEI Services Table Pointer Lib

--*/

#ifndef __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_H__
#define __EDKII_GLUE_PEI_SERVICES_TABLE_POINTER_LIB_H__

/**
  The function returns the pointer to PEI services.

  The function returns the pointer to PEI services.
  It will ASSERT() if the pointer to PEI services is NULL.

  @retval  The pointer to PeiServices.

**/
EFI_PEI_SERVICES **
EFIAPI
GetPeiServicesTablePointer (
  VOID
  );

#endif

