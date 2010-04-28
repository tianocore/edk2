/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ScsiPassThru.c

Abstract:

  SCSI Pass Through protocol.
   
--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (ScsiPassThru)

EFI_GUID  gEfiScsiPassThruProtocolGuid = EFI_SCSI_PASS_THRU_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiScsiPassThruProtocolGuid, "SCSI Pass Through Protocol", "EFI 1.0 SCSI Pass Through protocol");
