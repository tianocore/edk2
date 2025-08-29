/** @file
  Platform BDS customizations.

  Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BdsPlatform.h"
#include <Guid/RootBridgesConnectedEventGroup.h>
#include <Guid/SerialPortLibVendor.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/VirtioDevice.h>
#include <Library/PlatformBmPrintScLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>
#include <Library/XenPlatformLib.h>

#include <Library/QemuFwCfgSimpleParserLib.h>
#include <Library/PlatformBootManagerCommonLib.h>

//
// Global data
//

VOID       *mEfiDevPathNotifyReg;
EFI_EVENT  mEfiDevPathEvent;
VOID       *mEmuVariableEventReg;
EFI_EVENT  mEmuVariableEvent;
UINT16     mHostBridgeDevId;

//
// Table of host IRQs matching PCI IRQs A-D
// (for configuring PCI Interrupt Line register)
//
CONST UINT8  PciHostIrqs[] = {
  0x0a, // LNKA, LNKE
  0x0a, // LNKB, LNKF
  0x0b, // LNKC, LNKG
  0x0b  // LNKD, LNKH
};

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
  IN VISIT_PCI_INSTANCE_CALLBACK  CallBackFunction
  );

VOID
InstallDevicePathCallback (
  VOID
  );

VOID
RestrictBootOptionsToFirmware (
  VOID
  )
{
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         BootOptionCount;
  UINTN                         Index;

  BootOptions = EfiBootManagerGetLoadOptions (
                  &BootOptionCount,
                  LoadOptionTypeBoot
                  );

  for (Index = 0; Index < BootOptionCount; ++Index) {
    EFI_DEVICE_PATH_PROTOCOL  *Node1;

    //
    // If the device path starts with Fv(...),
    // then keep the boot option.
    //
    Node1 = BootOptions[Index].FilePath;
    if (((DevicePathType (Node1) == MEDIA_DEVICE_PATH) &&
         (DevicePathSubType (Node1) == MEDIA_PIWG_FW_VOL_DP)))
    {
      continue;
    }

    //
    // Delete the boot option.
    //
    EfiBootManagerDeleteLoadOptionVariable (
      BootOptions[Index].OptionNumber,
      LoadOptionTypeBoot
      );
  }

  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
}

VOID
PlatformRegisterOptionsAndKeys (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_INPUT_KEY                 Enter;
  EFI_INPUT_KEY                 F2;
  EFI_INPUT_KEY                 Esc;
  EFI_BOOT_MANAGER_LOAD_OPTION  BootOption;

  //
  // Register ENTER as CONTINUE key
  //
  Enter.ScanCode    = SCAN_NULL;
  Enter.UnicodeChar = CHAR_CARRIAGE_RETURN;
  Status            = EfiBootManagerRegisterContinueKeyOption (0, &Enter, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // Map F2 to Boot Manager Menu
  //
  F2.ScanCode     = SCAN_F2;
  F2.UnicodeChar  = CHAR_NULL;
  Esc.ScanCode    = SCAN_ESC;
  Esc.UnicodeChar = CHAR_NULL;
  Status          = EfiBootManagerGetBootManagerMenu (&BootOption);
  ASSERT_EFI_ERROR (Status);
  Status = EfiBootManagerAddKeyOptionVariable (
             NULL,
             (UINT16)BootOption.OptionNumber,
             0,
             &F2,
             NULL
             );
  ASSERT (Status == EFI_SUCCESS || Status == EFI_ALREADY_STARTED);
  Status = EfiBootManagerAddKeyOptionVariable (
             NULL,
             (UINT16)BootOption.OptionNumber,
             0,
             &Esc,
             NULL
             );
  ASSERT (Status == EFI_SUCCESS || Status == EFI_ALREADY_STARTED);
}

EFI_STATUS
EFIAPI
ConnectRootBridge (
  IN EFI_HANDLE  RootBridgeHandle,
  IN VOID        *Instance,
  IN VOID        *Context
  );

STATIC
EFI_STATUS
EFIAPI
ConnectVirtioPciRng (
  IN EFI_HANDLE  Handle,
  IN VOID        *Instance,
  IN VOID        *Context
  );

STATIC
VOID
SaveS3BootScript (
  VOID
  );

//
// BDS Platform Functions
//

/**
  Do the platform init, can be customized by OEM/IBV

  Possible things that can be done in PlatformBootManagerBeforeConsole:

  > Update console variable: 1. include hot-plug devices;
  >                          2. Clear ConIn and add SOL for AMT
  > Register new Driver#### or Boot####
  > Register new Key####: e.g.: F12
  > Signal ReadyToLock event
  > Authentication action: 1. connect Auth devices;
  >                        2. Identify auto logon user.
**/
VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  )
{
  EFI_HANDLE     Handle;
  EFI_STATUS     Status;
  UINT16         FrontPageTimeout;
  RETURN_STATUS  PcdStatus;
  BOOLEAN        FirmwareSetupEnabled;

  DEBUG ((DEBUG_INFO, "PlatformBootManagerBeforeConsole\n"));
  InstallDevicePathCallback ();

  VisitAllInstancesOfProtocol (
    &gEfiPciRootBridgeIoProtocolGuid,
    ConnectRootBridge,
    NULL
    );

  //
  // Signal the ACPI platform driver that it can download QEMU ACPI tables.
  //
  EfiEventGroupSignal (&gRootBridgesConnectedEventGroupGuid);

  //
  // We can't signal End-of-Dxe earlier than this. Namely, End-of-Dxe triggers
  // the preparation of S3 system information. That logic has a hard dependency
  // on the presence of the FACS ACPI table. Since our ACPI tables are only
  // installed after PCI enumeration completes, we must not trigger the S3 save
  // earlier, hence we can't signal End-of-Dxe earlier.
  //
  EfiEventGroupSignal (&gEfiEndOfDxeEventGroupGuid);

  if (PcdGetBool (PcdAcpiS3Enable)) {
    //
    // Save the boot script too. Note that this will require us to emit the
    // DxeSmmReadyToLock event just below, which in turn locks down SMM.
    //
    SaveS3BootScript ();
  }

  //
  // We need to connect all trusted consoles for TCG PP. Here we treat all
  // consoles in OVMF to be trusted consoles.
  //
  // Cloud Hypervisor doesn't emulate any LPC bridge, which is why it must
  // rely on the serial I/O port to be connected as a console. It reuses the
  // definition from Xen as it is very generic.
  //
  PlatformInitializeConsole (
    (XenDetected () || PcdGet16 (PcdOvmfHostBridgePciDevId) == CLOUDHV_DEVICE_ID) ? gXenPlatformConsole : gPlatformConsole
    );

  //
  // Process TPM PPI request; this may require keyboard input
  //
  Tcg2PhysicalPresenceLibProcessRequest (NULL);

  //
  // Prevent further changes to LockBoxes or SMRAM.
  // Any TPM 2 Physical Presence Interface opcode must be handled before.
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiDxeSmmReadyToLockProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Dispatch deferred images after EndOfDxe event and ReadyToLock
  // installation.
  //
  EfiBootManagerDispatchDeferredImages ();

  //
  // GPU passthrough only allows Console enablement after ROM image load
  //
  PlatformInitializeConsole (
    XenDetected () ? gXenPlatformConsole : gPlatformConsole
    );

  FrontPageTimeout = GetFrontPageTimeoutFromQemu ();
  PcdStatus        = PcdSet16S (PcdPlatformBootTimeOut, FrontPageTimeout);
  ASSERT_RETURN_ERROR (PcdStatus);
  //
  // Reflect the PCD in the standard Timeout variable.
  //
  Status = gRT->SetVariable (
                  EFI_TIME_OUT_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  (EFI_VARIABLE_NON_VOLATILE |
                   EFI_VARIABLE_BOOTSERVICE_ACCESS |
                   EFI_VARIABLE_RUNTIME_ACCESS),
                  sizeof FrontPageTimeout,
                  &FrontPageTimeout
                  );
  DEBUG ((
    EFI_ERROR (Status) ? DEBUG_ERROR : DEBUG_VERBOSE,
    "%a: SetVariable(%s, %u): %r\n",
    __func__,
    EFI_TIME_OUT_VARIABLE_NAME,
    FrontPageTimeout,
    Status
    ));

  Status = QemuFwCfgParseBool (
             "opt/org.tianocore/FirmwareSetupSupport",
             &FirmwareSetupEnabled
             );

  if (RETURN_ERROR (Status)) {
    FirmwareSetupEnabled = TRUE;
  }

  PlatformRegisterFvBootOption (
    &gUiAppFileGuid,
    L"EFI Firmware Setup",
    LOAD_OPTION_ACTIVE | LOAD_OPTION_CATEGORY_APP,
    FirmwareSetupEnabled
    );

  if (!FeaturePcdGet (PcdBootRestrictToFirmware)) {
    PlatformRegisterOptionsAndKeys ();
  }

  //
  // Install both VIRTIO_DEVICE_PROTOCOL and (dependent) EFI_RNG_PROTOCOL
  // instances on Virtio PCI RNG devices.
  //
  VisitAllInstancesOfProtocol (
    &gEfiPciIoProtocolGuid,
    ConnectVirtioPciRng,
    NULL
    );
}

