/** @file

  This PRM Module demonstrates how to define an ACPI parameter buffer that is used by a PRM handler.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PrmModule.h>

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>

// TEMP
#include <Library/DebugLib.h>

#define PARAM_BUFFER_TEST_SIGNATURE  SIGNATURE_32('T','E','S','T')

//
// PRM Handler GUIDs
//

// {2e4f2d13-6240-4ed0-a401-c723fbdc34e8}
#define CHECK_PARAM_BUFFER_PRM_HANDLER_GUID  {0x2e4f2d13, 0x6240, 0x4ed0, {0xa4, 0x01, 0xc7, 0x23, 0xfb, 0xdc, 0x34, 0xe8}}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler checks if a parameter buffer is provided with the data signature
  ('T', 'E', 'S', 'T') at the beginning of the buffer.

  The contents are expected to be updated by ACPI code at OS runtime.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (CheckParamBufferPrmHandler) {
  if (ParameterBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*((UINT32 *)ParameterBuffer) == PARAM_BUFFER_TEST_SIGNATURE) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

//
// Register the PRM export information for this PRM Module
//
PRM_MODULE_EXPORT (
  PRM_HANDLER_EXPORT_ENTRY (CHECK_PARAM_BUFFER_PRM_HANDLER_GUID, CheckParamBufferPrmHandler)
  );

/**
  Module entry point.

  @param[in]   ImageHandle     The image handle.
  @param[in]   SystemTable     A pointer to the system table.

  @retval  EFI_SUCCESS         This function always returns success.

**/
EFI_STATUS
EFIAPI
PrmSampleAcpiParameterBufferModuleInit (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
