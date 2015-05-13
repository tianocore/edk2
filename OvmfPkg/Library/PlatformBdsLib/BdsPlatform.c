/** @file
  Platform BDS customizations.

  Copyright (c) 2004 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BdsPlatform.h"
#include <Library/QemuBootOrderLib.h>


//
// Global data
//

VOID          *mEfiDevPathNotifyReg;
EFI_EVENT     mEfiDevPathEvent;
VOID          *mEmuVariableEventReg;
EFI_EVENT     mEmuVariableEvent;
BOOLEAN       mDetectVgaOnly;
UINT16        mHostBridgeDevId;

//
// Table of host IRQs matching PCI IRQs A-D
// (for configuring PCI Interrupt Line register)
//
CONST UINT8 PciHostIrqs[] = {
  0x0a, 0x0a, 0x0b, 0x0b
};

//
// Array Size macro
//
#define ARRAY_SIZE(array) (sizeof (array) / sizeof (array[0]))

//
// Type definitions
//

typedef
EFI_STATUS
(EFIAPI *PROTOCOL_INSTANCE_CALLBACK)(
  IN EFI_HANDLE           Handle,
  IN VOID                 *Instance,
  IN VOID                 *Context
  );

/**
  @param[in]  Handle - Handle of PCI device instance
  @param[in]  PciIo - PCI IO protocol instance
  @param[in]  Pci - PCI Header register block
**/
typedef
EFI_STATUS
(EFIAPI *VISIT_PCI_INSTANCE_CALLBACK)(
  IN EFI_HANDLE           Handle,
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN PCI_TYPE00           *Pci
  );


//
// Function prototypes
//

EFI_STATUS
VisitAllInstancesOfProtocol (
  IN EFI_GUID                    *Id,
  IN PROTOCOL_INSTANCE_CALLBACK  CallBackFunction,
  IN VOID                        *Context
  );

EFI_STATUS
VisitAllPciInstancesOfProtocol (
  IN VISIT_PCI_INSTANCE_CALLBACK CallBackFunction
  );

VOID
InstallDevicePathCallback (
  VOID
  );

//
// BDS Platform Functions
//
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
  DEBUG ((EFI_D_INFO, "PlatformBdsInit\n"));
  InstallDevicePathCallback ();
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
  // Make all the PCI_IO protocols on PCI Seg 0 show up
  //
  BdsLibConnectDevicePath (gPlatformRootBridges[0]);

  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &gPlatformRootBridges[0],
                  &RootHandle
                  );
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
  CHAR16                    *DevPathStr;

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

  //
  // Print Device Path
  //
  DevPathStr = DevicePathToStr(DevicePath);
  if (DevPathStr != NULL) {
    DEBUG((
      EFI_D_INFO,
      "BdsPlatform.c+%d: COM%d DevPath: %s\n",
      __LINE__,
      gPnp16550ComPortDeviceNode.UID + 1,
      DevPathStr
      ));
    FreePool(DevPathStr);
  }

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

  //
  // Print Device Path
  //
  DevPathStr = DevicePathToStr(DevicePath);
  if (DevPathStr != NULL) {
    DEBUG((
      EFI_D_INFO,
      "BdsPlatform.c+%d: COM%d DevPath: %s\n",
      __LINE__,
      gPnp16550ComPortDeviceNode.UID + 1,
      DevPathStr
      ));
    FreePool(DevPathStr);
  }

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
VisitAllInstancesOfProtocol (
  IN EFI_GUID                    *Id,
  IN PROTOCOL_INSTANCE_CALLBACK  CallBackFunction,
  IN VOID                        *Context
  )
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  VOID                      *Instance;

  //
  // Start to check all the PciIo to find all possible device
  //
  HandleCount = 0;
  HandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  Id,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], Id, &Instance);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = (*CallBackFunction) (
               HandleBuffer[Index],
               Instance,
               Context
               );
  }

  gBS->FreePool (HandleBuffer);

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
VisitingAPciInstance (
  IN EFI_HANDLE  Handle,
  IN VOID        *Instance,
  IN VOID        *Context
  )
{
  EFI_STATUS                Status;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;

  PciIo = (EFI_PCI_IO_PROTOCOL*) Instance;

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
    return Status;
  }

  return (*(VISIT_PCI_INSTANCE_CALLBACK)(UINTN) Context) (
           Handle,
           PciIo,
           &Pci
           );

}



EFI_STATUS
VisitAllPciInstances (
  IN VISIT_PCI_INSTANCE_CALLBACK CallBackFunction
  )
{
  return VisitAllInstancesOfProtocol (
           &gEfiPciIoProtocolGuid,
           VisitingAPciInstance,
           (VOID*)(UINTN) CallBackFunction
           );
}


