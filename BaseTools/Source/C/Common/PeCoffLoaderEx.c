/** @file
IA32 and X64 Specific relocation fixups

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
Portions Copyright (c) 2011 - 2013, ARM Ltd. All rights reserved.<BR>
Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
Copyright (c) 2022, Loongson Technology Corporation Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

--*/

#include <Common/UefiBaseTypes.h>
#include <IndustryStandard/PeImage.h>
#include "PeCoffLib.h"
#include "CommonLib.h"
#include "EfiUtilityMsgs.h"


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

UINT32 *RiscVHi20Fixup = NULL;

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

/*++

Routine Description:

  Performs an RISC-V specific relocation fixup

Arguments:

  Reloc      - Pointer to the relocation record

  Fixup      - Pointer to the address to fix up

  FixupData  - Pointer to a buffer to log the fixups

  Adjust     - The offset to adjust the fixup

Returns:

  Status code

--*/
RETURN_STATUS
PeCoffLoaderRelocateRiscVImage (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  )
{
  UINT32 Value;
  UINT32 Value2;

  switch ((*Reloc) >> 12) {
  case EFI_IMAGE_REL_BASED_RISCV_HI20:
      RiscVHi20Fixup = (UINT32 *) Fixup;
      break;

  case EFI_IMAGE_REL_BASED_RISCV_LOW12I:
      if (RiscVHi20Fixup != NULL) {
        Value = (UINT32)(RV_X(*RiscVHi20Fixup, 12, 20) << 12);
        Value2 = (UINT32)(RV_X(*(UINT32 *)Fixup, 20, 12));
        if (Value2 & (RISCV_IMM_REACH/2)) {
          Value2 |= ~(RISCV_IMM_REACH-1);
        }
        Value += Value2;
        Value += (UINT32)Adjust;
        Value2 = RISCV_CONST_HIGH_PART (Value);
        *(UINT32 *)RiscVHi20Fixup = (RV_X (Value2, 12, 20) << 12) | \
                                           (RV_X (*(UINT32 *)RiscVHi20Fixup, 0, 12));
        *(UINT32 *)Fixup = (RV_X (Value, 0, 12) << 20) | \
                           (RV_X (*(UINT32 *)Fixup, 0, 20));
      }
      RiscVHi20Fixup = NULL;
      break;

  case EFI_IMAGE_REL_BASED_RISCV_LOW12S:
      if (RiscVHi20Fixup != NULL) {
        Value = (UINT32)(RV_X(*RiscVHi20Fixup, 12, 20) << 12);
        Value2 = (UINT32)(RV_X(*(UINT32 *)Fixup, 7, 5) | (RV_X(*(UINT32 *)Fixup, 25, 7) << 5));
        if (Value2 & (RISCV_IMM_REACH/2)) {
          Value2 |= ~(RISCV_IMM_REACH-1);
        }
        Value += Value2;
        Value += (UINT32)Adjust;
        Value2 = RISCV_CONST_HIGH_PART (Value);
        *(UINT32 *)RiscVHi20Fixup = (RV_X (Value2, 12, 20) << 12) | \
                                           (RV_X (*(UINT32 *)RiscVHi20Fixup, 0, 12));
        Value2 = *(UINT32 *)Fixup & 0x01fff07f;
        Value &= RISCV_IMM_REACH - 1;
        *(UINT32 *)Fixup = Value2 | (UINT32)(((RV_X(Value, 0, 5) << 7) | (RV_X(Value, 5, 7) << 25)));
      }
      RiscVHi20Fixup = NULL;
      break;

  default:
      return EFI_UNSUPPORTED;

  }
  return RETURN_SUCCESS;
}

