/*++

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BdsPlatform.c

Abstract:

  This file include all platform action which can be customized
  by IBV/OEM.

--*/

#include "BdsPlatform.h"

#define IS_PCI_ISA_PDECODE(_p)        IS_CLASS3 (_p, PCI_CLASS_BRIDGE, PCI_CLASS_BRIDGE_ISA_PDECODE, 0)

extern BOOLEAN  gConnectAllHappened;
extern USB_CLASS_FORMAT_DEVICE_PATH gUsbClassKeyboardDevicePath;

EFI_GUID                    *gTableGuidArray[] = {
    &gEfiAcpi20TableGuid, &gEfiAcpiTableGuid, &gEfiSmbiosTableGuid, &gEfiMpsTableGuid
  };

//
// BDS Platform Functions
//

VOID
GetSystemTablesFromHob (
  VOID
  )
/*++

Routine Description:
  Find GUID'ed HOBs that contain EFI_PHYSICAL_ADDRESS of ACPI, SMBIOS, MPs tables

Arguments:
  None

Returns:
  None.

--*/
{
  EFI_PEI_HOB_POINTERS        GuidHob;
  EFI_PEI_HOB_POINTERS        HobStart;
  EFI_PHYSICAL_ADDRESS        *Table;
  UINTN                       Index;

  //
  // Get Hob List
  //
  HobStart.Raw = GetHobList ();
  //
  // Iteratively add ACPI Table, SMBIOS Table, MPS Table to EFI System Table
  //
  for (Index = 0; Index < sizeof (gTableGuidArray) / sizeof (*gTableGuidArray); ++Index) {
    GuidHob.Raw = GetNextGuidHob (gTableGuidArray[Index], HobStart.Raw);
    if (GuidHob.Raw != NULL) {
      Table = GET_GUID_HOB_DATA (GuidHob.Guid);
      if (Table != NULL) {
        //
        // Check if Mps Table/Smbios Table/Acpi Table exists in E/F seg,
        // According to UEFI Spec, we should make sure Smbios table, 
        // ACPI table and Mps tables kept in memory of specified type
        //
        ConvertSystemTable(gTableGuidArray[Index], (VOID**)&Table);
        gBS->InstallConfigurationTable (gTableGuidArray[Index], (VOID *)Table);
      }
    }
  }

  return ;
}

#if 0
VOID
PrintMemoryMap (
  VOID
  )
{
  EFI_MEMORY_DESCRIPTOR       *MemMap;
  EFI_MEMORY_DESCRIPTOR       *MemMapPtr;
  UINTN                       MemMapSize;
  UINTN                       MapKey, DescriptorSize;
  UINTN                       Index;
  UINT32                      DescriptorVersion;
  UINT64                      Bytes;
  EFI_STATUS                  Status;

  MemMapSize = 0;
  MemMap     = NULL;
  Status = gBS->GetMemoryMap (&MemMapSize, MemMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  MemMapSize += EFI_PAGE_SIZE;
  Status = gBS->AllocatePool (EfiBootServicesData, MemMapSize, &MemMap);
  ASSERT (Status == EFI_SUCCESS);
  Status = gBS->GetMemoryMap (&MemMapSize, MemMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  ASSERT (Status == EFI_SUCCESS);
  MemMapPtr = MemMap;

  ASSERT (DescriptorVersion == EFI_MEMORY_DESCRIPTOR_VERSION);

  for (Index = 0; Index < MemMapSize / DescriptorSize; Index ++) {
    Bytes = LShiftU64 (MemMap->NumberOfPages, 12);
    DEBUG ((EFI_D_ERROR, "%lX-%lX  %lX %lX %X\n",
          MemMap->PhysicalStart, 
          MemMap->PhysicalStart + Bytes - 1,
          MemMap->NumberOfPages, 
          MemMap->Attribute,
          (UINTN)MemMap->Type));
    MemMap = (EFI_MEMORY_DESCRIPTOR *)((UINTN)MemMap + DescriptorSize);
  }

  gBS->FreePool (MemMapPtr);
}
#endif

VOID
UpdateMemoryMap (
  VOID
  )
{
  EFI_STATUS                      Status;
  EFI_PEI_HOB_POINTERS            GuidHob;
  VOID                            *Table;
  MEMORY_DESC_HOB                 MemoryDescHob;
  UINTN                           Index;
  EFI_PHYSICAL_ADDRESS            Memory;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR Descriptor;
  
  GuidHob.Raw = GetFirstGuidHob (&gLdrMemoryDescriptorGuid);
  if (GuidHob.Raw == NULL) {
    DEBUG ((EFI_D_ERROR, "Fail to get gEfiLdrMemoryDescriptorGuid from GUID HOB LIST!\n"));
    return;
  }
  Table = GET_GUID_HOB_DATA (GuidHob.Guid);
  if (Table == NULL) {
    DEBUG ((EFI_D_ERROR, "Fail to get gEfiLdrMemoryDescriptorGuid from GUID HOB LIST!\n"));
    return;
  }
  MemoryDescHob.MemDescCount = *(UINTN *)Table;
  MemoryDescHob.MemDesc      = *(EFI_MEMORY_DESCRIPTOR **)((UINTN)Table + sizeof(UINTN));

  //
  // Add ACPINVS, ACPIReclaim, and Reserved memory to MemoryMap
  //
  for (Index = 0; Index < MemoryDescHob.MemDescCount; Index++) {
    if (MemoryDescHob.MemDesc[Index].PhysicalStart < 0x100000) {
      continue;
    }
    if (MemoryDescHob.MemDesc[Index].PhysicalStart >= 0x100000000ULL) {
      continue;
    }
    if ((MemoryDescHob.MemDesc[Index].Type == EfiReservedMemoryType) ||
        (MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesData) ||
        (MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesCode) ||
        (MemoryDescHob.MemDesc[Index].Type == EfiACPIReclaimMemory) ||
        (MemoryDescHob.MemDesc[Index].Type == EfiACPIMemoryNVS)) {
      DEBUG ((EFI_D_ERROR, "PhysicalStart - 0x%016lx, ", MemoryDescHob.MemDesc[Index].PhysicalStart));
      DEBUG ((EFI_D_ERROR, "PageNumber    - 0x%016lx, ", MemoryDescHob.MemDesc[Index].NumberOfPages));
      DEBUG ((EFI_D_ERROR, "Attribute     - 0x%016lx, ", MemoryDescHob.MemDesc[Index].Attribute));
      DEBUG ((EFI_D_ERROR, "Type          - 0x%08x\n", MemoryDescHob.MemDesc[Index].Type));
      if ((MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesData) ||
          (MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesCode)) {
        //
        // For RuntimeSevicesData and RuntimeServicesCode, they are BFV or DxeCore.
        // The memory type is assigned in EfiLdr
        //
        Status = gDS->GetMemorySpaceDescriptor (MemoryDescHob.MemDesc[Index].PhysicalStart, &Descriptor);
        if (EFI_ERROR (Status)) {
          continue;
        }
        if (Descriptor.GcdMemoryType != EfiGcdMemoryTypeReserved) {
          //
          // BFV or tested DXE core
          //
          continue;
        }
        //
        // Untested DXE Core region, free and remove
        //
        Status = gDS->FreeMemorySpace (
                        MemoryDescHob.MemDesc[Index].PhysicalStart,
                        LShiftU64 (MemoryDescHob.MemDesc[Index].NumberOfPages, EFI_PAGE_SHIFT)
                        );
        if (EFI_ERROR (Status)) {
          DEBUG ((EFI_D_ERROR, "FreeMemorySpace fail - %r!\n", Status));
          continue;
        }
        Status = gDS->RemoveMemorySpace (
                        MemoryDescHob.MemDesc[Index].PhysicalStart,
                        LShiftU64 (MemoryDescHob.MemDesc[Index].NumberOfPages, EFI_PAGE_SHIFT)
                        );
        if (EFI_ERROR (Status)) {
          DEBUG ((EFI_D_ERROR, "RemoveMemorySpace fail - %r!\n", Status));
          continue;
        }

        //
        // Convert Runtime type to BootTime type
        //
        if (MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesData) {
          MemoryDescHob.MemDesc[Index].Type = EfiBootServicesData;
        } else {
          MemoryDescHob.MemDesc[Index].Type = EfiBootServicesCode;
        }

        //
        // PassThrough, let below code add and alloate.
        //
      }
      //
      // ACPI or reserved memory
      //
      Status = gDS->AddMemorySpace (
                      EfiGcdMemoryTypeSystemMemory,
                      MemoryDescHob.MemDesc[Index].PhysicalStart,
                      LShiftU64 (MemoryDescHob.MemDesc[Index].NumberOfPages, EFI_PAGE_SHIFT),
                      MemoryDescHob.MemDesc[Index].Attribute
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "AddMemorySpace fail - %r!\n", Status));
        if ((MemoryDescHob.MemDesc[Index].Type == EfiACPIReclaimMemory) ||
            (MemoryDescHob.MemDesc[Index].Type == EfiACPIMemoryNVS)) {
          //
          // For EfiACPIReclaimMemory and EfiACPIMemoryNVS, it must success.
          // For EfiReservedMemoryType, there maybe overlap. So skip check here.
          //
//          ASSERT_EFI_ERROR (Status);
        }
        continue;
      }

      Memory = MemoryDescHob.MemDesc[Index].PhysicalStart;
      Status = gBS->AllocatePages (
                      AllocateAddress,
                      (EFI_MEMORY_TYPE)MemoryDescHob.MemDesc[Index].Type,
                      (UINTN)MemoryDescHob.MemDesc[Index].NumberOfPages,
                      &Memory
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "AllocatePages fail - %r!\n", Status));
        //
        // For the page added, it must be allocated.
        //
//        ASSERT_EFI_ERROR (Status);
        continue;
      }
    }
  }
  
}

