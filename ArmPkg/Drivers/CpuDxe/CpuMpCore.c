/** @file
*
*  Copyright (c) 2011-2021, Arm Limited. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Guid/ArmMpCoreInfo.h>

ARM_PROCESSOR_TABLE  mArmProcessorTableTemplate = {
  {
    EFI_ARM_PROCESSOR_TABLE_SIGNATURE,
    0,
    EFI_ARM_PROCESSOR_TABLE_REVISION,
    EFI_ARM_PROCESSOR_TABLE_OEM_ID,
    EFI_ARM_PROCESSOR_TABLE_OEM_TABLE_ID,
    EFI_ARM_PROCESSOR_TABLE_OEM_REVISION,
    EFI_ARM_PROCESSOR_TABLE_CREATOR_ID,
    EFI_ARM_PROCESSOR_TABLE_CREATOR_REVISION,
    { 0 },
    0
  },   // ARM Processor table header
  0,   // Number of entries in ARM processor Table
  NULL // ARM Processor Table
};

/** Publish ARM Processor Data table in UEFI SYSTEM Table.
 * @param  HobStart               Pointer to the beginning of the HOB List from PEI.
 *
 * Description : This function iterates through HOB list and finds ARM processor Table Entry HOB.
 *               If  the ARM processor Table Entry HOB is found, the HOB data is copied to run-time memory
 *               and a pointer is assigned to it in ARM processor table. Then the ARM processor table is
 *               installed in EFI configuration table.
**/
VOID
EFIAPI
PublishArmProcessorTable (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = GetHobList ();

  // Iterate through the HOBs and find if there is ARM PROCESSOR ENTRY HOB
  for ( ; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    // Check for Correct HOB type
    if ((GET_HOB_TYPE (Hob)) == EFI_HOB_TYPE_GUID_EXTENSION) {
      // Check for correct GUID type
      if (CompareGuid (&(Hob.Guid->Name), &gArmMpCoreInfoGuid)) {
        ARM_PROCESSOR_TABLE  *ArmProcessorTable;
        EFI_STATUS           Status;

        // Allocate Runtime memory for ARM processor table
        ArmProcessorTable = (ARM_PROCESSOR_TABLE *)AllocateRuntimePool (sizeof (ARM_PROCESSOR_TABLE));

        // Check if the memory allocation is successful or not
        ASSERT (NULL != ArmProcessorTable);

        // Set ARM processor table to default values
        CopyMem (ArmProcessorTable, &mArmProcessorTableTemplate, sizeof (ARM_PROCESSOR_TABLE));

        // Fill in Length fields of ARM processor table
        ArmProcessorTable->Header.Length  = sizeof (ARM_PROCESSOR_TABLE);
        ArmProcessorTable->Header.DataLen = GET_GUID_HOB_DATA_SIZE (Hob);

        // Fill in Identifier(ARM processor table GUID)
        ArmProcessorTable->Header.Identifier = gArmMpCoreInfoGuid;

        // Set Number of ARM core entries in the Table
        ArmProcessorTable->NumberOfEntries = GET_GUID_HOB_DATA_SIZE (Hob)/sizeof (ARM_CORE_INFO);

        // Allocate runtime memory for ARM processor Table entries
        ArmProcessorTable->ArmCpus = (ARM_CORE_INFO *)AllocateRuntimePool (
                                                        ArmProcessorTable->NumberOfEntries * sizeof (ARM_CORE_INFO)
                                                        );

        // Check if the memory allocation is successful or not
        ASSERT (NULL != ArmProcessorTable->ArmCpus);

        // Copy ARM Processor Table data from HOB list to newly allocated memory
        CopyMem (ArmProcessorTable->ArmCpus, GET_GUID_HOB_DATA (Hob), ArmProcessorTable->Header.DataLen);

        // Install the ARM Processor table into EFI system configuration table
        Status = gBS->InstallConfigurationTable (&gArmMpCoreInfoGuid, ArmProcessorTable);

        ASSERT_EFI_ERROR (Status);
      }
    }
  }
}
