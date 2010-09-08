/** @file
  Specific relocation fixups for ARM architecture.

  Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BasePeCoffLibInternals.h"


//
// The PE/COFF specification needs to get update for ARMv7 MOVW/MOVT
// When it gets updated we can move these defines to PeImage.h
//
#define EFI_IMAGE_REL_BASED_ARM_THUMB_MOVW  11
#define EFI_IMAGE_REL_BASED_ARM_THUMB_MOVT  12


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
  Address  = (UINT16)(Movt & 0x000000ff);         // imm8
  Address |= (UINT16)((Movt >> 4) &  0x0000f700); // imm4 imm3
  Address |= (Movt & BIT26 ? BIT11 : 0);          // i
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
  Patch  = ((Address >> 12) & 0x000f);            // imm4 
  Patch |= (((Address & BIT11) != 0) ? BIT10 : 0); // i
  // Mask out instruction bits and or in address
  *(Instruction) = (*Instruction & ~0x040f) | Patch;

  // Second 16-bit chunk of instruction
  Patch  =  Address & 0x000000ff;          // imm8
  Patch |=  ((Address << 4) & 0x00007000); // imm3
  // Mask out instruction bits and or in address
  Instruction++;
  *Instruction = (*Instruction & ~0x70ff) | Patch;
}


/**
  Performs an ARM-based specific relocation fixup and is a no-op on other
  instruction sets.

  @param  Reloc       Pointer to Pointer to the relocation record.
  @param  Fixup       Pointer to the address to fix up.
  @param  FixupData   Pointer to a buffer to log the fixups.
  @param  Adjust      The offset to adjust the fixup.

  @return Status code.

**/
RETURN_STATUS
PeCoffLoaderRelocateImageEx (
  IN UINT16      **Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  )
{
  UINT16      *Fixup16;
  UINT16      FixupVal;
  UINT16      *Addend;

  Fixup16   = (UINT16 *) Fixup;

  switch ((**Reloc) >> 12) {
  case EFI_IMAGE_REL_BASED_ARM_THUMB_MOVW:
    FixupVal = ThumbMovtImmediateAddress (Fixup16) + (UINT16)Adjust;
    ThumbMovtImmediatePatch (Fixup16, FixupVal);

    if (*FixupData != NULL) {
      *FixupData             = ALIGN_POINTER (*FixupData, sizeof (UINT16));
      *(UINT16 *)*FixupData  = *Fixup16;
      *FixupData             = *FixupData + sizeof (UINT16);
    }
    break;

  case EFI_IMAGE_REL_BASED_ARM_THUMB_MOVT:
    // For MOVT you need to know the lower 16-bits do do the math
    // So this relocation entry is really two entries.
    *Reloc = *Reloc + 1;
    Addend = *Reloc; 
    FixupVal = (UINT16)(((ThumbMovtImmediateAddress (Fixup16) << 16) + Adjust + *Addend) >> 16);
    ThumbMovtImmediatePatch (Fixup16, FixupVal);

    if (*FixupData != NULL) {
      *FixupData             = ALIGN_POINTER (*FixupData, sizeof (UINT16));
      *(UINT16 *)*FixupData  = *Fixup16;
      *FixupData             = *FixupData + sizeof (UINT16);
    }
    break;
  
  default:
    return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}

/**
  Returns TRUE if the machine type of PE/COFF image is supported. Supported
  does not mean the image can be executed it means the PE/COFF loader supports
  loading and relocating of the image type. It's up to the caller to support
  the entry point.
  
  The IA32/X64 version PE/COFF loader/relocater both support IA32, X64 and EBC images.

  @param  Machine   Machine type from the PE Header.

  @return TRUE if this PE/COFF loader can load the image

**/
BOOLEAN
PeCoffLoaderImageFormatSupported (
  IN  UINT16  Machine
  )
{
  if ((Machine == IMAGE_FILE_MACHINE_ARMTHUMB_MIXED) || (Machine ==  IMAGE_FILE_MACHINE_EBC)) {
    return TRUE; 
  }

  return FALSE;
}

/**
  Performs an ARM-based specific re-relocation fixup and is a no-op on other
  instruction sets. This is used to re-relocated the image into the EFI virtual
  space for runtime calls.

  @param  Reloc       The pointer to the relocation record.
  @param  Fixup       The pointer to the address to fix up.
  @param  FixupData   The pointer to a buffer to log the fixups.
  @param  Adjust      The offset to adjust the fixup.

  @return Status code.

**/
RETURN_STATUS
PeHotRelocateImageEx (
  IN UINT16      **Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  )
{
  UINT16  *Fixup16;
  UINT16  FixupVal;
  UINT16  *Addend;

  Fixup16 = (UINT16 *)Fixup;

  switch ((**Reloc) >> 12) {
  case EFI_IMAGE_REL_BASED_ARM_THUMB_MOVW:
    *FixupData  = ALIGN_POINTER (*FixupData, sizeof (UINT16));
    if (*(UINT16 *) (*FixupData) == *Fixup16) {
      FixupVal = ThumbMovtImmediateAddress (Fixup16) + (UINT16)Adjust;
      ThumbMovtImmediatePatch (Fixup16, FixupVal);
    }
    break;

  case EFI_IMAGE_REL_BASED_ARM_THUMB_MOVT:
    *FixupData  = ALIGN_POINTER (*FixupData, sizeof (UINT16));
    if (*(UINT16 *) (*FixupData) == *Fixup16) {
      // For MOVT you need to know the lower 16-bits do do the math
      // So this relocation entry is really two entries.
      *Reloc = *Reloc + 1;
      Addend = *Reloc; 
      FixupVal = (UINT16)(((ThumbMovtImmediateAddress (Fixup16) << 16) + Adjust + *Addend) >> 16);
      ThumbMovtImmediatePatch (Fixup16, FixupVal);
    }
    break;

  default:
    DEBUG ((EFI_D_ERROR, "PeHotRelocateEx:unknown fixed type\n"));
    return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}

