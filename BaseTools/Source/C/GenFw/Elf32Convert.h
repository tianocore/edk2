/** @file
Header file for Elf32 Convert solution

Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ELF_32_CONVERT_
#define _ELF_32_CONVERT_

BOOLEAN
InitializeElf32 (
  UINT8               *FileBuffer,
  ELF_FUNCTION_TABLE  *ElfFunctions
  );

#endif
