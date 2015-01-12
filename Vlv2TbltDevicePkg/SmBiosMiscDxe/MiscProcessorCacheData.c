/*++

Copyright (c) 2012 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscBiosProcessorCache.c

Abstract:

  Processor cache static data.
  Misc. subclass type 7.
  SMBIOS type 7.

--*/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"


//
// Static (possibly build generated) Processor cache data.
//
MISC_SMBIOS_TABLE_DATA(EFI_CACHE_VARIABLE_RECORD, MiscProcessorCache) = {
0
};
