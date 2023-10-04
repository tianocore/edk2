/** @file
  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Guid/MemoryAllocationHob.h>
#include <Guid/DebugPrintErrorLevel.h>
#include <Guid/SerialPortInfoGuid.h>
#include <Guid/MemoryMapInfoGuid.h>
#include <Guid/AcpiBoardInfoGuid.h>
#include <Guid/GraphicsInfoHob.h>
#include <UniversalPayload/SmbiosTable.h>
#include <UniversalPayload/AcpiTable.h>
#include <UniversalPayload/UniversalPayload.h>
#include <UniversalPayload/ExtraData.h>
#include <UniversalPayload/SerialPortInfo.h>
#include <UniversalPayload/DeviceTree.h>
#include <UniversalPayload/PciRootBridges.h>
#include <IndustryStandard/SmBios.h>
#include <Library/PrintLib.h>
#include <Library/FdtLib.h>


#define MEMORY_ATTRIBUTE_DEFAULT  (EFI_RESOURCE_ATTRIBUTE_PRESENT                   | \
                                     EFI_RESOURCE_ATTRIBUTE_INITIALIZED             | \
                                     EFI_RESOURCE_ATTRIBUTE_TESTED                  | \
                                     EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE             | \
                                     EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE       | \
                                     EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE | \
                                     EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE    )

extern VOID  *mHobList;

/**
  Build ACPI board info HOB using infomation from ACPI table

  @param  AcpiTableBase      ACPI table start address in memory

  @retval  A pointer to ACPI board HOB ACPI_BOARD_INFO. Null if build HOB failure.
**/
ACPI_BOARD_INFO *
BuildHobFromAcpi (
  IN   UINT64  AcpiTableBase
  );

/**
  Build a Handoff Information Table HOB

  This function initialize a HOB region from EfiMemoryBegin to
  EfiMemoryTop. And EfiFreeMemoryBottom and EfiFreeMemoryTop should
  be inside the HOB region.

  @param[in] EfiMemoryBottom       Total memory start address
  @param[in] EfiMemoryTop          Total memory end address.
  @param[in] EfiFreeMemoryBottom   Free memory start address
  @param[in] EfiFreeMemoryTop      Free memory end address.

  @return   The pointer to the handoff HOB table.

**/
EFI_HOB_HANDOFF_INFO_TABLE *
EFIAPI
HobConstructor (
  IN VOID  *EfiMemoryBottom,
  IN VOID  *EfiMemoryTop,
  IN VOID  *EfiFreeMemoryBottom,
  IN VOID  *EfiFreeMemoryTop
  );

