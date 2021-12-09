/** @file

  SEV-SNP Page Validation functions.

  Copyright (c) 2021 AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SNP_PAGE_STATE_INTERNAL_H_
#define SNP_PAGE_STATE_INTERNAL_H_

//
// SEV-SNP Page states
//
typedef enum {
  SevSnpPagePrivate,
  SevSnpPageShared,
} SEV_SNP_PAGE_STATE;

VOID
InternalSetPageState (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN                 NumPages,
  IN SEV_SNP_PAGE_STATE    State,
  IN BOOLEAN               UseLargeEntry
  );

VOID
SnpPageStateFailureTerminate (
  VOID
  );

#endif
