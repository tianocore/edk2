/** @file
  The file provides the protocol that disables the behavior that all memory
  gets accepted at ExitBootServices(). This protocol is only meant to be called
  by the OS loader, and not EDK2 itself. The SEV naming is due to the
  coincidence that only SEV-SNP needs this protocol, since SEV-SNP Linux
  support was released before it had support for unaccepted memory. The
  technology enablement thus does not strictly imply support for the unaccepted
  memory type.

  Copyright (c) 2023, Google LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SEV_MEMORY_ACCEPTANCE_H_
#define SEV_MEMORY_ACCEPTANCE_H_

#define OVMF_SEV_MEMORY_ACCEPTANCE_PROTOCOL_GUID \
  {0xc5a010fe, \
   0x38a7, \
   0x4531, \
   {0x8a, 0x4a, 0x05, 0x00, 0xd2, 0xfd, 0x16, 0x49}}

typedef struct _OVMF_SEV_MEMORY_ACCEPTANCE_PROTOCOL
    OVMF_SEV_MEMORY_ACCEPTANCE_PROTOCOL;

/**
  @param This A pointer to a OVMF_SEV_MEMORY_ACCEPTANCE_PROTOCOL.
**/
typedef
  EFI_STATUS
(EFIAPI *OVMF_SEV_ALLOW_UNACCEPTED_MEMORY)(
  IN  OVMF_SEV_MEMORY_ACCEPTANCE_PROTOCOL  *This
  );

///
/// The OVMF_SEV_MEMORY_ACCEPTANCE_PROTOCOL allows the OS loader to
/// indicate to EDK2 that ExitBootServices should not accept all memory.
///
struct _OVMF_SEV_MEMORY_ACCEPTANCE_PROTOCOL {
  OVMF_SEV_ALLOW_UNACCEPTED_MEMORY    AllowUnacceptedMemory;
};

#endif
