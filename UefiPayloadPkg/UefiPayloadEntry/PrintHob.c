/** @file
  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UefiPayloadEntry.h"
#include <UniversalPayload/AcpiTable.h>
#include <UniversalPayload/SerialPortInfo.h>
#include <UniversalPayload/PciRootBridges.h>
#include <UniversalPayload/ExtraData.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/AcpiBoardInfoGuid.h>
#include <Guid/BootManagerMenu.h>

#define ROW_LIMITER 16

typedef
EFI_STATUS
(*HOB_PRINT_HANDLER) (
  IN  VOID            *Hob,
  IN  UINT16          HobLength
);

typedef struct{
  UINT16               Type;
  CHAR8                *Name;
  HOB_PRINT_HANDLER    PrintHandler;
} HOB_PRINT_HANDLER_TABLE;

CHAR8 * mMemoryTypeStr[] = {
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

CHAR8 * mResource_Type_List[] = {
  "EFI_RESOURCE_SYSTEM_MEMORY          ", //0x00000000
  "EFI_RESOURCE_MEMORY_MAPPED_IO       ", //0x00000001
  "EFI_RESOURCE_IO                     ", //0x00000002
  "EFI_RESOURCE_FIRMWARE_DEVICE        ", //0x00000003
  "EFI_RESOURCE_MEMORY_MAPPED_IO_PORT  ", //0x00000004
  "EFI_RESOURCE_MEMORY_RESERVED        ", //0x00000005
  "EFI_RESOURCE_IO_RESERVED            ", //0x00000006
  "EFI_RESOURCE_MAX_MEMORY_TYPE        "  //0x00000007
};

typedef
EFI_STATUS
(*GUID_HOB_PRINT) (
  IN  UINT8          *HobRaw,
  IN  UINT16         HobLength
);

typedef struct {
  EFI_GUID          *Guid;
  GUID_HOB_PRINT    PrintHandler;
  CHAR8             *GuidName;
} GUID_HOB_PRINT_HANDLE;

typedef struct{
 EFI_GUID               *Guid;
 CHAR8                  *Type;
} PRINT_MEMORY_ALLOCCATION_HOB;


/**
  Print the Hex value of a given range.
  @param[in]  DataStart      A pointer to the start of data to be printed.
  @param[in]  DataSize       The length of the data to be printed.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintHex (
  IN  UINT8         *DataStart,
  IN  UINT16         DataSize
  )
{
  UINTN  Index1;
  UINTN  Index2;
  UINT8  *StartAddr;

  StartAddr = DataStart;
  for (Index1 = 0; Index1 * ROW_LIMITER < DataSize; Index1++) {
    DEBUG ((DEBUG_VERBOSE, "   0x%04p:", (DataStart - StartAddr)));
    for (Index2 = 0; (Index2 < ROW_LIMITER) && (Index1 * ROW_LIMITER + Index2 < DataSize); Index2++){
      DEBUG ((DEBUG_VERBOSE, " %02x", *DataStart));
      DataStart++;
    }
    DEBUG ((DEBUG_VERBOSE, "\n"));
  }

  return EFI_SUCCESS;
}

/**
  Print the information in HandOffHob.

  @param[in]  HobStart       A pointer to the HOB of type EFI_HOB_TYPE_HANDOFF.
  @param[in]  HobLength      The length in bytes of HOB of type EFI_HOB_TYPE_HANDOFF.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintHandOffHob(
  IN  VOID          *HobStart,
  IN  UINT16        HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  ASSERT (HobLength >= sizeof (*Hob.HandoffInformationTable));
  DEBUG ((DEBUG_INFO, "   BootMode            = 0x%x\n",  Hob.HandoffInformationTable->BootMode));
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
  IN  VOID          *HobStart,
  IN  UINT16        HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *) HobStart;

  if(CompareGuid (&Hob.MemoryAllocation->AllocDescriptor.Name, &gEfiHobMemoryAllocStackGuid)) {
    ASSERT (HobLength >= sizeof (*Hob.MemoryAllocationStack));
    DEBUG ((DEBUG_INFO, "   Type              = EFI_HOB_MEMORY_ALLOCATION_STACK\n"));
  } else if (CompareGuid (&Hob.MemoryAllocation->AllocDescriptor.Name, &gEfiHobMemoryAllocBspStoreGuid)) {
    ASSERT (HobLength >= sizeof (*Hob.MemoryAllocationBspStore));
    DEBUG ((DEBUG_INFO, "   Type              = EFI_HOB_MEMORY_ALLOCATION_BSP_STORE\n"));
  } else if (CompareGuid (&Hob.MemoryAllocation->AllocDescriptor.Name, &gEfiHobMemoryAllocModuleGuid)) {
    ASSERT (HobLength >= sizeof (*Hob.MemoryAllocationModule));
    DEBUG ((DEBUG_INFO, "   Type              = EFI_HOB_MEMORY_ALLOCATION_MODULE\n"));
    DEBUG ((DEBUG_INFO, "   Module Name       = %g\n", Hob.MemoryAllocationModule->ModuleName));
    DEBUG ((DEBUG_INFO, "   Physical Address  = 0x%lx\n", Hob.MemoryAllocationModule->EntryPoint));
  } else {
    ASSERT (HobLength >= sizeof (*Hob.MemoryAllocation));
    DEBUG ((DEBUG_INFO, "   Type              = EFI_HOB_TYPE_MEMORY_ALLOCATION\n"));
  }
  DEBUG ((DEBUG_INFO, "   MemoryBaseAddress = 0x%lx\n", Hob.MemoryAllocationStack->AllocDescriptor.MemoryBaseAddress));
  DEBUG ((DEBUG_INFO, "   MemoryLength      = 0x%lx\n", Hob.MemoryAllocationStack->AllocDescriptor.MemoryLength));
  DEBUG ((DEBUG_INFO, "   MemoryType        = %a \n",   mMemoryTypeStr[Hob.MemoryAllocationStack->AllocDescriptor.MemoryType]));
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
  IN  VOID          *HobStart,
  IN  UINT16        HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *) HobStart;
  ASSERT (HobLength >= sizeof (*Hob.ResourceDescriptor));

  DEBUG ((DEBUG_INFO, "   ResourceType      = %a\n", mResource_Type_List[Hob.ResourceDescriptor->ResourceType]));
  if(!IsZeroGuid (&Hob.ResourceDescriptor->Owner)) {
    DEBUG ((DEBUG_INFO, " Owner             = %g\n", Hob.ResourceDescriptor->Owner));
  }
  DEBUG ((DEBUG_INFO, "   ResourceAttribute = 0x%x\n",  Hob.ResourceDescriptor->ResourceAttribute));
  DEBUG ((DEBUG_INFO, "   PhysicalStart     = 0x%lx\n", Hob.ResourceDescriptor->PhysicalStart));
  DEBUG ((DEBUG_INFO, "   ResourceLength    = 0x%lx\n", Hob.ResourceDescriptor->ResourceLength));
  return EFI_SUCCESS;
}

/**
  Print the information in Acpi Guid Hob.

  @param[in] HobRaw          A pointer to the start of gUniversalPayloadAcpiTableGuid HOB.
  @param[in] HobLength       The size of the HOB data buffer.

  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintAcpiGuidHob (
  IN  UINT8          *HobRaw,
  IN  UINT16         HobLength
  )
{
  UNIVERSAL_PAYLOAD_ACPI_TABLE         *AcpiTableHob;
  AcpiTableHob = (UNIVERSAL_PAYLOAD_ACPI_TABLE *) GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= AcpiTableHob->Header.Length);
  DEBUG ((DEBUG_INFO, "   Revision  = 0x%x\n",  AcpiTableHob->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length    = 0x%x\n",  AcpiTableHob->Header.Length));
  DEBUG ((DEBUG_INFO, "   Rsdp      = 0x%p\n",  (UINT64) AcpiTableHob->Rsdp));
  return EFI_SUCCESS;
}

/**
  Print the information in Serial Guid Hob.
  @param[in] HobRaw          A pointer to the start of gUniversalPayloadSerialPortInfoGuid HOB.
  @param[in] HobLength       The size of the HOB data buffer.

  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintSerialGuidHob (
  IN  UINT8          *HobRaw,
  IN  UINT16         HobLength
  )
{
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO         *SerialPortInfo;
  SerialPortInfo = (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO *) GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= SerialPortInfo->Header.Length);
  DEBUG ((DEBUG_INFO, "   Revision       = 0x%x\n",  SerialPortInfo->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length         = 0x%x\n",  SerialPortInfo->Header.Length));
  DEBUG ((DEBUG_INFO, "   UseMmio        = 0x%x\n",  SerialPortInfo->UseMmio));
  DEBUG ((DEBUG_INFO, "   RegisterStride = 0x%x\n",  SerialPortInfo->RegisterStride));
  DEBUG ((DEBUG_INFO, "   BaudRate       = %d\n",    SerialPortInfo->BaudRate));
  DEBUG ((DEBUG_INFO, "   RegisterBase   = 0x%lx\n", SerialPortInfo->RegisterBase));
  return EFI_SUCCESS;
}

/**
  Print the information in Smbios Guid Hob.
  @param[in] HobRaw          A pointer to the start of gUniversalPayloadSmbios3TableGuid HOB.
  @param[in] HobLength       The size of the HOB data buffer.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintSmbios3GuidHob (
  IN  UINT8          *HobRaw,
  IN  UINT16         HobLength
  )
{
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE         *SmBiosTable;
  SmBiosTable = (UNIVERSAL_PAYLOAD_SMBIOS_TABLE *) GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= SmBiosTable->Header.Length);
  DEBUG ((DEBUG_INFO, "   Revision         = 0x%x\n",  SmBiosTable->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length           = 0x%x\n",  SmBiosTable->Header.Length));
  DEBUG ((DEBUG_INFO, "   SmBiosEntryPoint = 0x%lx\n", (UINT64) SmBiosTable->SmBiosEntryPoint));
  return EFI_SUCCESS;
}

/**
  Print the information in Smbios Guid Hob.
  @param[in] HobRaw          A pointer to the start of gUniversalPayloadSmbiosTableGuid HOB.
  @param[in] HobLength       The size of the HOB data buffer.

  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintSmbiosTablGuidHob (
  IN  UINT8          *HobRaw,
  IN  UINT16         HobLength
  )
{
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE         *SmBiosTable;
  SmBiosTable = (UNIVERSAL_PAYLOAD_SMBIOS_TABLE *) GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= SmBiosTable->Header.Length);
  DEBUG ((DEBUG_INFO, "   Revision         = 0x%x\n",  SmBiosTable->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length           = 0x%x\n",  SmBiosTable->Header.Length));
  DEBUG ((DEBUG_INFO, "   SmBiosEntryPoint = 0x%lx\n", (UINT64) SmBiosTable->SmBiosEntryPoint));
  return EFI_SUCCESS;
}

/**
  Print the information in Acpi BoardInfo Guid Hob.
  @param[in] HobRaw          A pointer to the start of gUefiAcpiBoardInfoGuid HOB.
  @param[in] HobLength       The size of the HOB data buffer.

  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintAcpiBoardInfoGuidHob (
  IN  UINT8          *HobRaw,
  IN  UINT16         HobLength
  )
{
  ACPI_BOARD_INFO          *AcpBoardInfo;
  AcpBoardInfo = (ACPI_BOARD_INFO *) GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= sizeof (*AcpBoardInfo));
  DEBUG ((DEBUG_INFO, "   Revision        = 0x%x\n",  AcpBoardInfo->Revision));
  DEBUG ((DEBUG_INFO, "   Reserved0       = 0x%x\n",  AcpBoardInfo->Reserved0));
  DEBUG ((DEBUG_INFO, "   ResetValue      = 0x%x\n",  AcpBoardInfo->ResetValue));
  DEBUG ((DEBUG_INFO, "   PmEvtBase       = 0x%lx\n", AcpBoardInfo->PmEvtBase));
  DEBUG ((DEBUG_INFO, "   PmGpeEnBase     = 0x%lx\n", AcpBoardInfo->PmGpeEnBase));
  DEBUG ((DEBUG_INFO, "   PmCtrlRegBase   = 0x%lx\n", AcpBoardInfo->PmCtrlRegBase));
  DEBUG ((DEBUG_INFO, "   PmTimerRegBase  = 0x%lx\n", AcpBoardInfo->PmTimerRegBase));
  DEBUG ((DEBUG_INFO, "   ResetRegAddress = 0x%lx\n", AcpBoardInfo->ResetRegAddress));
  DEBUG ((DEBUG_INFO, "   PcieBaseAddress = 0x%lx\n", AcpBoardInfo->PcieBaseAddress));
  DEBUG ((DEBUG_INFO, "   PcieBaseSize    = 0x%lx\n", AcpBoardInfo->PcieBaseSize));
  return EFI_SUCCESS;
}

/**
  Print the information in Pci RootBridge Info Guid Hob.
  @param[in] HobRaw          A pointer to the start of gUniversalPayloadPciRootBridgeInfoGuid HOB.
  @param[in] HobLength       The size of the HOB data buffer.

  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintPciRootBridgeInfoGuidHob (
  IN  UINT8          *HobRaw,
  IN  UINT16         HobLength
  )
{
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES *PciRootBridges;
  UINTN                              Index;
  UINTN                              Length;
  Index = 0;
  PciRootBridges = (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES *) GET_GUID_HOB_DATA (HobRaw);
  Length = sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES) + PciRootBridges->Count * sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE);
  ASSERT (HobLength >= Length);
  DEBUG ((DEBUG_INFO, "   Revision         = 0x%x\n", PciRootBridges->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length           = 0x%x\n", PciRootBridges->Header.Length));
  DEBUG ((DEBUG_INFO, "   Count            = 0x%x\n", PciRootBridges->Count));
  DEBUG ((DEBUG_INFO, "   ResourceAssigned = %a\n",   (PciRootBridges->ResourceAssigned ? "True" : "False")));

  while(Index < PciRootBridges->Count) {
    DEBUG ((DEBUG_INFO, "   Root Bridge Index[%d]:\n", Index));
    DEBUG ((DEBUG_INFO, "   Segment                 = 0x%x\n",  PciRootBridges->RootBridge[Index].Segment));
    DEBUG ((DEBUG_INFO, "   Supports                = 0x%lx\n", PciRootBridges->RootBridge[Index].Supports));
    DEBUG ((DEBUG_INFO, "   Attributes              = 0x%lx\n", PciRootBridges->RootBridge[Index].Attributes));
    DEBUG ((DEBUG_INFO, "   DmaAbove4G              = 0x%x\n",  PciRootBridges->RootBridge[Index].DmaAbove4G));
    DEBUG ((DEBUG_INFO, "   NoExtendedConfigSpace   = 0x%x\n",  PciRootBridges->RootBridge[Index].NoExtendedConfigSpace));
    DEBUG ((DEBUG_INFO, "   AllocationAttributes    = 0x%lx\n", PciRootBridges->RootBridge[Index].AllocationAttributes));
    DEBUG ((DEBUG_INFO, "   Bus.Base                = 0x%lx\n", PciRootBridges->RootBridge[Index].Bus.Base));
    DEBUG ((DEBUG_INFO, "   Bus.Limit               = 0x%lx\n", PciRootBridges->RootBridge[Index].Bus.Limit));
    DEBUG ((DEBUG_INFO, "   Bus.Translation         = 0x%lx\n", PciRootBridges->RootBridge[Index].Bus.Translation));
    DEBUG ((DEBUG_INFO, "   Io.Base                 = 0x%lx\n", PciRootBridges->RootBridge[Index].Io.Base));
    DEBUG ((DEBUG_INFO, "   Io.Limit                = 0x%lx\n", PciRootBridges->RootBridge[Index].Io.Limit));
    DEBUG ((DEBUG_INFO, "   Io.Translation          = 0x%lx\n", PciRootBridges->RootBridge[Index].Io.Translation));
    DEBUG ((DEBUG_INFO, "   Mem.Base                = 0x%lx\n", PciRootBridges->RootBridge[Index].Mem.Base));
    DEBUG ((DEBUG_INFO, "   Mem.Limit               = 0x%lx\n", PciRootBridges->RootBridge[Index].Mem.Limit));
    DEBUG ((DEBUG_INFO, "   Mem.Translation         = 0x%lx\n", PciRootBridges->RootBridge[Index].Mem.Translation));
    DEBUG ((DEBUG_INFO, "   MemAbove4G.Base         = 0x%lx\n", PciRootBridges->RootBridge[Index].MemAbove4G.Base));
    DEBUG ((DEBUG_INFO, "   MemAbove4G.Limit        = 0x%lx\n", PciRootBridges->RootBridge[Index].MemAbove4G.Limit));
    DEBUG ((DEBUG_INFO, "   MemAbove4G.Translation  = 0x%lx\n", PciRootBridges->RootBridge[Index].MemAbove4G.Translation));
    DEBUG ((DEBUG_INFO, "   PMem.Base               = 0x%lx\n", PciRootBridges->RootBridge[Index].PMem.Base));
    DEBUG ((DEBUG_INFO, "   PMem.Limit              = 0x%lx\n", PciRootBridges->RootBridge[Index].PMem.Limit));
    DEBUG ((DEBUG_INFO, "   PMem.Translation        = 0x%lx\n", PciRootBridges->RootBridge[Index].PMem.Translation));
    DEBUG ((DEBUG_INFO, "   PMemAbove4G.Base        = 0x%lx\n", PciRootBridges->RootBridge[Index].PMemAbove4G.Base));
    DEBUG ((DEBUG_INFO, "   PMemAbove4G.Limit       = 0x%lx\n", PciRootBridges->RootBridge[Index].PMemAbove4G.Limit));
    DEBUG ((DEBUG_INFO, "   PMemAbove4G.Translation = 0x%lx\n", PciRootBridges->RootBridge[Index].PMemAbove4G.Translation));
    Index+=1;
  }
  return EFI_SUCCESS;
}

/**
  Print the information in Extra Data Guid Hob.
  @param[in]  HobRaw         A pointer to the start of gUniversalPayloadExtraDataGuid HOB.
  @param[in] HobLength       The size of the HOB data buffer.

  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintExtraDataGuidHob (
  IN  UINT8          *HobRaw,
  IN  UINT16         HobLength
  )
{
  UNIVERSAL_PAYLOAD_EXTRA_DATA *ExtraData;
  UINTN                        Index;
  UINTN                        Length;

  Index     = 0;
  ExtraData = (UNIVERSAL_PAYLOAD_EXTRA_DATA *) GET_GUID_HOB_DATA (HobRaw);
  Length = sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA) + ExtraData->Count * sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY);
  ASSERT (HobLength >= Length);
  DEBUG ((DEBUG_INFO, "   Revision  = 0x%x\n", ExtraData->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length    = 0x%x\n", ExtraData->Header.Length));
  DEBUG ((DEBUG_INFO, "   Count     = 0x%x\n", ExtraData->Count));

  while (Index < ExtraData->Count) {
    DEBUG ((DEBUG_INFO, "   Id[%d]     = %a\n",    Index,ExtraData->Entry[Index].Identifier));
    DEBUG ((DEBUG_INFO, "   Base[%d]   = 0x%lx\n", Index,ExtraData->Entry[Index].Base));
    DEBUG ((DEBUG_INFO, "   Size[%d]   = 0x%lx\n", Index,ExtraData->Entry[Index].Size));
    Index+=1;
  }
  return EFI_SUCCESS;
}

/**
  Print the information in MemoryTypeInfoGuidHob.
  @param[in] HobRaw          A pointer to the start of gEfiMemoryTypeInformationGuid HOB.
  @param[in] HobLength       The size of the HOB data buffer.

  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintMemoryTypeInfoGuidHob (
  IN  UINT8          *HobRaw,
  IN  UINT16         HobLength
  )
{
  EFI_MEMORY_TYPE_INFORMATION *MemoryTypeInfo;

  MemoryTypeInfo = (EFI_MEMORY_TYPE_INFORMATION *) GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= sizeof (*MemoryTypeInfo));
  DEBUG ((DEBUG_INFO, "   Type            = 0x%x\n", MemoryTypeInfo->Type));
  DEBUG ((DEBUG_INFO, "   NumberOfPages   = 0x%x\n", MemoryTypeInfo->NumberOfPages));
  return EFI_SUCCESS;
}

/**
  Print the information in EdkiiBootManagerMenuFileGuid.
  @param[in] HobRaw          A pointer to the start of gEdkiiBootManagerMenuFileGuid HOB.
  @param[in] HobLength       The size of the HOB data buffer.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintBootManagerMenuGuidHob (
  IN  UINT8          *HobRaw,
  IN  UINT16         HobLength
  )
{
  UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU *BootManagerMenuFile;

  BootManagerMenuFile = (UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU *) GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= sizeof (*BootManagerMenuFile));
  DEBUG ((DEBUG_INFO, "   Revision  = 0x%x\n", BootManagerMenuFile->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length    = 0x%x\n", BootManagerMenuFile->Header.Length));
  DEBUG ((DEBUG_INFO, "   FileName  = %g\n",   &BootManagerMenuFile->FileName));
  return EFI_SUCCESS;
}

//
// Mappint table for dump Guid Hob information.
// This table can be easily extented.
//
GUID_HOB_PRINT_HANDLE GuidHobPrintHandleTable[] = {
  {&gUniversalPayloadAcpiTableGuid,         PrintAcpiGuidHob,              "gUniversalPayloadAcpiTableGuid(ACPI table Guid)"},
  {&gUniversalPayloadSerialPortInfoGuid,    PrintSerialGuidHob,            "gUniversalPayloadSerialPortInfoGuid(Serial Port Info)"},
  {&gUniversalPayloadSmbios3TableGuid,      PrintSmbios3GuidHob,           "gUniversalPayloadSmbios3TableGuid(SmBios Guid)"},
  {&gUniversalPayloadSmbiosTableGuid,       PrintSmbiosTablGuidHob,        "gUniversalPayloadSmbiosTableGuid(SmBios Guid)"},
  {&gUefiAcpiBoardInfoGuid,                 PrintAcpiBoardInfoGuidHob,     "gUefiAcpiBoardInfoGuid(Acpi Guid)"},
  {&gUniversalPayloadPciRootBridgeInfoGuid, PrintPciRootBridgeInfoGuidHob, "gUniversalPayloadPciRootBridgeInfoGuid(Pci Guid)"},
  {&gEfiMemoryTypeInformationGuid,          PrintMemoryTypeInfoGuidHob,    "gEfiMemoryTypeInformationGuid(Memory Type Information Guid)"},
  {&gUniversalPayloadExtraDataGuid,         PrintExtraDataGuidHob,         "gUniversalPayloadExtraDataGuid(PayLoad Extra Data Guid)"},
  {&gEdkiiBootManagerMenuFileGuid,          PrintBootManagerMenuGuidHob,   "gEdkiiBootManagerMenuFileGuid(Boot Manager Menu File Guid)"}
};

/**
  Print the Guid Hob using related print handle function.
  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_GUID_EXTENSION.
  @param[in] HobLength       The length in bytes of the HOB of type EFI_HOB_TYPE_GUID_EXTENSION.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintGuidHob (
  IN  VOID          *HobStart,
  IN  UINT16        HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 Index;
  EFI_STATUS            Status;

  Hob.Raw = (UINT8 *) HobStart;
  ASSERT (HobLength >= sizeof (Hob.Guid));

  for (Index = 0; Index < ARRAY_SIZE (GuidHobPrintHandleTable); Index++) {
    if (CompareGuid (&Hob.Guid->Name, GuidHobPrintHandleTable[Index].Guid)) {
      DEBUG ((DEBUG_INFO, "   Guid   = %a\n", GuidHobPrintHandleTable[Index].GuidName));
      Status = GuidHobPrintHandleTable[Index].PrintHandler (Hob.Raw, GET_GUID_HOB_DATA_SIZE (Hob.Raw));
      return Status;
    }
  }
  DEBUG ((DEBUG_INFO, "   Name = %g\n", &Hob.Guid->Name));
  PrintHex (GET_GUID_HOB_DATA (Hob.Raw), GET_GUID_HOB_DATA_SIZE (Hob.Raw));
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
  IN  VOID          *HobStart,
  IN  UINT16        HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *) HobStart;
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
  IN  VOID          *HobStart,
  IN  UINT16        HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *) HobStart;
  ASSERT (HobLength >= sizeof (*Hob.Cpu));

  DEBUG ((DEBUG_INFO, "   SizeOfMemorySpace = 0x%lx\n", Hob.Cpu->SizeOfMemorySpace));
  DEBUG ((DEBUG_INFO, "   SizeOfIoSpace     = 0x%lx\n", Hob.Cpu->SizeOfIoSpace));
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
  IN  VOID          *HobStart,
  IN  UINT16        HobLength
  )
{
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
  IN  VOID          *HobStart,
  IN  UINT16        HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *) HobStart;
  ASSERT (HobLength >= sizeof (*Hob.FirmwareVolume2));

  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.FirmwareVolume2->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length      = 0x%lx\n", Hob.FirmwareVolume2->Length));
  DEBUG ((DEBUG_INFO, "   FvName      = %g\n",    &Hob.FirmwareVolume2->FvName));
  DEBUG ((DEBUG_INFO, "   FileName    = %g\n",    &Hob.FirmwareVolume2->FileName));
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
  IN  VOID          *HobStart,
  IN  UINT16        HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *) HobStart;
  ASSERT (HobLength >= sizeof (*Hob.Capsule));

  DEBUG ((DEBUG_INFO, "   BaseAddress = 0x%lx\n", Hob.Capsule->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length = 0x%lx\n",      Hob.Capsule->Length));
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
  IN  VOID          *HobStart,
  IN  UINT16        HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  ASSERT (HobLength >= sizeof (*Hob.FirmwareVolume3));

  DEBUG ((DEBUG_INFO, "   BaseAddress          = 0x%lx\n", Hob.FirmwareVolume3->BaseAddress));
  DEBUG ((DEBUG_INFO, "   Length               = 0x%lx\n", Hob.FirmwareVolume3->Length));
  DEBUG ((DEBUG_INFO, "   AuthenticationStatus = 0x%x\n",  Hob.FirmwareVolume3->AuthenticationStatus));
  DEBUG ((DEBUG_INFO, "   ExtractedFv          = %a\n",    (Hob.FirmwareVolume3->ExtractedFv ? "True" : "False")));
  DEBUG ((DEBUG_INFO, "   FVName               = %g\n",    &Hob.FirmwareVolume3->FvName));
  DEBUG ((DEBUG_INFO, "   FileName             = %g\n",    &Hob.FirmwareVolume3->FileName));
  return EFI_SUCCESS;
}

//
// Mappint table from Hob type to Hob print function.
//
HOB_PRINT_HANDLER_TABLE mHobHandles[] = {
 {EFI_HOB_TYPE_HANDOFF,             "EFI_HOB_TYPE_HANDOFF",              PrintHandOffHob},
 {EFI_HOB_TYPE_MEMORY_ALLOCATION,   "EFI_HOB_TYPE_MEMORY_ALLOCATION",    PrintMemoryAllocationHob},
 {EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, "EFI_HOB_TYPE_RESOURCE_DESCRIPTOR",  PrintResourceDiscriptorHob},
 {EFI_HOB_TYPE_GUID_EXTENSION,      "EFI_HOB_TYPE_GUID_EXTENSION",       PrintGuidHob},
 {EFI_HOB_TYPE_FV,                  "EFI_HOB_TYPE_FV",                   PrintFvHob},
 {EFI_HOB_TYPE_CPU,                 "EFI_HOB_TYPE_CPU",                  PrintCpuHob},
 {EFI_HOB_TYPE_MEMORY_POOL,         "EFI_HOB_TYPE_MEMORY_POOL",          PrintMemoryPoolHob},
 {EFI_HOB_TYPE_FV2,                 "EFI_HOB_TYPE_FV2",                  PrintFv2Hob},
 {EFI_HOB_TYPE_UEFI_CAPSULE,        "EFI_HOB_TYPE_UEFI_CAPSULE",         PrintCapsuleHob},
 {EFI_HOB_TYPE_FV3,                 "EFI_HOB_TYPE_FV3",                  PrintFv3Hob}
};


/**
  Print all HOBs info from the HOB list.
  @param[in] HobStart A pointer to the HOB list
  @return    The pointer to the HOB list.
**/
VOID
PrintHob (
  IN CONST VOID             *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 Count;
  UINTN                 Index;
  ASSERT (HobStart != NULL);

  Hob.Raw = (UINT8 *) HobStart;
  DEBUG ((DEBUG_INFO, "Print all Hob information from Hob 0x%p\n", Hob.Raw));

  Count = 0;
  //
  // Parse the HOB list to see which type it is, and print the information.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    for (Index = 0; Index < ARRAY_SIZE (mHobHandles); Index++) {
      if (Hob.Header->HobType == mHobHandles[Index].Type) {
        DEBUG ((DEBUG_INFO, "HOB[%d]: Type = %a, Offset = 0x%p, Length = 0x%x\n", Count, mHobHandles[Index].Name, (Hob.Raw - (UINT8 *) HobStart), Hob.Header->HobLength));
        mHobHandles[Index].PrintHandler (Hob.Raw, Hob.Header->HobLength);
        break;
      }
    }
    if (Index == ARRAY_SIZE (mHobHandles)) {
      DEBUG ((DEBUG_INFO, "HOB[%d]: Type = %d, Offset = 0x%p, Length = 0x%x\n", Count, Hob.Header->HobType, (Hob.Raw - (UINT8 *)HobStart), Hob.Header->HobLength));
      DEBUG ((DEBUG_INFO, "   Unkown Hob type\n"));
      PrintHex (Hob.Raw, Hob.Header->HobLength);
    }
    Count++;
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  DEBUG ((DEBUG_INFO, "There are totally %d Hobs, the End Hob address is %p\n", Count, Hob.Raw));
}
