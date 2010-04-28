/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  CompatibleMemoryTested.h
    
Abstract:

  Tiano Guid used for all Compatible Memory Range Tested GUID.  

--*/

#ifndef _COMPATIBLE_MEMORY_TESTED_GUID_H_
#define _COMPATIBLE_MEMORY_TESTED_GUID_H_

#define EFI_COMPATIBLE_MEMORY_TESTED_PROTOCOL_GUID \
  { \
    0x64c475ef, 0x344b, 0x492c, {0x93, 0xad, 0xab, 0x9e, 0xb4, 0x39, 0x50, 0x4} \
  }

extern EFI_GUID gEfiCompatibleMemoryTestedGuid;

#endif
