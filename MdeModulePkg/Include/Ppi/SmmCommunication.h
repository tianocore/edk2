/** @file
  EFI SMM Communication PPI definition.

  This Ppi provides a means of communicating between PEIM and SMI
  handlers inside of SMM.
  This Ppi is produced and consumed only in S3 resume boot path.
  It is NOT available in normal boot path.

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef _SMM_COMMUNICATION_PPI_H_
#define _SMM_COMMUNICATION_PPI_H_

#define EFI_PEI_SMM_COMMUNICATION_PPI_GUID \
  { \
    0xae933e1c, 0xcc47, 0x4e38, { 0x8f, 0xe, 0xe2, 0xf6, 0x1d, 0x26, 0x5, 0xdf } \
  }

typedef struct _EFI_PEI_SMM_COMMUNICATION_PPI  EFI_PEI_SMM_COMMUNICATION_PPI;

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_SMM_COMMUNICATION_PPI instance.
  @param[in] CommBuffer          A pointer to the buffer to convey into SMRAM.
  @param[in] CommSize            The size of the data buffer being passed in.On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMM_COMMUNICATE)(
  IN CONST EFI_PEI_SMM_COMMUNICATION_PPI   *This,
  IN OUT VOID                              *CommBuffer,
  IN OUT UINTN                             *CommSize
  );

///
/// EFI SMM Communication Protocol provides runtime services for communicating
/// between DXE drivers and a registered SMI handler.
///
struct _EFI_PEI_SMM_COMMUNICATION_PPI {
  EFI_PEI_SMM_COMMUNICATE  Communicate;
};

extern EFI_GUID gEfiPeiSmmCommunicationPpiGuid;

#endif
