/** @file
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <IndustryStandard/Pci22.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>
#include <Library/FdtLib.h>
#include <Library/PciLib.h>
#include <Library/PeiServicesLib.h>
#include <UniversalPayload/UniversalPayload.h>
#include <UniversalPayload/AcpiTable.h>
#include <UniversalPayload/SerialPortInfo.h>
#include <UniversalPayload/SmbiosTable.h>
#include <UniversalPayload/PciRootBridges.h>
#include <Guid/GraphicsInfoHob.h>
#include <Guid/UniversalPayloadSerialPortDeviceParentInfo.h>
#include <Guid/UniversalPayloadBase.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Ppi/PciDevice.h>

#define IGD_BUS_NUM  0x00
#define IGD_DEV_NUM  0x02
#define IGD_FUN_NUM  0x00

EDKII_PCI_DEVICE_PPI  *mPciDevicePpi;
BOOLEAN               mResourceAssigned;

CHAR8  *mMemoryAllocType[] = {
  "Reserved",
  "LoaderCode",
  "LoaderData",
  "boot-code",
  "boot-data",
  "runtime-code",
  "runtime-data",
  "ConventionalMemory",
  "UnusableMemory",
  "acpi",
  "acpi-nvs",
  "mmio",
  "MemoryMappedIOPortSpace",
  "PalCode",
  "PersistentMemory",
};

/**
  The wrapper function of PeiServicesLocatePpi() for gEdkiiPeiPciDevicePpiGuid
   and Save the PPI to mPciDevicePpi.
  @retval EFI_SUCCESS        If it locate gEdkiiPeiPciDevicePpiGuid successfully.
  @retval EFI_NOT_FOUND      If it can't find gEdkiiPeiPciDevicePpiGuid.
**/
EFI_STATUS
EFIAPI
LocatePciDevicePpi (
  VOID
  )
{
  EFI_STATUS              Status;
  EFI_PEI_PPI_DESCRIPTOR  *PpiDescriptor;

  mPciDevicePpi = NULL;
  Status        = PeiServicesLocatePpi (
                    &gEdkiiPeiPciDevicePpiGuid,
                    0,
                    &PpiDescriptor,
                    (void **)&mPciDevicePpi
                    );
  if (EFI_ERROR (Status) || (mPciDevicePpi == NULL)) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  It will build FDT based on memory information from Hobs.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForMemory (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS                   Status;
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  VOID                         *HobStart;
  VOID                         *Fdt;
  INT32                        TempNode;
  CHAR8                        TempStr[32];
  UINT64                       RegTmp[2];

  Fdt = FdtBase;

  HobStart = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  //
  // Scan resource descriptor hobs to set memory nodes
  //
  for (Hob.Raw = HobStart; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      ResourceHob = Hob.ResourceDescriptor;
      // Memory
      if (ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
        // DEBUG ((DEBUG_ERROR, "Found hob for memory: base %016lX  length %016lX\n", ResourceHob->PhysicalStart, ResourceHob->ResourceLength));

        Status   = AsciiSPrint (TempStr, sizeof (TempStr), "memory@%lX", ResourceHob->PhysicalStart);
        TempNode = FdtAddSubnode (Fdt, 0, TempStr);
        ASSERT (TempNode > 0);

        RegTmp[0] = CpuToFdt64 (ResourceHob->PhysicalStart);
        RegTmp[1] = CpuToFdt64 (ResourceHob->ResourceLength);
        Status    = FdtSetProp (Fdt, TempNode, "reg", &RegTmp, sizeof (RegTmp));
        ASSERT_EFI_ERROR (Status);

        Status = FdtSetProp (Fdt, TempNode, "device_type", "memory", (UINT32)(AsciiStrLen ("memory")+1));
        ASSERT_EFI_ERROR (Status);
      }
    }
  }

  return Status;
}

