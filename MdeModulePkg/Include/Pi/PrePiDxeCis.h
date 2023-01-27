/** @file
  Include file matches things in PI.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MDE_MODULEPKG_PRE_PI_DXE_CIS_H_
#define MDE_MODULEPKG_PRE_PI_DXE_CIS_H_

///
/// A memory region that describes system memory that has not been accepted
/// by a corresponding call to the underlying isolation architecture.
///
/// This memory region has not been defined in PI spec, so it is defined in
/// PrePiDxeCis.h. And it is defined in the format of captial letters
/// because only capital letters are allowed to be used for #define declarations.
///
/// After this memory region is defined in PI spec, it should be a value in
/// EFI_GCD_MEMORY_TYPE in PiDxeCis.h.
///
#define  EFI_GCD_MEMORY_TYPE_UNACCEPTED  6

#endif
