/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*  
*  This program and the accompanying materials                          
*  are licensed and made available under the terms and conditions of the BSD License         
*  which accompanies this distribution.  The full text of the license may be found at        
*  http://opensource.org/licenses/bsd-license.php                                            
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
*
**/

#include <Library/ArmMPCoreMailBoxLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>

VOID ArmClearMPCoreMailbox() {
\s\sMmioWrite32(PcdGet32(PcdMPCoreMailboxClearAddress),PcdGet32(PcdMPCoreMailboxClearValue));
}

UINTN ArmGetMPCoreMailbox() {
    return MmioRead32(PcdGet32(PcdMPCoreMailboxGetAddress));
}
