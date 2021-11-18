/** @file

  Copyright (c) 2018, Linaro. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PLATFORM_BOOT_MANAGER_PROTOCOL_H__
#define __PLATFORM_BOOT_MANAGER_PROTOCOL_H__

//
// Protocol interface structure
//
typedef struct _PLATFORM_BOOT_MANAGER_PROTOCOL PLATFORM_BOOT_MANAGER_PROTOCOL;

//
// Function Prototypes
//

/*
  Get predefined boot options for platform.

  @param[out] Count            The number of elements in each of
                               BootOptions and BootKeys. On successful
                               return, Count is at least one.

  @param[out] BootOptions      An array of platform boot options.
                               BootOptions is pool-allocated by
                               GET_PLATFORM_BOOT_OPTIONS_AND_KEYS, and
                               GET_PLATFORM_BOOT_OPTIONS_AND_KEYS populates
                               every element in BootOptions with
                               EfiBootManagerInitializeLoadOption().
                               BootOptions shall not contain duplicate
                               entries. The caller is responsible for
                               releasing BootOptions after use with
                               EfiBootManagerFreeLoadOptions().

  @param[out] BootKeys         A pool-allocated array of platform boot
                               hotkeys. For every 0 <= Index < Count, if
                               BootOptions[Index] is not to be associated
                               with a hotkey, then BootKeys[Index] is
                               zero-filled. Otherwise, BootKeys[Index]
                               specifies the boot hotkey for
                               BootOptions[Index]. BootKeys shall not
                               contain duplicate entries (other than
                               zero-filled entries). The caller is
                               responsible for releasing BootKeys with
                               FreePool() after use.

  @retval EFI_SUCCESS          Count, BootOptions and BootKeys have
                               been set.

  @retval EFI_OUT_OF_RESOURCES Memory allocation failed.

  @retval EFI_UNSUPPORTED      The platform doesn't provide boot options
                               as a feature.

  @retval EFI_NOT_FOUND        The platform could provide boot options
                               as a feature, but none have been
                               configured.

  @return                      Error codes propagated from underlying
                               functions.
*/
typedef
EFI_STATUS
(EFIAPI *GET_PLATFORM_BOOT_OPTIONS_AND_KEYS)(
  OUT UINTN                              *Count,
  OUT EFI_BOOT_MANAGER_LOAD_OPTION       **BootOptions,
  OUT EFI_INPUT_KEY                      **BootKeys
  );

struct _PLATFORM_BOOT_MANAGER_PROTOCOL {
  GET_PLATFORM_BOOT_OPTIONS_AND_KEYS    GetPlatformBootOptionsAndKeys;
};

extern EFI_GUID  gPlatformBootManagerProtocolGuid;

#endif /* __PLATFORM_BOOT_MANAGER_PROTOCOL_H__ */
