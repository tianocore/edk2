/** @file
  Provides an abstracted interface for configuring PK related variable protection.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_PK_PROTECTION_LIB_H_
#define PLATFORM_PK_PROTECTION_LIB_H_

/**
  Disable any applicable protection against variable 'PK'. The implementation
  of this interface is platform specific, depending on the protection techniques
  used per platform.

  Note: It is the platform's responsibility to conduct cautious operation after
        disabling this protection.

  @retval     EFI_SUCCESS             State has been successfully updated.
  @retval     Others                  Error returned from implementation specific
                                      underying APIs.

**/
EFI_STATUS
EFIAPI
DisablePKProtection (
  VOID
  );

#endif
