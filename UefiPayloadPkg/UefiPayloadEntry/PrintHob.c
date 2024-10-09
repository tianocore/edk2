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
#include <Library/HobPrintLib.h>

typedef
EFI_STATUS
(*GUID_HOB_PRINT) (
  IN  UINT8   *HobRaw,
  IN  UINT16  HobLength
  );

typedef struct {
  EFI_GUID          *Guid;
  GUID_HOB_PRINT    PrintHandler;
  CHAR8             *GuidName;
} GUID_HOB_PRINT_HANDLE;

/**
  Print the information in Acpi Guid Hob.

  @param[in] HobRaw          A pointer to the start of gUniversalPayloadAcpiTableGuid HOB.
  @param[in] HobLength       The size of the HOB data buffer.

  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
PrintAcpiGuidHob (
  IN  UINT8   *HobRaw,
  IN  UINT16  HobLength
  )
{
  UNIVERSAL_PAYLOAD_ACPI_TABLE  *AcpiTableHob;

  AcpiTableHob = (UNIVERSAL_PAYLOAD_ACPI_TABLE *)GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= AcpiTableHob->Header.Length);
  DEBUG ((DEBUG_INFO, "   Revision  = 0x%x\n", AcpiTableHob->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length    = 0x%x\n", AcpiTableHob->Header.Length));
  DEBUG ((DEBUG_INFO, "   Rsdp      = 0x%p\n", (UINT64)AcpiTableHob->Rsdp));
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
  IN  UINT8   *HobRaw,
  IN  UINT16  HobLength
  )
{
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *SerialPortInfo;

  SerialPortInfo = (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO *)GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= SerialPortInfo->Header.Length);
  DEBUG ((DEBUG_INFO, "   Revision       = 0x%x\n", SerialPortInfo->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length         = 0x%x\n", SerialPortInfo->Header.Length));
  DEBUG ((DEBUG_INFO, "   UseMmio        = 0x%x\n", SerialPortInfo->UseMmio));
  DEBUG ((DEBUG_INFO, "   RegisterStride = 0x%x\n", SerialPortInfo->RegisterStride));
  DEBUG ((DEBUG_INFO, "   BaudRate       = %d\n", SerialPortInfo->BaudRate));
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
  IN  UINT8   *HobRaw,
  IN  UINT16  HobLength
  )
{
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE  *SmBiosTable;

  SmBiosTable = (UNIVERSAL_PAYLOAD_SMBIOS_TABLE *)GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= SmBiosTable->Header.Length);
  DEBUG ((DEBUG_INFO, "   Revision         = 0x%x\n", SmBiosTable->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length           = 0x%x\n", SmBiosTable->Header.Length));
  DEBUG ((DEBUG_INFO, "   SmBiosEntryPoint = 0x%lx\n", (UINT64)SmBiosTable->SmBiosEntryPoint));
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
  IN  UINT8   *HobRaw,
  IN  UINT16  HobLength
  )
{
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE  *SmBiosTable;

  SmBiosTable = (UNIVERSAL_PAYLOAD_SMBIOS_TABLE *)GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= SmBiosTable->Header.Length);
  DEBUG ((DEBUG_INFO, "   Revision         = 0x%x\n", SmBiosTable->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length           = 0x%x\n", SmBiosTable->Header.Length));
  DEBUG ((DEBUG_INFO, "   SmBiosEntryPoint = 0x%lx\n", (UINT64)SmBiosTable->SmBiosEntryPoint));
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
  IN  UINT8   *HobRaw,
  IN  UINT16  HobLength
  )
{
  ACPI_BOARD_INFO  *AcpBoardInfo;

  AcpBoardInfo = (ACPI_BOARD_INFO *)GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= sizeof (*AcpBoardInfo));
  DEBUG ((DEBUG_INFO, "   Revision        = 0x%x\n", AcpBoardInfo->Revision));
  DEBUG ((DEBUG_INFO, "   Reserved0       = 0x%x\n", AcpBoardInfo->Reserved0));
  DEBUG ((DEBUG_INFO, "   ResetValue      = 0x%x\n", AcpBoardInfo->ResetValue));
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
  IN  UINT8   *HobRaw,
  IN  UINT16  HobLength
  )
{
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *PciRootBridges;
  UINTN                               Index;
  UINTN                               Length;

  Index          = 0;
  PciRootBridges = (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES *)GET_GUID_HOB_DATA (HobRaw);
  Length         = sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES) + (PciRootBridges->Count * sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE));
  ASSERT (HobLength >= Length);
  DEBUG ((DEBUG_INFO, "   Revision         = 0x%x\n", PciRootBridges->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length           = 0x%x\n", PciRootBridges->Header.Length));
  DEBUG ((DEBUG_INFO, "   Count            = 0x%x\n", PciRootBridges->Count));
  DEBUG ((DEBUG_INFO, "   ResourceAssigned = %a\n", (PciRootBridges->ResourceAssigned ? "True" : "False")));

  while (Index < PciRootBridges->Count) {
    DEBUG ((DEBUG_INFO, "   Root Bridge Index[%d]:\n", Index));
    DEBUG ((DEBUG_INFO, "   Segment                 = 0x%x\n", PciRootBridges->RootBridge[Index].Segment));
    DEBUG ((DEBUG_INFO, "   Supports                = 0x%lx\n", PciRootBridges->RootBridge[Index].Supports));
    DEBUG ((DEBUG_INFO, "   Attributes              = 0x%lx\n", PciRootBridges->RootBridge[Index].Attributes));
    DEBUG ((DEBUG_INFO, "   DmaAbove4G              = 0x%x\n", PciRootBridges->RootBridge[Index].DmaAbove4G));
    DEBUG ((DEBUG_INFO, "   NoExtendedConfigSpace   = 0x%x\n", PciRootBridges->RootBridge[Index].NoExtendedConfigSpace));
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
    Index += 1;
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
  IN  UINT8   *HobRaw,
  IN  UINT16  HobLength
  )
{
  UNIVERSAL_PAYLOAD_EXTRA_DATA  *ExtraData;
  UINTN                         Index;
  UINTN                         Length;

  Index     = 0;
  ExtraData = (UNIVERSAL_PAYLOAD_EXTRA_DATA *)GET_GUID_HOB_DATA (HobRaw);
  Length    = sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA) + ExtraData->Count * sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY);
  ASSERT (HobLength >= Length);
  DEBUG ((DEBUG_INFO, "   Revision  = 0x%x\n", ExtraData->Header.Revision));
  DEBUG ((DEBUG_INFO, "   Length    = 0x%x\n", ExtraData->Header.Length));
  DEBUG ((DEBUG_INFO, "   Count     = 0x%x\n", ExtraData->Count));

  while (Index < ExtraData->Count) {
    DEBUG ((DEBUG_INFO, "   Id[%d]     = %a\n", Index, ExtraData->Entry[Index].Identifier));
    DEBUG ((DEBUG_INFO, "   Base[%d]   = 0x%lx\n", Index, ExtraData->Entry[Index].Base));
    DEBUG ((DEBUG_INFO, "   Size[%d]   = 0x%lx\n", Index, ExtraData->Entry[Index].Size));
    Index += 1;
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
  IN  UINT8   *HobRaw,
  IN  UINT16  HobLength
  )
{
  EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInfo;

  MemoryTypeInfo = (EFI_MEMORY_TYPE_INFORMATION *)GET_GUID_HOB_DATA (HobRaw);
  ASSERT (HobLength >= sizeof (*MemoryTypeInfo));
  DEBUG ((DEBUG_INFO, "   Type            = 0x%x\n", MemoryTypeInfo->Type));
  DEBUG ((DEBUG_INFO, "   NumberOfPages   = 0x%x\n", MemoryTypeInfo->NumberOfPages));
  return EFI_SUCCESS;
}

//
// Mappint table for dump Guid Hob information.
// This table can be easily extented.
//
GUID_HOB_PRINT_HANDLE  GuidHobPrintHandleTable[] = {
  { &gUniversalPayloadAcpiTableGuid,         PrintAcpiGuidHob,              "gUniversalPayloadAcpiTableGuid(ACPI table Guid)"             },
  { &gUniversalPayloadSerialPortInfoGuid,    PrintSerialGuidHob,            "gUniversalPayloadSerialPortInfoGuid(Serial Port Info)"       },
  { &gUniversalPayloadSmbios3TableGuid,      PrintSmbios3GuidHob,           "gUniversalPayloadSmbios3TableGuid(SmBios Guid)"              },
  { &gUniversalPayloadSmbiosTableGuid,       PrintSmbiosTablGuidHob,        "gUniversalPayloadSmbiosTableGuid(SmBios Guid)"               },
  { &gUefiAcpiBoardInfoGuid,                 PrintAcpiBoardInfoGuidHob,     "gUefiAcpiBoardInfoGuid(Acpi Guid)"                           },
  { &gUniversalPayloadPciRootBridgeInfoGuid, PrintPciRootBridgeInfoGuidHob, "gUniversalPayloadPciRootBridgeInfoGuid(Pci Guid)"            },
  { &gEfiMemoryTypeInformationGuid,          PrintMemoryTypeInfoGuidHob,    "gEfiMemoryTypeInformationGuid(Memory Type Information Guid)" },
  { &gUniversalPayloadExtraDataGuid,         PrintExtraDataGuidHob,         "gUniversalPayloadExtraDataGuid(PayLoad Extra Data Guid)"     }
};

/**
  Print the Guid Hob using related print handle function.
  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_GUID_EXTENSION.
  @param[in] HobLength       The length in bytes of the HOB of type EFI_HOB_TYPE_GUID_EXTENSION.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval EFI_UNSUPPORTED    If the HOB GUID is not supported.
**/
EFI_STATUS
InternalPrintGuidHob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 Index;
  EFI_STATUS            Status;

  Hob.Raw = (UINT8 *)HobStart;
  ASSERT (HobLength >= sizeof (Hob.Guid));

  for (Index = 0; Index < ARRAY_SIZE (GuidHobPrintHandleTable); Index++) {
    if (CompareGuid (&Hob.Guid->Name, GuidHobPrintHandleTable[Index].Guid)) {
      DEBUG ((DEBUG_INFO, "   Guid   = %a\n", GuidHobPrintHandleTable[Index].GuidName));
      Status = GuidHobPrintHandleTable[Index].PrintHandler (Hob.Raw, GET_GUID_HOB_DATA_SIZE (Hob.Raw));
      return Status;
    }
  }

  return EFI_UNSUPPORTED;
}

