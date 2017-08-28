/** @file
  EFI SMM Status Code Protocol as defined in the PI 1.2 specification.

  This protocol provides the basic status code services while in SMM. 

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _SMM_STATUS_CODE_H__
#define _SMM_STATUS_CODE_H__

#include <Protocol/MmStatusCode.h>

#define EFI_SMM_STATUS_CODE_PROTOCOL_GUID EFI_MM_STATUS_CODE_PROTOCOL_GUID

typedef EFI_MM_STATUS_CODE_PROTOCOL  EFI_SMM_STATUS_CODE_PROTOCOL;

typedef EFI_MM_REPORT_STATUS_CODE EFI_SMM_REPORT_STATUS_CODE;

extern EFI_GUID gEfiSmmStatusCodeProtocolGuid;

#endif

