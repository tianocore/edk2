/** @file -- MmCommunicationPei.h
  Provides an interface to send MM request in PEI

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MM_COMMUNICATION_PEI_H_
#define MM_COMMUNICATION_PEI_H_

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>

#include <Protocol/MmCommunication.h>

#include <IndustryStandard/ArmStdSmc.h>

#include <Ppi/MmCommunication.h>

/**
  Entry point of PEI MM Communication driver

  @param  FileHandle   Handle of the file being invoked.
                       Type EFI_PEI_FILE_HANDLE is defined in FfsFindNextFile().
  @param  PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS  If the interface could be successfully installed
  @retval Others       Returned from PeiServicesInstallPpi()
**/
EFI_STATUS
EFIAPI
MmCommunicationPeiInitialize (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  );

/**
  MmCommunicationPeim
  Communicates with a registered handler.
  This function provides a service to send and receive messages from a registered UEFI service during PEI.

  @param[in]      This            The EFI_PEI_MM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer      Pointer to the data buffer
  @param[in, out] CommSize        The size of the data buffer being passed in. On exit, the
                                  size of data being returned. Zero if the handler does not
                                  wish to reply with any data.

  @retval EFI_SUCCESS             The message was successfully posted.
  @retval EFI_INVALID_PARAMETER   CommBuffer was NULL or *CommSize does not match
                                  MessageLength + sizeof (EFI_MM_COMMUNICATE_HEADER).
  @retval EFI_BAD_BUFFER_SIZE     The buffer is too large for the MM implementation.
                                  If this error is returned, the MessageLength field
                                  in the CommBuffer header or the integer pointed by
                                  CommSize, are updated to reflect the maximum payload
                                  size the implementation can accommodate.
  @retval EFI_ACCESS_DENIED       The CommunicateBuffer parameter or CommSize parameter,
                                  if not omitted, are in address range that cannot be
                                  accessed by the MM environment.
**/
EFI_STATUS
EFIAPI
MmCommunicationPeim (
  IN CONST EFI_PEI_MM_COMMUNICATION_PPI  *This,
  IN OUT VOID                            *CommBuffer,
  IN OUT UINTN                           *CommSize
  );

#endif /* MM_COMMUNICATION_PEI_H_ */
