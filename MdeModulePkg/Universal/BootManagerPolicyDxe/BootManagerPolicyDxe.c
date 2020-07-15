/** @file
  This module produces Boot Manager Policy protocol.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Protocol/BootManagerPolicy.h>
#include <Protocol/ManagedNetwork.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootManagerLib.h>

CHAR16 mNetworkDeviceList[] = L"_NDL";

/**
  Connect all the system drivers to controllers and create the network device list in NV storage.

  @retval EFI_SUCCESS      Network devices are connected.
  @retval EFI_DEVICE_ERROR No network device is connected.

**/
EFI_STATUS
ConnectAllAndCreateNetworkDeviceList (
  VOID
  )
{
  EFI_STATUS                      Status;
  EFI_HANDLE                      *Handles;
  UINTN                           HandleCount;
  EFI_DEVICE_PATH_PROTOCOL        *SingleDevice;
  EFI_DEVICE_PATH_PROTOCOL        *Devices;
  EFI_DEVICE_PATH_PROTOCOL        *TempDevicePath;

  EfiBootManagerConnectAll ();

  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiManagedNetworkServiceBindingProtocolGuid, NULL, &HandleCount, &Handles);
  if (EFI_ERROR (Status)) {
    Handles = NULL;
    HandleCount = 0;
  }

  Devices = NULL;
  while (HandleCount-- != 0) {
    Status = gBS->HandleProtocol (Handles[HandleCount], &gEfiDevicePathProtocolGuid, (VOID **) &SingleDevice);
    if (EFI_ERROR (Status) || (SingleDevice == NULL)) {
      continue;
    }
    TempDevicePath = Devices;
    Devices = AppendDevicePathInstance (Devices, SingleDevice);
    if (TempDevicePath != NULL) {
      FreePool (TempDevicePath);
    }
  }

  if (Devices != NULL) {
    Status = gRT->SetVariable (
                    mNetworkDeviceList,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    GetDevicePathSize (Devices),
                    Devices
                    );
    //
    // Fails to save the network device list to NV storage is not a fatal error.
    // Only impact is performance.
    //
    FreePool (Devices);
  }

  return (Devices == NULL) ? EFI_DEVICE_ERROR : EFI_SUCCESS;
}

/**
  Connect the network devices.

  @retval EFI_SUCCESS      At least one network device was connected.
  @retval EFI_DEVICE_ERROR Network devices were not connected due to an error.
**/
EFI_STATUS
ConnectNetwork (
  VOID
  )
{
  EFI_STATUS                    Status;
  BOOLEAN                       OneConnected;
  EFI_DEVICE_PATH_PROTOCOL      *Devices;
  EFI_DEVICE_PATH_PROTOCOL      *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *SingleDevice;
  UINTN                         Size;

  OneConnected = FALSE;
  GetVariable2 (mNetworkDeviceList, &gEfiCallerIdGuid, (VOID **) &Devices, NULL);
  TempDevicePath = Devices;
  while (TempDevicePath != NULL) {
    SingleDevice = GetNextDevicePathInstance (&TempDevicePath, &Size);
    Status = EfiBootManagerConnectDevicePath (SingleDevice, NULL);
    if (!EFI_ERROR (Status)) {
      OneConnected = TRUE;
    }
    FreePool (SingleDevice);
  }
  if (Devices != NULL) {
    FreePool (Devices);
  }

  if (OneConnected) {
    return EFI_SUCCESS;
  } else {
    //
    // Cached network devices list doesn't exist or is NOT valid.
    //
    return ConnectAllAndCreateNetworkDeviceList ();
  }
}

