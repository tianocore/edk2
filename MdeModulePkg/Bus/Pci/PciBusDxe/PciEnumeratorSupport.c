/** @file
  PCI emumeration support functions implementation for PCI Bus module.

Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PciBus.h"

extern CHAR16  *mBarTypeStr[];

/**
  This routine is used to check whether the pci device is present.

  @param PciRootBridgeIo   Pointer to instance of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param Pci               Output buffer for PCI device configuration space.
  @param Bus               PCI bus NO.
  @param Device            PCI device NO.
  @param Func              PCI Func NO.

  @retval EFI_NOT_FOUND    PCI device not present.
  @retval EFI_SUCCESS      PCI device is found.

**/
EFI_STATUS
PciDevicePresent (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL     *PciRootBridgeIo,
  OUT PCI_TYPE00                          *Pci,
  IN  UINT8                               Bus,
  IN  UINT8                               Device,
  IN  UINT8                               Func
  )
{
  UINT64      Address;
  EFI_STATUS  Status;

  //
  // Create PCI address map in terms of Bus, Device and Func
  //
  Address = EFI_PCI_ADDRESS (Bus, Device, Func, 0);

  //
  // Read the Vendor ID register
  //
  Status = PciRootBridgeIo->Pci.Read (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint32,
                                  Address,
                                  1,
                                  Pci
                                  );

  if (!EFI_ERROR (Status) && (Pci->Hdr).VendorId != 0xffff) {
    //
    // Read the entire config header for the device
    //
    Status = PciRootBridgeIo->Pci.Read (
                                    PciRootBridgeIo,
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
  Collect all the resource information under this root bridge.

  A database that records all the information about pci device subject to this
  root bridge will then be created.

  @param Bridge         Parent bridge instance.
  @param StartBusNumber Bus number of begining.

  @retval EFI_SUCCESS   PCI device is found.
  @retval other         Some error occurred when reading PCI bridge information.

**/
EFI_STATUS
PciPciDeviceInfoCollector (
  IN PCI_IO_DEVICE                      *Bridge,
  IN UINT8                              StartBusNumber
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

          Status  = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET, 1, &SecBus);

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
  Seach required device and create PCI device instance.

  @param Bridge     Parent bridge instance.
  @param Pci        Input PCI device information block.
  @param Bus        PCI bus NO.
  @param Device     PCI device NO.
  @param Func       PCI func  NO.
  @param PciDevice  Output of searched PCI device instance.

  @retval EFI_SUCCESS           Successfully created PCI device instance.
  @retval EFI_OUT_OF_RESOURCES  Cannot get PCI device information.

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

  DEBUG ((
    EFI_D_INFO,
    "PciBus: Discovered %s @ [%02x|%02x|%02x]\n",
    IS_PCI_BRIDGE (Pci) ?     L"PPB" :
    IS_CARDBUS_BRIDGE (Pci) ? L"P2C" :
                              L"PCI",
    Bus, Device, Func
    ));

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

  if (PciIoDevice == NULL) {
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
  Dump the PCI BAR information.

  @param PciIoDevice     PCI IO instance.
**/
VOID
DumpPciBars (
  IN PCI_IO_DEVICE                    *PciIoDevice
  )
{
  UINTN                               Index;

  for (Index = 0; Index < PCI_MAX_BAR; Index++) {
    if (PciIoDevice->PciBar[Index].BarType == PciBarTypeUnknown) {
      continue;
    }

    DEBUG ((
      EFI_D_INFO,
      "   BAR[%d]: Type = %s; Alignment = 0x%lx;\tLength = 0x%lx;\tOffset = 0x%02x\n",
      Index, mBarTypeStr[MIN (PciIoDevice->PciBar[Index].BarType, PciBarTypeMaxType)],
      PciIoDevice->PciBar[Index].Alignment, PciIoDevice->PciBar[Index].Length, PciIoDevice->PciBar[Index].Offset
      ));
  }

  for (Index = 0; Index < PCI_MAX_BAR; Index++) {
    if ((PciIoDevice->VfPciBar[Index].BarType == PciBarTypeUnknown) && (PciIoDevice->VfPciBar[Index].Length == 0)) {
      continue;
    }

    DEBUG ((
      EFI_D_INFO,
      " VFBAR[%d]: Type = %s; Alignment = 0x%lx;\tLength = 0x%lx;\tOffset = 0x%02x\n",
      Index, mBarTypeStr[MIN (PciIoDevice->VfPciBar[Index].BarType, PciBarTypeMaxType)],
      PciIoDevice->VfPciBar[Index].Alignment, PciIoDevice->VfPciBar[Index].Length, PciIoDevice->VfPciBar[Index].Offset
      ));
  }
  DEBUG ((EFI_D_INFO, "\n"));
}

/**
  Create PCI device instance for PCI device.

  @param Bridge   Parent bridge instance.
  @param Pci      Input PCI device information block.
  @param Bus      PCI device Bus NO.
  @param Device   PCI device Device NO.
  @param Func     PCI device's func NO.

  @return  Created PCI device instance.

**/
PCI_IO_DEVICE *
GatherDeviceInfo (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  IN UINT8                            Bus,
  IN UINT8                            Device,
  IN UINT8                            Func
  )
{
  UINTN                           Offset;
  UINTN                           BarIndex;
  PCI_IO_DEVICE                   *PciIoDevice;

  PciIoDevice = CreatePciIoDevice (
                  Bridge,
                  Pci,
                  Bus,
                  Device,
                  Func
                  );

  if (PciIoDevice == NULL) {
    return NULL;
  }

  //
  // If it is a full enumeration, disconnect the device in advance
  //
  if (gFullEnumeration) {

    PCI_DISABLE_COMMAND_REGISTER (PciIoDevice, EFI_PCI_COMMAND_BITS_OWNED);

  }

  //
  // Start to parse the bars
  //
  for (Offset = 0x10, BarIndex = 0; Offset <= 0x24 && BarIndex < PCI_MAX_BAR; BarIndex++) {
    Offset = PciParseBar (PciIoDevice, Offset, BarIndex);
  }

  //
  // Parse the SR-IOV VF bars
  //
  if (PcdGetBool (PcdSrIovSupport) && PciIoDevice->SrIovCapabilityOffset != 0) {
    for (Offset = PciIoDevice->SrIovCapabilityOffset + EFI_PCIE_CAPABILITY_ID_SRIOV_BAR0, BarIndex = 0;
         Offset <= PciIoDevice->SrIovCapabilityOffset + EFI_PCIE_CAPABILITY_ID_SRIOV_BAR5;
         BarIndex++) {

      ASSERT (BarIndex < PCI_MAX_BAR);
      Offset = PciIovParseVfBar (PciIoDevice, Offset, BarIndex);
    }
  }

  DEBUG_CODE (DumpPciBars (PciIoDevice););
  return PciIoDevice;
}

/**
  Create PCI device instance for PCI-PCI bridge.

  @param Bridge   Parent bridge instance.
  @param Pci      Input PCI device information block.
  @param Bus      PCI device Bus NO.
  @param Device   PCI device Device NO.
  @param Func     PCI device's func NO.

  @return  Created PCI device instance.

**/
PCI_IO_DEVICE *
GatherPpbInfo (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  IN UINT8                            Bus,
  IN UINT8                            Device,
  IN UINT8                            Func
  )
{
  PCI_IO_DEVICE                   *PciIoDevice;
  EFI_STATUS                      Status;
  UINT8                           Value;
  EFI_PCI_IO_PROTOCOL             *PciIo;
  UINT8                           Temp;
  UINT32                          PMemBaseLimit;
  UINT16                          PrefetchableMemoryBase;
  UINT16                          PrefetchableMemoryLimit;

  PciIoDevice = CreatePciIoDevice (
                  Bridge,
                  Pci,
                  Bus,
                  Device,
                  Func
                  );

  if (PciIoDevice == NULL) {
    return NULL;
  }

  if (gFullEnumeration) {
    PCI_DISABLE_COMMAND_REGISTER (PciIoDevice, EFI_PCI_COMMAND_BITS_OWNED);

    //
    // Initalize the bridge control register
    //
    PCI_DISABLE_BRIDGE_CONTROL_REGISTER (PciIoDevice, EFI_PCI_BRIDGE_CONTROL_BITS_OWNED);

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
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &Temp);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &gAllOne);
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &Value);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &Temp);

  if (Value != 0) {
    if ((Value & 0x01) != 0) {
      PciIoDevice->Decodes |= EFI_BRIDGE_IO32_DECODE_SUPPORTED;
    } else {
      PciIoDevice->Decodes |= EFI_BRIDGE_IO16_DECODE_SUPPORTED;
    }
  }

  //
  // if PcdPciBridgeIoAlignmentProbe is TRUE, PCI bus driver probes
  // PCI bridge supporting non-stardard I/O window alignment less than 4K.
  //

  PciIoDevice->BridgeIoAlignment = 0xFFF;
  if (FeaturePcdGet (PcdPciBridgeIoAlignmentProbe)) {
    //
    // Check any bits of bit 3-1 of I/O Base Register are writable.
    // if so, it is assumed non-stardard I/O window alignment is supported by this bridge.
    // Per spec, bit 3-1 of I/O Base Register are reserved bits, so its content can't be assumed.
    //
    Value = (UINT8)(Temp ^ (BIT3 | BIT2 | BIT1));
    PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &Value);
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &Value);
    PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &Temp);
    Value = (UINT8)((Value ^ Temp) & (BIT3 | BIT2 | BIT1));
    switch (Value) {
      case BIT3:
        PciIoDevice->BridgeIoAlignment = 0x7FF;
        break;
      case BIT3 | BIT2:
        PciIoDevice->BridgeIoAlignment = 0x3FF;
        break;
      case BIT3 | BIT2 | BIT1:
        PciIoDevice->BridgeIoAlignment = 0x1FF;
        break;
    }
  }

  Status = BarExisted (
            PciIoDevice,
            0x24,
            NULL,
            &PMemBaseLimit
            );

  //
  // Test if it supports 64 memory or not
  //
  // The bottom 4 bits of both the Prefetchable Memory Base and Prefetchable Memory Limit
  // registers:
  //   0 - the bridge supports only 32 bit addresses.
  //   1 - the bridge supports 64-bit addresses.
  //
  PrefetchableMemoryBase = (UINT16)(PMemBaseLimit & 0xffff);
  PrefetchableMemoryLimit = (UINT16)(PMemBaseLimit >> 16);
  if (!EFI_ERROR (Status) &&
      (PrefetchableMemoryBase & 0x000f) == 0x0001 &&
      (PrefetchableMemoryLimit & 0x000f) == 0x0001) {
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

  DEBUG_CODE (DumpPciBars (PciIoDevice););

  return PciIoDevice;
}


