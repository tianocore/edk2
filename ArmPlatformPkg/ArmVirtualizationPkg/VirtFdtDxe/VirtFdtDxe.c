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
#include <libfdt.h>

#include <Guid/Fdt.h>
#include <Guid/VirtioMmioTransport.h>

#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH                  Vendor;
  UINT64                              PhysBase;
  EFI_DEVICE_PATH_PROTOCOL            End;
} VIRTIO_TRANSPORT_DEVICE_PATH;
#pragma pack ()

typedef enum {
  PropertyTypeUnknown,
  PropertyTypeGic,
  PropertyTypeRtc,
  PropertyTypeVirtio,
  PropertyTypeUart,
  PropertyTypeTimer,
  PropertyTypePsci,
  PropertyTypeFwCfg,
} PROPERTY_TYPE;

typedef struct {
  PROPERTY_TYPE Type;
  CHAR8         Compatible[20];
} PROPERTY;

STATIC CONST PROPERTY CompatibleProperties[] = {
  { PropertyTypeGic,     "arm,cortex-a15-gic"  },
  { PropertyTypeRtc,     "arm,pl031"           },
  { PropertyTypeVirtio,  "virtio,mmio"         },
  { PropertyTypeUart,    "arm,pl011"           },
  { PropertyTypeTimer,   "arm,armv7-timer"     },
  { PropertyTypeTimer,   "arm,armv8-timer"     },
  { PropertyTypePsci,    "arm,psci-0.2"        },
  { PropertyTypeFwCfg,   "qemu,fw-cfg-mmio"    },
  { PropertyTypeUnknown, ""                    }
};

typedef struct {
  UINT32  Type;
  UINT32  Number;
  UINT32  Flags;
} INTERRUPT_PROPERTY;

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
  UINT64                         DistBase, CpuBase;
  CONST INTERRUPT_PROPERTY       *InterruptProp;
  INT32                          SecIntrNum, IntrNum, VirtIntrNum, HypIntrNum;
  CONST CHAR8                    *PsciMethod;
  UINT64                         FwCfgSelectorAddress;
  UINT64                         FwCfgSelectorSize;
  UINT64                         FwCfgDataAddress;
  UINT64                         FwCfgDataSize;

  DeviceTreeBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeBaseAddress);
  ASSERT (DeviceTreeBase != NULL);

  if (fdt_check_header (DeviceTreeBase) != 0) {
    DEBUG ((EFI_D_ERROR, "%a: No DTB found @ 0x%p\n", __FUNCTION__, DeviceTreeBase));
    return EFI_NOT_FOUND;
  }

  Status = gBS->InstallConfigurationTable (&gFdtTableGuid, DeviceTreeBase);
  ASSERT_EFI_ERROR (Status);

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
    ASSERT ((RegProp != NULL) || (PropType == PropertyTypeTimer) ||
      (PropType == PropertyTypePsci));

    switch (PropType) {
    case PropertyTypeFwCfg:
      ASSERT (Len == 2 * sizeof (UINT64));

      FwCfgDataAddress     = fdt64_to_cpu (((UINT64 *)RegProp)[0]);
      FwCfgDataSize        = 8;
      FwCfgSelectorAddress = FwCfgDataAddress + FwCfgDataSize;
      FwCfgSelectorSize    = 2;

      //
      // The following ASSERT()s express
      //
      //   Address + Size - 1 <= MAX_UINTN
      //
      // for both registers, that is, that the last byte in each MMIO range is
      // expressible as a MAX_UINTN. The form below is mathematically
      // equivalent, and it also prevents any unsigned overflow before the
      // comparison.
      //
      ASSERT (FwCfgSelectorAddress <= MAX_UINTN - FwCfgSelectorSize + 1);
      ASSERT (FwCfgDataAddress     <= MAX_UINTN - FwCfgDataSize     + 1);

      PcdSet64 (PcdFwCfgSelectorAddress, FwCfgSelectorAddress);
      PcdSet64 (PcdFwCfgDataAddress,     FwCfgDataAddress);

      DEBUG ((EFI_D_INFO, "Found FwCfg @ 0x%Lx/0x%Lx\n", FwCfgSelectorAddress,
        FwCfgDataAddress));
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

    case PropertyTypeGic:
      ASSERT (Len == 32);

      DistBase = fdt64_to_cpu (((UINT64 *)RegProp)[0]);
      CpuBase  = fdt64_to_cpu (((UINT64 *)RegProp)[2]);
      ASSERT (DistBase < MAX_UINT32);
      ASSERT (CpuBase < MAX_UINT32);

      PcdSet32 (PcdGicDistributorBase, (UINT32)DistBase);
      PcdSet32 (PcdGicInterruptInterfaceBase, (UINT32)CpuBase);

      DEBUG ((EFI_D_INFO, "Found GIC @ 0x%Lx/0x%Lx\n", DistBase, CpuBase));
      break;

    case PropertyTypeRtc:
      ASSERT (Len == 16);

      RegBase = fdt64_to_cpu (((UINT64 *)RegProp)[0]);
      ASSERT (RegBase < MAX_UINT32);

      PcdSet32 (PcdPL031RtcBase, (UINT32)RegBase);

      DEBUG ((EFI_D_INFO, "Found PL031 RTC @ 0x%Lx\n", RegBase));
      RtcNode = Node;
      break;

    case PropertyTypeTimer:
      //
      // - interrupts : Interrupt list for secure, non-secure, virtual and
      //  hypervisor timers, in that order.
      //
      InterruptProp = fdt_getprop (DeviceTreeBase, Node, "interrupts", &Len);
      ASSERT (Len == 48);

      SecIntrNum = fdt32_to_cpu (InterruptProp[0].Number)
                   + (InterruptProp[0].Type ? 16 : 0);
      IntrNum = fdt32_to_cpu (InterruptProp[1].Number)
                + (InterruptProp[1].Type ? 16 : 0);
      VirtIntrNum = fdt32_to_cpu (InterruptProp[2].Number)
                    + (InterruptProp[2].Type ? 16 : 0);
      HypIntrNum = fdt32_to_cpu (InterruptProp[3].Number)
                   + (InterruptProp[3].Type ? 16 : 0);

      DEBUG ((EFI_D_INFO, "Found Timer interrupts %d, %d, %d, %d\n",
        SecIntrNum, IntrNum, VirtIntrNum, HypIntrNum));

      PcdSet32 (PcdArmArchTimerSecIntrNum, SecIntrNum);
      PcdSet32 (PcdArmArchTimerIntrNum, IntrNum);
      PcdSet32 (PcdArmArchTimerVirtIntrNum, VirtIntrNum);
      PcdSet32 (PcdArmArchTimerHypIntrNum, HypIntrNum);
      break;

    case PropertyTypePsci:
      PsciMethod = fdt_getprop (DeviceTreeBase, Node, "method", &Len);

      if (PsciMethod && AsciiStrnCmp (PsciMethod, "hvc", 3) == 0) {
        PcdSet32 (PcdArmPsciMethod, 1);
      } else if (PsciMethod && AsciiStrnCmp (PsciMethod, "smc", 3) == 0) {
        PcdSet32 (PcdArmPsciMethod, 2);
      } else {
        DEBUG ((EFI_D_ERROR, "%a: Unknown PSCI method \"%a\"\n", __FUNCTION__,
          PsciMethod));
      }
      break;

    default:
      break;
    }
  }

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
  return EFI_SUCCESS;
}
