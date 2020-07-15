/** @file

  This driver produces Extended SCSI Pass Thru Protocol instances for
  virtio-scsi devices.

  The implementation is basic:

  - No hotplug / hot-unplug.

  - Although EFI_EXT_SCSI_PASS_THRU_PROTOCOL.PassThru() could be a good match
    for multiple in-flight virtio-scsi requests, we stick to synchronous
    requests for now.

  - Timeouts are not supported for EFI_EXT_SCSI_PASS_THRU_PROTOCOL.PassThru().

  - Only one channel is supported. (At the time of this writing, host-side
    virtio-scsi supports a single channel too.)

  - Only one request queue is used (for the one synchronous request).

  - The ResetChannel() and ResetTargetLun() functions of
    EFI_EXT_SCSI_PASS_THRU_PROTOCOL are not supported (which is allowed by the
    UEFI 2.3.1 Errata C specification), although
    VIRTIO_SCSI_T_TMF_LOGICAL_UNIT_RESET could be a good match. That would
    however require client code for the control queue, which is deemed
    unreasonable for now.

  Copyright (C) 2012, Red Hat, Inc.
  Copyright (c) 2012 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Inc, All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/VirtioScsi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtioLib.h>

#include "VirtioScsi.h"

/**

  Convenience macros to read and write configuration elements of the
  virtio-scsi VirtIo device.

  The following macros make it possible to specify only the "core parameters"
  for such accesses and to derive the rest. By the time VIRTIO_CFG_WRITE()
  returns, the transaction will have been completed.

  @param[in] Dev       Pointer to the VSCSI_DEV structure.

  @param[in] Field     A field name from VSCSI_HDR, identifying the virtio-scsi
                       configuration item to access.

  @param[in] Value     (VIRTIO_CFG_WRITE() only.) The value to write to the
                       selected configuration item.

  @param[out] Pointer  (VIRTIO_CFG_READ() only.) The object to receive the
                       value read from the configuration item. Its type must be
                       one of UINT8, UINT16, UINT32, UINT64.


  @return  Status codes returned by Virtio->WriteDevice() / Virtio->ReadDevice().

**/

#define VIRTIO_CFG_WRITE(Dev, Field, Value)  ((Dev)->VirtIo->WriteDevice (  \
                                                (Dev)->VirtIo,              \
                                                OFFSET_OF_VSCSI (Field),    \
                                                SIZE_OF_VSCSI (Field),      \
                                                (Value)                     \
                                                ))

#define VIRTIO_CFG_READ(Dev, Field, Pointer) ((Dev)->VirtIo->ReadDevice (   \
                                                (Dev)->VirtIo,              \
                                                OFFSET_OF_VSCSI (Field),    \
                                                SIZE_OF_VSCSI (Field),      \
                                                sizeof *(Pointer),          \
                                                (Pointer)                   \
                                                ))


//
// UEFI Spec 2.3.1 + Errata C, 14.7 Extended SCSI Pass Thru Protocol specifies
// the PassThru() interface. Beside returning a status code, the function must
// set some fields in the EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET in/out
// parameter on return. The following is a full list of those fields, for
// easier validation of PopulateRequest(), ParseResponse(), and
// ReportHostAdapterError() below.
//
// - InTransferLength
// - OutTransferLength
// - HostAdapterStatus
// - TargetStatus
// - SenseDataLength
// - SenseData
//
// On any return from the PassThru() interface, these fields must be set,
// except if the returned status code is explicitly exempt. (Actually the
// implementation here conservatively sets these fields even in case not all
// of them would be required by the specification.)
//

