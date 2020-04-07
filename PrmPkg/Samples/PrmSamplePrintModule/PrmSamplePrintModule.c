/** @file

  A sample PRM Module implementation. This PRM Module provides 3 PRM handlers that simply take a DEBUG print
  function from the OS and invoke it with a debug message internal the PRM handler.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PrmModule.h>

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>

//
// PRM Handler GUIDs
//

// {d5f2ad5f-a347-4d3e-87bc-c2ce63029cc8}
#define PRM_HANDLER_1_GUID {0xd5f2ad5f, 0xa347, 0x4d3e, {0x87, 0xbc, 0xc2, 0xce, 0x63, 0x02, 0x9c, 0xc8}}

// {a9e7adc3-8cd0-429a-8915-10946ebde318}
#define PRM_HANDLER_2_GUID  {0xa9e7adc3, 0x8cd0, 0x429a, {0x89, 0x15, 0x10, 0x94, 0x6e, 0xbd, 0xe3, 0x18}}

// {b688c214-4081-4eeb-8d26-1eb5a3bcf11a}
#define PRM_HANDLER_N_GUID {0xb688c214, 0x4081, 0x4eeb, {0x8d, 0x26, 0x1e, 0xb5, 0xa3, 0xbc, 0xf1, 0x1a}}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler currently uses the OS_SERVICES to write a debug message
  indicating this is PRM handler 1.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
EFI_STATUS
PRM_EXPORT_API
EFIAPI
PrmHandler1 (
  IN VOID                         *ParameterBuffer,
  IN PRM_CONTEXT_BUFFER           *ContextBUffer
  )
{
  PRM_OS_SERVICE_DEBUG_PRINT      OsServiceDebugPrint;

  if (ParameterBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // In the POC, the OS debug print service is assumed to be at the beginning of ParameterBuffer
  OsServiceDebugPrint = *((PRM_OS_SERVICE_DEBUG_PRINT *) ParameterBuffer);
  if (OsServiceDebugPrint == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  OsServiceDebugPrint ("PRM1 handler sample message!\n");

  return EFI_SUCCESS;
}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler currently uses the OS_SERVICES to write a debug message
  indicating this is PRM handler 2.

  @param[in]  ParameterBuffer    A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer      A pointer to the PRM handler context buffer

  @retval EFI_STATUS             The PRM handler executed successfully.
  @retval Others                 An error occurred in the PRM handler.

**/
EFI_STATUS
PRM_EXPORT_API
EFIAPI
PrmHandler2 (
  IN VOID                         *ParameterBuffer,
  IN PRM_CONTEXT_BUFFER           *ContextBUffer
  )
{
  PRM_OS_SERVICE_DEBUG_PRINT      OsServiceDebugPrint;

  if (ParameterBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // In the POC, the OS debug print service is assumed to be at the beginning of ParameterBuffer
  OsServiceDebugPrint = *((PRM_OS_SERVICE_DEBUG_PRINT *) ParameterBuffer);
  if (OsServiceDebugPrint == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  OsServiceDebugPrint ("PRM2 handler sample message!\n");

  return EFI_SUCCESS;
}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler currently uses the OS_SERVICES to write a debug message
  indicating this is PRM handler N.

  @param[in]  ParameterBuffer    A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer      A pointer to the PRM handler context buffer

  @retval EFI_STATUS             The PRM handler executed successfully.
  @retval Others                 An error occurred in the PRM handler.

**/
EFI_STATUS
PRM_EXPORT_API
EFIAPI
PrmHandlerN (
  IN VOID                         *ParameterBuffer,
  IN PRM_CONTEXT_BUFFER           *ContextBUffer
  )
{
  PRM_OS_SERVICE_DEBUG_PRINT      OsServiceDebugPrint;

  if (ParameterBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // In the POC, the OS debug print service is assumed to be at the beginning of ParameterBuffer
  OsServiceDebugPrint = *((PRM_OS_SERVICE_DEBUG_PRINT *) ParameterBuffer);
  if (OsServiceDebugPrint == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  OsServiceDebugPrint ("PRMN handler sample message!\n");

  return EFI_SUCCESS;
}

//
// Register the PRM export information for this PRM Module
//
PRM_MODULE_EXPORT (
  PRM_HANDLER_EXPORT_ENTRY (PRM_HANDLER_1_GUID, PrmHandler1),
  PRM_HANDLER_EXPORT_ENTRY (PRM_HANDLER_2_GUID, PrmHandler2),
  PRM_HANDLER_EXPORT_ENTRY (PRM_HANDLER_N_GUID, PrmHandlerN)
  );

EFI_STATUS
EFIAPI
PrmSamplePrintModuleInit (
  IN  EFI_HANDLE                  ImageHandle,
  IN  EFI_SYSTEM_TABLE            *SystemTable
  )
{
  return EFI_SUCCESS;
}
