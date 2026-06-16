/** @file
  This module provides communication with RAS Agent over RPMI/MPXY.

  @par Glossary:
    - RPMI - RAS Platform Management Interface
    - MPXY - Message Proxy extension in the RISC-V SBI specification

  Copyright (c) 2026, Qualcomm Technologies, Inc.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>

//
// Limit the maximum number of agents tracked by this library instance.
// This is intentionally capped to match the MPXY channel-list chunk size.
//
#define MAX_RAS_AGENTS  64

typedef enum {
  DtGhesV2,
  NumErrDescTypes
} ERROR_DESCRIPTOR_TYPE;

#define ERROR_DESCRIPTOR_TYPE_SHIFT  4
#define MAX_ERROR_DESCRIPTOR_TYPES   (0x1UL << ERROR_DESCRIPTOR_TYPE_SHIFT)
#define ERROR_DESCRIPTOR_TYPE_MASK   (MAX_ERROR_DESCRIPTOR_TYPES - 1)

/**
  Initialize the RAS agent client

  @retval EFI_SUCCESS  If initialization is successful
**/
EFI_STATUS
EFIAPI
RacInit (
  VOID
  );

/**
   Get number of RAS agents present

   @retval Returns number of RAS agents present
**/
EFI_STATUS
EFIAPI
RacGetNumRasAgents (
  VOID
  );

/**
   Get the list of RAS agent MPXY channel Ids

   @param[in]  ChannelIdSize   Sizeof the buffer provided to fetch list of channel Ids
   @param[out] ChannelId       Pointer to the buffer to fetch list of channel Ids
   @param[out] NumChannelIds   Number of channel Ids put in the buffer

   @retval EFI_SUCCESS on success
   @retval EFI_INVALID_PARAMETER if any parameters are invalid
**/
EFI_STATUS
EFIAPI
RacGetRasAgentMpxyChannelId (
  IN  UINT32  ChannelIdSize,
  OUT UINT32  *ChannelId,
  OUT UINT32  *NumChannelIds
  );

/**
   Open a RAS agent's MPXY channel

   @param[in] ChannelId  RAS agent's MPXY channel
   @retval EFI_SUCCESS on success
   @retval EFI errors as returned by `SbiMpxyChannelOpen`
**/
EFI_STATUS
EFIAPI
RacOpenRasAgentChannel (
  IN  UINT32  ChannelId
  );

/**
   Close a RAS agent's MPXY channel

   @param[in] ChannelId  RAS agent's MPXY channel
   @retval    EFI_SUCCESS on success
   @retval    EFI_ABORTED if a found context has zero reference count
   @retval    EFI errors as returned by `SbiMpxyChannelClose`
**/
EFI_STATUS
EFIAPI
RacCloseRasAgentChannel (
  IN  UINT32  ChannelId
  );

/**
  Get the number of hardware error sources from the RAS Agent

  @param NumErrorSources  Pointer to an array of 32-bit integers which will
                          contain number of hardware error sources available.

  @retval EFI_SUCCESS  If fetching the number of error sources succeeded.
**/
EFI_STATUS
EFIAPI
RacGetNumberErrorSources (
  IN  UINT32  ChannelId,
  OUT UINT32  *NumErrorSources
  );

/**
  Get the list of hardware error source IDs from the RAS Agent

  @param ErrorSourceList  Will contain pointer to error of 32-bit integers
                          containing the error source IDs.
  @param NumSources       Will contain the number of IDs in *ErrorSourceList

  @retval EFI_SUCCESS  If fetching the error source IDs succeeded.
**/
EFI_STATUS
EFIAPI
RacGetErrorSourceIDList (
  IN  UINT32  ChannelId,
  OUT UINT32  **ErrorSourceList,
  OUT UINT32  *NumSources
  );

/**
  Get the hardware error source descriptor for a given error source ID.

  @param SourceID  Error source ID for which descriptor is to be fetched
  @param DescriptorType  Type of error descriptor (GHES version 2 or platform specific).
  @param ErrorDescriptor  Pointer to buffer containing the descriptor. The caller
                         must free the buffer when done.
  @param ErrorDescriptorSize  Size of the error descriptor buffer in ErrorDescriptor

  @retval EFI_SUCCESS  On success.
**/
EFI_STATUS
EFIAPI
RacGetErrorSourceDescriptor (
  IN  UINT32  ChannelId,
  IN  UINT32  SourceID,
  OUT UINTN   *DescriptorType,
  OUT VOID    **ErrorDescriptor,
  OUT UINT32  *ErrorDescriptorSize
  );