/**

  Populate a virtio-scsi request from the Extended SCSI Pass Thru Protocol
  packet.

  The caller is responsible for pre-zeroing the virtio-scsi request. The
  Extended SCSI Pass Thru Protocol packet is modified, to be forwarded outwards
  by VirtioScsiPassThru(), if invalid or unsupported parameters are detected.

  @param[in] Dev          The virtio-scsi host device the packet targets.

  @param[in] Target       The SCSI target controlled by the virtio-scsi host
                          device.

  @param[in] Lun          The Logical Unit Number under the SCSI target.

  @param[in out] Packet   The Extended SCSI Pass Thru Protocol packet the
                          function translates to a virtio-scsi request. On
                          failure this parameter relays error contents.

  @param[out]    Request  The pre-zeroed virtio-scsi request to populate. This
                          parameter is volatile-qualified because we expect the
                          caller to append it to a virtio ring, thus
                          assignments to Request must be visible when the
                          function returns.


  @retval EFI_SUCCESS  The Extended SCSI Pass Thru Protocol packet was valid,
                       Request has been populated.

  @return              Otherwise, invalid or unsupported parameters were
                       detected. Status codes are meant for direct forwarding
                       by the EFI_EXT_SCSI_PASS_THRU_PROTOCOL.PassThru()
                       implementation.

**/
STATIC
EFI_STATUS
EFIAPI
PopulateRequest (
  IN     CONST    VSCSI_DEV                                   *Dev,
  IN              UINT16                                      Target,
  IN              UINT64                                      Lun,
  IN OUT          EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet,
  OUT    volatile VIRTIO_SCSI_REQ                             *Request
  )
{
  UINTN Idx;

  if (
      //
      // bidirectional transfer was requested, but the host doesn't support it
      //
      (Packet->InTransferLength > 0 && Packet->OutTransferLength > 0 &&
       !Dev->InOutSupported) ||

      //
      // a target / LUN was addressed that's impossible to encode for the host
      //
      Target > 0xFF || Lun >= 0x4000 ||

      //
      // Command Descriptor Block bigger than VIRTIO_SCSI_CDB_SIZE
      //
      Packet->CdbLength > VIRTIO_SCSI_CDB_SIZE ||

      //
      // From virtio-0.9.5, 2.3.2 Descriptor Table:
      // "no descriptor chain may be more than 2^32 bytes long in total".
      //
      (UINT64) Packet->InTransferLength + Packet->OutTransferLength > SIZE_1GB
      ) {

    //
    // this error code doesn't require updates to the Packet output fields
    //
    return EFI_UNSUPPORTED;
  }

  if (
      //
      // addressed invalid device
      //
      Target > Dev->MaxTarget || Lun > Dev->MaxLun ||

      //
      // invalid direction (there doesn't seem to be a macro for the "no data
      // transferred" "direction", eg. for TEST UNIT READY)
      //
      Packet->DataDirection > EFI_EXT_SCSI_DATA_DIRECTION_BIDIRECTIONAL ||

      //
      // trying to receive, but destination pointer is NULL, or contradicting
      // transfer direction
      //
      (Packet->InTransferLength > 0 &&
       (Packet->InDataBuffer == NULL ||
        Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_WRITE
        )
       ) ||

      //
      // trying to send, but source pointer is NULL, or contradicting transfer
      // direction
      //
      (Packet->OutTransferLength > 0 &&
       (Packet->OutDataBuffer == NULL ||
        Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ
        )
       )
      ) {

    //
    // this error code doesn't require updates to the Packet output fields
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Catch oversized requests eagerly. If this condition evaluates to false,
  // then the combined size of a bidirectional request will not exceed the
  // virtio-scsi device's transfer limit either.
  //
  if (ALIGN_VALUE (Packet->OutTransferLength, 512) / 512
        > Dev->MaxSectors / 2 ||
      ALIGN_VALUE (Packet->InTransferLength,  512) / 512
        > Dev->MaxSectors / 2) {
    Packet->InTransferLength  = (Dev->MaxSectors / 2) * 512;
    Packet->OutTransferLength = (Dev->MaxSectors / 2) * 512;
    Packet->HostAdapterStatus =
                        EFI_EXT_SCSI_STATUS_HOST_ADAPTER_DATA_OVERRUN_UNDERRUN;
    Packet->TargetStatus      = EFI_EXT_SCSI_STATUS_TARGET_GOOD;
    Packet->SenseDataLength   = 0;
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // target & LUN encoding: see virtio-0.9.5, Appendix I: SCSI Host Device,
  // Device Operation: request queues
  //
  Request->Lun[0] = 1;
  Request->Lun[1] = (UINT8) Target;
  Request->Lun[2] = (UINT8) (((UINT32)Lun >> 8) | 0x40);
  Request->Lun[3] = (UINT8) Lun;

  //
  // CopyMem() would cast away the "volatile" qualifier before access, which is
  // undefined behavior (ISO C99 6.7.3p5)
  //
  for (Idx = 0; Idx < Packet->CdbLength; ++Idx) {
    Request->Cdb[Idx] = ((UINT8 *) Packet->Cdb)[Idx];
  }

  return EFI_SUCCESS;
}


/**

  Parse the virtio-scsi device's response, translate it to an EFI status code,
  and update the Extended SCSI Pass Thru Protocol packet, to be returned by
  the EFI_EXT_SCSI_PASS_THRU_PROTOCOL.PassThru() implementation.

  @param[in out] Packet  The Extended SCSI Pass Thru Protocol packet that has
                         been translated to a virtio-scsi request with
                         PopulateRequest(), and processed by the host. On
                         output this parameter is updated with response or
                         error contents.

  @param[in] Response    The virtio-scsi response structure to parse. We expect
                         it to come from a virtio ring, thus it is qualified
                         volatile.


  @return  PassThru() status codes mandated by UEFI Spec 2.3.1 + Errata C, 14.7
           Extended SCSI Pass Thru Protocol.

**/
STATIC
EFI_STATUS
EFIAPI
ParseResponse (
  IN OUT                EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet,
  IN     CONST volatile VIRTIO_SCSI_RESP                           *Response
  )
{
  UINTN ResponseSenseLen;
  UINTN Idx;

  //
  // return sense data (length and contents) in all cases, truncated if needed
  //
  ResponseSenseLen = MIN (Response->SenseLen, VIRTIO_SCSI_SENSE_SIZE);
  if (Packet->SenseDataLength > ResponseSenseLen) {
    Packet->SenseDataLength = (UINT8) ResponseSenseLen;
  }
  for (Idx = 0; Idx < Packet->SenseDataLength; ++Idx) {
    ((UINT8 *) Packet->SenseData)[Idx] = Response->Sense[Idx];
  }

  //
  // Report actual transfer lengths. The logic below covers all three
  // DataDirections (read, write, bidirectional).
  //
  // -+- @ 0
  //  |
  //  | write                                       ^  @ Residual (unprocessed)
  //  |                                             |
  // -+- @ OutTransferLength                       -+- @ InTransferLength
  //  |                                             |
  //  | read                                        |
  //  |                                             |
  //  V  @ OutTransferLength + InTransferLength    -+- @ 0
  //
  if (Response->Residual <= Packet->InTransferLength) {
    Packet->InTransferLength  -= Response->Residual;
  }
  else {
    Packet->OutTransferLength -= Response->Residual - Packet->InTransferLength;
    Packet->InTransferLength   = 0;
  }

  //
  // report target status in all cases
  //
  Packet->TargetStatus = Response->Status;

  //
  // host adapter status and function return value depend on virtio-scsi
  // response code
  //
  switch (Response->Response) {
  case VIRTIO_SCSI_S_OK:
    Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_OK;
    return EFI_SUCCESS;

  case VIRTIO_SCSI_S_OVERRUN:
    Packet->HostAdapterStatus =
                        EFI_EXT_SCSI_STATUS_HOST_ADAPTER_DATA_OVERRUN_UNDERRUN;
    break;

  case VIRTIO_SCSI_S_BAD_TARGET:
    //
    // This is non-intuitive but explicitly required by the
    // EFI_EXT_SCSI_PASS_THRU_PROTOCOL.PassThru() specification for
    // disconnected (but otherwise valid) target / LUN addresses.
    //
    Packet->HostAdapterStatus =
                              EFI_EXT_SCSI_STATUS_HOST_ADAPTER_TIMEOUT_COMMAND;
    return EFI_TIMEOUT;

  case VIRTIO_SCSI_S_RESET:
    Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_BUS_RESET;
    break;

  case VIRTIO_SCSI_S_BUSY:
    Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_OK;
    return EFI_NOT_READY;

  //
  // Lump together the rest. The mapping for VIRTIO_SCSI_S_ABORTED is
  // intentional as well, not an oversight.
  //
  case VIRTIO_SCSI_S_ABORTED:
  case VIRTIO_SCSI_S_TRANSPORT_FAILURE:
  case VIRTIO_SCSI_S_TARGET_FAILURE:
  case VIRTIO_SCSI_S_NEXUS_FAILURE:
  case VIRTIO_SCSI_S_FAILURE:
  default:
    Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_OTHER;
  }

  return EFI_DEVICE_ERROR;
}


/**

  The function can be used to create a fake host adapter error.

  When VirtioScsiPassThru() is failed due to some reasons then this function
  can be called to construct a host adapter error.

  @param[out] Packet  The Extended SCSI Pass Thru Protocol packet that the host
                      adapter error shall be placed in.


  @retval EFI_DEVICE_ERROR  The function returns this status code
                            unconditionally, to be propagated by
                            VirtioScsiPassThru().

**/
STATIC
EFI_STATUS
ReportHostAdapterError (
  OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet
  )
{
  Packet->InTransferLength  = 0;
  Packet->OutTransferLength = 0;
  Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_OTHER;
  Packet->TargetStatus      = EFI_EXT_SCSI_STATUS_TARGET_GOOD;
  Packet->SenseDataLength   = 0;
  return EFI_DEVICE_ERROR;
}


//
// The next seven functions implement EFI_EXT_SCSI_PASS_THRU_PROTOCOL
// for the virtio-scsi HBA. Refer to UEFI Spec 2.3.1 + Errata C, sections
// - 14.1 SCSI Driver Model Overview,
// - 14.7 Extended SCSI Pass Thru Protocol.
//

EFI_STATUS
EFIAPI
VirtioScsiPassThru (
  IN     EFI_EXT_SCSI_PASS_THRU_PROTOCOL            *This,
  IN     UINT8                                      *Target,
  IN     UINT64                                     Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet,
  IN     EFI_EVENT                                  Event   OPTIONAL
  )
{
  VSCSI_DEV                 *Dev;
  UINT16                    TargetValue;
  EFI_STATUS                Status;
  volatile VIRTIO_SCSI_REQ  Request;
  volatile VIRTIO_SCSI_RESP *Response;
  VOID                      *ResponseBuffer;
  DESC_INDICES              Indices;
  VOID                      *RequestMapping;
  VOID                      *ResponseMapping;
  VOID                      *InDataMapping;
  VOID                      *OutDataMapping;
  EFI_PHYSICAL_ADDRESS      RequestDeviceAddress;
  EFI_PHYSICAL_ADDRESS      ResponseDeviceAddress;
  EFI_PHYSICAL_ADDRESS      InDataDeviceAddress;
  EFI_PHYSICAL_ADDRESS      OutDataDeviceAddress;
  VOID                      *InDataBuffer;
  UINTN                     InDataNumPages;
  BOOLEAN                   OutDataBufferIsMapped;

  //
  // Set InDataMapping,OutDataMapping,InDataDeviceAddress and OutDataDeviceAddress to
  // suppress incorrect compiler/analyzer warnings.
  //
  InDataMapping        = NULL;
  OutDataMapping       = NULL;
  InDataDeviceAddress  = 0;
  OutDataDeviceAddress = 0;

  ZeroMem ((VOID*) &Request, sizeof (Request));

  Dev = VIRTIO_SCSI_FROM_PASS_THRU (This);
  CopyMem (&TargetValue, Target, sizeof TargetValue);

  InDataBuffer = NULL;
  OutDataBufferIsMapped = FALSE;
  InDataNumPages = 0;

  Status = PopulateRequest (Dev, TargetValue, Lun, Packet, &Request);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Map the virtio-scsi Request header buffer
  //
  Status = VirtioMapAllBytesInSharedBuffer (
             Dev->VirtIo,
             VirtioOperationBusMasterRead,
             (VOID *) &Request,
             sizeof Request,
             &RequestDeviceAddress,
             &RequestMapping);
  if (EFI_ERROR (Status)) {
    return ReportHostAdapterError (Packet);
  }

  //
  // Map the input buffer
  //
  if (Packet->InTransferLength > 0) {
    //
    // Allocate a intermediate input buffer. This is mainly to handle the
    // following case:
    //  * caller submits a bi-directional request
    //  * we perform the request fine
    //  * but we fail to unmap the "InDataMapping"
    //
    // In that case simply returning the EFI_DEVICE_ERROR is not sufficient. In
    // addition to the error code we also need to update Packet fields
    // accordingly so that we report the full loss of the incoming transfer.
    //
    // We allocate a temporary buffer and map it with BusMasterCommonBuffer. If
    // the Virtio request is successful then we copy the data from temporary
    // buffer into Packet->InDataBuffer.
    //
    InDataNumPages = EFI_SIZE_TO_PAGES ((UINTN)Packet->InTransferLength);
    Status = Dev->VirtIo->AllocateSharedPages (
                            Dev->VirtIo,
                            InDataNumPages,
                            &InDataBuffer
                            );
    if (EFI_ERROR (Status)) {
      Status = ReportHostAdapterError (Packet);
      goto UnmapRequestBuffer;
    }

    ZeroMem (InDataBuffer, Packet->InTransferLength);

    Status = VirtioMapAllBytesInSharedBuffer (
               Dev->VirtIo,
               VirtioOperationBusMasterCommonBuffer,
               InDataBuffer,
               Packet->InTransferLength,
               &InDataDeviceAddress,
               &InDataMapping
               );
    if (EFI_ERROR (Status)) {
      Status = ReportHostAdapterError (Packet);
      goto FreeInDataBuffer;
    }
  }

  //
  // Map the output buffer
  //
  if (Packet->OutTransferLength > 0) {
    Status = VirtioMapAllBytesInSharedBuffer (
               Dev->VirtIo,
               VirtioOperationBusMasterRead,
               Packet->OutDataBuffer,
               Packet->OutTransferLength,
               &OutDataDeviceAddress,
               &OutDataMapping
               );
    if (EFI_ERROR (Status)) {
      Status = ReportHostAdapterError (Packet);
      goto UnmapInDataBuffer;
    }

    OutDataBufferIsMapped = TRUE;
  }

  //
  // Response header is bi-direction (we preset with host status and expect
  // the device to update it). Allocate a response buffer which can be mapped
  // to access equally by both processor and device.
  //
  Status = Dev->VirtIo->AllocateSharedPages (
                          Dev->VirtIo,
                          EFI_SIZE_TO_PAGES (sizeof *Response),
                          &ResponseBuffer
                          );
  if (EFI_ERROR (Status)) {
    Status = ReportHostAdapterError (Packet);
    goto UnmapOutDataBuffer;
  }

  Response = ResponseBuffer;

  ZeroMem ((VOID *)Response, sizeof (*Response));

  //
  // preset a host status for ourselves that we do not accept as success
  //
  Response->Response = VIRTIO_SCSI_S_FAILURE;

  //
  // Map the response buffer with BusMasterCommonBuffer so that response
  // buffer can be accessed by both host and device.
  //
  Status = VirtioMapAllBytesInSharedBuffer (
             Dev->VirtIo,
             VirtioOperationBusMasterCommonBuffer,
             ResponseBuffer,
             sizeof (*Response),
             &ResponseDeviceAddress,
             &ResponseMapping
             );
  if (EFI_ERROR (Status)) {
    Status = ReportHostAdapterError (Packet);
    goto FreeResponseBuffer;
  }

  VirtioPrepare (&Dev->Ring, &Indices);

  //
  // ensured by VirtioScsiInit() -- this predicate, in combination with the
  // lock-step progress, ensures we don't have to track free descriptors.
  //
  ASSERT (Dev->Ring.QueueSize >= 4);

  //
  // enqueue Request
  //
  VirtioAppendDesc (
    &Dev->Ring,
    RequestDeviceAddress,
    sizeof Request,
    VRING_DESC_F_NEXT,
    &Indices
    );

  //
  // enqueue "dataout" if any
  //
  if (Packet->OutTransferLength > 0) {
    VirtioAppendDesc (
      &Dev->Ring,
      OutDataDeviceAddress,
      Packet->OutTransferLength,
      VRING_DESC_F_NEXT,
      &Indices
      );
  }

  //
  // enqueue Response, to be written by the host
  //
  VirtioAppendDesc (
    &Dev->Ring,
    ResponseDeviceAddress,
    sizeof *Response,
    VRING_DESC_F_WRITE | (Packet->InTransferLength > 0 ? VRING_DESC_F_NEXT : 0),
    &Indices
    );

  //
  // enqueue "datain" if any, to be written by the host
  //
  if (Packet->InTransferLength > 0) {
    VirtioAppendDesc (
      &Dev->Ring,
      InDataDeviceAddress,
      Packet->InTransferLength,
      VRING_DESC_F_WRITE,
      &Indices
      );
  }

  // If kicking the host fails, we must fake a host adapter error.
  // EFI_NOT_READY would save us the effort, but it would also suggest that the
  // caller retry.
  //
  if (VirtioFlush (Dev->VirtIo, VIRTIO_SCSI_REQUEST_QUEUE, &Dev->Ring,
        &Indices, NULL) != EFI_SUCCESS) {
    Status = ReportHostAdapterError (Packet);
    goto UnmapResponseBuffer;
  }

  Status = ParseResponse (Packet, Response);

  //
  // If virtio request was successful and it was a CPU read request then we
  // have used an intermediate buffer. Copy the data from intermediate buffer
  // to the final buffer.
  //
  if (InDataBuffer != NULL) {
    CopyMem (Packet->InDataBuffer, InDataBuffer, Packet->InTransferLength);
  }

UnmapResponseBuffer:
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, ResponseMapping);

FreeResponseBuffer:
  Dev->VirtIo->FreeSharedPages (
                 Dev->VirtIo,
                 EFI_SIZE_TO_PAGES (sizeof *Response),
                 ResponseBuffer
                 );

UnmapOutDataBuffer:
  if (OutDataBufferIsMapped) {
    Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, OutDataMapping);
  }

UnmapInDataBuffer:
  if (InDataBuffer != NULL) {
    Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, InDataMapping);
  }