EFI_STATUS
DisableUsbLegacySupport(
  void
  )
/*++

Routine Description:
  Disabble the USB legacy Support in all Ehci and Uhci.
  This function assume all PciIo handles have been created in system.
  
Arguments:
  None
  
Returns:
  EFI_SUCCESS
  EFI_NOT_FOUND
--*/
{
  EFI_STATUS                            Status;
  EFI_HANDLE                            *HandleArray;
  UINTN                                 HandleArrayCount;
  UINTN                                 Index;
  EFI_PCI_IO_PROTOCOL                   *PciIo;
  UINT8                                 Class[3];
  UINT16                                Command;
  UINT32                                HcCapParams;
  UINT32                                ExtendCap;
  UINT32                                Value;
  UINT32                                TimeOut;
  
  //
  // Find the usb host controller 
  //   
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleArrayCount,
                  &HandleArray
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleArrayCount; Index++) {
      Status = gBS->HandleProtocol (
                      HandleArray[Index],
                      &gEfiPciIoProtocolGuid,
                      (VOID **)&PciIo
                      );
      if (!EFI_ERROR (Status)) {
        //
        // Find the USB host controller controller
        //
        Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x09, 3, &Class);
        if (!EFI_ERROR (Status)) {
          if ((PCI_CLASS_SERIAL == Class[2]) &&
              (PCI_CLASS_SERIAL_USB == Class[1])) {
            if (PCI_IF_UHCI == Class[0]) {
              //
              // Found the UHCI, then disable the legacy support
              //
              Command = 0;
              Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0xC0, 1, &Command);
            } else if (PCI_IF_EHCI == Class[0]) {
              //
              // Found the EHCI, then disable the legacy support
              //
              Status = PciIo->Mem.Read (
                                   PciIo,
                                   EfiPciIoWidthUint32,
                                   0,                   //EHC_BAR_INDEX
                                   (UINT64) 0x08,       //EHC_HCCPARAMS_OFFSET
                                   1,
                                   &HcCapParams
                                   );
              
              ExtendCap = (HcCapParams >> 8) & 0xFF;
              //
              // Disable the SMI in USBLEGCTLSTS firstly
              //
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);
              Value &= 0xFFFF0000;
              PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);
              
              //
              // Get EHCI Ownership from legacy bios
              //
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
              Value |= (0x1 << 24);
              PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

              TimeOut = 40;
              while (TimeOut--) {
                gBS->Stall (500);

                PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

                if ((Value & 0x01010000) == 0x01000000) {
                  break;
                }
              }
            }
          } 
        }
      }
    }
  } else {
    return Status;
  }
  gBS->FreePool (HandleArray);
  return EFI_SUCCESS;
}


