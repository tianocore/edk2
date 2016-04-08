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
  }
  return EFI_SUCCESS;
}
