/** @file

  A sample PRM Module implementation. This PRM Module provides 3 PRM handlers that simply take a DEBUG print
  function from the OS and invoke it with a debug message internal the PRM handler.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PrmModule.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

//
// PRM Handler GUIDs
//

// {149a5cb3-6a9c-403f-940a-156abf63938a}
#define PRM_HANDLER_1_GUID {0x149a5cb3, 0x6a9c, 0x403f, {0x94, 0x0a, 0x15, 0x6a, 0xbf, 0x63, 0x93, 0x8a}}

// Note: If the signature size is modified, the PRM Handler test code in this module needs to be updated.
#define MEMORY_ALLOCATION_TEST_DATA_SIGNATURE     SIGNATURE_32('T','E','S','T')
#define MEMORY_ALLOCATION_TEST_DATA_SIZE          sizeof(UINT32)
#define MEMORY_ALLOCATION_TEST_DATA_BUFFER_SIZE   256

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler currently uses the OS_SERVICES to write a debug message
  indicating this is PRM handler 1.

  @param[in]  ParameterBuffer    A pointer to the PRM handler parameter buffer
  @param[in]  ContextBuffer      A pointer to the PRM handler context buffer

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
  EFI_STATUS                      Status;
  UINTN                           Index;
  VOID                            *NonPagedPool;
  CHAR8                           DebugMessage[256];

  if (OsServices == NULL || OsServices->DebugPrint == NULL || OsServices->AllocateMemory == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OsServices->DebugPrint ("Memory Allocation PrmHandler1 entry.\n");
  OsServices->DebugPrint ("  Requesting allocation of a 256 byte non-paged pool...\n");

  NonPagedPool = NULL;
  NonPagedPool = OsServices->AllocateMemory (MEMORY_ALLOCATION_TEST_DATA_BUFFER_SIZE, FALSE);
  if (NonPagedPool == NULL) {
    OsServices->DebugPrint ("  NULL was returned from AllocateMemory()...\n");
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiSPrint (
    &DebugMessage[0],
    ARRAY_SIZE (DebugMessage),
    "  Buffer address returned from AllocateMemory() = 0x%016lx.\n",
    (UINTN) NonPagedPool
    );
  OsServices->DebugPrint (&DebugMessage[0]);

  // Write the test data
  OsServices->DebugPrint ("  Beginning memory buffer write and read back test...\n");
  SetMem32 (NonPagedPool, MEMORY_ALLOCATION_TEST_DATA_BUFFER_SIZE, MEMORY_ALLOCATION_TEST_DATA_SIGNATURE);

  // Read back and verify the test data is valid
  for (Index = 0, Status = EFI_SUCCESS; Index < (MEMORY_ALLOCATION_TEST_DATA_BUFFER_SIZE / MEMORY_ALLOCATION_TEST_DATA_SIZE); Index++) {
    if (((UINT32 *) NonPagedPool)[Index] != MEMORY_ALLOCATION_TEST_DATA_SIGNATURE) {
      Status = EFI_DEVICE_ERROR;
      break;
    }
  }
  if (EFI_ERROR (Status)) {
    OsServices->DebugPrint ("    Memory write & read test failed.\n");
  } else {
    OsServices->DebugPrint ("    Memory write & read test passed.\n");
  }

  OsServices->DebugPrint ("Memory Allocation PrmHandler1 exit.\n");

  return EFI_SUCCESS;
}

//
// Register the PRM export information for this PRM Module
//
PRM_MODULE_EXPORT (
  PRM_HANDLER_EXPORT_ENTRY (PRM_HANDLER_1_GUID, PrmHandler1)
  );

EFI_STATUS
EFIAPI
PrmSampleMemoryAllocationModuleInit (
  IN  EFI_HANDLE                  ImageHandle,
  IN  EFI_SYSTEM_TABLE            *SystemTable
  )
{
  return EFI_SUCCESS;
}