/**
  Create PCI device instance for PCI Card bridge device.

  @param Bridge   Parent bridge instance.
  @param Pci      Input PCI device information block.
  @param Bus      PCI device Bus NO.
  @param Device   PCI device Device NO.
  @param Func     PCI device's func NO.

  @return  Created PCI device instance.

**/
PCI_IO_DEVICE *
GatherP2CInfo (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  IN UINT8                            Bus,
  IN UINT8                            Device,
  IN UINT8                            Func
  )
{
  PCI_IO_DEVICE                   *PciIoDevice;

  PciIoDevice = CreatePciIoDevice (
                  Bridge,
                  Pci,
                  Bus,
                  Device,
                  Func
                  );

  if (PciIoDevice == NULL) {
    return NULL;
  }

  if (gFullEnumeration) {
    PCI_DISABLE_COMMAND_REGISTER (PciIoDevice, EFI_PCI_COMMAND_BITS_OWNED);

    //
    // Initalize the bridge control register
    //
    PCI_DISABLE_BRIDGE_CONTROL_REGISTER (PciIoDevice, EFI_PCCARD_BRIDGE_CONTROL_BITS_OWNED);
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

  DEBUG_CODE (DumpPciBars (PciIoDevice););

  return PciIoDevice;
}

/**
  Create device path for pci deivce.

  @param ParentDevicePath  Parent bridge's path.
  @param PciIoDevice       Pci device instance.

  @return Device path protocol instance for specific pci device.

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
  Check whether the PCI IOV VF bar is existed or not.

  @param PciIoDevice       A pointer to the PCI_IO_DEVICE.
  @param Offset            The offset.
  @param BarLengthValue    The bar length value returned.
  @param OriginalBarValue  The original bar value returned.

  @retval EFI_NOT_FOUND    The bar doesn't exist.
  @retval EFI_SUCCESS      The bar exist.

**/
EFI_STATUS
VfBarExisted (
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

  //
  // Ensure it is called properly
  //
  ASSERT (PciIoDevice->SrIovCapabilityOffset != 0);
  if (PciIoDevice->SrIovCapabilityOffset == 0) {
    return EFI_NOT_FOUND;
  }

  PciIo = &PciIoDevice->PciIo;

  //
  // Preserve the original value
  //

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, (UINT32)Offset, 1, &OriginalValue);

  //
  // Raise TPL to high level to disable timer interrupt while the BAR is probed
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, (UINT32)Offset, 1, &gAllOne);
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, (UINT32)Offset, 1, &Value);

  //
  // Write back the original value
  //
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, (UINT32)Offset, 1, &OriginalValue);

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
  Check whether the bar is existed or not.

  @param PciIoDevice       A pointer to the PCI_IO_DEVICE.
  @param Offset            The offset.
  @param BarLengthValue    The bar length value returned.
  @param OriginalBarValue  The original bar value returned.

  @retval EFI_NOT_FOUND    The bar doesn't exist.
  @retval EFI_SUCCESS      The bar exist.

