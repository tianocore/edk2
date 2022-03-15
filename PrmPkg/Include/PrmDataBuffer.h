/** @file

  Definitions for the Platform Runtime Mechanism (PRM) data buffer structures.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_DATA_BUFFER_H_
#define PRM_DATA_BUFFER_H_

#include <Uefi.h>

#define PRM_DATA_BUFFER_HEADER_SIGNATURE  SIGNATURE_32('P','R','M','D')

#pragma pack(push, 1)

///
/// A generic header that describes the PRM data buffer.
///
typedef struct {
  ///
  /// PRM Data Buffer signature.
  ///
  UINT32    Signature;
  ///
  /// Length of the entire data buffer, including the size of the header.
  ///
  UINT32    Length;
} PRM_DATA_BUFFER_HEADER;

///
/// A PRM data buffer is a generic header followed by variable length arbitrary data.
///
typedef struct {
  ///
  /// The header is required at the beginning of every PRM data buffer.
  ///
  PRM_DATA_BUFFER_HEADER    Header;

  ///
  /// The beginning of data immediately follows the header.
  ///
  UINT8                     Data[1];
} PRM_DATA_BUFFER;

#pragma pack(pop)

#endif
