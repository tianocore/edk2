/** @file
  Implementation for PlatformBootManagerLib library class interfaces.

  Copyright (C) 2015-2016, Red Hat, Inc.
  Copyright (c) 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Pci22.h>
#include <IndustryStandard/Virtio095.h>
#include <Library/BootLogoLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformBmPrintScLib.h>
#include <Library/QemuBootOrderLib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>
#include <Library/TpmPlatformHierarchyLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/VirtioDevice.h>
#include <Guid/EventGroup.h>
#include <Guid/GlobalVariable.h>
#include <Guid/RootBridgesConnectedEventGroup.h>
#include <Guid/SerialPortLibVendor.h>

#include "PlatformBm.h"

#define DP_NODE_LEN(Type)  { (UINT8)sizeof (Type), (UINT8)(sizeof (Type) >> 8) }

#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH            SerialDxe;
  UART_DEVICE_PATH              Uart;
  VENDOR_DEFINED_DEVICE_PATH    TermType;
  EFI_DEVICE_PATH_PROTOCOL      End;
} PLATFORM_SERIAL_CONSOLE;
#pragma pack ()

STATIC PLATFORM_SERIAL_CONSOLE  mSerialConsole = {
  //
  // VENDOR_DEVICE_PATH SerialDxe
  //
  {
    { HARDWARE_DEVICE_PATH,  HW_VENDOR_DP, DP_NODE_LEN (VENDOR_DEVICE_PATH) },
    EDKII_SERIAL_PORT_LIB_VENDOR_GUID
  },

  //
  // UART_DEVICE_PATH Uart
  //
  {
    { MESSAGING_DEVICE_PATH, MSG_UART_DP,  DP_NODE_LEN (UART_DEVICE_PATH)   },
    0,                                      // Reserved
    FixedPcdGet64 (PcdUartDefaultBaudRate), // BaudRate
    FixedPcdGet8 (PcdUartDefaultDataBits),  // DataBits
    FixedPcdGet8 (PcdUartDefaultParity),    // Parity
    FixedPcdGet8 (PcdUartDefaultStopBits)   // StopBits
  },

  //
  // VENDOR_DEFINED_DEVICE_PATH TermType
  //
  {
    {
      MESSAGING_DEVICE_PATH, MSG_VENDOR_DP,
      DP_NODE_LEN (VENDOR_DEFINED_DEVICE_PATH)
    }
    //
    // Guid to be filled in dynamically
    //
  },

  //
  // EFI_DEVICE_PATH_PROTOCOL End
  //
  {
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    DP_NODE_LEN (EFI_DEVICE_PATH_PROTOCOL)
  }
};

#pragma pack (1)
typedef struct {
  USB_CLASS_DEVICE_PATH       Keyboard;
  EFI_DEVICE_PATH_PROTOCOL    End;
} PLATFORM_USB_KEYBOARD;
#pragma pack ()

STATIC PLATFORM_USB_KEYBOARD  mUsbKeyboard = {
  //
  // USB_CLASS_DEVICE_PATH Keyboard
  //
  {
    {
      MESSAGING_DEVICE_PATH, MSG_USB_CLASS_DP,
      DP_NODE_LEN (USB_CLASS_DEVICE_PATH)
    },
    0xFFFF, // VendorId: any
    0xFFFF, // ProductId: any
    3,      // DeviceClass: HID
    1,      // DeviceSubClass: boot
    1       // DeviceProtocol: keyboard
  },

  //
  // EFI_DEVICE_PATH_PROTOCOL End
  //
  {
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE,
    DP_NODE_LEN (EFI_DEVICE_PATH_PROTOCOL)
  }
};

/**
  Check if the handle satisfies a particular condition.

  @param[in] Handle      The handle to check.
  @param[in] ReportText  A caller-allocated string passed in for reporting
                         purposes. It must never be NULL.

  @retval TRUE   The condition is satisfied.
  @retval FALSE  Otherwise. This includes the case when the condition could not
                 be fully evaluated due to an error.
**/
typedef
BOOLEAN
(EFIAPI *FILTER_FUNCTION)(
  IN EFI_HANDLE   Handle,
  IN CONST CHAR16 *ReportText
  );

