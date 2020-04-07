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
#define DUMP_STATIC_DATA_BUFFER_PRM_HANDLER_GUID {0xe1466081, 0x7562, 0x430f, {0x89, 0x6b, 0xb0, 0xe5, 0x23, 0xdc, 0x33, 0x5a}}

/**
  Dumps the contents of a given buffer.

  @param[in]  OsServiceDebugPrint   A pointer to the debug print OS service.
  @param[in]  Buffer                A pointer to the buffer that should be dumped.
  @param[in]  BufferSize            The size of Buffer in bytes.

**/
STATIC
VOID
DumpBuffer (
  IN PRM_OS_SERVICE_DEBUG_PRINT OsServiceDebugPrint,
  IN CONST VOID                 *Buffer,
  IN UINTN                      BufferSize
  )
{
  UINTN Count;
  CONST UINT8 *Char = Buffer;
  CHAR8 DebugMessage[16];

  if (OsServiceDebugPrint == NULL || Buffer == NULL) {
    return;
  }

  OsServiceDebugPrint ("    ");
  for (Count = 0; Count < BufferSize; Count++)
  {
    if (Count && !(Count % 16)) {
      OsServiceDebugPrint ("\n    ");
    }
    AsciiSPrint (
      &DebugMessage[0],
      ARRAY_SIZE (DebugMessage),
      "%02X ",
      Char[Count]
      );
    OsServiceDebugPrint (&DebugMessage[0]);
  }
  OsServiceDebugPrint ("\n\n");
}

/**
  Prints the contents of this PRM module's static data buffer.

  @param[in]  OsServiceDebugPrint   A pointer to the debug print OS service.
  @param[in]  StaticDataBuffer      A pointer to the static buffer.

**/
VOID
EFIAPI
PrintStaticDataBuffer (
  IN PRM_OS_SERVICE_DEBUG_PRINT                       OsServiceDebugPrint,
  IN CONST STATIC_DATA_SAMPLE_CONTEXT_BUFFER_MODULE   *StaticDataBuffer
  )
{
  CHAR8 DebugMessage[256];

  if (OsServiceDebugPrint == NULL || StaticDataBuffer == NULL) {
    return;
  }

  AsciiSPrint (
    &DebugMessage[0],
    ARRAY_SIZE (DebugMessage),
    "  Policy1Enabled = 0x%x.\n",
    StaticDataBuffer->Policy1Enabled
    );
  OsServiceDebugPrint (&DebugMessage[0]);

  AsciiSPrint (
    &DebugMessage[0],
    ARRAY_SIZE (DebugMessage),
    "  Policy2Enabled = 0x%x.\n",
    StaticDataBuffer->Policy2Enabled
    );
  OsServiceDebugPrint (&DebugMessage[0]);

  OsServiceDebugPrint ("  Dumping SomeValueArray:\n");
  DumpBuffer (
    OsServiceDebugPrint,
    (CONST VOID *) &StaticDataBuffer->SomeValueArray[0],
    ARRAY_SIZE (StaticDataBuffer->SomeValueArray)
    );
}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler attempts to read the contents of the static data buffer that were configured
  during the firmware boot environment and print those contents at OS runtime.

  @param[in]  OsServices          An array of pointers to OS provided services for PRM handlers
  @param[in]  Context             Handler context info

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (DumpStaticDataBufferPrmHandler)
{
  PRM_OS_SERVICE_DEBUG_PRINT      OsServiceDebugPrint;

  if (ContextBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // In the POC, the OS debug print service is assumed to be at the beginning of ParameterBuffer
  OsServiceDebugPrint = *((PRM_OS_SERVICE_DEBUG_PRINT *) ParameterBuffer);
  if (OsServiceDebugPrint == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OsServiceDebugPrint ("Context Buffer DumpStaticDataBufferPrmHandler entry.\n");

  if (ContextBuffer->StaticDataBuffer == NULL) {
    OsServiceDebugPrint ("The static buffer is not allocated!\n");
    return EFI_INVALID_PARAMETER;
  }

  OsServiceDebugPrint ("  Printing the contents of the static data buffer:\n");

  //
  // Verify PRM data buffer signature is valid
  //
  if (
    ContextBuffer->Signature != PRM_CONTEXT_BUFFER_SIGNATURE ||
    ContextBuffer->StaticDataBuffer->Header.Signature != PRM_DATA_BUFFER_HEADER_SIGNATURE) {
    OsServiceDebugPrint ("  A buffer signature is invalid!\n");
    return EFI_NOT_FOUND;
  }

  PrintStaticDataBuffer (
    OsServiceDebugPrint,
    (CONST STATIC_DATA_SAMPLE_CONTEXT_BUFFER_MODULE *) &(ContextBuffer->StaticDataBuffer->Data[0])
    );

  OsServiceDebugPrint ("Context Buffer DumpStaticDataBufferPrmHandler exit.\n");

  return EFI_SUCCESS;
}

//
// Register the PRM export information for this PRM Module
//
PRM_MODULE_EXPORT (
  PRM_HANDLER_EXPORT_ENTRY (DUMP_STATIC_DATA_BUFFER_PRM_HANDLER_GUID, DumpStaticDataBufferPrmHandler)
  );

EFI_STATUS
EFIAPI
PrmSampleContextBufferModuleInit (
  IN  EFI_HANDLE                  ImageHandle,
  IN  EFI_SYSTEM_TABLE            *SystemTable
  )
{
  return EFI_SUCCESS;
}