/**
  It will parse FDT based on DTB from bootloaders.

  @param[in]  FdtBase               Address of the Fdt data.

  @retval EFI_SUCCESS               If it completed successfully.
  @retval Others                    If it failed to parse DTB.
**/
UINTN
ParseDtb (
  IN VOID                           *FdtBase
  )
{
  VOID                                     *Fdt;
  INT32                                    Node;
  INT32                                    Property;
  INT32                                    Depth;
  FDT_NODE_HEADER                          *NodePtr;
  CONST FDT_PROPERTY                       *PropertyPtr;
  CONST CHAR8                              *TempStr;
  INT32                                    TempLen;
  UINT32                                   *Data32;
  UINT64                                   *Data64;
  UINT64                                   StartAddress;
#if FixedPcdGet8 (PcdHandOffFdtEnable) == TRUE
  INT32                                    SubNode;
  UINT64                                   NumberOfBytes;
  UINT32                                   Attribute;
#endif
//  EFI_GUID                                 *Guid;
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO       *Serial;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES       *PciRootBridgeInfo;
  UEFI_PAYLOAD_DEBUG_PRINT_ERROR_LEVEL     *DebugPrintErrorLevelInfo;
  UNIVERSAL_PAYLOAD_BASE                   *PayloadBase;
  UNIVERSAL_PAYLOAD_ACPI_TABLE             *PldAcpiTable;
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE           *SmbiosTable;
  EFI_PEI_GRAPHICS_INFO_HOB                *GraphicsInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB         *GraphicsDev;
#if FixedPcdGet8 (PcdHandOffFdtEnable) == FALSE
  ACPI_BOARD_INFO                          *AcpiBoardInfo;
#endif
  UINT8                                    SizeOfMemorySpace;
  UINT64                                   FrameBufferBase;
  UINT64                                   FrameBufferSize;

  Fdt = FdtBase;
  Depth = 0;
  FrameBufferBase = 0;
  FrameBufferSize = 0;

  DEBUG ((DEBUG_INFO, "FDT = 0x%x  %x\n", Fdt, Fdt32ToCpu (*((UINT32 *) Fdt))));
  DEBUG ((DEBUG_INFO, "Start parsing DTB data"));

  //
  // Build SMBIOS HOB .
  //
  SmbiosTable = BuildGuidHob (&gUniversalPayloadSmbios3TableGuid, sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE) + sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT));
  ASSERT (SmbiosTable != NULL);
  if (SmbiosTable == NULL) {
    return  EFI_OUT_OF_RESOURCES;
  }
  SmbiosTable->Header.Revision = UNIVERSAL_PAYLOAD_SMBIOS_TABLE_REVISION;
  SmbiosTable->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE);


  PropertyPtr = FdtGetProperty (Fdt, 0, "smbios", &TempLen);
  DEBUG ((DEBUG_INFO, "FdtGetProperty Fdt :%xsmbios , TempLen :%x\n",(UINTN)Fdt,TempLen));
  ASSERT (TempLen > 0);
  if (TempLen > 0) {
    Data64 = (UINT64 *) (PropertyPtr->Data);
    StartAddress = Fdt64ToCpu (*Data64);
    DEBUG ((DEBUG_INFO, "\n         Property(00000000)  smbios"));
    DEBUG ((DEBUG_INFO, "  %016lX\n", StartAddress));

    SmbiosTable->SmBiosEntryPoint = (EFI_PHYSICAL_ADDRESS) StartAddress;
    CopyMem ((VOID *) (UINTN) SmbiosTable->SmBiosEntryPoint, &StartAddress, sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT));
  }

  //
  // Build gUniversalPayloadAcpiTableGuid HOB with RSDP.
  //
  PldAcpiTable = BuildGuidHob (&gUniversalPayloadAcpiTableGuid, sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE));
  ASSERT (PldAcpiTable != NULL);
  if (PldAcpiTable == NULL) {
    return  EFI_OUT_OF_RESOURCES;
  }

  PldAcpiTable->Header.Revision = UNIVERSAL_PAYLOAD_ACPI_TABLE_REVISION;
  PldAcpiTable->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE);

  PropertyPtr = FdtGetProperty (Fdt, 0, "acpi", &TempLen);
  ASSERT (TempLen > 0);
  if (TempLen > 0) {
    Data64 = (UINT64 *) (PropertyPtr->Data);
    StartAddress = Fdt64ToCpu (*Data64);
    DEBUG ((DEBUG_INFO, "\n         Property(00000000)  acpi"));
    DEBUG ((DEBUG_INFO, "  %016lX\n", StartAddress));
 
    PldAcpiTable->Rsdp = (EFI_PHYSICAL_ADDRESS)StartAddress;
#if FixedPcdGet8 (PcdHandOffFdtEnable) == FALSE
    AcpiBoardInfo = BuildHobFromAcpi ((UINT64)PldAcpiTable->Rsdp);
    ASSERT (AcpiBoardInfo != NULL);
#endif
  }

  for (Node = FdtNextNode (Fdt, 0, &Depth); Node >= 0; Node = FdtNextNode (Fdt, Node, &Depth)) {
    if (Depth != 1) {
      continue;
    }

    NodePtr = (FDT_NODE_HEADER *) ((CONST CHAR8 *) Fdt + Node + Fdt32ToCpu (((FDT_HEADER *) Fdt)->OffsetDtStruct));
    DEBUG ((DEBUG_INFO, "\n   Node(%08x)  %a   Depth %x", Node, NodePtr->Name, Depth));

#if FixedPcdGet8 (PcdHandOffFdtEnable) == TRUE
    // memory node
    if (AsciiStrnCmp (NodePtr->Name, "memory@", AsciiStrLen("memory@")) == 0) {
      Attribute = MEMORY_ATTRIBUTE_DEFAULT;
      for (Property = FdtFirstPropertyOffset (Fdt, Node); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
        PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
        TempStr = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);
        if (AsciiStrCmp (TempStr, "reg") == 0) {
          Data64 = (UINT64 *) (PropertyPtr->Data);
          StartAddress = Fdt64ToCpu (*Data64);
          NumberOfBytes = Fdt64ToCpu (*(Data64 + 1));
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %016lX  %016lX", StartAddress, NumberOfBytes));
        }
        else if (AsciiStrCmp (TempStr, "attr") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          Attribute = Fdt32ToCpu (*Data32);
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %X", Attribute));
        }
      }
      BuildResourceDescriptorHob (EFI_RESOURCE_SYSTEM_MEMORY, Attribute, StartAddress, NumberOfBytes);

      /*if (AsciiStrCmp (TempStr, "memory") == 0) {
        BuildResourceDescriptorHob (EFI_RESOURCE_SYSTEM_MEMORY, Attribute, StartAddress, NumberOfBytes);
      }
      else if (AsciiStrCmp (TempStr, "mmio") == 0) {
        BuildResourceDescriptorHob (EFI_RESOURCE_MEMORY_MAPPED_IO, Attribute, StartAddress, NumberOfBytes);
      }
      else if (AsciiStrCmp (TempStr, "reserved") == 0) {
        BuildResourceDescriptorHob (EFI_RESOURCE_MEMORY_RESERVED, Attribute, StartAddress, NumberOfBytes);
      }*/
    } // end of memory node

    // reserved-memory
    else if (AsciiStrCmp (NodePtr->Name, "reserved-memory") == 0) {
      for (SubNode = FdtFirstSubnode (Fdt, Node); SubNode >= 0; SubNode = FdtNextSubnode (Fdt, SubNode)) {
        NodePtr = (FDT_NODE_HEADER *) ((CONST CHAR8 *) Fdt + SubNode + Fdt32ToCpu (((FDT_HEADER *) Fdt)->OffsetDtStruct));
        DEBUG ((DEBUG_INFO, "\n      SubNode(%08x)  %a", SubNode, NodePtr->Name));

        Attribute = MEMORY_ATTRIBUTE_DEFAULT;
        for (Property = FdtFirstPropertyOffset (Fdt, SubNode); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
          PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
          TempStr = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);
          if (AsciiStrCmp (TempStr, "reg") == 0) {
            Data64 = (UINT64 *) (PropertyPtr->Data);
            StartAddress = Fdt64ToCpu (*Data64);
            NumberOfBytes = Fdt64ToCpu (*(Data64 + 1));
            //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
            //DEBUG ((DEBUG_INFO, "  %016lX  %016lX", StartAddress, NumberOfBytes));
          }
          else if (AsciiStrCmp (TempStr, "Attr") == 0) {
            Data32 = (UINT32 *) (PropertyPtr->Data);
            Attribute = Fdt32ToCpu (*Data32);
            DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
            DEBUG ((DEBUG_INFO, "  %X", Attribute));
          }
        }

        if (AsciiStrnCmp (NodePtr->Name, "mmio", AsciiStrLen ("mmio")) == 0) {
          DEBUG ((DEBUG_INFO, "  mmio"));
          BuildResourceDescriptorHob (EFI_RESOURCE_MEMORY_MAPPED_IO, Attribute, StartAddress, NumberOfBytes);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "reserved", AsciiStrLen ("reserved")) == 0) {
          DEBUG ((DEBUG_INFO, "  reserved"));
          BuildResourceDescriptorHob (EFI_RESOURCE_MEMORY_RESERVED, Attribute, StartAddress, NumberOfBytes);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "framebuffer", AsciiStrLen ("framebuffer")) == 0) {
          FrameBufferBase = StartAddress;
          FrameBufferSize = NumberOfBytes;
        }
      }
    } // end of reserved-memory

    // memory allocation
    else if (AsciiStrCmp (NodePtr->Name, "memory-allocation") == 0) {
      for (SubNode = FdtFirstSubnode (Fdt, Node); SubNode >= 0; SubNode = FdtNextSubnode (Fdt, SubNode)) {
        NodePtr = (FDT_NODE_HEADER *) ((CONST CHAR8 *) Fdt + SubNode + Fdt32ToCpu (((FDT_HEADER *) Fdt)->OffsetDtStruct));
        DEBUG ((DEBUG_INFO, "\n      SubNode(%08X)  %a", SubNode, NodePtr->Name));

        Guid = NULL;
        PropertyPtr = FdtGetProperty (Fdt, SubNode, "reg", &TempLen);
        ASSERT (TempLen > 0);
        if (TempLen > 0) {
          Data64 = (UINT64 *) (PropertyPtr->Data);
          StartAddress = Fdt64ToCpu (*Data64);
          NumberOfBytes = Fdt64ToCpu (*(Data64 + 1));
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %016lX  %016lX", StartAddress, NumberOfBytes));
        }

        if (AsciiStrnCmp (NodePtr->Name, "Reserved", AsciiStrLen ("Reserved")) == 0) {
          DEBUG ((DEBUG_INFO, "  reserved"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiReservedMemoryType);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "LoaderCode", AsciiStrLen ("LoaderCode")) == 0) {
          DEBUG ((DEBUG_INFO, "  LoaderCode"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiLoaderCode);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "LoaderData", AsciiStrLen ("LoaderData")) == 0) {
          DEBUG ((DEBUG_INFO, "  LoaderData"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiLoaderData);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "BootServicesCode", AsciiStrLen ("BootServicesCode")) == 0) {
          DEBUG ((DEBUG_INFO, "  BootServicesCode"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiBootServicesCode);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "BootServicesData", AsciiStrLen ("BootServicesData")) == 0) {
          DEBUG ((DEBUG_INFO, "  BootServicesData"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiBootServicesData);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "RuntimeServicesCode", AsciiStrLen ("RuntimeServicesCode")) == 0) {
          DEBUG ((DEBUG_INFO, "  RuntimeServicesCode"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiRuntimeServicesCode);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "RuntimeServicesData", AsciiStrLen ("RuntimeServicesData")) == 0) {
          DEBUG ((DEBUG_INFO, "  RuntimeServicesData"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiRuntimeServicesData);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "ConventionalMemory", AsciiStrLen ("ConventionalMemory")) == 0) {
          DEBUG ((DEBUG_INFO, "  ConventionalMemory"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiConventionalMemory);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "UnusableMemory", AsciiStrLen ("UnusableMemory")) == 0) {
          DEBUG ((DEBUG_INFO, "  UnusableMemory"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiUnusableMemory);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "ACPIReclaimMemory", AsciiStrLen ("ACPIReclaimMemory")) == 0) {
          DEBUG ((DEBUG_INFO, "  ACPIReclaimMemory"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiACPIReclaimMemory);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "ACPIMemoryNVS", AsciiStrLen ("ACPIMemoryNVS")) == 0) {
          DEBUG ((DEBUG_INFO, "  ACPIMemoryNVS"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiACPIMemoryNVS);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "MemoryMappedIO", AsciiStrLen ("MemoryMappedIO")) == 0) {
          DEBUG ((DEBUG_INFO, "  MemoryMappedIO"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiMemoryMappedIO);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "MemoryMappedIOPortSpace", AsciiStrLen ("MemoryMappedIOPortSpace")) == 0) {
          DEBUG ((DEBUG_INFO, "  MemoryMappedIOPortSpace"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiMemoryMappedIOPortSpace);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "PalCode", AsciiStrLen ("PalCode")) == 0) {
          DEBUG ((DEBUG_INFO, "  PalCode"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiPalCode);
        }
        else if (AsciiStrnCmp (NodePtr->Name, "PersistentMemory", AsciiStrLen ("PersistentMemory")) == 0) {
          DEBUG ((DEBUG_INFO, "  PersistentMemory"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiPersistentMemory);
        }
      }
    } // end of memory allocation
#endif

    if (AsciiStrnCmp (NodePtr->Name, "serial@", AsciiStrLen ("serial@")) == 0) {
      //
      // Create SerialPortInfo HOB.
      //
      Serial = BuildGuidHob (&gUniversalPayloadSerialPortInfoGuid, sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO));
      ASSERT (Serial != NULL);
      if (Serial == NULL) {
        break;
      }
      Serial->Header.Revision = UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_REVISION;
      Serial->Header.Length = sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO);

      for (Property = FdtFirstPropertyOffset (Fdt, Node); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
        PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
        TempStr = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);
        if (AsciiStrCmp (TempStr, "current-speed") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));

          Serial->BaudRate = Fdt32ToCpu (*Data32);
        }
        else if (AsciiStrCmp (TempStr, "reg") == 0) {
          Data64 = (UINT64 *) (PropertyPtr->Data);
          StartAddress = Fdt64ToCpu (*Data64);
          DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          DEBUG ((DEBUG_INFO, "  %016lX", StartAddress));

          Serial->RegisterBase = StartAddress;
        }
        else if (AsciiStrCmp (TempStr, "reg-shift") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));

          Serial->RegisterStride = (UINT8) Fdt32ToCpu (*Data32);
        }
        else if (AsciiStrCmp (TempStr, "reg-width") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));
        }
        else if (AsciiStrCmp (TempStr, "use-mmio") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));

          Serial->UseMmio = (BOOLEAN) Fdt32ToCpu (*Data32);
        }
      }
    }

    else if (AsciiStrCmp (NodePtr->Name, "graphic") == 0) {
      //
      // Create GraphicInfo HOB.
      //
      GraphicsInfo = BuildGuidHob (&gEfiGraphicsInfoHobGuid, sizeof (EFI_PEI_GRAPHICS_INFO_HOB));
      ASSERT (GraphicsInfo != NULL);
      if (GraphicsInfo == NULL) {
        break;
      }
      GraphicsDev = BuildGuidHob (&gEfiGraphicsDeviceInfoHobGuid, sizeof (EFI_PEI_GRAPHICS_DEVICE_INFO_HOB));
      ASSERT (GraphicsDev != NULL);
      if (GraphicsDev == NULL) {
        break;
      }

      if (FrameBufferBase != 0) {
        GraphicsInfo->FrameBufferBase = FrameBufferBase;
      }
      if (FrameBufferSize != 0) {
        GraphicsInfo->FrameBufferSize = (UINT32) FrameBufferSize;
      }

      for (Property = FdtFirstPropertyOffset (Fdt, Node); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
        PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
        TempStr = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);
        if (AsciiStrCmp (TempStr, "resolution") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsInfo->GraphicsMode.HorizontalResolution = Fdt32ToCpu (*Data32);
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*(Data32 + 1))));
          GraphicsInfo->GraphicsMode.VerticalResolution = Fdt32ToCpu (*(Data32 + 1));
        }
        else if (AsciiStrCmp (TempStr, "pixel-format") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsInfo->GraphicsMode.PixelFormat = Fdt32ToCpu(*Data32);
        }
        else if (AsciiStrCmp (TempStr, "pixel-mask") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsInfo->GraphicsMode.PixelInformation.RedMask = Fdt32ToCpu (*Data32);
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*(Data32 + 1))));
          GraphicsInfo->GraphicsMode.PixelInformation.GreenMask = Fdt32ToCpu (*(Data32 + 1));
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*(Data32 + 2))));
          GraphicsInfo->GraphicsMode.PixelInformation.BlueMask = Fdt32ToCpu (*(Data32 + 2));
        }
        else if (AsciiStrCmp (TempStr, "pixe-scanline") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsInfo->GraphicsMode.PixelsPerScanLine = Fdt32ToCpu (*Data32);
        }
        else if (AsciiStrCmp (TempStr, "id") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsDev->VendorId = Fdt32ToCpu (*Data32) >> 16;
          GraphicsDev->DeviceId = Fdt32ToCpu (*Data32) & 0xffff;
        }
        else if (AsciiStrCmp (TempStr, "subsystem-id") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsDev->SubsystemVendorId = Fdt32ToCpu (*Data32) >> 16;
          GraphicsDev->SubsystemId = Fdt32ToCpu (*Data32) & 0xffff;
        }
        else if (AsciiStrCmp (TempStr, "revision-id") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsDev->RevisionId = (UINT8) Fdt32ToCpu (*Data32);
        }
        else if (AsciiStrCmp (TempStr, "bar-index") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsDev->BarIndex = (UINT8) Fdt32ToCpu (*Data32);
        }
      }
    }

    else if (AsciiStrCmp (NodePtr->Name, "cpu-info") == 0) {
      PropertyPtr = FdtGetProperty (Fdt, Node,"address_width", &TempLen);
      ASSERT(TempLen > 0);
      if (TempLen > 0) {
        Data32 = (UINT32 *) (PropertyPtr->Data);
        DEBUG ((DEBUG_INFO, "\n         Property(00000000)  address_width"));
        DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));
        SizeOfMemorySpace = (UINT8) Fdt32ToCpu (*Data32);

        BuildCpuHob (SizeOfMemorySpace, 16);
      }
    }

    else if (AsciiStrCmp (NodePtr->Name, "PayloadBase") == 0) {
      //
      // Build PayloadBase HOB .
      //
      PayloadBase = BuildGuidHob (&gUniversalPayloadBaseGuid, sizeof (UNIVERSAL_PAYLOAD_BASE));
      ASSERT (PayloadBase != NULL);
      if (PayloadBase == NULL){
        return  EFI_OUT_OF_RESOURCES;
      }
      PayloadBase->Header.Revision = UNIVERSAL_PAYLOAD_BASE_REVISION;
      PayloadBase->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_BASE);

      PropertyPtr = FdtGetProperty (Fdt, Node, "entry", &TempLen);
      ASSERT (TempLen > 0);
      if (TempLen > 0) {
        Data64 = (UINT64 *) (PropertyPtr->Data);
        StartAddress = Fdt64ToCpu (*Data64);
        //DEBUG ((DEBUG_INFO, "\n         Property(00000000)  entry"));
        //DEBUG ((DEBUG_INFO, "  %016lX\n", StartAddress));

        PayloadBase->Entry = (EFI_PHYSICAL_ADDRESS) StartAddress;
      }
    }

    // Optional
    else if (AsciiStrCmp (NodePtr->Name, "pcirbinfo") == 0) {
      //
      // Create PCI Root Bridge Info Hob.
      //
      PciRootBridgeInfo = BuildGuidHob (&gUniversalPayloadPciRootBridgeInfoGuid, sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES) + sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE));
      ASSERT (PciRootBridgeInfo != NULL);
      if (PciRootBridgeInfo == NULL) {
        break;
      }
      PciRootBridgeInfo->Header.Length = sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES) + sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE);
      PciRootBridgeInfo->Header.Revision = UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION;

      for (Property = FdtFirstPropertyOffset (Fdt, Node); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
        PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
        TempStr = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);
        if (AsciiStrCmp (TempStr, "count") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));

          PciRootBridgeInfo->Count = (UINT8) Fdt32ToCpu (*Data32);
        }
        else if (AsciiStrCmp (TempStr, "ResourceAssigned") == 0) {
          Data32 = (UINT32 *) (PropertyPtr->Data);
          //DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));

          PciRootBridgeInfo->ResourceAssigned = (BOOLEAN) Fdt32ToCpu (*Data32);
        }
      }
    }

    else if (AsciiStrCmp (NodePtr->Name, "DebugPrintErrorLevel") == 0) {
      //
      // Create DebugPrintErrorLevel Hob.
      //
      DebugPrintErrorLevelInfo = BuildGuidHob (&gEdkiiDebugPrintErrorLevelGuid, sizeof (UEFI_PAYLOAD_DEBUG_PRINT_ERROR_LEVEL));
      ASSERT (DebugPrintErrorLevelInfo != NULL);
      if (DebugPrintErrorLevelInfo == NULL) {
        break;
      }
      DebugPrintErrorLevelInfo->Header.Revision = UEFI_PAYLOAD_DEBUG_PRINT_ERROR_LEVEL_REVISION;
      DebugPrintErrorLevelInfo->Header.Length = sizeof (UEFI_PAYLOAD_DEBUG_PRINT_ERROR_LEVEL);

      PropertyPtr = FdtGetProperty (Fdt, Node, "errorlevel", &TempLen);
      ASSERT (TempLen > 0);
      if (TempLen > 0) {
        Data32 = (UINT32 *) (PropertyPtr->Data);
        DEBUG ((DEBUG_INFO, "\n         Property(00000000)  errorlevel"));
        DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));
        DebugPrintErrorLevelInfo->ErrorLevel = Fdt32ToCpu (*Data32);
      }
    }

  }
  DEBUG ((DEBUG_INFO, "\n"));

  return 0;
}

