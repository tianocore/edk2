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
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Ppi/MmAccess.h>
#include <Ppi/MmControl.h>
#include <Ppi/MmCommunication.h>
#include <Ppi/MmCommunication3.h>
#include <Ppi/MmCoreFvLocationPpi.h>
#include <Protocol/MmCommunication.h>
#include <Library/MmPlatformHobProducerLib.h>

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
  Communicates with a registered handler.

  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_MM_COMMUNICATE3 instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into MMRAM.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
**/
EFI_STATUS
EFIAPI
Communicate3 (
  IN CONST EFI_PEI_MM_COMMUNICATION3_PPI  *This,
  IN OUT VOID                             *CommBuffer
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

/**
  Add a new HOB to the HOB List.

  @param[in] Hob          The pointer of new HOB buffer.
  @param[in] HobType      Type of the new HOB.
  @param[in] HobLength    Length of the new HOB to allocate.

  @return  NULL if there is no space to create a hob.
  @return  The address point to the new created hob.

**/
VOID *
MmIplCreateHob (
  IN VOID    *Hob,
  IN UINT16  HobType,
  IN UINT16  HobLength
  );

/**
  Create the MM foundation specific HOB list which StandaloneMm Core needed.

  This function build the MM foundation specific HOB list needed by StandaloneMm Core
  based on the PEI HOB list.

  @param[in]      FoundationHobList   The foundation HOB list to be used for HOB creation.
  @param[in, out] FoundationHobSize   The foundation HOB size.
                                      On return, the expected/used size.
  @param[in]      PlatformHobList     Platform HOB list.
  @param[in]      PlatformHobSize     Platform HOB size.
  @param[in]      MmFvBase            Base of firmare volume which included MM core dirver.
  @param[in]      MmFvSize            Size of firmare volume which included MM core dirver.
  @param[in]      MmCoreFileName      File name of MM core dirver.
  @param[in]      MmCoreImageAddress  Image address of MM core dirver.
  @param[in]      MmCoreImageSize     Image size of MM core dirver.
  @param[in]      MmCoreEntryPoint    Entry pinter of MM core dirver.
  @param[in]      MmProfileDataHob    Pointer to MM profile data HOB.
  @param[in]      Block               Pointer of MMRAM descriptor block.

  @retval RETURN_BUFFER_TOO_SMALL     The buffer is too small for HOB creation.
                                      BufferSize is updated to indicate the expected buffer size.
                                      When the input BufferSize is bigger than the expected buffer size,
                                      the BufferSize value will be changed the used buffer size.
  @retval RETURN_SUCCESS              HOB List is created/updated successfully or the input Length is 0.

**/
RETURN_STATUS
CreateMmFoundationHobList (
  IN UINT8                           *FoundationHobList,
  IN OUT UINTN                       *FoundationHobSize,
  IN UINT8                           *PlatformHobList,
  IN UINTN                           PlatformHobSize,
  IN EFI_PHYSICAL_ADDRESS            MmFvBase,
  IN UINT64                          MmFvSize,
  IN EFI_GUID                        *MmCoreFileName,
  IN EFI_PHYSICAL_ADDRESS            MmCoreImageAddress,
  IN UINT64                          MmCoreImageSize,
  IN EFI_PHYSICAL_ADDRESS            MmCoreEntryPoint,
  IN EFI_HOB_MEMORY_ALLOCATION       *MmProfileDataHob,
  IN EFI_MMRAM_HOB_DESCRIPTOR_BLOCK  *Block
  );

/**
  Build memory allocation HOB in PEI HOB list for MM profile data.

  This function is to allocate memory for MM profile data.

  @return          NULL if MM profile data memory allocation HOB build fail.
  @return          Pointer of MM profile data memory allocation HOB if build successfully.

**/
EFI_HOB_MEMORY_ALLOCATION *
BuildMmProfileDataHobInPeiHobList (
  VOID
  );

/**

  Builds a Handoff Information Table HOB.

  @param Hob       - Pointer to handoff information table HOB.
  @param HobEnd    - End of the HOB list.

**/
VOID
CreateMmHobHandoffInfoTable (
  IN EFI_HOB_HANDOFF_INFO_TABLE  *Hob,
  IN VOID                        *HobEnd
  );

#endif
