/** @file

  Copyright (c) 2004  - 2015, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  BdsPlatform.c

Abstract:

  This file include all platform action which can be customized
  by IBV/OEM.

--*/

#include "BdsPlatform.h"
#include "SetupMode.h"
#include <Guid/SetupVariable.h>
#include <Library/TcgPhysicalPresenceLib.h>
#include <Library/TrEEPhysicalPresenceLib.h>
#include <Protocol/I2cMasterMcg.h>
#include <TianoApi.h>
#include <PlatformBaseAddresses.h>
#include <Protocol/GlobalNvsArea.h>
#include <Library/DxeServicesTableLib.h>
#include <Protocol/BlockIo.h>
#include <PchRegs/PchRegsPcu.h>
#include <Library/S3BootScriptLib.h>
#include "PchAccess.h"
#include "PchRegs/PchRegsSata.h"
#include <Library/SerialPortLib.h>
#include <Library/DebugLib.h>

#include <Library/GenericBdsLib/InternalBdsLib.h>
#include <Library/GenericBdsLib/String.h>
#include <Library/NetLib.h>

EFI_GUID *ConnectDriverTable[] = {
  &gEfiMmioDeviceProtocolGuid,
  &gEfiI2cMasterProtocolGuid,
  &gEfiI2cHostProtocolGuid
};

#define SHELL_ENVIRONMENT_INTERFACE_PROTOCOL \
  { \
    0x47c7b221, 0xc42a, 0x11d2, 0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b \
  }
VOID               *mShellImageCallbackReg = NULL;



EFI_USER_PROFILE_HANDLE                           mCurrentUser = NULL;
EFI_EVENT                                         mHotKeyTimerEvent = NULL;
EFI_EVENT                                         mHitHotkeyEvent = NULL;
EFI_EVENT                                         mUsbKeyboardConnectEvent = NULL;
BOOLEAN                                           mHotKeyPressed = FALSE;
VOID                                              *mHitHotkeyRegistration;
#define KEYBOARD_TIMER_INTERVAL                   20000 // 0.02s

VOID
ConnectUSBController (
  VOID
  );

EFI_STATUS
PlatformBdsConnectSimpleConsole (
  IN BDS_CONSOLE_CONNECT_ENTRY   *PlatformConsole
);

VOID 
BootIntoFirmwareInterface(
  VOID
  );
  
VOID
EFIAPI
PlatformBdsInitHotKeyEvent (
  VOID
  );

VOID
EFIAPI
DisableAhciCtlr (
  IN EFI_EVENT                          Event,
  IN VOID                               *Context
  )
{
  UINT32                    PmcDisableAddress;
  UINT8                     SataStorageAmount;
  UINT32                    SataBase;
  UINT16                    SataPortStatus;


  DEBUG ((EFI_D_INFO, "Disable AHCI event is signalled\n"));
  SataStorageAmount = 0;
  SataBase = *(UINT32*) Context;

  //
  // BayTrail-M EDS chapter 16 ---- PCI IO Register Offset 92 (SATA Port Control and Status)
  //
  SataPortStatus = MmioRead16 (SataBase + R_PCH_SATA_PCS);

  //
  // Bit 8 EN: Port 0 Present
  //
  if ((SataPortStatus & 0x100) == 0x100) {
    SataStorageAmount++;
  }

  //
  // Bit 9 EN: Port 1 Present
  //
  if ((SataPortStatus & 0x200) == 0x200) {
    SataStorageAmount++;
  }

  //
  // Disable SATA controller when it sets to AHCI mode without carrying any devices
  // in order to prevent AHCI yellow bang under Win device manager.
  //
  if (SataStorageAmount == 0) {
    PmcDisableAddress = (MmioRead32 ((PCH_PCI_EXPRESS_BASE_ADDRESS + (UINT32) (31 << 15)) + R_PCH_LPC_PMC_BASE) & B_PCH_LPC_PMC_BASE_BAR) + R_PCH_PMC_FUNC_DIS;
    MmioOr32 (PmcDisableAddress, B_PCH_PMC_FUNC_DIS_SATA);
    S3BootScriptSaveMemWrite (
      EfiBootScriptWidthUint32,
      (UINTN) PmcDisableAddress,
      1,
      (VOID *) (UINTN) PmcDisableAddress
      );
  }
}

VOID
InstallReadyToLock (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_SMM_ACCESS2_PROTOCOL  *SmmAccess;
  EFI_ACPI_S3_SAVE_PROTOCOL *AcpiS3Save;

  //
  // Install DxeSmmReadyToLock protocol prior to the processing of boot options
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmmAccess2ProtocolGuid,
                  NULL,
                  (VOID **) &SmmAccess
                  );
  if (!EFI_ERROR (Status)) {

    //
    // Prepare S3 information, this MUST be done before DxeSmmReadyToLock
    //
    Status = gBS->LocateProtocol (
                    &gEfiAcpiS3SaveProtocolGuid,
                    NULL,
                    (VOID **)&AcpiS3Save
                    );
    if (!EFI_ERROR (Status)) {
      AcpiS3Save->S3Save (AcpiS3Save, NULL);
    }

    Handle = NULL;
    Status = gBS->InstallProtocolInterface (
                    &Handle,
                    &gExitPmAuthProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);

    Handle = NULL;
    Status = gBS->InstallProtocolInterface (
                    &Handle,
                    &gEfiDxeSmmReadyToLockProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return ;
}

VOID
EFIAPI
ShellImageCallback (
  IN EFI_EVENT                          Event,
  IN VOID                               *Context
  )
{
 BdsSetConsoleMode (TRUE);
 DEBUG ((EFI_D_INFO, "BdsEntry ShellImageCallback \n"));
}

//
// BDS Platform Functions
//
/**
  Platform Bds init. Incude the platform firmware vendor, revision
  and so crc check.

  @param VOID

  @retval  None.

**/
VOID
EFIAPI
PlatformBdsInit (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   ShellImageEvent;
  EFI_GUID    ShellEnvProtocol = SHELL_ENVIRONMENT_INTERFACE_PROTOCOL;

  #ifdef __GNUC__
  SerialPortWrite((UINT8 *)">>>>BdsEntry[GCC]\r\n", 19);
  #else
  SerialPortWrite((UINT8 *)">>>>BdsEntry\r\n", 14);
  #endif
  BdsLibSaveMemoryTypeInformation ();

  //
  // Before user authentication, the user identification devices need be connected
  // from the platform customized device paths
  //
  PlatformBdsConnectAuthDevice ();

  //
  // As console is not ready, the auto logon user will be identified.
  //
  BdsLibUserIdentify (&mCurrentUser);

  //
  // Change Gop mode when boot into Shell
  //
  if (mShellImageCallbackReg == NULL) {
    Status = gBS->CreateEvent (
                    EFI_EVENT_NOTIFY_SIGNAL,
                    EFI_TPL_CALLBACK,
                    ShellImageCallback,
                    NULL,
                    &ShellImageEvent
                    );
    if (!EFI_ERROR (Status)) {
      Status = gBS->RegisterProtocolNotify (
                      &ShellEnvProtocol,
                      ShellImageEvent,
                      &mShellImageCallbackReg
                      );

      DEBUG ((EFI_D_INFO, "BdsEntry ShellImageCallback \n"));
    }
  }
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

  UINTN                                 VarSize;
  SYSTEM_CONFIGURATION  mSystemConfiguration;

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

  //
  // Select display devices
  //
  VarSize = sizeof(SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable(
                  L"Setup",
                  &gEfiNormalSetupGuid,
                  NULL,
                  &VarSize,
                  &mSystemConfiguration
                  );
  if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &gEfiNormalSetupGuid,
              NULL,
              &VarSize,
              &mSystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }    

  if(mSystemConfiguration.BootDisplayDevice != 0x0)
  {
    ACPI_ADR_DEVICE_PATH         AcpiAdr;
    EFI_DEVICE_PATH_PROTOCOL  *MyDevicePath = NULL;

    AcpiAdr.Header.Type     = ACPI_DEVICE_PATH;
    AcpiAdr.Header.SubType  = ACPI_ADR_DP;

    switch (mSystemConfiguration.BootDisplayDevice) {
    case 1:
      AcpiAdr.ADR= ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, PORT_CRT, 0);    //CRT Device
      break;
    case 2:
      AcpiAdr.ADR= ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_EXTERNAL_DIGITAL, PORT_B_HDMI, 0);  //HDMI Device Port B
      break;
    case 3:
      AcpiAdr.ADR= ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_EXTERNAL_DIGITAL, PORT_B_DP, 0);    //DP PortB
      break;
    case 4:
      AcpiAdr.ADR= ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_EXTERNAL_DIGITAL, PORT_C_DP, 0);    //DP PortC
      break;
    case 5:
      AcpiAdr.ADR= ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_INTERNAL_DIGITAL, PORT_C_DP, 0);    //eDP Port C
      break;
    case 6:
      AcpiAdr.ADR= ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_INTERNAL_DIGITAL, PORT_MIPI_A, 0);  //DSI Port A
      break;
    case 7:
      AcpiAdr.ADR= ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_INTERNAL_DIGITAL, PORT_MIPI_C, 0);  //DSI Port C
      break;
    default:
      AcpiAdr.ADR= ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, PORT_CRT, 0);
      break;
    }

    SetDevicePathNodeLength (&AcpiAdr.Header, sizeof (ACPI_ADR_DEVICE_PATH));

    MyDevicePath = AppendDevicePathNode(MyDevicePath, (EFI_DEVICE_PATH_PROTOCOL*)&AcpiAdr);

    gBS->ConnectController (
           PciDeviceHandle,
           NULL,
           MyDevicePath,
           FALSE
           );

    FreePool(MyDevicePath);
  }
  else
  {
    gBS->ConnectController (
           PciDeviceHandle,
           NULL,
           NULL,
           FALSE
           );
  }

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
                      (VOID**)&TempDevicePath
                      );
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
      }
    }
    gBS->FreePool (GopHandleBuffer);
  }

  return EFI_SUCCESS;
}