/**
  Process a handle.

  @param[in] Handle      The handle to process.
  @param[in] ReportText  A caller-allocated string passed in for reporting
                         purposes. It must never be NULL.
**/
typedef
VOID
(EFIAPI *CALLBACK_FUNCTION)(
  IN EFI_HANDLE   Handle,
  IN CONST CHAR16 *ReportText
  );

/**
  Locate all handles that carry the specified protocol, filter them with a
  callback function, and pass each handle that passes the filter to another
  callback.

  @param[in] ProtocolGuid  The protocol to look for.

  @param[in] Filter        The filter function to pass each handle to. If this
                           parameter is NULL, then all handles are processed.

  @param[in] Process       The callback function to pass each handle to that
                           clears the filter.
**/
STATIC
VOID
FilterAndProcess (
  IN EFI_GUID           *ProtocolGuid,
  IN FILTER_FUNCTION    Filter         OPTIONAL,
  IN CALLBACK_FUNCTION  Process
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *Handles;
  UINTN       NoHandles;
  UINTN       Idx;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  ProtocolGuid,
                  NULL /* SearchKey */,
                  &NoHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    //
    // This is not an error, just an informative condition.
    //
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: %g: %r\n",
      __func__,
      ProtocolGuid,
      Status
      ));
    return;
  }

  ASSERT (NoHandles > 0);
  for (Idx = 0; Idx < NoHandles; ++Idx) {
    CHAR16         *DevicePathText;
    STATIC CHAR16  Fallback[] = L"<device path unavailable>";

    //
    // The ConvertDevicePathToText() function handles NULL input transparently.
    //
    DevicePathText = ConvertDevicePathToText (
                       DevicePathFromHandle (Handles[Idx]),
                       FALSE, // DisplayOnly
                       FALSE  // AllowShortcuts
                       );
    if (DevicePathText == NULL) {
      DevicePathText = Fallback;
    }

    if ((Filter == NULL) || Filter (Handles[Idx], DevicePathText)) {
      Process (Handles[Idx], DevicePathText);
    }

    if (DevicePathText != Fallback) {
      FreePool (DevicePathText);
    }
  }

  gBS->FreePool (Handles);
}

/**
  This FILTER_FUNCTION checks if a handle corresponds to a PCI display device.
**/
STATIC
BOOLEAN
EFIAPI
IsPciDisplay (
  IN EFI_HANDLE    Handle,
  IN CONST CHAR16  *ReportText
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           Pci;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo
                  );
  if (EFI_ERROR (Status)) {
    //
    // This is not an error worth reporting.
    //
    return FALSE;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0 /* Offset */,
                        sizeof Pci / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %s: %r\n", __func__, ReportText, Status));
    return FALSE;
  }

  return IS_PCI_DISPLAY (&Pci);
}

/**
  This function checks if a handle corresponds to the Virtio Device ID given
  at the VIRTIO_DEVICE_PROTOCOL level.
**/
STATIC
BOOLEAN
EFIAPI
IsVirtio (
  IN EFI_HANDLE    Handle,
  IN CONST CHAR16  *ReportText,
  IN UINT16        VirtIoDeviceId
  )
{
  EFI_STATUS              Status;
  VIRTIO_DEVICE_PROTOCOL  *VirtIo;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gVirtioDeviceProtocolGuid,
                  (VOID **)&VirtIo
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return (BOOLEAN)(VirtIo->SubSystemDeviceId ==
                   VirtIoDeviceId);
}

