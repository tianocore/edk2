/** @file
  Implementation for PlatformBootManagerLib library class interfaces.

  Copyright (C) 2015-2016, Red Hat, Inc.
  Copyright (c) 2014 - 2019, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Pci22.h>
#include <Library/BootLogoLib.h>
#include <Library/CapsuleLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/EsrtManagement.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/NonDiscoverableDevice.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PlatformBootManager.h>
#include <Guid/EventGroup.h>
#include <Guid/NonDiscoverableDevice.h>
#include <Guid/TtyTerm.h>
#include <Guid/SerialPortLibVendor.h>

#include "PlatformBm.h"

#define DP_NODE_LEN(Type) { (UINT8)sizeof (Type), (UINT8)(sizeof (Type) >> 8) }

#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH         SerialDxe;
  UART_DEVICE_PATH           Uart;
  VENDOR_DEFINED_DEVICE_PATH TermType;
  EFI_DEVICE_PATH_PROTOCOL   End;
} PLATFORM_SERIAL_CONSOLE;
#pragma pack ()

STATIC PLATFORM_SERIAL_CONSOLE mSerialConsole = {
  //
  // VENDOR_DEVICE_PATH SerialDxe
  //
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, DP_NODE_LEN (VENDOR_DEVICE_PATH) },
    EDKII_SERIAL_PORT_LIB_VENDOR_GUID
  },

  //
  // UART_DEVICE_PATH Uart
  //
  {
    { MESSAGING_DEVICE_PATH, MSG_UART_DP, DP_NODE_LEN (UART_DEVICE_PATH) },
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
  USB_CLASS_DEVICE_PATH    Keyboard;
  EFI_DEVICE_PATH_PROTOCOL End;
} PLATFORM_USB_KEYBOARD;
#pragma pack ()

STATIC PLATFORM_USB_KEYBOARD mUsbKeyboard = {
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
(EFIAPI *FILTER_FUNCTION) (
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
(EFIAPI *CALLBACK_FUNCTION)  (
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
  IN EFI_GUID          *ProtocolGuid,
  IN FILTER_FUNCTION   Filter         OPTIONAL,
  IN CALLBACK_FUNCTION Process
  )
{
  EFI_STATUS Status;
  EFI_HANDLE *Handles;
  UINTN      NoHandles;
  UINTN      Idx;

  Status = gBS->LocateHandleBuffer (ByProtocol, ProtocolGuid,
                  NULL /* SearchKey */, &NoHandles, &Handles);
  if (EFI_ERROR (Status)) {
    //
    // This is not an error, just an informative condition.
    //
    DEBUG ((EFI_D_VERBOSE, "%a: %g: %r\n", __FUNCTION__, ProtocolGuid,
      Status));
    return;
  }

  ASSERT (NoHandles > 0);
  for (Idx = 0; Idx < NoHandles; ++Idx) {
    CHAR16        *DevicePathText;
    STATIC CHAR16 Fallback[] = L"<device path unavailable>";

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

    if (Filter == NULL || Filter (Handles[Idx], DevicePathText)) {
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
  IN EFI_HANDLE   Handle,
  IN CONST CHAR16 *ReportText
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00          Pci;

  Status = gBS->HandleProtocol (Handle, &gEfiPciIoProtocolGuid,
                  (VOID**)&PciIo);
  if (EFI_ERROR (Status)) {
    //
    // This is not an error worth reporting.
    //
    return FALSE;
  }

  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0 /* Offset */,
                        sizeof Pci / sizeof (UINT32), &Pci);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: %s: %r\n", __FUNCTION__, ReportText, Status));
    return FALSE;
  }

  return IS_PCI_DISPLAY (&Pci);
}


/**
  This FILTER_FUNCTION checks if a handle corresponds to a non-discoverable
  USB host controller.
**/
STATIC
BOOLEAN
EFIAPI
IsUsbHost (
  IN EFI_HANDLE   Handle,
  IN CONST CHAR16 *ReportText
  )
{
  NON_DISCOVERABLE_DEVICE   *Device;
  EFI_STATUS                Status;

  Status = gBS->HandleProtocol (Handle,
                  &gEdkiiNonDiscoverableDeviceProtocolGuid,
                  (VOID **)&Device);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (CompareGuid (Device->Type, &gEdkiiNonDiscoverableUhciDeviceGuid) ||
      CompareGuid (Device->Type, &gEdkiiNonDiscoverableEhciDeviceGuid) ||
      CompareGuid (Device->Type, &gEdkiiNonDiscoverableXhciDeviceGuid)) {
    return TRUE;
  }
  return FALSE;
}


