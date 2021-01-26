/** @file
  MM driver instance of SmiHandlerProfile Library.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MM_SMI_HANDLER_PROFILE_LIB_H_
#define _MM_SMI_HANDLER_PROFILE_LIB_H_

/**
  The common constructor function for SMI handler profile.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
MmSmiHandlerProfileLibInitialization (
  VOID
  );

#endif //_SMM_SMI_HANDLER_PROFILE_LIB_H_
