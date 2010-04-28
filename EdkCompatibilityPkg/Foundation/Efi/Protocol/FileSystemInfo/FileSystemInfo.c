/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FileSystemInfo.c

Abstract:

  FileSystemInfo protocol as defined in the EFI 1.0 specification.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (FileSystemInfo)

EFI_GUID  gEfiFileSystemInfoGuid = EFI_FILE_SYSTEM_INFO_ID_GUID;

EFI_GUID_STRING(&gEfiFileSystemInfoGuid, "File System Info ID", "EFI File System Info ID GUID");