VOID
EFIAPI
PlatformBdsInit (
  VOID
  )
/*++

Routine Description:

  Platform Bds init. Incude the platform firmware vendor, revision
  and so crc check.

Arguments:

Returns:

  None.

--*/
{
  GetSystemTablesFromHob ();

  UpdateMemoryMap ();
  
  //
  // Append Usb Keyboard short form DevicePath into "ConInDev" 
  //
  BdsLibUpdateConsoleVariable (
    VarConsoleInpDev,
    (EFI_DEVICE_PATH_PROTOCOL *) &gUsbClassKeyboardDevicePath,
    NULL
    );
}

UINT64
GetPciExpressBaseAddressForRootBridge (
  IN UINTN    HostBridgeNumber,
  IN UINTN    RootBridgeNumber
  )
/*++

Routine Description:
  This routine is to get PciExpress Base Address for this RootBridge

Arguments:
  HostBridgeNumber - The number of HostBridge
  RootBridgeNumber - The number of RootBridge
    
Returns:
  UINT64 - PciExpressBaseAddress for this HostBridge and RootBridge

--*/
{
  EFI_PCI_EXPRESS_BASE_ADDRESS_INFORMATION *PciExpressBaseAddressInfo;
  UINTN                                    BufferSize;
  UINT32                                   Index;
  UINT32                                   Number;
  EFI_PEI_HOB_POINTERS                     GuidHob;

  //
  // Get PciExpressAddressInfo Hob
  //
  PciExpressBaseAddressInfo = NULL;
  BufferSize                = 0;
  GuidHob.Raw = GetFirstGuidHob (&gEfiPciExpressBaseAddressGuid);
  if (GuidHob.Raw != NULL) {
    PciExpressBaseAddressInfo = GET_GUID_HOB_DATA (GuidHob.Guid);
    BufferSize                = GET_GUID_HOB_DATA_SIZE (GuidHob.Guid);
  } else {
    return 0;
  }

  //
  // Search the PciExpress Base Address in the Hob for current RootBridge
  //
  Number = (UINT32)(BufferSize / sizeof(EFI_PCI_EXPRESS_BASE_ADDRESS_INFORMATION));
  for (Index = 0; Index < Number; Index++) {
    if ((PciExpressBaseAddressInfo[Index].HostBridgeNumber == HostBridgeNumber) &&
        (PciExpressBaseAddressInfo[Index].RootBridgeNumber == RootBridgeNumber)) {
      return PciExpressBaseAddressInfo[Index].PciExpressBaseAddress;
    }
  }

  //
  // Do not find the PciExpress Base Address in the Hob
  //
  return 0;  
}

VOID
PatchPciRootBridgeDevicePath (
  IN UINTN    HostBridgeNumber,
  IN UINTN    RootBridgeNumber,
  IN PLATFORM_ROOT_BRIDGE_DEVICE_PATH  *RootBridge
  )
{
  UINT64  PciExpressBase;

  PciExpressBase = GetPciExpressBaseAddressForRootBridge (HostBridgeNumber, RootBridgeNumber);
  
  DEBUG ((EFI_D_INFO, "Get PciExpress Address from Hob: 0x%X\n", PciExpressBase));
  
  if (PciExpressBase != 0) {
    RootBridge->PciRootBridge.HID = EISA_PNP_ID(0x0A08);
  }
}

EFI_STATUS
ConnectRootBridge (
  VOID
  )