/**
  Do platform specific PCI Device check and add them to
  ConOut, ConIn, ErrOut.

  @param[in]  Handle - Handle of PCI device instance
  @param[in]  PciIo - PCI IO protocol instance
  @param[in]  Pci - PCI Header register block

  @retval EFI_SUCCESS - PCI Device check and Console variable update successfully.
  @retval EFI_STATUS - PCI Device check or Console variable update fail.

**/
EFI_STATUS
EFIAPI
DetectAndPreparePlatformPciDevicePath (
  IN EFI_HANDLE           Handle,
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN PCI_TYPE00           *Pci
  )
{
  EFI_STATUS                Status;

  Status = PciIo->Attributes (
    PciIo,
    EfiPciIoAttributeOperationEnable,
    EFI_PCI_DEVICE_ENABLE,
    NULL
    );
  ASSERT_EFI_ERROR (Status);

  if (!mDetectVgaOnly) {
    //
    // Here we decide whether it is LPC Bridge
    //
    if ((IS_PCI_LPC (Pci)) ||
        ((IS_PCI_ISA_PDECODE (Pci)) &&
         (Pci->Hdr.VendorId == 0x8086) &&
         (Pci->Hdr.DeviceId == 0x7000)
        )
       ) {
      //
      // Add IsaKeyboard to ConIn,
      // add IsaSerial to ConOut, ConIn, ErrOut
      //
      DEBUG ((EFI_D_INFO, "Found LPC Bridge device\n"));
      PrepareLpcBridgeDevicePath (Handle);
      return EFI_SUCCESS;
    }
    //
    // Here we decide which Serial device to enable in PCI bus
    //
    if (IS_PCI_16550SERIAL (Pci)) {
      //
      // Add them to ConOut, ConIn, ErrOut.
      //
      DEBUG ((EFI_D_INFO, "Found PCI 16550 SERIAL device\n"));
      PreparePciSerialDevicePath (Handle);
      return EFI_SUCCESS;
    }
  }

  //
  // Here we decide which VGA device to enable in PCI bus
  //
  if (IS_PCI_VGA (Pci)) {
    //
    // Add them to ConOut.
    //
    DEBUG ((EFI_D_INFO, "Found PCI VGA device\n"));
    PreparePciVgaDevicePath (Handle);
    return EFI_SUCCESS;
  }

  return Status;
}


/**
  Do platform specific PCI Device check and add them to ConOut, ConIn, ErrOut

  @param[in]  DetectVgaOnly - Only detect VGA device if it's TRUE.

  @retval EFI_SUCCESS - PCI Device check and Console variable update successfully.
  @retval EFI_STATUS - PCI Device check or Console variable update fail.

**/
EFI_STATUS
DetectAndPreparePlatformPciDevicePaths (
  BOOLEAN DetectVgaOnly
  )
{
  mDetectVgaOnly = DetectVgaOnly;
  return VisitAllPciInstances (DetectAndPreparePlatformPciDevicePath);
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
    DetectAndPreparePlatformPciDevicePaths (FALSE);

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
    DetectAndPreparePlatformPciDevicePaths (TRUE);
  }

  //
  // Connect the all the default console with current cosole variable
  //
  Status = BdsLibConnectAllDefaultConsoles ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


