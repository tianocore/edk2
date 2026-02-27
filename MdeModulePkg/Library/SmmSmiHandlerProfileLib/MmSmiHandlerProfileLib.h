/** @file
  MM driver instance of SmiHandlerProfile Library.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

/**
  The common constructor function for SMI handler profile.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
MmSmiHandlerProfileLibInitialization (
  VOID
  );
