/** @file
  This file provides IPMI PPI implementation.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/IpmiPlatformInitLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ManageabilityTransportLib.h>
#include <Library/ManageabilityTransportIpmiLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <Library/PeiServicesLib.h>
#include <IndustryStandard/Ipmi.h>

#include <Ppi/IpmiPpi.h>
#include <Ppi/MemoryDiscovered.h>

#include "IpmiProtocolCommon.h"
#include "IpmiPpiInternal.h"

/**
  Release a failed, current-generation initialization candidate.

  @param[in,out] Private  Provider context, already guarded against reentry.

  @retval EFI_SUCCESS  There is no remaining candidate.
  @retval Other        Release failed; retain the candidate for a clean retry.
**/
STATIC
EFI_STATUS
PeiIpmiReleasePendingTransport (
  IN OUT PEI_IPMI_PPI_INTERNAL  *Private
  )
{
  EFI_STATUS  Status;

  if (Private->PendingTransportToken == NULL) {
    return EFI_SUCCESS;
  }

  Status = ReleaseTransportSession (Private->PendingTransportToken);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to release current transport - %r\n", __func__, Status));
    return Status;
  }

  Private->PendingTransportToken = NULL;
  return EFI_SUCCESS;
}

/**
  Build and initialize a complete transport graph before publishing its token.

  @param[in,out] Private  Provider context with TransportBusy set and no ready token.

  @retval EFI_SUCCESS  The initialized token and capabilities are published.
  @retval Other        Initialization or cleanup failed; no ready token is cached.
**/
STATIC
EFI_STATUS
PeiIpmiInitializeTransport (
  IN OUT PEI_IPMI_PPI_INTERNAL  *Private
  )
{
  EFI_STATUS                                    Status;
  EFI_STATUS                                    CleanupStatus;
  MANAGEABILITY_TRANSPORT_CAPABILITY            Capability;
  UINT32                                        MaximumPayload;
  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS     AdditionalStatus;
  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  HardwareInformation;

  HardwareInformation.Pointer = NULL;
  AdditionalStatus            = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NOT_AVAILABLE;

  Status = PeiIpmiReleasePendingTransport (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HelperAcquireManageabilityTransport (
             &gManageabilityProtocolIpmiGuid,
             &Private->PendingTransportToken
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to acquire IPMI transport - %r\n", __func__, Status));
    goto Exit;
  }

  Status = GetTransportCapability (Private->PendingTransportToken, &Capability);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get transport capability - %r\n", __func__, Status));
    goto Exit;
  }

  MaximumPayload = MANAGEABILITY_TRANSPORT_PAYLOAD_SIZE_FROM_CAPABILITY (Capability);
  if (MaximumPayload != (1 << MANAGEABILITY_TRANSPORT_CAPABILITY_MAXIMUM_PAYLOAD_NOT_AVAILABLE)) {
    MaximumPayload -= 1;
  }

  Status = SetupIpmiTransportHardwareInformation (Private->PendingTransportToken, &HardwareInformation);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to set up transport hardware information - %r\n", __func__, Status));
    goto Exit;
  }

  Status = HelperInitManageabilityTransport (
             Private->PendingTransportToken,
             HardwareInformation,
             &AdditionalStatus
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to initialize IPMI transport - %r (0x%x)\n", __func__, Status, AdditionalStatus));
    goto Exit;
  }

  Private->TransportCapability     = Capability;
  Private->TransportMaximumPayload = MaximumPayload;
  Private->TransportToken          = Private->PendingTransportToken;
  Private->PendingTransportToken   = NULL;

Exit:
  // The KCS backend copies this configuration by value; it does not own it.
  if (HardwareInformation.Pointer != NULL) {
    FreePool (HardwareInformation.Pointer);
  }

  if (EFI_ERROR (Status)) {
    CleanupStatus = PeiIpmiReleasePendingTransport (Private);
    if (EFI_ERROR (CleanupStatus)) {
      return CleanupStatus;
    }
  }

  return Status;
}