**/
EFI_STATUS
BarExisted (
  IN  PCI_IO_DEVICE *PciIoDevice,
  IN  UINTN         Offset,
  OUT UINT32        *BarLengthValue,
  OUT UINT32        *OriginalBarValue
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
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, (UINT8) Offset, 1, &OriginalValue);

  //
  // Raise TPL to high level to disable timer interrupt while the BAR is probed
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, (UINT8) Offset, 1, &gAllOne);
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, (UINT8) Offset, 1, &Value);

  //
  // Write back the original value
  //
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, (UINT8) Offset, 1, &OriginalValue);

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
  Test whether the device can support given attributes.

  @param PciIoDevice      Pci device instance.
  @param Command          Input command register value, and
                          returned supported register value.
  @param BridgeControl    Inout bridge control value for PPB or P2C, and
                          returned supported bridge control value.
  @param OldCommand       Returned and stored old command register offset.
  @param OldBridgeControl Returned and stored old Bridge control value for PPB or P2C.

**/
VOID
PciTestSupportedAttribute (
  IN     PCI_IO_DEVICE                      *PciIoDevice,
  IN OUT UINT16                             *Command,
  IN OUT UINT16                             *BridgeControl,
     OUT UINT16                             *OldCommand,
     OUT UINT16                             *OldBridgeControl
  )
{
  EFI_TPL OldTpl;

  //
  // Preserve the original value
  //
  PCI_READ_COMMAND_REGISTER (PciIoDevice, OldCommand);

  //
  // Raise TPL to high level to disable timer interrupt while the BAR is probed
  //
  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  PCI_SET_COMMAND_REGISTER (PciIoDevice, *Command);
  PCI_READ_COMMAND_REGISTER (PciIoDevice, Command);

  //
  // Write back the original value
  //
  PCI_SET_COMMAND_REGISTER (PciIoDevice, *OldCommand);

  //
  // Restore TPL to its original level
  //
  gBS->RestoreTPL (OldTpl);

  if (IS_PCI_BRIDGE (&PciIoDevice->Pci) || IS_CARDBUS_BRIDGE (&PciIoDevice->Pci)) {

    //
    // Preserve the original value
    //
    PCI_READ_BRIDGE_CONTROL_REGISTER (PciIoDevice, OldBridgeControl);

    //
    // Raise TPL to high level to disable timer interrupt while the BAR is probed
    //
    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

    PCI_SET_BRIDGE_CONTROL_REGISTER (PciIoDevice, *BridgeControl);
    PCI_READ_BRIDGE_CONTROL_REGISTER (PciIoDevice, BridgeControl);

    //
    // Write back the original value
    //
    PCI_SET_BRIDGE_CONTROL_REGISTER (PciIoDevice, *OldBridgeControl);

    //
    // Restore TPL to its original level
    //
    gBS->RestoreTPL (OldTpl);

  } else {
    *OldBridgeControl = 0;
    *BridgeControl    = 0;
  }
}

