/** @file

  This PRM Module demonstrates how to configure the module data resources in the firmware boot environment
  and access those resources in a PRM handler at OS runtime.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PrmModule.h>

#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

#include <Samples/PrmSampleContextBufferModule/Include/StaticData.h>

//
// PRM Handler GUIDs
//

// {e1466081-7562-430f-896b-b0e523dc335a}
#define CHECK_STATIC_DATA_BUFFER_PRM_HANDLER_GUID  {0xe1466081, 0x7562, 0x430f, {0x89, 0x6b, 0xb0, 0xe5, 0x23, 0xdc, 0x33, 0x5a}}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler checks that a static data buffer can be accessed from a given context buffer.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (CheckStaticDataBufferPrmHandler) {
  if (ContextBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ContextBuffer->StaticDataBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Verify PRM data buffer signature is valid
  //
  if (
      (ContextBuffer->Signature != PRM_CONTEXT_BUFFER_SIGNATURE) ||
      (ContextBuffer->StaticDataBuffer->Header.Signature != PRM_DATA_BUFFER_HEADER_SIGNATURE))
  {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

//
// Register the PRM export information for this PRM Module
//
PRM_MODULE_EXPORT (
  PRM_HANDLER_EXPORT_ENTRY (CHECK_STATIC_DATA_BUFFER_PRM_HANDLER_GUID, CheckStaticDataBufferPrmHandler)
  );

/**
  Module entry point.

  @param[in]   ImageHandle     The image handle.
  @param[in]   SystemTable     A pointer to the system table.

  @retval  EFI_SUCCESS         This function always returns success.

**/
EFI_STATUS
EFIAPI
PrmSampleContextBufferModuleInit (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