/*++

Routine Description:

  Connect RootBridge

Arguments:

  None.
 
Returns:

  EFI_SUCCESS             - Connect RootBridge successfully.
  EFI_STATUS              - Connect RootBridge fail.

--*/
{
  EFI_STATUS                Status;
  EFI_HANDLE                RootHandle;

  //
  // Patch Pci Root Bridge Device Path
  //
  PatchPciRootBridgeDevicePath (0, 0, &gPlatformRootBridge0);

  //
  // Make all the PCI_IO protocols on PCI Seg 0 show up
  //
  BdsLibConnectDevicePath (gPlatformRootBridges[0]);

  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid, 
                  &gPlatformRootBridges[0], 
                  &RootHandle
                  );
  DEBUG ((EFI_D_INFO, "Pci Root bridge handle is 0x%X\n", RootHandle));
  
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->ConnectController (RootHandle, NULL, NULL, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PrepareLpcBridgeDevicePath (
  IN EFI_HANDLE                DeviceHandle
  )
/*++

Routine Description:

  Add IsaKeyboard to ConIn,
  add IsaSerial to ConOut, ConIn, ErrOut.
  LPC Bridge: 06 01 00

Arguments:

  DeviceHandle            - Handle of PCIIO protocol.
 
Returns:

  EFI_SUCCESS             - LPC bridge is added to ConOut, ConIn, and ErrOut.
  EFI_STATUS              - No LPC bridge is added.

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;

  DevicePath = NULL;
  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID*)&DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  TempDevicePath = DevicePath;

  //
  // Register Keyboard
  //
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gPnpPs2KeyboardDeviceNode);

  BdsLibUpdateConsoleVariable (VarConsoleInp, DevicePath, NULL);

  //
  // Register COM1
  //
  DevicePath = TempDevicePath;
  gPnp16550ComPortDeviceNode.UID = 0;

  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gPnp16550ComPortDeviceNode);
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gUartDeviceNode);
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gTerminalTypeDeviceNode);

  BdsLibUpdateConsoleVariable (VarConsoleOut, DevicePath, NULL);
  BdsLibUpdateConsoleVariable (VarConsoleInp, DevicePath, NULL);
  BdsLibUpdateConsoleVariable (VarErrorOut, DevicePath, NULL);

  //
  // Register COM2
  //
  DevicePath = TempDevicePath;
  gPnp16550ComPortDeviceNode.UID = 1;

  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gPnp16550ComPortDeviceNode);
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gUartDeviceNode);
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gTerminalTypeDeviceNode);

  BdsLibUpdateConsoleVariable (VarConsoleOut, DevicePath, NULL);
  BdsLibUpdateConsoleVariable (VarConsoleInp, DevicePath, NULL);
  BdsLibUpdateConsoleVariable (VarErrorOut, DevicePath, NULL);

  return EFI_SUCCESS;
}

EFI_STATUS
GetGopDevicePath (
   IN  EFI_DEVICE_PATH_PROTOCOL *PciDevicePath,
   OUT EFI_DEVICE_PATH_PROTOCOL **GopDevicePath
   )
{
  UINTN                           Index;
  EFI_STATUS                      Status;
  EFI_HANDLE                      PciDeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL        *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *TempPciDevicePath;
  UINTN                           GopHandleCount;
  EFI_HANDLE                      *GopHandleBuffer;

  if (PciDevicePath == NULL || GopDevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Initialize the GopDevicePath to be PciDevicePath
  //
  *GopDevicePath    = PciDevicePath;
  TempPciDevicePath = PciDevicePath;

  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &TempPciDevicePath,
                  &PciDeviceHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Try to connect this handle, so that GOP dirver could start on this 
  // device and create child handles with GraphicsOutput Protocol installed
  // on them, then we get device paths of these child handles and select 
  // them as possible console device.
  //
  gBS->ConnectController (PciDeviceHandle, NULL, NULL, FALSE);

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &GopHandleCount,
                  &GopHandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Add all the child handles as possible Console Device
    //
    for (Index = 0; Index < GopHandleCount; Index++) {
      Status = gBS->HandleProtocol (GopHandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID*)&TempDevicePath);
      if (EFI_ERROR (Status)) {
        continue;
      }
      if (CompareMem (
            PciDevicePath,
            TempDevicePath,
            GetDevicePathSize (PciDevicePath) - END_DEVICE_PATH_LENGTH
            ) == 0) {
        //
        // In current implementation, we only enable one of the child handles
        // as console device, i.e. sotre one of the child handle's device
        // path to variable "ConOut"
        // In futhure, we could select all child handles to be console device
        //       

        *GopDevicePath = TempDevicePath;

        //
        // Delete the PCI device's path that added by GetPlugInPciVgaDevicePath()
        // Add the integrity GOP device path.
        //
        BdsLibUpdateConsoleVariable (VarConsoleOutDev, NULL, PciDevicePath);
        BdsLibUpdateConsoleVariable (VarConsoleOutDev, TempDevicePath, NULL);
      }
    }
    gBS->FreePool (GopHandleBuffer);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PreparePciVgaDevicePath (
  IN EFI_HANDLE                DeviceHandle
  )
/*++

Routine Description:

  Add PCI VGA to ConOut.
  PCI VGA: 03 00 00

Arguments:

  DeviceHandle            - Handle of PCIIO protocol.
 
Returns:

  EFI_SUCCESS             - PCI VGA is added to ConOut.
  EFI_STATUS              - No PCI VGA device is added.

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *GopDevicePath;

  DevicePath    = NULL;
  GopDevicePath = NULL;
  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID*)&DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  GetGopDevicePath (DevicePath, &GopDevicePath);
  DevicePath = GopDevicePath;

  BdsLibUpdateConsoleVariable (VarConsoleOut, DevicePath, NULL);
  
  return EFI_SUCCESS;
}

EFI_STATUS
PreparePciSerialDevicePath (
  IN EFI_HANDLE                DeviceHandle
  )
/*++

Routine Description:

  Add PCI Serial to ConOut, ConIn, ErrOut.
  PCI Serial: 07 00 02

Arguments:

  DeviceHandle            - Handle of PCIIO protocol.
 
Returns:

  EFI_SUCCESS             - PCI Serial is added to ConOut, ConIn, and ErrOut.
  EFI_STATUS              - No PCI Serial device is added.

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  
  DevicePath = NULL;
  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID*)&DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gUartDeviceNode);
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gTerminalTypeDeviceNode);

  BdsLibUpdateConsoleVariable (VarConsoleOut, DevicePath, NULL);
  BdsLibUpdateConsoleVariable (VarConsoleInp, DevicePath, NULL);
  BdsLibUpdateConsoleVariable (VarErrorOut, DevicePath, NULL);
  
  return EFI_SUCCESS;
}

EFI_STATUS
DetectAndPreparePlatformPciDevicePath (
  BOOLEAN DetectVgaOnly
  )
/*++

Routine Description:

  Do platform specific PCI Device check and add them to ConOut, ConIn, ErrOut

Arguments:

  DetectVgaOnly           - Only detect VGA device if it's TRUE.
 
Returns:

  EFI_SUCCESS             - PCI Device check and Console variable update successfully.
  EFI_STATUS              - PCI Device check or Console variable update fail.

--*/
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;

  //
  // Start to check all the PciIo to find all possible device
  //
  HandleCount = 0;
  HandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID*)&PciIo);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Check for all PCI device
    //
    Status = PciIo->Pci.Read (
                      PciIo,
                      EfiPciIoWidthUint32,
                      0,
                      sizeof (Pci) / sizeof (UINT32),
                      &Pci
                      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (!DetectVgaOnly) {
      //
      // Here we decide whether it is LPC Bridge
      //
      if ((IS_PCI_LPC (&Pci)) ||
          ((IS_PCI_ISA_PDECODE (&Pci)) && (Pci.Hdr.VendorId == 0x8086) && (Pci.Hdr.DeviceId == 0x7110))) {
        //
        // Add IsaKeyboard to ConIn,
        // add IsaSerial to ConOut, ConIn, ErrOut
        //
        DEBUG ((EFI_D_INFO, "Find the LPC Bridge device\n"));
        PrepareLpcBridgeDevicePath (HandleBuffer[Index]);
        continue;
      }
      //
      // Here we decide which Serial device to enable in PCI bus 
      //
      if (IS_PCI_16550SERIAL (&Pci)) {
        //
        // Add them to ConOut, ConIn, ErrOut.
        //
        DEBUG ((EFI_D_INFO, "Find the 16550 SERIAL device\n"));
        PreparePciSerialDevicePath (HandleBuffer[Index]);
        continue;
      }
    }

    //
    // Here we decide which VGA device to enable in PCI bus 
    //
    if (IS_PCI_VGA (&Pci)) {
      //
      // Add them to ConOut.
      //
      DEBUG ((EFI_D_INFO, "Find the VGA device\n"));
      PreparePciVgaDevicePath (HandleBuffer[Index]);
      continue;
    }
  }
  
  gBS->FreePool (HandleBuffer);
  
  return EFI_SUCCESS;
}

