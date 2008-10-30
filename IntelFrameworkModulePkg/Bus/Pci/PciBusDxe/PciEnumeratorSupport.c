/**@file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "PciBus.h"
#include "PciEnumeratorSupport.h"
#include "PciCommand.h"
#include "PciIo.h"

/**
  This routine is used to check whether the pci device is present.
  
  @param PciRootBridgeIo   Pointer to instance of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
  @param Pci               Output buffer for PCI device structure
  @param Bus               PCI bus NO
  @param Device            PCI device NO
  @param Func              PCI Func NO
  
  @retval EFI_NOT_FOUND device not present
  @retval EFI_SUCCESS   device is found.
**/
EFI_STATUS
PciDevicePresent (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  PCI_TYPE00                          *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
{
  UINT64      Address;
  EFI_STATUS  Status;

  //
  // Create PCI address map in terms of Bus, Device and Func
  //
  Address = EFI_PCI_ADDRESS (Bus, Device, Func, 0);

  //
  // Read the Vendor Id register
  //
  Status = PciRootBridgeIoRead (
                                  PciRootBridgeIo,
                                  NULL,
                                  EfiPciWidthUint32,
                                  Address,
                                  1,
                                  Pci
                                  );

  if (!EFI_ERROR (Status) && (Pci->Hdr).VendorId != 0xffff) {

    //
    // Read the entire config header for the device
    //

    Status = PciRootBridgeIoRead (
                                    PciRootBridgeIo,
                                    NULL,
                                    EfiPciWidthUint32,
                                    Address,
                                    sizeof (PCI_TYPE00) / sizeof (UINT32),
                                    Pci
                                    );

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Collect all the resource information under this root bridge
  A database that records all the information about pci device subject to this
  root bridge will then be created.
    
  @param Bridge         Parent bridge instance
  @param StartBusNumer  Bus number of begining 
**/
EFI_STATUS
PciPciDeviceInfoCollector (
  IN PCI_IO_DEVICE                      *Bridge,
  UINT8                                 StartBusNumber
  )
{
  EFI_STATUS          Status;
  PCI_TYPE00          Pci;
  UINT8               Device;
  UINT8               Func;
  UINT8               SecBus;
  PCI_IO_DEVICE       *PciIoDevice;
  EFI_PCI_IO_PROTOCOL *PciIo;

  Status  = EFI_SUCCESS;
  SecBus  = 0;

  for (Device = 0; Device <= PCI_MAX_DEVICE; Device++) {

    for (Func = 0; Func <= PCI_MAX_FUNC; Func++) {

      //
      // Check to see whether PCI device is present
      //

      Status = PciDevicePresent (
                Bridge->PciRootBridgeIo,
                &Pci,
                (UINT8) StartBusNumber,
                (UINT8) Device,
                (UINT8) Func
                );

      if (!EFI_ERROR (Status)) {

        //
        // Call back to host bridge function
        //
        PreprocessController (Bridge, (UINT8) StartBusNumber, Device, Func, EfiPciBeforeResourceCollection);

        //
        // Collect all the information about the PCI device discovered
        //
        Status = PciSearchDevice (
                  Bridge,
                  &Pci,
                  (UINT8) StartBusNumber,
                  Device,
                  Func,
                  &PciIoDevice
                  );

        //
        // Recursively scan PCI busses on the other side of PCI-PCI bridges
        //
        //

        if (!EFI_ERROR (Status) && (IS_PCI_BRIDGE (&Pci) || IS_CARDBUS_BRIDGE (&Pci))) {

          //
          // If it is PPB, we need to get the secondary bus to continue the enumeration
          //
          PciIo   = &(PciIoDevice->PciIo);

          Status  = PciIoRead (PciIo, EfiPciIoWidthUint8, 0x19, 1, &SecBus);

          if (EFI_ERROR (Status)) {
            return Status;
          }

          //
          // Get resource padding for PPB
          //
          GetResourcePaddingPpb (PciIoDevice);

          //
          // Deep enumerate the next level bus
          //
          Status = PciPciDeviceInfoCollector (
                    PciIoDevice,
                    (UINT8) (SecBus)
                    );

        }

        if (Func == 0 && !IS_PCI_MULTI_FUNC (&Pci)) {

          //
          // Skip sub functions, this is not a multi function device
          //
          Func = PCI_MAX_FUNC;
        }
      }

    }
  }

  return EFI_SUCCESS;
}

/**
  Seach required device and get PCI device info block
  
  @param Bridge     Parent bridge instance
  @param Pci        Output of PCI device info block
  @param Bus        PCI bus NO.
  @param Device     PCI device NO.
  @param Func       PCI func  NO.
  @param PciDevice  output of searched PCI device instance
**/
EFI_STATUS
PciSearchDevice (
  IN  PCI_IO_DEVICE                         *Bridge,
  IN  PCI_TYPE00                            *Pci,
  IN  UINT8                                 Bus,
  IN  UINT8                                 Device,
  IN  UINT8                                 Func,
  OUT PCI_IO_DEVICE                         **PciDevice
  )
{
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = NULL;

  if (!IS_PCI_BRIDGE (Pci)) {

    if (IS_CARDBUS_BRIDGE (Pci)) {
      PciIoDevice = GatherP2CInfo (
                      Bridge,
                      Pci,
                      Bus,
                      Device,
                      Func
                      );
      if ((PciIoDevice != NULL) && gFullEnumeration) {
        InitializeP2C (PciIoDevice);
      }
    } else {

      //
      // Create private data for Pci Device
      //
      PciIoDevice = GatherDeviceInfo (
                      Bridge,
                      Pci,
                      Bus,
                      Device,
                      Func
                      );

    }

  } else {

    //
    // Create private data for PPB
    //
    PciIoDevice = GatherPpbInfo (
                    Bridge,
                    Pci,
                    Bus,
                    Device,
                    Func
                    );

    //
    // Special initialization for PPB including making the PPB quiet
    //
    if ((PciIoDevice != NULL) && gFullEnumeration) {
      InitializePpb (PciIoDevice);
    }
  }

  if (!PciIoDevice) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Update the bar information for this PCI device so as to support some specific device
  //
  UpdatePciInfo (PciIoDevice);

  if (PciIoDevice->DevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Detect this function has option rom
  //
  if (gFullEnumeration) {

    if (!IS_CARDBUS_BRIDGE (Pci)) {

      GetOpRomInfo (PciIoDevice);

    }

    ResetPowerManagementFeature (PciIoDevice);

  }

  //
  // Insert it into a global tree for future reference
  //
  InsertPciDevice (Bridge, PciIoDevice);

  //
  // Determine PCI device attributes
  //

  if (PciDevice != NULL) {
    *PciDevice = PciIoDevice;
  }

  return EFI_SUCCESS;
}

/**
  Create PCI private data for PCI device
  
  @param Bridge Parent bridge instance
  @param Pci    PCI bar block
  @param Bus    PCI device Bus NO.
  @param Device PCI device DeviceNO.
  @param Func   PCI device's func NO.
  
  @return new PCI device's private date structure.
**/
PCI_IO_DEVICE *
GatherDeviceInfo (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
{
  UINTN                           Offset;
  UINTN                           BarIndex;
  PCI_IO_DEVICE                   *PciIoDevice;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;

  PciRootBridgeIo = Bridge->PciRootBridgeIo;
  PciIoDevice = CreatePciIoDevice (
                  PciRootBridgeIo,
                  Pci,
                  Bus,
                  Device,
                  Func
                  );

  if (!PciIoDevice) {
    return NULL;
  }

  //
  // Create a device path for this PCI device and store it into its private data
  //
  CreatePciDevicePath (
    Bridge->DevicePath,
    PciIoDevice
    );

  //
  // If it is a full enumeration, disconnect the device in advance
  //
  if (gFullEnumeration) {

    PciDisableCommandRegister (PciIoDevice, EFI_PCI_COMMAND_BITS_OWNED);

  }

  //
  // Start to parse the bars
  //
  for (Offset = 0x10, BarIndex = 0; Offset <= 0x24; BarIndex++) {
    Offset = PciParseBar (PciIoDevice, Offset, BarIndex);
  }

  return PciIoDevice;
}

/**
  Create private data for bridge device's PPB.
  
  @param Bridge     Parent bridge 
  @param Pci        Pci device block
  @param Bus        Bridge device's bus NO.
  @param Device     Bridge device's device NO.
  @param Func       Bridge device's func NO.
  
  @return bridge device instance
**/
PCI_IO_DEVICE *
GatherPpbInfo (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
{
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  PCI_IO_DEVICE                   *PciIoDevice;
  EFI_STATUS                      Status;
  UINT32                          Value;
  EFI_PCI_IO_PROTOCOL             *PciIo;
  UINT8                           Temp;

  PciRootBridgeIo = Bridge->PciRootBridgeIo;
  PciIoDevice = CreatePciIoDevice (
                  PciRootBridgeIo,
                  Pci,
                  Bus,
                  Device,
                  Func
                  );

  if (!PciIoDevice) {
    return NULL;
  }

  //
  // Create a device path for this PCI device and store it into its private data
  //
  CreatePciDevicePath (
    Bridge->DevicePath,
    PciIoDevice
    );

  if (gFullEnumeration) {
    PciDisableCommandRegister (PciIoDevice, EFI_PCI_COMMAND_BITS_OWNED);

    //
    // Initalize the bridge control register
    //
    PciDisableBridgeControlRegister (PciIoDevice, EFI_PCI_BRIDGE_CONTROL_BITS_OWNED);

  }

  //
  // PPB can have two BARs
  //
  if (PciParseBar (PciIoDevice, 0x10, PPB_BAR_0) == 0x14) {
    //
    // Not 64-bit bar
    //
    PciParseBar (PciIoDevice, 0x14, PPB_BAR_1);
  }

  PciIo = &PciIoDevice->PciIo;

  //
  // Test whether it support 32 decode or not
  //
  PciIoRead (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &Temp);
  PciIoWrite (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &gAllOne);
  PciIoRead (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &Value);
  PciIoWrite (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &Temp);

  if (Value) {
    if (Value & 0x01) {
      PciIoDevice->Decodes |= EFI_BRIDGE_IO32_DECODE_SUPPORTED;
    } else {
      PciIoDevice->Decodes |= EFI_BRIDGE_IO16_DECODE_SUPPORTED;
    }
  }

  Status = BarExisted (
            PciIoDevice,
            0x24,
            NULL,
            NULL
            );

  //
  // test if it supports 64 memory or not
  //
  if (!EFI_ERROR (Status)) {

    Status = BarExisted (
              PciIoDevice,
              0x28,
              NULL,
              NULL
              );

    if (!EFI_ERROR (Status)) {
      PciIoDevice->Decodes |= EFI_BRIDGE_PMEM32_DECODE_SUPPORTED;
      PciIoDevice->Decodes |= EFI_BRIDGE_PMEM64_DECODE_SUPPORTED;
    } else {
      PciIoDevice->Decodes |= EFI_BRIDGE_PMEM32_DECODE_SUPPORTED;
    }
  }

  //
  // Memory 32 code is required for ppb
  //
  PciIoDevice->Decodes |= EFI_BRIDGE_MEM32_DECODE_SUPPORTED;

  GetResourcePaddingPpb (PciIoDevice);

  return PciIoDevice;
}

/**
  Create private data for hotplug bridge device
  
  @param Bridge Parent bridge instance
  @param Pci    PCI bar block
  @param Bus    hotplug bridge device's bus NO.
  @param Device hotplug bridge device's device NO.
  @param Func   hotplug bridge device's Func NO.
  
  @return hotplug bridge device instance
**/
PCI_IO_DEVICE *
GatherP2CInfo (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
{
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  PCI_IO_DEVICE                   *PciIoDevice;

  PciRootBridgeIo = Bridge->PciRootBridgeIo;
  PciIoDevice = CreatePciIoDevice (
                  PciRootBridgeIo,
                  Pci,
                  Bus,
                  Device,
                  Func
                  );

  if (!PciIoDevice) {
    return NULL;
  }

  //
  // Create a device path for this PCI device and store it into its private data
  //
  CreatePciDevicePath (
    Bridge->DevicePath,
    PciIoDevice
    );

  if (gFullEnumeration) {
    PciDisableCommandRegister (PciIoDevice, EFI_PCI_COMMAND_BITS_OWNED);

    //
    // Initalize the bridge control register
    //
    PciDisableBridgeControlRegister (PciIoDevice, EFI_PCCARD_BRIDGE_CONTROL_BITS_OWNED);

  }
  //
  // P2C only has one bar that is in 0x10
  //
  PciParseBar (PciIoDevice, 0x10, P2C_BAR_0);

  //
  // Read PciBar information from the bar register
  //
  GetBackPcCardBar (PciIoDevice);
  PciIoDevice->Decodes = EFI_BRIDGE_MEM32_DECODE_SUPPORTED  |
                         EFI_BRIDGE_PMEM32_DECODE_SUPPORTED |
                         EFI_BRIDGE_IO32_DECODE_SUPPORTED;

  return PciIoDevice;
}

/**
  Create device path for pci deivce
  
  @param ParentDevicePath  Parent bridge's path
  @param PciIoDevice       Pci device instance
  
  @return device path protocol instance for specific pci device.
**/
EFI_DEVICE_PATH_PROTOCOL *
CreatePciDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL *ParentDevicePath,
  IN  PCI_IO_DEVICE            *PciIoDevice
  )
{

  PCI_DEVICE_PATH PciNode;

  //
  // Create PCI device path
  //
  PciNode.Header.Type     = HARDWARE_DEVICE_PATH;
  PciNode.Header.SubType  = HW_PCI_DP;
  SetDevicePathNodeLength (&PciNode.Header, sizeof (PciNode));

  PciNode.Device          = PciIoDevice->DeviceNumber;
  PciNode.Function        = PciIoDevice->FunctionNumber;
  PciIoDevice->DevicePath = AppendDevicePathNode (ParentDevicePath, &PciNode.Header);

  return PciIoDevice->DevicePath;
}

/**
  Check the bar is existed or not.

  @param PciIoDevice       - A pointer to the PCI_IO_DEVICE.
  @param Offset            - The offset.
  @param BarLengthValue    - The bar length value.
  @param OriginalBarValue  - The original bar value.

  @retval EFI_NOT_FOUND     - The bar don't exist.
  @retval EFI_SUCCESS       - The bar exist.

**/
EFI_STATUS
BarExisted (
  IN PCI_IO_DEVICE *PciIoDevice,
  IN UINTN         Offset,
  OUT UINT32       *BarLengthValue,
  OUT UINT32       *OriginalBarValue
  )

{
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT32              OriginalValue;
  UINT32              Value;
  EFI_TPL             OldTpl;

  PciIo = &PciIoDevice->PciIo;

  //
  // Preserve the original value
  //

  PciIoRead (PciIo, EfiPciIoWidthUint32, (UINT8) Offset, 1, &OriginalValue);

  //
  // Raise TPL to high level to disable timer interrupt while the BAR is probed
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  PciIoWrite (PciIo, EfiPciIoWidthUint32, (UINT8) Offset, 1, &gAllOne);
  PciIoRead (PciIo, EfiPciIoWidthUint32, (UINT8) Offset, 1, &Value);

  //
  // Write back the original value
  //
  PciIoWrite (PciIo, EfiPciIoWidthUint32, (UINT8) Offset, 1, &OriginalValue);

  //
  // Restore TPL to its original level
  //
  gBS->RestoreTPL (OldTpl);

  if (BarLengthValue != NULL) {
    *BarLengthValue = Value;
  }

  if (OriginalBarValue != NULL) {
    *OriginalBarValue = OriginalValue;
  }

  if (Value == 0) {
    return EFI_NOT_FOUND;
  } else {
    return EFI_SUCCESS;
  }
}

/**
  Test whether the device can support attributes 
  
  @param PciIoDevice   Pci device instance
  @param Command       Command register value.
  @param BridgeControl Bridge control value for PPB or P2C.
  @param OldCommand    Old command register offset
  @param OldBridgeControl Old Bridge control value for PPB or P2C.
  
  @return EFI_SUCCESS
**/
EFI_STATUS
PciTestSupportedAttribute (
  IN PCI_IO_DEVICE                      *PciIoDevice,
  IN UINT16                             *Command,
  IN UINT16                             *BridgeControl,
  IN UINT16                             *OldCommand,
  IN UINT16                             *OldBridgeControl
  )
{
  EFI_TPL OldTpl;

  //
  // Preserve the original value
  //
  PciReadCommandRegister (PciIoDevice, OldCommand);

  //
  // Raise TPL to high level to disable timer interrupt while the BAR is probed
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  PciSetCommandRegister (PciIoDevice, *Command);
  PciReadCommandRegister (PciIoDevice, Command);

  //
  // Write back the original value
  //
  PciSetCommandRegister (PciIoDevice, *OldCommand);

  //
  // Restore TPL to its original level
  //
  gBS->RestoreTPL (OldTpl);

  if (IS_PCI_BRIDGE (&PciIoDevice->Pci) || IS_CARDBUS_BRIDGE (&PciIoDevice->Pci)) {

    //
    // Preserve the original value
    //
    PciReadBridgeControlRegister (PciIoDevice, OldBridgeControl);

    //
    // Raise TPL to high level to disable timer interrupt while the BAR is probed
    //
    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

    PciSetBridgeControlRegister (PciIoDevice, *BridgeControl);
    PciReadBridgeControlRegister (PciIoDevice, BridgeControl);

    //
    // Write back the original value
    //
    PciSetBridgeControlRegister (PciIoDevice, *OldBridgeControl);

    //
    // Restore TPL to its original level
    //
    gBS->RestoreTPL (OldTpl);

  } else {
    *OldBridgeControl = 0;
    *BridgeControl    = 0;
  }

  return EFI_SUCCESS;
}

/**
  Set the supported or current attributes of a PCI device
  
  @param PciIoDevice   - Structure pointer for PCI device.
  @param Command       - Command register value.
  @param BridgeControl - Bridge control value for PPB or P2C.
  @param Option        - Make a choice of EFI_SET_SUPPORTS or EFI_SET_ATTRIBUTES.
  
**/
EFI_STATUS
PciSetDeviceAttribute (
  IN PCI_IO_DEVICE                      *PciIoDevice,
  IN UINT16                             Command,
  IN UINT16                             BridgeControl,
  IN UINTN                              Option
  )
{
  UINT64  Attributes;

  Attributes = 0;

  if (Command & EFI_PCI_COMMAND_IO_SPACE) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_IO;
  }

  if (Command & EFI_PCI_COMMAND_MEMORY_SPACE) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_MEMORY;
  }

  if (Command & EFI_PCI_COMMAND_BUS_MASTER) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_BUS_MASTER;
  }

  if (Command & EFI_PCI_COMMAND_VGA_PALETTE_SNOOP) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO;
  }

  if (BridgeControl & EFI_PCI_BRIDGE_CONTROL_ISA) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_ISA_IO;
  }

  if (BridgeControl & EFI_PCI_BRIDGE_CONTROL_VGA) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_IO;
    Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY;
    Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO;
  }

  if (BridgeControl & EFI_PCI_BRIDGE_CONTROL_VGA_16) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_IO_16;
    Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16;
  }

  if (Option == EFI_SET_SUPPORTS) {

    Attributes |= EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE |
                  EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED        |
                  EFI_PCI_IO_ATTRIBUTE_MEMORY_DISABLE       |
                  EFI_PCI_IO_ATTRIBUTE_EMBEDDED_DEVICE      |
                  EFI_PCI_IO_ATTRIBUTE_EMBEDDED_ROM         |
                  EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE;

    if (Attributes & EFI_PCI_IO_ATTRIBUTE_IO) {
      Attributes |= EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO;
      Attributes |= EFI_PCI_IO_ATTRIBUTE_ISA_IO;
    }

    if (IS_PCI_BRIDGE (&PciIoDevice->Pci) || IS_CARDBUS_BRIDGE (&PciIoDevice->Pci)) {
      //
      // For bridge, it should support IDE attributes
      //
      Attributes |= EFI_PCI_IO_ATTRIBUTE_IDE_SECONDARY_IO;
      Attributes |= EFI_PCI_IO_ATTRIBUTE_IDE_PRIMARY_IO;
    } else {

      if (IS_PCI_IDE (&PciIoDevice->Pci)) {
        Attributes |= EFI_PCI_IO_ATTRIBUTE_IDE_SECONDARY_IO;
        Attributes |= EFI_PCI_IO_ATTRIBUTE_IDE_PRIMARY_IO;
      }

      if (IS_PCI_VGA (&PciIoDevice->Pci)) {
        Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY;
        Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_IO;
      }
    }

    PciIoDevice->Supports = Attributes;
    PciIoDevice->Supports &= ( (PciIoDevice->Parent->Supports) | \
                               EFI_PCI_IO_ATTRIBUTE_IO | EFI_PCI_IO_ATTRIBUTE_MEMORY | \
                               EFI_PCI_IO_ATTRIBUTE_BUS_MASTER );

  } else {
    PciIoDevice->Attributes = Attributes;
  }

  return EFI_SUCCESS;
}

