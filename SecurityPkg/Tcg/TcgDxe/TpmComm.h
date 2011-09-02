/** @file  
  Definitions and function prototypes used by TPM DXE driver.

Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TPM_COMM_H_
#define _TPM_COMM_H_

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
  );

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
  );

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
     OUT  VOID                      *Buffer,
  IN      UINTN                     Size
  );

/**
  Send formatted command to TPM for execution and return formatted data from response.

  @param[in] TisReg    TPM Handle.  
  @param[in] Fmt       Format control string.  
  @param[in] ...       The variable argument list.
 
  @retval EFI_SUCCESS  Operation completed successfully.
  @retval EFI_TIMEOUT  The register can't run into the expected status in time.

**/
EFI_STATUS
EFIAPI
TisPcExecute (
  IN      TIS_TPM_HANDLE            TisReg,
  IN      CONST CHAR8               *Fmt,
  ...
  );

#endif  // _TPM_COMM_H_