EFI_STATUS
PlatformBdsConnectConsole (
  IN BDS_CONSOLE_CONNECT_ENTRY   *PlatformConsole
  )
/*++

Routine Description:

  Connect the predefined platform default console device. Always try to find
  and enable the vga device if have.

Arguments:

  PlatformConsole         - Predfined platform default console device array.
 
Returns:

  EFI_SUCCESS             - Success connect at least one ConIn and ConOut 
                            device, there must have one ConOut device is 
                            active vga device.
  
  EFI_STATUS              - Return the status of 
                            BdsLibConnectAllDefaultConsoles ()

--*/
{
  EFI_STATUS                         Status;
  UINTN                              Index;
  EFI_DEVICE_PATH_PROTOCOL           *VarConout;
  EFI_DEVICE_PATH_PROTOCOL           *VarConin;
  UINTN                              DevicePathSize;

  //
  // Connect RootBridge
  //
  ConnectRootBridge ();

  VarConout = BdsLibGetVariableAndSize (
                VarConsoleOut,
                &gEfiGlobalVariableGuid,
                &DevicePathSize
                );
  VarConin = BdsLibGetVariableAndSize (
               VarConsoleInp,
               &gEfiGlobalVariableGuid,
               &DevicePathSize
               );
  
  if (VarConout == NULL || VarConin == NULL) {
    //
    // Do platform specific PCI Device check and add them to ConOut, ConIn, ErrOut
    //
    DetectAndPreparePlatformPciDevicePath (FALSE);

    //
    // Have chance to connect the platform default console,
    // the platform default console is the minimue device group
    // the platform should support
    //
    for (Index = 0; PlatformConsole[Index].DevicePath != NULL; ++Index) {
      //
      // Update the console variable with the connect type
      //
      if ((PlatformConsole[Index].ConnectType & CONSOLE_IN) == CONSOLE_IN) {
        BdsLibUpdateConsoleVariable (VarConsoleInp, PlatformConsole[Index].DevicePath, NULL);
      }
      if ((PlatformConsole[Index].ConnectType & CONSOLE_OUT) == CONSOLE_OUT) {
        BdsLibUpdateConsoleVariable (VarConsoleOut, PlatformConsole[Index].DevicePath, NULL);
      }
      if ((PlatformConsole[Index].ConnectType & STD_ERROR) == STD_ERROR) {
        BdsLibUpdateConsoleVariable (VarErrorOut, PlatformConsole[Index].DevicePath, NULL);
      }
    }
  } else {
    //
    // Only detect VGA device and add them to ConOut
    //
    DetectAndPreparePlatformPciDevicePath (TRUE);
  }
  
  //
  // The ConIn devices connection will start the USB bus, should disable all
  // Usb legacy support firstly.
  // Caution: Must ensure the PCI bus driver has been started. Since the 
  // ConnectRootBridge() will create all the PciIo protocol, it's safe here now
  //
  Status = DisableUsbLegacySupport();
  
  //
  // Connect the all the default console with current cosole variable
  //
  Status = BdsLibConnectAllDefaultConsoles ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

VOID
PlatformBdsConnectSequence (
  VOID
  )
/*++

Routine Description:

  Connect with predeined platform connect sequence, 
  the OEM/IBV can customize with their own connect sequence.
  
Arguments:

  None.
 
Returns:

  None.
  
--*/
{
  UINTN Index;

  Index = 0;

  //
  // Here we can get the customized platform connect sequence
  // Notes: we can connect with new variable which record the
  // last time boots connect device path sequence
  //
  while (gPlatformConnectSequence[Index] != NULL) {
    //
    // Build the platform boot option
    //
    BdsLibConnectDevicePath (gPlatformConnectSequence[Index]);
    Index++;
  }

}

VOID
PlatformBdsGetDriverOption (
  IN OUT LIST_ENTRY              *BdsDriverLists
  )
/*++

Routine Description:

  Load the predefined driver option, OEM/IBV can customize this
  to load their own drivers
  
Arguments:

  BdsDriverLists  - The header of the driver option link list.
 
Returns:

  None.
  
--*/
{
  UINTN Index;

  Index = 0;

  //
  // Here we can get the customized platform driver option
  //
  while (gPlatformDriverOption[Index] != NULL) {
    //
    // Build the platform boot option
    //
    BdsLibRegisterNewOption (BdsDriverLists, gPlatformDriverOption[Index], NULL, L"DriverOrder");
    Index++;
  }

}

VOID
PlatformBdsDiagnostics (
  IN EXTENDMEM_COVERAGE_LEVEL    MemoryTestLevel,
  IN BOOLEAN                     QuietBoot,
  IN BASEM_MEMORY_TEST           BaseMemoryTest
  )
/*++

Routine Description:

  Perform the platform diagnostic, such like test memory. OEM/IBV also
  can customize this fuction to support specific platform diagnostic.
  
Arguments:

  MemoryTestLevel  - The memory test intensive level
  
  QuietBoot        - Indicate if need to enable the quiet boot

  BaseMemoryTest   - A pointer to BdsMemoryTest()
 
Returns:

  None.
  
--*/
{
  EFI_STATUS  Status;

  //
  // Here we can decide if we need to show
  // the diagnostics screen
  // Notes: this quiet boot code should be remove
  // from the graphic lib
  //
  if (QuietBoot) {
    Status = EnableQuietBoot (PcdGetPtr(PcdLogoFile));
    if (EFI_ERROR (Status)) {
      DisableQuietBoot ();
      return;
    }

    //
    // Perform system diagnostic
    //
    Status = BaseMemoryTest (MemoryTestLevel);
    if (EFI_ERROR (Status)) {
      DisableQuietBoot ();
    }

    return ;
  }
  //
  // Perform system diagnostic
  //
  Status = BaseMemoryTest (MemoryTestLevel);
}

VOID
EFIAPI
PlatformBdsPolicyBehavior (
  IN OUT LIST_ENTRY              *DriverOptionList,
  IN OUT LIST_ENTRY              *BootOptionList,
  IN PROCESS_CAPSULES            ProcessCapsules,
  IN BASEM_MEMORY_TEST           BaseMemoryTest
  )
/*++

Routine Description:

  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.
  
Arguments:

  DriverOptionList - The header of the driver option link list
  
  BootOptionList   - The header of the boot option link list
 
Returns:

  None.
  
--*/
{
  EFI_STATUS                         Status;
  UINT16                             Timeout;
  EFI_EVENT                          UserInputDurationTime;
  UINTN                              Index;
  EFI_INPUT_KEY                      Key;
  EFI_BOOT_MODE                      BootMode;

  //
  // Init the time out value
  //
  Timeout = PcdGet16 (PcdPlatformBootTimeOut);

  //
  // Load the driver option as the driver option list
  //
  PlatformBdsGetDriverOption (DriverOptionList);

  //
  // Get current Boot Mode
  //
  Status = BdsLibGetBootMode (&BootMode);
  DEBUG ((EFI_D_ERROR, "Boot Mode:%x\n", BootMode));

  //
  // Go the different platform policy with different boot mode
  // Notes: this part code can be change with the table policy
  //
  ASSERT (BootMode == BOOT_WITH_FULL_CONFIGURATION);
  //
  // Connect platform console
  //
  Status = PlatformBdsConnectConsole (gPlatformConsole);
  if (EFI_ERROR (Status)) {
    //
    // Here OEM/IBV can customize with defined action
    //
    PlatformBdsNoConsoleAction ();
  }
  //
  // Create a 300ms duration event to ensure user has enough input time to enter Setup
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  0,
                  NULL,
                  NULL,
                  &UserInputDurationTime
                  );
  ASSERT (Status == EFI_SUCCESS);
  Status = gBS->SetTimer (UserInputDurationTime, TimerRelative, 3000000);
  ASSERT (Status == EFI_SUCCESS);
  //
  // Memory test and Logo show
  //
  PlatformBdsDiagnostics (IGNORE, TRUE, BaseMemoryTest);

  //
  // Perform some platform specific connect sequence
  //
  PlatformBdsConnectSequence ();

  //
  // Give one chance to enter the setup if we
  // have the time out
  //
  // BUGBUG: hard code timeout to 5 second to show logo in graphic mode.
  Timeout = 5;  
  if (Timeout != 0) {
    PlatformBdsEnterFrontPage (Timeout, FALSE);
  }

  //
  //BdsLibConnectAll ();
  //BdsLibEnumerateAllBootOption (BootOptionList);  
  
  //
  // Please uncomment above ConnectAll and EnumerateAll code and remove following first boot
  // checking code in real production tip.
  //          
  // In BOOT_WITH_FULL_CONFIGURATION boot mode, should always connect every device 
  // and do enumerate all the default boot options. But in development system board, the boot mode 
  // cannot be BOOT_ASSUMING_NO_CONFIGURATION_CHANGES because the machine box
  // is always open. So the following code only do the ConnectAll and EnumerateAll at first boot.
  //
  Status = BdsLibBuildOptionFromVar (BootOptionList, L"BootOrder");
  if (EFI_ERROR(Status)) {
    //
    // If cannot find "BootOrder" variable,  it may be first boot. 
    // Try to connect all devices and enumerate all boot options here.
    //
    BdsLibConnectAll ();
    BdsLibEnumerateAllBootOption (BootOptionList);
  } 

  //
  // To give the User a chance to enter Setup here, if user set TimeOut is 0.
  // BDS should still give user a chance to enter Setup
  // Check whether the user input after the duration time has expired 
  //
  gBS->WaitForEvent (1, &UserInputDurationTime, &Index);
  gBS->CloseEvent (UserInputDurationTime);
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  
  if (!EFI_ERROR (Status)) {
    //
    // Enter Setup if user input 
    //
    Timeout = 0xffff;
    PlatformBdsEnterFrontPage (Timeout, FALSE);
  }
  
  return ;

}

