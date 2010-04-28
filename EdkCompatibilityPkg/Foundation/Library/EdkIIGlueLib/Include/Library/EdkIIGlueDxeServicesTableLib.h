/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  
	

Module Name:

  EdkIIGlueDxeServicesTableLib.h
  
Abstract: 

  Library that provides a global pointer to the DXE Services Table

--*/

#ifndef __EDKII_GLUE_DXE_SERVICES_TABLE_LIB_H__
#define __EDKII_GLUE_DXE_SERVICES_TABLE_LIB_H__

//
// Cache copy of the DXE Services Table
//
extern EFI_DXE_SERVICES  *gDS;

#endif