FreeInDataBuffer:
  if (InDataBuffer != NULL) {
    Dev->VirtIo->FreeSharedPages (Dev->VirtIo, InDataNumPages, InDataBuffer);
  }

UnmapRequestBuffer:
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, RequestMapping);

  return Status;
}


EFI_STATUS
EFIAPI
VirtioScsiGetNextTargetLun (
  IN     EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN OUT UINT8                           **TargetPointer,
  IN OUT UINT64                          *Lun
  )
{
  UINT8     *Target;
  UINTN     Idx;
  UINT16    LastTarget;
  VSCSI_DEV *Dev;

  //
  // the TargetPointer input parameter is unnecessarily a pointer-to-pointer
  //
  Target = *TargetPointer;

  //
  // Search for first non-0xFF byte. If not found, return first target & LUN.
  //
  for (Idx = 0; Idx < TARGET_MAX_BYTES && Target[Idx] == 0xFF; ++Idx)
    ;
  if (Idx == TARGET_MAX_BYTES) {
    SetMem (Target, TARGET_MAX_BYTES, 0x00);
    *Lun = 0;
    return EFI_SUCCESS;
  }

  //
  // see the TARGET_MAX_BYTES check in "VirtioScsi.h"
  //
  CopyMem (&LastTarget, Target, sizeof LastTarget);

  //
  // increment (target, LUN) pair if valid on input
  //
  Dev = VIRTIO_SCSI_FROM_PASS_THRU (This);
  if (LastTarget > Dev->MaxTarget || *Lun > Dev->MaxLun) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Lun < Dev->MaxLun) {
    ++*Lun;
    return EFI_SUCCESS;
  }

  if (LastTarget < Dev->MaxTarget) {
    *Lun = 0;
    ++LastTarget;
    CopyMem (Target, &LastTarget, sizeof LastTarget);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}


