/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.            

Module Name:

  OperatorPresence.h

Abstract:

  PPI published by the PEIM who's responsible for detecting operator presence

--*/

#ifndef _PEI_OPERATOR_PRESENCE_H_
#define _PEI_OPERATOR_PRESENCE_H_

#define PEI_OPERATOR_PRESENCE_PPI_GUID  \
  { 0x20a7378c, 0xaa83, 0x4ce1, {0x82, 0x1f, 0x47, 0x40, 0xee, 0x1b, 0x3f, 0x9f} }

typedef struct _PEI_OPERATOR_PRESENCE_PPI {
  BOOLEAN                           OperatorPresent;
} PEI_OPERATOR_PRESENCE_PPI;

extern EFI_GUID                     gPeiOperatorPresencePpiGuid;

#endif  //  _PEI_OPERATOR_PRESENCE_H_
