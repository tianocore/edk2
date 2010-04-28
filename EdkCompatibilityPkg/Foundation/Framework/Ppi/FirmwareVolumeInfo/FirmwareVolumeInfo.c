/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FirmwareVolumeInfo.c

Abstract:

  PI 1.0 spec definition.

--*/


#include "Tiano.h"
#include "Pei.h"
#include EFI_PPI_DEFINITION (FirmwareVolumeInfo)

EFI_GUID gEfiFirmwareVolumeInfoPpiGuid = EFI_PEI_FIRMWARE_VOLUME_INFO_PPI_GUID;
EFI_GUID_STRING(&gEfiFirmwareVolumeInfoPpiGuid, "FirmwareVolumeInfo", "FirmwareVolumeInfo PPI");
