/** @file
  This file defines the data structures per HOB specification v0.9.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  These definitions are from the HOB Spec 0.9 that were not adopted by the PI specifications.

**/

#ifndef _HOB_H_
#define _HOB_H_

///
/// Capsule volume HOB -- identical to a firmware volume.
/// This macro is defined to comply with the hob Framework Spec. And the marco was
/// retired in the PI1.0 specification.
///
#define EFI_HOB_TYPE_CV  0x0008

typedef struct {
  EFI_HOB_GENERIC_HEADER    Header;
  EFI_PHYSICAL_ADDRESS      BaseAddress;
  UINT64                    Length;
} EFI_HOB_CAPSULE_VOLUME;

#endif
