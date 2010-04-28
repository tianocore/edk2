/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    Hob.h
    
Abstract:

  GUIDs used for HOB List entries in the in the EFI 1.0 system table

  These GUIDs point the HOB List passed from PEI to DXE.

--*/

#ifndef _HOB_GUID_H_
#define _HOB_GUID_H_

#define EFI_HOB_LIST_GUID \
  { \
    0x7739f24c, 0x93d7, 0x11d4, {0x9a, 0x3a, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d} \
  }

extern EFI_GUID gEfiHobListGuid;

#endif