/**

  Search out all the platform pci or agp video device. The function may will
  find multiple video device, and return all enabled device path.

  @param PlugInPciVgaDevicePath    Return the platform plug in pci video device
                                   path if the system have plug in pci video device.
  @param OnboardPciVgaDevicePath   Return the platform active agp video device path
                                   if the system have plug in agp video device or on
                                   chip agp device.

  @retval EFI_SUCCSS               Get all platform active video device path.
  @retval EFI_STATUS               Return the status of gBS->LocateDevicePath (),
                                   gBS->ConnectController (),
                                   and gBS->LocateHandleBuffer ().

**/
EFI_STATUS
GetPlugInPciVgaDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **PlugInPciVgaDevicePath,
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **OnboardPciVgaDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                RootHandle;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  UINTN                     Index1;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  BOOLEAN                   PlugInPciVga;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;

  DevicePath = NULL;
  PlugInPciVga = TRUE;
  HandleCount = 0;
  HandleBuffer = NULL;

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

  Status = gBS->ConnectController (
                  RootHandle,
                  NULL,
                  NULL,
                  FALSE
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Start to check all the pci io to find all possible VGA device
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
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID**)&PciIo
                    );
    if (!EFI_ERROR (Status)) {

      //
      // Check for all VGA device
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

      //
      // Here we decide which VGA device to enable in PCI bus
      //
      // The first plugin PCI VGA card device will be present as PCI VGA
      // The onchip AGP or AGP card will be present as AGP VGA
      //
      if (!IS_PCI_VGA (&Pci)) {
        continue;
      }

      //
      // Set the device as the possible console out device,
      //
      // Below code will make every VGA device to be one
      // of the possibe console out device
      //
      PlugInPciVga = TRUE;
      gBS->HandleProtocol (
             HandleBuffer[Index],
             &gEfiDevicePathProtocolGuid,
             (VOID**)&DevicePath
             );

      Index1 = 0;

      while (gPlatformAllPossiblePciVgaConsole[Index1] != NULL) {
        if (CompareMem (
              DevicePath,
              gPlatformAllPossiblePciVgaConsole[Index1],
              GetDevicePathSize (gPlatformAllPossiblePciVgaConsole[Index1])
              ) == 0) {

          //
          // This device is an AGP device
          //
          *OnboardPciVgaDevicePath = DevicePath;
          PlugInPciVga = FALSE;
          break;
        }

        Index1 ++;
      }

      if (PlugInPciVga) {
        *PlugInPciVgaDevicePath = DevicePath;
      }
    }
  }

  FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

/**

  Find the platform  active vga, and base on the policy to enable the vga as
  the console out device. The policy is driven by one setup variable "VBIOS".

  None.

  @param EFI_UNSUPPORTED         There is no active vga device

  @retval EFI_STATUS             Return the status of BdsLibGetVariableAndSize ()

**/
EFI_STATUS
PlatformBdsForceActiveVga (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *PlugInPciVgaDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *OnboardPciVgaDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathFirst;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathSecond;
  EFI_DEVICE_PATH_PROTOCOL  *GopDevicePath;
  UINTN                VarSize;
  SYSTEM_CONFIGURATION  mSystemConfiguration;

  Status = EFI_SUCCESS;
  PlugInPciVgaDevicePath = NULL;
  OnboardPciVgaDevicePath = NULL;

  //
  // Check the policy which is the first enabled VGA
  //
  GetPlugInPciVgaDevicePath (&PlugInPciVgaDevicePath, &OnboardPciVgaDevicePath);

  if (PlugInPciVgaDevicePath == NULL && OnboardPciVgaDevicePath == NULL) {
    return EFI_UNSUPPORTED;
  }

  VarSize = sizeof(SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable(
                  L"Setup",
                  &gEfiNormalSetupGuid,
                  NULL,
                  &VarSize,
                  &mSystemConfiguration
                  );
  if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &gEfiNormalSetupGuid,
              NULL,
              &VarSize,
              &mSystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }    


  if ((PlugInPciVgaDevicePath == NULL && OnboardPciVgaDevicePath != NULL) ) {
    DEBUG ((EFI_D_ERROR,"Update onboard PCI VGA ...\n"));
    DevicePathFirst  = OnboardPciVgaDevicePath;
    DevicePathSecond = PlugInPciVgaDevicePath;
    goto UpdateConOut;
  }
  if(OnboardPciVgaDevicePath != NULL && mSystemConfiguration.PrimaryVideoAdaptor == 0) {
    DEBUG ((EFI_D_ERROR,"Update onboard PCI VGA When set primary!!!...\n"));
    DevicePathFirst  = OnboardPciVgaDevicePath;
    DevicePathSecond = PlugInPciVgaDevicePath;
    goto UpdateConOut;
  }

  DEBUG ((EFI_D_ERROR,"Update plug in PCI VGA ...\n"));
  DevicePathFirst  = PlugInPciVgaDevicePath;
  DevicePathSecond = OnboardPciVgaDevicePath;

UpdateConOut:
  GetGopDevicePath (DevicePathFirst, &GopDevicePath);
  DevicePathFirst = GopDevicePath;

  Status = BdsLibUpdateConsoleVariable (
             L"ConOut",
             DevicePathFirst,
             DevicePathSecond
             );

  return Status;
}

VOID
UpdateConsoleResolution(
  VOID
  )
{
  UINT32                 HorizontalResolution;
  UINT32                 VerticalResolution;
  SYSTEM_CONFIGURATION   SystemConfiguration;
  UINTN                  VarSize;
  EFI_STATUS             Status;


  HorizontalResolution = PcdGet32 (PcdSetupVideoHorizontalResolution);
  VerticalResolution = PcdGet32 (PcdSetupVideoVerticalResolution);

  VarSize = sizeof(SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable(
                  L"Setup",
                  &gEfiNormalSetupGuid,
                  NULL,
                  &VarSize,
                  &SystemConfiguration
                  );
  if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &gEfiNormalSetupGuid,
              NULL,
              &VarSize,
              &SystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }  

  switch (SystemConfiguration.IgdFlatPanel) {

  case 0:
    //
    // Use the detault PCD values.
    //
    break;

  case 1:
    HorizontalResolution = 640;
    VerticalResolution = 480;
    break;

  case 2:
    HorizontalResolution = 800;
    VerticalResolution = 600;
    break;

  case 3:
    HorizontalResolution = 1024;
    VerticalResolution = 768;
    break;

  case 4:
  HorizontalResolution = 1280;
  VerticalResolution = 1024;
  break;

  case 5:
  HorizontalResolution = 1366;
  VerticalResolution = 768;
  break;

  case 6:
  HorizontalResolution = 1680;
  VerticalResolution = 1050;
  break;

  case 7:
  HorizontalResolution = 1920;
  VerticalResolution = 1200;
  break;

  case 8:
  HorizontalResolution = 1280;
  VerticalResolution = 800;
  break;
  }

  PcdSet32 (PcdSetupVideoHorizontalResolution, HorizontalResolution);
  PcdSet32 (PcdSetupVideoVerticalResolution, VerticalResolution);
  DEBUG ((EFI_D_ERROR, "HorizontalResolution = %x; VerticalResolution = %x", HorizontalResolution, VerticalResolution));

  return;
}

/**
  Connect the predefined platform default console device. Always try to find
  and enable the vga device if have.

  @param PlatformConsole    Predfined platform default console device array.

  @retval EFI_SUCCESS       Success connect at least one ConIn and ConOut
                            device, there must have one ConOut device is
                            active vga device.

  @retval EFI_STATUS        Return the status of
                            BdsLibConnectAllDefaultConsoles ()

**/
EFI_STATUS
PlatformBdsConnectConsole (
  IN BDS_CONSOLE_CONNECT_ENTRY   *PlatformConsole
)
{
  EFI_STATUS                         Status;
  UINTN                              Index;
  EFI_DEVICE_PATH_PROTOCOL           *VarConout;
  EFI_DEVICE_PATH_PROTOCOL           *VarConin;
  UINTN                              DevicePathSize;

  UpdateConsoleResolution();

  Index = 0;
  Status = EFI_SUCCESS;
  DevicePathSize = 0;
  VarConout = BdsLibGetVariableAndSize (
                L"ConOut",
                &gEfiGlobalVariableGuid,
                &DevicePathSize
                );
  VarConin = BdsLibGetVariableAndSize (
               L"ConIn",
               &gEfiGlobalVariableGuid,
               &DevicePathSize
               );
  if (VarConout == NULL || VarConin == NULL) {
    //
    // Have chance to connect the platform default console,
    // the platform default console is the minimue device group
    // the platform should support
    //
    while (PlatformConsole[Index].DevicePath != NULL) {

      //
      // Update the console variable with the connect type
      //
      if ((PlatformConsole[Index].ConnectType & CONSOLE_IN) == CONSOLE_IN) {
        BdsLibUpdateConsoleVariable (L"ConIn", PlatformConsole[Index].DevicePath, NULL);
      }

      if ((PlatformConsole[Index].ConnectType & CONSOLE_OUT) == CONSOLE_OUT) {
        BdsLibUpdateConsoleVariable (L"ConOut", PlatformConsole[Index].DevicePath, NULL);
      }

      if ((PlatformConsole[Index].ConnectType & STD_ERROR) == STD_ERROR) {
        BdsLibUpdateConsoleVariable (L"ErrOut", PlatformConsole[Index].DevicePath, NULL);
      }

      Index ++;
    }
  }

  //
  // Make sure we have at least one active VGA, and have the right
  // active VGA in console variable
  //
  Status = PlatformBdsForceActiveVga ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((EFI_D_INFO, "DISPLAY INIT DONE\n"));

  //
  // Connect the all the default console with current console variable
  //
  Status = BdsLibConnectAllDefaultConsoles ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Connect with predeined platform connect sequence,
  the OEM/IBV can customize with their own connect sequence.

  @param None.

  @retval None.

**/
VOID
PlatformBdsConnectSequence (
  VOID
  )
{
  UINTN                     Index;

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
    Index ++;
  }

  //
  // Just use the simple policy to connect all devices
  // There should be no difference between debug tip and release tip, or it will be extremely hard to debug.
  //
  // There is case that IdeController driver will write boot script in driver model Start() function. It will be rejected by boot script save.
  // It is only found when DEBUG disabled, because we are using BdsLibConnectAll() when DEBUG enabled.
  //
  // So we use BdsLibConnectAll() here to make sure IdeController.Start() is invoked before InstallReadyToLock().
  // We may also consider to connect SataController only later if needed.
  //
  BdsLibConnectAll ();
}

