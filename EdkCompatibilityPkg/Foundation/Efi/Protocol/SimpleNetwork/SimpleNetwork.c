/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SimpleNetwork.c

Abstract:

  Simple Network protocol as defined in the EFI 1.0 specification.

  Basic network device abstraction.

  Rx    - Received
  Tx    - Transmit
  MCast - MultiCast
  ...

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (SimpleNetwork)

EFI_GUID  gEfiSimpleNetworkProtocolGuid = EFI_SIMPLE_NETWORK_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiSimpleNetworkProtocolGuid, "Simple Network Protocol", "EFI 1.0 Simple Network Protocol");