EFI_STATUS
EFIAPI
ConnectRootBridge (
  IN EFI_HANDLE  RootBridgeHandle,
  IN VOID        *Instance,
  IN VOID        *Context
  )
{
  EFI_STATUS  Status;

  //
  // Make the PCI bus driver connect the root bridge, non-recursively. This
  // will produce a number of child handles with PciIo on them.
  //
  Status = gBS->ConnectController (
                  RootBridgeHandle, // ControllerHandle
                  NULL,             // DriverImageHandle
                  NULL,             // RemainingDevicePath -- produce all
                                    //   children
                  FALSE             // Recursive
                  );
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
ConnectVirtioPciRng (
  IN EFI_HANDLE  Handle,
  IN VOID        *Instance,
  IN VOID        *Context
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  EFI_STATUS           Status;
  UINT16               VendorId;
  UINT16               DeviceId;
  UINT8                RevisionId;
  BOOLEAN              Virtio10;
  UINT16               SubsystemId;

  PciIo = Instance;

  //
  // Read and check VendorId.
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint16,
                        PCI_VENDOR_ID_OFFSET,
                        1,
                        &VendorId
                        );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  if (VendorId != VIRTIO_VENDOR_ID) {
    return EFI_SUCCESS;
  }

  //
  // Read DeviceId and RevisionId.
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint16,
                        PCI_DEVICE_ID_OFFSET,
                        1,
                        &DeviceId
                        );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_REVISION_ID_OFFSET,
                        1,
                        &RevisionId
                        );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // From DeviceId and RevisionId, determine whether the device is a
  // modern-only Virtio 1.0 device. In case of Virtio 1.0, DeviceId can
  // immediately be restricted to VIRTIO_SUBSYSTEM_ENTROPY_SOURCE, and
  // SubsystemId will only play a sanity-check role. Otherwise, DeviceId can
  // only be sanity-checked, and SubsystemId will decide.
  //
  if ((DeviceId == 0x1040 + VIRTIO_SUBSYSTEM_ENTROPY_SOURCE) &&
      (RevisionId >= 0x01))
  {
    Virtio10 = TRUE;
  } else if ((DeviceId >= 0x1000) && (DeviceId <= 0x103F) && (RevisionId == 0x00)) {
    Virtio10 = FALSE;
  } else {
    return EFI_SUCCESS;
  }

  //
  // Read and check SubsystemId as dictated by Virtio10.
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint16,
                        PCI_SUBSYSTEM_ID_OFFSET,
                        1,
                        &SubsystemId
                        );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  if ((Virtio10 && (SubsystemId >= 0x40)) ||
      (!Virtio10 && (SubsystemId == VIRTIO_SUBSYSTEM_ENTROPY_SOURCE)))
  {
    Status = gBS->ConnectController (
                    Handle, // ControllerHandle
                    NULL,   // DriverImageHandle -- connect all drivers
                    NULL,   // RemainingDevicePath -- produce all child handles
                    FALSE   // Recursive -- don't follow child handles
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    gDS->Dispatch ();
  }

  return EFI_SUCCESS;

Error:
  DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, Status));
  return Status;
}

