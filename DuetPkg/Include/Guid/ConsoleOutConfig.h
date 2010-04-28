/**@file
  Setup Variable data structure for Duet platform.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.   

**/

#ifndef __DUET_CONSOLEOUT_CONFIG_H__
#define __DUET_CONSOLEOUT_CONFIG_H__

#define DUET_CONSOLEOUT_CONFIG_GUID  \
  { 0xED150714, 0xDF30, 0x407D, { 0xB2, 0x4A, 0x4B, 0x74, 0x2F, 0xD5, 0xCE, 0xA2 } }

#pragma pack(1)
typedef struct {
  //
  // Console output mode
  //
  UINT32        ConOutColumn;
  UINT32        ConOutRow;
} DUET_CONSOLEOUT_CONFIG;
#pragma pack()

extern EFI_GUID   gDuetConsoleOutConfigGuid;

#endif
