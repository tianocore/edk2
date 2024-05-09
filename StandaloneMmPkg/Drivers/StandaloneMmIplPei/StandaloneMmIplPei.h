/** @file
  Standalone MM IPL Header file

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef STANDALONE_MM_IPL_PEI_H_
#define STANDALONE_MM_IPL_PEI_H_

#include <StandaloneMm.h>
#include <Guid/MmCommBuffer.h>
#include <Guid/MmramMemoryReserve.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MmUnblockMemoryLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Ppi/MmControl.h>
#include <Ppi/MmCommunication.h>
#include <Protocol/MmCommunication.h>

/**
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_MM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into MMRAM.
  @param[in, out] CommSize       The size of the data buffer being passed in.On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_NOT_STARTED        The service is NOT started.
**/
EFI_STATUS
EFIAPI
Communicate (
  IN CONST EFI_PEI_MM_COMMUNICATION_PPI  *This,
  IN OUT VOID                            *CommBuffer,
  IN OUT UINTN                           *CommSize
  );

#endif