EFI_STATUS
EFIAPI
VirtioScsiBuildDevicePath (
  IN     EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN     UINT8                           *Target,
  IN     UINT64                          Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL        **DevicePath
  )
{
  UINT16           TargetValue;
  VSCSI_DEV        *Dev;
  SCSI_DEVICE_PATH *ScsiDevicePath;

  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&TargetValue, Target, sizeof TargetValue);
  Dev = VIRTIO_SCSI_FROM_PASS_THRU (This);
  if (TargetValue > Dev->MaxTarget || Lun > Dev->MaxLun || Lun > 0xFFFF) {
    return EFI_NOT_FOUND;
  }

  ScsiDevicePath = AllocatePool (sizeof *ScsiDevicePath);
  if (ScsiDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ScsiDevicePath->Header.Type      = MESSAGING_DEVICE_PATH;
  ScsiDevicePath->Header.SubType   = MSG_SCSI_DP;
  ScsiDevicePath->Header.Length[0] = (UINT8)  sizeof *ScsiDevicePath;
  ScsiDevicePath->Header.Length[1] = (UINT8) (sizeof *ScsiDevicePath >> 8);
  ScsiDevicePath->Pun              = TargetValue;
  ScsiDevicePath->Lun              = (UINT16) Lun;

  *DevicePath = &ScsiDevicePath->Header;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
VirtioScsiGetTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN  EFI_DEVICE_PATH_PROTOCOL        *DevicePath,
  OUT UINT8                           **TargetPointer,
  OUT UINT64                          *Lun
  )
{
  SCSI_DEVICE_PATH *ScsiDevicePath;
  VSCSI_DEV        *Dev;
  UINT8            *Target;

  if (DevicePath == NULL || TargetPointer == NULL || *TargetPointer == NULL ||
      Lun == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DevicePath->Type    != MESSAGING_DEVICE_PATH ||
      DevicePath->SubType != MSG_SCSI_DP) {
    return EFI_UNSUPPORTED;
  }

  ScsiDevicePath = (SCSI_DEVICE_PATH *) DevicePath;
  Dev = VIRTIO_SCSI_FROM_PASS_THRU (This);
  if (ScsiDevicePath->Pun > Dev->MaxTarget ||
      ScsiDevicePath->Lun > Dev->MaxLun) {
    return EFI_NOT_FOUND;
  }

  //
  // a) the TargetPointer input parameter is unnecessarily a pointer-to-pointer
  // b) see the TARGET_MAX_BYTES check in "VirtioScsi.h"
  // c) ScsiDevicePath->Pun is an UINT16
  //
  Target = *TargetPointer;
  CopyMem (Target, &ScsiDevicePath->Pun, 2);
  SetMem (Target + 2, TARGET_MAX_BYTES - 2, 0x00);

  *Lun = ScsiDevicePath->Lun;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
VirtioScsiResetChannel (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This
  )
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
VirtioScsiResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN UINT8                           *Target,
  IN UINT64                          Lun
  )
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
VirtioScsiGetNextTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN OUT UINT8                       **TargetPointer
  )
{
  UINT8     *Target;
  UINTN     Idx;
  UINT16    LastTarget;
  VSCSI_DEV *Dev;

  //
  // the TargetPointer input parameter is unnecessarily a pointer-to-pointer
  //
  Target = *TargetPointer;

  //
  // Search for first non-0xFF byte. If not found, return first target.
  //
  for (Idx = 0; Idx < TARGET_MAX_BYTES && Target[Idx] == 0xFF; ++Idx)
    ;
  if (Idx == TARGET_MAX_BYTES) {
    SetMem (Target, TARGET_MAX_BYTES, 0x00);
    return EFI_SUCCESS;
  }

  //
  // see the TARGET_MAX_BYTES check in "VirtioScsi.h"
  //
  CopyMem (&LastTarget, Target, sizeof LastTarget);

  //
  // increment target if valid on input
  //
  Dev = VIRTIO_SCSI_FROM_PASS_THRU (This);
  if (LastTarget > Dev->MaxTarget) {
    return EFI_INVALID_PARAMETER;
  }

  if (LastTarget < Dev->MaxTarget) {
    ++LastTarget;
    CopyMem (Target, &LastTarget, sizeof LastTarget);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}


STATIC
EFI_STATUS
EFIAPI
VirtioScsiInit (
  IN OUT VSCSI_DEV *Dev
  )
{
  UINT8      NextDevStat;
  EFI_STATUS Status;
  UINT64     RingBaseShift;
  UINT64     Features;
  UINT16     MaxChannel; // for validation only
  UINT32     NumQueues;  // for validation only
  UINT16     QueueSize;

  //
  // Execute virtio-0.9.5, 2.2.1 Device Initialization Sequence.
  //
  NextDevStat = 0;             // step 1 -- reset device
  Status = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  NextDevStat |= VSTAT_ACK;    // step 2 -- acknowledge device presence
  Status = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  NextDevStat |= VSTAT_DRIVER; // step 3 -- we know how to drive it
  Status = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // Set Page Size - MMIO VirtIo Specific
  //
  Status = Dev->VirtIo->SetPageSize (Dev->VirtIo, EFI_PAGE_SIZE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // step 4a -- retrieve and validate features
  //
  Status = Dev->VirtIo->GetDeviceFeatures (Dev->VirtIo, &Features);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  Dev->InOutSupported = (BOOLEAN) ((Features & VIRTIO_SCSI_F_INOUT) != 0);

  Status = VIRTIO_CFG_READ (Dev, MaxChannel, &MaxChannel);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if (MaxChannel != 0) {
    //
    // this driver is for a single-channel virtio-scsi HBA
    //
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  Status = VIRTIO_CFG_READ (Dev, NumQueues, &NumQueues);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if (NumQueues < 1) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  Status = VIRTIO_CFG_READ (Dev, MaxTarget, &Dev->MaxTarget);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if (Dev->MaxTarget > PcdGet16 (PcdVirtioScsiMaxTargetLimit)) {
    Dev->MaxTarget = PcdGet16 (PcdVirtioScsiMaxTargetLimit);
  }

  Status = VIRTIO_CFG_READ (Dev, MaxLun, &Dev->MaxLun);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if (Dev->MaxLun > PcdGet32 (PcdVirtioScsiMaxLunLimit)) {
    Dev->MaxLun = PcdGet32 (PcdVirtioScsiMaxLunLimit);
  }

  Status = VIRTIO_CFG_READ (Dev, MaxSectors, &Dev->MaxSectors);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if (Dev->MaxSectors < 2) {
    //
    // We must be able to halve it for bidirectional transfers
    // (see EFI_BAD_BUFFER_SIZE in PopulateRequest()).
    //
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  Features &= VIRTIO_SCSI_F_INOUT | VIRTIO_F_VERSION_1 |
              VIRTIO_F_IOMMU_PLATFORM;

  //
  // In virtio-1.0, feature negotiation is expected to complete before queue
  // discovery, and the device can also reject the selected set of features.
  //
  if (Dev->VirtIo->Revision >= VIRTIO_SPEC_REVISION (1, 0, 0)) {
    Status = Virtio10WriteFeatures (Dev->VirtIo, Features, &NextDevStat);
    if (EFI_ERROR (Status)) {
      goto Failed;
    }
  }

  //
  // step 4b -- allocate request virtqueue
  //
  Status = Dev->VirtIo->SetQueueSel (Dev->VirtIo, VIRTIO_SCSI_REQUEST_QUEUE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  Status = Dev->VirtIo->GetQueueNumMax (Dev->VirtIo, &QueueSize);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  //
  // VirtioScsiPassThru() uses at most four descriptors
  //
  if (QueueSize < 4) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  Status = VirtioRingInit (Dev->VirtIo, QueueSize, &Dev->Ring);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // If anything fails from here on, we must release the ring resources
  //
  Status = VirtioRingMap (
             Dev->VirtIo,
             &Dev->Ring,
             &RingBaseShift,
             &Dev->RingMap
             );
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  //
  // Additional steps for MMIO: align the queue appropriately, and set the
  // size. If anything fails from here on, we must unmap the ring resources.
  //
  Status = Dev->VirtIo->SetQueueNum (Dev->VirtIo, QueueSize);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  Status = Dev->VirtIo->SetQueueAlign (Dev->VirtIo, EFI_PAGE_SIZE);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  //
  // step 4c -- Report GPFN (guest-physical frame number) of queue.
  //
  Status = Dev->VirtIo->SetQueueAddress (
                          Dev->VirtIo,
                          &Dev->Ring,
                          RingBaseShift
                          );
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  //
  // step 5 -- Report understood features and guest-tuneables.
  //
  if (Dev->VirtIo->Revision < VIRTIO_SPEC_REVISION (1, 0, 0)) {
    Features &= ~(UINT64)(VIRTIO_F_VERSION_1 | VIRTIO_F_IOMMU_PLATFORM);
    Status = Dev->VirtIo->SetGuestFeatures (Dev->VirtIo, Features);
    if (EFI_ERROR (Status)) {
      goto UnmapQueue;
    }
  }

  //
  // We expect these maximum sizes from the host. Since they are
  // guest-negotiable, ask for them rather than just checking them.
  //
  Status = VIRTIO_CFG_WRITE (Dev, CdbSize, VIRTIO_SCSI_CDB_SIZE);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }
  Status = VIRTIO_CFG_WRITE (Dev, SenseSize, VIRTIO_SCSI_SENSE_SIZE);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  //
  // step 6 -- initialization complete
  //
  NextDevStat |= VSTAT_DRIVER_OK;
  Status = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  //
  // populate the exported interface's attributes
  //
  Dev->PassThru.Mode             = &Dev->PassThruMode;
  Dev->PassThru.PassThru         = &VirtioScsiPassThru;
  Dev->PassThru.GetNextTargetLun = &VirtioScsiGetNextTargetLun;
  Dev->PassThru.BuildDevicePath  = &VirtioScsiBuildDevicePath;
  Dev->PassThru.GetTargetLun     = &VirtioScsiGetTargetLun;
  Dev->PassThru.ResetChannel     = &VirtioScsiResetChannel;
  Dev->PassThru.ResetTargetLun   = &VirtioScsiResetTargetLun;
  Dev->PassThru.GetNextTarget    = &VirtioScsiGetNextTarget;

  //
  // AdapterId is a target for which no handle will be created during bus scan.
  // Prevent any conflict with real devices.
  //
  Dev->PassThruMode.AdapterId = 0xFFFFFFFF;

  //
  // Set both physical and logical attributes for non-RAID SCSI channel. See
  // Driver Writer's Guide for UEFI 2.3.1 v1.01, 20.1.5 Implementing Extended
  // SCSI Pass Thru Protocol.
  //
  Dev->PassThruMode.Attributes = EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL |
                                 EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL;

  //
  // no restriction on transfer buffer alignment
  //
  Dev->PassThruMode.IoAlign = 0;

  return EFI_SUCCESS;

UnmapQueue:
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Dev->RingMap);

ReleaseQueue:
  VirtioRingUninit (Dev->VirtIo, &Dev->Ring);

Failed:
  //
  // Notify the host about our failure to setup: virtio-0.9.5, 2.2.2.1 Device
  // Status. VirtIo access failure here should not mask the original error.
  //
  NextDevStat |= VSTAT_FAILED;
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);

  Dev->InOutSupported = FALSE;
  Dev->MaxTarget      = 0;
  Dev->MaxLun         = 0;
  Dev->MaxSectors     = 0;

  return Status; // reached only via Failed above
}


STATIC
VOID
EFIAPI
VirtioScsiUninit (
  IN OUT VSCSI_DEV *Dev
  )
{
  //
  // Reset the virtual device -- see virtio-0.9.5, 2.2.2.1 Device Status. When
  // VIRTIO_CFG_WRITE() returns, the host will have learned to stay away from
  // the old comms area.
  //
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);

  Dev->InOutSupported = FALSE;
  Dev->MaxTarget      = 0;
  Dev->MaxLun         = 0;
  Dev->MaxSectors     = 0;

  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Dev->RingMap);
  VirtioRingUninit (Dev->VirtIo, &Dev->Ring);

  SetMem (&Dev->PassThru,     sizeof Dev->PassThru,     0x00);
  SetMem (&Dev->PassThruMode, sizeof Dev->PassThruMode, 0x00);
}