/**
  This CALLBACK_FUNCTION attempts to connect a handle non-recursively, asking
  the matching driver to produce all first-level child handles.
**/
STATIC
VOID
EFIAPI
Connect (
  IN EFI_HANDLE   Handle,
  IN CONST CHAR16 *ReportText
  )
{
  EFI_STATUS Status;

  Status = gBS->ConnectController (
                  Handle, // ControllerHandle
                  NULL,   // DriverImageHandle
                  NULL,   // RemainingDevicePath -- produce all children
                  FALSE   // Recursive
                  );
  DEBUG ((EFI_ERROR (Status) ? EFI_D_ERROR : EFI_D_VERBOSE, "%a: %s: %r\n",
    __FUNCTION__, ReportText, Status));
}


/**
  This CALLBACK_FUNCTION retrieves the EFI_DEVICE_PATH_PROTOCOL from the
  handle, and adds it to ConOut and ErrOut.
**/
STATIC
VOID
EFIAPI
AddOutput (
  IN EFI_HANDLE   Handle,
  IN CONST CHAR16 *ReportText
  )
{
  EFI_STATUS               Status;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;

  DevicePath = DevicePathFromHandle (Handle);
  if (DevicePath == NULL) {
    DEBUG ((EFI_D_ERROR, "%a: %s: handle %p: device path not found\n",
      __FUNCTION__, ReportText, Handle));
    return;
  }

  Status = EfiBootManagerUpdateConsoleVariable (ConOut, DevicePath, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: %s: adding to ConOut: %r\n", __FUNCTION__,
      ReportText, Status));
    return;
  }

  Status = EfiBootManagerUpdateConsoleVariable (ErrOut, DevicePath, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: %s: adding to ErrOut: %r\n", __FUNCTION__,
      ReportText, Status));
    return;
  }

  DEBUG ((EFI_D_VERBOSE, "%a: %s: added to ConOut and ErrOut\n", __FUNCTION__,
    ReportText));
}

STATIC
VOID
PlatformRegisterFvBootOption (
  CONST EFI_GUID                   *FileGuid,
  CHAR16                           *Description,
  UINT32                           Attributes,
  EFI_INPUT_KEY                    *Key
  )
{
  EFI_STATUS                        Status;
  INTN                              OptionIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION      NewOption;
  EFI_BOOT_MANAGER_LOAD_OPTION      *BootOptions;
  UINTN                             BootOptionCount;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH FileNode;
  EFI_LOADED_IMAGE_PROTOCOL         *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **) &LoadedImage
                  );
  ASSERT_EFI_ERROR (Status);

  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);
  DevicePath = DevicePathFromHandle (LoadedImage->DeviceHandle);
  ASSERT (DevicePath != NULL);
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *) &FileNode
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
                  &BootOptionCount, LoadOptionTypeBoot
                  );

  OptionIndex = EfiBootManagerFindLoadOption (
                  &NewOption, BootOptions, BootOptionCount
                  );

  if (OptionIndex == -1) {
    Status = EfiBootManagerAddLoadOptionVariable (&NewOption, MAX_UINTN);
    ASSERT_EFI_ERROR (Status);
    Status = EfiBootManagerAddKeyOptionVariable (NULL,
               (UINT16)NewOption.OptionNumber, 0, Key, NULL);
    ASSERT (Status == EFI_SUCCESS || Status == EFI_ALREADY_STARTED);
  }
  EfiBootManagerFreeLoadOption (&NewOption);
  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
}


