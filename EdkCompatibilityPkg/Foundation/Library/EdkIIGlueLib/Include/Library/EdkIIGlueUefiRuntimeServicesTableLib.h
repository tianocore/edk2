/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGlueUefiRuntimeServicesTableLib.h
  
Abstract: 

  Library that provides a global pointer to the UEFI Runtime Services Tables

--*/

#ifndef __EDKII_GLUE_UEFI_RUNTIME_SERVICES_TABLE_LIB_H__
#define __EDKII_GLUE_UEFI_RUNTIME_SERVICES_TABLE_LIB_H__

//
// To avoid symbol collision with gRT in EfiDriverLib
//
#define gRT                  gGlueRT

//
// Cached copy of the EFI Runtime Services Table
//
extern EFI_RUNTIME_SERVICES  *gRT;

#endif
