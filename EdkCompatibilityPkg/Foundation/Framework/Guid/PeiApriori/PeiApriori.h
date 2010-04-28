/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeiApriori.h

Abstract:

   PI 1.0 spec definition.

--*/

#ifndef __PEI_APRIORI_GUID_H__
#define __PEI_APRIORI_GUID_H__

#include "Tiano.h"

//
// GUIDs defined by the FFS specification.
//
#define EFI_PEI_APRIORI_FILE_NAME_GUID \
  { 0x1b45cc0a, 0x156a, 0x428a, {0xaf, 0x62, 0x49, 0x86, 0x4d, 0xa0, 0xe6, 0xe6}}


extern EFI_GUID gEfiPeiAprioriGuid;

#endif
