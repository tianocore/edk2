/** @file
  Include file for EFI_DEBUG_INFO

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

**/

#ifndef _MAX_BBS_ENTRIES_H
#define _MAX_BBS_ENTRIES_H

#pragma pack(1)

typedef struct {
  UINT32  ErrorLevel;
  //
  // 12 * sizeof (UINT64) Var Arg stack
  //
  // ascii DEBUG () Format string
  //
} EFI_DEBUG_INFO;

#pragma pack()


#endif