STATIC
VOID
GetPlatformOptions (
  VOID
  )
{
  EFI_STATUS                      Status;
  EFI_BOOT_MANAGER_LOAD_OPTION    *CurrentBootOptions;
  EFI_BOOT_MANAGER_LOAD_OPTION    *BootOptions;
  EFI_INPUT_KEY                   *BootKeys;
  PLATFORM_BOOT_MANAGER_PROTOCOL  *PlatformBootManager;
  UINTN                           CurrentBootOptionCount;
  UINTN                           Index;
  UINTN                           BootCount;

  Status = gBS->LocateProtocol (&gPlatformBootManagerProtocolGuid, NULL,
                  (VOID **)&PlatformBootManager);
  if (EFI_ERROR (Status)) {
    return;
  }
  Status = PlatformBootManager->GetPlatformBootOptionsAndKeys (
                                  &BootCount,
                                  &BootOptions,
                                  &BootKeys
                                  );
  if (EFI_ERROR (Status)) {
    return;
  }
  //
  // Fetch the existent boot options. If there are none, CurrentBootCount
  // will be zeroed.
  //
  CurrentBootOptions = EfiBootManagerGetLoadOptions (
                         &CurrentBootOptionCount,
                         LoadOptionTypeBoot
                         );
  //
  // Process the platform boot options.
  //
  for (Index = 0; Index < BootCount; Index++) {
    INTN    Match;
    UINTN   BootOptionNumber;

    //
    // If there are any preexistent boot options, and the subject platform boot
    // option is already among them, then don't try to add it. Just get its
    // assigned boot option number so we can associate a hotkey with it. Note
    // that EfiBootManagerFindLoadOption() deals fine with (CurrentBootOptions
    // == NULL) if (CurrentBootCount == 0).
    //
    Match = EfiBootManagerFindLoadOption (
              &BootOptions[Index],
              CurrentBootOptions,
              CurrentBootOptionCount
              );
    if (Match >= 0) {
      BootOptionNumber = CurrentBootOptions[Match].OptionNumber;
    } else {
      //
      // Add the platform boot options as a new one, at the end of the boot
      // order. Note that if the platform provided this boot option with an
      // unassigned option number, then the below function call will assign a
      // number.
      //
      Status = EfiBootManagerAddLoadOptionVariable (
                 &BootOptions[Index],
                 MAX_UINTN
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: failed to register \"%s\": %r\n",
          __FUNCTION__, BootOptions[Index].Description, Status));
        continue;
      }
      BootOptionNumber = BootOptions[Index].OptionNumber;
    }

    //
    // Register a hotkey with the boot option, if requested.
    //
    if (BootKeys[Index].UnicodeChar == L'\0') {
      continue;
    }

    Status = EfiBootManagerAddKeyOptionVariable (
               NULL,
               BootOptionNumber,
               0,
               &BootKeys[Index],
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: failed to register hotkey for \"%s\": %r\n",
        __FUNCTION__, BootOptions[Index].Description, Status));
    }
  }
  EfiBootManagerFreeLoadOptions (CurrentBootOptions, CurrentBootOptionCount);
  EfiBootManagerFreeLoadOptions (BootOptions, BootCount);
  FreePool (BootKeys);
}

