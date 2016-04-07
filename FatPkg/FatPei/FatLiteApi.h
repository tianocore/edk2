/** @file
  Definitions for FAT recovery PEIM API functions

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FAT_API_H_
#define _FAT_API_H_

//
// API data structures
//
typedef VOID  *PEI_FILE_HANDLE;

typedef enum {
  Fat12,
  Fat16,
  Fat32,
  FatUnknown
} PEI_FAT_TYPE;

#endif