/**
  Determine if the device can support Fast Back to Back attribute
  
  @param PciIoDevice  Pci device instance
  @param StatusIndex  Status register value
**/
EFI_STATUS
GetFastBackToBackSupport (
  IN PCI_IO_DEVICE                      *PciIoDevice,
  IN UINT8                              StatusIndex
  )
{
  EFI_PCI_IO_PROTOCOL *PciIo;
  EFI_STATUS          Status;
  UINT32              StatusRegister;

  //
  // Read the status register
  //
  PciIo   = &PciIoDevice->PciIo;
  Status  = PciIoRead (PciIo, EfiPciIoWidthUint16, StatusIndex, 1, &StatusRegister);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check the Fast B2B bit
  //
  if (StatusRegister & EFI_PCI_FAST_BACK_TO_BACK_CAPABLE) {
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }

}

/**
  Process the option ROM for all the children of the specified parent PCI device.
  It can only be used after the first full Option ROM process.

  @param PciIoDevice Pci device instance
  
  @retval EFI_SUCCESS Success Operation.
**/
EFI_STATUS
ProcessOptionRomLight (
  IN PCI_IO_DEVICE                      *PciIoDevice
  )
{
  PCI_IO_DEVICE   *Temp;
  LIST_ENTRY      *CurrentLink;

  //
  // For RootBridge, PPB , P2C, go recursively to traverse all its children
  //
  CurrentLink = PciIoDevice->ChildList.ForwardLink;
  while (CurrentLink && CurrentLink != &PciIoDevice->ChildList) {

    Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);

    if (!IsListEmpty (&Temp->ChildList)) {
      ProcessOptionRomLight (Temp);
    }

    PciRomGetImageMapping (Temp);

    //
    // The OpRom has already been processed in the first round
    //
    Temp->AllOpRomProcessed = TRUE;

    CurrentLink = CurrentLink->ForwardLink;
  }

  return EFI_SUCCESS;
}

