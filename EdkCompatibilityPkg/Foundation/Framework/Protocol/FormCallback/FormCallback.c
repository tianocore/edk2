/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FormCallback.c

Abstract:

  The EFI_FORM_CALLBACK_PROTOCOL is the defined interface for access to custom 
  NV storage devices as well as communication of user selections in a more 
  interactive environment.  This protocol should be published by hardware 
  specific drivers which want to export access to custom hardware storage or 
  publish IFR which has a requirement to call back the original driver.
 
--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (FormCallback)

EFI_GUID  gEfiFormCallbackProtocolGuid = EFI_FORM_CALLBACK_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiFormCallbackProtocolGuid, "Form Callback Protocol", "Form Callback 1.0 protocol");