/**
  Set the supported or current attributes of a PCI device.

  @param PciIoDevice    Structure pointer for PCI device.
  @param Command        Command register value.
  @param BridgeControl  Bridge control value for PPB or P2C.
  @param Option         Make a choice of EFI_SET_SUPPORTS or EFI_SET_ATTRIBUTES.

**/
VOID
PciSetDeviceAttribute (
  IN PCI_IO_DEVICE                      *PciIoDevice,
  IN UINT16                             Command,
  IN UINT16                             BridgeControl,
  IN UINTN                              Option
  )
{
  UINT64  Attributes;

  Attributes = 0;

  if ((Command & EFI_PCI_COMMAND_IO_SPACE) != 0) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_IO;
  }

  if ((Command & EFI_PCI_COMMAND_MEMORY_SPACE) != 0) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_MEMORY;
  }

  if ((Command & EFI_PCI_COMMAND_BUS_MASTER) != 0) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_BUS_MASTER;
  }

  if ((Command & EFI_PCI_COMMAND_VGA_PALETTE_SNOOP) != 0) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO;
  }

  if ((BridgeControl & EFI_PCI_BRIDGE_CONTROL_ISA) != 0) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_ISA_IO;
  }

  if ((BridgeControl & EFI_PCI_BRIDGE_CONTROL_VGA) != 0) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_IO;
    Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY;
    Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO;
  }

  if ((BridgeControl & EFI_PCI_BRIDGE_CONTROL_VGA_16) != 0) {
    Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_IO_16;
    Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16;
  }

  if (Option == EFI_SET_SUPPORTS) {

    Attributes |= (UINT64) (EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE |
                  EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED        |
                  EFI_PCI_IO_ATTRIBUTE_MEMORY_DISABLE       |
                  EFI_PCI_IO_ATTRIBUTE_EMBEDDED_DEVICE      |
                  EFI_PCI_IO_ATTRIBUTE_EMBEDDED_ROM         |
                  EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE);

    if (IS_PCI_LPC (&PciIoDevice->Pci)) {
        Attributes |= EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO;
        Attributes |= (mReserveIsaAliases ? (UINT64) EFI_PCI_IO_ATTRIBUTE_ISA_IO : \
                                            (UINT64) EFI_PCI_IO_ATTRIBUTE_ISA_IO_16);
    }

    if (IS_PCI_BRIDGE (&PciIoDevice->Pci) || IS_CARDBUS_BRIDGE (&PciIoDevice->Pci)) {
      //
      // For bridge, it should support IDE attributes
      //
      Attributes |= EFI_PCI_IO_ATTRIBUTE_IDE_SECONDARY_IO;
      Attributes |= EFI_PCI_IO_ATTRIBUTE_IDE_PRIMARY_IO;

      if (mReserveVgaAliases) {
        Attributes &= ~(UINT64)(EFI_PCI_IO_ATTRIBUTE_VGA_IO_16 | \
                                EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16);
      } else {
        Attributes &= ~(UINT64)(EFI_PCI_IO_ATTRIBUTE_VGA_IO | \
                                EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO);
      }
    } else {

      if (IS_PCI_IDE (&PciIoDevice->Pci)) {
        Attributes |= EFI_PCI_IO_ATTRIBUTE_IDE_SECONDARY_IO;
        Attributes |= EFI_PCI_IO_ATTRIBUTE_IDE_PRIMARY_IO;
      }

      if (IS_PCI_VGA (&PciIoDevice->Pci)) {
        Attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY;
        Attributes |= (mReserveVgaAliases ? (UINT64) EFI_PCI_IO_ATTRIBUTE_VGA_IO : \
                                            (UINT64) EFI_PCI_IO_ATTRIBUTE_VGA_IO_16);
      }
    }

    PciIoDevice->Supports = Attributes;
    PciIoDevice->Supports &= ( (PciIoDevice->Parent->Supports) | \
                               EFI_PCI_IO_ATTRIBUTE_IO | EFI_PCI_IO_ATTRIBUTE_MEMORY | \
                               EFI_PCI_IO_ATTRIBUTE_BUS_MASTER );

  } else {
    //
    // When this attribute is clear, the RomImage and RomSize fields in the PCI IO were
    // initialized based on the PCI option ROM found through the ROM BAR of the PCI controller.
    // When this attribute is set, the PCI option ROM described by the RomImage and RomSize
    // fields is not from the the ROM BAR of the PCI controller.
    //
    if (!PciIoDevice->EmbeddedRom) {
      Attributes |= EFI_PCI_IO_ATTRIBUTE_EMBEDDED_ROM;
    }
    PciIoDevice->Attributes = Attributes;
  }
}

/**
  Determine if the device can support Fast Back to Back attribute.

  @param PciIoDevice  Pci device instance.
  @param StatusIndex  Status register value.

  @retval EFI_SUCCESS       This device support Fast Back to Back attribute.
  @retval EFI_UNSUPPORTED   This device doesn't support Fast Back to Back attribute.

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
  Status  = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, StatusIndex, 1, &StatusRegister);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check the Fast B2B bit
  //
  if ((StatusRegister & EFI_PCI_FAST_BACK_TO_BACK_CAPABLE) != 0) {
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
}

/**
  Process the option ROM for all the children of the specified parent PCI device.
  It can only be used after the first full Option ROM process.

  @param PciIoDevice Pci device instance.

**/
VOID
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
  while (CurrentLink != NULL && CurrentLink != &PciIoDevice->ChildList) {

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
}