/**
  This FILTER_FUNCTION checks if a handle corresponds to a Virtio RNG device at
  the VIRTIO_DEVICE_PROTOCOL level.
**/
STATIC
BOOLEAN
EFIAPI
IsVirtioRng (
  IN EFI_HANDLE    Handle,
  IN CONST CHAR16  *ReportText
  )
{
  return IsVirtio (Handle, ReportText, VIRTIO_SUBSYSTEM_ENTROPY_SOURCE);
}

/**
  This FILTER_FUNCTION checks if a handle corresponds to a Virtio serial device at
  the VIRTIO_DEVICE_PROTOCOL level.
**/
STATIC
BOOLEAN
EFIAPI
IsVirtioSerial (
  IN EFI_HANDLE    Handle,
  IN CONST CHAR16  *ReportText
  )
{
  return IsVirtio (Handle, ReportText, VIRTIO_SUBSYSTEM_CONSOLE);
}

/**
  This function checks if a handle corresponds to the Virtio Device ID given
  at the EFI_PCI_IO_PROTOCOL level.
**/
STATIC
BOOLEAN
EFIAPI
IsVirtioPci (
  IN EFI_HANDLE    Handle,
  IN CONST CHAR16  *ReportText,
  IN UINT16        VirtIoDeviceId
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT16               VendorId;
  UINT16               DeviceId;
  UINT8                RevisionId;
  BOOLEAN              Virtio10;
  UINT16               SubsystemId;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

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
    goto PciError;
  }

  if (VendorId != VIRTIO_VENDOR_ID) {
    return FALSE;
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
    goto PciError;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_REVISION_ID_OFFSET,
                        1,
                        &RevisionId
                        );
  if (EFI_ERROR (Status)) {
    goto PciError;
  }

  //
  // From DeviceId and RevisionId, determine whether the device is a
  // modern-only Virtio 1.0 device. In case of Virtio 1.0, DeviceId can
  // immediately be restricted to VirtIoDeviceId, and
  // SubsystemId will only play a sanity-check role. Otherwise, DeviceId can
  // only be sanity-checked, and SubsystemId will decide.
  //
  if ((DeviceId == 0x1040 + VirtIoDeviceId) &&
      (RevisionId >= 0x01))
  {
    Virtio10 = TRUE;
  } else if ((DeviceId >= 0x1000) && (DeviceId <= 0x103F) && (RevisionId == 0x00)) {
    Virtio10 = FALSE;
  } else {
    return FALSE;
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
    goto PciError;
  }

  if (Virtio10 && (SubsystemId >= 0x40)) {
    return TRUE;
  }

  if (!Virtio10 && (SubsystemId == VirtIoDeviceId)) {
    return TRUE;
  }

  return FALSE;

PciError:
  DEBUG ((DEBUG_ERROR, "%a: %s: %r\n", __func__, ReportText, Status));
  return FALSE;
}

/**
  This FILTER_FUNCTION checks if a handle corresponds to a Virtio RNG device at
  the EFI_PCI_IO_PROTOCOL level.
**/
STATIC
BOOLEAN
EFIAPI
IsVirtioPciRng (
  IN EFI_HANDLE    Handle,
  IN CONST CHAR16  *ReportText
  )
{
  return IsVirtioPci (Handle, ReportText, VIRTIO_SUBSYSTEM_ENTROPY_SOURCE);
}

/**
  This FILTER_FUNCTION checks if a handle corresponds to a Virtio serial device at
  the EFI_PCI_IO_PROTOCOL level.
**/
STATIC
BOOLEAN
EFIAPI
IsVirtioPciSerial (
  IN EFI_HANDLE    Handle,
  IN CONST CHAR16  *ReportText
  )
{
  return IsVirtioPci (Handle, ReportText, VIRTIO_SUBSYSTEM_CONSOLE);
}