/**

  Load the predefined driver option, OEM/IBV can customize this
  to load their own drivers

  @param BdsDriverLists  The header of the driver option link list.

  @retval None.

**/
VOID
PlatformBdsGetDriverOption (
  IN OUT LIST_ENTRY                  *BdsDriverLists
  )
{
  UINTN                              Index;

  Index = 0;

  //
  // Here we can get the customized platform driver option
  //
  while (gPlatformDriverOption[Index] != NULL) {

    //
    // Build the platform boot option
    //
    BdsLibRegisterNewOption (BdsDriverLists, gPlatformDriverOption[Index], NULL, L"DriverOrder");
    Index ++;
  }

}

/**
  This function is used for some critical time if the the system
  have no any boot option, and there is no time out for user to add
  the new boot option. This can also treat as the platform default
  boot option.

  @param BdsBootOptionList   The header of the boot option link list.

  @retval None.

**/
VOID
PlatformBdsPredictBootOption (
  IN OUT LIST_ENTRY                  *BdsBootOptionList
  )
{
  UINTN                              Index;

  Index = 0;

  //
  // Here give chance to get platform boot option data
  //
  while (gPlatformBootOption[Index] != NULL) {

    //
    // Build the platform boot option
    //
    BdsLibRegisterNewOption (BdsBootOptionList, gPlatformBootOption[Index], NULL, L"BootOrder");
    Index ++;
  }
}

/**
  Perform the platform diagnostic, such like test memory. OEM/IBV also
  can customize this fuction to support specific platform diagnostic.

  @param MemoryTestLevel   The memory test intensive level
  @param QuietBoot         Indicate if need to enable the quiet boot
  @param BaseMemoryTest    A pointer to BdsMemoryTest()

  @retval  None.

**/
VOID
PlatformBdsDiagnostics (
  IN EXTENDMEM_COVERAGE_LEVEL    MemoryTestLevel,
  IN BOOLEAN                     QuietBoot,
  IN BASEM_MEMORY_TEST           BaseMemoryTest
  )
{
  EFI_STATUS                     Status;

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

    return;
  }

  //
  // Perform system diagnostic
  //
  Status = BaseMemoryTest (MemoryTestLevel);
}


