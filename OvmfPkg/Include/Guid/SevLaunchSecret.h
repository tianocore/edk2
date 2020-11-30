 /** @file
   UEFI Configuration Table for exposing the SEV Launch Secret location to UEFI
   applications (boot loaders).

   Copyright (C) 2020 James Bottomley, IBM Corporation.
   SPDX-License-Identifier: BSD-2-Clause-Patent
 **/

#ifndef SEV_LAUNCH_SECRET_H_
#define SEV_LAUNCH_SECRET_H_

#include <Uefi/UefiBaseType.h>

#define SEV_LAUNCH_SECRET_GUID                          \
  { 0xadf956ad,                                         \
    0xe98c,                                             \
    0x484c,                                             \
    { 0xae, 0x11, 0xb5, 0x1c, 0x7d, 0x33, 0x64, 0x47 }, \
  }

typedef struct {
  UINT32 Base;
  UINT32 Size;
} SEV_LAUNCH_SECRET_LOCATION;

extern EFI_GUID gSevLaunchSecretGuid;

#endif // SEV_LAUNCH_SECRET_H_