/**
  This CALLBACK_FUNCTION attempts to connect a handle non-recursively, asking
  the matching driver to produce all first-level child handles.
**/
STATIC
VOID
EFIAPI
Connect (
  IN EFI_HANDLE    Handle,
  IN CONST CHAR16  *ReportText
  )
{
  EFI_STATUS  Status;

  Status = gBS->ConnectController (
                  Handle, // ControllerHandle
                  NULL,   // DriverImageHandle
                  NULL,   // RemainingDevicePath -- produce all children
                  FALSE   // Recursive
                  );
  DEBUG ((
    EFI_ERROR (Status) ? DEBUG_ERROR : DEBUG_VERBOSE,
    "%a: %s: %r\n",
    __func__,
    ReportText,
    Status
    ));
}

/**
  This CALLBACK_FUNCTION retrieves the EFI_DEVICE_PATH_PROTOCOL from the
  handle, and adds it to ConOut and ErrOut.
**/
STATIC
VOID
EFIAPI
AddOutput (
  IN EFI_HANDLE    Handle,
  IN CONST CHAR16  *ReportText
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath = DevicePathFromHandle (Handle);
  if (DevicePath == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %s: handle %p: device path not found\n",
      __func__,
      ReportText,
      Handle
      ));
    return;
  }

  Status = EfiBootManagerUpdateConsoleVariable (ConOut, DevicePath, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %s: adding to ConOut: %r\n",
      __func__,
      ReportText,
      Status
      ));
    return;
  }

  Status = EfiBootManagerUpdateConsoleVariable (ErrOut, DevicePath, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %s: adding to ErrOut: %r\n",
      __func__,
      ReportText,
      Status
      ));
    return;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: %s: added to ConOut and ErrOut\n",
    __func__,
    ReportText
    ));
}

/**
  This CALLBACK_FUNCTION retrieves the EFI_DEVICE_PATH_PROTOCOL from
  the handle, appends serial, uart and terminal nodes, finally updates
  ConIn, ConOut and ErrOut.
**/
STATIC
VOID
EFIAPI
SetupVirtioSerial (
  IN EFI_HANDLE    Handle,
  IN CONST CHAR16  *ReportText
  )
{
  STATIC CONST ACPI_HID_DEVICE_PATH  SerialNode = {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)),
        (UINT8)((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)
      },
    },
    EISA_PNP_ID (0x0501),
    0
  };

  STATIC CONST UART_DEVICE_PATH  UartNode = {
    {
      MESSAGING_DEVICE_PATH,
      MSG_UART_DP,
      {
        (UINT8)(sizeof (UART_DEVICE_PATH)),
        (UINT8)((sizeof (UART_DEVICE_PATH)) >> 8)
      },
    },
    0,
    115200,
    8,
    1,
    1
  };

  STATIC VENDOR_DEVICE_PATH  TerminalNode = {
    {
      MESSAGING_DEVICE_PATH,
      MSG_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      },
    },
    // copy from PcdTerminalTypeGuidBuffer
  };

  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath, *OldDevicePath;

  DevicePath = DevicePathFromHandle (Handle);

  if (DevicePath == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %s: handle %p: device path not found\n",
      __func__,
      ReportText,
      Handle
      ));
    return;
  }

  CopyGuid (
    &TerminalNode.Guid,
    PcdGetPtr (PcdTerminalTypeGuidBuffer)
    );

  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 &SerialNode.Header
                 );

  OldDevicePath = DevicePath;
  DevicePath    = AppendDevicePathNode (
                    DevicePath,
                    &UartNode.Header
                    );
  FreePool (OldDevicePath);

  OldDevicePath = DevicePath;
  DevicePath    = AppendDevicePathNode (
                    DevicePath,
                    &TerminalNode.Header
                    );
  FreePool (OldDevicePath);

  Status = EfiBootManagerUpdateConsoleVariable (ConIn, DevicePath, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %s: adding to ConIn: %r\n",
      __func__,
      ReportText,
      Status
      ));
    return;
  }

  Status = EfiBootManagerUpdateConsoleVariable (ConOut, DevicePath, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,

      "%a: %s: adding to ConOut: %r\n",
      __func__,
      ReportText,
      Status
      ));
    return;
  }

  Status = EfiBootManagerUpdateConsoleVariable (ErrOut, DevicePath, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %s: adding to ErrOut: %r\n",
      __func__,
      ReportText,
      Status
      ));
    return;
  }

  FreePool (DevicePath);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: %s: added to ConIn, ConOut and ErrOut\n",
    __func__,
    ReportText
    ));
}