/**
  For EFI boot option, BDS separate them as six types:
  1. Network - The boot option points to the SimpleNetworkProtocol device.
               Bds will try to automatically create this type boot option when enumerate.
  2. Shell   - The boot option points to internal flash shell.
               Bds will try to automatically create this type boot option when enumerate.
  3. Removable BlockIo      - The boot option only points to the removable media
                              device, like USB flash disk, DVD, Floppy etc.
                              These device should contain a *removable* blockIo
                              protocol in their device handle.
                              Bds will try to automatically create this type boot option
                              when enumerate.
  4. Fixed BlockIo          - The boot option only points to a Fixed blockIo device,
                              like HardDisk.
                              These device should contain a *fixed* blockIo
                              protocol in their device handle.
                              BDS will skip fixed blockIo devices, and NOT
                              automatically create boot option for them. But BDS
                              will help to delete those fixed blockIo boot option,
                              whose description rule conflict with other auto-created
                              boot options.
  5. Non-BlockIo Simplefile - The boot option points to a device whose handle
                              has SimpleFileSystem Protocol, but has no blockio
                              protocol. These devices do not offer blockIo
                              protocol, but BDS still can get the
                              \EFI\BOOT\boot{machinename}.EFI by SimpleFileSystem
                              Protocol.
  6. File    - The boot option points to a file. These boot options are usually
               created by user manually or OS loader. BDS will not delete or modify
               these boot options.

  This function will enumerate all possible boot device in the system, and
  automatically create boot options for Network, Shell, Removable BlockIo,
  and Non-BlockIo Simplefile devices.
  It will only execute once of every boot.

  @param  BdsBootOptionList      The header of the link list which indexed all
                                 current boot options

  @retval EFI_SUCCESS            Finished all the boot device enumerate and create
                                 the boot option base on that boot device

  @retval EFI_OUT_OF_RESOURCES   Failed to enumerate the boot device and create the boot option list
**/
EFI_STATUS
EFIAPI
PlatformBdsLibEnumerateAllBootOption (
  IN OUT LIST_ENTRY          *BdsBootOptionList
  )
{
  EFI_STATUS                    Status;
  UINT16                        FloppyNumber;
  UINT16                        HarddriveNumber;
  UINT16                        CdromNumber;
  UINT16                        UsbNumber;
  UINT16                        MiscNumber;
  UINT16                        ScsiNumber;
  UINT16                        NonBlockNumber;
  UINTN                         NumberBlockIoHandles;
  EFI_HANDLE                    *BlockIoHandles;
  EFI_BLOCK_IO_PROTOCOL         *BlkIo;
  BOOLEAN                       Removable[2];
  UINTN                         RemovableIndex;
  UINTN                         Index;
  UINTN                         NumOfLoadFileHandles;
  EFI_HANDLE                    *LoadFileHandles;
  UINTN                         FvHandleCount;
  EFI_HANDLE                    *FvHandleBuffer;
  EFI_FV_FILETYPE               Type;
  UINTN                         Size;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINT32                        AuthenticationStatus;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  UINTN                         DevicePathType;
  CHAR16                        Buffer[40];
  EFI_HANDLE                    *FileSystemHandles;
  UINTN                         NumberFileSystemHandles;
  BOOLEAN                       NeedDelete;
  EFI_IMAGE_DOS_HEADER          DosHeader;
  CHAR8                         *PlatLang;
  CHAR8                         *LastLang;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       HdrData;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;
  CHAR16                        *MacStr;
  CHAR16                        *IPverStr;
  EFI_HANDLE                    *NetworkHandles;
  UINTN                         BufferSize;

  FloppyNumber    = 0;
  HarddriveNumber = 0;
  CdromNumber     = 0;
  UsbNumber       = 0;
  MiscNumber      = 0;
  ScsiNumber      = 0;
  PlatLang        = NULL;
  LastLang        = NULL;
  ZeroMem (Buffer, sizeof (Buffer));

  //
  // If the boot device enumerate happened, just get the boot
  // device from the boot order variable
  //
  if (mEnumBootDevice) {
    GetVariable2 (LAST_ENUM_LANGUAGE_VARIABLE_NAME, &gLastEnumLangGuid, (VOID**)&LastLang, NULL);
    GetEfiGlobalVariable2 (L"PlatformLang", (VOID**)&PlatLang, NULL);
    ASSERT (PlatLang != NULL);
    if ((LastLang != NULL) && (AsciiStrCmp (LastLang, PlatLang) == 0)) {
      Status = BdsLibBuildOptionFromVar (BdsBootOptionList, L"BootOrder");
      FreePool (LastLang);
      FreePool (PlatLang);
      return Status;
    } else {
      Status = gRT->SetVariable (
        LAST_ENUM_LANGUAGE_VARIABLE_NAME,
        &gLastEnumLangGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
        AsciiStrSize (PlatLang),
        PlatLang
        );
      //
      // Failure to set the variable only impacts the performance next time enumerating the boot options.
      //

      if (LastLang != NULL) {
        FreePool (LastLang);
      }
      FreePool (PlatLang);
    }
  }

  //
  // Notes: this dirty code is to get the legacy boot option from the
  // BBS table and create to variable as the EFI boot option, it should
  // be removed after the CSM can provide legacy boot option directly
  //
  REFRESH_LEGACY_BOOT_OPTIONS;

  //
  // Delete invalid boot option
  //
  BdsDeleteAllInvalidEfiBootOption ();

  //
  // Parse removable media followed by fixed media.
  // The Removable[] array is used by the for-loop below to create removable media boot options 
  // at first, and then to create fixed media boot options.
  //
  Removable[0]  = FALSE;
  Removable[1]  = TRUE;

  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiBlockIoProtocolGuid,
        NULL,
        &NumberBlockIoHandles,
        &BlockIoHandles
        );

  for (RemovableIndex = 0; RemovableIndex < 2; RemovableIndex++) {
    for (Index = 0; Index < NumberBlockIoHandles; Index++) {
      Status = gBS->HandleProtocol (
                      BlockIoHandles[Index],
                      &gEfiBlockIoProtocolGuid,
                      (VOID **) &BlkIo
                      );
      //
      // skip the logical partition
      //
      if (EFI_ERROR (Status) || BlkIo->Media->LogicalPartition) {
        continue;
      }

      //
      // firstly fixed block io then the removable block io
      //
      if (BlkIo->Media->RemovableMedia == Removable[RemovableIndex]) {
        continue;
      }
      DevicePath  = DevicePathFromHandle (BlockIoHandles[Index]);
      DevicePathType = BdsGetBootTypeFromDevicePath (DevicePath);

      switch (DevicePathType) {
      case BDS_EFI_ACPI_FLOPPY_BOOT:
        if (FloppyNumber != 0) {
          UnicodeSPrint (Buffer, sizeof (Buffer), L"%s %d", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_FLOPPY)), FloppyNumber);
        } else {
          UnicodeSPrint (Buffer, sizeof (Buffer), L"%s", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_FLOPPY)));
        }
        BdsLibBuildOptionFromHandle (BlockIoHandles[Index], BdsBootOptionList, Buffer);
        FloppyNumber++;
        break;

      //
      // Assume a removable SATA device should be the DVD/CD device, a fixed SATA device should be the Hard Drive device.
      //
      case BDS_EFI_MESSAGE_ATAPI_BOOT:
      case BDS_EFI_MESSAGE_SATA_BOOT:
        if (BlkIo->Media->RemovableMedia) {
          if (CdromNumber != 0) {
            UnicodeSPrint (Buffer, sizeof (Buffer), L"%s %d", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_CD_DVD)), CdromNumber);
          } else {
            UnicodeSPrint (Buffer, sizeof (Buffer), L"%s", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_CD_DVD)));
          }
          CdromNumber++;
        } else {
          if (HarddriveNumber != 0) {
            UnicodeSPrint (Buffer, sizeof (Buffer), L"%s %d", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_HARDDRIVE)), HarddriveNumber);
          } else {
            UnicodeSPrint (Buffer, sizeof (Buffer), L"%s", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_HARDDRIVE)));
          }
          HarddriveNumber++;
        }
        DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Buffer: %S\n", Buffer));
        BdsLibBuildOptionFromHandle (BlockIoHandles[Index], BdsBootOptionList, Buffer);
        break;

      case BDS_EFI_MESSAGE_USB_DEVICE_BOOT:
        if (UsbNumber != 0) {
          UnicodeSPrint (Buffer, sizeof (Buffer), L"%s %d", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_USB)), UsbNumber);
        } else {
          UnicodeSPrint (Buffer, sizeof (Buffer), L"%s", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_USB)));
        }
        BdsLibBuildOptionFromHandle (BlockIoHandles[Index], BdsBootOptionList, Buffer);
        UsbNumber++;
        break;

      case BDS_EFI_MESSAGE_SCSI_BOOT:
        if (ScsiNumber != 0) {
          UnicodeSPrint (Buffer, sizeof (Buffer), L"%s %d", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_SCSI)), ScsiNumber);
        } else {
          UnicodeSPrint (Buffer, sizeof (Buffer), L"%s", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_SCSI)));
        }
        BdsLibBuildOptionFromHandle (BlockIoHandles[Index], BdsBootOptionList, Buffer);
        ScsiNumber++;
        break;

      case BDS_EFI_MESSAGE_MISC_BOOT:
      default:
        if (MiscNumber != 0) {
          UnicodeSPrint (Buffer, sizeof (Buffer), L"%s %d", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_MISC)), MiscNumber);
        } else {
          UnicodeSPrint (Buffer, sizeof (Buffer), L"%s", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_MISC)));
        }
        BdsLibBuildOptionFromHandle (BlockIoHandles[Index], BdsBootOptionList, Buffer);
        MiscNumber++;
        break;
      }
    }
  }

  if (NumberBlockIoHandles != 0) {
    FreePool (BlockIoHandles);
  }

  //
  // If there is simple file protocol which does not consume block Io protocol, create a boot option for it here.
  //
  NonBlockNumber = 0;
  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &NumberFileSystemHandles,
        &FileSystemHandles
        );
  for (Index = 0; Index < NumberFileSystemHandles; Index++) {
    Status = gBS->HandleProtocol (
                    FileSystemHandles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlkIo
                    );
     if (!EFI_ERROR (Status)) {
      //
      //  Skip if the file system handle supports a BlkIo protocol,
      //
      continue;
    }

    //
    // Do the removable Media thing. \EFI\BOOT\boot{machinename}.EFI
    //  machinename is ia32, ia64, x64, ...
    //
    Hdr.Union  = &HdrData;
    NeedDelete = TRUE;
    Status     = BdsLibGetImageHeader (
                   FileSystemHandles[Index],
                   EFI_REMOVABLE_MEDIA_FILE_NAME,
                   &DosHeader,
                   Hdr
                   );
    if (!EFI_ERROR (Status) &&
        EFI_IMAGE_MACHINE_TYPE_SUPPORTED (Hdr.Pe32->FileHeader.Machine) &&
        Hdr.Pe32->OptionalHeader.Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION) {
      NeedDelete = FALSE;
    }

    if (NeedDelete) {
      //
      // No such file or the file is not a EFI application, delete this boot option
      //
      BdsLibDeleteOptionFromHandle (FileSystemHandles[Index]);
    } else {
      if (NonBlockNumber != 0) {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"%s %d", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_NON_BLOCK)), NonBlockNumber);
      } else {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"%s", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_NON_BLOCK)));
      }
      BdsLibBuildOptionFromHandle (FileSystemHandles[Index], BdsBootOptionList, Buffer);
      NonBlockNumber++;
    }
  }

  if (NumberFileSystemHandles != 0) {
    FreePool (FileSystemHandles);
  }

  //
  // Check if we have on flash shell
  //
  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiFirmwareVolume2ProtocolGuid,
        NULL,
        &FvHandleCount,
        &FvHandleBuffer
        );
  for (Index = 0; Index < FvHandleCount; Index++) {
    gBS->HandleProtocol (
          FvHandleBuffer[Index],
          &gEfiFirmwareVolume2ProtocolGuid,
          (VOID **) &Fv
          );

    Status = Fv->ReadFile (
                  Fv,
                  PcdGetPtr(PcdShellFile),
                  NULL,
                  &Size,
                  &Type,
                  &Attributes,
                  &AuthenticationStatus
                  );
    if (EFI_ERROR (Status)) {
      //
      // Skip if no shell file in the FV
      //
      continue;
    }
    //
    // Build the shell boot option
    //
    BdsLibBuildOptionFromShell (FvHandleBuffer[Index], BdsBootOptionList);
  }

  if (FvHandleCount != 0) {
    FreePool (FvHandleBuffer);
  }

  //
  // Parse Network Boot Device
  //
  NumOfLoadFileHandles = 0;
  //
  // Search Load File protocol for PXE boot option.
  //
  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiLoadFileProtocolGuid,
        NULL,
        &NumOfLoadFileHandles,
        &LoadFileHandles
        );

  for (Index = 0; Index < NumOfLoadFileHandles; Index++) {

//
//Locate EFI_DEVICE_PATH_PROTOCOL to dynamically get IPv4/IPv6 protocol information.
//

 Status = gBS->HandleProtocol (
                  LoadFileHandles[Index],
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath
                  );
  
 ASSERT_EFI_ERROR (Status);

  while (!IsDevicePathEnd (DevicePath)) {
    if ((DevicePath->Type == MESSAGING_DEVICE_PATH) &&
        (DevicePath->SubType == MSG_IPv4_DP)) {

  //
  //Get handle infomation
  //
  BufferSize = 0;
  NetworkHandles = NULL;
  Status = gBS->LocateHandle (
                  ByProtocol, 
                  &gEfiSimpleNetworkProtocolGuid,
                  NULL,
                  &BufferSize,
                  NetworkHandles
                  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    NetworkHandles = AllocateZeroPool(BufferSize);
    if (NetworkHandles == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
    Status = gBS->LocateHandle(
                    ByProtocol,
                    &gEfiSimpleNetworkProtocolGuid,
                    NULL,
                    &BufferSize,
                    NetworkHandles
                    );
 }
               
  //
  //Get the MAC string
  //
  Status = NetLibGetMacString (
             *NetworkHandles,
             NULL,
             &MacStr
             );
  if (EFI_ERROR (Status)) {	
    return Status;
  }
  IPverStr = L" IPv4";
  UnicodeSPrint (Buffer, sizeof (Buffer), L"%s%s%s", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_NETWORK)),MacStr,IPverStr);
  break;
  }
    if((DevicePath->Type == MESSAGING_DEVICE_PATH) &&
        (DevicePath->SubType == MSG_IPv6_DP)) {

  //
  //Get handle infomation
  //
  BufferSize = 0;
  NetworkHandles = NULL;
  Status = gBS->LocateHandle (
                  ByProtocol, 
                  &gEfiSimpleNetworkProtocolGuid,
                  NULL,
                  &BufferSize,
                  NetworkHandles
                  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    NetworkHandles = AllocateZeroPool(BufferSize);
    if (NetworkHandles == NULL) {
       return (EFI_OUT_OF_RESOURCES);
    }
    Status = gBS->LocateHandle(
                    ByProtocol,
                    &gEfiSimpleNetworkProtocolGuid,
                    NULL,
                    &BufferSize,
                    NetworkHandles
                    );
 }
                    
  //
  //Get the MAC string
  //
  Status = NetLibGetMacString (
             *NetworkHandles,
             NULL,
             &MacStr
             );
  if (EFI_ERROR (Status)) {	
    return Status;
  }
      IPverStr = L" IPv6";
      UnicodeSPrint (Buffer, sizeof (Buffer), L"%s%s%s", BdsLibGetStringById (STRING_TOKEN (STR_DESCRIPTION_NETWORK)),MacStr,IPverStr);
      break;
    }
    DevicePath = NextDevicePathNode (DevicePath);
  }
  
    BdsLibBuildOptionFromHandle (LoadFileHandles[Index], BdsBootOptionList, Buffer);
  }

  if (NumOfLoadFileHandles != 0) {
    FreePool (LoadFileHandles);
  }

  //
  // Check if we have on flash shell
  //
 /* gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiFirmwareVolume2ProtocolGuid,
        NULL,
        &FvHandleCount,
        &FvHandleBuffer
        );
  for (Index = 0; Index < FvHandleCount; Index++) {
    gBS->HandleProtocol (
          FvHandleBuffer[Index],
          &gEfiFirmwareVolume2ProtocolGuid,
          (VOID **) &Fv
          );

    Status = Fv->ReadFile (
                  Fv,
                  PcdGetPtr(PcdShellFile),
                  NULL,
                  &Size,
                  &Type,
                  &Attributes,
                  &AuthenticationStatus
                  );
    if (EFI_ERROR (Status)) {
      //
      // Skip if no shell file in the FV
      //
      continue;
    }
    //
    // Build the shell boot option
    //
    BdsLibBuildOptionFromShell (FvHandleBuffer[Index], BdsBootOptionList);
  }

  if (FvHandleCount != 0) {
    FreePool (FvHandleBuffer);
  } */
  
  //
  // Make sure every boot only have one time
  // boot device enumerate
  //
  Status = BdsLibBuildOptionFromVar (BdsBootOptionList, L"BootOrder");
  mEnumBootDevice = TRUE;

  return Status;
} 



