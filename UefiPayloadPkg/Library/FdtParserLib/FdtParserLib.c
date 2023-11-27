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
#include <Library/IoLib.h>
#include <Guid/MemoryAllocationHob.h>
#include <Guid/DebugPrintErrorLevel.h>
#include <Guid/SerialPortInfoGuid.h>
#include <Guid/MemoryMapInfoGuid.h>
#include <Guid/AcpiBoardInfoGuid.h>
#include <Guid/GraphicsInfoHob.h>
#include <Guid/UniversalPayloadBase.h>
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
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciIo.h>

#define MEMORY_ATTRIBUTE_DEFAULT  (EFI_RESOURCE_ATTRIBUTE_PRESENT                   | \
                                     EFI_RESOURCE_ATTRIBUTE_INITIALIZED             | \
                                     EFI_RESOURCE_ATTRIBUTE_TESTED                  | \
                                     EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE             | \
                                     EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE       | \
                                     EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE | \
                                     EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE    )



#define ROOT_BRIDGE_SUPPORTS_DEFAULT  (EFI_PCI_IO_ATTRIBUTE_VGA_IO_16 | \
                              EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16 | \
                              EFI_PCI_IO_ATTRIBUTE_ISA_IO_16 | \
                              EFI_PCI_IO_ATTRIBUTE_IDE_PRIMARY_IO | \
                              EFI_PCI_IO_ATTRIBUTE_VGA_IO | \
                              EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY | \
                              EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO | \
                              EFI_PCI_IO_ATTRIBUTE_ISA_IO | \
                              EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO )

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

  @return   The address to the new hob list
