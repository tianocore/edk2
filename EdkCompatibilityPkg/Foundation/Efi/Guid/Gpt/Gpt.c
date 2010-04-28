/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    Gpt.c
    
Abstract:

  Guids used for the GPT as defined in EFI 1.0

  GPT defines a new disk partitioning scheme and also describes 
  usage of the legacy Master Boot Record (MBR) partitioning scheme. 

--*/

#include "EfiSpec.h"
#include EFI_GUID_DEFINITION (Gpt)

EFI_GUID  gEfiPartTypeUnusedGuid = EFI_PART_TYPE_UNUSED_GUID;

EFI_GUID_STRING(&gEfiPartTypeUnusedGuid, "G0", "Null Partition Type GUID");

EFI_GUID  gEfiPartTypeSystemPartGuid = EFI_PART_TYPE_EFI_SYSTEM_PART_GUID;

EFI_GUID_STRING(&gEfiPartTypeSystemPartGuid, "ESP", "EFI System Partition GUID");

EFI_GUID  gEfiPartTypeLegacyMbrGuid = EFI_PART_TYPE_LEGACY_MBR_GUID;

EFI_GUID_STRING(&gEfiPartTypeLegacyMbrGuid, "Legacy MBR", "Legacy Master Boot Record Partition GUID");