//
// Event notification function enqueued by ExitBootServices().
//

STATIC
VOID
EFIAPI
VirtioScsiExitBoot (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
{
  VSCSI_DEV *Dev;

  DEBUG ((DEBUG_VERBOSE, "%a: Context=0x%p\n", __FUNCTION__, Context));
  //
  // Reset the device. This causes the hypervisor to forget about the virtio
  // ring.
  //
  // We allocated said ring in EfiBootServicesData type memory, and code
  // executing after ExitBootServices() is permitted to overwrite it.
  //
  Dev = Context;
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);
}


//
// Probe, start and stop functions of this driver, called by the DXE core for
// specific devices.
//
// The following specifications document these interfaces:
// - Driver Writer's Guide for UEFI 2.3.1 v1.01, 9 Driver Binding Protocol
// - UEFI Spec 2.3.1 + Errata C, 10.1 EFI Driver Binding Protocol
//
// The implementation follows:
// - Driver Writer's Guide for UEFI 2.3.1 v1.01
//   - 5.1.3.4 OpenProtocol() and CloseProtocol()
// - UEFI Spec 2.3.1 + Errata C
//   -  6.3 Protocol Handler Services
//

EFI_STATUS
EFIAPI
VirtioScsiDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS             Status;
  VIRTIO_DEVICE_PROTOCOL *VirtIo;

  //
  // Attempt to open the device with the VirtIo set of interfaces. On success,
  // the protocol is "instantiated" for the VirtIo device. Covers duplicate open
  // attempts (EFI_ALREADY_STARTED).
  //
  Status = gBS->OpenProtocol (
                  DeviceHandle,               // candidate device
                  &gVirtioDeviceProtocolGuid, // for generic VirtIo access
                  (VOID **)&VirtIo,           // handle to instantiate
                  This->DriverBindingHandle,  // requestor driver identity
                  DeviceHandle,               // ControllerHandle, according to
                                              // the UEFI Driver Model
                  EFI_OPEN_PROTOCOL_BY_DRIVER // get exclusive VirtIo access to
                                              // the device; to be released
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (VirtIo->SubSystemDeviceId != VIRTIO_SUBSYSTEM_SCSI_HOST) {
    Status = EFI_UNSUPPORTED;
  }

  //
  // We needed VirtIo access only transitorily, to see whether we support the
  // device or not.
  //
  gBS->CloseProtocol (DeviceHandle, &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle, DeviceHandle);
  return Status;
}


