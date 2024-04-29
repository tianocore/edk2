/** @file
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
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

#define N_NON_RELOCATABLE       BIT31
#define P_NON_PREFETCHABLE      BIT30
#define SS_CONFIGURATION_SPACE  0
#define SS_IO_SPACE             BIT23
#define SS_32BIT_MEMORY_SPACE   BIT24
#define SS_64BIT_MEMORY_SPACE   BIT23+BIT24

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
EFIAPI
ParseDtb (
  IN VOID  *FdtBase
  )
{
  VOID                                *Fdt;
  INT32                               Node;
  INT32                               Property;
  INT32                               Depth;
  FDT_NODE_HEADER                     *NodePtr;
  CONST FDT_PROPERTY                  *PropertyPtr;
  CONST CHAR8                         *TempStr;
  INT32                               TempLen;
  UINT32                              *Data32;
  UINT64                              *Data64;
  UINT64                              StartAddress;
  INT32                               SubNode;
  UINT64                              NumberOfBytes;
  UINT32                              Attribute;
  UINT8                               ECCAttribute;
  UINT32                              ECCData, ECCData2;
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *Serial;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *PciRootBridgeInfo;
  UNIVERSAL_PAYLOAD_ACPI_TABLE        *PlatformAcpiTable;
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE      *SmbiosTable;
  UNIVERSAL_PAYLOAD_BASE              *PayloadBase;
  EFI_PEI_GRAPHICS_INFO_HOB           *GraphicsInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB    *GraphicsDev;
  UINT8                               SizeOfMemorySpace;
  UINT64                              FrameBufferBase;
  UINT32                              FrameBufferSize;
  UINTN                               MinimalNeededSize;
  EFI_PHYSICAL_ADDRESS                FreeMemoryBottom;
  EFI_PHYSICAL_ADDRESS                FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS                MemoryBottom;
  EFI_PHYSICAL_ADDRESS                MemoryTop;
  BOOLEAN                             IsHobConstructed;
  UINTN                               NewHobList;
  UINT8                               RootBridgeCount;
  UINT8                               index;
  UINTN                               HobDataSize;
  UINT8                               PciEnumDone;
  EFI_BOOT_MODE                       BootMode;
  UINT32                              MemType;
  UINT16                              GmaID;
  UINT8                               Base;
  UINT16                              *Data16;
  CHAR8                               *GmaStr;

  Fdt               = FdtBase;
  Depth             = 0;
  FrameBufferBase   = 0;
  FrameBufferSize   = 0;
  MinimalNeededSize = FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
  IsHobConstructed  = FALSE;
  NewHobList        = 0;
  PlatformAcpiTable = NULL;
  SmbiosTable       = NULL;
  PciRootBridgeInfo = NULL;
  RootBridgeCount   = 0;
  index             = 0;
  HobDataSize       = 0;
  PciEnumDone       = 0;
  BootMode          = 0;
  MemType           = 0;
  GmaID             = 0;

  DEBUG ((DEBUG_INFO, "FDT = 0x%x  %x\n", Fdt, Fdt32ToCpu (*((UINT32 *)Fdt))));
  DEBUG ((DEBUG_INFO, "Start parsing DTB data\n"));
  DEBUG ((DEBUG_INFO, "MinimalNeededSize :%x\n", MinimalNeededSize));

  for (Node = FdtNextNode (Fdt, 0, &Depth); Node >= 0; Node = FdtNextNode (Fdt, Node, &Depth)) {
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
          if (!IsHobConstructed) {
            if ((NumberOfBytes > MinimalNeededSize) && (StartAddress < BASE_4GB)) {
              MemoryBottom     = StartAddress + NumberOfBytes - MinimalNeededSize;
              FreeMemoryBottom = MemoryBottom;
              FreeMemoryTop    = StartAddress + NumberOfBytes;
              MemoryTop        = FreeMemoryTop;

              DEBUG ((DEBUG_INFO, "MemoryBottom :0x%llx\n", MemoryBottom));
              DEBUG ((DEBUG_INFO, "FreeMemoryBottom :0x%llx\n", FreeMemoryBottom));
              DEBUG ((DEBUG_INFO, "FreeMemoryTop :0x%llx\n", FreeMemoryTop));
              DEBUG ((DEBUG_INFO, "MemoryTop :0x%llx\n", MemoryTop));
              mHobList         =  HobConstructor ((VOID *)(UINTN)MemoryBottom, (VOID *)(UINTN)MemoryTop, (VOID *)(UINTN)FreeMemoryBottom, (VOID *)(UINTN)FreeMemoryTop);
              IsHobConstructed = TRUE;
              NewHobList       = (UINTN)mHobList;
              break;
            }
          }
        }
      }
    } // end of memory node
    else if (AsciiStrnCmp (NodePtr->Name, "pci-rb", AsciiStrLen ("pci-rb")) == 0) {
      RootBridgeCount++;
    }
  }

  index = RootBridgeCount - 1;
  Depth = 0;
  for (Node = FdtNextNode (Fdt, 0, &Depth); Node >= 0; Node = FdtNextNode (Fdt, Node, &Depth)) {
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
        } else if (AsciiStrCmp (TempStr, "ecc-detection-bits") == 0) {
          Data32  = (UINT32 *)(PropertyPtr->Data);
          ECCData = Fdt32ToCpu (*Data32);
        } else if (AsciiStrCmp (TempStr, "ecc-correction-bits") == 0) {
          Data32   = (UINT32 *)(PropertyPtr->Data);
          ECCData2 = Fdt32ToCpu (*Data32);
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

      BuildResourceDescriptorHob (EFI_RESOURCE_SYSTEM_MEMORY, Attribute, StartAddress, NumberOfBytes);
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
              SmbiosTable->SmBiosEntryPoint = (EFI_PHYSICAL_ADDRESS)(UINTN)(StartAddress);
            }
          }
        }
      }
    } // end of reserved-memory

    if (AsciiStrnCmp (NodePtr->Name, "framebuffer@", AsciiStrLen ("framebuffer@")) == 0) {
      //
      // Create GraphicInfo HOB.
      //
      GraphicsInfo = BuildGuidHob (&gEfiGraphicsInfoHobGuid, sizeof (EFI_PEI_GRAPHICS_INFO_HOB));
      ASSERT (GraphicsInfo != NULL);
      if (GraphicsInfo == NULL) {
        break;
      }

      ZeroMem (GraphicsInfo, sizeof (EFI_PEI_GRAPHICS_INFO_HOB));

      for (Property = FdtFirstPropertyOffset (Fdt, Node); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
        PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
        TempStr     = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);
        if (AsciiStrCmp (TempStr, "reg") == 0) {
          Data32                        = (UINT32 *)(PropertyPtr->Data);
          FrameBufferBase               = Fdt32ToCpu (*(Data32 + 0));
          FrameBufferSize               = Fdt32ToCpu (*(Data32 + 1));
          GraphicsInfo->FrameBufferBase = FrameBufferBase;
          GraphicsInfo->FrameBufferSize = (UINT32)FrameBufferSize;
        } else if (AsciiStrCmp (TempStr, "stride") == 0) {
          Data32                                       = (UINT32 *)(PropertyPtr->Data);
          GraphicsInfo->GraphicsMode.PixelsPerScanLine = Fdt32ToCpu (*Data32);
        } else if (AsciiStrCmp (TempStr, "width") == 0) {
          Data32                                          = (UINT32 *)(PropertyPtr->Data);
          GraphicsInfo->GraphicsMode.HorizontalResolution = Fdt32ToCpu (*Data32);
        } else if (AsciiStrCmp (TempStr, "height") == 0) {
          Data32                                        = (UINT32 *)(PropertyPtr->Data);
          GraphicsInfo->GraphicsMode.VerticalResolution = Fdt32ToCpu (*Data32);
        } else if (AsciiStrCmp (TempStr, "format") == 0) {
          TempStr = (CHAR8 *)(PropertyPtr->Data);
          if (AsciiStrCmp (TempStr, "a8r8g8b8") == 0) {
            GraphicsInfo->GraphicsMode.PixelFormat = PixelRedGreenBlueReserved8BitPerColor;
          } else if (AsciiStrCmp (TempStr, "a8b8g8r8") == 0) {
            GraphicsInfo->GraphicsMode.PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
          } else {
            GraphicsInfo->GraphicsMode.PixelFormat = PixelFormatMax;
          }
        } else if (AsciiStrCmp (TempStr, "display") == 0) {
          GmaStr = (CHAR8 *)(PropertyPtr->Data);
          GmaStr++;
          DEBUG ((DEBUG_INFO, "  display (%s)", GmaStr));
        }
      }
    } else if (AsciiStrCmp (NodePtr->Name, "options") == 0) {
      DEBUG ((DEBUG_INFO, "  Found options node (%08X)", Node));

      for (SubNode = FdtFirstSubnode (Fdt, Node); SubNode >= 0; SubNode = FdtNextSubnode (Fdt, SubNode)) {
        NodePtr = (FDT_NODE_HEADER *)((CONST CHAR8 *)Fdt + SubNode + Fdt32ToCpu (((FDT_HEADER *)Fdt)->OffsetDtStruct));
        DEBUG ((DEBUG_INFO, "\n      SubNode(%08X)  %a", SubNode, NodePtr->Name));

        if (AsciiStrnCmp (NodePtr->Name, "upl-images@", AsciiStrLen ("upl-images@")) == 0) {
          DEBUG ((DEBUG_INFO, "  Found image@ node \n"));
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

          PropertyPtr = FdtGetProperty (Fdt, SubNode, "addr", &TempLen);

          ASSERT (TempLen > 0);
          if (TempLen > 0) {
            Data64       = (UINT64 *)(PropertyPtr->Data);
            StartAddress = Fdt64ToCpu (*Data64);
            DEBUG ((DEBUG_INFO, "\n         Property(00000000)  entry"));
            DEBUG ((DEBUG_INFO, "  %016lX\n", StartAddress));

            PayloadBase->Entry = (EFI_PHYSICAL_ADDRESS)StartAddress;
          }
        }

        if (AsciiStrnCmp (NodePtr->Name, "upl-params", AsciiStrLen ("upl-params")) == 0) {
          PropertyPtr = FdtGetProperty (Fdt, SubNode, "addr-width", &TempLen);
          if (TempLen > 0) {
            Data32 = (UINT32 *)(PropertyPtr->Data);
            DEBUG ((DEBUG_INFO, "\n         Property(00000000)  address_width"));
            DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));
            SizeOfMemorySpace = (UINT8)Fdt32ToCpu (*Data32);
            BuildCpuHob (SizeOfMemorySpace, PcdGet8 (SizeOfIoSpace));
          }

          PropertyPtr = FdtGetProperty (Fdt, SubNode, "pci-enum-done", &TempLen);
          if (TempLen > 0) {
            Data16      = (UINT16 *)(PropertyPtr->Data);
            PciEnumDone = (UINT8)Fdt16ToCpu (*Data16);
            DEBUG ((DEBUG_INFO, "  Found PciEnumDone (%08X)\n", PciEnumDone));
          }

          PropertyPtr = FdtGetProperty (Fdt, SubNode, "boot-mode", &TempLen);
          if (TempLen > 0) {
            TempStr = (CHAR8 *)(PropertyPtr->Data);
            if (AsciiStrCmp (TempStr, "normal") == 0) {
              BootMode = BOOT_WITH_FULL_CONFIGURATION;
            } else if (AsciiStrCmp (TempStr, "fast") == 0) {
              BootMode = BOOT_WITH_MINIMAL_CONFIGURATION;
            } else if (AsciiStrCmp (TempStr, "full") == 0) {
              BootMode = BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS;
            } else if (AsciiStrCmp (TempStr, "default") == 0) {
              BootMode = BOOT_WITH_DEFAULT_SETTINGS;
            } else if (AsciiStrCmp (TempStr, "s4") == 0) {
              BootMode = BOOT_ON_S4_RESUME;
            } else if (AsciiStrCmp (TempStr, "s3") == 0) {
              BootMode = BOOT_ON_S3_RESUME;
            }
          }
        }
      }
    }
    // Optional
    else if (AsciiStrnCmp (NodePtr->Name, "pci-rb", AsciiStrLen ("pci-rb")) == 0) {
      DEBUG ((DEBUG_INFO, "  Found pci-rb node (%08X)", Node));

      HobDataSize = sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES) + RootBridgeCount *sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE);
      //
      // Create PCI Root Bridge Info Hob.
      //
      if (PciRootBridgeInfo == NULL) {
        PciRootBridgeInfo = BuildGuidHob (&gUniversalPayloadPciRootBridgeInfoGuid, HobDataSize);
        ASSERT (PciRootBridgeInfo != NULL);
        if (PciRootBridgeInfo == NULL) {
          break;
        }

        ZeroMem (PciRootBridgeInfo, HobDataSize);
        PciRootBridgeInfo->Header.Length    = sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES);
        PciRootBridgeInfo->Header.Revision  = UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION;
        PciRootBridgeInfo->Count            = RootBridgeCount;
        PciRootBridgeInfo->ResourceAssigned = (BOOLEAN)PciEnumDone;
      }

      for (SubNode = FdtFirstSubnode (Fdt, Node); SubNode >= 0; SubNode = FdtNextSubnode (Fdt, SubNode)) {
        NodePtr = (FDT_NODE_HEADER *)((CONST CHAR8 *)Fdt + SubNode + Fdt32ToCpu (((FDT_HEADER *)Fdt)->OffsetDtStruct));
        DEBUG ((DEBUG_INFO, "\n      SubNode(%08X)  %a", SubNode, NodePtr->Name));

        if (AsciiStrnCmp (NodePtr->Name, GmaStr, AsciiStrLen (GmaStr)) == 0) {
          DEBUG ((DEBUG_INFO, "  Found gma@ node \n"));
          //
          // Build Graphic info HOB .
          //
          GraphicsDev = BuildGuidHob (&gEfiGraphicsDeviceInfoHobGuid, sizeof (EFI_PEI_GRAPHICS_DEVICE_INFO_HOB));
          ASSERT (GraphicsDev != NULL);
          if (GraphicsDev == NULL) {
            break;
          }

          SetMem (GraphicsDev, sizeof (EFI_PEI_GRAPHICS_DEVICE_INFO_HOB), 0xFF);

          PropertyPtr = FdtGetProperty (Fdt, SubNode, "vendor-id", &TempLen);
          ASSERT (TempLen > 0);
          if (TempLen > 0) {
            Data32 = (UINT32 *)(PropertyPtr->Data);
            GmaID  = (UINT16)Fdt32ToCpu (*Data32);
            DEBUG ((DEBUG_INFO, "\n   vendor-id"));
            DEBUG ((DEBUG_INFO, "  %016lX\n", GmaID));

            GraphicsDev->VendorId = GmaID;
          }

          PropertyPtr = FdtGetProperty (Fdt, SubNode, "device-id", &TempLen);
          ASSERT (TempLen > 0);
          if (TempLen > 0) {
            Data32 = (UINT32 *)(PropertyPtr->Data);
            GmaID  = (UINT16)Fdt32ToCpu (*Data32);
            DEBUG ((DEBUG_INFO, "\n   device-id"));
            DEBUG ((DEBUG_INFO, "  %016lX\n", GmaID));

            GraphicsDev->DeviceId = GmaID;
          }

          PropertyPtr = FdtGetProperty (Fdt, SubNode, "revision-id", &TempLen);
          ASSERT (TempLen > 0);
          if (TempLen > 0) {
            Data32 = (UINT32 *)(PropertyPtr->Data);
            GmaID  = (UINT16)Fdt32ToCpu (*Data32);
            DEBUG ((DEBUG_INFO, "\n   revision-id"));
            DEBUG ((DEBUG_INFO, "  %016lX\n", GmaID));

            GraphicsDev->RevisionId = (UINT8)GmaID;
          }

          PropertyPtr = FdtGetProperty (Fdt, SubNode, "subsystem-vendor-id", &TempLen);
          ASSERT (TempLen > 0);
          if (TempLen > 0) {
            Data32 = (UINT32 *)(PropertyPtr->Data);
            GmaID  = (UINT16)Fdt32ToCpu (*Data32);
            DEBUG ((DEBUG_INFO, "\n   subsystem-vendor-id"));
            DEBUG ((DEBUG_INFO, "  %016lX\n", GmaID));

            GraphicsDev->SubsystemVendorId = GmaID;
          }

          PropertyPtr = FdtGetProperty (Fdt, SubNode, "subsystem-id", &TempLen);
          ASSERT (TempLen > 0);
          if (TempLen > 0) {
            Data32 = (UINT32 *)(PropertyPtr->Data);
            GmaID  = (UINT16)Fdt32ToCpu (*Data32);
            DEBUG ((DEBUG_INFO, "\n   subsystem-id"));
            DEBUG ((DEBUG_INFO, "  %016lX\n", GmaID));

            GraphicsDev->SubsystemId = GmaID;
          }
        }

        if (AsciiStrnCmp (NodePtr->Name, "espi", AsciiStrLen ("espi")) == 0) {
          SSubNode = FdtFirstSubnode (Fdt, SubNode); // serial
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

          for (Property = FdtFirstPropertyOffset (Fdt, SSubNode); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
            PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
            TempStr     = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);
            if (AsciiStrCmp (TempStr, "current-speed") == 0) {
              Data32 = (UINT32 *)(PropertyPtr->Data);
              DEBUG ((DEBUG_INFO, "\n         Property(%08X)  %a", Property, TempStr));
              DEBUG ((DEBUG_INFO, "  %X", Fdt32ToCpu (*Data32)));
              Serial->BaudRate = Fdt32ToCpu (*Data32);
            } else if (AsciiStrCmp (TempStr, "reg") == 0) {
              Data32       = (UINT32 *)(PropertyPtr->Data);
              Attribute    = Fdt32ToCpu (*(Data32 + 0));
              StartAddress = Fdt32ToCpu (*(Data32 + 1));
              DEBUG ((DEBUG_INFO, "\n in espi serial  Property(%08X)  %a", Property, TempStr));
              DEBUG ((DEBUG_INFO, " StartAddress   %016lX\n", StartAddress));
              DEBUG ((DEBUG_INFO, " Attribute      %016lX\n", Attribute));
              Serial->RegisterBase = StartAddress;
              Serial->UseMmio      = Attribute == 1 ? FALSE : TRUE;
            }
          }
        }
      }

      for (Property = FdtFirstPropertyOffset (Fdt, Node); Property >= 0; Property = FdtNextPropertyOffset (Fdt, Property)) {
        PropertyPtr = FdtGetPropertyByOffset (Fdt, Property, &TempLen);
        TempStr     = FdtGetString (Fdt, Fdt32ToCpu (PropertyPtr->NameOffset), NULL);

        if (AsciiStrCmp (TempStr, "ranges") == 0) {
          DEBUG ((DEBUG_INFO, "  Found ranges Property TempLen (%08X)\n", TempLen));

          PciRootBridgeInfo->RootBridge[index].AllocationAttributes = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM | EFI_PCI_HOST_BRIDGE_MEM64_DECODE;
          PciRootBridgeInfo->RootBridge[index].Supports             = ROOT_BRIDGE_SUPPORTS_DEFAULT;
          PciRootBridgeInfo->RootBridge[index].PMemAbove4G.Base     = PcdGet64 (PcdPciReservedPMemAbove4GBBase);
          PciRootBridgeInfo->RootBridge[index].PMemAbove4G.Limit    = PcdGet64 (PcdPciReservedPMemAbove4GBLimit);
          PciRootBridgeInfo->RootBridge[index].PMem.Base            = PcdGet32 (PcdPciReservedPMemBase);
          PciRootBridgeInfo->RootBridge[index].PMem.Limit           = PcdGet32 (PcdPciReservedPMemLimit);
          PciRootBridgeInfo->RootBridge[index].UID                  = index;
          PciRootBridgeInfo->RootBridge[index].HID                  = EISA_PNP_ID (0x0A03);

          Data32 = (UINT32 *)(PropertyPtr->Data);
          for (Base = 0; Base < 16; Base = Base + 7) {
            DEBUG ((DEBUG_INFO, "  Base :%x \n", Base));
            MemType = Fdt32ToCpu (*(Data32 + Base));
            if (((MemType) & (SS_64BIT_MEMORY_SPACE)) == SS_64BIT_MEMORY_SPACE) {
              DEBUG ((DEBUG_INFO, "  To program 64 mm \n"));
              PciRootBridgeInfo->RootBridge[index].MemAbove4G.Base  = Fdt32ToCpu (*(Data32 + Base + 2)) + LShiftU64 (Fdt32ToCpu (*(Data32 + Base + 1)), 32);
              PciRootBridgeInfo->RootBridge[index].MemAbove4G.Limit = PciRootBridgeInfo->RootBridge[index].MemAbove4G.Base + LShiftU64 (Fdt32ToCpu (*(Data32 + Base + 5)), 32) +  Fdt32ToCpu (*(Data32 + Base + 6)) -1;
            } else if (((MemType) & (SS_32BIT_MEMORY_SPACE)) == SS_32BIT_MEMORY_SPACE) {
              DEBUG ((DEBUG_INFO, "  To program 32 mem \n"));
              PciRootBridgeInfo->RootBridge[index].Mem.Base  = Fdt32ToCpu (*(Data32 + Base + 2));
              PciRootBridgeInfo->RootBridge[index].Mem.Limit = PciRootBridgeInfo->RootBridge[index].Mem.Base + Fdt32ToCpu (*(Data32 + Base + 6)) -1;
            } else if (((MemType) & (SS_IO_SPACE)) == SS_IO_SPACE) {
              DEBUG ((DEBUG_INFO, "  To program Io\n"));
              PciRootBridgeInfo->RootBridge[index].Io.Base  = Fdt32ToCpu (*(Data32 + Base + 2));
              PciRootBridgeInfo->RootBridge[index].Io.Limit = PciRootBridgeInfo->RootBridge[index].Io.Base + Fdt32ToCpu (*(Data32 +  Base + 6)) -1;
            }
          }

          DEBUG ((DEBUG_INFO, "RootBridgeCount %x, index :%x\n", RootBridgeCount, index));

          DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.Base %x, \n", PciRootBridgeInfo->RootBridge[index].Mem.Base));
          DEBUG ((DEBUG_INFO, "PciRootBridge->Mem.limit %x, \n", PciRootBridgeInfo->RootBridge[index].Mem.Limit));

          DEBUG ((DEBUG_INFO, "PciRootBridge->MemAbove4G.Base %llx, \n", PciRootBridgeInfo->RootBridge[index].MemAbove4G.Base));
          DEBUG ((DEBUG_INFO, "PciRootBridge->MemAbove4G.limit %llx, \n", PciRootBridgeInfo->RootBridge[index].MemAbove4G.Limit));

          DEBUG ((DEBUG_INFO, "PciRootBridge->Io.Base %llx, \n", PciRootBridgeInfo->RootBridge[index].Io.Base));
          DEBUG ((DEBUG_INFO, "PciRootBridge->Io.limit %llx, \n", PciRootBridgeInfo->RootBridge[index].Io.Limit));
          if (index > 0) {
            index--;
          }
        }

        if (AsciiStrCmp (TempStr, "bus-range") == 0) {
          DEBUG ((DEBUG_INFO, "  Found bus-range Property TempLen (%08X)\n", TempLen));

          Data16                                               = (UINT16 *)(PropertyPtr->Data);
          PciRootBridgeInfo->RootBridge[index].Bus.Base        = Fdt16ToCpu (*Data16) & 0xFF;
          PciRootBridgeInfo->RootBridge[index].Bus.Limit       = Fdt16ToCpu (*(Data16 + 1)) & 0xFF;
          PciRootBridgeInfo->RootBridge[index].Bus.Translation = 0;

          DEBUG ((DEBUG_INFO, "PciRootBridge->Bus.Base %x, \n", PciRootBridgeInfo->RootBridge[index].Bus.Base));
          DEBUG ((DEBUG_INFO, "PciRootBridge->Bus.limit %x, \n", PciRootBridgeInfo->RootBridge[index].Bus.Limit));
        }
      }
    }
  }

  ((EFI_HOB_HANDOFF_INFO_TABLE *)(mHobList))->BootMode = BootMode;
  DEBUG ((DEBUG_INFO, "\n"));

  return NewHobList;
}

/**
  It will Parse FDT -node based on information from bootloaders.
  @param[in]  FdtBase   The starting memory address of FdtBase
  @retval HobList   The base address of Hoblist.

**/
UINTN
EFIAPI
FdtNodeParser (
  IN VOID  *FdtBase
  )
{
  return ParseDtb (FdtBase);
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
UINTN
EFIAPI
UplInitHob (
  IN VOID  *FdtBase
  )
{
  UINTN  NHobAddress;

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

    return (UINTN)(mHobList);
  }

  return NHobAddress;
}