VOID
EFIAPI
PlatformBdsBootSuccess (
  IN  BDS_COMMON_OPTION *Option
  )
/*++

Routine Description:
  
  Hook point after a boot attempt succeeds. We don't expect a boot option to
  return, so the EFI 1.0 specification defines that you will default to an
  interactive mode and stop processing the BootOrder list in this case. This
  is alos a platform implementation and can be customized by IBV/OEM.

Arguments:

  Option - Pointer to Boot Option that succeeded to boot.

Returns:
  
  None.

--*/
{
  CHAR16  *TmpStr;

  //
  // If Boot returned with EFI_SUCCESS and there is not in the boot device
  // select loop then we need to pop up a UI and wait for user input.
  //
  TmpStr = Option->StatusString;
  if (TmpStr != NULL) {
    BdsLibOutputStrings (gST->ConOut, TmpStr, Option->Description, L"\n\r", NULL);
    gBS->FreePool (TmpStr);
  }
}

VOID
EFIAPI
PlatformBdsBootFail (
  IN  BDS_COMMON_OPTION  *Option,
  IN  EFI_STATUS         Status,
  IN  CHAR16             *ExitData,
  IN  UINTN              ExitDataSize
  )
/*++

Routine Description:
  
  Hook point after a boot attempt fails.

Arguments:
  
  Option - Pointer to Boot Option that failed to boot.

  Status - Status returned from failed boot.

  ExitData - Exit data returned from failed boot.

  ExitDataSize - Exit data size returned from failed boot.

Returns:
  
  None.

--*/
{
  CHAR16  *TmpStr;

  //
  // If Boot returned with failed status then we need to pop up a UI and wait
  // for user input.
  //
  TmpStr = Option->StatusString;
  if (TmpStr != NULL) {
    BdsLibOutputStrings (gST->ConOut, TmpStr, Option->Description, L"\n\r", NULL);
    gBS->FreePool (TmpStr);
  }

}

EFI_STATUS
PlatformBdsNoConsoleAction (
  VOID
  )
