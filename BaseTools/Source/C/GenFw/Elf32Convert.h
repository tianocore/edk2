/** @file
Header file for Elf32 Convert solution

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ELF_32_CONVERT_
#define _ELF_32_CONVERT_

BOOLEAN
InitializeElf32 (
  UINT8               *FileBuffer,
  ELF_FUNCTION_TABLE  *ElfFunctions
  );

#endif