/**
  Configure PCI Interrupt Line register for applicable devices
  Ported from SeaBIOS, src/fw/pciinit.c, *_pci_slot_get_irq()

  @param[in]  Handle - Handle of PCI device instance
  @param[in]  PciIo - PCI IO protocol instance
  @param[in]  PciHdr - PCI Header register block

  @retval EFI_SUCCESS - PCI Interrupt Line register configured successfully.

**/
EFI_STATUS
EFIAPI
SetPciIntLine (
  IN EFI_HANDLE           Handle,
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN PCI_TYPE00           *PciHdr
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevPathNode;
  UINTN                     RootSlot;
  UINTN                     Idx;
  UINT8                     IrqLine;
  EFI_STATUS                Status;

  Status = EFI_SUCCESS;

  if (PciHdr->Device.InterruptPin != 0) {

    DevPathNode = DevicePathFromHandle (Handle);
    ASSERT (DevPathNode != NULL);

    //
    // Compute index into PciHostIrqs[] table by walking
    // the device path and adding up all device numbers
    //
    Status = EFI_NOT_FOUND;
    RootSlot = 0;
    Idx = PciHdr->Device.InterruptPin - 1;
    while (!IsDevicePathEnd (DevPathNode)) {
      if (DevicePathType (DevPathNode) == HARDWARE_DEVICE_PATH &&
          DevicePathSubType (DevPathNode) == HW_PCI_DP) {

        Idx += ((PCI_DEVICE_PATH *)DevPathNode)->Device;

        //
        // Unlike SeaBIOS, which starts climbing from the leaf device
        // up toward the root, we traverse the device path starting at
        // the root moving toward the leaf node.
        // The slot number of the top-level parent bridge is needed for
        // Q35 cases with more than 24 slots on the root bus.
        //
        if (Status != EFI_SUCCESS) {
          Status = EFI_SUCCESS;
          RootSlot = ((PCI_DEVICE_PATH *)DevPathNode)->Device;
        }
      }

      DevPathNode = NextDevicePathNode (DevPathNode);
    }
    if (EFI_ERROR (Status)) {
      return Status;
    }
    if (RootSlot == 0) {
      DEBUG((
        EFI_D_ERROR,
        "%a: PCI host bridge (00:00.0) should have no interrupts!\n",
        __FUNCTION__
        ));
      ASSERT (FALSE);
    }

    //
    // Final PciHostIrqs[] index calculation depends on the platform
    // and should match SeaBIOS src/fw/pciinit.c *_pci_slot_get_irq()
    //
    switch (mHostBridgeDevId) {
      case INTEL_82441_DEVICE_ID:
        Idx -= 1;
        break;
      case INTEL_Q35_MCH_DEVICE_ID:
        //
        // SeaBIOS contains the following comment:
        // "Slots 0-24 rotate slot:pin mapping similar to piix above, but
        //  with a different starting index - see q35-acpi-dsdt.dsl.
        //
        //  Slots 25-31 all use LNKA mapping (or LNKE, but A:D = E:H)"
        //
        if (RootSlot > 24) {
          //
          // in this case, subtract back out RootSlot from Idx
          // (SeaBIOS never adds it to begin with, but that would make our
          //  device path traversal loop above too awkward)
          //
          Idx -= RootSlot;
        }
        break;
      default:
        ASSERT (FALSE); // should never get here
    }
    Idx %= ARRAY_SIZE (PciHostIrqs);
    IrqLine = PciHostIrqs[Idx];

    //
    // Set PCI Interrupt Line register for this device to PciHostIrqs[Idx]
    //
    Status = PciIo->Pci.Write (
               PciIo,
               EfiPciIoWidthUint8,
               PCI_INT_LINE_OFFSET,
               1,
               &IrqLine
               );
  }

  return Status;
}


VOID
PciAcpiInitialization (
  )
{
  UINTN  Pmba;

  //
  // Query Host Bridge DID to determine platform type
  //
  mHostBridgeDevId = PcdGet16 (PcdOvmfHostBridgePciDevId);
  switch (mHostBridgeDevId) {
    case INTEL_82441_DEVICE_ID:
      Pmba = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMBA);
      //
      // 00:01.0 ISA Bridge (PIIX4) LNK routing targets
      //
      PciWrite8 (PCI_LIB_ADDRESS (0, 1, 0, 0x60), 0x0b); // A
      PciWrite8 (PCI_LIB_ADDRESS (0, 1, 0, 0x61), 0x0b); // B
      PciWrite8 (PCI_LIB_ADDRESS (0, 1, 0, 0x62), 0x0a); // C
      PciWrite8 (PCI_LIB_ADDRESS (0, 1, 0, 0x63), 0x0a); // D
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      Pmba = POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE);
      //
      // 00:1f.0 LPC Bridge (Q35) LNK routing targets
      //
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x60), 0x0a); // A
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x61), 0x0a); // B
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x62), 0x0b); // C
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x63), 0x0b); // D
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x68), 0x0a); // E
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x69), 0x0a); // F
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x6a), 0x0b); // G
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x6b), 0x0b); // H
      break;
    default:
      DEBUG ((EFI_D_ERROR, "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __FUNCTION__, mHostBridgeDevId));
      ASSERT (FALSE);
      return;
  }

  //
  // Initialize PCI_INTERRUPT_LINE for applicable present PCI devices
  //
  VisitAllPciInstances (SetPciIntLine);

  //
  // Set ACPI SCI_EN bit in PMCNTRL
  //
  IoOr16 ((PciRead32 (Pmba) & ~BIT0) + 4, BIT0);
}


