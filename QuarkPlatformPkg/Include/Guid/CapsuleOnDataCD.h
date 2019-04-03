/** @file
Capsule on Data CD GUID.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

This is the contract between the recovery module and device recovery module
in order to convey the name of a given recovery module type

**/

#ifndef _CAPSULE_ON_DATA_CD_H
#define _CAPSULE_ON_DATA_CD_H

#define PEI_CAPSULE_ON_DATA_CD_GUID \
  { \
  0x5cac0099, 0x0dc9, 0x48e5, {0x80, 0x68, 0xbb, 0x95, 0xf5, 0x40, 0x0a, 0x9f } \
  };

extern EFI_GUID gPeiCapsuleOnDataCDGuid;

#endif
