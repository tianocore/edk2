/** @file
  Prints all the HOBs.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/HobPrintLib.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/MemoryAllocationHob.h>

#define ROW_LIMITER  16

typedef struct {
  UINT16               Type;
  CHAR8                *Name;
  HOB_PRINT_HANDLER    PrintHandler;
} HOB_PRINT_HANDLER_TABLE;

CHAR8  *mMemoryTypeStr[] = {
  "EfiReservedMemoryType",
  "EfiLoaderCode",
  "EfiLoaderData",
  "EfiBootServicesCode",
  "EfiBootServicesData",
  "EfiRuntimeServicesCode",
  "EfiRuntimeServicesData",
  "EfiConventionalMemory",
  "EfiUnusableMemory",
  "EfiACPIReclaimMemory",
  "EfiACPIMemoryNVS",
  "EfiMemoryMappedIO",
  "EfiMemoryMappedIOPortSpace",
  "EfiPalCode",
  "EfiPersistentMemory",
  "EfiMaxMemoryType"
};

CHAR8  *mResource_Type_List[] = {
  "EFI_RESOURCE_SYSTEM_MEMORY          ", // 0x00000000
  "EFI_RESOURCE_MEMORY_MAPPED_IO       ", // 0x00000001
  "EFI_RESOURCE_IO                     ", // 0x00000002
  "EFI_RESOURCE_FIRMWARE_DEVICE        ", // 0x00000003
  "EFI_RESOURCE_MEMORY_MAPPED_IO_PORT  ", // 0x00000004
  "EFI_RESOURCE_MEMORY_RESERVED        ", // 0x00000005
  "EFI_RESOURCE_IO_RESERVED            ", // 0x00000006
  "EFI_RESOURCE_MAX_MEMORY_TYPE        "  // 0x00000007
};

/**
  Print the Hex value of a given range.

  @param[in]  ErrorLevel     Error Level to print the Hex value.
  @param[in]  DataStart      A pointer to the start of data to be printed.
  @param[in]  DataSize       The length of the data to be printed.

  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintHex (
  IN  UINT32  ErrorLevel,
  IN  UINT8   *DataStart,
  IN  UINT16  DataSize
  )
{
  UINTN  Index1;
  UINTN  Index2;
  UINT8  *StartAddr;

  StartAddr = DataStart;
  for (Index1 = 0; Index1 * ROW_LIMITER < DataSize; Index1++) {
    DEBUG ((ErrorLevel, "   0x%04p:", (DataStart - StartAddr)));
    for (Index2 = 0; (Index2 < ROW_LIMITER) && (Index1 * ROW_LIMITER + Index2 < DataSize); Index2++) {
      DEBUG ((ErrorLevel, " %02x", *DataStart));
      DataStart++;
    }

    DEBUG ((ErrorLevel, "\n"));
  }

  return EFI_SUCCESS;
}

/**
  Print the Hex value of the Invalid HOB.

  @param[in]  HobStart       A pointer to the Invalid HOB.
  @param[in]  HobLength      The length in bytes of the Invalid HOB.

  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintInvalidHob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  DEBUG ((DEBUG_ERROR, "   Invalid HOB. Full hex dump in below:\n"));
  PrintHex (DEBUG_ERROR, HobStart, HobLength);
  return RETURN_INVALID_PARAMETER;
}

/**
  Print the information in HandOffHob.

  @param[in]  HobStart       A pointer to the HOB of type EFI_HOB_TYPE_HANDOFF.
  @param[in]  HobLength      The length in bytes of HOB of type EFI_HOB_TYPE_HANDOFF.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintHandOffHob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *)HobStart;
  if (HobLength < sizeof (*Hob.HandoffInformationTable)) {
    return PrintInvalidHob (HobStart, HobLength);
  }

  DEBUG ((DEBUG_INFO, "   BootMode            = 0x%x\n", Hob.HandoffInformationTable->BootMode));
  DEBUG ((DEBUG_INFO, "   EfiMemoryTop        = 0x%lx\n", Hob.HandoffInformationTable->EfiMemoryTop));
  DEBUG ((DEBUG_INFO, "   EfiMemoryBottom     = 0x%lx\n", Hob.HandoffInformationTable->EfiMemoryBottom));
  DEBUG ((DEBUG_INFO, "   EfiFreeMemoryTop    = 0x%lx\n", Hob.HandoffInformationTable->EfiFreeMemoryTop));
  DEBUG ((DEBUG_INFO, "   EfiFreeMemoryBottom = 0x%lx\n", Hob.HandoffInformationTable->EfiFreeMemoryBottom));
  DEBUG ((DEBUG_INFO, "   EfiEndOfHobList     = 0x%lx\n", Hob.HandoffInformationTable->EfiEndOfHobList));
  return EFI_SUCCESS;
}

/**
  Print the information in Memory Allocation Hob.
  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_MEMORY_ALLOCATION.
  @param[in] HobLength       The length in bytes of HOB of type EFI_HOB_TYPE_MEMORY_ALLOCATION.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintMemoryAllocationHob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *)HobStart;

  if (CompareGuid (&Hob.MemoryAllocation->AllocDescriptor.Name, &gEfiHobMemoryAllocStackGuid)) {
    if (HobLength < sizeof (*Hob.MemoryAllocationStack)) {
      return PrintInvalidHob (HobStart, HobLength);
    }

    DEBUG ((DEBUG_INFO, "   Type              = EFI_HOB_MEMORY_ALLOCATION_STACK\n"));
  } else if (CompareGuid (&Hob.MemoryAllocation->AllocDescriptor.Name, &gEfiHobMemoryAllocBspStoreGuid)) {
    if (HobLength < sizeof (*Hob.MemoryAllocationBspStore)) {
      return PrintInvalidHob (HobStart, HobLength);
    }

    DEBUG ((DEBUG_INFO, "   Type              = EFI_HOB_MEMORY_ALLOCATION_BSP_STORE\n"));
  } else if (CompareGuid (&Hob.MemoryAllocation->AllocDescriptor.Name, &gEfiHobMemoryAllocModuleGuid)) {
    if (HobLength < sizeof (*Hob.MemoryAllocationModule)) {
      return PrintInvalidHob (HobStart, HobLength);
    }

    DEBUG ((DEBUG_INFO, "   Type              = EFI_HOB_MEMORY_ALLOCATION_MODULE\n"));
    DEBUG ((DEBUG_INFO, "   ModuleName        = %g\n", &Hob.MemoryAllocationModule->ModuleName));
    DEBUG ((DEBUG_INFO, "   EntryPoint        = 0x%lx\n", Hob.MemoryAllocationModule->EntryPoint));
  } else {
    if (HobLength < sizeof (*Hob.MemoryAllocation)) {
      return PrintInvalidHob (HobStart, HobLength);
    }

    DEBUG ((DEBUG_INFO, "   Type              = EFI_HOB_TYPE_MEMORY_ALLOCATION\n"));
  }

  DEBUG ((DEBUG_INFO, "   Name              = %g\n", &Hob.MemoryAllocationStack->AllocDescriptor.Name));
  DEBUG ((DEBUG_INFO, "   MemoryBaseAddress = 0x%lx\n", Hob.MemoryAllocationStack->AllocDescriptor.MemoryBaseAddress));
  DEBUG ((DEBUG_INFO, "   MemoryLength      = 0x%lx\n", Hob.MemoryAllocationStack->AllocDescriptor.MemoryLength));
  DEBUG ((DEBUG_INFO, "   MemoryType        = %a \n", mMemoryTypeStr[Hob.MemoryAllocationStack->AllocDescriptor.MemoryType]));
  return EFI_SUCCESS;
}

/**
  Print the information in Resource Discriptor Hob.
  @param[in]  HobStart       A pointer to HOB of type EFI_HOB_TYPE_RESOURCE_DESCRIPTOR.
  @param[in]  HobLength      The Length in bytes of HOB of type EFI_HOB_TYPE_RESOURCE_DESCRIPTOR.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintResourceDiscriptorHob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *)HobStart;
  ASSERT (HobLength >= sizeof (*Hob.ResourceDescriptor));

  DEBUG ((DEBUG_INFO, "   ResourceType      = %a\n", mResource_Type_List[Hob.ResourceDescriptor->ResourceType]));
  if (!IsZeroGuid (&Hob.ResourceDescriptor->Owner)) {
    DEBUG ((DEBUG_INFO, "   Owner             = %g\n", &Hob.ResourceDescriptor->Owner));
  }

  DEBUG ((DEBUG_INFO, "   ResourceAttribute = 0x%x\n", Hob.ResourceDescriptor->ResourceAttribute));
  DEBUG ((DEBUG_INFO, "   PhysicalStart     = 0x%lx\n", Hob.ResourceDescriptor->PhysicalStart));
  DEBUG ((DEBUG_INFO, "   ResourceLength    = 0x%lx\n", Hob.ResourceDescriptor->ResourceLength));
  return EFI_SUCCESS;
}

/**
  Print the Guid Hob using related print handle function.
  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_GUID_EXTENSION.
  @param[in] HobLength       The length in bytes of the HOB of type EFI_HOB_TYPE_GUID_EXTENSION.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintGuidHob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINT16                DataLength;

  Hob.Raw = (UINT8 *)HobStart;
  ASSERT (HobLength >= sizeof (*Hob.Guid));

  DataLength = GET_GUID_HOB_DATA_SIZE (Hob.Raw);

  DEBUG ((DEBUG_INFO, "   Name       = %g\n", &Hob.Guid->Name));
  DEBUG ((DEBUG_INFO, "   DataLength = 0x%x\n", DataLength));
  PrintHex (DEBUG_VERBOSE, GET_GUID_HOB_DATA (Hob.Raw), DataLength);
  return EFI_SUCCESS;
}

/**
  Print the information in FV Hob.
  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_FV.
  @param[in] HobLength       The length in bytes of the HOB of type EFI_HOB_TYPE_FV.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintFvHob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *)HobStart;
  ASSERT (HobLength >= sizeof (*Hob.FirmwareVolume));

  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.FirmwareVolume->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length      = 0x%lx\n", Hob.FirmwareVolume->Length));
  return EFI_SUCCESS;
}

/**
  Print the information in Cpu Hob.
  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_CPU.
  @param[in] HobLength       The length in bytes of the HOB of type EFI_HOB_TYPE_CPU.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintCpuHob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *)HobStart;
  ASSERT (HobLength >= sizeof (*Hob.Cpu));

  DEBUG ((DEBUG_INFO, "   SizeOfMemorySpace = 0x%x\n", Hob.Cpu->SizeOfMemorySpace));
  DEBUG ((DEBUG_INFO, "   SizeOfIoSpace     = 0x%x\n", Hob.Cpu->SizeOfIoSpace));
  return EFI_SUCCESS;
}

/**
  Print the information in MemoryPoolHob.
  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_MEMORY_POOL.
  @param[in] HobLength       The length in bytes of the HOB of type EFI_HOB_TYPE_MEMORY_POOL.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintMemoryPoolHob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINT16                AllocationSize;

  Hob.Raw = (UINT8 *)HobStart;
  ASSERT (HobLength >= sizeof (*Hob.Pool));

  AllocationSize = HobLength - sizeof (EFI_HOB_GENERIC_HEADER);
  DEBUG ((DEBUG_INFO, "   AllocationSize    = 0x%lx\n", AllocationSize));

  PrintHex (DEBUG_VERBOSE, Hob.Raw + sizeof (EFI_HOB_GENERIC_HEADER), AllocationSize);

  return EFI_SUCCESS;
}

/**
  Print the information in Fv2Hob.
  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_FV2.
  @param[in] HobLength       The length in bytes of the HOB of type EFI_HOB_TYPE_FV2.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintFv2Hob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *)HobStart;
  ASSERT (HobLength >= sizeof (*Hob.FirmwareVolume2));

  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.FirmwareVolume2->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length      = 0x%lx\n", Hob.FirmwareVolume2->Length));
  DEBUG ((DEBUG_INFO, "   FvName      = %g\n", &Hob.FirmwareVolume2->FvName));
  DEBUG ((DEBUG_INFO, "   FileName    = %g\n", &Hob.FirmwareVolume2->FileName));
  return EFI_SUCCESS;
}

/**
  Print the information in Capsule Hob.
  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_UEFI_CAPSULE.
  @param[in] HobLength       The length in bytes of the HOB of type EFI_HOB_TYPE_UEFI_CAPSULE.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintCapsuleHob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *)HobStart;
  ASSERT (HobLength >= sizeof (*Hob.Capsule));

  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.Capsule->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n", Hob.Capsule->Length));
  return EFI_SUCCESS;
}

/**
  Print the information in Fv3 Hob.
  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_FV3.
  @param[in] HobLength       The length in bytes of the HOB of type EFI_HOB_TYPE_FV3.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintFv3Hob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *)HobStart;
  ASSERT (HobLength >= sizeof (*Hob.FirmwareVolume3));

  DEBUG ((DEBUG_INFO, "   BaseAddress          = 0x%lx\n", Hob.FirmwareVolume3->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length               = 0x%lx\n", Hob.FirmwareVolume3->Length));
  DEBUG ((DEBUG_INFO, "   AuthenticationStatus = 0x%x\n", Hob.FirmwareVolume3->AuthenticationStatus));
  DEBUG ((DEBUG_INFO, "   ExtractedFv          = %a\n", (Hob.FirmwareVolume3->ExtractedFv ? "True" : "False")));
  DEBUG ((DEBUG_INFO, "   FvName               = %g\n", &Hob.FirmwareVolume3->FvName));
  DEBUG ((DEBUG_INFO, "   FileName             = %g\n", &Hob.FirmwareVolume3->FileName));
  return EFI_SUCCESS;
}

//
// Mapping table from Hob type to Hob print function.
//
HOB_PRINT_HANDLER_TABLE  mHobHandles[] = {
  { EFI_HOB_TYPE_HANDOFF,             "EFI_HOB_TYPE_HANDOFF",             PrintHandOffHob            },
  { EFI_HOB_TYPE_MEMORY_ALLOCATION,   "EFI_HOB_TYPE_MEMORY_ALLOCATION",   PrintMemoryAllocationHob   },
  { EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, "EFI_HOB_TYPE_RESOURCE_DESCRIPTOR", PrintResourceDiscriptorHob },
  { EFI_HOB_TYPE_GUID_EXTENSION,      "EFI_HOB_TYPE_GUID_EXTENSION",      PrintGuidHob               },
  { EFI_HOB_TYPE_FV,                  "EFI_HOB_TYPE_FV",                  PrintFvHob                 },
  { EFI_HOB_TYPE_CPU,                 "EFI_HOB_TYPE_CPU",                 PrintCpuHob                },
  { EFI_HOB_TYPE_MEMORY_POOL,         "EFI_HOB_TYPE_MEMORY_POOL",         PrintMemoryPoolHob         },
  { EFI_HOB_TYPE_FV2,                 "EFI_HOB_TYPE_FV2",                 PrintFv2Hob                },
  { EFI_HOB_TYPE_UEFI_CAPSULE,        "EFI_HOB_TYPE_UEFI_CAPSULE",        PrintCapsuleHob            },
  { EFI_HOB_TYPE_FV3,                 "EFI_HOB_TYPE_FV3",                 PrintFv3Hob                }
};

/**
  Print all HOBs info from the HOB list.

  @param[in] HobStart       A pointer to the HOB list.
  @param[in] PrintHandler   A custom handler to print HOB info.

**/
VOID
EFIAPI
PrintHobList (
  IN CONST VOID         *HobStart,
  IN HOB_PRINT_HANDLER  PrintHandler
  )
{
  EFI_STATUS            Status;
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 Count;
  UINTN                 Index;

  ASSERT (HobStart != NULL);

  Hob.Raw = (UINT8 *)HobStart;
  DEBUG ((DEBUG_INFO, "Print all Hob information from Hob 0x%p\n", Hob.Raw));

  Status = EFI_SUCCESS;
  Count  = 0;
  //
  // Parse the HOB list to see which type it is, and print the information.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    //
    // Print HOB generic information
    //
    for (Index = 0; Index < ARRAY_SIZE (mHobHandles); Index++) {
      if (Hob.Header->HobType == mHobHandles[Index].Type) {
        DEBUG ((DEBUG_INFO, "HOB[%d]: Type = %a, Offset = 0x%p, Length = 0x%x\n", Count, mHobHandles[Index].Name, (Hob.Raw - (UINT8 *)HobStart), Hob.Header->HobLength));
        break;
      }
    }

    if (Index == ARRAY_SIZE (mHobHandles)) {
      DEBUG ((DEBUG_INFO, "HOB[%d]: Type = %d, Offset = 0x%p, Length = 0x%x\n", Count, Hob.Header->HobType, (Hob.Raw - (UINT8 *)HobStart), Hob.Header->HobLength));
    }

    //
    // Process custom HOB print handler first
    //
    if (PrintHandler != NULL) {
      Status = PrintHandler (Hob.Raw, Hob.Header->HobLength);
    }

    //
    // Process internal HOB print handler
    //
    if ((PrintHandler == NULL) || EFI_ERROR (Status)) {
      if (Index < ARRAY_SIZE (mHobHandles)) {
        mHobHandles[Index].PrintHandler (Hob.Raw, Hob.Header->HobLength);
      } else {
        DEBUG ((DEBUG_INFO, "   Unkown Hob type, full hex dump in below:\n"));
        PrintHex (DEBUG_INFO, Hob.Raw, Hob.Header->HobLength);
      }
    }

    Count++;
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  DEBUG ((DEBUG_INFO, "There are totally %d Hobs, the End Hob address is %p\n", Count, Hob.Raw));
}
