/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DataHub.c

Abstract:

  The logging hub protocol is used both by agents wishing to log
  errors and those wishing to be made aware of all information that
  has been logged.

  For more information please look at Intel Platform Innovation 
  Framework for EFI Data Hub Specification.

--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (DataHub)

EFI_GUID  gEfiDataHubProtocolGuid = EFI_DATA_HUB_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiDataHubProtocolGuid, "DataHub Protocol", "EFI Data Hub Protocol");