STATIC
VOID
PlatformRegisterOptionsAndKeys (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_INPUT_KEY                Enter;
  EFI_INPUT_KEY                F2;
  EFI_INPUT_KEY                Esc;
  EFI_BOOT_MANAGER_LOAD_OPTION BootOption;

  GetPlatformOptions ();

  //
  // Register ENTER as CONTINUE key
  //
  Enter.ScanCode    = SCAN_NULL;
  Enter.UnicodeChar = CHAR_CARRIAGE_RETURN;
  Status = EfiBootManagerRegisterContinueKeyOption (0, &Enter, NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // Map F2 and ESC to Boot Manager Menu
  //
  F2.ScanCode     = SCAN_F2;
  F2.UnicodeChar  = CHAR_NULL;
  Esc.ScanCode    = SCAN_ESC;
  Esc.UnicodeChar = CHAR_NULL;
  Status = EfiBootManagerGetBootManagerMenu (&BootOption);
  ASSERT_EFI_ERROR (Status);
  Status = EfiBootManagerAddKeyOptionVariable (
             NULL, (UINT16) BootOption.OptionNumber, 0, &F2, NULL
             );
  ASSERT (Status == EFI_SUCCESS || Status == EFI_ALREADY_STARTED);
  Status = EfiBootManagerAddKeyOptionVariable (
             NULL, (UINT16) BootOption.OptionNumber, 0, &Esc, NULL
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
  //
  // Signal EndOfDxe PI Event
  //
  EfiEventGroupSignal (&gEfiEndOfDxeEventGroupGuid);

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
  // The core BDS code connects short-form USB device paths by explicitly
  // looking for handles with PCI I/O installed, and checking the PCI class
  // code whether it matches the one for a USB host controller. This means
  // non-discoverable USB host controllers need to have the non-discoverable
  // PCI driver attached first.
  //
  FilterAndProcess (&gEdkiiNonDiscoverableDeviceProtocolGuid, IsUsbHost, Connect);

  //
  // Add the hardcoded short-form USB keyboard device path to ConIn.
  //
  EfiBootManagerUpdateConsoleVariable (ConIn,
    (EFI_DEVICE_PATH_PROTOCOL *)&mUsbKeyboard, NULL);

  //
  // Add the hardcoded serial console device path to ConIn, ConOut, ErrOut.
  //
  STATIC_ASSERT (FixedPcdGet8 (PcdDefaultTerminalType) == 4,
    "PcdDefaultTerminalType must be TTYTERM");
  STATIC_ASSERT (FixedPcdGet8 (PcdUartDefaultParity) != 0,
    "PcdUartDefaultParity must be set to an actual value, not 'default'");
  STATIC_ASSERT (FixedPcdGet8 (PcdUartDefaultStopBits) != 0,
    "PcdUartDefaultStopBits must be set to an actual value, not 'default'");

  CopyGuid (&mSerialConsole.TermType.Guid, &gEfiTtyTermGuid);

  EfiBootManagerUpdateConsoleVariable (ConIn,
    (EFI_DEVICE_PATH_PROTOCOL *)&mSerialConsole, NULL);
  EfiBootManagerUpdateConsoleVariable (ConOut,
    (EFI_DEVICE_PATH_PROTOCOL *)&mSerialConsole, NULL);
  EfiBootManagerUpdateConsoleVariable (ErrOut,
    (EFI_DEVICE_PATH_PROTOCOL *)&mSerialConsole, NULL);

  //
  // Register platform-specific boot options and keyboard shortcuts.
  //
  PlatformRegisterOptionsAndKeys ();
}

STATIC
VOID
HandleCapsules (
  VOID
  )
{
  ESRT_MANAGEMENT_PROTOCOL    *EsrtManagement;
  EFI_PEI_HOB_POINTERS        HobPointer;
  EFI_CAPSULE_HEADER          *CapsuleHeader;
  BOOLEAN                     NeedReset;
  EFI_STATUS                  Status;

  DEBUG ((DEBUG_INFO, "%a: processing capsules ...\n", __FUNCTION__));

  Status = gBS->LocateProtocol (&gEsrtManagementProtocolGuid, NULL,
                  (VOID **)&EsrtManagement);
  if (!EFI_ERROR (Status)) {
    EsrtManagement->SyncEsrtFmp ();
  }

  //
  // Find all capsule images from hob
  //
  HobPointer.Raw = GetHobList ();
  NeedReset = FALSE;
  while ((HobPointer.Raw = GetNextHob (EFI_HOB_TYPE_UEFI_CAPSULE,
                             HobPointer.Raw)) != NULL) {
    CapsuleHeader = (VOID *)(UINTN)HobPointer.Capsule->BaseAddress;

    Status = ProcessCapsuleImage (CapsuleHeader);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: failed to process capsule %p - %r\n",
        __FUNCTION__, CapsuleHeader, Status));
      return;
    }

    NeedReset = TRUE;
    HobPointer.Raw = GET_NEXT_HOB (HobPointer);
  }

  if (NeedReset) {
      DEBUG ((DEBUG_WARN, "%a: capsule update successful, resetting ...\n",
        __FUNCTION__));

      gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
      CpuDeadLoop();
  }
}


