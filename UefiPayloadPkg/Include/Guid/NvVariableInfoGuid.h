/** @file
  This file defines the hob structure for the SPI flash variable info.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __NV_VARIABLE_INFO_GUID_H__
#define __NV_VARIABLE_INFO_GUID_H__

//
// NV variable hob info GUID
//
extern EFI_GUID gNvVariableInfoGuid;

typedef struct {
  UINT8                  Revision;
  UINT8                  Reserved[3];
  UINT32                 VariableStoreBase;
  UINT32                 VariableStoreSize;
} NV_VARIABLE_INFO;

#endif
