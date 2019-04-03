/** @file
  TCG MOR (Memory Overwrite Request) Control Driver.

  This driver initilize MemoryOverwriteRequestControl variable. It
  will clear MOR_CLEAR_MEMORY_BIT bit if it is set. It will also do TPer Reset for
  those encrypted drives through EFI_STORAGE_SECURITY_COMMAND_PROTOCOL at EndOfDxe.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TcgMor.h"

UINT8    mMorControl;

/**
  Ready to Boot Event notification handler.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
VOID
EFIAPI
OnReadyToBoot (
  IN      EFI_EVENT                 Event,
  IN      VOID                      *Context
  )
{
  EFI_STATUS  Status;
  UINTN       DataSize;

  if (MOR_CLEAR_MEMORY_VALUE (mMorControl) == 0x0) {
    //
    // MorControl is expected, directly return to avoid unnecessary variable operation
    //
    return ;
  }
  //
  // Clear MOR_CLEAR_MEMORY_BIT
  //
  DEBUG ((EFI_D_INFO, "TcgMor: Clear MorClearMemory bit\n"));
  mMorControl &= 0xFE;

  DataSize = sizeof (mMorControl);
  Status   = gRT->SetVariable (
               MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
               &gEfiMemoryOverwriteControlDataGuid,
               EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
               DataSize,
               &mMorControl
               );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "TcgMor: Clear MOR_CLEAR_MEMORY_BIT failure, Status = %r\n"));
  }
}

/**
  Send TPer Reset command to reset eDrive to lock all protected bands.
  Typically, there are 2 mechanism for resetting eDrive. They are:
  1. TPer Reset through IEEE 1667 protocol.
  2. TPer Reset through native TCG protocol.
  This routine will detect what protocol the attached eDrive comform to, TCG or
  IEEE 1667 protocol. Then send out TPer Reset command separately.

  @param[in] Ssp      The pointer to EFI_STORAGE_SECURITY_COMMAND_PROTOCOL instance.
  @param[in] MediaId  ID of the medium to receive data from or send data to.

**/
VOID
InitiateTPerReset (
  IN  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *Ssp,
  IN  UINT32                                   MediaId
  )
{

  EFI_STATUS                                   Status;
  UINT8                                        *Buffer;
  UINTN                                        XferSize;
  UINTN                                        Len;
  UINTN                                        Index;
  BOOLEAN                                      TcgFlag;
  BOOLEAN                                      IeeeFlag;
  SUPPORTED_SECURITY_PROTOCOLS_PARAMETER_DATA  *Data;

  Buffer        = NULL;
  TcgFlag       = FALSE;
  IeeeFlag      = FALSE;

  //
  // ATA8-ACS 7.57.6.1 indicates the Transfer Length field requirements a multiple of 512.
  // If the length of the TRUSTED RECEIVE parameter data is greater than the Transfer Length,
  // then the device shall return the TRUSTED RECEIVE parameter data truncated to the requested Transfer Length.
  //
  Len           = ROUNDUP512(sizeof(SUPPORTED_SECURITY_PROTOCOLS_PARAMETER_DATA));
  Buffer        = AllocateZeroPool(Len);

  if (Buffer == NULL) {
    return;
  }

  //
  // When the Security Protocol field is set to 00h, and SP Specific is set to 0000h in a TRUSTED RECEIVE
  // command, the device basic information data shall be returned.
  //
  Status = Ssp->ReceiveData (
                  Ssp,
                  MediaId,
                  100000000,                    // Timeout 10-sec
                  0,                            // SecurityProtocol
                  0,                            // SecurityProtocolSpecifcData
                  Len,                          // PayloadBufferSize,
                  Buffer,                       // PayloadBuffer
                  &XferSize
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // In returned data, the ListLength field indicates the total length, in bytes,
  // of the supported security protocol list.
  //
  Data = (SUPPORTED_SECURITY_PROTOCOLS_PARAMETER_DATA*)Buffer;
  Len  = ROUNDUP512(sizeof (SUPPORTED_SECURITY_PROTOCOLS_PARAMETER_DATA) +
                    (Data->SupportedSecurityListLength[0] << 8) +
                    (Data->SupportedSecurityListLength[1])
                    );

  //
  // Free original buffer and allocate new buffer.
  //
  FreePool(Buffer);
  Buffer = AllocateZeroPool(Len);
  if (Buffer == NULL) {
    return;
  }

  //
  // Read full supported security protocol list from device.
  //
  Status = Ssp->ReceiveData (
                  Ssp,
                  MediaId,
                  100000000,                    // Timeout 10-sec
                  0,                            // SecurityProtocol
                  0,                            // SecurityProtocolSpecifcData
                  Len,                          // PayloadBufferSize,
                  Buffer,                       // PayloadBuffer
                  &XferSize
                  );

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Data = (SUPPORTED_SECURITY_PROTOCOLS_PARAMETER_DATA*)Buffer;
  Len  = (Data->SupportedSecurityListLength[0] << 8) + Data->SupportedSecurityListLength[1];

  //
  // Iterate full supported security protocol list to check if TCG or IEEE 1667 protocol
  // is supported.
  //
  for (Index = 0; Index < Len; Index++) {
    if (Data->SupportedSecurityProtocol[Index] == SECURITY_PROTOCOL_TCG) {
      //
      // Found a  TCG device.
      //
      TcgFlag = TRUE;
      DEBUG ((EFI_D_INFO, "This device is a TCG protocol device\n"));
      break;
    }

    if (Data->SupportedSecurityProtocol[Index] == SECURITY_PROTOCOL_IEEE1667) {
      //
      // Found a IEEE 1667 device.
      //
      IeeeFlag = TRUE;
      DEBUG ((EFI_D_INFO, "This device is a IEEE 1667 protocol device\n"));
      break;
    }
  }

  if (!TcgFlag && !IeeeFlag) {
    DEBUG ((EFI_D_INFO, "Neither a TCG nor IEEE 1667 protocol device is found\n"));
    goto Exit;
  }

  if (TcgFlag) {
    //
    // As long as TCG protocol is supported, send out a TPer Reset
    // TCG command to the device via the TrustedSend command with a non-zero Transfer Length.
    //
    Status = Ssp->SendData (
                    Ssp,
                    MediaId,
                    100000000,                    // Timeout 10-sec
                    SECURITY_PROTOCOL_TCG,        // SecurityProtocol
                    0x0400,                       // SecurityProtocolSpecifcData
                    512,                          // PayloadBufferSize,
                    Buffer                        // PayloadBuffer
                    );

    if (!EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "Send TPer Reset Command Successfully !\n"));
    } else {
      DEBUG ((EFI_D_INFO, "Send TPer Reset Command Fail !\n"));
    }
  }

  if (IeeeFlag) {
    //
    // TBD : Perform a TPer Reset via IEEE 1667 Protocol
    //
    DEBUG ((EFI_D_INFO, "IEEE 1667 Protocol didn't support yet!\n"));
  }