/*++

Routine Description:
  
  This function is remained for IBV/OEM to do some platform action,
  if there no console device can be connected.

Arguments:
  
  None.
  
Returns:
  
  EFI_SUCCESS      - Direct return success now.

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
ConvertSystemTable (
  IN     EFI_GUID        *TableGuid,
  IN OUT VOID            **Table
  )
/*++

Routine Description:
  Convert ACPI Table /Smbios Table /MP Table if its location is lower than Address:0x100000
  Assumption here:
   As in legacy Bios, ACPI/Smbios/MP table is required to place in E/F Seg, 
   So here we just check if the range is E/F seg, 
   and if Not, assume the Memory type is EfiACPIReclaimMemory/EfiACPIMemoryNVS

Arguments:
  TableGuid - Guid of the table
  Table     - pointer to the table  

Returns:
  EFI_SUCEESS - Convert Table successfully
  Other       - Failed

--*/
{
  EFI_STATUS      Status;
  VOID            *AcpiHeader;
  UINTN           AcpiTableLen;
  
  //
  // If match acpi guid (1.0, 2.0, or later), Convert ACPI table according to version. 
  //
  AcpiHeader = (VOID*)(UINTN)(*(UINT64 *)(*Table));
  
  if (CompareGuid(TableGuid, &gEfiAcpiTableGuid) || CompareGuid(TableGuid, &gEfiAcpi20TableGuid)){
    if (((EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)AcpiHeader)->Reserved == 0x00){
      //
      // If Acpi 1.0 Table, then RSDP structure doesn't contain Length field, use structure size
      //
      AcpiTableLen = sizeof (EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER);
    } else if (((EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)AcpiHeader)->Reserved >= 0x02){
      //
      // If Acpi 2.0 or later, use RSDP Length fied.
      //
      AcpiTableLen = ((EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)AcpiHeader)->Length;
    } else {
      //
      // Invalid Acpi Version, return
      //
      return EFI_UNSUPPORTED;
    }
    Status = ConvertAcpiTable (AcpiTableLen, Table);
    return Status; 
  }
  
  //
  // If matches smbios guid, convert Smbios table.
  //
  if (CompareGuid(TableGuid, &gEfiSmbiosTableGuid)){
    Status = ConvertSmbiosTable (Table);
    return Status;
  }
  
  //
  // If the table is MP table?
  //
  if (CompareGuid(TableGuid, &gEfiMpsTableGuid)){
    Status = ConvertMpsTable (Table);
    return Status;
  }
  
  return EFI_UNSUPPORTED;
}  


EFI_STATUS
ConvertAcpiTable (
  IN     UINTN                       TableLen,
  IN OUT VOID                        **Table
  )
/*++

Routine Description:
  Convert RSDP of ACPI Table if its location is lower than Address:0x100000
  Assumption here:
   As in legacy Bios, ACPI table is required to place in E/F Seg, 
   So here we just check if the range is E/F seg, 
   and if Not, assume the Memory type is EfiACPIReclaimMemory/EfiACPIMemoryNVS

Arguments:
  TableLen  - Acpi RSDP length
  Table     - pointer to the table  

Returns:
  EFI_SUCEESS - Convert Table successfully
  Other       - Failed

--*/
{
  VOID                  *AcpiTableOri;
  VOID                  *AcpiTableNew;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  BufferPtr;

  
  AcpiTableOri    =  (VOID *)(UINTN)(*(UINT64*)(*Table));
  if (((UINTN)AcpiTableOri < 0x100000) && ((UINTN)AcpiTableOri > 0xE0000)) {
    BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiACPIMemoryNVS,
                    EFI_SIZE_TO_PAGES(TableLen),
                    &BufferPtr
                    );
    ASSERT_EFI_ERROR (Status);
    AcpiTableNew = (VOID *)(UINTN)BufferPtr;
    CopyMem (AcpiTableNew, AcpiTableOri, TableLen);
  } else {
    AcpiTableNew = AcpiTableOri;
  }
  //
  // Change configuration table Pointer
  //
  *Table = AcpiTableNew;
  
  return EFI_SUCCESS;
}

EFI_STATUS
ConvertSmbiosTable (
  IN OUT VOID        **Table
  )
/*++

Routine Description:

  Convert Smbios Table if the Location of the SMBios Table is lower than Addres 0x100000
  Assumption here:
   As in legacy Bios, Smbios table is required to place in E/F Seg, 
   So here we just check if the range is F seg, 
   and if Not, assume the Memory type is EfiACPIMemoryNVS/EfiRuntimeServicesData
Arguments:
  Table     - pointer to the table

Returns:
  EFI_SUCEESS - Convert Table successfully
  Other       - Failed

--*/
{
  SMBIOS_TABLE_ENTRY_POINT *SmbiosTableNew;
  SMBIOS_TABLE_ENTRY_POINT *SmbiosTableOri;
  EFI_STATUS               Status;
  UINT32                   SmbiosEntryLen;
  UINT32                   BufferLen;
  EFI_PHYSICAL_ADDRESS     BufferPtr;
  
  SmbiosTableNew  = NULL;
  SmbiosTableOri  = NULL;
  
  //
  // Get Smibos configuration Table 
  //
  SmbiosTableOri =  (SMBIOS_TABLE_ENTRY_POINT *)(UINTN)(*(UINT64*)(*Table));
  
  if ((SmbiosTableOri == NULL) ||
      ((UINTN)SmbiosTableOri > 0x100000) ||
      ((UINTN)SmbiosTableOri < 0xF0000)){
    return EFI_SUCCESS;
  }
  //
  // Relocate the Smibos memory
  //
  BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
  if (SmbiosTableOri->SmbiosBcdRevision != 0x21) {
    SmbiosEntryLen  = SmbiosTableOri->EntryPointLength;
  } else {
    //
    // According to Smbios Spec 2.4, we should set entry point length as 0x1F if version is 2.1
    //
    SmbiosEntryLen = 0x1F;
  }
  BufferLen = SmbiosEntryLen + SYS_TABLE_PAD(SmbiosEntryLen) + SmbiosTableOri->TableLength;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(BufferLen),
                  &BufferPtr
                  );
  ASSERT_EFI_ERROR (Status);
  SmbiosTableNew = (SMBIOS_TABLE_ENTRY_POINT *)(UINTN)BufferPtr;
  CopyMem (
    SmbiosTableNew, 
    SmbiosTableOri,
    SmbiosEntryLen
    );
  // 
  // Get Smbios Structure table address, and make sure the start address is 32-bit align
  //
  BufferPtr += SmbiosEntryLen + SYS_TABLE_PAD(SmbiosEntryLen);
  CopyMem (
    (VOID *)(UINTN)BufferPtr, 
    (VOID *)(UINTN)(SmbiosTableOri->TableAddress),
    SmbiosTableOri->TableLength
    );
  SmbiosTableNew->TableAddress = (UINT32)BufferPtr;
  SmbiosTableNew->IntermediateChecksum = 0;
  SmbiosTableNew->IntermediateChecksum = 
          CalculateCheckSum8 ((UINT8*)SmbiosTableNew + 0x10, SmbiosEntryLen -0x10);
  //
  // Change the SMBIOS pointer
  //
  *Table = SmbiosTableNew;
  
  return EFI_SUCCESS;  
} 