/**

  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.

  @param DriverOptionList - The header of the driver option link list
  @param  BootOptionList   - The header of the boot option link list
  @param ProcessCapsules  - A pointer to ProcessCapsules()
  @param BaseMemoryTest   - A pointer to BaseMemoryTest()

  @retval None.

**/
VOID
EFIAPI
PlatformBdsPolicyBehavior (
  IN OUT LIST_ENTRY                  *DriverOptionList,
  IN OUT LIST_ENTRY                  *BootOptionList,
  IN PROCESS_CAPSULES                ProcessCapsules,
  IN BASEM_MEMORY_TEST               BaseMemoryTest
  )
{
  EFI_STATUS                         Status;
  UINT16                             Timeout;
  EFI_BOOT_MODE                      BootMode;
  BOOLEAN                            DeferredImageExist;
  UINTN                              Index;
  CHAR16                             CapsuleVarName[36];
  CHAR16                             *TempVarName;
  SYSTEM_CONFIGURATION               SystemConfiguration;
  UINTN                              VarSize;
  BOOLEAN                            SetVariableFlag;
  PLATFORM_PCI_DEVICE_PATH           *EmmcBootDevPath;
  EFI_GLOBAL_NVS_AREA_PROTOCOL       *GlobalNvsArea;
  EFI_HANDLE                         FvProtocolHandle;
  UINTN                              HandleCount;
  EFI_HANDLE                         *HandleBuffer;
  UINTN                              Index1;
  UINTN                              SataPciRegBase = 0;
  UINT16                             SataModeSelect = 0;
  VOID                               *RegistrationExitPmAuth = NULL;
  EFI_EVENT                          Event;
  BOOLEAN                            IsFirstBoot;
  UINT16                             *BootOrder;
  UINTN                              BootOrderSize;

  Timeout = PcdGet16 (PcdPlatformBootTimeOut);
  if (Timeout > 10 ) {
    //we think the Timeout variable is corrupted
    Timeout = 10;
  }
  	
  VarSize = sizeof(SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable(
                  NORMAL_SETUP_NAME,
                  &gEfiNormalSetupGuid,
                  NULL,
                  &VarSize,
                  &SystemConfiguration
                  );

  if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &gEfiNormalSetupGuid,
              NULL,
              &VarSize,
              &SystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }  

  //
  // Load the driver option as the driver option list
  //
  PlatformBdsGetDriverOption (DriverOptionList);

  //
  // Get current Boot Mode
  //
  BootMode = GetBootModeHob();

  //
  // Clear all the capsule variables CapsuleUpdateData, CapsuleUpdateData1, CapsuleUpdateData2...
  // as early as possible which will avoid the next time boot after the capsule update
  // will still into the capsule loop
  //
  StrCpy (CapsuleVarName, EFI_CAPSULE_VARIABLE_NAME);
  TempVarName = CapsuleVarName + StrLen (CapsuleVarName);
  Index = 0;
  SetVariableFlag = TRUE;
  while (SetVariableFlag) {
    if (Index > 0) {
      UnicodeValueToString (TempVarName, 0, Index, 0);
    }
    Status = gRT->SetVariable (
                    CapsuleVarName,
                    &gEfiCapsuleVendorGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS |
                    EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    0,
                    (VOID *)NULL
                    );
    if (EFI_ERROR (Status)) {
      //
      // There is no capsule variables, quit
      //
      SetVariableFlag = FALSE;
      continue;
    }
    Index++;
  }

  //
  // No deferred images exist by default
  //
  DeferredImageExist = FALSE;
  if ((BootMode != BOOT_WITH_MINIMAL_CONFIGURATION) && (PcdGet32(PcdFlashFvShellSize) > 0)){
    gDS->ProcessFirmwareVolume (
           (VOID *)(UINTN)PcdGet32(PcdFlashFvShellBase),
           PcdGet32(PcdFlashFvShellSize),
           &FvProtocolHandle
           );
  }

  if (SystemConfiguration.FastBoot == 1) {
    BootOrder = BdsLibGetVariableAndSize (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  &BootOrderSize
                  );
    if ((BootOrder != NULL) && (BootMode != BOOT_ON_FLASH_UPDATE)) {
      //
      // BootOrder exist, it means system has boot before. We can do fast boot.
      //
      BootMode = BOOT_WITH_MINIMAL_CONFIGURATION;
    }
  }


  //
  // Use eMMC to boot OS and turn on AHCI, when SATA HDD is diconnected,
  // SATA AHCI CTLR device will show yellow bang, implement this solution to solve it.
  //
  SataPciRegBase  = MmPciAddress (0, 0, PCI_DEVICE_NUMBER_PCH_SATA, 0, 0);
  SataModeSelect  = MmioRead16 (SataPciRegBase + R_PCH_SATA_MAP) & B_PCH_SATA_MAP_SMS_MASK;
  Status          = EFI_SUCCESS;
  if (SataModeSelect != V_PCH_SATA_MAP_SMS_IDE) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    DisableAhciCtlr,
                    &SataPciRegBase,
                    &Event
                     );
    if (!EFI_ERROR (Status)) {
      Status = gBS->RegisterProtocolNotify (
                      &gExitPmAuthProtocolGuid,
                      Event,
                      &RegistrationExitPmAuth
                      );
    }
  }

  switch (BootMode) {

  case BOOT_WITH_MINIMAL_CONFIGURATION:
    PlatformBdsInitHotKeyEvent ();
    PlatformBdsConnectSimpleConsole (gPlatformSimpleConsole);


    //
    // Check to see if it's needed to dispatch more DXE drivers.
    //
    for (Index = 0; Index < sizeof(ConnectDriverTable)/sizeof(EFI_GUID *); Index++) {
      Status = gBS->LocateHandleBuffer (
                      ByProtocol,
                      ConnectDriverTable[Index],
                      NULL,
                      &HandleCount,
                      &HandleBuffer
                      );
      if (!EFI_ERROR (Status)) {
        for (Index1 = 0; Index1 < HandleCount; Index1++) {
          gBS->ConnectController (
                 HandleBuffer[Index1],
                 NULL,
                 NULL,
                 TRUE
                 );
        }
      }

      if (HandleBuffer != NULL) {
        FreePool (HandleBuffer);
      }

      gDS->Dispatch ();
    }

    //
    //  Locate the Global NVS Protocol.
    //
    Status = gBS->LocateProtocol (
                    &gEfiGlobalNvsAreaProtocolGuid,
                    NULL,
                    (void **)&GlobalNvsArea
                    );
    if (GlobalNvsArea->Area->emmcVersion == 0){
      EmmcBootDevPath = (PLATFORM_PCI_DEVICE_PATH *)gPlatformSimpleBootOption[0];
      EmmcBootDevPath->PciDevice.Device = 0x10;
    }

    //
    // Connect boot device here to give time to read keyboard.
    //
    BdsLibConnectDevicePath (gPlatformSimpleBootOption[0]);

    //
    // This is a workround for dectecting hotkey from USB keyboard.
    //
    gBS->Stall(KEYBOARD_TIMER_INTERVAL);

    if (mHotKeyTimerEvent != NULL) {
      gBS->SetTimer (
             mHotKeyTimerEvent,
             TimerCancel,
             0
             );
      gBS->CloseEvent (mHotKeyTimerEvent);
      mHotKeyTimerEvent = NULL;
    }
    if (mHotKeyPressed) {
      //
      // Skip show progress count down
      //
      Timeout = 0xFFFF;
      goto FULL_CONFIGURATION;
    }

    if (SystemConfiguration.QuietBoot) {
      EnableQuietBoot (PcdGetPtr(PcdLogoFile));
    } else {
      PlatformBdsDiagnostics (IGNORE, FALSE, BaseMemoryTest);
    }


    #ifdef TPM_ENABLED
    TcgPhysicalPresenceLibProcessRequest();
    #endif
    #ifdef FTPM_ENABLE
    TrEEPhysicalPresenceLibProcessRequest(NULL);
    #endif
    //
    // Close boot script and install ready to lock
    //
    InstallReadyToLock ();

    //
    // Give one chance to enter the setup if we 
    // select Gummiboot "Reboot Into Firmware Interface" and Fast Boot is enabled.
    //
    BootIntoFirmwareInterface();
    break;

  case BOOT_ASSUMING_NO_CONFIGURATION_CHANGES:

    //
    // In no-configuration boot mode, we can connect the
    // console directly.
    //
    BdsLibConnectAllDefaultConsoles ();
    PlatformBdsDiagnostics (IGNORE, TRUE, BaseMemoryTest);

    //
    // Perform some platform specific connect sequence
    //
    PlatformBdsConnectSequence ();

    //
    // As console is ready, perform user identification again.
    //
    if (mCurrentUser == NULL) {
      PlatformBdsUserIdentify (&mCurrentUser, &DeferredImageExist);
      if (DeferredImageExist) {
        //
        // After user authentication, the deferred drivers was loaded again.
        // Here, need to ensure the deferred images are connected.
        //
        BdsLibConnectAllDefaultConsoles ();
        PlatformBdsConnectSequence ();
      }
    }

    //
    // Close boot script and install ready to lock
    //
    InstallReadyToLock ();

    //
    // Notes: current time out = 0 can not enter the
    // front page
    //
    PlatformBdsEnterFrontPageWithHotKey (Timeout, FALSE);

    //
    // Check the boot option with the boot option list
    //
    BdsLibBuildOptionFromVar (BootOptionList, L"BootOrder");
    break;

  case BOOT_ON_FLASH_UPDATE:

    //
    // Boot with the specific configuration
    //
    PlatformBdsConnectConsole (gPlatformConsole);
    PlatformBdsDiagnostics (EXTENSIVE, FALSE, BaseMemoryTest);
    BdsLibConnectAll ();

    //
    // Perform user identification
    //
    if (mCurrentUser == NULL) {
      PlatformBdsUserIdentify (&mCurrentUser, &DeferredImageExist);
      if (DeferredImageExist) {
        //
        // After user authentication, the deferred drivers was loaded again.
        // Here, need to ensure the deferred images are connected.
        //
        BdsLibConnectAll ();
      }
    }

    //
    // Close boot script and install ready to lock
    //
    InstallReadyToLock ();

    ProcessCapsules (BOOT_ON_FLASH_UPDATE);
    break;

  case BOOT_IN_RECOVERY_MODE:

    //
    // In recovery mode, just connect platform console
    // and show up the front page
    //
    PlatformBdsConnectConsole (gPlatformConsole);
    PlatformBdsDiagnostics (EXTENSIVE, FALSE, BaseMemoryTest);
    BdsLibConnectAll ();

    //
    // Perform user identification
    //
    if (mCurrentUser == NULL) {
      PlatformBdsUserIdentify (&mCurrentUser, &DeferredImageExist);
      if (DeferredImageExist) {
        //
        // After user authentication, the deferred drivers was loaded again.
        // Here, need to ensure the deferred drivers are connected.
        //
        BdsLibConnectAll ();
      }
    }

    //
    // Close boot script and install ready to lock
    //
    InstallReadyToLock ();

    //
    // In recovery boot mode, we still enter to the
    // frong page now
    //
    PlatformBdsEnterFrontPageWithHotKey (Timeout, FALSE);
    break;

FULL_CONFIGURATION:
  case BOOT_WITH_FULL_CONFIGURATION:
  case BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS:
  case BOOT_WITH_DEFAULT_SETTINGS:
  default:

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
    // Chenyunh[TODO]: This is Workgroud to show the fs for uSDcard,
    // Need to root cause this issue.
    //
    DEBUG ((DEBUG_ERROR, "Start to reconnect all driver.\n"));
    BdsLibDisconnectAllEfi();
    BdsLibConnectAll ();
    DEBUG ((DEBUG_ERROR, "End to reconnect all driver.\n"));

    //
    // Perform some platform specific connect sequence
    //
    PlatformBdsConnectSequence ();
    if (SystemConfiguration.QuietBoot) {
        EnableQuietBoot (PcdGetPtr(PcdLogoFile));
    } else {
        PlatformBdsDiagnostics (IGNORE, FALSE, BaseMemoryTest);
    }

    //
    // Do a pre-delay so Hard Disk can spin up and see more logo.
    //
    gBS->Stall(SystemConfiguration.HddPredelay * 1000000);

    //
    // Perform user identification
    //
    if (mCurrentUser == NULL) {
      PlatformBdsUserIdentify (&mCurrentUser, &DeferredImageExist);
      if (DeferredImageExist) {
        //
        // After user authentication, the deferred drivers was loaded again.
        // Here, need to ensure the deferred drivers are connected.
        //
        Status = PlatformBdsConnectConsole (gPlatformConsole);
        if (EFI_ERROR (Status)) {
          PlatformBdsNoConsoleAction ();
        }
        PlatformBdsConnectSequence ();
      }
    }
   #ifdef TPM_ENABLED
   TcgPhysicalPresenceLibProcessRequest();
   #endif
   #ifdef FTPM_ENABLE
   TrEEPhysicalPresenceLibProcessRequest(NULL);
   #endif
    //
    // Close boot script and install ready to lock
    //
    InstallReadyToLock ();

    //
    // Here we have enough time to do the enumeration of boot device
    //
    PlatformBdsLibEnumerateAllBootOption (BootOptionList);

    //
    // Give one chance to enter the setup if we
    // have the time out
    //
    PlatformBdsEnterFrontPageWithHotKey (Timeout, FALSE);

	//
	// Give one chance to enter the setup if we 
	// select Gummiboot "Reboot Into Firmware Interface"
	//
	BootIntoFirmwareInterface();

    //
    // In default boot mode, always find all boot
    // option and do enumerate all the default boot option
    //
    if (Timeout == 0) {
      BdsLibBuildOptionFromVar (BootOptionList, L"BootOrder");
      if (IsListEmpty(BootOptionList)) {
        PlatformBdsPredictBootOption (BootOptionList);
      }

      return;
    }

    
    break;
  }


  IsFirstBoot = PcdGetBool(PcdBootState);
  if (IsFirstBoot) {
    PcdSetBool(PcdBootState, FALSE);
  }
  return;

}

