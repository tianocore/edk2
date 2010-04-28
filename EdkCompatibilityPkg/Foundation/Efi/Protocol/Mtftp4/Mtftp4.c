/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  Mtftp4.c

Abstract:

  UEFI Multicast Trivial File Transfer Protocol GUID Declaration.
  
--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (Mtftp4)

EFI_GUID gEfiMtftp4ServiceBindingProtocolGuid  = EFI_MTFTP4_SERVICE_BINDING_PROTOCOL_GUID;
EFI_GUID gEfiMtftp4ProtocolGuid                = EFI_MTFTP4_PROTOCOL_GUID;

EFI_GUID_STRING (
  &gEfiMtftp4ServiceBindingProtocolGuid, 
  "MTFTP4 Service Binding Protocol", 
  "MTFTP4 Service Binding Protocol"
  );
    
EFI_GUID_STRING (
  &gEfiMtftp4ProtocolGuid,               
  "MTFTP4 Protocol",                 
  "MTFTP4 Protocol"
  );