EFI_STATUS
EFIAPI
VirtioScsiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  VSCSI_DEV  *Dev;
  EFI_STATUS Status;

  Dev = (VSCSI_DEV *) AllocateZeroPool (sizeof *Dev);
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (DeviceHandle, &gVirtioDeviceProtocolGuid,
                  (VOID **)&Dev->VirtIo, This->DriverBindingHandle,
                  DeviceHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR (Status)) {
    goto FreeVirtioScsi;
  }

  //
  // VirtIo access granted, configure virtio-scsi device.
  //
  Status = VirtioScsiInit (Dev);
  if (EFI_ERROR (Status)) {
    goto CloseVirtIo;
  }

  Status = gBS->CreateEvent (EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_CALLBACK,
                  &VirtioScsiExitBoot, Dev, &Dev->ExitBoot);
  if (EFI_ERROR (Status)) {
    goto UninitDev;
  }

  //
  // Setup complete, attempt to export the driver instance's PassThru
  // interface.
  //
  Dev->Signature = VSCSI_SIG;
  Status = gBS->InstallProtocolInterface (&DeviceHandle,
                  &gEfiExtScsiPassThruProtocolGuid, EFI_NATIVE_INTERFACE,
                  &Dev->PassThru);
  if (EFI_ERROR (Status)) {
    goto CloseExitBoot;
  }

  return EFI_SUCCESS;

