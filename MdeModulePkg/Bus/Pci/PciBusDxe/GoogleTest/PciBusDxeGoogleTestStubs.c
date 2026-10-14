/** @file
  Stubs for PciBusDxe code outside the PciParseBar test scope.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "../PciBus.h"

EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL  *gIncompatiblePciDeviceSupport;
EFI_DRIVER_BINDING_PROTOCOL                   gPciBusDriverBinding;
BOOLEAN                                       gFullEnumeration;
UINT64                                        gAllZero;
BOOLEAN                                       mReserveIsaAliases;
BOOLEAN                                       mReserveVgaAliases;
CHAR16                                        *mBarTypeStr[PciBarTypeMaxType + 1];
EDKII_DEVICE_SECURITY_PROTOCOL                *mDeviceSecurityProtocol;
EFI_GUID                                      gEdkiiDeviceIdentifierTypePciGuid;

//
// The following functions only satisfy build and linker requirements for
// dependencies outside the PciParseBar unit test scope. Their no-op behavior
// and default return values do not model production behavior.
//

//
// Normally provided by the DevicePathLib library class.
//

/**
  Sets the length, in bytes, of a device path node.

  @param  Node    A pointer to a device path node data structure.
  @param  Length  The length, in bytes, of the device path node.

  @return Length.

**/
UINT16
EFIAPI
SetDevicePathNodeLength (
  IN OUT VOID  *Node,
  IN UINTN     Length
  )
{
  return 0;
}

/**
  Creates a new device path by appending a device path node.

  @param  DevicePath      A pointer to a device path data structure.
  @param  DevicePathNode  A pointer to a single device path node.

  @retval NULL    There is not enough memory for the new device path.
  @retval Others  A pointer to the new device path.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
AppendDevicePathNode (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath OPTIONAL,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode OPTIONAL
  )
{
  return NULL;
}

//
// Normally provided by PciBusDxe/PciIo.c.
//

/**
  Initializes a PCI I/O instance.

  @param  PciIoDevice  PCI device instance.

**/
VOID
InitializePciIoInstance (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
}

/**
  Checks a BAR type for a PCI resource.

  @param  PciIoDevice  PCI device instance.
  @param  BarIndex     The BAR index of the standard PCI configuration header.
  @param  BarType      Memory or I/O.

  @retval TRUE   The PCI device's BAR type matches BarType.
  @retval FALSE  The PCI device's BAR type does not match BarType.

**/
BOOLEAN
CheckBarType (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINT8          BarIndex,
  IN PCI_BAR_TYPE   BarType
  )
{
  return FALSE;
}

//
// Normally provided by PciBusDxe/PciCommand.c.
//

