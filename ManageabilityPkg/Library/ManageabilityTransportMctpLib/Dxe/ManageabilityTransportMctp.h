/** @file

  Manageability transport MCTP internal used definitions.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MANAGEABILITY_TRANSPORT_MCTP_LIB_INTERNAL_H_
#define MANAGEABILITY_TRANSPORT_MCTP_LIB_INTERNAL_H_

#include <Library/ManageabilityTransportLib.h>

#define MANAGEABILITY_TRANSPORT_MCTP_SIGNATURE  SIGNATURE_32 ('M', 'T', 'M', 'C')

///
/// Manageability transport KCS internal data structure.
///
typedef struct {
  UINTN                            Signature;
  MANAGEABILITY_TRANSPORT_TOKEN    Token;
} MANAGEABILITY_TRANSPORT_MCTP;

#define MANAGEABILITY_TRANSPORT_MCTP_FROM_LINK(a)  CR (a, MANAGEABILITY_TRANSPORT_MCTP, Token, MANAGEABILITY_TRANSPORT_MCTP_SIGNATURE)

#endif // MANAGEABILITY_TRANSPORT_MCTP_LIB_INTERNAL_H_