STATIC
VOID
PlatformRegisterFvBootOption (
  EFI_GUID  *FileGuid,
  CHAR16    *Description,
  UINT32    Attributes
  )
{
  EFI_STATUS                         Status;
  INTN                               OptionIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION       NewOption;
  EFI_BOOT_MANAGER_LOAD_OPTION       *BootOptions;
  UINTN                              BootOptionCount;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL           *DevicePath;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  ASSERT_EFI_ERROR (Status);

  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);
  DevicePath = DevicePathFromHandle (LoadedImage->DeviceHandle);
  ASSERT (DevicePath != NULL);
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&FileNode
                 );
  ASSERT (DevicePath != NULL);

  Status = EfiBootManagerInitializeLoadOption (
             &NewOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             Attributes,
             Description,
             DevicePath,
             NULL,
             0
             );
  ASSERT_EFI_ERROR (Status);
  FreePool (DevicePath);

  BootOptions = EfiBootManagerGetLoadOptions (
                  &BootOptionCount,
                  LoadOptionTypeBoot
                  );

  OptionIndex = EfiBootManagerFindLoadOption (
                  &NewOption,
                  BootOptions,
                  BootOptionCount
                  );

  if (OptionIndex == -1) {
    Status = EfiBootManagerAddLoadOptionVariable (&NewOption, MAX_UINTN);
    ASSERT_EFI_ERROR (Status);
  }

  EfiBootManagerFreeLoadOption (&NewOption);
  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
}

