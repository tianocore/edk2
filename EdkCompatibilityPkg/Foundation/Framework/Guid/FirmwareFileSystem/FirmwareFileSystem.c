/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FirmwareFileSystem.c
    
Abstract:

  Tiano Guid used to define the Firmware File System.  See the EFI Firmware 
  File System Specification for more details.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (FirmwareFileSystem)

EFI_GUID  gEfiFirmwareFileSystemGuid    = EFI_FIRMWARE_FILE_SYSTEM_GUID;
EFI_GUID  gEfiFirmwareVolumeTopFileGuid = EFI_FFS_VOLUME_TOP_FILE_GUID;

EFI_GUID_STRING(&gEfiFirmwareFileSystemGuid, "Firmware File System GUID", "EFI Firmware File System GUID");
EFI_GUID_STRING(&gEfiFirmwareVolumeTopFileGuid, "Firmware Volume Top File GUID", "EFI FFS Volume Top File GUID");
