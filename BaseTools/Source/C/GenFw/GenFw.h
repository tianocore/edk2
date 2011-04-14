/** @file

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _GEN_FW_H_
#define _GEN_FW_H_

VOID
SetHiiResourceHeader (
  UINT8   *HiiBinData,
  UINT32  OffsetToFile
  );

INTN
IsElfHeader (
  UINT8  *FileBuffer
  );

BOOLEAN
ConvertElf (
  UINT8  **FileBuffer,
  UINT32 *FileLength
  );

#endif