/**
  Operates on a PCI register through the PCI I/O interface.

  @param  PciIoDevice  Pointer to an instance of PCI_IO_DEVICE.
  @param  Command      Operator command.
  @param  Offset       Address within the PCI configuration space.
  @param  Operation    Type of operation.
  @param  PtrCommand   Return buffer holding the old PCI command, if Operation
                       is not EFI_SET_REGISTER.

  @return Status of the PCI I/O operation.

**/
EFI_STATUS
PciOperateRegister (
  IN  PCI_IO_DEVICE  *PciIoDevice,
  IN  UINT16         Command,
  IN  UINT8          Offset,
  IN  UINT8          Operation,
  OUT UINT16         *PtrCommand
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Locates a capability register block by capability ID.

  @param  PciIoDevice  A pointer to the PCI_IO_DEVICE.
  @param  CapId        The capability ID.
  @param  Offset       A pointer to the offset returned.
  @param  NextRegBlock A pointer to the next block returned.

  @retval EFI_SUCCESS      The capability register block was located.
  @retval EFI_UNSUPPORTED  The PCI device does not support capabilities.
  @retval EFI_NOT_FOUND    The PCI device supports capabilities, but the
                           requested register block was not found.

**/
EFI_STATUS
LocateCapabilityRegBlock (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINT8          CapId,
  IN OUT UINT8      *Offset,
  OUT UINT8         *NextRegBlock OPTIONAL
  )
{
  return EFI_NOT_FOUND;
}

/**
  Locates a PCI Express capability register block by capability ID.

  @param  PciIoDevice  A pointer to the PCI_IO_DEVICE.
  @param  CapId        The capability ID.
  @param  Offset       A pointer to the offset returned.
  @param  NextRegBlock A pointer to the next block returned.

  @retval EFI_SUCCESS      The capability register block was located.
  @retval EFI_UNSUPPORTED  The PCI device does not support capabilities.
  @retval EFI_NOT_FOUND    The PCI device supports capabilities, but the
                           requested register block was not found.

**/
EFI_STATUS
LocatePciExpressCapabilityRegBlock (
  IN     PCI_IO_DEVICE  *PciIoDevice,
  IN     UINT16         CapId,
  IN OUT UINT32         *Offset,
  OUT UINT32            *NextRegBlock OPTIONAL
  )
{
  return EFI_NOT_FOUND;
}

//
// Normally provided by PciBusDxe/PciDeviceSupport.c.
//

/**
  Inserts a root bridge into the PCI device pool.

  @param  RootBridge  A pointer to the PCI_IO_DEVICE.

**/
VOID
InsertRootBridge (
  IN PCI_IO_DEVICE  *RootBridge
  )
{
}

/**
  Inserts a PCI device node under a bridge.

  @param  Bridge         The PCI bridge.
  @param  PciDeviceNode  The PCI device to insert.

**/
VOID
InsertPciDevice (
  IN PCI_IO_DEVICE  *Bridge,
  IN PCI_IO_DEVICE  *PciDeviceNode
  )
{
}

/**
  Destroys a root bridge and removes it from the device tree.

  @param  RootBridge  The bridge to remove.

**/
VOID
DestroyRootBridge (
  IN PCI_IO_DEVICE  *RootBridge
  )
{
}

/**
  Creates a root bridge device.

  @param  RootBridgeHandle  The specified root bridge handle.

  @return The created root bridge device instance, or NULL if no instance was
          created.

**/
PCI_IO_DEVICE *
CreateRootBridge (
  IN EFI_HANDLE  RootBridgeHandle
  )
{
  return NULL;
}

/**
  Gets a root bridge device instance by root bridge handle.

  @param  RootBridgeHandle  The root bridge handle.

  @return The root bridge device instance, or NULL if no instance was found.

**/
PCI_IO_DEVICE *
GetRootBridgeByHandle (
  EFI_HANDLE  RootBridgeHandle
  )
{
  return NULL;
}

//
// Normally provided by PciBusDxe/PciResourceSupport.c.
//

/**
  Gets padding resources for a PCI-to-PCI bridge.

  @param  PciIoDevice  PCI-to-PCI bridge device instance.

**/
VOID
GetResourcePaddingPpb (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
}

//
// Normally provided by PciBusDxe/PciEnumerator.c.
//

/**
  Provides platform hooks to initialize a PCI controller during enumeration.

  @param  Bridge  The PCI bridge device instance.
  @param  Bus     The bus number of the PCI device.
  @param  Device  The device number of the PCI device.
  @param  Func    The function number of the PCI device.
  @param  Phase   The phase of PCI device enumeration.

  @retval EFI_SUCCESS            The controller was preprocessed.
  @retval EFI_INVALID_PARAMETER  The bridge or phase is invalid.
  @retval EFI_DEVICE_ERROR       Programming failed due to a hardware error.

**/
EFI_STATUS
PreprocessController (
  IN PCI_IO_DEVICE                                 *Bridge,
  IN UINT8                                         Bus,
  IN UINT8                                         Device,
  IN UINT8                                         Func,
  IN EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE  Phase
  )
{
  return EFI_UNSUPPORTED;
}

//
// Normally provided by PciBusDxe/PciDriverOverride.c.
//

/**
  Initializes a PCI Driver Override instance.

  @param  PciIoDevice  PCI device instance.

**/
VOID
InitializePciDriverOverrideInstance (
  IN OUT PCI_IO_DEVICE  *PciIoDevice
  )
{
}

//
// Normally provided by PciBusDxe/PciRomTable.c.
//

/**
  Gets the option ROM image mapping for a PCI device.

  @param  PciIoDevice  PCI device instance.

  @retval TRUE   An image mapping was found.
  @retval FALSE  An image mapping was not found.

**/
BOOLEAN
PciRomGetImageMapping (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
  return FALSE;
}

//
// Normally provided by PciBusDxe/PciOptionRomSupport.c.
//

/**
  Initializes a PCI Load File 2 instance.

  @param  PciIoDevice  PCI I/O device.

**/
VOID
InitializePciLoadFile2 (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
}

/**
  Gets a PCI device's option ROM information.

  @param  PciIoDevice  On input, the PCI device instance. On output, the
                       instance with its option ROM size updated.

  @retval EFI_NOT_FOUND  The PCI device does not have an option ROM.
  @retval EFI_SUCCESS    The PCI device has an option ROM.

**/
EFI_STATUS
GetOpRomInfo (
  IN OUT PCI_IO_DEVICE  *PciIoDevice
  )
{
  return EFI_NOT_FOUND;
}

//
// Normally provided by PciBusDxe/PciPowerManagement.c.
//

/**
  Disables PWE assertion and puts a device in the D0 state if it supports PCI
  power management.

  @param  PciIoDevice  PCI device instance.

  @retval EFI_UNSUPPORTED  The PCI device does not support power management.
  @retval EFI_SUCCESS      PWE was disabled successfully.

**/
EFI_STATUS
ResetPowerManagementFeature (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
  return EFI_UNSUPPORTED;
}

//
// Normally provided by PciBusDxe/PciLib.c.
//

/**
  Retrieves PCI Card device BAR information through the PCI I/O interface.

  @param  PciIoDevice  PCI Card device instance.

**/
VOID
GetBackPcCardBar (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
}

/**
  Removes rejected PCI devices from a specific root bridge.

  @param  RootBridgeHandle  The parent root bridge handle.
  @param  Bridge            The bridge device instance.

**/
VOID
RemoveRejectedPciDevices (
  IN EFI_HANDLE     RootBridgeHandle,
  IN PCI_IO_DEVICE  *Bridge
  )
{
}

/**
  Programs the Resizable BAR register.

  @param  PciIoDevice     A pointer to the PCI_IO_DEVICE.
  @param  ResizableBarOp  PciResizableBarMax sets the BAR to its maximum size;
                          PciResizableBarMin sets it to its minimum size.

  @retval EFI_SUCCESS  The Resizable BAR register was programmed.
  @retval other        An error occurred while programming the register.

**/
EFI_STATUS
PciProgramResizableBar (
  IN PCI_IO_DEVICE                *PciIoDevice,
  IN PCI_RESIZABLE_BAR_OPERATION  ResizableBarOp
  )
{
  return EFI_UNSUPPORTED;
}