/**
  Remove all MemoryMapped(...)/FvFile(...) and Fv(...)/FvFile(...) boot options
  whose device paths do not resolve exactly to an FvFile in the system.

  This removes any boot options that point to binaries built into the firmware
  and have become stale due to any of the following:
  - FvMain's base address or size changed (historical),
  - FvMain's FvNameGuid changed,
  - the FILE_GUID of the pointed-to binary changed,
  - the referenced binary is no longer built into the firmware.

  EfiBootManagerFindLoadOption() used in PlatformRegisterFvBootOption() only
  avoids exact duplicates.
**/
STATIC
VOID
RemoveStaleFvFileOptions (
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
    EFI_DEVICE_PATH_PROTOCOL  *Node1, *Node2, *SearchNode;
    EFI_STATUS                Status;
    EFI_HANDLE                FvHandle;

    //
    // If the device path starts with neither MemoryMapped(...) nor Fv(...),
    // then keep the boot option.
    //
    Node1 = BootOptions[Index].FilePath;
    if (!((DevicePathType (Node1) == HARDWARE_DEVICE_PATH) &&
          (DevicePathSubType (Node1) == HW_MEMMAP_DP)) &&
        !((DevicePathType (Node1) == MEDIA_DEVICE_PATH) &&
          (DevicePathSubType (Node1) == MEDIA_PIWG_FW_VOL_DP)))
    {
      continue;
    }

    //
    // If the second device path node is not FvFile(...), then keep the boot
    // option.
    //
    Node2 = NextDevicePathNode (Node1);
    if ((DevicePathType (Node2) != MEDIA_DEVICE_PATH) ||
        (DevicePathSubType (Node2) != MEDIA_PIWG_FW_FILE_DP))
    {
      continue;
    }

    //
    // Locate the Firmware Volume2 protocol instance that is denoted by the
    // boot option. If this lookup fails (i.e., the boot option references a
    // firmware volume that doesn't exist), then we'll proceed to delete the
    // boot option.
    //
    SearchNode = Node1;
    Status     = gBS->LocateDevicePath (
                        &gEfiFirmwareVolume2ProtocolGuid,
                        &SearchNode,
                        &FvHandle
                        );

    if (!EFI_ERROR (Status)) {
      //
      // The firmware volume was found; now let's see if it contains the FvFile
      // identified by GUID.
      //
      EFI_FIRMWARE_VOLUME2_PROTOCOL      *FvProtocol;
      MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  *FvFileNode;
      UINTN                              BufferSize;
      EFI_FV_FILETYPE                    FoundType;
      EFI_FV_FILE_ATTRIBUTES             FileAttributes;
      UINT32                             AuthenticationStatus;

      Status = gBS->HandleProtocol (
                      FvHandle,
                      &gEfiFirmwareVolume2ProtocolGuid,
                      (VOID **)&FvProtocol
                      );
      ASSERT_EFI_ERROR (Status);

      FvFileNode = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)Node2;
      //
      // Buffer==NULL means we request metadata only: BufferSize, FoundType,
      // FileAttributes.
      //
      Status = FvProtocol->ReadFile (
                             FvProtocol,
                             &FvFileNode->FvFileName, // NameGuid
                             NULL,                    // Buffer
                             &BufferSize,
                             &FoundType,
                             &FileAttributes,
                             &AuthenticationStatus
                             );
      if (!EFI_ERROR (Status)) {
        //
        // The FvFile was found. Keep the boot option.
        //
        continue;
      }
    }

    //
    // Delete the boot option.
    //
    Status = EfiBootManagerDeleteLoadOptionVariable (
               BootOptions[Index].OptionNumber,
               LoadOptionTypeBoot
               );
    DEBUG_CODE_BEGIN ();
    CHAR16  *DevicePathString;

    DevicePathString = ConvertDevicePathToText (
                         BootOptions[Index].FilePath,
                         FALSE,
                         FALSE
                         );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_VERBOSE,
      "%a: removing stale Boot#%04x %s: %r\n",
      __func__,
      (UINT32)BootOptions[Index].OptionNumber,
      DevicePathString == NULL ? L"<unavailable>" : DevicePathString,
      Status
      ));
    if (DevicePathString != NULL) {
      FreePool (DevicePathString);
    }

    DEBUG_CODE_END ();
  }

  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
}