EFI_STATUS
ConvertMpsTable (
  IN OUT VOID          **Table
  )
/*++

Routine Description:

  Convert MP Table if the Location of the SMBios Table is lower than Addres 0x100000
  Assumption here:
   As in legacy Bios, MP table is required to place in E/F Seg, 
   So here we just check if the range is E/F seg, 
   and if Not, assume the Memory type is EfiACPIMemoryNVS/EfiRuntimeServicesData
Arguments:
  Table     - pointer to the table

Returns:
  EFI_SUCEESS - Convert Table successfully
  Other       - Failed

--*/
{
  UINT32                                       Data32;
  UINT32                                       FPLength;
  EFI_LEGACY_MP_TABLE_FLOATING_POINTER         *MpsFloatingPointerOri;
  EFI_LEGACY_MP_TABLE_FLOATING_POINTER         *MpsFloatingPointerNew;
  EFI_LEGACY_MP_TABLE_HEADER                   *MpsTableOri;
  EFI_LEGACY_MP_TABLE_HEADER                   *MpsTableNew;
  VOID                                         *OemTableOri;
  VOID                                         *OemTableNew;
  EFI_STATUS                                   Status;
  EFI_PHYSICAL_ADDRESS                         BufferPtr;
  
  //
  // Get MP configuration Table 
  //
  MpsFloatingPointerOri = (EFI_LEGACY_MP_TABLE_FLOATING_POINTER *)(UINTN)(*(UINT64*)(*Table));
  if (!(((UINTN)MpsFloatingPointerOri <= 0x100000) && 
        ((UINTN)MpsFloatingPointerOri >= 0xF0000))){
    return EFI_SUCCESS;
  }
  //
  // Get Floating pointer structure length
  //
  FPLength = MpsFloatingPointerOri->Length * 16;
  Data32   = FPLength + SYS_TABLE_PAD (FPLength);
  MpsTableOri = (EFI_LEGACY_MP_TABLE_HEADER *)(UINTN)(MpsFloatingPointerOri->PhysicalAddress);
  if (MpsTableOri != NULL) {
    Data32 += MpsTableOri->BaseTableLength;
    Data32 += MpsTableOri->ExtendedTableLength;
    if (MpsTableOri->OemTablePointer != 0x00) {
      Data32 += SYS_TABLE_PAD (Data32);
      Data32 += MpsTableOri->OemTableSize;
    }
  } else {
    return EFI_SUCCESS;
  }
  //
  // Relocate memory
  //
  BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(Data32),
                  &BufferPtr
                  );
  ASSERT_EFI_ERROR (Status); 
  MpsFloatingPointerNew = (EFI_LEGACY_MP_TABLE_FLOATING_POINTER *)(UINTN)BufferPtr;
  CopyMem (MpsFloatingPointerNew, MpsFloatingPointerOri, FPLength);
  //
  // If Mp Table exists
  //
  if (MpsTableOri != NULL) {
    //
    // Get Mps table length, including Ext table
    //
    BufferPtr = BufferPtr + FPLength + SYS_TABLE_PAD (FPLength);
    MpsTableNew = (EFI_LEGACY_MP_TABLE_HEADER *)(UINTN)BufferPtr;
    CopyMem (MpsTableNew, MpsTableOri, MpsTableOri->BaseTableLength + MpsTableOri->ExtendedTableLength);
    
    if ((MpsTableOri->OemTableSize != 0x0000) && (MpsTableOri->OemTablePointer != 0x0000)){
        BufferPtr += MpsTableOri->BaseTableLength + MpsTableOri->ExtendedTableLength;
        BufferPtr += SYS_TABLE_PAD (BufferPtr);
        OemTableNew = (VOID *)(UINTN)BufferPtr;
        OemTableOri = (VOID *)(UINTN)MpsTableOri->OemTablePointer;
        CopyMem (OemTableNew, OemTableOri, MpsTableOri->OemTableSize);
        MpsTableNew->OemTablePointer = (UINT32)(UINTN)OemTableNew;
    }
    MpsTableNew->Checksum = 0;
    MpsTableNew->Checksum = CalculateCheckSum8 ((UINT8*)MpsTableNew, MpsTableOri->BaseTableLength);
    MpsFloatingPointerNew->PhysicalAddress = (UINT32)(UINTN)MpsTableNew;
    MpsFloatingPointerNew->Checksum = 0;
    MpsFloatingPointerNew->Checksum = CalculateCheckSum8 ((UINT8*)MpsFloatingPointerNew, FPLength);
  }
  //
  // Change the pointer
  //
  *Table = MpsFloatingPointerNew;
  
  return EFI_SUCCESS;  
} 
  
/**
  Lock the ConsoleIn device in system table. All key
  presses will be ignored until the Password is typed in. The only way to
  disable the password is to type it in to a ConIn device.

  @param  Password        Password used to lock ConIn device.

  @retval EFI_SUCCESS     lock the Console In Spliter virtual handle successfully.
  @retval EFI_UNSUPPORTED Password not found

**/
EFI_STATUS
EFIAPI
LockKeyboards (
  IN  CHAR16    *Password
  )
{
    return EFI_UNSUPPORTED;
}

/**
  This function locks platform flash that is not allowed to be updated during normal boot path.
  The flash layout is platform specific.

  **/
VOID
EFIAPI
PlatformBdsLockNonUpdatableFlash (
  VOID
  )
{
  return;
}
