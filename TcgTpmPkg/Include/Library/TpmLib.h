/** @file
  TpmLib to call Tcg TPM reference code.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

/**
  Execute TPM command.

  This function calls ExecuteCommand() in Tcg TPM reference code.
  See the detail ExecuteCommand()'s comment.

  @param [in]       RequestSize     Request Buffer Size
  @param [in]       Request         Request Buffer
  @param [in, out]  ResponseSize    Response Buffer Size
  @param [in, out]  Response        Response Buffer

  @return  EFI_SUCCESS              Success to exectue requested command
                                    But need to check the result of
                                    command execution in Response.
  @return  EFI_INVALID_PARAMETER    Invalid parameters.

**/
EFI_STATUS
EFIAPI
TpmLibExecuteCommand (
  IN      UINT32  RequestSize,
  IN      UINT8   *Request,
  IN OUT  UINT32  *ResponseSize,
  IN OUT  UINT8   **Response
  );

/**
  Initailize TPM.

  This function calls _TPM_Init() in Tcg TPM reference code.

**/
VOID
EFIAPI
TpmLibTpmInit (
  VOID
  );

/**
  This indication from the TPM interface indicates the start of
  an H-CRTM measurement sequence. On receipt of this indication,
  the TPM will initialize an H-CRTM Event Sequence context

  This function calls _TPM_Hash_Start() in Tcg TPM reference code.

**/
VOID
EFIAPI
TpmLibTpmHashStart (
  VOID
  );

/**
  This indication from the TPM interface indicates arrival of
  one or more octets of data that are to be included in the H-CRTM
  Event Sequence sequence context created by the _TPM_Hash_Start indication.
  The context holds data for each hash algorithm for
  each PCR bank implemented on the TPM.
  If no H-CRTM Event Sequence context exists, this indication is discarded
  and no other action is performed.

  This function calls _TPM_Hash_Data() in Tcg TPM reference code.

  @param [in] DataSize Size of data to be extend
  @param [in] Data     Data Buffer

**/
VOID
EFIAPI
TpmLibTpmHashData (
  IN UINT32  DataSize,
  IN UINT8   *Data
  );

/**
  This indication from the TPM interface indicates
  the end of the H-CRTM measurement. This indication is discarded and
  no other action performed if the TPM does not contain
  an H-CRTM Event Sequence context.

  This function calls _TPM_Hash_End() in Tcg TPM reference code.

**/
VOID
EFIAPI
TpmLibTpmHashEnd (
  VOID
  );

/**
  This is called when TPM's NV storage isn't initailized only
  (PlatformTpmLibNVNeedsManufacture() return FALSE) otherwise,
  TPM's NV storage is cleared all.

  Before calling this, PlatformTpmLibNVEnable() must be called.

  This function calls TPM_Manufacture() in Tcg TPM reference code.

  @return   -2          NV System not available
  @return   -1          FAILURE - System is incorrectly compiled.
  @return    0          success

**/
INT32
EFIAPI
TpmLibTpmManufacture (
  VOID
  );

/**
  Get current locality.

  This function calls _plat__LocalityGet() in Tcg TPM reference code.

  @return    Locality   Current locality.

**/
UINT8
EFIAPI
TpmLibGetLocality (
  VOID
  );

/**
  Set current locality.

  See the detail _plat__LocalitySet()'s comment.

  @param [in]   Locality

**/
VOID
EFIAPI
TpmLibSetLocality (
  IN UINT8  Locality
  );