CloseExitBoot:
  gBS->CloseEvent (Dev->ExitBoot);

UninitDev:
  VirtioScsiUninit (Dev);

CloseVirtIo:
  gBS->CloseProtocol (DeviceHandle, &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle, DeviceHandle);

FreeVirtioScsi:
  FreePool (Dev);

  return Status;
}


EFI_STATUS
EFIAPI
VirtioScsiDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  )
{
  EFI_STATUS                      Status;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *PassThru;
  VSCSI_DEV                       *Dev;

  Status = gBS->OpenProtocol (
                  DeviceHandle,                     // candidate device
                  &gEfiExtScsiPassThruProtocolGuid, // retrieve the SCSI iface
                  (VOID **)&PassThru,               // target pointer
                  This->DriverBindingHandle,        // requestor driver ident.
                  DeviceHandle,                     // lookup req. for dev.
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL    // lookup only, no new ref.
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dev = VIRTIO_SCSI_FROM_PASS_THRU (PassThru);

  //
  // Handle Stop() requests for in-use driver instances gracefully.
  //
  Status = gBS->UninstallProtocolInterface (DeviceHandle,
                  &gEfiExtScsiPassThruProtocolGuid, &Dev->PassThru);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseEvent (Dev->ExitBoot);

  VirtioScsiUninit (Dev);

  gBS->CloseProtocol (DeviceHandle, &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle, DeviceHandle);

  FreePool (Dev);

  return EFI_SUCCESS;
}


//
// The static object that groups the Supported() (ie. probe), Start() and
// Stop() functions of the driver together. Refer to UEFI Spec 2.3.1 + Errata
// C, 10.1 EFI Driver Binding Protocol.
//
STATIC EFI_DRIVER_BINDING_PROTOCOL gDriverBinding = {
  &VirtioScsiDriverBindingSupported,
  &VirtioScsiDriverBindingStart,
  &VirtioScsiDriverBindingStop,
  0x10, // Version, must be in [0x10 .. 0xFFFFFFEF] for IHV-developed drivers
  NULL, // ImageHandle, to be overwritten by
        // EfiLibInstallDriverBindingComponentName2() in VirtioScsiEntryPoint()
  NULL  // DriverBindingHandle, ditto
};


//
// The purpose of the following scaffolding (EFI_COMPONENT_NAME_PROTOCOL and
// EFI_COMPONENT_NAME2_PROTOCOL implementation) is to format the driver's name
// in English, for display on standard console devices. This is recommended for
// UEFI drivers that follow the UEFI Driver Model. Refer to the Driver Writer's
// Guide for UEFI 2.3.1 v1.01, 11 UEFI Driver and Controller Names.
//
// Device type names ("Virtio SCSI Host Device") are not formatted because the
// driver supports only that device type. Therefore the driver name suffices
// for unambiguous identification.
//

STATIC
EFI_UNICODE_STRING_TABLE mDriverNameTable[] = {
  { "eng;en", L"Virtio SCSI Host Driver" },
  { NULL,     NULL                   }
};

STATIC
EFI_COMPONENT_NAME_PROTOCOL gComponentName;

EFI_STATUS
EFIAPI
VirtioScsiGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gComponentName) // Iso639Language
           );
}

EFI_STATUS
EFIAPI
VirtioScsiGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  EFI_HANDLE                  DeviceHandle,
  IN  EFI_HANDLE                  ChildHandle,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_COMPONENT_NAME_PROTOCOL gComponentName = {
  &VirtioScsiGetDriverName,
  &VirtioScsiGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

STATIC
EFI_COMPONENT_NAME2_PROTOCOL gComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)     &VirtioScsiGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) &VirtioScsiGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};


//
// Entry point of this driver.
//
EFI_STATUS
EFIAPI
VirtioScsiEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDriverBinding,
           ImageHandle,
           &gComponentName,
           &gComponentName2
           );
}
