/** @file  
  Utility functions used by TPM Dxe driver.

Copyright (c) 2005 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <IndustryStandard/Tpm12.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseCryptLib.h>

#include "TpmComm.h"

/**
  Extend a TPM PCR.

  @param[in]  DigestToExtend  The 160 bit value representing the event to be recorded.  
  @param[in]  PcrIndex        The PCR to be updated.
  @param[out] NewPcrValue     New PCR value after extend.  
  
  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The command was unsuccessful.

**/
EFI_STATUS
TpmCommExtend (
  IN      TPM_DIGEST                *DigestToExtend,
  IN      TPM_PCRINDEX              PcrIndex,
     OUT  TPM_DIGEST                *NewPcrValue
  )
{
  EFI_STATUS                        Status;
  TPM_DIGEST                        NewValue;
  TPM_RQU_COMMAND_HDR               CmdHdr;
  TPM_RSP_COMMAND_HDR               RspHdr;

  if (NewPcrValue == NULL) {
    NewPcrValue = &NewValue;
  }

  CmdHdr.tag = TPM_TAG_RQU_COMMAND;
  CmdHdr.paramSize =
    sizeof (CmdHdr) + sizeof (PcrIndex) + sizeof (*DigestToExtend);
  CmdHdr.ordinal = TPM_ORD_Extend;
  Status = TisPcExecute (
             "%h%d%r%/%h%r",
             &CmdHdr,
             PcrIndex,
             DigestToExtend,
             (UINTN)sizeof (*DigestToExtend),
             &RspHdr,
             NewPcrValue,
             (UINTN)sizeof (*NewPcrValue)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (RspHdr.returnCode != 0) {
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

/**
  Get TPM capability flags.

  @param[in]  FlagSubcap   Flag subcap.  
  @param[out] FlagBuffer   Pointer to the buffer for returned flag structure.
  @param[in]  FlagSize     Size of the buffer.  
  
  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR The command was unsuccessful.

**/
EFI_STATUS
TpmCommGetFlags (
  IN      UINT32                    FlagSubcap,
     OUT  VOID                      *FlagBuffer,
  IN      UINTN                     FlagSize
  )
{
  EFI_STATUS                        Status;
  TPM_RQU_COMMAND_HDR               CmdHdr;
  TPM_RSP_COMMAND_HDR               RspHdr;
  UINT32                            Size;

  CmdHdr.tag = TPM_TAG_RQU_COMMAND;
  CmdHdr.paramSize = sizeof (CmdHdr) + sizeof (UINT32) * 3;
  CmdHdr.ordinal = TPM_ORD_GetCapability;

  Status = TisPcExecute (
             "%h%d%d%d%/%h%d%r",
             &CmdHdr,
             TPM_CAP_FLAG,
             sizeof (FlagSubcap),
             FlagSubcap,
             &RspHdr,
             &Size,
             FlagBuffer,
             FlagSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (RspHdr.returnCode != 0) {
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

/**
  Add a new entry to the Event Log.

  @param[in, out] EventLogPtr   Pointer to the Event Log data.  
  @param[in, out] LogSize       Size of the Event Log.  
  @param[in]      MaxSize       Maximum size of the Event Log.
  @param[in]      NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.  
  @param[in]      NewEventData  Pointer to the new event data.  
  
  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.

**/
EFI_STATUS
TpmCommLogEvent (
  IN OUT  UINT8                     **EventLogPtr,
  IN OUT  UINTN                     *LogSize,
  IN      UINTN                     MaxSize,
  IN      TCG_PCR_EVENT_HDR         *NewEventHdr,
  IN      UINT8                     *NewEventData
  )
{
  UINTN                            NewLogSize;

  //
  // Prevent Event Overflow
  //
  if (NewEventHdr->EventSize > (UINTN)(~0) - sizeof (*NewEventHdr)) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewLogSize = sizeof (*NewEventHdr) + NewEventHdr->EventSize;
  if (NewLogSize > MaxSize - *LogSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  *EventLogPtr += *LogSize;
  *LogSize += NewLogSize;
  CopyMem (*EventLogPtr, NewEventHdr, sizeof (*NewEventHdr));
  CopyMem (
    *EventLogPtr + sizeof (*NewEventHdr),
    NewEventData,
    NewEventHdr->EventSize
    );
  return EFI_SUCCESS;
}

/**
  Single function calculates SHA1 digest value for all raw data. It
  combines Sha1Init(), Sha1Update() and Sha1Final().

  @param[in]  Data          Raw data to be digested.
  @param[in]  DataLen       Size of the raw data.
  @param[out] Digest        Pointer to a buffer that stores the final digest.

  @retval     EFI_SUCCESS   Always successfully calculate the final digest.
**/
EFI_STATUS
EFIAPI
TpmCommHashAll (
  IN  CONST UINT8                   *Data,
  IN        UINTN                   DataLen,
  OUT       TPM_DIGEST              *Digest
  )
{
  VOID     *Sha1Ctx;
  UINTN    CtxSize;

  CtxSize = Sha1GetContextSize ();
  Sha1Ctx = AllocatePool (CtxSize);
  ASSERT (Sha1Ctx != NULL);

  Sha1Init (Sha1Ctx);
  Sha1Update (Sha1Ctx, Data, DataLen);
  Sha1Final (Sha1Ctx, (UINT8 *)Digest);

  FreePool (Sha1Ctx);

  return EFI_SUCCESS;
}
