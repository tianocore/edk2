/** @file
  EFI MM Communication v3 PPI definition.

  This Ppi provides a means of communicating between PEIM and MMI
  handlers inside of MM.

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_COMMUNICATION3_PPI_H_
#define MM_COMMUNICATION3_PPI_H_

#include <Pi/PiMultiPhase.h>

#define EFI_PEI_MM_COMMUNICATION3_PPI_GUID \
  { \
    0xe70febf6, 0x13ef, 0x4a21, { 0x89, 0x9e, 0xb2, 0x36, 0xf8, 0x25, 0xff, 0xc9 } \
  }

typedef struct _EFI_PEI_MM_COMMUNICATION3_PPI EFI_PEI_MM_COMMUNICATION3_PPI;

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_MM_COMMUNICATE3 instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into MMRAM.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_MM_COMMUNICATE3)(
  IN CONST EFI_PEI_MM_COMMUNICATION3_PPI   *This,
  IN OUT VOID                              *CommBuffer
  );

///
/// EFI MM Communication PPI provides runtime services for communicating
/// between DXE drivers and a registered SMI handler.
///
struct _EFI_PEI_MM_COMMUNICATION3_PPI {
  EFI_PEI_MM_COMMUNICATE3    Communicate;
};

extern EFI_GUID  gEfiPeiMmCommunication3PpiGuid;

#endif
