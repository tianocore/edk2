/** @file  
  Utility functions used by TPM Dxe driver.

Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <IndustryStandard/Tpm12.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/TpmCommLib.h>
#include <Library/BaseMemoryLib.h>

#include "TpmComm.h"

/**
  Extend a TPM PCR.

  @param[in]  TpmHandle       TPM handle.  
  @param[in]  DigestToExtend  The 160 bit value representing the event to be recorded.  
  @param[in]  PcrIndex        The PCR to be updated.
  @param[out] NewPcrValue     New PCR value after extend.  
  
  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The command was unsuccessful.

**/
EFI_STATUS
TpmCommExtend (
  IN      TIS_TPM_HANDLE            TpmHandle,
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
             TpmHandle,
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

  @param[in]  TpmHandle    TPM handle.  
  @param[in]  FlagSubcap   Flag subcap.  
  @param[out] FlagBuffer   Pointer to the buffer for returned flag structure.
  @param[in]  FlagSize     Size of the buffer.  
  
  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR The command was unsuccessful.

**/
EFI_STATUS
TpmCommGetFlags (
  IN      TIS_TPM_HANDLE            TpmHandle,
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
             TpmHandle,
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
  UINT32                            NewLogSize;

  NewLogSize = sizeof (*NewEventHdr) + NewEventHdr->EventSize;
  if (NewLogSize + *LogSize > MaxSize) {
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