/**
  Check memory generation before any cached transport graph is dereferenced.

  Both the notification and every submit use this path. A generation transition
  abandons pointers without releasing them; the backend independently abandons
  its HOB ownership on its next lookup. Ordinary commands only locate the memory
  PPI and reuse the initialized token.

  @param[in,out] Private  Migrated provider context, not a saved image global.

  @retval EFI_SUCCESS    A current-generation transport is ready.
  @retval EFI_NOT_READY  Provider is disabled, busy, or memory state regressed.
  @retval Other          Memory lookup, initialization, or cleanup failed.
**/
STATIC
EFI_STATUS
PeiIpmiEnsureTransportReady (
  IN OUT PEI_IPMI_PPI_INTERNAL  *Private
  )
{
  EFI_STATUS  Status;
  VOID        *MemoryDiscovered;

  if (!Private->Enabled || Private->TransportBusy) {
    DEBUG ((DEBUG_ERROR, "%a: IPMI provider is disabled or busy.\n", __func__));
    return EFI_NOT_READY;
  }

  Status = PeiServicesLocatePpi (&gEfiPeiMemoryDiscoveredPpiGuid, 0, NULL, &MemoryDiscovered);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "%a: Memory Discovered lookup failed - %r\n", __func__, Status));
    return Status;
  }

  if (!EFI_ERROR (Status) && !Private->PermanentMemory) {
    // TemporaryRamDone may already have run. Do not follow either old graph.
    Private->TransportToken          = NULL;
    Private->PendingTransportToken   = NULL;
    Private->TransportCapability     = 0;
    Private->TransportMaximumPayload = 0;
    Private->PermanentMemory         = TRUE;
  } else if ((Status == EFI_NOT_FOUND) && Private->PermanentMemory) {
    DEBUG ((DEBUG_ERROR, "%a: Permanent memory signal is no longer available.\n", __func__));
    return EFI_NOT_READY;
  }

  if (Private->TransportToken != NULL) {
    return EFI_SUCCESS;
  }

  Private->TransportBusy = TRUE;
  Status                 = PeiIpmiInitializeTransport (Private);
  Private->TransportBusy = FALSE;
  return Status;
}