/**
  Add IsaKeyboard to ConIn; add IsaSerial to ConOut, ConIn, ErrOut.

  @param[in] DeviceHandle  Handle of the LPC Bridge device.

  @retval EFI_SUCCESS  Console devices on the LPC bridge have been added to
                       ConOut, ConIn, and ErrOut.

  @return              Error codes, due to EFI_DEVICE_PATH_PROTOCOL missing
                       from DeviceHandle.
**/
EFI_STATUS
PrepareLpcBridgeDevicePath (
  IN EFI_HANDLE  DeviceHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  CHAR16                    *DevPathStr;

  DevicePath = NULL;
  Status     = gBS->HandleProtocol (
                      DeviceHandle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID *)&DevicePath
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TempDevicePath = DevicePath;

  //
  // Register Keyboard
  //
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gPnpPs2KeyboardDeviceNode
                 );

  EfiBootManagerUpdateConsoleVariable (ConIn, DevicePath, NULL);

  //
  // Register COM1
  //
  DevicePath                     = TempDevicePath;
  gPnp16550ComPortDeviceNode.UID = 0;

  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gPnp16550ComPortDeviceNode
                 );
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gUartDeviceNode
                 );
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gTerminalTypeDeviceNode
                 );

  //
  // Print Device Path
  //
  DevPathStr = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
  if (DevPathStr != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "BdsPlatform.c+%d: COM%d DevPath: %s\n",
      DEBUG_LINE_NUMBER,
      gPnp16550ComPortDeviceNode.UID + 1,
      DevPathStr
      ));
    FreePool (DevPathStr);
  }

  EfiBootManagerUpdateConsoleVariable (ConOut, DevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ConIn, DevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ErrOut, DevicePath, NULL);

  //
  // Register COM2
  //
  DevicePath                     = TempDevicePath;
  gPnp16550ComPortDeviceNode.UID = 1;

  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gPnp16550ComPortDeviceNode
                 );
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gUartDeviceNode
                 );
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gTerminalTypeDeviceNode
                 );

  //
  // Print Device Path
  //
  DevPathStr = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
  if (DevPathStr != NULL) {
    DEBUG ((
      DEBUG_INFO,
      "BdsPlatform.c+%d: COM%d DevPath: %s\n",
      DEBUG_LINE_NUMBER,
      gPnp16550ComPortDeviceNode.UID + 1,
      DevPathStr
      ));
    FreePool (DevPathStr);
  }

  EfiBootManagerUpdateConsoleVariable (ConOut, DevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ConIn, DevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ErrOut, DevicePath, NULL);

  return EFI_SUCCESS;
}

typedef struct {
  VENDOR_DEVICE_PATH          Guid;
  EFI_DEVICE_PATH_PROTOCOL    End;
} SERIAL_DEVICE_PATH;

SERIAL_DEVICE_PATH  serialDevicePath = {
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP,                   { sizeof (VENDOR_DEVICE_PATH),       0 }
    },
    EDKII_SERIAL_PORT_LIB_VENDOR_GUID
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
  }
};

VOID
PrepareMicrovmDevicePath (
  VOID
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINT16                    HostBridgeDevId;

  HostBridgeDevId = PcdGet16 (PcdOvmfHostBridgePciDevId);
  if (HostBridgeDevId != MICROVM_PSEUDO_DEVICE_ID) {
    return;
  }

  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)&serialDevicePath;
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gUartDeviceNode
                 );
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gTerminalTypeDeviceNode
                 );

  EfiBootManagerUpdateConsoleVariable (ConOut, DevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ConIn, DevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ErrOut, DevicePath, NULL);
}

