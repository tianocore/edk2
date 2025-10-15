/** @file

  Copyright (c) 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2017 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PAYLOAD_ENTRY_HELPER_LIB_H_
#define PAYLOAD_ENTRY_HELPER_LIB_H_

/**
  Add a new HOB to the HOB List.

  @param HobType            Type of the new HOB.
  @param HobLength          Length of the new HOB to allocate.

  @return  NULL if there is no space to create a hob.
  @return  The address point to the new created hob.

**/
VOID *
EFIAPI
CreateHob (
  IN  UINT16  HobType,
  IN  UINT16  HobLength
  );

#endif // PAYLOAD_ENTRY_HELPER_LIB_H_