/**
 Determine the related attributes of all devices under a Root Bridge
 
 @param PciIoDevice   PCI device instance
 
**/
EFI_STATUS
DetermineDeviceAttribute (
  IN PCI_IO_DEVICE                      *PciIoDevice
  )
{
  UINT16          Command;
  UINT16          BridgeControl;
  UINT16          OldCommand;
  UINT16          OldBridgeControl;
  BOOLEAN         FastB2BSupport;

  /*
  UINT8  IdePI;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  */
  PCI_IO_DEVICE   *Temp;
  LIST_ENTRY      *CurrentLink;
  EFI_STATUS      Status;

  //
  // For Root Bridge, just copy it by RootBridgeIo proctocol
  // so as to keep consistent with the actual attribute
  //
  if (!PciIoDevice->Parent) {
    Status = PciIoDevice->PciRootBridgeIo->GetAttributes (
                                            PciIoDevice->PciRootBridgeIo,
                                            &PciIoDevice->Supports,
                                            &PciIoDevice->Attributes
                                            );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {

    //
    // Set the attributes to be checked for common PCI devices and PPB or P2C
    // Since some devices only support part of them, it is better to set the
    // attribute according to its command or bridge control register
    //
    Command = EFI_PCI_COMMAND_IO_SPACE     |
              EFI_PCI_COMMAND_MEMORY_SPACE |
              EFI_PCI_COMMAND_BUS_MASTER   |
              EFI_PCI_COMMAND_VGA_PALETTE_SNOOP;

    BridgeControl = EFI_PCI_BRIDGE_CONTROL_ISA | EFI_PCI_BRIDGE_CONTROL_VGA | EFI_PCI_BRIDGE_CONTROL_VGA_16;

    //
    // Test whether the device can support attributes above
    //
    PciTestSupportedAttribute (PciIoDevice, &Command, &BridgeControl, &OldCommand, &OldBridgeControl);

    //
    // Set the supported attributes for specified PCI device
    //
    PciSetDeviceAttribute (PciIoDevice, Command, BridgeControl, EFI_SET_SUPPORTS);

    //
    // Set the current attributes for specified PCI device
    //
    PciSetDeviceAttribute (PciIoDevice, OldCommand, OldBridgeControl, EFI_SET_ATTRIBUTES);

    //
    // Enable other supported attributes but not defined in PCI_IO_PROTOCOL
    //
    PciEnableCommandRegister (PciIoDevice, EFI_PCI_COMMAND_MEMORY_WRITE_AND_INVALIDATE);

    //
    // Enable IDE native mode
    //
    /*
    if (IS_PCI_IDE(&PciIoDevice->Pci)) {

      PciIo = &PciIoDevice->PciIo;

      PciIoRead (
                              PciIo,
                              EfiPciIoWidthUint8,
                              0x09,
                              1,
                              &IdePI
                              );

      //
      // Set native mode if it can be supported
      //
      IdePI |= (((IdePI & 0x0F) >> 1) & 0x05);

      PciIoWrite (
                              PciIo,
                              EfiPciIoWidthUint8,
                              0x09,
                              1,
                              &IdePI
                              );

    }
    */
  }

  FastB2BSupport = TRUE;

  //
  // P2C can not support FB2B on the secondary side
  //
  if (IS_CARDBUS_BRIDGE (&PciIoDevice->Pci)) {
    FastB2BSupport = FALSE;
  }

  //
  // For RootBridge, PPB , P2C, go recursively to traverse all its children
  //
  CurrentLink = PciIoDevice->ChildList.ForwardLink;
  while (CurrentLink && CurrentLink != &PciIoDevice->ChildList) {

    Temp    = PCI_IO_DEVICE_FROM_LINK (CurrentLink);
    Status  = DetermineDeviceAttribute (Temp);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Detect Fast Bact to Bact support for the device under the bridge
    //
    Status = GetFastBackToBackSupport (Temp, PCI_PRIMARY_STATUS_OFFSET);
    if (FastB2BSupport && EFI_ERROR (Status)) {
      FastB2BSupport = FALSE;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }
  //
  // Set or clear Fast Back to Back bit for the whole bridge
  //
  if (!IsListEmpty (&PciIoDevice->ChildList)) {

    if (IS_PCI_BRIDGE (&PciIoDevice->Pci)) {

      Status = GetFastBackToBackSupport (PciIoDevice, PCI_BRIDGE_STATUS_REGISTER_OFFSET);

      if (EFI_ERROR (Status) || (!FastB2BSupport)) {
        FastB2BSupport = FALSE;
        PciDisableBridgeControlRegister (PciIoDevice, EFI_PCI_BRIDGE_CONTROL_FAST_BACK_TO_BACK);
      } else {
        PciEnableBridgeControlRegister (PciIoDevice, EFI_PCI_BRIDGE_CONTROL_FAST_BACK_TO_BACK);
      }
    }

    CurrentLink = PciIoDevice->ChildList.ForwardLink;
    while (CurrentLink && CurrentLink != &PciIoDevice->ChildList) {
      Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);
      if (FastB2BSupport) {
        PciEnableCommandRegister (Temp, EFI_PCI_COMMAND_FAST_BACK_TO_BACK);
      } else {
        PciDisableCommandRegister (Temp, EFI_PCI_COMMAND_FAST_BACK_TO_BACK);
      }

      CurrentLink = CurrentLink->ForwardLink;
    }
  }
  //
  // End for IsListEmpty
  //
  return EFI_SUCCESS;
}

/**
  This routine is used to update the bar information for those incompatible PCI device
  
  @param PciIoDevice      Pci device instance
  @return EFI_UNSUPPORTED failed to update Pci Info
**/
EFI_STATUS
UpdatePciInfo (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
  EFI_STATUS                        Status;
  UINTN                             BarIndex;
  UINTN                             BarEndIndex;
  BOOLEAN                           SetFlag;
  EFI_PCI_DEVICE_INFO               PciDeviceInfo;
  VOID                              *Configuration;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Ptr;

  Configuration = NULL;
  Status        = EFI_SUCCESS;

  if (gEfiIncompatiblePciDeviceSupport == NULL) {
    //
    // It can only be supported after the Incompatible PCI Device
    // Support Protocol has been installed
    //
    Status = gBS->LocateProtocol (
                    &gEfiIncompatiblePciDeviceSupportProtocolGuid,
                    NULL,
                    (VOID **) &gEfiIncompatiblePciDeviceSupport
                    );
  }
  if (Status == EFI_SUCCESS) {
      //
      // Check whether the device belongs to incompatible devices from protocol or not
      // If it is , then get its special requirement in the ACPI table
      //
      Status = gEfiIncompatiblePciDeviceSupport->CheckDevice (
                                                  gEfiIncompatiblePciDeviceSupport,
                                                  PciIoDevice->Pci.Hdr.VendorId,
                                                  PciIoDevice->Pci.Hdr.DeviceId,
                                                  PciIoDevice->Pci.Hdr.RevisionID,
                                                  PciIoDevice->Pci.Device.SubsystemVendorID,
                                                  PciIoDevice->Pci.Device.SubsystemID,
                                                  &Configuration
                                                  );

  }

  if (EFI_ERROR (Status)) {
    //
    // Check whether the device belongs to incompatible devices from library or not
    // If it is , then get its special requirement in the ACPI table
    //
    if (PcdGet8 (PcdPciIncompatibleDeviceSupportMask) & PCI_INCOMPATIBLE_ACPI_RESOURCE_SUPPORT) {
      PciDeviceInfo.VendorID          = PciIoDevice->Pci.Hdr.VendorId;
      PciDeviceInfo.DeviceID          = PciIoDevice->Pci.Hdr.DeviceId;
      PciDeviceInfo.RevisionID        = PciIoDevice->Pci.Hdr.RevisionID;
      PciDeviceInfo.SubsystemVendorID = PciIoDevice->Pci.Device.SubsystemVendorID;
      PciDeviceInfo.SubsystemID       = PciIoDevice->Pci.Device.SubsystemID;

      Status = PciResourceUpdateCheck (&PciDeviceInfo, &Configuration);
    }
  }

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Update PCI device information from the ACPI table
  //
  Ptr = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;

  while (Ptr->Desc != ACPI_END_TAG_DESCRIPTOR) {

    if (Ptr->Desc != ACPI_ADDRESS_SPACE_DESCRIPTOR) {
      //
      // The format is not support
      //
      break;
    }

    BarIndex    = (UINTN) Ptr->AddrTranslationOffset;
    BarEndIndex = BarIndex;

    //
    // Update all the bars in the device
    //
    if (BarIndex == PCI_BAR_ALL) {
      BarIndex    = 0;
      BarEndIndex = PCI_MAX_BAR - 1;
    }

    if (BarIndex >= PCI_MAX_BAR) {
      Ptr++;
      continue;
    }

    for (; BarIndex <= BarEndIndex; BarIndex++) {
      SetFlag = FALSE;
      switch (Ptr->ResType) {
      case ACPI_ADDRESS_SPACE_TYPE_MEM:

        //
        // Make sure the bar is memory type
        //
        if (CheckBarType (PciIoDevice, (UINT8) BarIndex, PciBarTypeMem)) {
          SetFlag = TRUE;
        }
        break;

      case ACPI_ADDRESS_SPACE_TYPE_IO:

        //
        // Make sure the bar is IO type
        //
        if (CheckBarType (PciIoDevice, (UINT8) BarIndex, PciBarTypeIo)) {
          SetFlag = TRUE;
        }
        break;
      }

      if (SetFlag) {

        //
        // Update the new alignment for the device
        //
        SetNewAlign (&(PciIoDevice->PciBar[BarIndex].Alignment), Ptr->AddrRangeMax);

        //
        // Update the new length for the device
        //
        if (Ptr->AddrLen != PCI_BAR_NOCHANGE) {
          PciIoDevice->PciBar[BarIndex].Length = Ptr->AddrLen;
        }
      }
    }

    Ptr++;
  }

  gBS->FreePool (Configuration);
  return Status;

}

/**
  This routine will update the alignment with the new alignment
  
  @param Alignment old alignment
  @param NewAlignment new alignment
  
**/
VOID
SetNewAlign (
  IN UINT64 *Alignment,
  IN UINT64 NewAlignment
  )
{
  UINT64  OldAlignment;
  UINTN   ShiftBit;

  //
  // The new alignment is the same as the original,
  // so skip it
  //
  if (NewAlignment == PCI_BAR_OLD_ALIGN) {
    return ;
  }
  //
  // Check the validity of the parameter
  //
   if (NewAlignment != PCI_BAR_EVEN_ALIGN  &&
       NewAlignment != PCI_BAR_SQUAD_ALIGN &&
       NewAlignment != PCI_BAR_DQUAD_ALIGN ) {
    *Alignment = NewAlignment;
    return ;
  }

  OldAlignment  = (*Alignment) + 1;
  ShiftBit      = 0;

  //
  // Get the first non-zero hex value of the length
  //
  while ((OldAlignment & 0x0F) == 0x00) {
    OldAlignment = RShiftU64 (OldAlignment, 4);
    ShiftBit += 4;
  }

  //
  // Adjust the alignment to even, quad or double quad boundary
  //
  if (NewAlignment == PCI_BAR_EVEN_ALIGN) {
    if (OldAlignment & 0x01) {
      OldAlignment = OldAlignment + 2 - (OldAlignment & 0x01);
    }
  } else if (NewAlignment == PCI_BAR_SQUAD_ALIGN) {
    if (OldAlignment & 0x03) {
      OldAlignment = OldAlignment + 4 - (OldAlignment & 0x03);
    }
  } else if (NewAlignment == PCI_BAR_DQUAD_ALIGN) {
    if (OldAlignment & 0x07) {
      OldAlignment = OldAlignment + 8 - (OldAlignment & 0x07);
    }
  }

  //
  // Update the old value
  //
  NewAlignment  = LShiftU64 (OldAlignment, ShiftBit) - 1;
  *Alignment    = NewAlignment;

  return ;
}

/**
  Parse PCI bar bit.
  
  @param PciIoDevice  Pci device instance
  @param Offset       bar offset
  @param BarIndex     bar index
  
  @return next bar offset.
**/
UINTN
PciParseBar (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINTN          Offset,
  IN UINTN          BarIndex
  )
{
  UINT32      Value;
  UINT32      OriginalValue;
  UINT32      Mask;
  UINT32      Data;
  UINT8       Index;
  EFI_STATUS  Status;

  OriginalValue = 0;
  Value         = 0;

  Status = BarExisted (
            PciIoDevice,
            Offset,
            &Value,
            &OriginalValue
            );

  if (EFI_ERROR (Status)) {
    PciIoDevice->PciBar[BarIndex].BaseAddress = 0;
    PciIoDevice->PciBar[BarIndex].Length      = 0;
    PciIoDevice->PciBar[BarIndex].Alignment   = 0;

    //
    // Some devices don't fully comply to PCI spec 2.2. So be to scan all the BARs anyway
    //
    PciIoDevice->PciBar[BarIndex].Offset = (UINT8) Offset;
    return Offset + 4;
  }

  PciIoDevice->PciBar[BarIndex].Offset = (UINT8) Offset;
  if (Value & 0x01) {
    //
    // Device I/Os
    //
    Mask = 0xfffffffc;

    if (Value & 0xFFFF0000) {
      //
      // It is a IO32 bar
      //
      PciIoDevice->PciBar[BarIndex].BarType   = PciBarTypeIo32;
      PciIoDevice->PciBar[BarIndex].Length    = ((~(Value & Mask)) + 1);
      PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;

    } else {
      //
      // It is a IO16 bar
      //
      PciIoDevice->PciBar[BarIndex].BarType   = PciBarTypeIo16;
      PciIoDevice->PciBar[BarIndex].Length    = 0x0000FFFF & ((~(Value & Mask)) + 1);
      PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;

    }
    //
    // Workaround. Some platforms inplement IO bar with 0 length
    // Need to treat it as no-bar
    //
    if (PciIoDevice->PciBar[BarIndex].Length == 0) {
      PciIoDevice->PciBar[BarIndex].BarType = (PCI_BAR_TYPE) 0;
    }

    PciIoDevice->PciBar[BarIndex].Prefetchable  = FALSE;
    PciIoDevice->PciBar[BarIndex].BaseAddress   = OriginalValue & Mask;

  } else {

    Mask  = 0xfffffff0;

    PciIoDevice->PciBar[BarIndex].BaseAddress = OriginalValue & Mask;

    switch (Value & 0x07) {

    //
    //memory space; anywhere in 32 bit address space
    //
    case 0x00:
      if (Value & 0x08) {
        PciIoDevice->PciBar[BarIndex].BarType = PciBarTypePMem32;
      } else {
        PciIoDevice->PciBar[BarIndex].BarType = PciBarTypeMem32;
      }

      PciIoDevice->PciBar[BarIndex].Length    = (~(Value & Mask)) + 1;
      PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;

      break;

    //
    // memory space; anywhere in 64 bit address space
    //
    case 0x04:
      if (Value & 0x08) {
        PciIoDevice->PciBar[BarIndex].BarType = PciBarTypePMem64;
      } else {
        PciIoDevice->PciBar[BarIndex].BarType = PciBarTypeMem64;
      }

      //
      // According to PCI 2.2,if the bar indicates a memory 64 decoding, next bar
      // is regarded as an extension for the first bar. As a result
      // the sizing will be conducted on combined 64 bit value
      // Here just store the masked first 32bit value for future size
      // calculation
      //
      PciIoDevice->PciBar[BarIndex].Length    = Value & Mask;
      PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;

      //
      // Increment the offset to point to next DWORD
      //
      Offset += 4;

      Status = BarExisted (
                PciIoDevice,
                Offset,
                &Value,
                &OriginalValue
                );

      if (EFI_ERROR (Status)) {
        return Offset + 4;
      }

      //
      // Fix the length to support some spefic 64 bit BAR
      //
      Data  = Value;
      Index = 0;
      for (Data = Value; Data != 0; Data >>= 1) {
        Index ++;
      }
      Value |= ((UINT32)(-1) << Index);

      //
      // Calculate the size of 64bit bar
      //
      PciIoDevice->PciBar[BarIndex].BaseAddress |= LShiftU64 ((UINT64) OriginalValue, 32);

      PciIoDevice->PciBar[BarIndex].Length    = PciIoDevice->PciBar[BarIndex].Length | LShiftU64 ((UINT64) Value, 32);
      PciIoDevice->PciBar[BarIndex].Length    = (~(PciIoDevice->PciBar[BarIndex].Length)) + 1;
      PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;

      break;

    //
    // reserved
    //
    default:
      PciIoDevice->PciBar[BarIndex].BarType   = PciBarTypeUnknown;
      PciIoDevice->PciBar[BarIndex].Length    = (~(Value & Mask)) + 1;
      PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;

      break;
    }
  }

  //
  // Check the length again so as to keep compatible with some special bars
  //
  if (PciIoDevice->PciBar[BarIndex].Length == 0) {
    PciIoDevice->PciBar[BarIndex].BarType     = PciBarTypeUnknown;
    PciIoDevice->PciBar[BarIndex].BaseAddress = 0;
    PciIoDevice->PciBar[BarIndex].Alignment   = 0;
  }

  //
  // Increment number of bar
  //
  return Offset + 4;
}

/**
  This routine is used to initialize the bar of a PCI device
  It can be called typically when a device is going to be rejected

  @param PciIoDevice Pci device instance
**/
EFI_STATUS
InitializePciDevice (
  IN PCI_IO_DEVICE *PciIoDevice
  )
{
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT8               Offset;

  PciIo = &(PciIoDevice->PciIo);

  //
  // Put all the resource apertures
  // Resource base is set to all ones so as to indicate its resource
  // has not been alloacted
  //
  for (Offset = 0x10; Offset <= 0x24; Offset += sizeof (UINT32)) {
    PciIoWrite (PciIo, EfiPciIoWidthUint32, Offset, 1, &gAllOne);
  }

  return EFI_SUCCESS;
}

/**
  Init PPB for bridge device
  
  @param  PciIoDevice Pci device instance
**/
EFI_STATUS
InitializePpb (
  IN PCI_IO_DEVICE *PciIoDevice
  )
{
  EFI_PCI_IO_PROTOCOL *PciIo;

  PciIo = &(PciIoDevice->PciIo);

  //
  // Put all the resource apertures including IO16
  // Io32, pMem32, pMem64 to quiescent state
  // Resource base all ones, Resource limit all zeros
  //
  PciIoWrite (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &gAllOne);
  PciIoWrite (PciIo, EfiPciIoWidthUint8, 0x1D, 1, &gAllZero);

  PciIoWrite (PciIo, EfiPciIoWidthUint16, 0x20, 1, &gAllOne);
  PciIoWrite (PciIo, EfiPciIoWidthUint16, 0x22, 1, &gAllZero);

  PciIoWrite (PciIo, EfiPciIoWidthUint16, 0x24, 1, &gAllOne);
  PciIoWrite (PciIo, EfiPciIoWidthUint16, 0x26, 1, &gAllZero);

  PciIoWrite (PciIo, EfiPciIoWidthUint32, 0x28, 1, &gAllOne);
  PciIoWrite (PciIo, EfiPciIoWidthUint32, 0x2C, 1, &gAllZero);

  //
  // don't support use io32 as for now
  //
  PciIoWrite (PciIo, EfiPciIoWidthUint16, 0x30, 1, &gAllOne);
  PciIoWrite (PciIo, EfiPciIoWidthUint16, 0x32, 1, &gAllZero);

  //
  // Force Interrupt line to zero for cards that come up randomly
  //
  PciIoWrite (PciIo, EfiPciIoWidthUint8, 0x3C, 1, &gAllZero);

  return EFI_SUCCESS;
}

/**
  Init private data for Hotplug bridge device
  
  @param PciIoDevice hotplug bridge device
**/
EFI_STATUS
InitializeP2C (
  IN PCI_IO_DEVICE *PciIoDevice
  )
{
  EFI_PCI_IO_PROTOCOL *PciIo;

  PciIo = &(PciIoDevice->PciIo);

  //
  // Put all the resource apertures including IO16
  // Io32, pMem32, pMem64 to quiescent state(
  // Resource base all ones, Resource limit all zeros
  //
  PciIoWrite (PciIo, EfiPciIoWidthUint32, 0x1c, 1, &gAllOne);
  PciIoWrite (PciIo, EfiPciIoWidthUint32, 0x20, 1, &gAllZero);

  PciIoWrite (PciIo, EfiPciIoWidthUint32, 0x24, 1, &gAllOne);
  PciIoWrite (PciIo, EfiPciIoWidthUint32, 0x28, 1, &gAllZero);

  PciIoWrite (PciIo, EfiPciIoWidthUint32, 0x2c, 1, &gAllOne);
  PciIoWrite (PciIo, EfiPciIoWidthUint32, 0x30, 1, &gAllZero);

  PciIoWrite (PciIo, EfiPciIoWidthUint32, 0x34, 1, &gAllOne);
  PciIoWrite (PciIo, EfiPciIoWidthUint32, 0x38, 1, &gAllZero);

  //
  // Force Interrupt line to zero for cards that come up randomly
  //
  PciIoWrite (PciIo, EfiPciIoWidthUint8, 0x3C, 1, &gAllZero);
  return EFI_SUCCESS;
}

/**
  Create and initiliaze general PCI I/O device instance for
  PCI device/bridge device/hotplug bridge device.
  
  @param PciRootBridgeIo   Pointer to instance of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
  @param Pci               Pci bar block
  @param Bus               device Bus NO.
  @param Device            device device NO.
  @param Func              device func NO.
  
  @return instance of PCI device
**/
PCI_IO_DEVICE *
CreatePciIoDevice (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN PCI_TYPE00                       *Pci,
  UINT8                               Bus,
  UINT8                               Device,
  UINT8                               Func
  )
{

  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = NULL;

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (PCI_IO_DEVICE),
                  (VOID **) &PciIoDevice
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  ZeroMem (PciIoDevice, sizeof (PCI_IO_DEVICE));

  PciIoDevice->Signature        = PCI_IO_DEVICE_SIGNATURE;
  PciIoDevice->Handle           = NULL;
  PciIoDevice->PciRootBridgeIo  = PciRootBridgeIo;
  PciIoDevice->DevicePath       = NULL;
  PciIoDevice->BusNumber        = Bus;
  PciIoDevice->DeviceNumber     = Device;
  PciIoDevice->FunctionNumber   = Func;
  PciIoDevice->Decodes          = 0;
  if (gFullEnumeration) {
    PciIoDevice->Allocated = FALSE;
  } else {
    PciIoDevice->Allocated = TRUE;
  }

  PciIoDevice->Registered         = FALSE;
  PciIoDevice->Attributes         = 0;
  PciIoDevice->Supports           = 0;
  PciIoDevice->BusOverride        = FALSE;
  PciIoDevice->AllOpRomProcessed  = FALSE;

  PciIoDevice->IsPciExp           = FALSE;

  CopyMem (&(PciIoDevice->Pci), Pci, sizeof (PCI_TYPE01));

  //
  // Initialize the PCI I/O instance structure
  //

  Status  = InitializePciIoInstance (PciIoDevice);
  Status  = InitializePciDriverOverrideInstance (PciIoDevice);

  if (EFI_ERROR (Status)) {
    gBS->FreePool (PciIoDevice);
    return NULL;
  }

  //
  // Initialize the reserved resource list
  //
  InitializeListHead (&PciIoDevice->ReservedResourceList);

  //
  // Initialize the driver list
  //
  InitializeListHead (&PciIoDevice->OptionRomDriverList);

  //
  // Initialize the child list
  //
  InitializeListHead (&PciIoDevice->ChildList);

  return PciIoDevice;
}

/**
  This routine is used to enumerate entire pci bus system
  in a given platform
  It is only called on the second start on the same Root Bridge.

  @param Controller  Parent bridge handler
  
  @return status of operation.
**/
EFI_STATUS
PciEnumeratorLight (
  IN EFI_HANDLE                    Controller
  )
{

  EFI_STATUS                        Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL   *PciRootBridgeIo;
  PCI_IO_DEVICE                     *RootBridgeDev;
  UINT16                            MinBus;
  UINT16                            MaxBus;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptors;

  MinBus      = 0;
  MaxBus      = PCI_MAX_BUS;
  Descriptors = NULL;

  //
  // If this root bridge has been already enumerated, then return successfully
  //
  if (GetRootBridgeByHandle (Controller) != NULL) {
    return EFI_SUCCESS;
  }

  //
  // Open pci root bridge io protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  (VOID **) &PciRootBridgeIo,
                  gPciBusDriverBinding.DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  Status = PciRootBridgeIo->Configuration (PciRootBridgeIo, (VOID **) &Descriptors);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (PciGetBusRange (&Descriptors, &MinBus, &MaxBus, NULL) == EFI_SUCCESS) {

    //
    // Create a device node for root bridge device with a NULL host bridge controller handle
    //
    RootBridgeDev = CreateRootBridge (Controller);

    if (!RootBridgeDev) {
      Descriptors++;
      continue;
    }

    //
    // Record the root bridge io protocol
    //
    RootBridgeDev->PciRootBridgeIo = PciRootBridgeIo;

    Status = PciPciDeviceInfoCollector (
              RootBridgeDev,
              (UINT8) MinBus
              );

    if (!EFI_ERROR (Status)) {

      //
      // Remove those PCI devices which are rejected when full enumeration
      //
      RemoveRejectedPciDevices (RootBridgeDev->Handle, RootBridgeDev);

      //
      // Process option rom light
      //
      ProcessOptionRomLight (RootBridgeDev);

      //
      // Determine attributes for all devices under this root bridge
      //
      DetermineDeviceAttribute (RootBridgeDev);

      //
      // If successfully, insert the node into device pool
      //
      InsertRootBridge (RootBridgeDev);
    } else {

      //
      // If unsuccessly, destroy the entire node
      //
      DestroyRootBridge (RootBridgeDev);
    }

    Descriptors++;
  }

  return EFI_SUCCESS;
}

/**
  Get bus range.
  
  @param Descriptors  A pointer to the address space descriptor.
  @param MinBus       The min bus.
  @param MaxBus       The max bus.
  @param BusRange     The bus range.
  
  @retval EFI_SUCCESS Success operation.
  @retval EFI_NOT_FOUND  can not find the specific bus.
**/
EFI_STATUS
PciGetBusRange (
  IN     EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  **Descriptors,
  OUT    UINT16                             *MinBus,
  OUT    UINT16                             *MaxBus,
  OUT    UINT16                             *BusRange
  )
{

  while ((*Descriptors)->Desc != ACPI_END_TAG_DESCRIPTOR) {
    if ((*Descriptors)->ResType == ACPI_ADDRESS_SPACE_TYPE_BUS) {
      if (MinBus != NULL) {
        *MinBus = (UINT16) (*Descriptors)->AddrRangeMin;
      }

      if (MaxBus != NULL) {
        *MaxBus = (UINT16) (*Descriptors)->AddrRangeMax;
      }

      if (BusRange != NULL) {
        *BusRange = (UINT16) (*Descriptors)->AddrLen;
      }

      return EFI_SUCCESS;
    }

    (*Descriptors)++;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
StartManagingRootBridge (
  IN PCI_IO_DEVICE *RootBridgeDev
  )
{
  EFI_HANDLE                      RootBridgeHandle;
  EFI_STATUS                      Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;

  //
  // Get the root bridge handle
  //
  RootBridgeHandle  = RootBridgeDev->Handle;
  PciRootBridgeIo   = NULL;

  //
  // Get the pci root bridge io protocol
  //
  Status = gBS->OpenProtocol (
                  RootBridgeHandle,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  (VOID **) &PciRootBridgeIo,
                  gPciBusDriverBinding.DriverBindingHandle,
                  RootBridgeHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  //
  // Store the PciRootBridgeIo protocol into root bridge private data
  //
  RootBridgeDev->PciRootBridgeIo = PciRootBridgeIo;

  return EFI_SUCCESS;

}

/**
  This routine can be used to check whether a PCI device should be rejected when light enumeration

  @param PciIoDevice  Pci device instance

  @retval TRUE      This device should be rejected
  @retval FALSE     This device shouldn't be rejected
  
**/
BOOLEAN
IsPciDeviceRejected (
  IN PCI_IO_DEVICE *PciIoDevice
  )
{
  EFI_STATUS  Status;
  UINT32      TestValue;
  UINT32      OldValue;
  UINT32      Mask;
  UINT8       BarOffset;

  //
  // PPB should be skip!
  //
  if (IS_PCI_BRIDGE (&PciIoDevice->Pci)) {
    return FALSE;
  }

  if (IS_CARDBUS_BRIDGE (&PciIoDevice->Pci)) {
    //
    // Only test base registers for P2C
    //
    for (BarOffset = 0x1C; BarOffset <= 0x38; BarOffset += 2 * sizeof (UINT32)) {

      Mask    = (BarOffset < 0x2C) ? 0xFFFFF000 : 0xFFFFFFFC;
      Status  = BarExisted (PciIoDevice, BarOffset, &TestValue, &OldValue);
      if (EFI_ERROR (Status)) {
        continue;
      }

      TestValue = TestValue & Mask;
      if ((TestValue != 0) && (TestValue == (OldValue & Mask))) {
        //
        // The bar isn't programed, so it should be rejected
        //
        return TRUE;
      }
    }

    return FALSE;
  }

  for (BarOffset = 0x14; BarOffset <= 0x24; BarOffset += sizeof (UINT32)) {
    //
    // Test PCI devices
    //
    Status = BarExisted (PciIoDevice, BarOffset, &TestValue, &OldValue);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (TestValue & 0x01) {

      //
      // IO Bar
      //

      Mask      = 0xFFFFFFFC;
      TestValue = TestValue & Mask;
      if ((TestValue != 0) && (TestValue == (OldValue & Mask))) {
        return TRUE;
      }

    } else {

      //
      // Mem Bar
      //

      Mask      = 0xFFFFFFF0;
      TestValue = TestValue & Mask;

      if ((TestValue & 0x07) == 0x04) {

        //
        // Mem64 or PMem64
        //
        BarOffset += sizeof (UINT32);
        if ((TestValue != 0) && (TestValue == (OldValue & Mask))) {

          //
          // Test its high 32-Bit BAR
          //

          Status = BarExisted (PciIoDevice, BarOffset, &TestValue, &OldValue);
          if (TestValue == OldValue) {
            return TRUE;
          }
        }

      } else {

        //
        // Mem32 or PMem32
        //
        if ((TestValue != 0) && (TestValue == (OldValue & Mask))) {
          return TRUE;
        }
      }
    }
  }

  return FALSE;
}

/**
  Reset and all bus number from specific bridge.
  
  @param Bridge           Parent specific bridge
  @param StartBusNumber   start bus number
**/
EFI_STATUS
ResetAllPpbBusNumber (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber
  )
{
  EFI_STATUS                      Status;
  PCI_TYPE00                      Pci;
  UINT8                           Device;
  UINT32                          Register;
  UINT8                           Func;
  UINT64                          Address;
  UINT8                           SecondaryBus;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;

  PciRootBridgeIo = Bridge->PciRootBridgeIo;

  for (Device = 0; Device <= PCI_MAX_DEVICE; Device++) {
    for (Func = 0; Func <= PCI_MAX_FUNC; Func++) {

      //
      // Check to see whether a pci device is present
      //
      Status = PciDevicePresent (
                PciRootBridgeIo,
                &Pci,
                StartBusNumber,
                Device,
                Func
                );

      if (!EFI_ERROR (Status) && (IS_PCI_BRIDGE (&Pci))) {

        Register  = 0;
        Address   = EFI_PCI_ADDRESS (StartBusNumber, Device, Func, 0x18);
        Status   = PciRootBridgeIoRead (
                                        PciRootBridgeIo,
                                        &Pci,
                                        EfiPciWidthUint32,
                                        Address,
                                        1,
                                        &Register
                                        );
        SecondaryBus = (UINT8)(Register >> 8);

        if (SecondaryBus != 0) {
          ResetAllPpbBusNumber (Bridge, SecondaryBus);
        }

        //
        // Reset register 18h, 19h, 1Ah on PCI Bridge
        //
        Register &= 0xFF000000;
        Status = PciRootBridgeIoWrite (
                                        PciRootBridgeIo,
                                        &Pci,
                                        EfiPciWidthUint32,
                                        Address,
                                        1,
                                        &Register
                                        );
      }

      if (Func == 0 && !IS_PCI_MULTI_FUNC (&Pci)) {
        //
        // Skip sub functions, this is not a multi function device
        //
        Func = PCI_MAX_FUNC;
      }
    }
  }

  return EFI_SUCCESS;
}

