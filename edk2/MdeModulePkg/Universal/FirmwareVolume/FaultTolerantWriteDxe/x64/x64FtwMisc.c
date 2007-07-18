
/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  x64FtwMisc.c
  
Abstract:
  
  X64 platform related code to support FtwLite..

Revision History

--*/


#include <FtwLite.h>

//
// MACROs for boot block update
//
#define BOOT_BLOCK_BASE

// STATIC
EFI_STATUS
GetSwapState (
  IN EFI_FTW_LITE_DEVICE    *FtwLiteDevice,
  OUT BOOLEAN               *SwapState
  )
/*++

Routine Description:

  Get swap state

Arguments:

  FtwLiteDevice - Calling context
  SwapState     - Swap state

Returns:

  EFI_SUCCESS - State successfully got

--*/
{
  return EFI_SUCCESS;
}

// STATIC
EFI_STATUS
SetSwapState (
  IN EFI_FTW_LITE_DEVICE    *FtwLiteDevice,
  IN  BOOLEAN               TopSwap
  )
/*++

Routine Description:
    Set swap state.

Arguments:
    FtwLiteDevice  - Indicates a pointer to the calling context.  
    TopSwap        - New swap state

Returns:
    EFI_SUCCESS   - The function completed successfully

Note:
    the Top-Swap bit (bit 13, D31: F0, Offset D4h). Note that
    software will not be able to clear the Top-Swap bit until the system is
    rebooted without GNT[A]# being pulled down.

--*/
{
  return EFI_SUCCESS;
}

BOOLEAN
IsBootBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  )
/*++

Routine Description:

  Check whether the block is a boot block.

Arguments:

  FtwLiteDevice - Calling context
  FvBlock       - Fvb protocol instance
  Lba           - Lba value

Returns:

  Is a boot block or not

--*/
{
  return FALSE;
}

EFI_STATUS
FlushSpareBlockToBootBlock (
  EFI_FTW_LITE_DEVICE      *FtwLiteDevice
  )
/*++

Routine Description:
    Copy the content of spare block to a boot block. Size is FTW_BLOCK_SIZE.
    Spare block is accessed by FTW backup FVB protocol interface. LBA is 
    FtwLiteDevice->FtwSpareLba.
    Boot block is accessed by BootFvb protocol interface. LBA is 0.

Arguments:
    FtwLiteDevice  - The private data of FTW_LITE driver

Returns:
    EFI_SUCCESS              - Spare block content is copied to boot block
    EFI_INVALID_PARAMETER    - Input parameter error
    EFI_OUT_OF_RESOURCES     - Allocate memory error
    EFI_ABORTED              - The function could not complete successfully

Notes:

--*/
{
  return EFI_SUCCESS;
}