/**
  Determine the related attributes of all devices under a Root Bridge.

  @param PciIoDevice   PCI device instance.

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
  PCI_IO_DEVICE   *Temp;
  LIST_ENTRY      *CurrentLink;
  EFI_STATUS      Status;

  //
  // For Root Bridge, just copy it by RootBridgeIo proctocol
  // so as to keep consistent with the actual attribute
  //
  if (PciIoDevice->Parent == NULL) {
    Status = PciIoDevice->PciRootBridgeIo->GetAttributes (
                                            PciIoDevice->PciRootBridgeIo,
                                            &PciIoDevice->Supports,
                                            &PciIoDevice->Attributes
                                            );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Assume the PCI Root Bridge supports DAC
    //
    PciIoDevice->Supports |= (UINT64)(EFI_PCI_IO_ATTRIBUTE_EMBEDDED_DEVICE |
                              EFI_PCI_IO_ATTRIBUTE_EMBEDDED_ROM |
                              EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE);

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
    PCI_ENABLE_COMMAND_REGISTER (PciIoDevice, EFI_PCI_COMMAND_MEMORY_WRITE_AND_INVALIDATE);
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
  while (CurrentLink != NULL && CurrentLink != &PciIoDevice->ChildList) {

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
        PCI_DISABLE_BRIDGE_CONTROL_REGISTER (PciIoDevice, EFI_PCI_BRIDGE_CONTROL_FAST_BACK_TO_BACK);
      } else {
        PCI_ENABLE_BRIDGE_CONTROL_REGISTER (PciIoDevice, EFI_PCI_BRIDGE_CONTROL_FAST_BACK_TO_BACK);
      }
    }

    CurrentLink = PciIoDevice->ChildList.ForwardLink;
    while (CurrentLink != NULL && CurrentLink != &PciIoDevice->ChildList) {
      Temp = PCI_IO_DEVICE_FROM_LINK (CurrentLink);
      if (FastB2BSupport) {
        PCI_ENABLE_COMMAND_REGISTER (Temp, EFI_PCI_COMMAND_FAST_BACK_TO_BACK);
      } else {
        PCI_DISABLE_COMMAND_REGISTER (Temp, EFI_PCI_COMMAND_FAST_BACK_TO_BACK);
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
  This routine is used to update the bar information for those incompatible PCI device.

  @param PciIoDevice      Input Pci device instance. Output Pci device instance with updated
                          Bar information.

  @retval EFI_SUCCESS     Successfully updated bar information.
  @retval EFI_UNSUPPORTED Given PCI device doesn't belong to incompatible PCI device list.

**/
EFI_STATUS
UpdatePciInfo (
  IN OUT PCI_IO_DEVICE    *PciIoDevice
  )
{
  EFI_STATUS                        Status;
  UINTN                             BarIndex;
  UINTN                             BarEndIndex;
  BOOLEAN                           SetFlag;
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

  if (EFI_ERROR (Status) || Configuration == NULL ) {
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

    if (BarIndex > PCI_MAX_BAR) {
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

  FreePool (Configuration);

  return EFI_SUCCESS;
}

/**
  This routine will update the alignment with the new alignment.

  @param Alignment    Input Old alignment. Output updated alignment.
  @param NewAlignment New alignment.

**/
VOID
SetNewAlign (
  IN OUT UINT64     *Alignment,
  IN     UINT64     NewAlignment
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
    if ((OldAlignment & 0x01) != 0) {
      OldAlignment = OldAlignment + 2 - (OldAlignment & 0x01);
    }
  } else if (NewAlignment == PCI_BAR_SQUAD_ALIGN) {
    if ((OldAlignment & 0x03) != 0) {
      OldAlignment = OldAlignment + 4 - (OldAlignment & 0x03);
    }
  } else if (NewAlignment == PCI_BAR_DQUAD_ALIGN) {
    if ((OldAlignment & 0x07) != 0) {
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
  Parse PCI IOV VF bar information and fill them into PCI device instance.

  @param PciIoDevice  Pci device instance.
  @param Offset       Bar offset.
  @param BarIndex     Bar index.

  @return Next bar offset.

**/
UINTN
PciIovParseVfBar (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINTN          Offset,
  IN UINTN          BarIndex
  )
{
  UINT32      Value;
  UINT32      OriginalValue;
  UINT32      Mask;
  EFI_STATUS  Status;

  //
  // Ensure it is called properly
  //
  ASSERT (PciIoDevice->SrIovCapabilityOffset != 0);
  if (PciIoDevice->SrIovCapabilityOffset == 0) {
    return 0;
  }

  OriginalValue = 0;
  Value         = 0;

  Status = VfBarExisted (
            PciIoDevice,
            Offset,
            &Value,
            &OriginalValue
            );

  if (EFI_ERROR (Status)) {
    PciIoDevice->VfPciBar[BarIndex].BaseAddress = 0;
    PciIoDevice->VfPciBar[BarIndex].Length      = 0;
    PciIoDevice->VfPciBar[BarIndex].Alignment   = 0;

    //
    // Scan all the BARs anyway
    //
    PciIoDevice->VfPciBar[BarIndex].Offset = (UINT16) Offset;
    return Offset + 4;
  }

  PciIoDevice->VfPciBar[BarIndex].Offset = (UINT16) Offset;
  if ((Value & 0x01) != 0) {
    //
    // Device I/Os. Impossible
    //
    ASSERT (FALSE);
    return Offset + 4;

  } else {

    Mask  = 0xfffffff0;

    PciIoDevice->VfPciBar[BarIndex].BaseAddress = OriginalValue & Mask;

    switch (Value & 0x07) {

    //
    //memory space; anywhere in 32 bit address space
    //
    case 0x00:
      if ((Value & 0x08) != 0) {
        PciIoDevice->VfPciBar[BarIndex].BarType = PciBarTypePMem32;
      } else {
        PciIoDevice->VfPciBar[BarIndex].BarType = PciBarTypeMem32;
      }

      PciIoDevice->VfPciBar[BarIndex].Length    = (~(Value & Mask)) + 1;
      PciIoDevice->VfPciBar[BarIndex].Alignment = PciIoDevice->VfPciBar[BarIndex].Length - 1;

      //
      // Adjust Length
      //
      PciIoDevice->VfPciBar[BarIndex].Length = MultU64x32 (PciIoDevice->VfPciBar[BarIndex].Length, PciIoDevice->InitialVFs);
      //
      // Adjust Alignment
      //
      if (PciIoDevice->VfPciBar[BarIndex].Alignment < PciIoDevice->SystemPageSize - 1) {
        PciIoDevice->VfPciBar[BarIndex].Alignment = PciIoDevice->SystemPageSize - 1;
      }

      break;

    //
    // memory space; anywhere in 64 bit address space
    //
    case 0x04:
      if ((Value & 0x08) != 0) {
        PciIoDevice->VfPciBar[BarIndex].BarType = PciBarTypePMem64;
      } else {
        PciIoDevice->VfPciBar[BarIndex].BarType = PciBarTypeMem64;
      }

      //
      // According to PCI 2.2,if the bar indicates a memory 64 decoding, next bar
      // is regarded as an extension for the first bar. As a result
      // the sizing will be conducted on combined 64 bit value
      // Here just store the masked first 32bit value for future size
      // calculation
      //
      PciIoDevice->VfPciBar[BarIndex].Length    = Value & Mask;
      PciIoDevice->VfPciBar[BarIndex].Alignment = PciIoDevice->VfPciBar[BarIndex].Length - 1;

      if (PciIoDevice->VfPciBar[BarIndex].Alignment < PciIoDevice->SystemPageSize - 1) {
        PciIoDevice->VfPciBar[BarIndex].Alignment = PciIoDevice->SystemPageSize - 1;
      }

      //
      // Increment the offset to point to next DWORD
      //
      Offset += 4;

      Status = VfBarExisted (
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
      Value |= ((UINT32) -1 << HighBitSet32 (Value));

      //
      // Calculate the size of 64bit bar
      //
      PciIoDevice->VfPciBar[BarIndex].BaseAddress |= LShiftU64 ((UINT64) OriginalValue, 32);

      PciIoDevice->VfPciBar[BarIndex].Length    = PciIoDevice->VfPciBar[BarIndex].Length | LShiftU64 ((UINT64) Value, 32);
      PciIoDevice->VfPciBar[BarIndex].Length    = (~(PciIoDevice->VfPciBar[BarIndex].Length)) + 1;
      PciIoDevice->VfPciBar[BarIndex].Alignment = PciIoDevice->VfPciBar[BarIndex].Length - 1;

      //
      // Adjust Length
      //
      PciIoDevice->VfPciBar[BarIndex].Length = MultU64x32 (PciIoDevice->VfPciBar[BarIndex].Length, PciIoDevice->InitialVFs);
      //
      // Adjust Alignment
      //
      if (PciIoDevice->VfPciBar[BarIndex].Alignment < PciIoDevice->SystemPageSize - 1) {
        PciIoDevice->VfPciBar[BarIndex].Alignment = PciIoDevice->SystemPageSize - 1;
      }

      break;

    //
    // reserved
    //
    default:
      PciIoDevice->VfPciBar[BarIndex].BarType   = PciBarTypeUnknown;
      PciIoDevice->VfPciBar[BarIndex].Length    = (~(Value & Mask)) + 1;
      PciIoDevice->VfPciBar[BarIndex].Alignment = PciIoDevice->VfPciBar[BarIndex].Length - 1;

      if (PciIoDevice->VfPciBar[BarIndex].Alignment < PciIoDevice->SystemPageSize - 1) {
        PciIoDevice->VfPciBar[BarIndex].Alignment = PciIoDevice->SystemPageSize - 1;
      }

      break;
    }
  }
  
  //
  // Check the length again so as to keep compatible with some special bars
  //
  if (PciIoDevice->VfPciBar[BarIndex].Length == 0) {
    PciIoDevice->VfPciBar[BarIndex].BarType     = PciBarTypeUnknown;
    PciIoDevice->VfPciBar[BarIndex].BaseAddress = 0;
    PciIoDevice->VfPciBar[BarIndex].Alignment   = 0;
  }
  
  //
  // Increment number of bar
  //
  return Offset + 4;
}

/**
  Parse PCI bar information and fill them into PCI device instance.

  @param PciIoDevice  Pci device instance.
  @param Offset       Bar offset.
  @param BarIndex     Bar index.

  @return Next bar offset.

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
  if ((Value & 0x01) != 0) {
    //
    // Device I/Os
    //
    Mask = 0xfffffffc;

    if ((Value & 0xFFFF0000) != 0) {
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
      if ((Value & 0x08) != 0) {
        PciIoDevice->PciBar[BarIndex].BarType = PciBarTypePMem32;
      } else {
        PciIoDevice->PciBar[BarIndex].BarType = PciBarTypeMem32;
      }

      PciIoDevice->PciBar[BarIndex].Length    = (~(Value & Mask)) + 1;
      if (PciIoDevice->PciBar[BarIndex].Length < (SIZE_4KB)) {
        //
        // Force minimum 4KByte alignment for Virtualization technology for Directed I/O
        //
        PciIoDevice->PciBar[BarIndex].Alignment = (SIZE_4KB - 1);
      } else {
        PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;
      }
      break;

    //
    // memory space; anywhere in 64 bit address space
    //
    case 0x04:
      if ((Value & 0x08) != 0) {
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
        //
        // the high 32 bit does not claim any BAR, we need to re-check the low 32 bit BAR again
        //
        if (PciIoDevice->PciBar[BarIndex].Length == 0) {
          //
          // some device implement MMIO bar with 0 length, need to treat it as no-bar
          //
          PciIoDevice->PciBar[BarIndex].BarType = PciBarTypeUnknown;
          return Offset + 4;
        }
      }

      //
      // Fix the length to support some spefic 64 bit BAR
      //
      if (Value == 0) {
        DEBUG ((EFI_D_INFO, "[PciBus]BAR probing for upper 32bit of MEM64 BAR returns 0, change to 0xFFFFFFFF.\n"));
        Value = (UINT32) -1;
      } else {
        Value |= ((UINT32)(-1) << HighBitSet32 (Value));
      }

      //
      // Calculate the size of 64bit bar
      //
      PciIoDevice->PciBar[BarIndex].BaseAddress |= LShiftU64 ((UINT64) OriginalValue, 32);

      PciIoDevice->PciBar[BarIndex].Length    = PciIoDevice->PciBar[BarIndex].Length | LShiftU64 ((UINT64) Value, 32);
      PciIoDevice->PciBar[BarIndex].Length    = (~(PciIoDevice->PciBar[BarIndex].Length)) + 1;
      if (PciIoDevice->PciBar[BarIndex].Length < (SIZE_4KB)) {
        //
        // Force minimum 4KByte alignment for Virtualization technology for Directed I/O
        //
        PciIoDevice->PciBar[BarIndex].Alignment = (SIZE_4KB - 1);
      } else {
        PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;
      }

      break;

    //
    // reserved
    //
    default:
      PciIoDevice->PciBar[BarIndex].BarType   = PciBarTypeUnknown;
      PciIoDevice->PciBar[BarIndex].Length    = (~(Value & Mask)) + 1;
      if (PciIoDevice->PciBar[BarIndex].Length < (SIZE_4KB)) {
        //
        // Force minimum 4KByte alignment for Virtualization technology for Directed I/O
        //
        PciIoDevice->PciBar[BarIndex].Alignment = (SIZE_4KB - 1);
      } else {
        PciIoDevice->PciBar[BarIndex].Alignment = PciIoDevice->PciBar[BarIndex].Length - 1;
      }
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
  This routine is used to initialize the bar of a PCI device.

  @param PciIoDevice Pci device instance.

  @note It can be called typically when a device is going to be rejected.

**/
VOID
InitializePciDevice (
  IN PCI_IO_DEVICE    *PciIoDevice
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
    PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, Offset, 1, &gAllOne);
  }
}

/**
  This routine is used to initialize the bar of a PCI-PCI Bridge device.

  @param  PciIoDevice PCI-PCI bridge device instance.

**/
VOID
InitializePpb (
  IN PCI_IO_DEVICE    *PciIoDevice
  )
{
  EFI_PCI_IO_PROTOCOL *PciIo;

  PciIo = &(PciIoDevice->PciIo);

  //
  // Put all the resource apertures including IO16
  // Io32, pMem32, pMem64 to quiescent state
  // Resource base all ones, Resource limit all zeros
  //
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x1C, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x1D, 1, &gAllZero);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x20, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x22, 1, &gAllZero);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x24, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x26, 1, &gAllZero);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x28, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x2C, 1, &gAllZero);

  //
  // Don't support use io32 as for now
  //
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x30, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x32, 1, &gAllZero);

  //
  // Force Interrupt line to zero for cards that come up randomly
  //
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x3C, 1, &gAllZero);
}

