/** @file
  This file declares a protocol to transform a capsule.

  It is essentially a hook into capsule processing mechanism that can be used
  to additionally validate a capsule or pre-process it and return a different
  capsule to be processed in its place.

  Copyright (c) 2026, 3mdeb Sp. z o.o. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __CAPSULE_TRANSFORMATION_H__
#define __CAPSULE_TRANSFORMATION_H__

#define CAPSULE_TRANSFORMATION_PROTOCOL_GUID \
  { 0xdc20bf2b, 0x0769, 0x4489, { 0x81, 0xde, 0x7f, 0x9e, 0x38, 0x03, 0x05, 0x14 }}

typedef struct _CAPSULE_TRANSFORMATION_PROTOCOL CAPSULE_TRANSFORMATION_PROTOCOL;

/**
  Optionally transform a capsule.

  @param[in]      This           A pointer to a CAPSULE_TRANSFORMATION_PROTOCOL.
  @param[in, out] CapsuleHeader  A pointer to a capsule header.
                                 On input points to the original capsule header.
                                 On output points to the transformed capsule header (which
                                 may be the original one).

  @retval EFI_SUCCESS             The capsule has been transformed successfully or needs no
                                  transformation.
  @retval EFI_INVALID_PARAMETER   An input pointer is NULL or some capsule check or
                                  transformation has failed.
  @retval Others                  Other implementation-specific error.
**/
typedef
EFI_STATUS
(EFIAPI *TRANSFORM_CAPSULE)(
  IN     CAPSULE_TRANSFORMATION_PROTOCOL  *This,
  IN OUT EFI_CAPSULE_HEADER               **CapsuleHeader
  );

///
/// Provides a hook into capsule processing mechanism to perform custom
/// preprocessing of a capsule.
///
struct _CAPSULE_TRANSFORMATION_PROTOCOL {
  TRANSFORM_CAPSULE    Transform;
};

extern EFI_GUID  gCapsuleTransformationProtocolGuid;

#endif
