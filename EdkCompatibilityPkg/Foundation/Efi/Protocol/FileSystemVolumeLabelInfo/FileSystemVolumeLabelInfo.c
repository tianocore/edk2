/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FileSystemVolumeLabelInfo.c

Abstract:

  SimpleFileSystem protocol as defined in the EFI 1.0 specification.

  The SimpleFileSystem protocol is the programatic access to the FAT (12,16,32) 
  file system specified in EFI 1.0. It can also be used to abstract any 
  file system other than FAT.

  EFI 1.0 can boot from any valid EFI image contained in a SimpleFileSystem
 
--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (FileSystemVolumeLabelInfo)

EFI_GUID  gEfiFileSystemVolumeLabelInfoIdGuid = EFI_FILE_SYSTEM_VOLUME_LABEL_INFO_ID_GUID;

EFI_GUID_STRING
  (&gEfiFileSystemVolumeLabelInfoIdGuid, "File System Vol Label ID", "EFI File System Volume Label Info ID GUID");
