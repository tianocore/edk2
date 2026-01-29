/** @file
Header file for Elf64 convert solution

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ELF_64_CONVERT_
#define _ELF_64_CONVERT_

BOOLEAN
InitializeElf64 (
  UINT8               *FileBuffer,
  ELF_FUNCTION_TABLE  *ElfFunctions
  );

#endif
