/*++

Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
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
#include EFI_GUID_DEFINITION (PeiFlushInstructionCache)
#include EFI_GUID_DEFINITION (PeiTransferControl)

//
// Prototypes
//
EFI_STATUS
EFIAPI
TransferControlSetJump (
  IN EFI_PEI_TRANSFER_CONTROL_PROTOCOL  *This,
  IN VOID			                    *Jump
  );

EFI_STATUS
EFIAPI
TransferControlLongJump (
  IN EFI_PEI_TRANSFER_CONTROL_PROTOCOL  *This,
  IN VOID			                    *Jump
  );

EFI_STATUS
EFIAPI
FlushInstructionCacheFlush (
  IN EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL *This,
  IN EFI_PHYSICAL_ADDRESS                     Start,
  IN UINT64                                   Length
  );

//
// Table declarations
//
EFI_PEI_TRANSFER_CONTROL_PROTOCOL mTransferControl = {
  TransferControlSetJump,
  TransferControlLongJump,
  sizeof (EFI_JUMP_BUFFER)
};

EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL  mFlushInstructionCache = {
  FlushInstructionCacheFlush
};


EFI_STATUS
EFIAPI
InstallEfiPeiTransferControl(
  IN OUT EFI_PEI_TRANSFER_CONTROL_PROTOCOL **This
  )
/*++

Routine Description:

  Installs the pointer to the transfer control mechanism

Arguments:

  This       - Pointer to transfer control mechanism.

Returns:

  This       - Pointer to transfer control mechanism.

--*/
{
  *This = &mTransferControl;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
InstallEfiPeiFlushInstructionCache (
  IN OUT EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL  **This
  )
/*++

Routine Description:

  Installs the pointer to the flush instruction cache mechanism

Arguments:

  This       - Pointer to flush instruction cache mechanism.

Returns:

  This       - Pointer to flush instruction cache mechanism.

--*/
{
  *This = &mFlushInstructionCache;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
FlushInstructionCacheFlush (
  IN EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL   *This,
  IN EFI_PHYSICAL_ADDRESS                       Start,
  IN UINT64                                     Length
  )
/*++

Routine Description:

  This routine would provide support for flushing the CPU instruction cache.
  In the case of IA32, this flushing is not necessary and is thus not implemented.

Arguments:

  Pointer to CPU Architectural Protocol interface
  Start adddress in memory to flush
  Length of memory to flush

Returns:

  Status
    EFI_SUCCESS

--*/
{
  return EFI_SUCCESS;
}

