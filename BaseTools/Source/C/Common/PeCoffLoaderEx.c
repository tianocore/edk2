/** @file

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    PeCoffLoaderEx.c

Abstract:

    IA32, X64 and IPF Specific relocation fixups

Revision History

--*/

#include <Common/UefiBaseTypes.h>
#include <IndustryStandard/PeImage.h>
#include "PeCoffLib.h"
#include "CommonLib.h"


#define EXT_IMM64(Value, Address, Size, InstPos, ValPos)  \
    Value |= (((UINT64)((*(Address) >> InstPos) & (((UINT64)1 << Size) - 1))) << ValPos)

#define INS_IMM64(Value, Address, Size, InstPos, ValPos)  \
    *(UINT32*)Address = (*(UINT32*)Address & ~(((1 << Size) - 1) << InstPos)) | \
          ((UINT32)((((UINT64)Value >> ValPos) & (((UINT64)1 << Size) - 1))) << InstPos)

#define IMM64_IMM7B_INST_WORD_X         3  
#define IMM64_IMM7B_SIZE_X              7  
#define IMM64_IMM7B_INST_WORD_POS_X     4  
#define IMM64_IMM7B_VAL_POS_X           0  

#define IMM64_IMM9D_INST_WORD_X         3  
#define IMM64_IMM9D_SIZE_X              9  
#define IMM64_IMM9D_INST_WORD_POS_X     18  
#define IMM64_IMM9D_VAL_POS_X           7  

#define IMM64_IMM5C_INST_WORD_X         3  
#define IMM64_IMM5C_SIZE_X              5  
#define IMM64_IMM5C_INST_WORD_POS_X     13  
#define IMM64_IMM5C_VAL_POS_X           16  

#define IMM64_IC_INST_WORD_X            3  
#define IMM64_IC_SIZE_X                 1  
#define IMM64_IC_INST_WORD_POS_X        12  
#define IMM64_IC_VAL_POS_X              21  

#define IMM64_IMM41a_INST_WORD_X        1  
#define IMM64_IMM41a_SIZE_X             10  
#define IMM64_IMM41a_INST_WORD_POS_X    14  
#define IMM64_IMM41a_VAL_POS_X          22  

#define IMM64_IMM41b_INST_WORD_X        1  
#define IMM64_IMM41b_SIZE_X             8  
#define IMM64_IMM41b_INST_WORD_POS_X    24  
#define IMM64_IMM41b_VAL_POS_X          32  

#define IMM64_IMM41c_INST_WORD_X        2  
#define IMM64_IMM41c_SIZE_X             23  
#define IMM64_IMM41c_INST_WORD_POS_X    0  
#define IMM64_IMM41c_VAL_POS_X          40  

#define IMM64_SIGN_INST_WORD_X          3  
#define IMM64_SIGN_SIZE_X               1  
#define IMM64_SIGN_INST_WORD_POS_X      27  
#define IMM64_SIGN_VAL_POS_X            63  

RETURN_STATUS
PeCoffLoaderRelocateIa32Image (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  )
/*++

Routine Description:

  Performs an IA-32 specific relocation fixup

Arguments:

  Reloc      - Pointer to the relocation record

  Fixup      - Pointer to the address to fix up

  FixupData  - Pointer to a buffer to log the fixups

  Adjust     - The offset to adjust the fixup

Returns:

  EFI_UNSUPPORTED   - Unsupported now

--*/
{
  return RETURN_UNSUPPORTED;
}

