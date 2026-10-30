/** @file
  FF-A endpoint discovery for PSA firmware update.

  Copyright (C) 2024 - 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/ArmFfaSvc.h>
#include <IndustryStandard/ArmFfaPartInfo.h>
#include <Library/ArmFfaLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "PsaFwuTransport.h"

/** Find an endpoint advertising the service and required properties. **/
EFI_STATUS
EFIAPI
FindFwuEndpoint (
  IN CONST EFI_GUID  *ServiceGuid,
  IN UINT32          RequiredProperties,
  OUT UINT16         *AgentId
  )
{
  EFI_STATUS              Status;
  EFI_STATUS              ReleaseStatus;
  EFI_FFA_PART_INFO_DESC  PartInfo;
  EFI_GUID                Guid;
  VOID                    *RxBuffer;
  UINT64                  RxBufferSize;
  UINT32                  Count;
  UINT32                  Size;
  UINT32                  Index;
  UINT16                  PartId;
  UINT16                  SelectedId;

  if (AgentId != NULL) {
    *AgentId = 0;
  }

  if ((ServiceGuid == NULL) || (AgentId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsFfaSupported ()) {
    return EFI_UNSUPPORTED;
  }

  Status = ArmFfaLibPartitionIdGet (&PartId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get caller partition ID: %r\n", __func__, Status));
    return Status;
  }

  Status = ArmFfaLibGetRxTxBuffers (NULL, NULL, &RxBuffer, &RxBufferSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get RX buffer: %r\n", __func__, Status));
    return Status;
  }

  if (RxBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: RX buffer is NULL.\n", __func__));
    return EFI_PROTOCOL_ERROR;
  }

  CopyMem (&Guid, ServiceGuid, sizeof (Guid));
  Status = ArmFfaLibPartitionInfoGet (
             &Guid,
             FFA_PART_INFO_FLAG_TYPE_DESC,
             &Count,
             &Size
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Partition query for service %g failed: %r\n", __func__, &Guid, Status));
    return Status;
  }

  // Only the ID and properties are required, including older descriptors.
  if ((Size < OFFSET_OF (EFI_FFA_PART_INFO_DESC, PartitionUuid)) ||
      (Count > RxBufferSize / Size))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid partition descriptors: size=%u count=%u RX capacity=%lu.\n",
      __func__,
      Size,
      Count,
      RxBufferSize
      ));
    ReleaseStatus = ArmFfaLibRxRelease (PartId);
    if (EFI_ERROR (ReleaseStatus)) {
      DEBUG ((DEBUG_ERROR, "%a: RX release after invalid partition descriptors failed: %r\n", __func__, ReleaseStatus));
    }

    return EFI_PROTOCOL_ERROR;
  }

  Status     = EFI_NOT_FOUND;
  SelectedId = 0;
  for (Index = 0; Index < Count; Index++) {
    ZeroMem (&PartInfo, sizeof (PartInfo));
    CopyMem (
      &PartInfo,
      (UINT8 *)RxBuffer + (UINTN)Index * Size,
      OFFSET_OF (EFI_FFA_PART_INFO_DESC, PartitionUuid)
      );
    if ((PartInfo.PartitionProps & RequiredProperties) != RequiredProperties) {
      continue;
    }

    SelectedId = PartInfo.PartitionId;
    Status     = EFI_SUCCESS;
    break;
  }

  if (Status == EFI_NOT_FOUND) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No endpoint for service %g has required properties 0x%08x.\n",
      __func__,
      &Guid,
      RequiredProperties
      ));
  }

  ReleaseStatus = ArmFfaLibRxRelease (PartId);
  if (EFI_ERROR (ReleaseStatus)) {
    DEBUG ((DEBUG_ERROR, "%a: RX release after partition query failed: %r\n", __func__, ReleaseStatus));
    return ReleaseStatus;
  }

  if (!EFI_ERROR (Status)) {
    *AgentId = SelectedId;
    DEBUG ((
      DEBUG_INFO,
      "%a: Selected endpoint 0x%x for service %g (properties 0x%08x).\n",
      __func__,
      SelectedId,
      &Guid,
      RequiredProperties
      ));
  }

  return Status;
}