/**
  This service enables submitting commands via Ipmi.

  @param[in]         This              This point for PEI_IPMI_PPI structure.
  @param[in]         NetFunction       Net function of the command.
  @param[in]         Command           IPMI Command.
  @param[in]         RequestData       Command Request Data.
  @param[in]         RequestDataSize   Size of Command Request Data.
  @param[out]        ResponseData      Command Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize  Size of Command Response Data.

  @retval EFI_SUCCESS            The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_NOT_FOUND          The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_NOT_READY          Ipmi Device is not ready for Ipmi command access.
  @retval EFI_DEVICE_ERROR       Ipmi Device hardware error.
  @retval EFI_TIMEOUT            The command time out.
  @retval EFI_UNSUPPORTED        The command was not successfully sent to the device.
  @retval EFI_OUT_OF_RESOURCES   The resource allocation is out of resource or data size error.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval Others                 The transport could not be rebuilt after the PEI memory
                                 transition; see PeiIpmiEnsureTransportReady ().
**/
EFI_STATUS
EFIAPI
PeiIpmiSubmitCommand (
  IN     PEI_IPMI_PPI  *This,
  IN     UINT8         NetFunction,
  IN     UINT8         Command,
  IN     UINT8         *RequestData,
  IN     UINT32        RequestDataSize,
  OUT    UINT8         *ResponseData,
  IN OUT UINT32        *ResponseDataSize
  )
{
  EFI_STATUS             Status;
  PEI_IPMI_PPI_INTERNAL  *PeiIpmiPpiinternal;

  if (This == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid IPMI PPI instance.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  PeiIpmiPpiinternal = MANAGEABILITY_IPMI_PPI_INTERNAL_FROM_LINK (This);
  Status             = PeiIpmiEnsureTransportReady (PeiIpmiPpiinternal);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PeiIpmiPpiinternal->TransportBusy = TRUE;
  Status                            = CommonIpmiSubmitCommand (
                                        PeiIpmiPpiinternal->TransportToken,
                                        NetFunction,
                                        Command,
                                        RequestData,
                                        RequestDataSize,
                                        ResponseData,
                                        ResponseDataSize
                                        );
  PeiIpmiPpiinternal->TransportBusy = FALSE;
  return Status;
}

/**
  Rebind the IPMI command callback and reconstruct the transport after migration.

  @param[in] PeiServices       PEI services table.
  @param[in] NotifyDescriptor  Embedded descriptor rebased by PEI Core.
  @param[in] Ppi               Unused signal PPI interface (may be NULL).

  @retval EFI_SUCCESS  Transport ready, or the descriptor is intentionally inert.
  @retval Other        Recovery failed; the next submit can retry.
**/
EFI_STATUS
EFIAPI
PeiIpmiMemoryDiscovered (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS             Status;
  PEI_IPMI_PPI_INTERNAL  *Private;

  Private = MANAGEABILITY_IPMI_PPI_INTERNAL_FROM_NOTIFY (NotifyDescriptor);
  if (!Private->Enabled) {
    // Registration can call us immediately. Failed publication also leaves
    // this registered descriptor alive but inert: PEI has no unregister service.
    return EFI_SUCCESS;
  }

  // PEI Core rebases notification callbacks, not methods inside heap PPIs.
  Private->PeiIpmiPpi.IpmiSubmitCommand = PeiIpmiSubmitCommand;

  Status = PeiIpmiEnsureTransportReady (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: IPMI memory recovery failed - %r\n", __func__, Status));
  }

  return Status;
}

/**
  Query the BMC Get Device ID command and log the result.

  @param[in]  TransportToken  The transport token used to send the command.

  @retval EFI_SUCCESS  The Device ID was retrieved and logged.
  @retval EFI_DEVICE_ERROR  The response is truncated or reports a command error.
  @retval Others       The Get Device ID command failed.

**/
STATIC
EFI_STATUS
PrintDeviceId (
  IN MANAGEABILITY_TRANSPORT_TOKEN  *TransportToken
  )
{
  EFI_STATUS                   Status;
  IPMI_GET_DEVICE_ID_RESPONSE  DeviceIdResponse;
  UINT32                       DataSize;

  ZeroMem (&DeviceIdResponse, sizeof (DeviceIdResponse));
  DataSize = sizeof (DeviceIdResponse);
  Status   = CommonIpmiSubmitCommand (
               TransportToken,
               IPMI_NETFN_APP,
               IPMI_APP_GET_DEVICE_ID,
               NULL,
               0,
               (UINT8 *)&DeviceIdResponse,
               &DataSize
               );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: IPMI Get Device ID failed - %r\n", __func__, Status));
    return Status;
  }

  // Auxiliary firmware revision information is optional in Get Device ID.
  if ((DataSize < OFFSET_OF (IPMI_GET_DEVICE_ID_RESPONSE, AuxFirmwareRevInfo)) ||
      (DeviceIdResponse.CompletionCode != IPMI_COMP_CODE_NORMAL))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid Get Device ID response (size %u, completion 0x%02x).\n",
      __func__,
      DataSize,
      DeviceIdResponse.CompletionCode
      ));
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a IPMI BMC Device ID: 0x%02X, firmware version: %d.%02X UpdateMode:%x\n",
    __func__,
    DeviceIdResponse.DeviceId,
    DeviceIdResponse.FirmwareRev1.Bits.MajorFirmwareRev,
    DeviceIdResponse.MinorFirmwareRev,
    DeviceIdResponse.FirmwareRev1.Bits.UpdateMode
    ));

  return EFI_SUCCESS;
}