RETURN_STATUS
PeCoffLoaderRelocateIpfImage (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup, 
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  )
/*++

Routine Description:

  Performs an Itanium-based specific relocation fixup

Arguments:

  Reloc      - Pointer to the relocation record

  Fixup      - Pointer to the address to fix up

  FixupData  - Pointer to a buffer to log the fixups

  Adjust     - The offset to adjust the fixup

Returns:

  Status code

--*/
{
  UINT64      *F64;
  UINT64      FixupVal;

  switch ((*Reloc) >> 12) {

    case EFI_IMAGE_REL_BASED_DIR64:
      F64 = (UINT64 *) Fixup;
      *F64 = *F64 + (UINT64) Adjust;
      if (*FixupData != NULL) {
        *FixupData = ALIGN_POINTER(*FixupData, sizeof(UINT64));
        *(UINT64 *)(*FixupData) = *F64;
        *FixupData = *FixupData + sizeof(UINT64);
      }
      break;

    case EFI_IMAGE_REL_BASED_IA64_IMM64:

      //
      // Align it to bundle address before fixing up the
      // 64-bit immediate value of the movl instruction.
      //

      Fixup = (CHAR8 *)((UINTN) Fixup & (UINTN) ~(15));
      FixupVal = (UINT64)0;
                       
      // 
      // Extract the lower 32 bits of IMM64 from bundle
      //
      EXT_IMM64(FixupVal,
                (UINT32 *)Fixup + IMM64_IMM7B_INST_WORD_X,
                IMM64_IMM7B_SIZE_X,
                IMM64_IMM7B_INST_WORD_POS_X,
                IMM64_IMM7B_VAL_POS_X
                );

      EXT_IMM64(FixupVal,
                (UINT32 *)Fixup + IMM64_IMM9D_INST_WORD_X,
                IMM64_IMM9D_SIZE_X,
                IMM64_IMM9D_INST_WORD_POS_X,
                IMM64_IMM9D_VAL_POS_X
                );

      EXT_IMM64(FixupVal,
                (UINT32 *)Fixup + IMM64_IMM5C_INST_WORD_X,
                IMM64_IMM5C_SIZE_X,
                IMM64_IMM5C_INST_WORD_POS_X,
                IMM64_IMM5C_VAL_POS_X
                );

      EXT_IMM64(FixupVal,
                (UINT32 *)Fixup + IMM64_IC_INST_WORD_X,
                IMM64_IC_SIZE_X,
                IMM64_IC_INST_WORD_POS_X,
                IMM64_IC_VAL_POS_X
                );

      EXT_IMM64(FixupVal,
                (UINT32 *)Fixup + IMM64_IMM41a_INST_WORD_X,
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
      INS_IMM64(FixupVal,
                ((UINT32 *)Fixup + IMM64_IMM7B_INST_WORD_X),
                IMM64_IMM7B_SIZE_X,
                IMM64_IMM7B_INST_WORD_POS_X,
                IMM64_IMM7B_VAL_POS_X
                );

      INS_IMM64(FixupVal,
                ((UINT32 *)Fixup + IMM64_IMM9D_INST_WORD_X),
                IMM64_IMM9D_SIZE_X,
                IMM64_IMM9D_INST_WORD_POS_X,
                IMM64_IMM9D_VAL_POS_X
                );

      INS_IMM64(FixupVal,
                ((UINT32 *)Fixup + IMM64_IMM5C_INST_WORD_X),
                IMM64_IMM5C_SIZE_X,
                IMM64_IMM5C_INST_WORD_POS_X,
                IMM64_IMM5C_VAL_POS_X
                );

      INS_IMM64(FixupVal,
                ((UINT32 *)Fixup + IMM64_IC_INST_WORD_X),
                IMM64_IC_SIZE_X,
                IMM64_IC_INST_WORD_POS_X,
                IMM64_IC_VAL_POS_X
                );

      INS_IMM64(FixupVal,
                ((UINT32 *)Fixup + IMM64_IMM41a_INST_WORD_X),
                IMM64_IMM41a_SIZE_X,
                IMM64_IMM41a_INST_WORD_POS_X,
                IMM64_IMM41a_VAL_POS_X
                );

      INS_IMM64(FixupVal,
                ((UINT32 *)Fixup + IMM64_IMM41b_INST_WORD_X),
                IMM64_IMM41b_SIZE_X,
                IMM64_IMM41b_INST_WORD_POS_X,
                IMM64_IMM41b_VAL_POS_X
                );

      INS_IMM64(FixupVal,
                ((UINT32 *)Fixup + IMM64_IMM41c_INST_WORD_X),
                IMM64_IMM41c_SIZE_X,
                IMM64_IMM41c_INST_WORD_POS_X,
                IMM64_IMM41c_VAL_POS_X
                );

      INS_IMM64(FixupVal,
                ((UINT32 *)Fixup + IMM64_SIGN_INST_WORD_X),
                IMM64_SIGN_SIZE_X,
                IMM64_SIGN_INST_WORD_POS_X,
                IMM64_SIGN_VAL_POS_X
                );

      F64 = (UINT64 *) Fixup;
      if (*FixupData != NULL) {
        *FixupData = ALIGN_POINTER(*FixupData, sizeof(UINT64));
        *(UINT64 *)(*FixupData) = *F64;
        *FixupData = *FixupData + sizeof(UINT64);
      }
      break;

    default:
      return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}

RETURN_STATUS
PeCoffLoaderRelocateX64Image (
  IN     UINT16       *Reloc,
  IN OUT CHAR8        *Fixup, 
  IN OUT CHAR8        **FixupData,
  IN     UINT64       Adjust
  )
/**
  Performs an x64 specific relocation fixup

  @param Reloc        Pointer to the relocation record
  @param Fixup        Pointer to the address to fix up
  @param FixupData    Pointer to a buffer to log the fixups
  @param Adjust       The offset to adjust the fixup
  
  @retval RETURN_SUCCESS      Success to perform relocation
  @retval RETURN_UNSUPPORTED  Unsupported.
**/
{
  UINT64      *F64;

  switch ((*Reloc) >> 12) {

    case EFI_IMAGE_REL_BASED_DIR64:
      F64 = (UINT64 *) Fixup;
      *F64 = *F64 + (UINT64) Adjust;
      if (*FixupData != NULL) {
        *FixupData = ALIGN_POINTER(*FixupData, sizeof(UINT64));
        *(UINT64 *)(*FixupData) = *F64;
        *FixupData = *FixupData + sizeof(UINT64);
      }
      break;

    default:
      return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}

/**
  Pass in a pointer to an ARM MOVT or MOVW immediate instruciton and 
  return the immediate data encoded in the instruction

  @param  Instruction   Pointer to ARM MOVT or MOVW immediate instruction

  @return Immediate address encoded in the instruction

**/
UINT16
ThumbMovtImmediateAddress (
  IN UINT16 *Instruction
  )
{
  UINT32  Movt;
  UINT16  Address;

  // Thumb2 is two 16-bit instructions working together. Not a single 32-bit instruction
  // Example MOVT R0, #0 is 0x0000f2c0 or 0xf2c0 0x0000
  Movt = (*Instruction << 16) | (*(Instruction + 1)); 

  // imm16 = imm4:i:imm3:imm8
  //         imm4 -> Bit19:Bit16
  //         i    -> Bit26
  //         imm3 -> Bit14:Bit12
  //         imm8 -> Bit7:Bit0
  Address  = (UINT16)(Movt & 0x000000ff);          // imm8
  Address |= (UINT16)((Movt >> 4) &  0x0000f700);  // imm4 imm3
  Address |= (((Movt & BIT26) != 0) ? BIT11 : 0);  // i
  return Address;
}


/**
  Update an ARM MOVT or MOVW immediate instruction immediate data.

  @param  Instruction   Pointer to ARM MOVT or MOVW immediate instruction
  @param  Address       New addres to patch into the instruction
**/
VOID
ThumbMovtImmediatePatch (
  IN OUT UINT16 *Instruction,
  IN     UINT16 Address
  )
{
  UINT16  Patch;

  // First 16-bit chunk of instruciton
  Patch  = ((Address >> 12) & 0x000f);             // imm4 
  Patch |= (((Address & BIT11) != 0) ? BIT10 : 0); // i
  *Instruction = (*Instruction & ~0x040f) | Patch;

  // Second 16-bit chunk of instruction
  Patch  =  Address & 0x000000ff;          // imm8
  Patch |=  ((Address << 4) & 0x00007000); // imm3
  Instruction++;
  *Instruction = (*Instruction & ~0x70ff) | Patch;
}

/**
  Pass in a pointer to an ARM MOVW/MOVT instruciton pair and 
  return the immediate data encoded in the two` instruction

  @param  Instructions  Pointer to ARM MOVW/MOVT insturction pair

  @return Immediate address encoded in the instructions

**/
UINT32
EFIAPI
ThumbMovwMovtImmediateAddress (
  IN UINT16 *Instructions
  )
{
  UINT16  *Word;
  UINT16  *Top;
  
  Word = Instructions;  // MOVW
  Top  = Word + 2;      // MOVT
  
  return (ThumbMovtImmediateAddress (Top) << 16) + ThumbMovtImmediateAddress (Word);
}


/**
  Update an ARM MOVW/MOVT immediate instruction instruction pair.

  @param  Instructions  Pointer to ARM MOVW/MOVT instruction pair
  @param  Address       New addres to patch into the instructions
**/
VOID
EFIAPI
ThumbMovwMovtImmediatePatch (
  IN OUT UINT16 *Instructions,
  IN     UINT32 Address
  )
{
  UINT16  *Word;
  UINT16  *Top;
  
  Word = (UINT16 *)Instructions;  // MOVW
  Top  = Word + 2;                // MOVT

  ThumbMovtImmediatePatch (Word, (UINT16)(Address & 0xffff));
  ThumbMovtImmediatePatch (Top, (UINT16)(Address >> 16));
}


/**
  Performs an ARM-based specific relocation fixup and is a no-op on other
  instruction sets.

  @param  Reloc       Pointer to the relocation record.
  @param  Fixup       Pointer to the address to fix up.
  @param  FixupData   Pointer to a buffer to log the fixups.
  @param  Adjust      The offset to adjust the fixup.

  @return Status code.

**/
RETURN_STATUS
PeCoffLoaderRelocateArmImage (
  IN UINT16      **Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  )
{
  UINT16      *Fixup16;
  UINT32      FixupVal;

  Fixup16   = (UINT16 *) Fixup;

  switch ((**Reloc) >> 12) {
  
  case EFI_IMAGE_REL_BASED_ARM_MOV32T:
    FixupVal = ThumbMovwMovtImmediateAddress (Fixup16) + (UINT32)Adjust;
    ThumbMovwMovtImmediatePatch (Fixup16, FixupVal);
    
    
    if (*FixupData != NULL) {
      *FixupData = ALIGN_POINTER(*FixupData, sizeof(UINT64));
      *(UINT64 *)(*FixupData) = *Fixup16;
      CopyMem (*FixupData, Fixup16, sizeof (UINT64));
    }
    break;
  
  case EFI_IMAGE_REL_BASED_ARM_MOV32A:
     // break omitted - ARM instruction encoding not implemented
  default:
    return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}
