/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiSpec.h

Abstract:

  EFI master include file.

  This is the main include file for EFI components. There should be
  no defines or macros added to this file, other than the EFI version 
  information already in this file.

  Don't add include files to the list for convenience, only add things
  that are architectural. Don't add Protocols or GUID include files here

--*/

#ifndef _EFI_SPEC_H_
#define _EFI_SPEC_H_

#include "EfiCommon.h"
#include "EfiApi.h"
#include "EfiDevicePath.h"


#endif