EFI_STATUS
GetGopDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *PciDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL  **GopDevicePath
  )
{
  UINTN                     Index;
  EFI_STATUS                Status;
  EFI_HANDLE                PciDeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempPciDevicePath;
  UINTN                     GopHandleCount;
  EFI_HANDLE                *GopHandleBuffer;

  if ((PciDevicePath == NULL) || (GopDevicePath == NULL)) {
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
  // Try to connect this handle, so that GOP driver could start on this
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
      Status = gBS->HandleProtocol (
                      GopHandleBuffer[Index],
                      &gEfiDevicePathProtocolGuid,
                      (VOID *)&TempDevicePath
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      if (CompareMem (
            PciDevicePath,
            TempDevicePath,
            GetDevicePathSize (PciDevicePath) - END_DEVICE_PATH_LENGTH
            ) == 0)
      {
        //
        // In current implementation, we only enable one of the child handles
        // as console device, i.e. sotre one of the child handle's device
        // path to variable "ConOut"
        // In future, we could select all child handles to be console device
        //

        *GopDevicePath = TempDevicePath;

        //
        // Delete the PCI device's path that added by
        // GetPlugInPciVgaDevicePath(). Add the integrity GOP device path.
        //
        EfiBootManagerUpdateConsoleVariable (ConOutDev, NULL, PciDevicePath);
        EfiBootManagerUpdateConsoleVariable (ConOutDev, TempDevicePath, NULL);
      }
    }

    gBS->FreePool (GopHandleBuffer);
  }

  return EFI_SUCCESS;
}

/**
  Add PCI display to ConOut.

  @param[in] DeviceHandle  Handle of the PCI display device.

  @retval EFI_SUCCESS  The PCI display device has been added to ConOut.

  @return              Error codes, due to EFI_DEVICE_PATH_PROTOCOL missing
                       from DeviceHandle.
**/
EFI_STATUS
PreparePciDisplayDevicePath (
  IN EFI_HANDLE  DeviceHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *GopDevicePath;

  DevicePath    = NULL;
  GopDevicePath = NULL;
  Status        = gBS->HandleProtocol (
                         DeviceHandle,
                         &gEfiDevicePathProtocolGuid,
                         (VOID *)&DevicePath
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  GetGopDevicePath (DevicePath, &GopDevicePath);
  DevicePath = GopDevicePath;

  EfiBootManagerUpdateConsoleVariable (ConOut, DevicePath, NULL);

  return EFI_SUCCESS;
}

/**
  Add PCI Serial to ConOut, ConIn, ErrOut.

  @param[in] DeviceHandle  Handle of the PCI serial device.

  @retval EFI_SUCCESS  The PCI serial device has been added to ConOut, ConIn,
                       ErrOut.

  @return              Error codes, due to EFI_DEVICE_PATH_PROTOCOL missing
                       from DeviceHandle.
**/
EFI_STATUS
PreparePciSerialDevicePath (
  IN EFI_HANDLE  DeviceHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath = NULL;
  Status     = gBS->HandleProtocol (
                      DeviceHandle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID *)&DevicePath
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gUartDeviceNode
                 );
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gTerminalTypeDeviceNode
                 );

  EfiBootManagerUpdateConsoleVariable (ConOut, DevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ConIn, DevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ErrOut, DevicePath, NULL);

  return EFI_SUCCESS;
}

EFI_STATUS
PrepareVirtioSerialDevicePath (
  IN EFI_HANDLE  DeviceHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath = NULL;
  Status     = gBS->HandleProtocol (
                      DeviceHandle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID *)&DevicePath
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gPnp16550ComPortDeviceNode.UID = 0;
  DevicePath                     = AppendDevicePathNode (
                                     DevicePath,
                                     (EFI_DEVICE_PATH_PROTOCOL *)&gPnp16550ComPortDeviceNode
                                     );
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gUartDeviceNode
                 );
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&gTerminalTypeDeviceNode
                 );

  EfiBootManagerUpdateConsoleVariable (ConOut, DevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ConIn, DevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ErrOut, DevicePath, NULL);

  return EFI_SUCCESS;
}

EFI_STATUS
PrepareVirtioKeyboardDevicePath (
  IN EFI_HANDLE  DeviceHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath = NULL;
  Status     = gBS->HandleProtocol (
                      DeviceHandle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID *)&DevicePath
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  EfiBootManagerUpdateConsoleVariable (ConIn, DevicePath, NULL);
  return EFI_SUCCESS;
}