/**
  This routine is used to initialize the bar of a PCI Card Bridge device.

  @param PciIoDevice  PCI Card bridge device.

**/
VOID
InitializeP2C (
  IN PCI_IO_DEVICE    *PciIoDevice
  )
{
  EFI_PCI_IO_PROTOCOL *PciIo;

  PciIo = &(PciIoDevice->PciIo);

  //
  // Put all the resource apertures including IO16
  // Io32, pMem32, pMem64 to quiescent state(
  // Resource base all ones, Resource limit all zeros
  //
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x1c, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x20, 1, &gAllZero);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x24, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x28, 1, &gAllZero);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x2c, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x30, 1, &gAllZero);

  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x34, 1, &gAllOne);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, 0x38, 1, &gAllZero);

  //
  // Force Interrupt line to zero for cards that come up randomly
  //
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 0x3C, 1, &gAllZero);
}

/**
  Create and initiliaze general PCI I/O device instance for
  PCI device/bridge device/hotplug bridge device.

  @param PciRootBridgeIo   Pointer to instance of EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param Pci               Input Pci information block.
  @param Bus               Device Bus NO.
  @param Device            Device device NO.
  @param Func              Device func NO.

  @return Instance of PCI device. NULL means no instance created.

**/
PCI_IO_DEVICE *
CreatePciIoDevice (
  IN PCI_IO_DEVICE                    *Bridge,
  IN PCI_TYPE00                       *Pci,
  IN UINT8                            Bus,
  IN UINT8                            Device,
  IN UINT8                            Func
  )
{
  PCI_IO_DEVICE        *PciIoDevice;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;

  PciIoDevice = AllocateZeroPool (sizeof (PCI_IO_DEVICE));
  if (PciIoDevice == NULL) {
    return NULL;
  }

  PciIoDevice->Signature        = PCI_IO_DEVICE_SIGNATURE;
  PciIoDevice->Handle           = NULL;
  PciIoDevice->PciRootBridgeIo  = Bridge->PciRootBridgeIo;
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
  InitializePciIoInstance (PciIoDevice);
  InitializePciDriverOverrideInstance (PciIoDevice);
  InitializePciLoadFile2 (PciIoDevice);
  PciIo = &PciIoDevice->PciIo;

  //
  // Create a device path for this PCI device and store it into its private data
  //
  CreatePciDevicePath (
    Bridge->DevicePath,
    PciIoDevice
    );

  //
  // Detect if PCI Express Device
  //
  PciIoDevice->PciExpressCapabilityOffset = 0;
  Status = LocateCapabilityRegBlock (
             PciIoDevice,
             EFI_PCI_CAPABILITY_ID_PCIEXP,
             &PciIoDevice->PciExpressCapabilityOffset,
             NULL
             );
  if (!EFI_ERROR (Status)) {
    PciIoDevice->IsPciExp = TRUE;
  }

  if (PcdGetBool (PcdAriSupport)) {
    //
    // Check if the device is an ARI device.
    //
    Status = LocatePciExpressCapabilityRegBlock (
               PciIoDevice,
               EFI_PCIE_CAPABILITY_ID_ARI,
               &PciIoDevice->AriCapabilityOffset,
               NULL
               );
    if (!EFI_ERROR (Status)) {
      //
      // We need to enable ARI feature before calculate BusReservation,
      // because FirstVFOffset and VFStride may change after that.
      //
      EFI_PCI_IO_PROTOCOL  *ParentPciIo;
      UINT32               Data32;

      //
      // Check if its parent supports ARI forwarding.
      //
      ParentPciIo = &Bridge->PciIo;
      ParentPciIo->Pci.Read (
                          ParentPciIo, 
                          EfiPciIoWidthUint32,
                          Bridge->PciExpressCapabilityOffset + EFI_PCIE_CAPABILITY_DEVICE_CAPABILITIES_2_OFFSET,
                          1,
                          &Data32
                          );
      if ((Data32 & EFI_PCIE_CAPABILITY_DEVICE_CAPABILITIES_2_ARI_FORWARDING) != 0) {
        //
        // ARI forward support in bridge, so enable it.
        //
        ParentPciIo->Pci.Read (
                            ParentPciIo,
                            EfiPciIoWidthUint32,
                            Bridge->PciExpressCapabilityOffset + EFI_PCIE_CAPABILITY_DEVICE_CONTROL_2_OFFSET,
                            1,
                            &Data32
                            );
        if ((Data32 & EFI_PCIE_CAPABILITY_DEVICE_CONTROL_2_ARI_FORWARDING) == 0) {
          Data32 |= EFI_PCIE_CAPABILITY_DEVICE_CONTROL_2_ARI_FORWARDING;
          ParentPciIo->Pci.Write (
                              ParentPciIo,
                              EfiPciIoWidthUint32,
                              Bridge->PciExpressCapabilityOffset + EFI_PCIE_CAPABILITY_DEVICE_CONTROL_2_OFFSET,
                              1,
                              &Data32
                              );
          DEBUG ((
            EFI_D_INFO,
            " ARI: forwarding enabled for PPB[%02x:%02x:%02x]\n",
            Bridge->BusNumber,
            Bridge->DeviceNumber,
            Bridge->FunctionNumber
            ));
        }
      }

      DEBUG ((EFI_D_INFO, " ARI: CapOffset = 0x%x\n", PciIoDevice->AriCapabilityOffset));
    }
  }

  //
  // Initialization for SR-IOV
  //

  if (PcdGetBool (PcdSrIovSupport)) {
    Status = LocatePciExpressCapabilityRegBlock (
               PciIoDevice,
               EFI_PCIE_CAPABILITY_ID_SRIOV,
               &PciIoDevice->SrIovCapabilityOffset,
               NULL
               );
    if (!EFI_ERROR (Status)) {
      UINT32    SupportedPageSize;
      UINT16    VFStride;
      UINT16    FirstVFOffset;
      UINT16    Data16;
      UINT32    PFRid;
      UINT32    LastVF;

      //
      // If the SR-IOV device is an ARI device, then Set ARI Capable Hierarchy for the device.
      //
      if (PcdGetBool (PcdAriSupport) && PciIoDevice->AriCapabilityOffset != 0) {
        PciIo->Pci.Read (
                     PciIo,
                     EfiPciIoWidthUint16,
                     PciIoDevice->SrIovCapabilityOffset + EFI_PCIE_CAPABILITY_ID_SRIOV_CONTROL,
                     1,
                     &Data16
                     );
        Data16 |= EFI_PCIE_CAPABILITY_ID_SRIOV_CONTROL_ARI_HIERARCHY;
        PciIo->Pci.Write (
                     PciIo,
                     EfiPciIoWidthUint16,
                     PciIoDevice->SrIovCapabilityOffset + EFI_PCIE_CAPABILITY_ID_SRIOV_CONTROL,
                     1,
                     &Data16
                     );
      }

      //
      // Calculate SystemPageSize
      //

      PciIo->Pci.Read (
                   PciIo,
                   EfiPciIoWidthUint32,
                   PciIoDevice->SrIovCapabilityOffset + EFI_PCIE_CAPABILITY_ID_SRIOV_SUPPORTED_PAGE_SIZE,
                   1,
                   &SupportedPageSize
                   );
      PciIoDevice->SystemPageSize = (PcdGet32 (PcdSrIovSystemPageSize) & SupportedPageSize);
      ASSERT (PciIoDevice->SystemPageSize != 0);

      PciIo->Pci.Write (
                   PciIo,
                   EfiPciIoWidthUint32,
                   PciIoDevice->SrIovCapabilityOffset + EFI_PCIE_CAPABILITY_ID_SRIOV_SYSTEM_PAGE_SIZE,
                   1,
                   &PciIoDevice->SystemPageSize
                   );
      //
      // Adjust SystemPageSize for Alignment usage later
      //
      PciIoDevice->SystemPageSize <<= 12;

      //
      // Calculate BusReservation for PCI IOV
      //

      //
      // Read First FirstVFOffset, InitialVFs, and VFStride
      //
      PciIo->Pci.Read (
                   PciIo,
                   EfiPciIoWidthUint16,
                   PciIoDevice->SrIovCapabilityOffset + EFI_PCIE_CAPABILITY_ID_SRIOV_FIRSTVF,
                   1,
                   &FirstVFOffset
                   );
      PciIo->Pci.Read (
                   PciIo,
                   EfiPciIoWidthUint16,
                   PciIoDevice->SrIovCapabilityOffset + EFI_PCIE_CAPABILITY_ID_SRIOV_INITIALVFS,
                   1,
                   &PciIoDevice->InitialVFs
                   );
      PciIo->Pci.Read (
                   PciIo,
                   EfiPciIoWidthUint16,
                   PciIoDevice->SrIovCapabilityOffset + EFI_PCIE_CAPABILITY_ID_SRIOV_VFSTRIDE,
                   1,
                   &VFStride
                   );
      //
      // Calculate LastVF
      //
      PFRid = EFI_PCI_RID(Bus, Device, Func);
      LastVF = PFRid + FirstVFOffset + (PciIoDevice->InitialVFs - 1) * VFStride;

      //
      // Calculate ReservedBusNum for this PF
      //
      PciIoDevice->ReservedBusNum = (UINT16)(EFI_PCI_BUS_OF_RID (LastVF) - Bus + 1);

      DEBUG ((
        EFI_D_INFO,
        " SR-IOV: SupportedPageSize = 0x%x; SystemPageSize = 0x%x; FirstVFOffset = 0x%x;\n",
        SupportedPageSize, PciIoDevice->SystemPageSize >> 12, FirstVFOffset
        ));
      DEBUG ((
        EFI_D_INFO,
        "         InitialVFs = 0x%x; ReservedBusNum = 0x%x; CapOffset = 0x%x\n",
        PciIoDevice->InitialVFs, PciIoDevice->ReservedBusNum, PciIoDevice->SrIovCapabilityOffset
        ));
    }
  }

  if (PcdGetBool (PcdMrIovSupport)) {
    Status = LocatePciExpressCapabilityRegBlock (
               PciIoDevice,
               EFI_PCIE_CAPABILITY_ID_MRIOV,
               &PciIoDevice->MrIovCapabilityOffset,
               NULL
               );
    if (!EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, " MR-IOV: CapOffset = 0x%x\n", PciIoDevice->MrIovCapabilityOffset));
    }
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
  in a given platform.

  It is only called on the second start on the same Root Bridge.

  @param  Controller     Parent bridge handler.

  @retval EFI_SUCCESS    PCI enumeration finished successfully.
  @retval other          Some error occurred when enumerating the pci bus system.

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

    if (RootBridgeDev == NULL) {
      Descriptors++;
      continue;
    }

    //
    // Record the root bridgeio protocol
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
  Get bus range from PCI resource descriptor list.

  @param Descriptors  A pointer to the address space descriptor.
  @param MinBus       The min bus returned.
  @param MaxBus       The max bus returned.
  @param BusRange     The bus range returned.

  @retval EFI_SUCCESS    Successfully got bus range.
  @retval EFI_NOT_FOUND  Can not find the specific bus.

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

/**
  This routine can be used to start the root bridge.

  @param RootBridgeDev     Pci device instance.

  @retval EFI_SUCCESS      This device started.
  @retval other            Failed to get PCI Root Bridge I/O protocol.

**/
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
  RootBridgeHandle = RootBridgeDev->Handle;
  PciRootBridgeIo  = NULL;

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
  This routine can be used to check whether a PCI device should be rejected when light enumeration.

  @param PciIoDevice  Pci device instance.

  @retval TRUE      This device should be rejected.
  @retval FALSE     This device shouldn't be rejected.

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

    if ((TestValue & 0x01) != 0) {

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
  Reset all bus number from specific bridge.

  @param Bridge           Parent specific bridge.
  @param StartBusNumber   Start bus number.

**/
VOID
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
        Status    = PciRootBridgeIo->Pci.Read (
                                           PciRootBridgeIo,
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
        Status = PciRootBridgeIo->Pci.Write (
                                        PciRootBridgeIo,
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
}

