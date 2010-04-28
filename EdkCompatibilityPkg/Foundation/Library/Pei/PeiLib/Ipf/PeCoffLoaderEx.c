/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    PeCoffLoaderEx.c

Abstract:

    Fixes Intel Itanium(TM) specific relocation types


Revision History

--*/

#include "TianoCommon.h"
#include "EfiImage.h"

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

EFI_STATUS
PeCoffLoaderRelocateImageEx (
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
      return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

BOOLEAN
PeCoffLoaderImageFormatSupported (
  IN  UINT16  Machine
  )
/*++
Routine Description:

  Returns TRUE if the machine type of PE/COFF image is supported. Supported 
  does not mean the image can be executed it means the PE/COFF loader supports
  loading and relocating of the image type. It's up to the caller to support
  the entry point. 

  This function implies the basic PE/COFF loader/relocator supports IPF, EBC,
  images. Calling the entry point in a correct mannor is up to the 
  consumer of this library.

Arguments:

  Machine   - Machine type from the PE Header.

Returns:

  TRUE      - if this PE/COFF loader can load the image
  FALSE     - if this PE/COFF loader cannot load the image

--*/
{
  if ((Machine == EFI_IMAGE_MACHINE_IA64) || (Machine == EFI_IMAGE_MACHINE_EBC)) {
    return TRUE; 
  }

  return FALSE;
}