EFI_STATUS
VisitAllInstancesOfProtocol (
  IN EFI_GUID                    *Id,
  IN PROTOCOL_INSTANCE_CALLBACK  CallBackFunction,
  IN VOID                        *Context
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;
  VOID        *Instance;

  //
  // Start to check all the PciIo to find all possible device
  //
  HandleCount  = 0;
  HandleBuffer = NULL;
  Status       = gBS->LocateHandleBuffer (
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

    Status = (*CallBackFunction)(
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
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           Pci;

  PciIo = (EFI_PCI_IO_PROTOCOL *)Instance;

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

  return (*(VISIT_PCI_INSTANCE_CALLBACK)(UINTN)Context)(
  Handle,
  PciIo,
  &Pci
  );
}

EFI_STATUS
VisitAllPciInstances (
  IN VISIT_PCI_INSTANCE_CALLBACK  CallBackFunction
  )
{
  return VisitAllInstancesOfProtocol (
           &gEfiPciIoProtocolGuid,
           VisitingAPciInstance,
           (VOID *)(UINTN)CallBackFunction
           );
}

/**
  Do platform specific PCI Device check and add them to
  ConOut, ConIn, ErrOut.

  @param[in]  Handle - Handle of PCI device instance
  @param[in]  PciIo - PCI IO protocol instance
  @param[in]  Pci - PCI Header register block

  @retval EFI_SUCCESS - PCI Device check and Console variable update
                        successfully.
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
  EFI_STATUS  Status;

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationEnable,
                    EFI_PCI_DEVICE_ENABLE,
                    NULL
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Here we decide whether it is LPC Bridge
  //
  if ((IS_PCI_LPC (Pci)) ||
      ((IS_PCI_ISA_PDECODE (Pci)) &&
       (Pci->Hdr.VendorId == 0x8086) &&
       (Pci->Hdr.DeviceId == 0x7000)
      )
      )
  {
    //
    // Add IsaKeyboard to ConIn,
    // add IsaSerial to ConOut, ConIn, ErrOut
    //
    DEBUG ((DEBUG_INFO, "Found LPC Bridge device\n"));
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
    DEBUG ((DEBUG_INFO, "Found PCI 16550 SERIAL device\n"));
    PreparePciSerialDevicePath (Handle);
    return EFI_SUCCESS;
  }

  //
  // Here we decide which display device to enable in PCI bus
  //
  if (IS_PCI_DISPLAY (Pci)) {
    //
    // Add them to ConOut.
    //
    DEBUG ((DEBUG_INFO, "Found PCI display device\n"));
    PreparePciDisplayDevicePath (Handle);
    return EFI_SUCCESS;
  }

  if (((Pci->Hdr.VendorId == 0x1af4) && (Pci->Hdr.DeviceId == 0x1003)) ||
      ((Pci->Hdr.VendorId == 0x1af4) && (Pci->Hdr.DeviceId == 0x1043)))
  {
    DEBUG ((DEBUG_INFO, "Found virtio serial device\n"));
    PrepareVirtioSerialDevicePath (Handle);
    return EFI_SUCCESS;
  }

  if ((Pci->Hdr.VendorId == 0x1af4) && (Pci->Hdr.DeviceId == 0x1052)) {
    DEBUG ((DEBUG_INFO, "Found virtio keyboard device\n"));
    PrepareVirtioKeyboardDevicePath (Handle);
    return EFI_SUCCESS;
  }

  return Status;
}

EFI_STATUS
EFIAPI
DetectAndPreparePlatformVirtioDevicePath (
  IN EFI_HANDLE  Handle,
  IN VOID        *Instance,
  IN VOID        *Context
  )
{
  VIRTIO_DEVICE_PROTOCOL  *VirtIo = (VIRTIO_DEVICE_PROTOCOL *)Instance;

  DEBUG ((DEBUG_INFO, "%a:%d: id %d\n", __func__, __LINE__, VirtIo->SubSystemDeviceId));

  switch (VirtIo->SubSystemDeviceId) {
    case VIRTIO_SUBSYSTEM_CONSOLE:
      PrepareVirtioSerialDevicePath (Handle);
      break;
    default:
      /* nothing */
      break;
  }

  return EFI_SUCCESS;
}