/**
  Print the information in MemoryPoolHob.
  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_MEMORY_POOL.
  @param[in] HobLength       The length in bytes of the HOB of type EFI_HOB_TYPE_MEMORY_POOL.
  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
InternalPrintMemoryPoolHob (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  return EFI_SUCCESS;
}

/**
  HOB Print Handler to print Guid Hob.

  @param[in] HobStart        A pointer to the HOB of type EFI_HOB_TYPE_GUID_EXTENSION.
  @param[in] HobLength       The length in bytes of the HOB of type EFI_HOB_TYPE_GUID_EXTENSION.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval EFI_UNSUPPORTED    If the HOB type is not supported.
**/
EFI_STATUS
InternalPrintHobs (
  IN  VOID    *HobStart,
  IN  UINT16  HobLength
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = (UINT8 *)HobStart;

  if (Hob.Header->HobType == EFI_HOB_TYPE_GUID_EXTENSION) {
    return InternalPrintGuidHob (Hob.Raw, HobLength);
  } else if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_POOL) {
    return InternalPrintMemoryPoolHob (Hob.Raw, HobLength);
  }

  return EFI_UNSUPPORTED;
}

/**
  Print all HOBs info from the HOB list.
  @param[in] HobStart A pointer to the HOB list
**/
VOID
PrintHob (
  IN CONST VOID  *HobStart
  )
{
  PrintHobList (HobStart, InternalPrintHobs);
}
