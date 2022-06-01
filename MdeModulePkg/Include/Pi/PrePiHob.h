/** @file
  HOB related definitions which has not been officially published in PI.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MDE_MODULEPKG_PRE_PI_HOB_H_
#define MDE_MODULEPKG_PRE_PI_HOB_H_

//
// BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED is defined for unaccepted memory.
// But this defitinion has not been officially in the PI spec. Base
// on the code-first we define BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED at
// MdeModulePkg/Include/Pi/PrePiHob.h.
//
#define BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED  0x00000007

#endif