STATIC
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
  // Map F2 and ESC to Boot Manager Menu
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
  UINT16         FrontPageTimeout;
  RETURN_STATUS  PcdStatus;
  EFI_STATUS     Status;

  //
  // Signal EndOfDxe PI Event
  //
  EfiEventGroupSignal (&gEfiEndOfDxeEventGroupGuid);

  //
  // Disable the TPM 2 platform hierarchy
  //
  ConfigureTpmPlatformHierarchy ();

  //
  // Dispatch deferred images after EndOfDxe event.
  //
  EfiBootManagerDispatchDeferredImages ();

  //
  // Locate the PCI root bridges and make the PCI bus driver connect each,
  // non-recursively. This will produce a number of child handles with PciIo on
  // them.
  //
  FilterAndProcess (&gEfiPciRootBridgeIoProtocolGuid, NULL, Connect);

  //
  // Signal the ACPI platform driver that it can download QEMU ACPI tables.
  //
  EfiEventGroupSignal (&gRootBridgesConnectedEventGroupGuid);

  //
  // Find all display class PCI devices (using the handles from the previous
  // step), and connect them non-recursively. This should produce a number of
  // child handles with GOPs on them.
  //
  FilterAndProcess (&gEfiPciIoProtocolGuid, IsPciDisplay, Connect);

  //
  // Now add the device path of all handles with GOP on them to ConOut and
  // ErrOut.
  //
  FilterAndProcess (&gEfiGraphicsOutputProtocolGuid, NULL, AddOutput);

  //
  // Add the hardcoded short-form USB keyboard device path to ConIn.
  //
  EfiBootManagerUpdateConsoleVariable (
    ConIn,
    (EFI_DEVICE_PATH_PROTOCOL *)&mUsbKeyboard,
    NULL
    );

  //
  // Add the hardcoded serial console device path to ConIn, ConOut, ErrOut.
  //
  CopyGuid (
    &mSerialConsole.TermType.Guid,
    PcdGetPtr (PcdTerminalTypeGuidBuffer)
    );
  EfiBootManagerUpdateConsoleVariable (
    ConIn,
    (EFI_DEVICE_PATH_PROTOCOL *)&mSerialConsole,
    NULL
    );
  EfiBootManagerUpdateConsoleVariable (
    ConOut,
    (EFI_DEVICE_PATH_PROTOCOL *)&mSerialConsole,
    NULL
    );
  EfiBootManagerUpdateConsoleVariable (
    ErrOut,
    (EFI_DEVICE_PATH_PROTOCOL *)&mSerialConsole,
    NULL
    );

  //
  // Set the front page timeout from the QEMU configuration.
  //
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

  //
  // Register platform-specific boot options and keyboard shortcuts.
  //
  PlatformRegisterOptionsAndKeys ();

  //
  // At this point, VIRTIO_DEVICE_PROTOCOL instances exist only for Virtio MMIO
  // transports. Install EFI_RNG_PROTOCOL instances on Virtio MMIO RNG devices.
  //
  FilterAndProcess (&gVirtioDeviceProtocolGuid, IsVirtioRng, Connect);

  //
  // Install both VIRTIO_DEVICE_PROTOCOL and (dependent) EFI_RNG_PROTOCOL
  // instances on Virtio PCI RNG devices.
  //
  FilterAndProcess (&gEfiPciIoProtocolGuid, IsVirtioPciRng, Connect);

  //
  // Register Virtio serial devices as console.
  //
  FilterAndProcess (&gVirtioDeviceProtocolGuid, IsVirtioSerial, SetupVirtioSerial);
  FilterAndProcess (&gEfiPciIoProtocolGuid, IsVirtioPciSerial, SetupVirtioSerial);
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
    > Dynamically switch output mode from 100x31 to 80x25 for certain scenario
    > Signal console ready platform customized event
  > Run diagnostics like memory testing
  > Connect certain devices
  > Dispatch additional option roms
  > Special boot: e.g.: USB boot, enter UI
**/
VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  )
{
  RETURN_STATUS  Status;
  BOOLEAN        Uninstall;

  //
  // Show the splash screen.
  //
  BootLogoEnableLogo ();

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
  // Process QEMU's -kernel command line option. The kernel booted this way
  // will receive ACPI tables: in PlatformBootManagerBeforeConsole(), we
  // connected any and all PCI root bridges, and then signaled the ACPI
  // platform driver.
  //
  TryRunningQemuKernel ();

  //
  // Connect the purported boot devices.
  //
  Status = ConnectDevicesFromQemu ();
  if (RETURN_ERROR (Status)) {
    //
    // Connect the rest of the devices.
    //
    EfiBootManagerConnectAll ();
  }

  //
  // Enumerate all possible boot options, then filter and reorder them based on
  // the QEMU configuration.
  //
  EfiBootManagerRefreshAllBootOption ();

  //
  // Register UEFI Shell
  //
  PlatformRegisterFvBootOption (
    &gUefiShellFileGuid,
    L"EFI Internal Shell",
    LOAD_OPTION_ACTIVE
    );

  RemoveStaleFvFileOptions ();
  SetBootOrderFromQemu ();

  PlatformBmPrintScRegisterHandler ();
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
  // that no progress update should be enacted.
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