#define VERSION_STRING_PREFIX    L"Tianocore/EDK2 firmware version "

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
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  UINTN                         FirmwareVerLength;
  UINTN                         PosX;
  UINTN                         PosY;
  EFI_INPUT_KEY                 Key;

  FirmwareVerLength = StrLen (PcdGetPtr (PcdFirmwareVersionString));

  //
  // Show the splash screen.
  //
  Status = BootLogoEnableLogo ();
  if (EFI_ERROR (Status)) {
    if (FirmwareVerLength > 0) {
      Print (VERSION_STRING_PREFIX L"%s\n",
        PcdGetPtr (PcdFirmwareVersionString));
    }
    Print (L"Press ESCAPE for boot options ");
  } else if (FirmwareVerLength > 0) {
    Status = gBS->HandleProtocol (gST->ConsoleOutHandle,
                    &gEfiGraphicsOutputProtocolGuid, (VOID **)&GraphicsOutput);
    if (!EFI_ERROR (Status)) {
      PosX = (GraphicsOutput->Mode->Info->HorizontalResolution -
              (StrLen (VERSION_STRING_PREFIX) + FirmwareVerLength) *
              EFI_GLYPH_WIDTH) / 2;
      PosY = 0;

      PrintXY (PosX, PosY, NULL, NULL, VERSION_STRING_PREFIX L"%s",
        PcdGetPtr (PcdFirmwareVersionString));
    }
  }

  //
  // On ARM, there is currently no reason to use the phased capsule
  // update approach where some capsules are dispatched before EndOfDxe
  // and some are dispatched after. So just handle all capsules here,
  // when the console is up and we can actually give the user some
  // feedback about what is going on.
  //
  HandleCapsules ();

  //
  // Register UEFI Shell
  //
  Key.ScanCode     = SCAN_NULL;
  Key.UnicodeChar  = L's';
  PlatformRegisterFvBootOption (&gUefiShellFileGuid, L"UEFI Shell", 0, &Key);
}

/**
  This function is called each second during the boot manager waits the
  timeout.

  @param TimeoutRemain  The remaining timeout.
**/
VOID
EFIAPI
PlatformBootManagerWaitCallback (
  UINT16          TimeoutRemain
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION Black;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION White;
  UINT16                              Timeout;
  EFI_STATUS                          Status;

  Timeout = PcdGet16 (PcdPlatformBootTimeOut);

  Black.Raw = 0x00000000;
  White.Raw = 0x00FFFFFF;

  Status = BootLogoUpdateProgress (
             White.Pixel,
             Black.Pixel,
             L"Press ESCAPE for boot options",
             White.Pixel,
             (Timeout - TimeoutRemain) * 100 / Timeout,
             0
             );
  if (EFI_ERROR (Status)) {
    Print (L".");
  }
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
  EFI_STATUS                   Status;
  EFI_BOOT_MANAGER_LOAD_OPTION BootManagerMenu;
  EFI_BOOT_MANAGER_LOAD_OPTION *BootOptions;
  UINTN                        OldBootOptionCount;
  UINTN                        NewBootOptionCount;

  //
  // Record the total number of boot configured boot options
  //
  BootOptions = EfiBootManagerGetLoadOptions (&OldBootOptionCount,
                  LoadOptionTypeBoot);
  EfiBootManagerFreeLoadOptions (BootOptions, OldBootOptionCount);

  //
  // Connect all devices, and regenerate all boot options
  //
  EfiBootManagerConnectAll ();
  EfiBootManagerRefreshAllBootOption ();

  //
  // Record the updated number of boot configured boot options
  //
  BootOptions = EfiBootManagerGetLoadOptions (&NewBootOptionCount,
                  LoadOptionTypeBoot);
  EfiBootManagerFreeLoadOptions (BootOptions, NewBootOptionCount);

  //
  // If the number of configured boot options has changed, reboot
  // the system so the new boot options will be taken into account
  // while executing the ordinary BDS bootflow sequence.
  // *Unless* persistent varstore is being emulated, since we would
  // then end up in an endless reboot loop.
  //
  if (!PcdGetBool (PcdEmuVariableNvModeEnable)) {
    if (NewBootOptionCount != OldBootOptionCount) {
      DEBUG ((DEBUG_WARN, "%a: rebooting after refreshing all boot options\n",
        __FUNCTION__));
      gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
    }
  }

  Status = EfiBootManagerGetBootManagerMenu (&BootManagerMenu);
  if (EFI_ERROR (Status)) {
    return;
  }

  for (;;) {
    EfiBootManagerBoot (&BootManagerMenu);
  }
}
