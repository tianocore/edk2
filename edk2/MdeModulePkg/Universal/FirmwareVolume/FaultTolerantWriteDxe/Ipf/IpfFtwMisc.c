/** @file

  Ipf platform related code to support FtwLite..

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

**/


#include "FtwLite.h"

//
// MACROs for boot block update
//
#define BOOT_BLOCK_BASE

/**

  Get swap state
  This is a internal function.

  @param FtwLiteDevice   Calling context
  @param SwapState       Swap state

  @retval  EFI_SUCCESS  State successfully got

**/
EFI_STATUS
GetSwapState (
  IN EFI_FTW_LITE_DEVICE    *FtwLiteDevice,
  OUT BOOLEAN               *SwapState
  )
{
  return EFI_SUCCESS;
}

/**
  Set swap state.
  This is a internal function.


  @param FtwLiteDevice   Indicates a pointer to the calling context.
  @param TopSwap         New swap state

  @retval  EFI_SUCCESS    The function completed successfully
                          Note:
                          the Top-Swap bit (bit 13, D31: F0, Offset D4h). Note that
                          software will not be able to clear the Top-Swap bit until the system is
                          rebooted without GNT[A]# being pulled down.

**/
EFI_STATUS
SetSwapState (
  IN EFI_FTW_LITE_DEVICE    *FtwLiteDevice,
  IN  BOOLEAN               TopSwap
  )
{
  return EFI_SUCCESS;
}

/**

  Check whether the block is a boot block.


  @param FtwLiteDevice   Calling context
  @param FvBlock         Fvb protocol instance
  @param Lba             Lba value

  @retval FALSE           This is a boot block.
  @retval TRUE            This is not a boot block.

**/
BOOLEAN
IsBootBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  )
{
  //
  // IPF doesn't support safe bootblock update
  // so treat bootblock as normal block
  //
  return FALSE;
}

/**
  Copy the content of spare block to a boot block. Size is FTW_BLOCK_SIZE.
  Spare block is accessed by FTW backup FVB protocol interface. LBA is
  FtwLiteDevice->FtwSpareLba.
  Boot block is accessed by BootFvb protocol interface. LBA is 0.


  @param FtwLiteDevice   The private data of FTW_LITE driver

  @retval  EFI_SUCCESS               Spare block content is copied to boot block
  @retval  EFI_INVALID_PARAMETER     Input parameter error
  @retval  EFI_OUT_OF_RESOURCES      Allocate memory error
  @retval  EFI_ABORTED               The function could not complete successfully
                                     Notes:

**/
EFI_STATUS
FlushSpareBlockToBootBlock (
  EFI_FTW_LITE_DEVICE      *FtwLiteDevice
  )
{
  return EFI_SUCCESS;
}
