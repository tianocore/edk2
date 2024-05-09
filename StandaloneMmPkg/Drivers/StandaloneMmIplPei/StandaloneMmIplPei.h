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
#include <Ppi/SmmControl.h>
#include <Ppi/MmCommunication.h>
#include <Protocol/SmmCommunication.h>

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

/**
  This is the callback function on end of PEI.

  This callback is used for call MmEndOfPeiHandler in standalone MM core.

  @param   PeiServices       General purpose services available to every PEIM.
  @param   NotifyDescriptor  The notification structure this PEIM registered on install.
  @param   Ppi               Pointer to the PPI data associated with this function.
  @retval  EFI_SUCCESS       Exit boot services successfully.
  @retval  Other             Exit boot services failed.
**/
EFI_STATUS
EFIAPI
EndOfPeiCallback (
  IN  EFI_PEI_SERVICES           **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN  VOID                       *Ppi
  );

#endif