/**
  It will build FDT based on memory allocation information from Hobs.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForMemAlloc (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS                      Status;
  EFI_PEI_HOB_POINTERS            Hob;
  VOID                            *HobStart;
  VOID                            *Fdt;
  INT32                           ParentNode;
  INT32                           TempNode;
  CHAR8                           TempStr[32];
  UINT64                          RegTmp[2];
  UINT32                          AllocMemType;
  EFI_GUID                        *AllocMemName;
  UINT8                           IsStackHob;
  UINT8                           IsBspStore;
  UINT32                          Data32;
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE  *SmbiosTable;
  UNIVERSAL_PAYLOAD_ACPI_TABLE    *AcpiTable;
  EFI_HOB_GUID_TYPE               *GuidHob;

  Fdt = FdtBase;

  ParentNode = FdtAddSubnode (Fdt, 0, "reserved-memory");
  ASSERT (ParentNode > 0);

  Data32 = CpuToFdt32 (2);
  Status = FdtSetProp (Fdt, ParentNode, "#address-cells", &Data32, sizeof (UINT32));
  Status = FdtSetProp (Fdt, ParentNode, "#size-cells", &Data32, sizeof (UINT32));

  GuidHob     = NULL;
  SmbiosTable = NULL;
  GuidHob     = GetFirstGuidHob (&gUniversalPayloadSmbios3TableGuid);
  if (GuidHob != NULL) {
    SmbiosTable = GET_GUID_HOB_DATA (GuidHob);
    DEBUG ((DEBUG_INFO, "To build Smbios memory FDT ,SmbiosTable :%lx, SmBiosEntryPoint :%lx\n", (UINTN)SmbiosTable, SmbiosTable->SmBiosEntryPoint));
    Status = AsciiSPrint (TempStr, sizeof (TempStr), "memory@%lX", SmbiosTable->SmBiosEntryPoint);
    DEBUG ((DEBUG_INFO, "To build Smbios memory FDT #2, SmbiosTable->Header.Length  :%x\n", SmbiosTable->Header.Length));
    TempNode = 0;
    TempNode = FdtAddSubnode (Fdt, ParentNode, TempStr);
    DEBUG ((DEBUG_INFO, "FdtAddSubnode %x", TempNode));
    RegTmp[0] = CpuToFdt64 (SmbiosTable->SmBiosEntryPoint);
    RegTmp[1] = CpuToFdt64 (SmbiosTable->Header.Length);
    FdtSetProp (Fdt, TempNode, "reg", &RegTmp, sizeof (RegTmp));
    ASSERT_EFI_ERROR (Status);
    FdtSetProp (Fdt, TempNode, "compatible", "smbios", (UINT32)(AsciiStrLen ("smbios")+1));
    ASSERT_EFI_ERROR (Status);
  }

  GuidHob = GetFirstGuidHob (&gUniversalPayloadAcpiTableGuid);
  if (GuidHob != NULL) {
    AcpiTable = (UNIVERSAL_PAYLOAD_ACPI_TABLE *)GET_GUID_HOB_DATA (GuidHob);
  }

  HobStart = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  //
  // Scan memory allocation hobs to set memory type
  //
  for (Hob.Raw = HobStart; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      AllocMemName = NULL;
      IsStackHob   = 0;
      IsBspStore   = 0;
      if (CompareGuid (&(Hob.MemoryAllocationModule->MemoryAllocationHeader.Name), &gEfiHobMemoryAllocModuleGuid)) {
        continue;
      } else if (IsZeroGuid (&(Hob.MemoryAllocationModule->MemoryAllocationHeader.Name)) == FALSE) {
        AllocMemName = &(Hob.MemoryAllocationModule->MemoryAllocationHeader.Name);

        if (CompareGuid (AllocMemName, &gEfiHobMemoryAllocStackGuid)) {
          IsStackHob = 1;
        } else if (CompareGuid (AllocMemName, &gEfiHobMemoryAllocBspStoreGuid)) {
          IsBspStore = 1;
        }
      }

      DEBUG ((
        DEBUG_ERROR,
        "Found hob for rsvd memory alloc: base %016lX  length %016lX  type %x\n",
        Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,
        Hob.MemoryAllocation->AllocDescriptor.MemoryLength,
        Hob.MemoryAllocation->AllocDescriptor.MemoryType
        ));

      AllocMemType = Hob.MemoryAllocation->AllocDescriptor.MemoryType;
      if (IsStackHob == 1) {
        Status = AsciiSPrint (
                   TempStr,
                   sizeof (TempStr),
                   "%a@%lX",
                   "stackhob",
                   Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress
                   );
      } else if (IsBspStore == 1) {
        Status = AsciiSPrint (
                   TempStr,
                   sizeof (TempStr),
                   "%a@%lX",
                   "bspstore",
                   Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress
                   );
      } else {
        Status = AsciiSPrint (
                   TempStr,
                   sizeof (TempStr),
                   "%a@%lX",
                   mMemoryAllocType[AllocMemType],
                   Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress
                   );
      }

      if (AsciiStrCmp (mMemoryAllocType[AllocMemType], "ConventionalMemory") == 0) {
        continue;
      }

      if (AsciiStrCmp (mMemoryAllocType[AllocMemType], "mmio") == 0) {
        Status = AsciiSPrint (TempStr, sizeof (TempStr), "mmio@%lX", Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress);
      } else {
        Status = AsciiSPrint (TempStr, sizeof (TempStr), "memory@%lX", Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress);
      }

      TempNode = FdtAddSubnode (Fdt, ParentNode, TempStr);
      DEBUG ((DEBUG_INFO, "FdtAddSubnode %x", TempNode));
      if (TempNode < 0) {
        continue;
      }

      RegTmp[0] = CpuToFdt64 (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress);
      RegTmp[1] = CpuToFdt64 (Hob.MemoryAllocation->AllocDescriptor.MemoryLength);
      Status    = FdtSetProp (Fdt, TempNode, "reg", &RegTmp, sizeof (RegTmp));
      ASSERT_EFI_ERROR (Status);

      if ((AsciiStrCmp (mMemoryAllocType[AllocMemType], "mmio") == 0)) {
        continue;
      }

      if (!(AsciiStrCmp (mMemoryAllocType[AllocMemType], "acpi-nvs") == 0) && (AsciiStrCmp (mMemoryAllocType[AllocMemType], "acpi") == 0)) {
        DEBUG ((DEBUG_INFO, "find acpi memory hob MemoryBaseAddress:%x , AcpiTable->Rsdp :%x\n", Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress, AcpiTable->Rsdp));
        if (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress == AcpiTable->Rsdp) {
          DEBUG ((DEBUG_INFO, "keep acpi memory hob  \n"));
          Status = FdtSetProp (Fdt, TempNode, "compatible", mMemoryAllocType[AllocMemType], (UINT32)(AsciiStrLen (mMemoryAllocType[AllocMemType])+1));
          ASSERT_EFI_ERROR (Status);
        } else {
          DEBUG ((DEBUG_INFO, "change acpi memory hob  \n"));
          Status = FdtSetProp (Fdt, TempNode, "compatible", mMemoryAllocType[4], (UINT32)(AsciiStrLen (mMemoryAllocType[4])+1));
          ASSERT_EFI_ERROR (Status);
        }
      } else {
        DEBUG ((DEBUG_INFO, "other memory hob  \n"));
        Status = FdtSetProp (Fdt, TempNode, "compatible", mMemoryAllocType[AllocMemType], (UINT32)(AsciiStrLen (mMemoryAllocType[AllocMemType])+1));
        ASSERT_EFI_ERROR (Status);
      }
    }
  }

  return Status;
}

/**
  It will build FDT based on serial information.
  @param[in] ISANode         ISANode.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForSerial (
  IN     INT32  ISANode,
  IN     VOID   *FdtBase
  )
{
  EFI_STATUS  Status;
  VOID        *Fdt;
  INT32       TempNode;
  UINT64      RegisterBase;
  CHAR8       TempStr[32];
  UINT32      RegData[3];
  UINT32      Data32;
  UINT64      Data64;

  Fdt          = FdtBase;
  RegisterBase = 0;

  //
  // Create SerialPortInfo FDT node.
  //
  Status   = AsciiSPrint (TempStr, sizeof (TempStr), "serial@%lX", (RegisterBase == 0) ? PcdGet64 (PcdSerialRegisterBase) : RegisterBase);
  TempNode = FdtAddSubnode (Fdt, ISANode, TempStr);
  ASSERT (TempNode > 0);

  Data32 = CpuToFdt32 (PcdGet32 (PcdSerialBaudRate));
  Status = FdtSetProp (Fdt, TempNode, "current-speed", &Data32, sizeof (Data32));
  ASSERT_EFI_ERROR (Status);

  if (PcdGetBool (PcdSerialUseMmio)) {
    Data32     = 0;
    RegData[0] = CpuToFdt32 (Data32);
  } else {
    Data32     = 1;
    RegData[0] = CpuToFdt32 (Data32);
  }

  Data64     = (RegisterBase == 0) ? PcdGet64 (PcdSerialRegisterBase) : RegisterBase;
  Data32     = (UINT32)((Data64 & 0x0FFFFFFFF));
  RegData[1] = CpuToFdt32 (Data32);
  RegData[2] = CpuToFdt32 (8);
  Status     = FdtSetProp (Fdt, TempNode, "reg", &RegData, sizeof (RegData));
  ASSERT_EFI_ERROR (Status);

  Data32 = CpuToFdt32 (1);
  Status = FdtSetProp (Fdt, TempNode, "reg-io-width", &Data32, sizeof (Data32));
  ASSERT_EFI_ERROR (Status);

  Status = FdtSetProp (Fdt, TempNode, "compatible", "isa", (UINT32)(AsciiStrLen ("isa")+1));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  It will build FDT based on serial information.

  @param[in] ISANode         ISANode.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForSerialLpss (
  IN     INT32  ISANode,
  IN     VOID   *FdtBase
  )
{
  EFI_HOB_GUID_TYPE                   *GuidHob;
  EFI_STATUS                          Status;
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *SerialPortInfo;
  VOID                                *Fdt;
  INT32                               TempNode;
  UINT32                              Data32;
  UINT32                              RegData[2];
  CHAR8                               TempStr[32];

  Status         = EFI_SUCCESS;
  SerialPortInfo = NULL;
  Fdt            = FdtBase;

  DEBUG ((DEBUG_INFO, "BuildFdtForSerialLpss start \n"));
  GuidHob = GetFirstGuidHob (&gUniversalPayloadSerialPortInfoGuid);
  while (GuidHob != NULL) {
    SerialPortInfo = (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO *)GET_GUID_HOB_DATA (GuidHob);

    if (!SerialPortInfo->UseMmio) {
      GuidHob = GET_NEXT_HOB (GuidHob);
      GuidHob = GetNextGuidHob (&gUniversalPayloadSerialPortInfoGuid, GuidHob);
      continue;
    }

    DEBUG ((DEBUG_INFO, "Create SerialPortInfo LPSS FDT node \n"));
    //
    // Create SerialPortInfo FDT node.
    //
    Status   = AsciiSPrint (TempStr, sizeof (TempStr), "serial@%lX", SerialPortInfo->RegisterBase);
    TempNode = FdtAddSubnode (Fdt, ISANode, TempStr);
    ASSERT (TempNode > 0);

    Data32 = CpuToFdt32 (SerialPortInfo->BaudRate);
    Status = FdtSetProp (Fdt, TempNode, "current-speed", &Data32, sizeof (Data32));
    ASSERT_EFI_ERROR (Status);

    RegData[0] = CpuToFdt32 ((UINT32)SerialPortInfo->RegisterBase);
    RegData[1] = CpuToFdt32 (0x80);
    Status     = FdtSetProp (Fdt, TempNode, "reg", &RegData, sizeof (RegData));
    ASSERT_EFI_ERROR (Status);

    Data32 = CpuToFdt32 (4);
    Status = FdtSetProp (Fdt, TempNode, "reg-io-width", &Data32, sizeof (Data32));
    ASSERT_EFI_ERROR (Status);

    Status = FdtSetProp (Fdt, TempNode, "compatible", "ns16550a", (UINT32)(AsciiStrLen ("ns16550a")+1));
    ASSERT_EFI_ERROR (Status);

    GuidHob = GET_NEXT_HOB (GuidHob);
    GuidHob = GetNextGuidHob (&gUniversalPayloadSerialPortInfoGuid, GuidHob);
  }

  return Status;
}

/**
  It will build FDT based on BuildFdtForPciRootBridge information.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForPciRootBridge (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS                                        Status;
  VOID                                              *Fdt;
  INT32                                             TempNode;
  INT32                                             GmaNode;
  INT32                                             eSPINode;
  CHAR8                                             TempStr[32];
  CHAR8                                             GmaStr[32];
  CHAR8                                             eSPIStr[32];
  UINT32                                            RegTmp[2];
  UINT32                                            RegData[21];
  UINT32                                            DMARegData[8];
  UINT64                                            Reg64Data[2];
  UINT32                                            Data32;
  UINT64                                            Data64;
  UINT8                                             BusNumber;
  UINT8                                             BusLimit;
  UINT8                                             BusBase;
  UINT8                                             DevBase;
  UINT8                                             FunBase;
  EFI_HOB_GUID_TYPE                                 *GuidHob;
  UNIVERSAL_PAYLOAD_GENERIC_HEADER                  *GenericHeader;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES                *PciRootBridgeInfo;
  UINT8                                             Index;
  PCI_TYPE00                                        PciData;
  UNIVERSAL_PAYLOAD_SERIAL_PORT_PARENT_DEVICE_INFO  *SerialParent;

  Fdt               = FdtBase;
  BusNumber         = 0;
  BusLimit          = 0;
  BusBase           = 0x80;
  DevBase           = 0x31;
  FunBase           = 0;
  Status            = EFI_SUCCESS;
  PciRootBridgeInfo = NULL;

  DEBUG ((DEBUG_INFO, "%a: #1 \n", __func__));
  //
  // Create BuildFdtForPciRootBridge FDT node.
  //

  GuidHob = GetFirstGuidHob (&gUniversalPayloadPciRootBridgeInfoGuid);
  if (GuidHob != NULL) {
    GenericHeader = (UNIVERSAL_PAYLOAD_GENERIC_HEADER *)GET_GUID_HOB_DATA (GuidHob);
    if ((sizeof (UNIVERSAL_PAYLOAD_GENERIC_HEADER) <= GET_GUID_HOB_DATA_SIZE (GuidHob)) && (GenericHeader->Length <= GET_GUID_HOB_DATA_SIZE (GuidHob))) {
      if ((GenericHeader->Revision == UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION) && (GenericHeader->Length >= sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES))) {
        DEBUG ((DEBUG_INFO, "%a: #2 \n", __func__));

        //
        // UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES structure is used when Revision equals to UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION
        //
        PciRootBridgeInfo = (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES *)GET_GUID_HOB_DATA (GuidHob);
      }
    }
  }

  GuidHob = GetFirstGuidHob (&gUniversalPayloadSerialPortParentDeviceInfoGuid);
  if (GuidHob != NULL) {
    SerialParent = (UNIVERSAL_PAYLOAD_SERIAL_PORT_PARENT_DEVICE_INFO *)GET_GUID_HOB_DATA (GuidHob);
    BusBase      = (SerialParent->ParentDevicePcieBaseAddress >> 20) & 0xFF;
    DevBase      = (SerialParent->ParentDevicePcieBaseAddress >> 15) & 0x1F;
    FunBase      = (SerialParent->ParentDevicePcieBaseAddress >> 12) & 0x07;
  }

  DEBUG ((DEBUG_INFO, "PciRootBridgeInfo->Count %x\n", PciRootBridgeInfo->Count));
  DEBUG ((DEBUG_INFO, "PciRootBridge->Segment %x, \n", PciRootBridgeInfo->RootBridge[0].Segment));

  DEBUG ((DEBUG_INFO, "PciRootBridge->Bus.Base %x, \n", PciRootBridgeInfo->RootBridge[0].Bus.Base));
  DEBUG ((DEBUG_INFO, "PciRootBridge->Bus.limit %x, \n", PciRootBridgeInfo->RootBridge[0].Bus.Limit));

  DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.Base %x, \n", PciRootBridgeInfo->RootBridge[0].Mem.Base));
  DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.limit %x, \n", PciRootBridgeInfo->RootBridge[0].Mem.Limit));

  DEBUG ((DEBUG_INFO, "PciRootBridge->MemAbove4G.Base %llx, \n", PciRootBridgeInfo->RootBridge[0].MemAbove4G.Base));
  DEBUG ((DEBUG_INFO, "PciRootBridge->MemAbove4G.limit %llx, \n", PciRootBridgeInfo->RootBridge[0].MemAbove4G.Limit));

  DEBUG ((DEBUG_INFO, "PciRootBridge->PMem.Base %llx, \n", PciRootBridgeInfo->RootBridge[0].PMem.Base));
  DEBUG ((DEBUG_INFO, "PciRootBridge->PMem.limit %llx, \n", PciRootBridgeInfo->RootBridge[0].PMem.Limit));

  DEBUG ((DEBUG_INFO, "PciRootBridge->Bus.Base %x, \n", PciRootBridgeInfo->RootBridge[1].Bus.Base));
  DEBUG ((DEBUG_INFO, "PciRootBridge->Bus.limit %x, \n", PciRootBridgeInfo->RootBridge[1].Bus.Limit));

  DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.Base %x, \n", PciRootBridgeInfo->RootBridge[1].Mem.Base));
  DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.limit %x, \n", PciRootBridgeInfo->RootBridge[1].Mem.Limit));

  DEBUG ((DEBUG_INFO, "PciRootBridge->MemAbove4G.Base %llx, \n", PciRootBridgeInfo->RootBridge[1].MemAbove4G.Base));
  DEBUG ((DEBUG_INFO, "PciRootBridge->MemAbove4G.limit %llx, \n", PciRootBridgeInfo->RootBridge[1].MemAbove4G.Limit));

  DEBUG ((DEBUG_INFO, "PciRootBridge->PMem.Base %x, \n", PciRootBridgeInfo->RootBridge[1].PMem.Base));
  DEBUG ((DEBUG_INFO, "PciRootBridge->PMem.limit %x, \n", PciRootBridgeInfo->RootBridge[1].PMem.Limit));

  if (PciRootBridgeInfo != NULL) {
    for (Index = 0; Index < PciRootBridgeInfo->Count; Index++) {
      UINTN  PciExpressBaseAddress;

      mResourceAssigned     = PciRootBridgeInfo->ResourceAssigned;
      PciExpressBaseAddress = PcdGet64 (PcdPciExpressBaseAddress) + (PCI_LIB_ADDRESS (PciRootBridgeInfo->RootBridge[Index].Bus.Base, 0, 0, 0));
      Status                = AsciiSPrint (TempStr, sizeof (TempStr), "pci-rb%d@%lX", Index, PciExpressBaseAddress);
      TempNode              = FdtAddSubnode (Fdt, 0, TempStr);
      ASSERT (TempNode > 0);
      SetMem (RegData, sizeof (RegData), 0);

      // non-reloc/non-prefetch/mmio, child-addr, parent-addr, length
      Data32     = (N_NON_RELOCATABLE + SS_32BIT_MEMORY_SPACE);
      RegData[0] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.Base RegData[0] %x, \n", Data32));

      // child-addr
      RegData[1] = CpuToFdt32 (0);
      Data32     = (UINT32)PciRootBridgeInfo->RootBridge[Index].Mem.Base;
      RegData[2] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.Base RegData[2] %x, \n", Data32));

      // parent-addr
      RegData[3] = CpuToFdt32 (0);
      RegData[4] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.Base RegData[4] %x, \n", Data32));

      // size
      Data64 = (PciRootBridgeInfo->RootBridge[Index].Mem.Limit - PciRootBridgeInfo->RootBridge[Index].Mem.Base + 1);
      if (Data64 & 0xFFFFFFFF00000000) {
        Data32 = (UINT32)RShiftU64 ((Data64 & 0xFFFFFFFF00000000), 31);
      } else {
        Data32 = 0;
      }

      DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.size RegData[5] %x, \n", Data32));
      RegData[5] = CpuToFdt32 (Data32);
      Data32     = (UINT32)((Data64 & 0x0FFFFFFFF));
      DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.size RegData[6] %x, \n", Data32));

      RegData[6] = CpuToFdt32 (Data32);

      // non-reloc/non-prefetch/64 mmio, child-addr, parent-addr, length
      Data32     = (N_NON_RELOCATABLE + SS_64BIT_MEMORY_SPACE);
      RegData[7] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->MemAbove4G.Base RegData[7] %x, \n", Data32));

      // child-addr
      Data64 = PciRootBridgeInfo->RootBridge[Index].MemAbove4G.Base;
      Data32 = (UINT32)RShiftU64 ((Data64 & 0xFFFFFFFF00000000), 32);

      RegData[8] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->MemAbove4G.Base RegData[8] %x, \n", Data32));
      Data32     = (UINT32)((Data64 & 0x0FFFFFFFF));
      RegData[9] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->MemAbove4G.Base RegData[9] %x, \n", Data32));

      // parent-addr
      RegData[10] = RegData[8];
      RegData[11] = RegData[9];

      // size
      Data64 = (PciRootBridgeInfo->RootBridge[Index].MemAbove4G.Limit - PciRootBridgeInfo->RootBridge[Index].MemAbove4G.Base + 1);
      if (Data64 & 0xFFFFFFFF00000000) {
        Data32 = (UINT32)RShiftU64 ((Data64 & 0xFFFFFFFF00000000), 32);
      } else {
        Data32 = 0;
      }

      RegData[12] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->MemAbove4G.size RegData[12] %x, \n", Data32));

      Data32      = (UINT32)((Data64 & 0x0FFFFFFFF));
      RegData[13] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->MemAbove4G.size RegData[13] %x, \n", Data32));

      // non-reloc/32bit/io, child-addr, parent-addr, length
      Data32 = (N_NON_RELOCATABLE + SS_IO_SPACE);

      RegData[14] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->Io.base RegData[14] %x, \n", Data32));

      Data32 = (UINT32)PciRootBridgeInfo->RootBridge[Index].Io.Base;
      // child-addr
      RegData[15] = CpuToFdt32 (0);
      RegData[16] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->Io.base RegData[16] %x, \n", Data32));

      // parent-addr
      RegData[17] = CpuToFdt32 (0);
      RegData[18] = CpuToFdt32 (Data32);
      // size
      Data64 = (PciRootBridgeInfo->RootBridge[Index].Io.Limit - PciRootBridgeInfo->RootBridge[Index].Io.Base + 1);
      if (Data64 & 0xFFFFFFFF00000000) {
        Data32 = (UINT32)RShiftU64 ((Data64 & 0xFFFFFFFF00000000), 32);
      } else {
        Data32 = 0;
      }

      RegData[19] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->Io.base size [19] %x, \n", Data32));

      Data32      = (UINT32)((Data64 & 0x0FFFFFFFF));
      RegData[20] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->Io.base size [20] %x, \n", Data32));

      Status = FdtSetProp (Fdt, TempNode, "ranges", &RegData, sizeof (RegData));
      ASSERT_EFI_ERROR (Status);

      // non-reloc/non-prefetch/memory, child-addr, parent-addr, length
      // indicate rb1 does not support above 4GB DMA
      Data32 = (N_NON_RELOCATABLE + SS_32BIT_MEMORY_SPACE);

      DMARegData[0] = CpuToFdt32 (Data32);
      DEBUG ((DEBUG_INFO, "PciRootBridge->DMA base  RegData[0] %x, \n", Data32));

      // child-addr
      DMARegData[2] = CpuToFdt32 (0);
      DMARegData[3] = CpuToFdt32 (0);
      // parent-addr
      DMARegData[4] = CpuToFdt32 (0);
      DMARegData[5] = CpuToFdt32 (0);
      // size
      DMARegData[6] = CpuToFdt32 (1);
      DMARegData[7] = CpuToFdt32 (0);

      Status = FdtSetProp (Fdt, TempNode, "dma-ranges", &DMARegData, sizeof (DMARegData));
      ASSERT_EFI_ERROR (Status);

      ASSERT (PciRootBridgeInfo->RootBridge[Index].Bus.Base <= 0xFF);
      ASSERT (PciRootBridgeInfo->RootBridge[Index].Bus.Limit <= 0xFF);

      Reg64Data[0] = CpuToFdt64 (PciExpressBaseAddress + LShiftU64 (PciRootBridgeInfo->RootBridge[Index].Bus.Base, 20));
      Reg64Data[1] = CpuToFdt64 (LShiftU64 (PciRootBridgeInfo->RootBridge[Index].Bus.Limit +1, 20));

      Status = FdtSetProp (Fdt, TempNode, "reg", &Reg64Data, sizeof (Reg64Data));
      ASSERT_EFI_ERROR (Status);

      BusNumber = PciRootBridgeInfo->RootBridge[Index].Bus.Base & 0xFF;
      RegTmp[0] = CpuToFdt32 (BusNumber);
      BusLimit  = PciRootBridgeInfo->RootBridge[Index].Bus.Limit & 0xFF;
      RegTmp[1] = CpuToFdt32 (BusLimit);
      DEBUG ((DEBUG_INFO, "PciRootBridge->BusNumber %x, \n", BusNumber));
      DEBUG ((DEBUG_INFO, "PciRootBridge->BusLimit  %x, \n", BusLimit));
      ASSERT (PciRootBridgeInfo->RootBridge[Index].Bus.Base <= 0xFF);
      ASSERT (PciRootBridgeInfo->RootBridge[Index].Bus.Limit <= 0xFF);

      Status = FdtSetProp (Fdt, TempNode, "bus-range", &RegTmp, sizeof (RegTmp));
      ASSERT_EFI_ERROR (Status);

      Data32 = CpuToFdt32 (2);
      Status = FdtSetProp (Fdt, TempNode, "#size-cells", &Data32, sizeof (UINT32));

      Reg64Data[0] = CpuToFdt64 (PciExpressBaseAddress + LShiftU64 (PciRootBridgeInfo->RootBridge[Index].Bus.Base, 20));
      Reg64Data[1] = CpuToFdt64 (LShiftU64 (PciRootBridgeInfo->RootBridge[Index].Bus.Limit +1, 20));

      Status = FdtSetProp (Fdt, TempNode, "reg", &Reg64Data, sizeof (Reg64Data));
      ASSERT_EFI_ERROR (Status);

      Data32 = CpuToFdt32 (3);
      Status = FdtSetProp (Fdt, TempNode, "#address-cells", &Data32, sizeof (UINT32));

      Status = FdtSetProp (Fdt, TempNode, "compatible", "pci-rb", (UINT32)(AsciiStrLen ("pci-rb")+1));
      ASSERT_EFI_ERROR (Status);

      if (Index == 0) {
        PciExpressBaseAddress = PcdGet64 (PcdPciExpressBaseAddress) + (PCI_LIB_ADDRESS (IGD_BUS_NUM, IGD_DEV_NUM, IGD_FUN_NUM, 0));
        Status                = AsciiSPrint (GmaStr, sizeof (GmaStr), "gma@%lX", PciExpressBaseAddress);
        GmaNode               = FdtAddSubnode (Fdt, TempNode, GmaStr);
        Status                = LocatePciDevicePpi ();
        if (!EFI_ERROR (Status)) {
          Status = mPciDevicePpi->PciIo.Pci.Read (
                                              &mPciDevicePpi->PciIo,
                                              (EFI_PCI_IO_PROTOCOL_WIDTH)EfiPciWidthUint16,
                                              PCI_VENDOR_ID_OFFSET,
                                              sizeof (PciData.Hdr.VendorId),
                                              &(PciData.Hdr.VendorId)
                                              );

          Status = mPciDevicePpi->PciIo.Pci.Read (
                                              &mPciDevicePpi->PciIo,
                                              (EFI_PCI_IO_PROTOCOL_WIDTH)EfiPciWidthUint16,
                                              PCI_DEVICE_ID_OFFSET,
                                              sizeof (PciData.Hdr.DeviceId),
                                              &(PciData.Hdr.DeviceId)
                                              );

          Status = mPciDevicePpi->PciIo.Pci.Read (
                                              &mPciDevicePpi->PciIo,
                                              (EFI_PCI_IO_PROTOCOL_WIDTH)EfiPciWidthUint8,
                                              PCI_REVISION_ID_OFFSET,
                                              sizeof (PciData.Hdr.RevisionID),
                                              &(PciData.Hdr.RevisionID)
                                              );

          Status = mPciDevicePpi->PciIo.Pci.Read (
                                              &mPciDevicePpi->PciIo,
                                              (EFI_PCI_IO_PROTOCOL_WIDTH)EfiPciWidthUint16,
                                              PCI_SVID_OFFSET,
                                              sizeof (PciData.Device.SubsystemVendorID),
                                              &(PciData.Device.SubsystemVendorID)
                                              );

          Status = mPciDevicePpi->PciIo.Pci.Read (
                                              &mPciDevicePpi->PciIo,
                                              (EFI_PCI_IO_PROTOCOL_WIDTH)EfiPciWidthUint16,
                                              PCI_SID_OFFSET,
                                              sizeof (PciData.Device.SubsystemID),
                                              &(PciData.Device.SubsystemID)
                                              );
        }

        Data32 = CpuToFdt32 (PciData.Device.SubsystemID);
        Status = FdtSetProp (Fdt, GmaNode, "subsystem-id", &Data32, sizeof (UINT32));

        Data32 = CpuToFdt32 (PciData.Device.SubsystemVendorID);
        Status = FdtSetProp (Fdt, GmaNode, "subsystem-vendor-id", &Data32, sizeof (UINT32));

        Data32 = CpuToFdt32 (PciData.Hdr.RevisionID);
        Status = FdtSetProp (Fdt, GmaNode, "revision-id", &Data32, sizeof (UINT32));

        Data32 = CpuToFdt32 (PciData.Hdr.DeviceId);
        Status = FdtSetProp (Fdt, GmaNode, "device-id", &Data32, sizeof (UINT32));

        Data32 = CpuToFdt32 (PciData.Hdr.VendorId);
        Status = FdtSetProp (Fdt, GmaNode, "vendor-id", &Data32, sizeof (UINT32));
      }

      if (SerialParent != NULL) {
        DEBUG ((DEBUG_INFO, "SerialParent->IsIsaCompatible  :%x , SerialParent->ParentDevicePcieBaseAddress :%x\n", SerialParent->IsIsaCompatible, SerialParent->ParentDevicePcieBaseAddress));
        DEBUG ((DEBUG_INFO, "BusBase  :%x , PciRootBridgeInfo->RootBridge[Index].Bus.Base :%x\n", BusBase, PciRootBridgeInfo->RootBridge[Index].Bus.Base));
      }

      {
        if ((BusBase >= PciRootBridgeInfo->RootBridge[Index].Bus.Base) && (BusBase <= PciRootBridgeInfo->RootBridge[Index].Bus.Limit)) {
          eSPINode = TempNode;
          if (SerialParent != NULL) {
            if (SerialParent->IsIsaCompatible) {
              Status   = AsciiSPrint (eSPIStr, sizeof (eSPIStr), "isa@%X,%X", DevBase, FunBase);
              eSPINode = FdtAddSubnode (Fdt, TempNode, eSPIStr);
              Status   = FdtSetProp (Fdt, eSPINode, "compatible", "isa", (UINT32)(AsciiStrLen ("isa")+1));
              ASSERT_EFI_ERROR (Status);
              Data32 = CpuToFdt32 (1);
              Status = FdtSetProp (Fdt, eSPINode, "#size-cells", &Data32, sizeof (UINT32));
              Data32 = CpuToFdt32 (2);
              Status = FdtSetProp (Fdt, eSPINode, "#address-cells", &Data32, sizeof (UINT32));
              Status = BuildFdtForSerial (eSPINode, FdtBase);
              ASSERT_EFI_ERROR (Status);
            }
          } else {
            Status = BuildFdtForSerialLpss (eSPINode, FdtBase);
            ASSERT_EFI_ERROR (Status);
          }
        }
      }
    }
  }

  DEBUG ((DEBUG_INFO, "%a: #3 \n", __func__));

  return Status;
}

/**
  It will build FDT based on FrameBuffer.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForFrameBuffer (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS                 Status;
  VOID                       *Fdt;
  INT32                      TempNode;
  UINT32                     Data32;
  CHAR8                      TempStr[32];
  UINT64                     RegData[2];
  EFI_HOB_GUID_TYPE          *GuidHob;
  EFI_PEI_GRAPHICS_INFO_HOB  *GraphicsInfo;

  Fdt = FdtBase;

  GuidHob = GetFirstGuidHob (&gEfiGraphicsInfoHobGuid);
  if (GuidHob != NULL) {
    GraphicsInfo = (EFI_PEI_GRAPHICS_INFO_HOB *)(GET_GUID_HOB_DATA (GuidHob));
    Status       = AsciiSPrint (TempStr, sizeof (TempStr), "framebuffer@%lX", GraphicsInfo->FrameBufferBase);
    TempNode     = FdtAddSubnode (Fdt, 0, TempStr);
    ASSERT (TempNode > 0);

    Status = FdtSetProp (Fdt, TempNode, "display", "&gma", (UINT32)(AsciiStrLen ("&gma")+1));
    ASSERT_EFI_ERROR (Status);

    Status = FdtSetProp (Fdt, TempNode, "format", "a8r8g8b8", (UINT32)(AsciiStrLen ("a8r8g8b8")+1));
    ASSERT_EFI_ERROR (Status);

    Data32 = CpuToFdt32 (GraphicsInfo->GraphicsMode.VerticalResolution);
    Status = FdtSetProp (Fdt, TempNode, "height", &Data32, sizeof (UINT32));
    ASSERT_EFI_ERROR (Status);

    Data32 = CpuToFdt32 (GraphicsInfo->GraphicsMode.HorizontalResolution);
    Status = FdtSetProp (Fdt, TempNode, "width", &Data32, sizeof (UINT32));
    ASSERT_EFI_ERROR (Status);

    RegData[0] = CpuToFdt64 (GraphicsInfo->FrameBufferBase);
    RegData[1] = CpuToFdt64 (GraphicsInfo->FrameBufferSize);
    Status     = FdtSetProp (Fdt, TempNode, "reg", &RegData, sizeof (RegData));
    ASSERT_EFI_ERROR (Status);

    Status = FdtSetProp (Fdt, TempNode, "compatible", "simple-framebuffer", (UINT32)(AsciiStrLen ("simple-framebuffer")+1));
    ASSERT_EFI_ERROR (Status);
  } else {
    Status   = AsciiSPrint (TempStr, sizeof (TempStr), "framebuffer@%lX", 0xB0000000);
    TempNode = FdtAddSubnode (Fdt, 0, TempStr);
    ASSERT (TempNode > 0);

    Status = FdtSetProp (Fdt, TempNode, "display", "&gma", (UINT32)(AsciiStrLen ("&gma")+1));
    ASSERT_EFI_ERROR (Status);

    Status = FdtSetProp (Fdt, TempNode, "format", "a8r8g8b8", (UINT32)(AsciiStrLen ("a8r8g8b8")+1));
    ASSERT_EFI_ERROR (Status);

    Data32 = CpuToFdt32 (1024);
    Status = FdtSetProp (Fdt, TempNode, "height", &Data32, sizeof (UINT32));
    ASSERT_EFI_ERROR (Status);

    Data32 = CpuToFdt32 (1280);
    Status = FdtSetProp (Fdt, TempNode, "width", &Data32, sizeof (UINT32));
    ASSERT_EFI_ERROR (Status);

    RegData[0] = CpuToFdt64 (0xB0000000);
    RegData[1] = CpuToFdt64 (0x500000);
    Status     = FdtSetProp (Fdt, TempNode, "reg", &RegData, sizeof (RegData));
    ASSERT_EFI_ERROR (Status);

    Status = FdtSetProp (Fdt, TempNode, "compatible", "simple-framebuffer", (UINT32)(AsciiStrLen ("simple-framebuffer")+1));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  It will build FDT for UPL required data.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForUplRequired (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS              Status;
  VOID                    *Fdt;
  VOID                    *Fit;
  INT32                   ParentNode;
  INT32                   CustomNode;
  INT32                   UPLParaNode;
  INT32                   UPLImageNode;
  EFI_HOB_CPU             *CpuHob;
  UINT64                  Data64;
  UINT32                  Data32;
  VOID                    *HobPtr;
  EFI_BOOT_MODE           BootMode;
  CHAR8                   TempStr[32];
  UINT8                   *GuidHob;
  UNIVERSAL_PAYLOAD_BASE  *PayloadBase;

  Fdt = FdtBase;
  Fit = NULL;

  //
  // Create Hob list FDT node.
  //
  ParentNode = FdtAddSubnode (Fdt, 0, "options");
  ASSERT (ParentNode > 0);

  UPLParaNode = FdtAddSubnode (Fdt, ParentNode, "upl-params");
  ASSERT (UPLParaNode > 0);

  //
  // Create CPU info FDT node
  //
  CpuHob = GetFirstHob (EFI_HOB_TYPE_CPU);
  ASSERT (CpuHob != NULL);

  if (mResourceAssigned) {
    Status = FdtSetProp (Fdt, UPLParaNode, "pci-enum-done", NULL, 0);
    ASSERT_EFI_ERROR (Status);
  }

  BootMode = GetBootModeHob ();

  Data32 = CpuToFdt32 ((UINT32)CpuHob->SizeOfMemorySpace);
  Status = FdtSetProp (Fdt, UPLParaNode, "addr-width", &Data32, sizeof (Data32));
  ASSERT_EFI_ERROR (Status);

  if (BootMode == BOOT_WITH_FULL_CONFIGURATION) {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "normal", (UINT32)(AsciiStrLen ("normal")+1));
  } else if (BootMode == BOOT_WITH_MINIMAL_CONFIGURATION) {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "fast", (UINT32)(AsciiStrLen ("fast")+1));
  } else if (BootMode == BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS) {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "full", (UINT32)(AsciiStrLen ("full")+1));
  } else if (BootMode == BOOT_WITH_DEFAULT_SETTINGS) {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "default", (UINT32)(AsciiStrLen ("default")+1));
  } else if (BootMode == BOOT_ON_S4_RESUME) {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "s4", (UINT32)(AsciiStrLen ("s4")+1));
  } else if (BootMode == BOOT_ON_S3_RESUME) {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "s3", (UINT32)(AsciiStrLen ("s3")+1));
  } else {
    Status = FdtSetProp (Fdt, UPLParaNode, "boot-mode", "na", (UINT32)(AsciiStrLen ("na")+1));
  }

  ASSERT_EFI_ERROR (Status);

  Status = FdtSetProp (Fdt, UPLParaNode, "compatible", "upl", (UINT32)(AsciiStrLen ("upl")+1));
  ASSERT_EFI_ERROR (Status);

  GuidHob = GetFirstGuidHob (&gUniversalPayloadBaseGuid);
  if (GuidHob != NULL) {
    PayloadBase = (UNIVERSAL_PAYLOAD_BASE *)GET_GUID_HOB_DATA (GuidHob);
    Fit         = (VOID *)(UINTN)PayloadBase->Entry;
    DEBUG ((DEBUG_INFO, "PayloadBase Entry = 0x%08x\n", PayloadBase->Entry));

    Status       = AsciiSPrint (TempStr, sizeof (TempStr), "upl-image@%lX", (UINTN)(Fit));
    UPLImageNode = FdtAddSubnode (Fdt, ParentNode, TempStr);

    Data64 = CpuToFdt64 ((UINTN)Fit);
    Status = FdtSetProp (FdtBase, UPLImageNode, "addr", &Data64, sizeof (Data64));
  }

  CustomNode = FdtAddSubnode (Fdt, ParentNode, "upl-custom");
  ASSERT (CustomNode > 0);

  HobPtr = GetHobList ();
  Data64 = CpuToFdt64 ((UINT64)(EFI_PHYSICAL_ADDRESS)HobPtr);
  Status = FdtSetProp (Fdt, CustomNode, "hoblistptr", &Data64, sizeof (Data64));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  It will build FDT for UPL consumed.
  @param[in] FdtBase         Address of the Fdt data.
  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required FDT.
**/
EFI_STATUS
BuildFdtForUPL (
  IN     VOID  *FdtBase
  )
{
  EFI_STATUS  Status;

  //
  // Build FDT for memory related
  //
  Status = BuildFdtForMemory (FdtBase);
  ASSERT_EFI_ERROR (Status);

  Status = BuildFdtForMemAlloc (FdtBase);
  ASSERT_EFI_ERROR (Status);

  Status = BuildFdtForPciRootBridge (FdtBase);
  ASSERT_EFI_ERROR (Status);

  Status = BuildFdtForFrameBuffer (FdtBase);
  ASSERT_EFI_ERROR (Status);

  Status = BuildFdtForUplRequired (FdtBase);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
