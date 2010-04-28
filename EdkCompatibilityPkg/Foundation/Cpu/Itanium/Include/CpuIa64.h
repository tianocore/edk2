/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  CpuIA64.h

Abstract:

--*/

#ifndef _CPU_IA64_H
#define _CPU_IA64_H

#include "Tiano.h"

UINT64
EfiReadTsc (
  VOID
  )
/*++                                                                                                                          
Routine Description:                                                
  Read Time stamp
Arguments:                
  None                 
Returns:                                                            
   Return the read data                                                
--*/
;

#endif