/**
  Hook point after a boot attempt succeeds. We don't expect a boot option to
  return, so the UEFI 2.0 specification defines that you will default to an
  interactive mode and stop processing the BootOrder list in this case. This
  is alos a platform implementation and can be customized by IBV/OEM.

  @param Option  Pointer to Boot Option that succeeded to boot.

  @retval None.

**/
VOID
EFIAPI
PlatformBdsBootSuccess (
  IN  BDS_COMMON_OPTION *Option
  )
{
  CHAR16  *TmpStr;

  //
  // If Boot returned with EFI_SUCCESS and there is not in the boot device
  // select loop then we need to pop up a UI and wait for user input.
  //
  TmpStr =  Option->StatusString;
  if (TmpStr != NULL) {
    BdsLibOutputStrings (gST->ConOut, TmpStr, Option->Description, L"\n\r", NULL);
    FreePool(TmpStr);
  }
}

/**
  Hook point after a boot attempt fails.

  @param Option - Pointer to Boot Option that failed to boot.
  @param Status - Status returned from failed boot.
  @param ExitData - Exit data returned from failed boot.
  @param ExitDataSize - Exit data size returned from failed boot.

  @retval None.

**/
VOID
EFIAPI
PlatformBdsBootFail (
  IN  BDS_COMMON_OPTION  *Option,
  IN  EFI_STATUS         Status,
  IN  CHAR16             *ExitData,
  IN  UINTN              ExitDataSize
  )
{
  CHAR16          *TmpStr;
  EFI_HANDLE      FvProtocolHandle;

  //
  // If Boot returned with failed status then we need to pop up a UI and wait
  // for user input.
  //
  TmpStr = Option->StatusString;
  if (TmpStr != NULL) {
    BdsLibOutputStrings (gST->ConOut, TmpStr, Option->Description, L"\n\r", NULL);
    FreePool(TmpStr);
  }
  if (PcdGet32(PcdFlashFvShellSize) > 0){
    gDS->ProcessFirmwareVolume (
           (VOID *)(UINTN)PcdGet32(PcdFlashFvShellBase),
           PcdGet32(PcdFlashFvShellSize),
           &FvProtocolHandle
           );
  }
  PlatformBdsConnectSequence ();
}

/**
  This function is remained for IBV/OEM to do some platform action,
  if there no console device can be connected.

  @param  None.

  @retval EFI_SUCCESS       Direct return success now.

**/
EFI_STATUS
PlatformBdsNoConsoleAction (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  This function locks the block

  @param Base            The base address flash region to be locked.

**/
VOID
BdsLockFv (
  IN EFI_PHYSICAL_ADDRESS  Base
  )
{
  EFI_FV_BLOCK_MAP_ENTRY      *BlockMap;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  EFI_PHYSICAL_ADDRESS        BaseAddress;
  UINT8                       Data;
  UINT32                      BlockLength;
  UINTN                       Index;

  BaseAddress = Base - 0x400000 + 2;
  FvHeader    = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) (Base));
  BlockMap    = &(FvHeader->BlockMap[0]);

  while ((BlockMap->NumBlocks != 0) && (BlockMap->Length != 0)) {
    BlockLength = BlockMap->Length;
    for (Index = 0; Index < BlockMap->NumBlocks; Index++) {
      Data = MmioOr8 ((UINTN) BaseAddress, 0x03);
      BaseAddress += BlockLength;
    }
    BlockMap++;
  }
}

VOID
EFIAPI
PlatformBdsLockNonUpdatableFlash (
  VOID
  )
{
  EFI_PHYSICAL_ADDRESS  Base;

  Base = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashFvMainBase);
  if (Base > 0) {
    BdsLockFv (Base);
  }

  Base = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashFvRecoveryBase);
  if (Base > 0) {
    BdsLockFv (Base);
  }
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
  Connect the predefined platform default authentication devices.

  This function connects the predefined device path for authentication device,
  and if the predefined device path has child device path, the child handle will
  be connected too. But the child handle of the child will not be connected.