/**
  Pass in a pointer to an ARM MOVT or MOVW immediate instruction and
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
  @param  Address       New address to patch into the instruction
**/
VOID
ThumbMovtImmediatePatch (
  IN OUT UINT16 *Instruction,
  IN     UINT16 Address
  )
{
  UINT16  Patch;

  // First 16-bit chunk of instruction
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
  Pass in a pointer to an ARM MOVW/MOVT instruction pair and
  return the immediate data encoded in the two` instruction

  @param  Instructions  Pointer to ARM MOVW/MOVT instruction pair

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
  @param  Address       New address to patch into the instructions
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
      CopyMem (*FixupData, Fixup16, sizeof (UINT64));
      *FixupData = *FixupData + sizeof(UINT64);
    }
    break;

  case EFI_IMAGE_REL_BASED_ARM_MOV32A:
     // break omitted - ARM instruction encoding not implemented
  default:
    return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}

/**
  Performs a LoongArch specific relocation fixup.

  @param[in]       Reloc       Pointer to the relocation record.
  @param[in, out]  Fixup       Pointer to the address to fix up.
  @param[in, out]  FixupData   Pointer to a buffer to log the fixups.
  @param[in]       Adjust      The offset to adjust the fixup.

  @return Status code.
**/
RETURN_STATUS
PeCoffLoaderRelocateLoongArch64Image (
  IN UINT16     *Reloc,
  IN OUT CHAR8  *Fixup,
  IN OUT CHAR8  **FixupData,
  IN UINT64     Adjust
  )
{
  UINT8  RelocType;
  UINT64 Value;
  UINT64 Tmp1;
  UINT64 Tmp2;

  RelocType = ((*Reloc) >> 12);
  Value     = 0;
  Tmp1      = 0;
  Tmp2      = 0;

  switch (RelocType) {
    case EFI_IMAGE_REL_BASED_LOONGARCH64_MARK_LA:
      // The next four instructions are used to load a 64 bit address, relocate all of them
      Value = (*(UINT32 *)Fixup & 0x1ffffe0) << 7 |       // lu12i.w 20bits from bit5
              (*((UINT32 *)Fixup + 1) & 0x3ffc00) >> 10;  // ori     12bits from bit10
      Tmp1   = *((UINT32 *)Fixup + 2) & 0x1ffffe0;        // lu32i.d 20bits from bit5
      Tmp2   = *((UINT32 *)Fixup + 3) & 0x3ffc00;         // lu52i.d 12bits from bit10
      Value  = Value | (Tmp1 << 27) | (Tmp2 << 42);
      Value += Adjust;

      *(UINT32 *)Fixup = (*(UINT32 *)Fixup & ~0x1ffffe0) | (((Value >> 12) & 0xfffff) << 5);
      if (*FixupData != NULL) {
        *FixupData              = ALIGN_POINTER (*FixupData, sizeof (UINT32));
        *(UINT32 *)(*FixupData) = *(UINT32 *)Fixup;
        *FixupData              = *FixupData + sizeof (UINT32);
      }

      Fixup           += sizeof (UINT32);
      *(UINT32 *)Fixup = (*(UINT32 *)Fixup & ~0x3ffc00) | ((Value & 0xfff) << 10);
      if (*FixupData != NULL) {
        *FixupData              = ALIGN_POINTER (*FixupData, sizeof (UINT32));
        *(UINT32 *)(*FixupData) = *(UINT32 *)Fixup;
        *FixupData              = *FixupData + sizeof (UINT32);
      }

      Fixup           += sizeof (UINT32);
      *(UINT32 *)Fixup = (*(UINT32 *)Fixup & ~0x1ffffe0) | (((Value >> 32) & 0xfffff) << 5);
      if (*FixupData != NULL) {
        *FixupData              = ALIGN_POINTER (*FixupData, sizeof (UINT32));
        *(UINT32 *)(*FixupData) = *(UINT32 *)Fixup;
        *FixupData              = *FixupData + sizeof (UINT32);
      }

      Fixup           += sizeof (UINT32);
      *(UINT32 *)Fixup = (*(UINT32 *)Fixup & ~0x3ffc00) | (((Value >> 52) & 0xfff) << 10);
      if (*FixupData != NULL) {
        *FixupData              = ALIGN_POINTER (*FixupData, sizeof (UINT32));
        *(UINT32 *)(*FixupData) = *(UINT32 *)Fixup;
        *FixupData              = *FixupData + sizeof (UINT32);
      }

      break;
    default:
      Error (NULL, 0, 3000, "", "PeCoffLoaderRelocateLoongArch64Image: Fixup[0x%x] Adjust[0x%llx] *Reloc[0x%x], type[0x%x].", *(UINT32 *)Fixup, Adjust, *Reloc, RelocType);
      return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}