/**
  Connect the predefined platform default console device.

  Always try to find and enable PCI display devices.

  @param[in] PlatformConsole  Predefined platform default console device array.
**/
VOID
PlatformInitializeConsole (
  IN PLATFORM_CONSOLE_CONNECT_ENTRY  *PlatformConsole
  )
{
  UINTN  Index;

  //
  // Do platform specific PCI Device check and add them to ConOut, ConIn,
  // ErrOut
  //
  VisitAllPciInstances (DetectAndPreparePlatformPciDevicePath);

  VisitAllInstancesOfProtocol (
    &gVirtioDeviceProtocolGuid,
    DetectAndPreparePlatformVirtioDevicePath,
    NULL
    );

  PrepareMicrovmDevicePath ();

  //
  // Have chance to connect the platform default console,
  // the platform default console is the minimum device group
  // the platform should support
  //
  for (Index = 0; PlatformConsole[Index].DevicePath != NULL; ++Index) {
    //
    // Update the console variable with the connect type
    //
    if ((PlatformConsole[Index].ConnectType & CONSOLE_IN) == CONSOLE_IN) {
      EfiBootManagerUpdateConsoleVariable (
        ConIn,
        PlatformConsole[Index].DevicePath,
        NULL
        );
    }

    if ((PlatformConsole[Index].ConnectType & CONSOLE_OUT) == CONSOLE_OUT) {
      EfiBootManagerUpdateConsoleVariable (
        ConOut,
        PlatformConsole[Index].DevicePath,
        NULL
        );
    }

    if ((PlatformConsole[Index].ConnectType & STD_ERROR) == STD_ERROR) {
      EfiBootManagerUpdateConsoleVariable (
        ErrOut,
        PlatformConsole[Index].DevicePath,
        NULL
        );
    }
  }
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
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  UINTN                     RootSlot;
  UINTN                     Idx;
  UINT8                     IrqLine;
  EFI_STATUS                Status;
  UINT32                    RootBusNumber;

  Status = EFI_SUCCESS;

  if (PciHdr->Device.InterruptPin != 0) {
    DevPathNode = DevicePathFromHandle (Handle);
    ASSERT (DevPathNode != NULL);
    DevPath = DevPathNode;

    RootBusNumber = 0;
    if ((DevicePathType (DevPathNode) == ACPI_DEVICE_PATH) &&
        (DevicePathSubType (DevPathNode) == ACPI_DP) &&
        (((ACPI_HID_DEVICE_PATH *)DevPathNode)->HID == EISA_PNP_ID (0x0A03)))
    {
      RootBusNumber = ((ACPI_HID_DEVICE_PATH *)DevPathNode)->UID;
    }

    //
    // Compute index into PciHostIrqs[] table by walking
    // the device path and adding up all device numbers
    //
    Status   = EFI_NOT_FOUND;
    RootSlot = 0;
    Idx      = PciHdr->Device.InterruptPin - 1;
    while (!IsDevicePathEnd (DevPathNode)) {
      if ((DevicePathType (DevPathNode) == HARDWARE_DEVICE_PATH) &&
          (DevicePathSubType (DevPathNode) == HW_PCI_DP))
      {
        Idx += ((PCI_DEVICE_PATH *)DevPathNode)->Device;

        //
        // Unlike SeaBIOS, which starts climbing from the leaf device
        // up toward the root, we traverse the device path starting at
        // the root moving toward the leaf node.
        // The slot number of the top-level parent bridge is needed for
        // Q35 cases with more than 24 slots on the root bus.
        //
        if (Status != EFI_SUCCESS) {
          Status   = EFI_SUCCESS;
          RootSlot = ((PCI_DEVICE_PATH *)DevPathNode)->Device;
        }
      }

      DevPathNode = NextDevicePathNode (DevPathNode);
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((RootBusNumber == 0) && (RootSlot == 0)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: PCI host bridge (00:00.0) should have no interrupts!\n",
        __func__
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

    Idx    %= ARRAY_SIZE (PciHostIrqs);
    IrqLine = PciHostIrqs[Idx];

    DEBUG_CODE_BEGIN ();
    {
      CHAR16         *DevPathString;
      STATIC CHAR16  Fallback[] = L"<failed to convert>";
      UINTN          Segment, Bus, Device, Function;

      DevPathString = ConvertDevicePathToText (DevPath, FALSE, FALSE);
      if (DevPathString == NULL) {
        DevPathString = Fallback;
      }

      Status = PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);
      ASSERT_EFI_ERROR (Status);

      DEBUG ((
        DEBUG_VERBOSE,
        "%a: [%02x:%02x.%x] %s -> 0x%02x\n",
        __func__,
        (UINT32)Bus,
        (UINT32)Device,
        (UINT32)Function,
        DevPathString,
        IrqLine
        ));

      if (DevPathString != Fallback) {
        FreePool (DevPathString);
      }
    }
    DEBUG_CODE_END ();

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
      PciWrite8 (PCI_LIB_ADDRESS (0, 1, 0, 0x60), PciHostIrqs[0]); // A
      PciWrite8 (PCI_LIB_ADDRESS (0, 1, 0, 0x61), PciHostIrqs[1]); // B
      PciWrite8 (PCI_LIB_ADDRESS (0, 1, 0, 0x62), PciHostIrqs[2]); // C
      PciWrite8 (PCI_LIB_ADDRESS (0, 1, 0, 0x63), PciHostIrqs[3]); // D
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      Pmba = POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE);
      //
      // 00:1f.0 LPC Bridge (Q35) LNK routing targets
      //
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x60), PciHostIrqs[0]); // A
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x61), PciHostIrqs[1]); // B
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x62), PciHostIrqs[2]); // C
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x63), PciHostIrqs[3]); // D
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x68), PciHostIrqs[0]); // E
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x69), PciHostIrqs[1]); // F
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x6a), PciHostIrqs[2]); // G
      PciWrite8 (PCI_LIB_ADDRESS (0, 0x1f, 0, 0x6b), PciHostIrqs[3]); // H
      break;
    case MICROVM_PSEUDO_DEVICE_ID:
    case CLOUDHV_DEVICE_ID:
      return;
    default:
      if (XenDetected ()) {
        //
        // There is no PCI bus in this case.
        //
        return;
      }

      DEBUG ((
        DEBUG_ERROR,
        "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __func__,
        mHostBridgeDevId
        ));
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

  //
  // Recognize PCI Mass Storage, and Xen PCI devices
  //
  if (IS_CLASS1 (PciHeader, PCI_CLASS_MASS_STORAGE) ||
      (XenDetected () && IS_CLASS2 (PciHeader, 0xFF, 0x80)))
  {
    DevicePath = NULL;
    Status     = gBS->HandleProtocol (
                        Handle,
                        &gEfiDevicePathProtocolGuid,
                        (VOID *)&DevicePath
                        );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Print Device Path
    //
    DevPathStr = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
    if (DevPathStr != NULL) {
      DEBUG ((
        DEBUG_INFO,
        "Found %s device: %s\n",
        (IS_CLASS1 (PciHeader, PCI_CLASS_MASS_STORAGE) ?
         L"Mass Storage" :
         L"Xen"
        ),
        DevPathStr
        ));
      FreePool (DevPathStr);
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

  @param  Event                 The event that occurred
  @param  Context               For EFI compatibility.  Not used.

**/
VOID
EFIAPI
EmuVariablesUpdatedCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  DEBUG ((DEBUG_INFO, "EmuVariablesUpdatedCallback\n"));
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
  RETURN_STATUS   PcdStatus;

  if (ConnectedToFileSystem) {
    return EFI_ALREADY_STARTED;
  }

  Status = ConnectNvVarsToFileSystem (Handle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ConnectedToFileSystem = TRUE;
  mEmuVariableEvent     =
    EfiCreateProtocolNotifyEvent (
      &gEfiDevicePathProtocolGuid,
      TPL_CALLBACK,
      EmuVariablesUpdatedCallback,
      NULL,
      &mEmuVariableEventReg
      );
  PcdStatus = PcdSet64S (
                PcdEmuVariableEvent,
                (UINT64)(UINTN)mEmuVariableEvent
                );
  ASSERT_RETURN_ERROR (PcdStatus);

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

/**
  Connect with predefined platform connect sequence.

  The OEM/IBV can customize with their own connect sequence.
**/
VOID
PlatformBdsConnectSequence (
  VOID
  )
{
  UINTN          Index;
  RETURN_STATUS  Status;

  DEBUG ((DEBUG_INFO, "PlatformBdsConnectSequence\n"));

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
    EfiBootManagerConnectDevicePath (gPlatformConnectSequence[Index], NULL);
    Index++;
  }

  Status = ConnectDevicesFromQemu ();
  if (RETURN_ERROR (Status)) {
    //
    // Just use the simple policy to connect all devices
    //
    DEBUG ((DEBUG_INFO, "EfiBootManagerConnectAll\n"));
    EfiBootManagerConnectAll ();
  }
}

/**
  Save the S3 boot script.

  Note that DxeSmmReadyToLock must be signaled after this function returns;
  otherwise the script wouldn't be saved actually.
**/
STATIC
VOID
SaveS3BootScript (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_S3_SAVE_STATE_PROTOCOL  *BootScript;
  STATIC CONST UINT8          Info[] = { 0xDE, 0xAD, 0xBE, 0xEF };

  Status = gBS->LocateProtocol (
                  &gEfiS3SaveStateProtocolGuid,
                  NULL,
                  (VOID **)&BootScript
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Despite the opcode documentation in the PI spec, the protocol
  // implementation embeds a deep copy of the info in the boot script, rather
  // than storing just a pointer to runtime or NVS storage.
  //
  Status = BootScript->Write (
                         BootScript,
                         EFI_BOOT_SCRIPT_INFORMATION_OPCODE,
                         (UINT32)sizeof Info,
                         (EFI_PHYSICAL_ADDRESS)(UINTN)&Info
                         );
  ASSERT_EFI_ERROR (Status);
}

/**
  Uninstall the EFI memory attribute protocol if it exists.
**/
STATIC
VOID
UninstallEfiMemoryAttributesProtocol (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;
  UINTN       Size;
  VOID        *MemoryAttributeProtocol;

  Size   = sizeof (Handle);
  Status = gBS->LocateHandle (
                  ByProtocol,
                  &gEfiMemoryAttributeProtocolGuid,
                  NULL,
                  &Size,
                  &Handle
                  );

  if (EFI_ERROR (Status)) {
    ASSERT (Status == EFI_NOT_FOUND);
    return;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiMemoryAttributeProtocolGuid,
                  &MemoryAttributeProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->UninstallProtocolInterface (
                  Handle,
                  &gEfiMemoryAttributeProtocolGuid,
                  MemoryAttributeProtocol
                  );
  ASSERT_EFI_ERROR (Status);
}

/**
  Do the platform specific action after the console is ready

  Possible things that can be done in PlatformBootManagerAfterConsole:

  > Console post action:
    > Dynamically switch output mode from 100x31 to 80x25 for certain senarino
    > Signal console ready platform customized event
  > Run diagnostics like memory testing
  > Connect certain devices
  > Dispatch aditional option roms
  > Special boot: e.g.: USB boot, enter UI
**/
VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  )
{
  EFI_BOOT_MODE  BootMode;
  BOOLEAN        Uninstall;

  DEBUG ((DEBUG_INFO, "PlatformBootManagerAfterConsole\n"));

  if (PcdGetBool (PcdOvmfFlashVariablesEnable)) {
    DEBUG ((
      DEBUG_INFO,
      "PlatformBdsPolicyBehavior: not restoring NvVars "
      "from disk since flash variables appear to be supported.\n"
      ));
  } else {
    //
    // Try to restore variables from the hard disk early so
    // they can be used for the other BDS connect operations.
    //
    PlatformBdsRestoreNvVarsFromHardDisk ();
  }

  //
  // Get current Boot Mode
  //
  BootMode = GetBootModeHob ();
  DEBUG ((DEBUG_INFO, "Boot Mode:%x\n", BootMode));

  //
  // Go the different platform policy with different boot mode
  // Notes: this part code can be change with the table policy
  //
  ASSERT (BootMode == BOOT_WITH_FULL_CONFIGURATION);

  //
  // Logo show
  //
  BootLogoEnableLogo ();

  //
  // Set PCI Interrupt Line registers and ACPI SCI_EN
  //
  PciAcpiInitialization ();

  //
  // Write qemu bootorder to efi variables
  //
  StoreQemuBootOrder ();

  //
  // Work around shim's terminally broken use of the EFI memory attributes
  // protocol, by uninstalling it if requested on the QEMU command line.
  //
  // E.g.,
  //       -fw_cfg opt/org.tianocore/UninstallMemAttrProtocol,string=y
  //
  Uninstall = FixedPcdGetBool (PcdUninstallMemAttrProtocol);
  QemuFwCfgParseBool ("opt/org.tianocore/UninstallMemAttrProtocol", &Uninstall);
  DEBUG ((
    DEBUG_WARN,
    "%a: %auninstalling EFI memory protocol\n",
    __func__,
    Uninstall ? "" : "not "
    ));
  if (Uninstall) {
    UninstallEfiMemoryAttributesProtocol ();
  }

  //
  // Process QEMU's -kernel command line option
  //
  TryRunningQemuKernel ();

  //
  // Perform some platform specific connect sequence
  //
  if (FeaturePcdGet (PcdBootRestrictToFirmware)) {
    RestrictBootOptionsToFirmware ();
  } else {
    PlatformBdsConnectSequence ();
    EfiBootManagerRefreshAllBootOption ();
  }

  BOOLEAN        ShellEnabled;
  RETURN_STATUS  RetStatus;

  RetStatus = QemuFwCfgParseBool (
                "opt/org.tianocore/EFIShellSupport",
                &ShellEnabled
                );

  if (RETURN_ERROR (RetStatus)) {
    ShellEnabled = TRUE;
  }

  //
  // Register UEFI Shell
  //
  PlatformRegisterFvBootOption (
    &gUefiShellFileGuid,
    L"EFI Internal Shell",
    LOAD_OPTION_ACTIVE | LOAD_OPTION_CATEGORY_APP,
    ShellEnabled
    );

  //
  // Register Grub
  //
  PlatformRegisterFvBootOption (
    &gGrubFileGuid,
    L"Grub Bootloader",
    LOAD_OPTION_ACTIVE,
    TRUE
    );

  RemoveStaleFvFileOptions ();
  SetBootOrderFromQemu ();

  PlatformBmPrintScRegisterHandler ();
}

/**
  This notification function is invoked when an instance of the
  EFI_DEVICE_PATH_PROTOCOL is produced.

  @param  Event                 The event that occurred
  @param  Context               For EFI compatibility.  Not used.

**/
VOID
EFIAPI
NotifyDevPath (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_HANDLE                Handle;
  EFI_STATUS                Status;
  UINTN                     BufferSize;
  EFI_DEVICE_PATH_PROTOCOL  *DevPathNode;
  ATAPI_DEVICE_PATH         *Atapi;

  //
  // Examine all new handles
  //
  for ( ; ;) {
    //
    // Get the next handle
    //
    BufferSize = sizeof (Handle);
    Status     = gBS->LocateHandle (
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
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevPathNode
                    );
    ASSERT_EFI_ERROR (Status);

    while (!IsDevicePathEnd (DevPathNode)) {
      //
      // Find the handler to dump this device path node
      //
      if (
          (DevicePathType (DevPathNode) == MESSAGING_DEVICE_PATH) &&
          (DevicePathSubType (DevPathNode) == MSG_ATAPI_DP)
          )
      {
        Atapi = (ATAPI_DEVICE_PATH *)DevPathNode;
        PciOr16 (
          PCI_LIB_ADDRESS (
            0,
            1,
            1,
            (Atapi->PrimarySecondary == 1) ? 0x42 : 0x40
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
  DEBUG ((DEBUG_INFO, "Registered NotifyDevPath Event\n"));
  mEfiDevPathEvent = EfiCreateProtocolNotifyEvent (
                       &gEfiDevicePathProtocolGuid,
                       TPL_CALLBACK,
                       NotifyDevPath,
                       NULL,
                       &mEfiDevPathNotifyReg
                       );
}

/**
  This function is called each second during the boot manager waits the
  timeout.

  @param TimeoutRemain  The remaining timeout.
**/
VOID
EFIAPI
PlatformBootManagerWaitCallback (
  UINT16  TimeoutRemain
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  Black;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  White;
  UINT16                               TimeoutInitial;

  TimeoutInitial = PcdGet16 (PcdPlatformBootTimeOut);

  //
  // If PcdPlatformBootTimeOut is set to zero, then we consider
  // that no progress update should be enacted (since we'd only
  // ever display a one-shot progress of either 0% or 100%).
  //
  if (TimeoutInitial == 0) {
    return;
  }

  Black.Raw = 0x00000000;
  White.Raw = 0x00FFFFFF;

  BootLogoUpdateProgress (
    White.Pixel,
    Black.Pixel,
    L"Start boot option",
    White.Pixel,
    (TimeoutInitial - TimeoutRemain) * 100 / TimeoutInitial,
    0
    );
}

/**
  The function is called when no boot option could be launched,
  including platform recovery options and options pointing to applications
  built into firmware volumes.

  If this function returns, BDS attempts to enter an infinite loop.
**/
VOID
EFIAPI
PlatformBootManagerUnableToBoot (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_INPUT_KEY                 Key;
  EFI_BOOT_MANAGER_LOAD_OPTION  BootManagerMenu;
  UINTN                         Index;

  if (FeaturePcdGet (PcdBootRestrictToFirmware)) {
    AsciiPrint (
      "%a: No bootable option was found.\n",
      gEfiCallerBaseName
      );
    CpuDeadLoop ();
  }

  //
  // BootManagerMenu doesn't contain the correct information when return status
  // is EFI_NOT_FOUND.
  //
  Status = EfiBootManagerGetBootManagerMenu (&BootManagerMenu);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Normally BdsDxe does not print anything to the system console, but this is
  // a last resort -- the end-user will likely not see any DEBUG messages
  // logged in this situation.
  //
  // AsciiPrint() will NULL-check gST->ConOut internally. We check gST->ConIn
  // here to see if it makes sense to request and wait for a keypress.
  //
  if (gST->ConIn != NULL) {
    AsciiPrint (
      "%a: No bootable option or device was found.\n"
      "%a: Press any key to enter the Boot Manager Menu.\n",
      gEfiCallerBaseName,
      gEfiCallerBaseName
      );
    Status = gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);
    ASSERT_EFI_ERROR (Status);
    ASSERT (Index == 0);

    //
    // Drain any queued keys.
    //
    while (!EFI_ERROR (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key))) {
      //
      // just throw away Key
      //
    }
  }

  for ( ; ;) {
    EfiBootManagerBoot (&BootManagerMenu);
  }
}
