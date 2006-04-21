/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    PeHotRelocateEx.c

Abstract:

    Fixes IPF specific relocation types


Revision History

--*/

#include "Runtime.h"
#include "PeHotRelocateEx.h"

EFI_STATUS
PeHotRelocateImageEx (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  )
/*++

Routine Description:

  Performs an IPF specific relocation fixup

Arguments:

  Reloc      - Pointer to the relocation record

  Fixup      - Pointer to the address to fix up

  FixupData  - Pointer to a buffer to log the fixups

  Adjust     - The offset to adjust the fixup

Returns:

  None

--*/
{
  UINT64  *F64;
  UINT64  FixupVal;

  switch ((*Reloc) >> 12) {
  case EFI_IMAGE_REL_BASED_DIR64:
    F64         = (UINT64 *) Fixup;
    *FixupData  = ALIGN_POINTER (*FixupData, sizeof (UINT64));
    if (*(UINT64 *) (*FixupData) == *F64) {
      *F64 = *F64 + (UINT64) Adjust;
    }

    *FixupData = *FixupData + sizeof (UINT64);
    break;

  case EFI_IMAGE_REL_BASED_IA64_IMM64:
    F64         = (UINT64 *) Fixup;
    *FixupData  = ALIGN_POINTER (*FixupData, sizeof (UINT64));
    if (*(UINT64 *) (*FixupData) == *F64) {
      //
      // Align it to bundle address before fixing up the
      // 64-bit immediate value of the movl instruction.
      //
      //
      Fixup     = (CHAR8 *) ((UINT64) Fixup & (UINT64)~(15));
      FixupVal  = (UINT64) 0;

      //
      // Extract the lower 32 bits of IMM64 from bundle
      //
      EXT_IMM64 (
        FixupVal,
        (UINT32 *) Fixup + IMM64_IMM7B_INST_WORD_X,
        IMM64_IMM7B_SIZE_X,
        IMM64_IMM7B_INST_WORD_POS_X,
        IMM64_IMM7B_VAL_POS_X
        );

      EXT_IMM64 (
        FixupVal,
        (UINT32 *) Fixup + IMM64_IMM9D_INST_WORD_X,
        IMM64_IMM9D_SIZE_X,
        IMM64_IMM9D_INST_WORD_POS_X,
        IMM64_IMM9D_VAL_POS_X
        );

      EXT_IMM64 (
        FixupVal,
        (UINT32 *) Fixup + IMM64_IMM5C_INST_WORD_X,
        IMM64_IMM5C_SIZE_X,
        IMM64_IMM5C_INST_WORD_POS_X,
        IMM64_IMM5C_VAL_POS_X
        );

      EXT_IMM64 (
        FixupVal,
        (UINT32 *) Fixup + IMM64_IC_INST_WORD_X,
        IMM64_IC_SIZE_X,
        IMM64_IC_INST_WORD_POS_X,
        IMM64_IC_VAL_POS_X
        );

      EXT_IMM64 (
        FixupVal,
        (UINT32 *) Fixup + IMM64_IMM41a_INST_WORD_X,
        IMM64_IMM41a_SIZE_X,
        IMM64_IMM41a_INST_WORD_POS_X,
        IMM64_IMM41a_VAL_POS_X
        );

      //
      // Update 64-bit address
      //
      FixupVal += Adjust;

      //
      // Insert IMM64 into bundle
      //
      INS_IMM64 (
        FixupVal,
        ((UINT32 *) Fixup + IMM64_IMM7B_INST_WORD_X),
        IMM64_IMM7B_SIZE_X,
        IMM64_IMM7B_INST_WORD_POS_X,
        IMM64_IMM7B_VAL_POS_X
        );

      INS_IMM64 (
        FixupVal,
        ((UINT32 *) Fixup + IMM64_IMM9D_INST_WORD_X),
        IMM64_IMM9D_SIZE_X,
        IMM64_IMM9D_INST_WORD_POS_X,
        IMM64_IMM9D_VAL_POS_X
        );

      INS_IMM64 (
        FixupVal,
        ((UINT32 *) Fixup + IMM64_IMM5C_INST_WORD_X),
        IMM64_IMM5C_SIZE_X,
        IMM64_IMM5C_INST_WORD_POS_X,
        IMM64_IMM5C_VAL_POS_X
        );

      INS_IMM64 (
        FixupVal,
        ((UINT32 *) Fixup + IMM64_IC_INST_WORD_X),
        IMM64_IC_SIZE_X,
        IMM64_IC_INST_WORD_POS_X,
        IMM64_IC_VAL_POS_X
        );

      INS_IMM64 (
        FixupVal,
        ((UINT32 *) Fixup + IMM64_IMM41a_INST_WORD_X),
        IMM64_IMM41a_SIZE_X,
        IMM64_IMM41a_INST_WORD_POS_X,
        IMM64_IMM41a_VAL_POS_X
        );

      INS_IMM64 (
        FixupVal,
        ((UINT32 *) Fixup + IMM64_IMM41b_INST_WORD_X),
        IMM64_IMM41b_SIZE_X,
        IMM64_IMM41b_INST_WORD_POS_X,
        IMM64_IMM41b_VAL_POS_X
        );

      INS_IMM64 (
        FixupVal,
        ((UINT32 *) Fixup + IMM64_IMM41c_INST_WORD_X),
        IMM64_IMM41c_SIZE_X,
        IMM64_IMM41c_INST_WORD_POS_X,
        IMM64_IMM41c_VAL_POS_X
        );

      INS_IMM64 (
        FixupVal,
        ((UINT32 *) Fixup + IMM64_SIGN_INST_WORD_X),
        IMM64_SIGN_SIZE_X,
        IMM64_SIGN_INST_WORD_POS_X,
        IMM64_SIGN_VAL_POS_X
        );

      *(UINT64 *) (*FixupData) = *F64;
    }

    *FixupData = *FixupData + sizeof (UINT64);
    break;

  default:
    DEBUG ((EFI_D_ERROR, "PeHotRelocateEx:unknown fixed type\n"));
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}


//
// Cache Flush Routine.
//
EFI_STATUS
FlushCpuCache (
  IN EFI_PHYSICAL_ADDRESS          Start,
  IN UINT64                        Length
  )
/*++

Routine Description:

  Flush cache with specified range.

Arguments:

  Start   - Start address
  Length  - Length in bytes

Returns:

  Status code
  
  EFI_SUCCESS - success

--*/
{
  SalFlushCache (Start, Length);
  return EFI_SUCCESS;
}
