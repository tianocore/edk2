/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StatusCodeCallerId.h
    
Abstract:

  GUID used to identify id for the caller who is initiating the Status Code.

--*/

#ifndef _STATUS_CODE_CALLER_ID_H__
#define _STATUS_CODE_CALLER_ID_H__

#include "EfiStatusCode.h"

#ifndef EFI_STANDARD_CALLER_ID_GUID

#define EFI_STANDARD_CALLER_ID_GUID \
  {0xC9DCF469, 0xA7C4, 0x11D5, {0x87, 0xDA, 0x00, 0x06, 0x29, 0x45, 0xC3, 0xB9}}

#endif

extern EFI_GUID gEfiCallerIdGuid;


#endif
