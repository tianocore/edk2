/** @file
  PEI Services Table Pointer Library services

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  PeiServicesTablePointerLib.h

**/

#ifndef __PEI_SERVICES_TABLE_POINTER_LIB_H__
#define __PEI_SERVICES_TABLE_POINTER_LIB_H__

/**
  The function returns the pointer to PEI services.
  
  The function returns the pointer to PEI services. 
  It will ASSERT() if the pointer to PEI services is NULL.

  @retval  The pointer to PeiServices.

**/
EFI_PEI_SERVICES **
GetPeiServicesTablePointer (
  VOID
  );

#endif

