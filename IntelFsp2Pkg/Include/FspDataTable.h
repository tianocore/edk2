/** @file
  The header file of FSP data table

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FSP_DATA_TABLE_H_
#define _FSP_DATA_TABLE_H_

#pragma pack(1)

#define FSP_DATA_SIGNATURE  SIGNATURE_32 ('F', 'S', 'P', 'D')

typedef struct  {
  UINT32  Signature;
  UINT32  Length;
  UINT32  FsptBase;
  UINT32  FspmBase;
  UINT32  FspsBase;
} FSP_DATA_TABLE;

#pragma pack()

#endif
