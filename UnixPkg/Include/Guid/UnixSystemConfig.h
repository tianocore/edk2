/**@file
  Setup Variable data structure for Unix platform.

Copyright (c) 2009, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


**/

#ifndef __UNIX_SYSTEM_CONFIGUE_H__
#define __UNIX_SYSTEM_CONFIGUE_H__

#define EFI_UXIX_SYSTEM_CONFIG_GUID  \
  {0x375ea976, 0x3ccd, 0x4e74, {0xa8, 0x45, 0x26, 0xb9, 0xb3, 0x24, 0xb1, 0x3c}}


#pragma pack(1)
typedef struct {
  //
  // Console output mode
  //
  UINT32        ConOutColumn;
  UINT32        ConOutRow;
} WIN_NT_SYSTEM_CONFIGURATION;
#pragma pack()


extern EFI_GUID   gEfiUnixSystemConfigGuid;

#endif
