/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    Processor.c

Abstract:

--*/

#include "Tiano.h"
#include "EfiJump.h"
#include "PeiHob.h"
#include EFI_GUID_DEFINITION (PeiFlushInstructionCache)
#include EFI_GUID_DEFINITION (PeiTransferControl)

EFI_STATUS
WinNtFlushInstructionCacheFlush (
  IN EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL   *This,
  IN EFI_PHYSICAL_ADDRESS                       Start,
  IN UINT64                                     Length
  );

EFI_PEI_TRANSFER_CONTROL_PROTOCOL         mTransferControl = {
  (EFI_PEI_TRANSFER_CONTROL_SET_JUMP)  SetJump,
  (EFI_PEI_TRANSFER_CONTROL_LONG_JUMP) LongJump,
  sizeof (EFI_JUMP_BUFFER)
};

EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL  mFlushInstructionCache = {
  WinNtFlushInstructionCacheFlush
};

EFI_STATUS
InstallEfiPeiTransferControl (
  IN OUT EFI_PEI_TRANSFER_CONTROL_PROTOCOL **This
  )
/*++

Routine Description:

  Installs the pointer to the transfer control mechanism

Arguments:

  This       - Pointer to transfer control mechanism.

Returns:

  EFI_SUCCESS     - Successfully installed.

--*/
{
  *This = &mTransferControl;
  return EFI_SUCCESS;
}

EFI_STATUS
InstallEfiPeiFlushInstructionCache (
  IN OUT EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL  **This
  )
/*++

Routine Description:

  Installs the pointer to the flush instruction cache mechanism

Arguments:

  This       - Pointer to flush instruction cache mechanism.

Returns:

  EFI_SUCCESS     - Successfully installed

--*/
{
  *This = &mFlushInstructionCache;
  return EFI_SUCCESS;
}

EFI_STATUS
WinNtFlushInstructionCacheFlush (
  IN EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL   *This,
  IN EFI_PHYSICAL_ADDRESS                       Start,
  IN UINT64                                     Length
  )
/*++

Routine Description:

  This routine would provide support for flushing the CPU instruction cache.

Arguments:

  This      - Pointer to CPU Architectural Protocol interface
  Start     - Start adddress in memory to flush
  Length    - Length of memory to flush

Returns:

  Status
    EFI_SUCCESS

--*/
{
  RtPioICacheFlush ((UINT8 *) Start, (UINTN) Length);
  return EFI_SUCCESS;
}