**/
VOID
EFIAPI
PlatformBdsConnectAuthDevice (
  VOID
  )
{
  EFI_STATUS                   Status;
  UINTN                        Index;
  UINTN                        HandleIndex;
  UINTN                        HandleCount;
  EFI_HANDLE                   *HandleBuffer;
  EFI_DEVICE_PATH_PROTOCOL     *ChildDevicePath;
  EFI_USER_MANAGER_PROTOCOL    *Manager;

  Status = gBS->LocateProtocol (
                  &gEfiUserManagerProtocolGuid,
                  NULL,
                  (VOID **) &Manager
                  );
  if (EFI_ERROR (Status)) {
    //
    // As user manager protocol is not installed, the authentication devices
    // should not be connected.
    //
    return ;
  }

  Index = 0;
  while (gUserAuthenticationDevice[Index] != NULL) {
    //
    // Connect the platform customized device paths
    //
    BdsLibConnectDevicePath (gUserAuthenticationDevice[Index]);
    Index++;
  }

  //
  // Find and connect the child device paths of the platform customized device paths
  //
  HandleBuffer = NULL;
  for (Index = 0; gUserAuthenticationDevice[Index] != NULL; Index++) {
    HandleCount = 0;
    Status = gBS->LocateHandleBuffer (
                    AllHandles,
                    NULL,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                    );
    ASSERT (!EFI_ERROR (Status));

    //
    // Find and connect the child device paths of gUserIdentificationDevice[Index]
    //
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
      ChildDevicePath = NULL;
      Status = gBS->HandleProtocol (
                      HandleBuffer[HandleIndex],
                      &gEfiDevicePathProtocolGuid,
                      (VOID **) &ChildDevicePath
                      );
      if (EFI_ERROR (Status) || ChildDevicePath == NULL) {
        continue;
      }

      if (CompareMem (
            ChildDevicePath,
            gUserAuthenticationDevice[Index],
            (GetDevicePathSize (gUserAuthenticationDevice[Index]) - sizeof (EFI_DEVICE_PATH_PROTOCOL))
            ) != 0) {
        continue;
      }
      gBS->ConnectController (
             HandleBuffer[HandleIndex],
             NULL,
             NULL,
             TRUE
             );
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }
}

/**
  This function is to identify a user, and return whether deferred images exist.

  @param[out]  User               Point to user profile handle.
  @param[out]  DeferredImageExist On return, points to TRUE if the deferred image
                                  exist or FALSE if it did not exist.

**/
VOID
EFIAPI
PlatformBdsUserIdentify (
  OUT EFI_USER_PROFILE_HANDLE        *User,
  OUT BOOLEAN                        *DeferredImageExist
  )
{
  EFI_STATUS                         Status;
  EFI_DEFERRED_IMAGE_LOAD_PROTOCOL   *DeferredImage;
  UINTN                              HandleCount;
  EFI_HANDLE                         *HandleBuf;
  UINTN                              Index;
  UINTN                              DriverIndex;
  EFI_DEVICE_PATH_PROTOCOL           *ImageDevicePath;
  VOID                               *DriverImage;
  UINTN                              ImageSize;
  BOOLEAN                            BootOption;

  //
  // Perform user identification
  //
  do {
    Status = BdsLibUserIdentify (User);
  } while (EFI_ERROR (Status));

  //
  // After user authentication now, try to find whether deferred image exists
  //
  HandleCount = 0;
  HandleBuf   = NULL;
  *DeferredImageExist = FALSE;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDeferredImageLoadProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuf
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuf[Index],
                    &gEfiDeferredImageLoadProtocolGuid,
                    (VOID **) &DeferredImage
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Find whether deferred image exists in this instance.
      //
      DriverIndex = 0;
      Status = DeferredImage->GetImageInfo(
                                DeferredImage,
                                DriverIndex,
                                &ImageDevicePath,
                                (VOID **) &DriverImage,
                                &ImageSize,
                                &BootOption
                                );
      if (!EFI_ERROR (Status)) {
        //
        // The deferred image is found.
        //
        FreePool (HandleBuf);
        *DeferredImageExist = TRUE;
        return ;
      }
    }
  }

  FreePool (HandleBuf);
}

UINTN gHotKey = 0;


EFI_STATUS
ShowProgressHotKey (
  IN UINT16                       TimeoutDefault
  )
{
  CHAR16                        *TmpStr;
  UINT16                        TimeoutRemain;
  EFI_STATUS                    Status;
  EFI_INPUT_KEY                 Key;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color;
  UINT32                        GpioValue;

  if (TimeoutDefault == 0) {
    return EFI_TIMEOUT;
  }

  gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
    
  if (DebugAssertEnabled())
  {
    DEBUG ((EFI_D_INFO, "\n\nStart showing progress bar... Press any key to stop it, or press <F2> or <DEL> to enter setup page! ...Zzz....\n"));
  }
  else
  {  
    #ifdef __GNUC__
    SerialPortWrite((UINT8 *)"\n\n>>>>Start boot option, Press <F2> or <DEL> to enter setup page(5 Sec)[GCC]", 76);
    #else
    SerialPortWrite((UINT8 *)"\n\n>>>>Start boot option, Press <F2> or <DEL> to enter setup page(5 Sec)", 71);
    #endif
  } 
  SetMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
  SetMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);
  SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);

  //
  // Clear the progress status bar first
  //
  TmpStr = L"Start boot option, Press <F2> or <DEL> to enter setup page.";
  PlatformBdsShowProgress (Foreground, Background, TmpStr, Color, 0, 0);

  TimeoutRemain = TimeoutDefault;
  while (TimeoutRemain != 0) {
    if (DebugAssertEnabled())
    {
    DEBUG ((EFI_D_INFO, "Showing progress bar...Remaining %d second!\n", TimeoutRemain));
    }
    else
    {	
    SerialPortWrite ((UINT8 *)".", 1);
    }
    Status = WaitForSingleEvent (gST->ConIn->WaitForKey, ONE_SECOND);
    if (Status != EFI_TIMEOUT) {
      break;
    }
    TimeoutRemain--;

    //
    // Show progress
    //
    if (TmpStr != NULL) {
      PlatformBdsShowProgress (
        Foreground,
        Background,
        TmpStr,
        Color,
        ((TimeoutDefault - TimeoutRemain) * 100 / TimeoutDefault),
        0
        );
    }
  }

  //
  // Timeout expired
  //
  if (TimeoutRemain == 0) {
    if (DebugAssertEnabled())
	{
	}
    else
    {	
    SerialPortWrite ((UINT8 *)"\r\n", 2);
    }
    return EFI_TIMEOUT;
  }

  //
  // User pressed some key
  //
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check Volume Up Key to enter Setup
  //
  GpioValue = MmioRead32 (IO_BASE_ADDRESS + 0x0668);  // The value of GPIOC_5
  if (((GpioValue & BIT0) == 0) && (Key.ScanCode == SCAN_UP)) {
    gHotKey = 0;
    return EFI_SUCCESS;
  }

  if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
    //
    // User pressed enter, equivalent to select "continue"
    //
    return EFI_TIMEOUT;
  }

  //
  //F2 --  Front Page
  //F5 --  Device Manager
  //F7 --  Boot Manager
  // do not use F8. generally people assume it is windows safe mode key.
  //F9 --  Boot order
  //
  DEBUG ((EFI_D_INFO, "[Key Pressed]: ScanCode 0x%x\n", Key.ScanCode));
  switch(Key.ScanCode) {
      case SCAN_F2:
          gHotKey = 0;
          break;

      case SCAN_DELETE:
          gHotKey = 0;
          break;

      case SCAN_F5:
          gHotKey = FRONT_PAGE_KEY_DEVICE_MANAGER;
          break;

      case SCAN_F7:
          gHotKey = FRONT_PAGE_KEY_BOOT_MANAGER;
          break;

      case SCAN_F9:
          gHotKey = FRONT_PAGE_KEY_BOOT_MAINTAIN;
          break;

      default:
          //set gHotKey to continue so that flow will not go into CallFrontPage
          gHotKey = FRONT_PAGE_KEY_CONTINUE;
          return EFI_TIMEOUT;
          break;
  }

  return EFI_SUCCESS;
}