**/
UINTN
ParseDtb (
  IN VOID  *FdtBase
  )
{
  VOID                                  *Fdt;
  INT32                                 Node;
  INT32                                 Property;
  INT32                                 Depth;
  FDT_NODE_HEADER                       *NodePtr;
  CONST FDT_PROPERTY                    *PropertyPtr;
  CONST CHAR8                           *TempStr;
  INT32                                 TempLen;
  UINT32                                *Data32;
  UINT64                                *Data64;
  UINT64                                StartAddress;
  INT32                                 SubNode;
  UINT64                                NumberOfBytes;
  UINT32                                Attribute;
  UINT8                                 ECCAttribute;
  UINT32                                ECCData, ECCData2;
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO    *Serial;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES    *PciRootBridgeInfo;
  UNIVERSAL_PAYLOAD_ACPI_TABLE          *PlatformAcpiTable;
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE        *SmbiosTable;
  UEFI_PAYLOAD_DEBUG_PRINT_ERROR_LEVEL  *DebugPrintErrorLevelInfo;
  UNIVERSAL_PAYLOAD_BASE                *PayloadBase;
  EFI_PEI_GRAPHICS_INFO_HOB             *GraphicsInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB      *GraphicsDev;
  // UINT8                                 SizeOfMemorySpace;
  UINT64                FrameBufferBase;
  UINT64                FrameBufferSize;
  UINTN                 MinimalNeededSize;
  EFI_PHYSICAL_ADDRESS  FreeMemoryBottom;
  EFI_PHYSICAL_ADDRESS  FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS  MemoryBottom;
  EFI_PHYSICAL_ADDRESS  MemoryTop;
  BOOLEAN               IsHobContructed;
  UINTN                 NewHobList;

  Fdt               = FdtBase;
  Depth             = 0;
  FrameBufferBase   = 0;
  FrameBufferSize   = 0;
  MinimalNeededSize = FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
  IsHobContructed   = FALSE;
  NewHobList        = 0;
  PlatformAcpiTable = NULL;
  SmbiosTable       = NULL;

  DEBUG ((DEBUG_INFO, "FDT = 0x%x  %x\n", Fdt, Fdt32ToCpu (*((UINT32 *)Fdt))));
  DEBUG ((DEBUG_INFO, "Start parsing DTB data\n"));
  DEBUG ((DEBUG_INFO, "MinimalNeededSize :%x\n", MinimalNeededSize));

  for (Node = FdtNextNode (Fdt, 0, &Depth); Node >= 0; Node = FdtNextNode (Fdt, Node, &Depth)) {
    if (Depth != 1) {
      continue;
    }

    NodePtr = (FDT_NODE_HEADER *)((CONST CHAR8 *)Fdt + Node + Fdt32ToCpu (((FDT_HEADER *)Fdt)->OffsetDtStruct));
    DEBUG ((DEBUG_INFO, "\n   Node(%08x)  %a   Depth %x", Node, NodePtr->Name, Depth));
    // memory node
    if (AsciiStrnCmp (NodePtr->Name, "memory@", AsciiStrLen ("memory@")) == 0) {
      for (Property = FdtFirstPropertyOffset (Fdt, Node); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
        PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
        TempStr     = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);
        if (AsciiStrCmp (TempStr, "reg") == 0) {
          Data64        = (UINT64 *)(PropertyPtr->Data);
          StartAddress  = Fdt64ToCpu (*Data64);
          NumberOfBytes = Fdt64ToCpu (*(Data64 + 1));
          DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          DEBUG ((DEBUG_INFO, "  %016lX  %016lX", StartAddress, NumberOfBytes));
          if (IsHobContructed != TRUE ) {
            if ((NumberOfBytes > MinimalNeededSize) && (StartAddress < BASE_4GB)) {
              MemoryBottom     = StartAddress + MinimalNeededSize;
              FreeMemoryBottom = MemoryBottom;
              FreeMemoryTop    = StartAddress + NumberOfBytes;
              MemoryTop        = FreeMemoryTop;

              DEBUG ((DEBUG_INFO, "MemoryBottom :0x%llx\n", MemoryBottom));
              DEBUG ((DEBUG_INFO, "FreeMemoryBottom :0x%llx\n", FreeMemoryBottom));
              DEBUG ((DEBUG_INFO, "FreeMemoryTop :0x%llx\n", FreeMemoryTop));
              DEBUG ((DEBUG_INFO, "MemoryTop :0x%llx\n", MemoryTop));
              mHobList        =  HobConstructor ((VOID *)(UINTN)MemoryBottom, (VOID *)(UINTN)MemoryTop, (VOID *)(UINTN)FreeMemoryBottom, (VOID *)(UINTN)FreeMemoryTop);
              IsHobContructed = TRUE;
              NewHobList      = (UINTN)mHobList;
              break;
            }
          }
        }
      }
    } // end of memory node
  }

  Depth = 0;
  for (Node = FdtNextNode (Fdt, 0, &Depth); Node >= 0; Node = FdtNextNode (Fdt, Node, &Depth)) {
    if (Depth != 1) {
      continue;
    }

    NodePtr = (FDT_NODE_HEADER *)((CONST CHAR8 *)Fdt + Node + Fdt32ToCpu (((FDT_HEADER *)Fdt)->OffsetDtStruct));
    DEBUG ((DEBUG_INFO, "\n   Node(%08x)  %a   Depth %x", Node, NodePtr->Name, Depth));

    // memory node
    if (AsciiStrnCmp (NodePtr->Name, "memory@", AsciiStrLen ("memory@")) == 0) {
      Attribute    = MEMORY_ATTRIBUTE_DEFAULT;
      ECCAttribute = 0;
      ECCData      = ECCData2 = 0;
      for (Property = FdtFirstPropertyOffset (Fdt, Node); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
        PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
        TempStr     = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);
        if (AsciiStrCmp (TempStr, "reg") == 0) {
          Data64        = (UINT64 *)(PropertyPtr->Data);
          StartAddress  = Fdt64ToCpu (*Data64);
          NumberOfBytes = Fdt64ToCpu (*(Data64 + 1));
          // DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          // DEBUG ((DEBUG_INFO, "  %016lX  %016lX", StartAddress, NumberOfBytes));
        } else if (AsciiStrCmp (TempStr, "ecc-detection-bits") == 0) {
          Data32  = (UINT32 *)(PropertyPtr->Data);
          ECCData = Fdt32ToCpu (*Data32);
        } else if (AsciiStrCmp (TempStr, "ecc-correction-bits") == 0) {
          Data32   = (UINT32 *)(PropertyPtr->Data);
          ECCData2 = Fdt32ToCpu (*Data32);
          // DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          // DEBUG ((DEBUG_INFO, "  %X", Attribute));
        }
      }

      if (ECCData == ECCData2) {
        if (ECCData == 1) {
          ECCAttribute = EFI_RESOURCE_ATTRIBUTE_SINGLE_BIT_ECC;
        } else if (ECCData == 2) {
          ECCAttribute = EFI_RESOURCE_ATTRIBUTE_MULTIPLE_BIT_ECC;
        }
      }

      if (ECCAttribute != 0) {
        Attribute |= ECCAttribute;
      }

      if ((FreeMemoryBottom - MinimalNeededSize) == StartAddress ) {
        StartAddress  += MinimalNeededSize;
        NumberOfBytes -= MinimalNeededSize;
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
        NodePtr = (FDT_NODE_HEADER *)((CONST CHAR8 *)Fdt + SubNode + Fdt32ToCpu (((FDT_HEADER *)Fdt)->OffsetDtStruct));
        DEBUG ((DEBUG_INFO, "\n      SubNode(%08X)  %a", SubNode, NodePtr->Name));

        PropertyPtr = FdtGetProperty (Fdt, SubNode, "reg", &TempLen);
        ASSERT (TempLen > 0);
        if (TempLen > 0) {
          Data64        = (UINT64 *)(PropertyPtr->Data);
          StartAddress  = Fdt64ToCpu (*Data64);
          NumberOfBytes = Fdt64ToCpu (*(Data64 + 1));
          DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          DEBUG ((DEBUG_INFO, "  %016lX  %016lX", StartAddress, NumberOfBytes));
        }

        if (AsciiStrnCmp (NodePtr->Name, "mmio@", AsciiStrLen ("mmio@")) == 0) {
          DEBUG ((DEBUG_INFO, "  MemoryMappedIO"));
          BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiMemoryMappedIO);
        } else {
          PropertyPtr = FdtGetProperty (Fdt, SubNode, "compatible", &TempLen);
          // ASSERT (TempLen > 0);
          if (!(TempLen > 0)) {
            BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiReservedMemoryType);
            continue;
          }

          TempStr = (CHAR8 *)(PropertyPtr->Data);

          if (AsciiStrnCmp (TempStr, "boot-code", AsciiStrLen ("boot-code")) == 0) {
            DEBUG ((DEBUG_INFO, "  boot-code"));
            BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiBootServicesCode);
          } else if (AsciiStrnCmp (TempStr, "boot-data", AsciiStrLen ("boot-data")) == 0) {
            DEBUG ((DEBUG_INFO, "  boot-data"));
            BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiBootServicesData);
          } else if (AsciiStrnCmp (TempStr, "runtime-code", AsciiStrLen ("runtime-code")) == 0) {
            DEBUG ((DEBUG_INFO, "  runtime-code"));
            BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiRuntimeServicesCode);
          } else if (AsciiStrnCmp (TempStr, "runtime-data", AsciiStrLen ("runtime-data")) == 0) {
            DEBUG ((DEBUG_INFO, "  runtime-data"));
            BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiRuntimeServicesData);
          } else if (AsciiStrnCmp (TempStr, "acpi", AsciiStrLen ("acpi")) == 0) {
            DEBUG ((DEBUG_INFO, "  acpi, StartAddress:%x, NumberOfBytes:%x", StartAddress, NumberOfBytes));
            BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiBootServicesData);
            PlatformAcpiTable = BuildGuidHob (&gUniversalPayloadAcpiTableGuid, sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE));
            if (PlatformAcpiTable != NULL) {
              DEBUG ((DEBUG_INFO, " build gUniversalPayloadAcpiTableGuid , NumberOfBytes:%x", NumberOfBytes));
              PlatformAcpiTable->Rsdp            = (EFI_PHYSICAL_ADDRESS)(UINTN)StartAddress;
              PlatformAcpiTable->Header.Revision = UNIVERSAL_PAYLOAD_ACPI_TABLE_REVISION;
              PlatformAcpiTable->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE);
            }
          } else if (AsciiStrnCmp (TempStr, "acpi-nvs", AsciiStrLen ("acpi-nvs")) == 0) {
            DEBUG ((DEBUG_INFO, "  acpi-nvs"));
            BuildMemoryAllocationHob (StartAddress, NumberOfBytes, EfiACPIMemoryNVS);
          } else if (AsciiStrnCmp (TempStr, "smbios", AsciiStrLen ("smbios")) == 0) {
            DEBUG ((DEBUG_INFO, " build smbios, NumberOfBytes:%x", NumberOfBytes));
            SmbiosTable = BuildGuidHob (&gUniversalPayloadSmbios3TableGuid, sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE) + sizeof (SMBIOS_TABLE_3_0_ENTRY_POINT));
            if (SmbiosTable != NULL) {
              SmbiosTable->Header.Revision  = UNIVERSAL_PAYLOAD_SMBIOS_TABLE_REVISION;
              SmbiosTable->Header.Length    = sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE);
              SmbiosTable->SmBiosEntryPoint = (EFI_PHYSICAL_ADDRESS)(UINTN)(&StartAddress);
            }
          }
        }
      }
    } // end of reserved-memory

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
      Serial->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO);
      Serial->RegisterStride  = 1;

      for (Property = FdtFirstPropertyOffset (Fdt, Node); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
        PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
        TempStr     = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);
        if (AsciiStrCmp (TempStr, "current-speed") == 0) {
          Data32 = (UINT32 *)(PropertyPtr->Data);
          DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));

          Serial->BaudRate = Fdt32ToCpu (*Data32);
        } else if (AsciiStrCmp (TempStr, "reg") == 0) {
          Data64       = (UINT64 *)(PropertyPtr->Data);
          StartAddress = Fdt64ToCpu (*(Data64 + 1));
          DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          DEBUG ((DEBUG_INFO, "  %016lX", StartAddress));

          Serial->RegisterBase = StartAddress;
          // } else if (AsciiStrCmp (TempStr, "reg-shift") == 0) {
          //  Data32 = (UINT32 *)(PropertyPtr->Data);
          //  DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //  DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));

          //  Serial->RegisterStride = (UINT8)Fdt32ToCpu (*Data32);
        } else if (AsciiStrCmp (TempStr, "reg-io-width") == 0) {
          Data32 = (UINT32 *)(PropertyPtr->Data);
          DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));
          Serial->UseMmio = Fdt32ToCpu (*(Data32)) == 4 ? TRUE : FALSE;
          // } else if (AsciiStrCmp (TempStr, "use-mmio") == 0) {
          //  Data32 = (UINT32 *)(PropertyPtr->Data);
          //  DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          //  DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));

          //  Serial->UseMmio = (BOOLEAN)Fdt32ToCpu (*Data32);
        }
      }
    } else if (AsciiStrCmp (NodePtr->Name, "graphic") == 0) {
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
        GraphicsInfo->FrameBufferSize = (UINT32)FrameBufferSize;
      }

      for (Property = FdtFirstPropertyOffset (Fdt, Node); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
        PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
        TempStr     = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);
        if (AsciiStrCmp (TempStr, "resolution") == 0) {
          Data32 = (UINT32 *)(PropertyPtr->Data);
          // DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          // DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsInfo->GraphicsMode.HorizontalResolution = Fdt32ToCpu (*Data32);
          // DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*(Data32 + 1))));
          GraphicsInfo->GraphicsMode.VerticalResolution = Fdt32ToCpu (*(Data32 + 1));
        } else if (AsciiStrCmp (TempStr, "pixel-format") == 0) {
          Data32 = (UINT32 *)(PropertyPtr->Data);
          // DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          // DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsInfo->GraphicsMode.PixelFormat = Fdt32ToCpu (*Data32);
        } else if (AsciiStrCmp (TempStr, "pixel-mask") == 0) {
          Data32 = (UINT32 *)(PropertyPtr->Data);
          // DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          // DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsInfo->GraphicsMode.PixelInformation.RedMask = Fdt32ToCpu (*Data32);
          // DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*(Data32 + 1))));
          GraphicsInfo->GraphicsMode.PixelInformation.GreenMask = Fdt32ToCpu (*(Data32 + 1));
          // DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*(Data32 + 2))));
          GraphicsInfo->GraphicsMode.PixelInformation.BlueMask = Fdt32ToCpu (*(Data32 + 2));
        } else if (AsciiStrCmp (TempStr, "pixe-scanline") == 0) {
          Data32 = (UINT32 *)(PropertyPtr->Data);
          // DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          // DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsInfo->GraphicsMode.PixelsPerScanLine = Fdt32ToCpu (*Data32);
        } else if (AsciiStrCmp (TempStr, "id") == 0) {
          Data32 = (UINT32 *)(PropertyPtr->Data);
          // DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          // DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsDev->VendorId = Fdt32ToCpu (*Data32) >> 16;
          GraphicsDev->DeviceId = Fdt32ToCpu (*Data32) & 0xffff;
        } else if (AsciiStrCmp (TempStr, "subsystem-id") == 0) {
          Data32 = (UINT32 *)(PropertyPtr->Data);
          // DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          // DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsDev->SubsystemVendorId = Fdt32ToCpu (*Data32) >> 16;
          GraphicsDev->SubsystemId       = Fdt32ToCpu (*Data32) & 0xffff;
        } else if (AsciiStrCmp (TempStr, "revision-id") == 0) {
          Data32 = (UINT32 *)(PropertyPtr->Data);
          // DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          // DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsDev->RevisionId = (UINT8)Fdt32ToCpu (*Data32);
        } else if (AsciiStrCmp (TempStr, "bar-index") == 0) {
          Data32 = (UINT32 *)(PropertyPtr->Data);
          // DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
          // DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu(*Data32)));
          GraphicsDev->BarIndex = (UINT8)Fdt32ToCpu (*Data32);
        }
      }

 #if 0
    } else if (AsciiStrCmp (NodePtr->Name, "cpu-info") == 0) {
      PropertyPtr = FdtGetProperty (Fdt, Node, "address_width", &TempLen);
      ASSERT (TempLen > 0);
      if (TempLen > 0) {
        Data32 = (UINT32 *)(PropertyPtr->Data);
        DEBUG ((DEBUG_INFO, "\n         Property(00000000)  address_width"));
        DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));
        SizeOfMemorySpace = (UINT8)Fdt32ToCpu (*Data32);

        BuildCpuHob (SizeOfMemorySpace, 16);
      }

 #endif
    } else if (AsciiStrCmp (NodePtr->Name, "options") == 0) {
      INT32  UPLImageNode;
      DEBUG ((DEBUG_INFO, "  Found options node (%08X)", Node));

      UPLImageNode = FdtSubnodeOffsetNameLen (Fdt, Node, "upl-image@", (INT32)AsciiStrLen ("upl-image@"));
      if (UPLImageNode > 0) {
        DEBUG ((DEBUG_INFO, "  Found image@ node (%08X)", UPLImageNode));
        //
        // Build PayloadBase HOB .
        //
        PayloadBase = BuildGuidHob (&gUniversalPayloadBaseGuid, sizeof (UNIVERSAL_PAYLOAD_BASE));
        ASSERT (PayloadBase != NULL);
        if (PayloadBase == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        PayloadBase->Header.Revision = UNIVERSAL_PAYLOAD_BASE_REVISION;
        PayloadBase->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_BASE);

        PropertyPtr = FdtGetProperty (Fdt, UPLImageNode, "addr", &TempLen);

        ASSERT (TempLen > 0);
        if (TempLen > 0) {
          Data64       = (UINT64 *)(PropertyPtr->Data);
          StartAddress = Fdt64ToCpu (*Data64);
          DEBUG ((DEBUG_INFO, "\n         Property(00000000)  entry"));
          DEBUG ((DEBUG_INFO, "  %016lX\n", StartAddress));

          PayloadBase->Entry = (EFI_PHYSICAL_ADDRESS)StartAddress;
        }
      }
    }
    // Optional
    else if (AsciiStrnCmp (NodePtr->Name, "pci-rb@", AsciiStrLen ("pci-rb@")) == 0) {
      //
      // Create PCI Root Bridge Info Hob.
      //
      PciRootBridgeInfo = BuildGuidHob (&gUniversalPayloadPciRootBridgeInfoGuid, sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES) + sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE));
      ASSERT (PciRootBridgeInfo != NULL);
      if (PciRootBridgeInfo == NULL) {
        break;
      }

      PciRootBridgeInfo->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES) + sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE);
      PciRootBridgeInfo->Header.Revision = UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION;
      PciRootBridgeInfo->Count = 0;
      PciRootBridgeInfo->ResourceAssigned = FALSE;

      for (Property = FdtFirstPropertyOffset (Fdt, Node); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
        PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
        TempStr     = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);
        if (AsciiStrCmp (TempStr, "ranges") == 0) {
          PciRootBridgeInfo->Count = 1;
          //PciRootBridgeInfo->ResourceAssigned = FALSE;
          PciRootBridgeInfo->RootBridge[0].AllocationAttributes = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM | EFI_PCI_HOST_BRIDGE_MEM64_DECODE;
          PciRootBridgeInfo->RootBridge[0].Supports             = ROOT_BRIDGE_SUPPORTS_DEFAULT;
          PciRootBridgeInfo->RootBridge[0].MemAbove4G.Base      = PcdGet64 (PcdPciReservedMemAbove4GBBase);;
          PciRootBridgeInfo->RootBridge[0].MemAbove4G.Limit     = PcdGet64 (PcdPciReservedMemAbove4GBLimit);
          PciRootBridgeInfo->RootBridge[0].PMemAbove4G.Base     = PcdGet64 (PcdPciReservedPMemAbove4GBBase);
          PciRootBridgeInfo->RootBridge[0].PMemAbove4G.Limit    = PcdGet64 (PcdPciReservedPMemAbove4GBLimit);;

          Data64 = (UINT64 *)(PropertyPtr->Data);
          PciRootBridgeInfo->RootBridge[0].Mem.Base   = Fdt64ToCpu (*Data64);
          PciRootBridgeInfo->RootBridge[0].Mem.Limit  = Fdt64ToCpu (*(Data64 + 3));
          PciRootBridgeInfo->RootBridge[0].PMem.Base  = Fdt64ToCpu (*(Data64 + 4));
          PciRootBridgeInfo->RootBridge[0].PMem.Limit = Fdt64ToCpu (*(Data64 + 7));
          PciRootBridgeInfo->RootBridge[0].Io.Base    = Fdt64ToCpu (*(Data64 + 8));
          PciRootBridgeInfo->RootBridge[0].Io.Limit   = Fdt64ToCpu (*(Data64 + 11));
        }
        if (AsciiStrCmp (TempStr, "bus-range") == 0) {
          UINT16 *Data16;
          Data16 = (UINT16 *)(PropertyPtr->Data);
          PciRootBridgeInfo->RootBridge[0].Bus.Base =  Fdt16ToCpu (*Data16);
          PciRootBridgeInfo->RootBridge[0].Bus.Limit = Fdt16ToCpu (*(Data16 + 1));
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
      DebugPrintErrorLevelInfo->Header.Length   = sizeof (UEFI_PAYLOAD_DEBUG_PRINT_ERROR_LEVEL);

      PropertyPtr = FdtGetProperty (Fdt, Node, "errorlevel", &TempLen);
      ASSERT (TempLen > 0);
      if (TempLen > 0) {
        Data32 = (UINT32 *)(PropertyPtr->Data);
        DEBUG ((DEBUG_INFO, "\n         Property(00000000)  errorlevel"));
        DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));
        DebugPrintErrorLevelInfo->ErrorLevel = Fdt32ToCpu (*Data32);
      }
    }
  }

  DEBUG ((DEBUG_INFO, "\n"));

  return NewHobList;
}

/**
  It will Parse FDT -node based on information from bootloaders.
  @param[in]  FdtBase   The starting memory address of FdtBase
  @retval HobList   The base address of Hoblist.

**/
UINT64
FdtNodeParser (
  IN VOID  *Fdt
  )
{
  return ParseDtb (Fdt);
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
  IN VOID  *FdtBase
  )
{
  UINT64  NHobAddress;

  NHobAddress = 0;
  //
  // Check parameter type(
  //
  if (FdtCheckHeader (FdtBase) == 0) {
    DEBUG ((DEBUG_INFO, "%a() FDT blob\n", __func__));
    NHobAddress = FdtNodeParser ((VOID *)FdtBase);
  } else {
    DEBUG ((DEBUG_INFO, "%a() HOb list\n", __func__));
    mHobList = FdtBase;

    return (UINT64)(mHobList);
  }

  return NHobAddress;
}