Exit:

  if (Buffer != NULL) {
    FreePool(Buffer);
  }
}

/**
  Notification function of END_OF_DXE.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    Pointer to the notification function's context.

**/
VOID
EFIAPI
TPerResetAtEndOfDxe (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL   *Ssp;
  EFI_BLOCK_IO_PROTOCOL                   *BlockIo;
  EFI_STATUS                              Status;
  UINTN                                   HandleCount;
  EFI_HANDLE                              *HandleBuffer;
  UINTN                                   Index;

  //
  // Locate all SSP protocol instances.
  //
  HandleCount  = 0;
  HandleBuffer = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiStorageSecurityCommandProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status) || (HandleCount == 0) || (HandleBuffer == NULL)) {
    return;
  }

  for (Index = 0; Index < HandleCount; Index ++) {
    //
    // Get the SSP interface.
    //
    Status = gBS->HandleProtocol(
                    HandleBuffer[Index],
                    &gEfiStorageSecurityCommandProtocolGuid,
                    (VOID **) &Ssp
                    );

    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = gBS->HandleProtocol(
                    HandleBuffer[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlockIo
                    );

    if (EFI_ERROR (Status)) {
      continue;
    }

    InitiateTPerReset (Ssp, BlockIo->Media->MediaId);
  }

  FreePool (HandleBuffer);
}

/**
  Entry Point for TCG MOR Control driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCEESS
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
MorDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       DataSize;
  EFI_EVENT   Event;

  ///
  /// The firmware is required to create the MemoryOverwriteRequestControl UEFI variable.
  ///

  DataSize = sizeof (mMorControl);
  Status = gRT->GetVariable (
                  MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                  &gEfiMemoryOverwriteControlDataGuid,
                  NULL,
                  &DataSize,
                  &mMorControl
                  );
  if (EFI_ERROR (Status)) {
    //
    // Set default value to 0
    //
    mMorControl = 0;
    Status = gRT->SetVariable (
                    MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                    &gEfiMemoryOverwriteControlDataGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    DataSize,
                    &mMorControl
                    );
    DEBUG ((EFI_D_INFO, "TcgMor: Create MOR variable! Status = %r\n", Status));
  } else {
    //
    // Create a Ready To Boot Event and Clear the MorControl bit in the call back function.
    //
    DEBUG ((EFI_D_INFO, "TcgMor: Create ReadyToBoot Event for MorControl Bit cleanning!\n"));
    Status = EfiCreateEventReadyToBootEx (
               TPL_CALLBACK,
               OnReadyToBoot,
               NULL,
               &Event
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Register EFI_END_OF_DXE_EVENT_GROUP_GUID event.
    //
    DEBUG ((EFI_D_INFO, "TcgMor: Create EndofDxe Event for Mor TPer Reset!\n"));
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    TPerResetAtEndOfDxe,
                    NULL,
                    &gEfiEndOfDxeEventGroupGuid,
                    &Event
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return Status;
}


