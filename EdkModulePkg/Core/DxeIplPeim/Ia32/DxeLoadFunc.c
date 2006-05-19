/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DxeLoadFunc.c

Abstract:

  Ia32-specifc functionality for DxeLoad.

--*/

#include <DxeIpl.h>

EFI_STATUS
CreateArchSpecificHobs (
  OUT EFI_PHYSICAL_ADDRESS      *BspStore
  )
/*++

Routine Description:

  Creates architecture-specific HOBs.

  Note: New parameters should NOT be added for any HOBs that are added to this
        function.  BspStore is a special case because it is required for the
        call to SwitchStacks() in DxeLoad().

Arguments:

  BspStore    - The address of the BSP Store for those architectures that need
                it.  Otherwise 0.

Returns:

  EFI_SUCCESS   - The HOBs were created successfully.

--*/
{
  *BspStore = 0;
  return EFI_SUCCESS;
}
