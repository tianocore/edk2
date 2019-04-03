/** @file
This is the driver that locates the MemoryConfigurationData HOB, if it
exists, and saves the data to nvRAM.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>

#include <Guid/MemoryConfigData.h>
#include <Guid/DebugMask.h>

EFI_STATUS
EFIAPI
SaveMemoryConfigEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

  Routine Description:
    This is the standard EFI driver point that detects whether there is a
    MemoryConfigurationData HOB and, if so, saves its data to nvRAM.

  Arguments:
    ImageHandle   - Handle for the image of this driver
    SystemTable   - Pointer to the EFI System Table

  Returns:
    EFI_SUCCESS   - if the data is successfully saved or there was no data
    EFI_NOT_FOUND - if the HOB list could not be located.
    EFI_UNLOAD_IMAGE - It is not success

--*/
{
  EFI_STATUS  Status;
  VOID        *HobList;
  EFI_HOB_GUID_TYPE *GuidHob;
  VOID        *HobData;
  VOID        *VariableData;
  UINTN       DataSize;
  UINTN       BufferSize;

  DataSize      = 0;
  VariableData  = NULL;
  GuidHob = NULL;
  HobList = NULL;
  HobData = NULL;
  Status  = EFI_UNSUPPORTED;

  //
  // Get the HOB list.  If it is not present, then ASSERT.
  //
  HobList = GetHobList ();
  ASSERT (HobList != NULL);

  //
  // Search for the Memory Configuration GUID HOB.  If it is not present, then
  // there's nothing we can do. It may not exist on the update path.
  //
  GuidHob = GetNextGuidHob (&gEfiMemoryConfigDataGuid, HobList);
  if (GuidHob != NULL) {
    HobData = GET_GUID_HOB_DATA (GuidHob);
    DataSize = GET_GUID_HOB_DATA_SIZE (GuidHob);
    //
    // Use the HOB to save Memory Configuration Data
    //
    BufferSize = DataSize;
    VariableData = AllocatePool (BufferSize);
    ASSERT (VariableData != NULL);
    Status = gRT->GetVariable (
                    EFI_MEMORY_CONFIG_DATA_NAME,
                    &gEfiMemoryConfigDataGuid,
                    NULL,
                    &BufferSize,
                    VariableData
                    );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      gBS->FreePool (VariableData);
      VariableData = AllocatePool (BufferSize);
      ASSERT (VariableData != NULL);
      Status = gRT->GetVariable (
                      EFI_MEMORY_CONFIG_DATA_NAME,
                      &gEfiMemoryConfigDataGuid,
                      NULL,
                      &BufferSize,
                      VariableData
                      );
    }

    if (EFI_ERROR(Status) || BufferSize != DataSize || CompareMem (HobData, VariableData, DataSize) != 0) {
      Status = gRT->SetVariable (
                      EFI_MEMORY_CONFIG_DATA_NAME,
                      &gEfiMemoryConfigDataGuid,
                      (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                      DataSize,
                      HobData
                      );
      ASSERT((Status == EFI_SUCCESS) || (Status == EFI_OUT_OF_RESOURCES));
    }

    gBS->FreePool (VariableData);
  }

  //
  // This driver does not produce any protocol services, so always unload it.
  //
  return Status;
}
