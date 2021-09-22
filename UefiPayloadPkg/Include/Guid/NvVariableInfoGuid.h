/** @file
  This file defines the hob structure for the SPI flash variable info.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NV_VARIABLE_INFO_GUID_H_
#define NV_VARIABLE_INFO_GUID_H_

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
