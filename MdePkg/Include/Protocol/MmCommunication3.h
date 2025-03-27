/** @file
  EFI MM Communication Protocol 3 as defined in the PI 1.9 specification.

  This protocol provides a means of communicating between drivers outside of MM and MMI
  handlers inside of MM.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2019, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_COMMUNICATION3_H_
#define MM_COMMUNICATION3_H_

#include <Pi/PiMultiPhase.h>

#define EFI_MM_COMMUNICATION3_PROTOCOL_GUID \
  { \
    0xf7234a14, 0xdf2, 0x46c0, { 0xad, 0x28, 0x90, 0xe6, 0xb8, 0x83, 0xa7, 0x2f } \
  }

typedef struct _EFI_MM_COMMUNICATION3_PROTOCOL EFI_MM_COMMUNICATION3_PROTOCOL;

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                     The EFI_MM_COMMUNICATION3_PROTOCOL instance.
  @param[in, out] CommBufferPhysical  Physical address of the MM communication buffer, of which content must
                                      start with EFI_MM_COMMUNICATE_HEADER_V3.
  @param[in, out] CommBufferVirtual   Virtual address of the MM communication buffer, of which content must
                                      start with EFI_MM_COMMUNICATE_HEADER_V3.

  @retval EFI_SUCCESS                 The message was successfully posted.
  @retval EFI_INVALID_PARAMETER       CommBufferPhysical was NULL or CommBufferVirtual was NULL.
  @retval EFI_BAD_BUFFER_SIZE         The buffer is too large for the MM implementation.
                                      If this error is returned, the MessageSize field
                                      in the CommBuffer header, are updated to reflect
                                      the maximum payload size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED           The CommunicateBuffer parameter are in address range
                                      that cannot be accessed by the MM environment.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MM_COMMUNICATE3)(
  IN CONST EFI_MM_COMMUNICATION3_PROTOCOL   *This,
  IN OUT VOID                               *CommBufferPhysical,
  IN OUT VOID                               *CommBufferVirtual
  );

///
/// EFI MM Communication Protocol provides runtime services for communicating
/// between DXE drivers and a registered MMI handler.
///
struct _EFI_MM_COMMUNICATION3_PROTOCOL {
  EFI_MM_COMMUNICATE3    Communicate;
};

extern EFI_GUID  gEfiMmCommunication3ProtocolGuid;

#endif