/**
  It will build a graphic device hob.

  @retval EFI_SUCCESS               If it completed successfully.
  @retval Others                    If it failed to parse DTB.
**/
EFI_STATUS
BuildGraphicDevHob (
  VOID
  );

/**
  It will initialize HOBs for UPL.

  @param[in]  FdtBase        Address of the Fdt data.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to initialize HOBs.
**/
UINT64
UplInitHob (
  IN VOID                         *FdtBase
  )
{
  UINT8                          *GuidHob;
  UNIVERSAL_PAYLOAD_DEVICE_TREE  *FdtHob;
  UINTN                          MinimalNeededSize;
  VOID                           *Fdt;
  INT32                          Node;
  CONST FDT_PROPERTY             *PropertyPtr;
  INT32                          TempLen;
  UINT64                         *Data64;
  UINT64                         StartAddress;
  UINT64                         NumberOfBytes;
  EFI_PHYSICAL_ADDRESS           FreeMemoryBottom;
  EFI_PHYSICAL_ADDRESS           FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS           MemoryBottom;
  EFI_PHYSICAL_ADDRESS           MemoryTop;

  DEBUG ((DEBUG_INFO, "FdtBase = 0x%x\n", FdtBase));

  //
  // Check parameter type(
  //
  if (FdtCheckHeader (FdtBase) == 0) {
    DEBUG ((DEBUG_INFO, "%a() FDT blob\n", __func__));
    Fdt = FdtBase;
  }
  else {
    DEBUG ((DEBUG_INFO, "%a() HOb list\n", __func__));
    mHobList = FdtBase;

    GuidHob = GetFirstGuidHob (&gUniversalPayloadDeviceTreeGuid);
    ASSERT (GuidHob != NULL);
    FdtHob = (UNIVERSAL_PAYLOAD_DEVICE_TREE *) GET_GUID_HOB_DATA (GuidHob);

    Fdt = (VOID *) (FdtHob->DeviceTreeAddress);
  }

  //
  // Look for if exists hob list node
  //
  Node = FdtSubnodeOffsetNameLen (Fdt, 0, "hoblistptr", (INT32) AsciiStrLen ("hoblistptr"));
  if (Node > 0) {
    PropertyPtr = FdtGetProperty (Fdt, Node,"hoblistptr", &TempLen);
    Data64 = (UINT64 *) (PropertyPtr->Data);
    StartAddress = Fdt64ToCpu (*Data64);
    DEBUG ((DEBUG_INFO, "  Found hob list node (%08X)", Node));
    DEBUG ((DEBUG_INFO, " -pointer  %016lX\n", StartAddress));

    return StartAddress;
  }

  //
  // Look for memory space for Hob usage
  //
  MinimalNeededSize = FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
  DEBUG ((DEBUG_INFO, "Hobs MinimalNeededSize = %x\n", MinimalNeededSize));

  Node = FdtSubnodeOffsetNameLen (Fdt, 0, "reserved-memory", (INT32) AsciiStrLen ("reserved-memory"));
  if (Node > 0) {
    Node = FdtSubnodeOffsetNameLen (Fdt, Node, "uefi", (INT32) AsciiStrLen ("uefi"));
    if (Node > 0) {
      PropertyPtr = FdtGetProperty (Fdt, Node,"reg", &TempLen);
      Data64 = (UINT64 *) (PropertyPtr->Data);
      StartAddress = Fdt64ToCpu (*Data64);
      NumberOfBytes = Fdt64ToCpu (*(Data64 + 1));
      DEBUG ((DEBUG_INFO, "  Found uefi memory node (%08X)", Node));
      DEBUG ((DEBUG_INFO, " -reg  %016lX  %016lX\n", StartAddress, NumberOfBytes));

      MemoryBottom    = StartAddress;
      FreeMemoryBottom = ALIGN_VALUE (MemoryBottom + 0x100000, SIZE_1MB);
      MemoryTop  = MemoryBottom + NumberOfBytes;
      FreeMemoryTop = MemoryTop;
      HobConstructor ((VOID *) (UINTN) MemoryBottom, (VOID *) (UINTN) MemoryTop, (VOID * )(UINTN) FreeMemoryBottom, (VOID *) (UINTN) FreeMemoryTop);
    }
  }

  //
  // Create an empty FvHob for the DXE FV that contains DXE core.
  //
  BuildFvHob ((EFI_PHYSICAL_ADDRESS) 0, 0);

  ParseDtb (Fdt);

  return 0;
}