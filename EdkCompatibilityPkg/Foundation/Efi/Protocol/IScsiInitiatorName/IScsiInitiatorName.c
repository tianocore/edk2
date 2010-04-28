/*++
  Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    IScsiInitatorName.c
    
Abstract: 
  EFI_ISCSI_INITIATOR_NAME_PROTOCOL as defined in UEFI 2.0.
  It rovides the ability to get and set the iSCSI Initiator Name.                                                  

Revision History

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (IScsiInitiatorName)

EFI_GUID gEfiIScsiInitiatorNameProtocolGuid = EFI_ISCSI_INITIATOR_NAME_PROTOCOL_GUID;
EFI_GUID_STRING(&gEfiIScsiInitiatorNameProtocolGuid, "ISCSI Initiator Name Protocol", "UEFI 2.0 ISCSI Initiator Name Protocol");
