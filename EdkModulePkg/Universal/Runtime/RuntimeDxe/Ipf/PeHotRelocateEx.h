/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    PeHotRelocateEx.h

Abstract:

    Fixes Intel Itanium(TM) specific relocation types


Revision History

--*/

#ifndef _PEHOTRELOCATE_EX_H_
#define _PEHOTRELOCATE_EX_H_

#define EXT_IMM64(Value, Address, Size, InstPos, ValPos) \
  Value |= (((UINT64) ((*(Address) >> InstPos) & (((UINT64) 1 << Size) - 1))) << ValPos)

#define INS_IMM64(Value, Address, Size, InstPos, ValPos) \
   * (UINT32 *) Address = \
    (*(UINT32 *) Address &~(((1 << Size) - 1) << InstPos)) | \
      ((UINT32) ((((UINT64) Value >> ValPos) & (((UINT64) 1 << Size) - 1))) << InstPos)

#define IMM64_IMM7B_INST_WORD_X       3
#define IMM64_IMM7B_SIZE_X            7
#define IMM64_IMM7B_INST_WORD_POS_X   4
#define IMM64_IMM7B_VAL_POS_X         0

#define IMM64_IMM9D_INST_WORD_X       3
#define IMM64_IMM9D_SIZE_X            9
#define IMM64_IMM9D_INST_WORD_POS_X   18
#define IMM64_IMM9D_VAL_POS_X         7

#define IMM64_IMM5C_INST_WORD_X       3
#define IMM64_IMM5C_SIZE_X            5
#define IMM64_IMM5C_INST_WORD_POS_X   13
#define IMM64_IMM5C_VAL_POS_X         16

#define IMM64_IC_INST_WORD_X          3
#define IMM64_IC_SIZE_X               1
#define IMM64_IC_INST_WORD_POS_X      12
#define IMM64_IC_VAL_POS_X            21

#define IMM64_IMM41a_INST_WORD_X      1
#define IMM64_IMM41a_SIZE_X           10
#define IMM64_IMM41a_INST_WORD_POS_X  14
#define IMM64_IMM41a_VAL_POS_X        22

#define IMM64_IMM41b_INST_WORD_X      1
#define IMM64_IMM41b_SIZE_X           8
#define IMM64_IMM41b_INST_WORD_POS_X  24
#define IMM64_IMM41b_VAL_POS_X        32

#define IMM64_IMM41c_INST_WORD_X      2
#define IMM64_IMM41c_SIZE_X           23
#define IMM64_IMM41c_INST_WORD_POS_X  0
#define IMM64_IMM41c_VAL_POS_X        40

#define IMM64_SIGN_INST_WORD_X        3
#define IMM64_SIGN_SIZE_X             1
#define IMM64_SIGN_INST_WORD_POS_X    27
#define IMM64_SIGN_VAL_POS_X          63

#endif