EFI_STATUS
EFIAPI
ConnectRecursivelyIfPciMassStorage (
  IN EFI_HANDLE           Handle,
  IN EFI_PCI_IO_PROTOCOL  *Instance,
  IN PCI_TYPE00           *PciHeader
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  CHAR16                    *DevPathStr;

  if (IS_CLASS1 (PciHeader, PCI_CLASS_MASS_STORAGE)) {
    DevicePath = NULL;
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiDevicePathProtocolGuid,
                    (VOID*)&DevicePath
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Print Device Path
    //
    DevPathStr = DevicePathToStr (DevicePath);
    if (DevPathStr != NULL) {
      DEBUG((
        EFI_D_INFO,
        "Found Mass Storage device: %s\n",
        DevPathStr
        ));
      FreePool(DevPathStr);
    }

    Status = gBS->ConnectController (Handle, NULL, NULL, TRUE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

  }

  return EFI_SUCCESS;
}


/**
  This notification function is invoked when the
  EMU Variable FVB has been changed.

  @param  Event                 The event that occured
  @param  Context               For EFI compatiblity.  Not used.

**/
VOID
EFIAPI
EmuVariablesUpdatedCallback (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
{
  DEBUG ((EFI_D_INFO, "EmuVariablesUpdatedCallback\n"));
  UpdateNvVarsOnFileSystem ();
}


EFI_STATUS
EFIAPI
VisitingFileSystemInstance (
  IN EFI_HANDLE  Handle,
  IN VOID        *Instance,
  IN VOID        *Context
  )
{
  EFI_STATUS      Status;
  STATIC BOOLEAN  ConnectedToFileSystem = FALSE;

  if (ConnectedToFileSystem) {
    return EFI_ALREADY_STARTED;
  }

  Status = ConnectNvVarsToFileSystem (Handle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ConnectedToFileSystem = TRUE;
  mEmuVariableEvent =
    EfiCreateProtocolNotifyEvent (
      &gEfiDevicePathProtocolGuid,
      TPL_CALLBACK,
      EmuVariablesUpdatedCallback,
      NULL,
      &mEmuVariableEventReg
      );
  PcdSet64 (PcdEmuVariableEvent, (UINT64)(UINTN) mEmuVariableEvent);

  return EFI_SUCCESS;
}


VOID
PlatformBdsRestoreNvVarsFromHardDisk (
  )
{
  VisitAllPciInstances (ConnectRecursivelyIfPciMassStorage);
  VisitAllInstancesOfProtocol (
    &gEfiSimpleFileSystemProtocolGuid,
    VisitingFileSystemInstance,
    NULL
    );

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

  DEBUG ((EFI_D_INFO, "PlatformBdsConnectSequence\n"));

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

  //
  // Just use the simple policy to connect all devices
  //
  BdsLibConnectAll ();

  PciAcpiInitialization ();

  //
  // Clear the logo after all devices are connected.
  //
  gST->ConOut->ClearScreen (gST->ConOut);
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
  DEBUG ((EFI_D_INFO, "PlatformBdsGetDriverOption\n"));
  return;
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

  BaseMemoryTest   - A pointer to BaseMemoryTest()

Returns:

  None.

--*/
{
  EFI_STATUS  Status;

  DEBUG ((EFI_D_INFO, "PlatformBdsDiagnostics\n"));

  //
  // Here we can decide if we need to show
  // the diagnostics screen
  // Notes: this quiet boot code should be remove
  // from the graphic lib
  //
  if (QuietBoot) {
    EnableQuietBoot (PcdGetPtr(PcdLogoFile));
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
  IN OUT LIST_ENTRY                  *DriverOptionList,
  IN OUT LIST_ENTRY                  *BootOptionList,
  IN PROCESS_CAPSULES                ProcessCapsules,
  IN BASEM_MEMORY_TEST               BaseMemoryTest
  )
/*++

Routine Description:

  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.

Arguments:

  DriverOptionList - The header of the driver option link list

  BootOptionList   - The header of the boot option link list

  ProcessCapsules  - A pointer to ProcessCapsules()

  BaseMemoryTest   - A pointer to BaseMemoryTest()

Returns:

  None.

--*/
{
  EFI_STATUS                         Status;
  EFI_BOOT_MODE                      BootMode;

  DEBUG ((EFI_D_INFO, "PlatformBdsPolicyBehavior\n"));

  ConnectRootBridge ();

  if (PcdGetBool (PcdOvmfFlashVariablesEnable)) {
    DEBUG ((EFI_D_INFO, "PlatformBdsPolicyBehavior: not restoring NvVars "
      "from disk since flash variables appear to be supported.\n"));
  } else {
    //
    // Try to restore variables from the hard disk early so
    // they can be used for the other BDS connect operations.
    //
    PlatformBdsRestoreNvVarsFromHardDisk ();
  }

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
  // Memory test and Logo show
  //
  PlatformBdsDiagnostics (IGNORE, TRUE, BaseMemoryTest);

  //
  // Perform some platform specific connect sequence
  //
  PlatformBdsConnectSequence ();

  //
  // Process QEMU's -kernel command line option
  //
  TryRunningQemuKernel ();

  DEBUG ((EFI_D_INFO, "BdsLibConnectAll\n"));
  BdsLibConnectAll ();
  BdsLibEnumerateAllBootOption (BootOptionList);

  SetBootOrderFromQemu (BootOptionList);
  //
  // The BootOrder variable may have changed, reload the in-memory list with
  // it.
  //
  BdsLibBuildOptionFromVar (BootOptionList, L"BootOrder");

  PlatformBdsEnterFrontPage (GetFrontPageTimeoutFromQemu(), TRUE);
}

VOID
EFIAPI
PlatformBdsBootSuccess (
  IN  BDS_COMMON_OPTION   *Option
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

  DEBUG ((EFI_D_INFO, "PlatformBdsBootSuccess\n"));
  //
  // If Boot returned with EFI_SUCCESS and there is not in the boot device
  // select loop then we need to pop up a UI and wait for user input.
  //
  TmpStr = Option->StatusString;
  if (TmpStr != NULL) {
    BdsLibOutputStrings (gST->ConOut, TmpStr, Option->Description, L"\n\r", NULL);
    FreePool (TmpStr);
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

  DEBUG ((EFI_D_INFO, "PlatformBdsBootFail\n"));

  //
  // If Boot returned with failed status then we need to pop up a UI and wait
  // for user input.
  //
  TmpStr = Option->StatusString;
  if (TmpStr != NULL) {
    BdsLibOutputStrings (gST->ConOut, TmpStr, Option->Description, L"\n\r", NULL);
    FreePool (TmpStr);
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
  DEBUG ((EFI_D_INFO, "PlatformBdsNoConsoleAction\n"));
  return EFI_SUCCESS;
}

VOID
EFIAPI
PlatformBdsLockNonUpdatableFlash (
  VOID
  )
{
  DEBUG ((EFI_D_INFO, "PlatformBdsLockNonUpdatableFlash\n"));
  return;
}


/**
  This notification function is invoked when an instance of the
  EFI_DEVICE_PATH_PROTOCOL is produced.

  @param  Event                 The event that occured
  @param  Context               For EFI compatiblity.  Not used.

**/
VOID
EFIAPI
NotifyDevPath (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
{
  EFI_HANDLE                            Handle;
  EFI_STATUS                            Status;
  UINTN                                 BufferSize;
  EFI_DEVICE_PATH_PROTOCOL             *DevPathNode;
  ATAPI_DEVICE_PATH                    *Atapi;

  //
  // Examine all new handles
  //
  for (;;) {
    //
    // Get the next handle
    //
    BufferSize = sizeof (Handle);
    Status = gBS->LocateHandle (
              ByRegisterNotify,
              NULL,
              mEfiDevPathNotifyReg,
              &BufferSize,
              &Handle
              );

    //
    // If not found, we're done
    //
    if (EFI_NOT_FOUND == Status) {
      break;
    }

    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Get the DevicePath protocol on that handle
    //
    Status = gBS->HandleProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID **)&DevPathNode);
    ASSERT_EFI_ERROR (Status);

    while (!IsDevicePathEnd (DevPathNode)) {
      //
      // Find the handler to dump this device path node
      //
      if (
           (DevicePathType(DevPathNode) == MESSAGING_DEVICE_PATH) &&
           (DevicePathSubType(DevPathNode) == MSG_ATAPI_DP)
         ) {
        Atapi = (ATAPI_DEVICE_PATH*) DevPathNode;
        PciOr16 (
          PCI_LIB_ADDRESS (
            0,
            1,
            1,
            (Atapi->PrimarySecondary == 1) ? 0x42: 0x40
            ),
          BIT15
          );
      }

      //
      // Next device path node
      //
      DevPathNode = NextDevicePathNode (DevPathNode);
    }
  }

  return;
}


VOID
InstallDevicePathCallback (
  VOID
  )
{
  DEBUG ((EFI_D_INFO, "Registered NotifyDevPath Event\n"));
  mEfiDevPathEvent = EfiCreateProtocolNotifyEvent (
                          &gEfiDevicePathProtocolGuid,
                          TPL_CALLBACK,
                          NotifyDevPath,
                          NULL,
                          &mEfiDevPathNotifyReg
                          );
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

