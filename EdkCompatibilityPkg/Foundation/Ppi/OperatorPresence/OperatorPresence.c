/*++

Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.            

Module Name:

  OperatorPresence.c

Abstract:

  PPI GUID definition for PEI_OPERATOR_PRESENCE_PPI

--*/

#include "Tiano.h"

#include EFI_PPI_DEFINITION (OperatorPresence)

EFI_GUID gPeiOperatorPresencePpiGuid = PEI_OPERATOR_PRESENCE_PPI_GUID;

EFI_GUID_STRING(&gPeiOperatorPresencePpiGuid, "OperatorPresence", "Operator Presence Detection");
