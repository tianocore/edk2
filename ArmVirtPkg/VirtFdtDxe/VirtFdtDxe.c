/** @file
*  Device tree enumeration DXE driver for ARM Virtual Machines
*
*  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>
*
*  This program and the accompanying materials are
*  licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/VirtioMmioDeviceLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/HobLib.h>
#include <libfdt.h>
#include <Library/XenIoMmioLib.h>

#include <Guid/Fdt.h>
#include <Guid/VirtioMmioTransport.h>
#include <Guid/FdtHob.h>

#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH                  Vendor;
  UINT64                              PhysBase;
  EFI_DEVICE_PATH_PROTOCOL            End;
} VIRTIO_TRANSPORT_DEVICE_PATH;
#pragma pack ()

typedef enum {
  PropertyTypeUnknown,
  PropertyTypeRtc,
  PropertyTypeVirtio,
  PropertyTypeUart,
  PropertyTypePciHost,
  PropertyTypeXen,
} PROPERTY_TYPE;

typedef struct {
  PROPERTY_TYPE Type;
  CHAR8         Compatible[32];
} PROPERTY;

STATIC CONST PROPERTY CompatibleProperties[] = {
  { PropertyTypeRtc,     "arm,pl031"             },
  { PropertyTypeVirtio,  "virtio,mmio"           },
  { PropertyTypeUart,    "arm,pl011"             },
  { PropertyTypePciHost, "pci-host-ecam-generic" },
  { PropertyTypeXen,     "xen,xen"               },
  { PropertyTypeUnknown, ""                      }
};

STATIC
PROPERTY_TYPE
GetTypeFromNode (
  IN CONST CHAR8 *NodeType,
  IN UINTN       Size
  )
{
  CONST CHAR8    *Compatible;
  CONST PROPERTY *CompatibleProperty;

  //
  // A 'compatible' node may contain a sequence of NULL terminated
  // compatible strings so check each one
  //
  for (Compatible = NodeType; Compatible < NodeType + Size && *Compatible;
       Compatible += 1 + AsciiStrLen (Compatible)) {
    for (CompatibleProperty = CompatibleProperties; CompatibleProperty->Compatible[0]; CompatibleProperty++) {
      if (AsciiStrCmp (CompatibleProperty->Compatible, Compatible) == 0) {
        return CompatibleProperty->Type;
      }
    }
  }
  return PropertyTypeUnknown;
}

//
// We expect the "ranges" property of "pci-host-ecam-generic" to consist of
// records like this.
//
#pragma pack (1)
typedef struct {
  UINT32 Type;
  UINT64 ChildBase;
  UINT64 CpuBase;
  UINT64 Size;
} DTB_PCI_HOST_RANGE_RECORD;
#pragma pack ()

#define DTB_PCI_HOST_RANGE_RELOCATABLE  BIT31
#define DTB_PCI_HOST_RANGE_PREFETCHABLE BIT30
#define DTB_PCI_HOST_RANGE_ALIASED      BIT29
#define DTB_PCI_HOST_RANGE_MMIO32       BIT25
#define DTB_PCI_HOST_RANGE_MMIO64       (BIT25 | BIT24)
#define DTB_PCI_HOST_RANGE_IO           BIT24
#define DTB_PCI_HOST_RANGE_TYPEMASK     (BIT31 | BIT30 | BIT29 | BIT25 | BIT24)

/**
  Process the device tree node describing the generic PCI host controller.

  param[in] DeviceTreeBase  Pointer to the device tree.

  param[in] Node            Offset of the device tree node whose "compatible"
                            property is "pci-host-ecam-generic".

  param[in] RegProp         Pointer to the "reg" property of Node. The caller
                            is responsible for ensuring that the size of the
                            property is 4 UINT32 cells.

  @retval EFI_SUCCESS         Parsing successful, properties parsed from Node
                              have been stored in dynamic PCDs.

  @retval EFI_PROTOCOL_ERROR  Parsing failed. PCDs are left unchanged.
**/
STATIC
EFI_STATUS
EFIAPI
ProcessPciHost (
  IN CONST VOID *DeviceTreeBase,
  IN INT32      Node,
  IN CONST VOID *RegProp
  )
{
  UINT64     ConfigBase, ConfigSize;
  CONST VOID *Prop;
  INT32      Len;
  UINT32     BusMin, BusMax;
  UINT32     RecordIdx;
  UINT64     IoBase, IoSize, IoTranslation;
  UINT64     MmioBase, MmioSize, MmioTranslation;

  //
  // Fetch the ECAM window.
  //
  ConfigBase = fdt64_to_cpu (((CONST UINT64 *)RegProp)[0]);
  ConfigSize = fdt64_to_cpu (((CONST UINT64 *)RegProp)[1]);

  //
  // Fetch the bus range (note: inclusive).
  //
  Prop = fdt_getprop (DeviceTreeBase, Node, "bus-range", &Len);
  if (Prop == NULL || Len != 2 * sizeof(UINT32)) {
    DEBUG ((EFI_D_ERROR, "%a: 'bus-range' not found or invalid\n",
      __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }
  BusMin = fdt32_to_cpu (((CONST UINT32 *)Prop)[0]);
  BusMax = fdt32_to_cpu (((CONST UINT32 *)Prop)[1]);

  //
  // Sanity check: the config space must accommodate all 4K register bytes of
  // all 8 functions of all 32 devices of all buses.
  //
  if (BusMax < BusMin || BusMax - BusMin == MAX_UINT32 ||
      DivU64x32 (ConfigSize, SIZE_4KB * 8 * 32) < BusMax - BusMin + 1) {
    DEBUG ((EFI_D_ERROR, "%a: invalid 'bus-range' and/or 'reg'\n",
      __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  //
  // Iterate over "ranges".
  //
  Prop = fdt_getprop (DeviceTreeBase, Node, "ranges", &Len);
  if (Prop == NULL || Len == 0 ||
      Len % sizeof (DTB_PCI_HOST_RANGE_RECORD) != 0) {
    DEBUG ((EFI_D_ERROR, "%a: 'ranges' not found or invalid\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  //
  // IoBase, IoTranslation, MmioBase and MmioTranslation are initialized only
  // in order to suppress '-Werror=maybe-uninitialized' warnings *incorrectly*
  // emitted by some gcc versions.
  //
  IoBase = 0;
  IoTranslation = 0;
  MmioBase = 0;
  MmioTranslation = 0;

  //
  // IoSize and MmioSize are initialized to zero because the logic below
  // requires it.
  //
  IoSize = 0;
  MmioSize = 0;
  for (RecordIdx = 0; RecordIdx < Len / sizeof (DTB_PCI_HOST_RANGE_RECORD);
       ++RecordIdx) {
    CONST DTB_PCI_HOST_RANGE_RECORD *Record;

    Record = (CONST DTB_PCI_HOST_RANGE_RECORD *)Prop + RecordIdx;
    switch (fdt32_to_cpu (Record->Type) & DTB_PCI_HOST_RANGE_TYPEMASK) {
    case DTB_PCI_HOST_RANGE_IO:
      IoBase = fdt64_to_cpu (Record->ChildBase);
      IoSize = fdt64_to_cpu (Record->Size);
      IoTranslation = fdt64_to_cpu (Record->CpuBase) - IoBase;
      break;

    case DTB_PCI_HOST_RANGE_MMIO32:
      MmioBase = fdt64_to_cpu (Record->ChildBase);
      MmioSize = fdt64_to_cpu (Record->Size);
      MmioTranslation = fdt64_to_cpu (Record->CpuBase) - MmioBase;

      if (MmioBase > MAX_UINT32 || MmioSize > MAX_UINT32 ||
          MmioBase + MmioSize > SIZE_4GB) {
        DEBUG ((EFI_D_ERROR, "%a: MMIO32 space invalid\n", __FUNCTION__));
        return EFI_PROTOCOL_ERROR;
      }

      if (MmioTranslation != 0) {
        DEBUG ((EFI_D_ERROR, "%a: unsupported nonzero MMIO32 translation "
          "0x%Lx\n", __FUNCTION__, MmioTranslation));
        return EFI_UNSUPPORTED;
      }

      break;
    }
  }
  if (IoSize == 0 || MmioSize == 0) {
    DEBUG ((EFI_D_ERROR, "%a: %a space empty\n", __FUNCTION__,
      (IoSize == 0) ? "IO" : "MMIO32"));
    return EFI_PROTOCOL_ERROR;
  }

  PcdSet64 (PcdPciExpressBaseAddress, ConfigBase);

  PcdSet32 (PcdPciBusMin, BusMin);
  PcdSet32 (PcdPciBusMax, BusMax);

  PcdSet64 (PcdPciIoBase,        IoBase);
  PcdSet64 (PcdPciIoSize,        IoSize);
  PcdSet64 (PcdPciIoTranslation, IoTranslation);

  PcdSet32 (PcdPciMmio32Base, (UINT32)MmioBase);
  PcdSet32 (PcdPciMmio32Size, (UINT32)MmioSize);

  PcdSetBool (PcdPciDisableBusEnumeration, FALSE);

  DEBUG ((EFI_D_INFO, "%a: Config[0x%Lx+0x%Lx) Bus[0x%x..0x%x] "
    "Io[0x%Lx+0x%Lx)@0x%Lx Mem[0x%Lx+0x%Lx)@0x%Lx\n", __FUNCTION__, ConfigBase,
    ConfigSize, BusMin, BusMax, IoBase, IoSize, IoTranslation, MmioBase,
    MmioSize, MmioTranslation));
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
InitializeVirtFdtDxe (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  VOID                           *Hob;
  VOID                           *DeviceTreeBase;
  INT32                          Node, Prev;
  INT32                          RtcNode;
  EFI_STATUS                     Status;
  CONST CHAR8                    *Type;
  INT32                          Len;
  PROPERTY_TYPE                  PropType;
  CONST VOID                     *RegProp;
  VIRTIO_TRANSPORT_DEVICE_PATH   *DevicePath;
  EFI_HANDLE                     Handle;
  UINT64                         RegBase;
  BOOLEAN                        HavePci;

  Hob = GetFirstGuidHob(&gFdtHobGuid);
  if (Hob == NULL || GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64)) {
    return EFI_NOT_FOUND;
  }
  DeviceTreeBase = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);

  if (fdt_check_header (DeviceTreeBase) != 0) {
    DEBUG ((EFI_D_ERROR, "%a: No DTB found @ 0x%p\n", __FUNCTION__, DeviceTreeBase));
    return EFI_NOT_FOUND;
  }

  DEBUG ((EFI_D_INFO, "%a: DTB @ 0x%p\n", __FUNCTION__, DeviceTreeBase));

  RtcNode = -1;
  HavePci = FALSE;
  //
  // Now enumerate the nodes and install peripherals that we are interested in,
  // i.e., GIC, RTC and virtio MMIO nodes
  //
  for (Prev = 0;; Prev = Node) {
    Node = fdt_next_node (DeviceTreeBase, Prev, NULL);
    if (Node < 0) {
      break;
    }

    Type = fdt_getprop (DeviceTreeBase, Node, "compatible", &Len);
    if (Type == NULL) {
      continue;
    }

    PropType = GetTypeFromNode (Type, Len);
    if (PropType == PropertyTypeUnknown) {
      continue;
    }

    //
    // Get the 'reg' property of this node. For now, we will assume
    // 8 byte quantities for base and size, respectively.
    // TODO use #cells root properties instead
    //
    RegProp = fdt_getprop (DeviceTreeBase, Node, "reg", &Len);
    ASSERT (RegProp != NULL);

    switch (PropType) {
    case PropertyTypePciHost:
      ASSERT (Len == 2 * sizeof (UINT64));
      Status = ProcessPciHost (DeviceTreeBase, Node, RegProp);
      ASSERT_EFI_ERROR (Status);
      HavePci = TRUE;
      break;

    case PropertyTypeVirtio:
      ASSERT (Len == 16);
      //
      // Create a unique device path for this transport on the fly
      //
      RegBase = fdt64_to_cpu (((UINT64 *)RegProp)[0]);
      DevicePath = (VIRTIO_TRANSPORT_DEVICE_PATH *)CreateDeviceNode (
                                    HARDWARE_DEVICE_PATH,
                                    HW_VENDOR_DP,
                                    sizeof (VIRTIO_TRANSPORT_DEVICE_PATH));
      if (DevicePath == NULL) {
        DEBUG ((EFI_D_ERROR, "%a: Out of memory\n", __FUNCTION__));
        break;
      }

      CopyMem (&DevicePath->Vendor.Guid, &gVirtioMmioTransportGuid,
        sizeof (EFI_GUID));
      DevicePath->PhysBase = RegBase;
      SetDevicePathNodeLength (&DevicePath->Vendor,
                               sizeof (*DevicePath) - sizeof (DevicePath->End));
      SetDevicePathEndNode (&DevicePath->End);

      Handle = NULL;
      Status = gBS->InstallProtocolInterface (&Handle,
                     &gEfiDevicePathProtocolGuid, EFI_NATIVE_INTERFACE,
                     DevicePath);
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "%a: Failed to install the EFI_DEVICE_PATH "
          "protocol on a new handle (Status == %r)\n",
          __FUNCTION__, Status));
        FreePool (DevicePath);
        break;
      }

      Status = VirtioMmioInstallDevice (RegBase, Handle);
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "%a: Failed to install VirtIO transport @ 0x%Lx "
          "on handle %p (Status == %r)\n", __FUNCTION__, RegBase,
          Handle, Status));

        Status = gBS->UninstallProtocolInterface (Handle,
                        &gEfiDevicePathProtocolGuid, DevicePath);
        ASSERT_EFI_ERROR (Status);
        FreePool (DevicePath);
      }
      break;

    case PropertyTypeRtc:
      ASSERT (Len == 16);

      RegBase = fdt64_to_cpu (((UINT64 *)RegProp)[0]);
      ASSERT (RegBase < MAX_UINT32);

      PcdSet32 (PcdPL031RtcBase, (UINT32)RegBase);

      DEBUG ((EFI_D_INFO, "Found PL031 RTC @ 0x%Lx\n", RegBase));
      RtcNode = Node;
      break;

    case PropertyTypeXen:
      ASSERT (Len == 16);

      //
      // Retrieve the reg base from this node and wire it up to the
      // MMIO flavor of the XenBus root device I/O protocol
      //
      RegBase = fdt64_to_cpu (((UINT64 *)RegProp)[0]);
      Handle = NULL;
      Status = XenIoMmioInstall (&Handle, RegBase);
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "%a: XenIoMmioInstall () failed on a new handle "
          "(Status == %r)\n", __FUNCTION__, Status));
        break;
      }

      DEBUG ((EFI_D_INFO, "Found Xen node with Grant table @ 0x%Lx\n", RegBase));

      break;

    default:
      break;
    }
  }

  if (!FeaturePcdGet (PcdPureAcpiBoot)) {
    //
    // Only install the FDT as a configuration table if we want to leave it up
    // to the OS to decide whether it prefers ACPI over DT.
    //
    Status = gBS->InstallConfigurationTable (&gFdtTableGuid, DeviceTreeBase);
    ASSERT_EFI_ERROR (Status);

    //
    // UEFI takes ownership of the RTC hardware, and exposes its functionality
    // through the UEFI Runtime Services GetTime, SetTime, etc. This means we
    // need to disable it in the device tree to prevent the OS from attaching its
    // device driver as well.
    //
    if ((RtcNode != -1) &&
        fdt_setprop_string (DeviceTreeBase, RtcNode, "status",
          "disabled") != 0) {
      DEBUG ((EFI_D_WARN, "Failed to set PL031 status to 'disabled'\n"));
    }

    if (HavePci) {
      //
      // Set the /chosen/linux,pci-probe-only property to 1, so that the PCI
      // setup we will perform in the firmware is honored by the Linux OS,
      // rather than torn down and done from scratch. This is generally a more
      // sensible approach, and aligns with what ACPI based OSes do in general.
      //
      // In case we are exposing an emulated VGA PCI device to the guest, which
      // may subsequently get exposed via the Graphics Output protocol and
      // driven as an efifb by Linux, we need this setting to prevent the
      // framebuffer from becoming unresponsive.
      //
      Node = fdt_path_offset (DeviceTreeBase, "/chosen");
      if (Node < 0) {
        Node = fdt_add_subnode (DeviceTreeBase, 0, "/chosen");
      }
      if (Node < 0 ||
          fdt_setprop_u32 (DeviceTreeBase, Node, "linux,pci-probe-only", 1) < 0) {
        DEBUG ((EFI_D_WARN, "Failed to set /chosen/linux,pci-probe-only property\n"));
      }
    }
  }
  return EFI_SUCCESS;
}