/**
  This function is the main entry of the platform setup entry.
  The function will present the main menu of the system setup,
  this is the platform reference part and can be customize.


  @param TimeoutDefault     The fault time out value before the system
                            continue to boot.
  @param ConnectAllHappened The indicater to check if the connect all have
                            already happened.

**/
VOID
PlatformBdsEnterFrontPageWithHotKey (
  IN UINT16                       TimeoutDefault,
  IN BOOLEAN                      ConnectAllHappened
  )
{
  EFI_STATUS                    Status;

  EFI_GRAPHICS_OUTPUT_PROTOCOL       *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *SimpleTextOut;
  UINTN                              BootTextColumn;
  UINTN                              BootTextRow;

  GraphicsOutput = NULL;
  SimpleTextOut = NULL;

  PERF_START (NULL, "BdsTimeOut", "BDS", 0);

  //
  // Indicate if we need connect all in the platform setup
  //
  if (ConnectAllHappened) {
    gConnectAllHappened = TRUE;
  }

  if (!mModeInitialized) {
    //
    // After the console is ready, get current video resolution
    // and text mode before launching setup at first time.
    //
    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID**)&GraphicsOutput
                    );
    if (EFI_ERROR (Status)) {
      GraphicsOutput = NULL;
    }

    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiSimpleTextOutProtocolGuid,
                    (VOID**)&SimpleTextOut
                    );
    if (EFI_ERROR (Status)) {
      SimpleTextOut = NULL;
    }

    if (GraphicsOutput != NULL) {
      //
      // Get current video resolution and text mode.
      //
      mBootHorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
      mBootVerticalResolution   = GraphicsOutput->Mode->Info->VerticalResolution;
    }

    if (SimpleTextOut != NULL) {
      Status = SimpleTextOut->QueryMode (
                                SimpleTextOut,
                                SimpleTextOut->Mode->Mode,
                                &BootTextColumn,
                                &BootTextRow
                                );
      mBootTextModeColumn = (UINT32)BootTextColumn;
      mBootTextModeRow    = (UINT32)BootTextRow;
    }

    //
    // Get user defined text mode for setup.
    //
    mSetupHorizontalResolution = PcdGet32 (PcdSetupVideoHorizontalResolution);
    mSetupVerticalResolution   = PcdGet32 (PcdSetupVideoVerticalResolution);
    mSetupTextModeColumn       = PcdGet32 (PcdSetupConOutColumn);
    mSetupTextModeRow          = PcdGet32 (PcdSetupConOutRow);

    mModeInitialized           = TRUE;
  }

  if (TimeoutDefault != 0xffff) {
    Status = ShowProgressHotKey (TimeoutDefault);

    //
    // Ensure screen is clear when switch Console from Graphics mode to Text mode
    //
    gST->ConOut->EnableCursor (gST->ConOut, TRUE);
    gST->ConOut->ClearScreen (gST->ConOut);

    if (EFI_ERROR (Status)) {
      //
      // Timeout or user press enter to continue
      //
      goto Exit;
    }
  }
  //
  // Install BM HiiPackages. 
  // Keep BootMaint HiiPackage, so that it can be covered by global setting. 
  //
	InitBMPackage ();
  do {

    BdsSetConsoleMode (TRUE);

    InitializeFrontPage (FALSE);

    //
    // Update Front Page strings
    //
    UpdateFrontPageStrings ();

    Status = EFI_SUCCESS;
    gCallbackKey = 0;
    if (gHotKey == 0) {
      Status = CallFrontPage ();
    } else {
      gCallbackKey = gHotKey;
      gHotKey = 0;
    }

    //
    // If gCallbackKey is greater than 1 and less or equal to 5,
    // it will launch configuration utilities.
    // 2 = set language
    // 3 = boot manager
    // 4 = device manager
    // 5 = boot maintenance manager
    //
    if (gCallbackKey != 0) {
      REPORT_STATUS_CODE (
        EFI_PROGRESS_CODE,
        (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_USER_SETUP)
        );
    }

    //
    // Based on the key that was set, we can determine what to do
    //
    switch (gCallbackKey) {
    //
    // The first 4 entries in the Front Page are to be GUARANTEED to remain constant so IHV's can
    // describe to their customers in documentation how to find their setup information (namely
    // under the device manager and specific buckets)
    //
    // These entries consist of the Continue, Select language, Boot Manager, and Device Manager
    //
    case FRONT_PAGE_KEY_CONTINUE:

      //
      // User hit continue
      //
      break;

    case FRONT_PAGE_KEY_LANGUAGE:

      //
      // User made a language setting change - display front page again
      //
      break;

    case FRONT_PAGE_KEY_BOOT_MANAGER:
      //
	  // Remove the installed BootMaint HiiPackages when exit.
      //
      FreeBMPackage ();

      //
      // User chose to run the Boot Manager
      //
      CallBootManager ();
	  
	  //
      // Reinstall BootMaint HiiPackages after exiting from Boot Manager.
      //
      InitBMPackage ();
      break;

    case FRONT_PAGE_KEY_DEVICE_MANAGER:

      //
      // Display the Device Manager
      //
      do {
        CallDeviceManager ();
      } while (gCallbackKey == FRONT_PAGE_KEY_DEVICE_MANAGER);
      break;

    case FRONT_PAGE_KEY_BOOT_MAINTAIN:

      //
      // Display the Boot Maintenance Manager
      //
      BdsStartBootMaint ();
      break;
    }

  } while (((UINTN)gCallbackKey) != FRONT_PAGE_KEY_CONTINUE);

  //
  //Will leave browser, check any reset required change is applied? if yes, reset system
  //
  SetupResetReminder ();
  //
  // Remove the installed BootMaint HiiPackages when exit.
  //
  FreeBMPackage ();

Exit:
  //
  // Automatically load current entry
  // Note: The following lines of code only execute when Auto boot
  // takes affect
  //
  PERF_END (NULL, "BdsTimeOut", "BDS", 0);
}


VOID 
BootIntoFirmwareInterface(
VOID
)
{
  EFI_STATUS        Status;
  UINTN             DataSize;
  UINT16            Timeout;    
  UINT64            OsIndication;

  
  OsIndication = 0;
  DataSize = sizeof(UINT64);
  Status = gRT->GetVariable (
                  L"OsIndications",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  &OsIndication
                  );
				  
  DEBUG ((EFI_D_INFO, "OSIndication Variable Value %d\n", OsIndication));
  //
  //Goto FrontPage directly when bit EFI_OS_INDICATIONS_BOOT_TO_FW_UI in OSIndication Variable is setted.
  //  
  if (!EFI_ERROR(Status) && (OsIndication != 0)) {				  
   Timeout = 0xffff;
   PlatformBdsEnterFrontPage (Timeout, FALSE);
   }
}


EFI_STATUS
PlatformBdsConnectSimpleConsole (
  IN BDS_CONSOLE_CONNECT_ENTRY   *PlatformConsole
)
{
  EFI_STATUS                         Status;
  UINTN                              Index;
  EFI_DEVICE_PATH_PROTOCOL           *VarConout;
  EFI_DEVICE_PATH_PROTOCOL           *VarConin;
  UINTN                              DevicePathSize;


  Index = 0;
  Status = EFI_SUCCESS;
  DevicePathSize = 0;
  VarConout = BdsLibGetVariableAndSize (
                L"ConOut",
                &gEfiGlobalVariableGuid,
                &DevicePathSize
                );
  VarConin = BdsLibGetVariableAndSize (
               L"ConIn",
               &gEfiGlobalVariableGuid,
               &DevicePathSize
               );
  if (VarConout == NULL || VarConin == NULL) {
    //
    // Have chance to connect the platform default console,
    // the platform default console is the minimue device group
    // the platform should support
    //
    while (PlatformConsole[Index].DevicePath != NULL) {

      //
      // Update the console variable with the connect type
      //
      if ((PlatformConsole[Index].ConnectType & CONSOLE_IN) == CONSOLE_IN) {
        BdsLibUpdateConsoleVariable (L"ConIn", PlatformConsole[Index].DevicePath, NULL);
      }

      if ((PlatformConsole[Index].ConnectType & CONSOLE_OUT) == CONSOLE_OUT) {
        BdsLibUpdateConsoleVariable (L"ConOut", PlatformConsole[Index].DevicePath, NULL);
      }

      if ((PlatformConsole[Index].ConnectType & STD_ERROR) == STD_ERROR) {
        BdsLibUpdateConsoleVariable (L"ErrOut", PlatformConsole[Index].DevicePath, NULL);
      }

      Index ++;
    }
  }

  //
  // Connect ConIn first to give keyboard time to parse hot key event.
  //
  Status = BdsLibConnectConsoleVariable (L"ConIn");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure we have at least one active VGA, and have the right
  // active VGA in console variable
  //
  Status = PlatformBdsForceActiveVga ();

  //
  // It seems impossible not to have any ConOut device on platform,
  // so we check the status here.
  //
  Status = BdsLibConnectConsoleVariable (L"ConOut");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


/**
  Timer handler to convert the key from USB.

  @param  Event                    Indicates the event that invoke this function.
  @param  Context                  Indicates the calling context.
**/
VOID
EFIAPI
HotKeyTimerHandler (
  IN  EFI_EVENT                 Event,
  IN  VOID                      *Context
  )
{
  EFI_STATUS                    Status;
  EFI_INPUT_KEY                 Key;

  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  if (EFI_ERROR (Status)) {
    return;
  }

  switch(Key.ScanCode) {
  case SCAN_F2:
    gHotKey = 0;
    mHotKeyPressed = TRUE;
    break;

  case SCAN_F5:
    gHotKey = FRONT_PAGE_KEY_DEVICE_MANAGER;
    mHotKeyPressed = TRUE;
    break;

  case SCAN_F7:
    gHotKey = FRONT_PAGE_KEY_BOOT_MANAGER;
    mHotKeyPressed = TRUE;
    break;

  case SCAN_F9:
    gHotKey = FRONT_PAGE_KEY_BOOT_MAINTAIN;
    mHotKeyPressed = TRUE;
    break;
  }

  if (mHotKeyPressed) {
    gBS->SetTimer (
           mHotKeyTimerEvent,
           TimerCancel,
           0
           );
    gBS->CloseEvent (mHotKeyTimerEvent);
    mHotKeyTimerEvent = NULL;
  }

  return;
}


/**
  Callback function for SimpleTextInEx protocol install events

  @param Event           the event that is signaled.
  @param Context         not used here.

**/
VOID
EFIAPI
HitHotkeyEvent (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_STATUS                         Status;

  Status = gBS->CloseEvent(mHitHotkeyEvent);
  if (EFI_ERROR (Status)) {
    return;
  }
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  HotKeyTimerHandler,
                  NULL,
                  &mHotKeyTimerEvent
                  );
  if (EFI_ERROR (Status)) {
    return;
  }
  Status = gBS->SetTimer (
                  mHotKeyTimerEvent,
                  TimerPeriodic,
                  KEYBOARD_TIMER_INTERVAL
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  return;
}


VOID
EFIAPI
PlatformBdsInitHotKeyEvent (
  VOID
  )
{
  EFI_STATUS      Status;

  //
  // Register Protocol notify for Hotkey service
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  HitHotkeyEvent,
                  NULL,
                  &mHitHotkeyEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for protocol notifications on this event
  //
  Status = gBS->RegisterProtocolNotify (
                  &gEfiSimpleTextInputExProtocolGuid,
                  mHitHotkeyEvent,
                  &mHitHotkeyRegistration
                  );
  ASSERT_EFI_ERROR (Status);
}