/**
  Connect a device path following the platforms EFI Boot Manager policy.

  The ConnectDevicePath() function allows the caller to connect a DevicePath using the
  same policy as the EFI Boot Manger.

  @param[in] This       A pointer to the EFI_BOOT_MANAGER_POLICY_PROTOCOL instance.
  @param[in] DevicePath Points to the start of the EFI device path to connect.
                        If DevicePath is NULL then all the controllers in the
                        system will be connected using the platforms EFI Boot
                        Manager policy.
  @param[in] Recursive  If TRUE, then ConnectController() is called recursively
                        until the entire tree of controllers below the
                        controller specified by DevicePath have been created.
                        If FALSE, then the tree of controllers is only expanded
                        one level. If DevicePath is NULL then Recursive is ignored.

  @retval EFI_SUCCESS            The DevicePath was connected.
  @retval EFI_NOT_FOUND          The DevicePath was not found.
  @retval EFI_NOT_FOUND          No driver was connected to DevicePath.
  @retval EFI_SECURITY_VIOLATION The user has no permission to start UEFI device
                                 drivers on the DevicePath.
  @retval EFI_UNSUPPORTED        The current TPL is not TPL_APPLICATION.
**/
EFI_STATUS
EFIAPI
BootManagerPolicyConnectDevicePath (
  IN EFI_BOOT_MANAGER_POLICY_PROTOCOL *This,
  IN EFI_DEVICE_PATH                  *DevicePath,
  IN BOOLEAN                          Recursive
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          Controller;

  if (EfiGetCurrentTpl () != TPL_APPLICATION) {
    return EFI_UNSUPPORTED;
  }

  if (DevicePath == NULL) {
    EfiBootManagerConnectAll ();
    return EFI_SUCCESS;
  }

  if (Recursive) {
    Status = EfiBootManagerConnectDevicePath (DevicePath, NULL);
  } else {
    Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &DevicePath, &Controller);
    if (!EFI_ERROR (Status)) {
      Status = gBS->ConnectController (Controller, NULL, DevicePath, FALSE);
    }
  }
  return Status;
}
/**
  Connect a class of devices using the platform Boot Manager policy.

  The ConnectDeviceClass() function allows the caller to request that the Boot
  Manager connect a class of devices.

  If Class is EFI_BOOT_MANAGER_POLICY_CONSOLE_GUID then the Boot Manager will
  use platform policy to connect consoles. Some platforms may restrict the
  number of consoles connected as they attempt to fast boot, and calling
  ConnectDeviceClass() with a Class value of EFI_BOOT_MANAGER_POLICY_CONSOLE_GUID
  must connect the set of consoles that follow the Boot Manager platform policy,
  and the EFI_SIMPLE_TEXT_INPUT_PROTOCOL, EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL, and
  the EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL are produced on the connected handles.
  The Boot Manager may restrict which consoles get connect due to platform policy,
  for example a security policy may require that a given console is not connected.

  If Class is EFI_BOOT_MANAGER_POLICY_NETWORK_GUID then the Boot Manager will
  connect the protocols the platforms supports for UEFI general purpose network
  applications on one or more handles. If more than one network controller is
  available a platform will connect, one, many, or all of the networks based
  on platform policy. Connecting UEFI networking protocols, like EFI_DHCP4_PROTOCOL,
  does not establish connections on the network. The UEFI general purpose network
  application that called ConnectDeviceClass() may need to use the published
  protocols to establish the network connection. The Boot Manager can optionally
  have a policy to establish a network connection.

  If Class is EFI_BOOT_MANAGER_POLICY_CONNECT_ALL_GUID then the Boot Manager
  will connect all UEFI drivers using the UEFI Boot Service
  EFI_BOOT_SERVICES.ConnectController(). If the Boot Manager has policy
  associated with connect all UEFI drivers this policy will be used.

  A platform can also define platform specific Class values as a properly generated
  EFI_GUID would never conflict with this specification.

  @param[in] This  A pointer to the EFI_BOOT_MANAGER_POLICY_PROTOCOL instance.
  @param[in] Class A pointer to an EFI_GUID that represents a class of devices
                   that will be connected using the Boot Mangers platform policy.

  @retval EFI_SUCCESS      At least one devices of the Class was connected.
  @retval EFI_DEVICE_ERROR Devices were not connected due to an error.
  @retval EFI_NOT_FOUND    The Class is not supported by the platform.
  @retval EFI_UNSUPPORTED  The current TPL is not TPL_APPLICATION.
**/
EFI_STATUS
EFIAPI
BootManagerPolicyConnectDeviceClass (
  IN EFI_BOOT_MANAGER_POLICY_PROTOCOL *This,
  IN EFI_GUID                         *Class
  )
{
  if (EfiGetCurrentTpl () != TPL_APPLICATION) {
    return EFI_UNSUPPORTED;
  }

  if (CompareGuid (Class, &gEfiBootManagerPolicyConnectAllGuid)) {
    ConnectAllAndCreateNetworkDeviceList ();
    return EFI_SUCCESS;
  }

  if (CompareGuid (Class, &gEfiBootManagerPolicyConsoleGuid)) {
    return EfiBootManagerConnectAllDefaultConsoles ();
  }

  if (CompareGuid (Class, &gEfiBootManagerPolicyNetworkGuid)) {
    return ConnectNetwork ();
  }

  return EFI_NOT_FOUND;
}

EFI_BOOT_MANAGER_POLICY_PROTOCOL  mBootManagerPolicy = {
  EFI_BOOT_MANAGER_POLICY_PROTOCOL_REVISION,
  BootManagerPolicyConnectDevicePath,
  BootManagerPolicyConnectDeviceClass
};

/**
  Install Boot Manager Policy Protocol.

  @param ImageHandle    The image handle.
  @param SystemTable    The system table.

  @retval  EFI_SUCEESS  The Boot Manager Policy protocol is successfully installed.
  @retval  Other        Return status from gBS->InstallMultipleProtocolInterfaces().

**/
EFI_STATUS
EFIAPI
BootManagerPolicyInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_HANDLE                               Handle;

  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiBootManagerPolicyProtocolGuid);

  Handle = NULL;
  return gBS->InstallMultipleProtocolInterfaces (
                &Handle,
                &gEfiBootManagerPolicyProtocolGuid, &mBootManagerPolicy,
                NULL
                );
}