/**
  The entry point of the Ipmi PPI PEIM.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS   Indicates that Ipmi initialization completed successfully.
  @retval Others        Indicates that Ipmi initialization could not complete successfully.
**/
EFI_STATUS
EFIAPI
PeiIpmiEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS             Status;
  EFI_STATUS             CleanupStatus;
  PEI_IPMI_PPI_INTERNAL  *PeiIpmiPpiinternal;

  Status = IpmiPlatformInitialize ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: IPMI platform initialization failed - %r\n", __func__, Status));
    return Status;
  }

  //
  // Allocate PEI_IPMI_PPI_INTERNAL memory for the dynamic variables,
  // as the global variable in PEI module is read only.
  //
  PeiIpmiPpiinternal = (PEI_IPMI_PPI_INTERNAL *)AllocateZeroPool (sizeof (PEI_IPMI_PPI_INTERNAL));
  if (PeiIpmiPpiinternal == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Not enough memory for PEI_IPMI_PPI_INTERNAL.\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  PeiIpmiPpiinternal->Signature                    = MANAGEABILITY_IPMI_PPI_INTERNAL_SIGNATURE;
  PeiIpmiPpiinternal->PeiIpmiPpi.IpmiSubmitCommand = PeiIpmiSubmitCommand;

  PeiIpmiPpiinternal->PpiDescriptor.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  PeiIpmiPpiinternal->PpiDescriptor.Guid  = &gPeiIpmiPpiGuid;
  PeiIpmiPpiinternal->PpiDescriptor.Ppi   = &PeiIpmiPpiinternal->PeiIpmiPpi;

  PeiIpmiPpiinternal->MemoryDiscoveredNotify.Flags  = EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  PeiIpmiPpiinternal->MemoryDiscoveredNotify.Guid   = &gEfiPeiMemoryDiscoveredPpiGuid;
  PeiIpmiPpiinternal->MemoryDiscoveredNotify.Notify = PeiIpmiMemoryDiscovered;

  // Register one inert descriptor before acquiring a session. Registration
  // failure therefore cannot leave an invisible active session.
  Status = PeiServicesNotifyPpi (&PeiIpmiPpiinternal->MemoryDiscoveredNotify);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to register Memory Discovered callback - %r\n", __func__, Status));
    FreePool (PeiIpmiPpiinternal);
    return Status;
  }

  PeiIpmiPpiinternal->Enabled = TRUE;
  Status                      = PeiIpmiEnsureTransportReady (PeiIpmiPpiinternal);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Status = PrintDeviceId (PeiIpmiPpiinternal->TransportToken);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Install IPMI PPI.
  //
  Status = PeiServicesInstallPpi (&PeiIpmiPpiinternal->PpiDescriptor);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install IPMI PPI - %r\n", __func__, Status));
    goto Error;
  }

  return EFI_SUCCESS;

Error:
  PeiIpmiPpiinternal->Enabled = FALSE;
  if (PeiIpmiPpiinternal->TransportToken != NULL) {
    PeiIpmiPpiinternal->PendingTransportToken = PeiIpmiPpiinternal->TransportToken;
    PeiIpmiPpiinternal->TransportToken        = NULL;
  }

  PeiIpmiPpiinternal->TransportCapability     = 0;
  PeiIpmiPpiinternal->TransportMaximumPayload = 0;
  PeiIpmiPpiinternal->TransportBusy           = TRUE;
  CleanupStatus                               = PeiIpmiReleasePendingTransport (PeiIpmiPpiinternal);
  PeiIpmiPpiinternal->TransportBusy           = FALSE;
  if (EFI_ERROR (CleanupStatus)) {
    Status = CleanupStatus;
  }

  // Never free a context containing a registered notification descriptor.
  // It remains disabled and cannot acquire a session after failed entry.
  return Status;
}
