/** @file
*
*  Copyright (c) 2013-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __FDT_H__
#define __FDT_H__

#define FDT_TABLE_GUID \
  { 0xb1b621d5, 0xf19c, 0x41a5, { 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0 } }

extern EFI_GUID gFdtTableGuid;

#define FDT_VARIABLE_GUID \
  { 0x25a4fd4a, 0x9703, 0x4ba9, { 0xa1, 0x90, 0xb7, 0xc8, 0x4e, 0xfb, 0x3e, 0x57 } }

extern EFI_GUID gFdtVariableGuid;

#endif /* __FDT_H__ */
