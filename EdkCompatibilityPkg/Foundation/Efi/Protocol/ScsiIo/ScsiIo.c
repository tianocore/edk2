/*++
  Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    ScsiIo.c
    
Abstract: 
  EFI_SCSI_IO_PROTOCOL as defined in UEFI 2.0.

Revision History

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (ScsiIo)

EFI_GUID gEfiScsiIoProtocolGuid = EFI_SCSI_IO_PROTOCOL_GUID;
EFI_GUID_STRING(&gEfiScsiIoProtocolGuid, "SCSI IO protoco", "UEFI 2.0 SCSI IO protocol");
