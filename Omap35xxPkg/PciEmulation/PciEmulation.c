/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2016, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/EmbeddedExternalDevice.h>

#include <TPS65950.h>
#include <Omap3530/Omap3530.h>

EMBEDDED_EXTERNAL_DEVICE   *gTPS65950;

#define HOST_CONTROLLER_OPERATION_REG_SIZE  0x44

STATIC
EFI_STATUS
ConfigureUSBHost (
  NON_DISCOVERABLE_DEVICE   *Device
  )
{
  EFI_STATUS Status;
  UINT8      Data = 0;

  // Take USB host out of force-standby mode
  MmioWrite32 (UHH_SYSCONFIG, UHH_SYSCONFIG_MIDLEMODE_NO_STANDBY
                            | UHH_SYSCONFIG_CLOCKACTIVITY_ON
                            | UHH_SYSCONFIG_SIDLEMODE_NO_STANDBY
                            | UHH_SYSCONFIG_ENAWAKEUP_ENABLE
                            | UHH_SYSCONFIG_AUTOIDLE_ALWAYS_RUN);
  MmioWrite32 (UHH_HOSTCONFIG, UHH_HOSTCONFIG_P3_CONNECT_STATUS_DISCONNECT
                             | UHH_HOSTCONFIG_P2_CONNECT_STATUS_DISCONNECT
                             | UHH_HOSTCONFIG_P1_CONNECT_STATUS_DISCONNECT
                             | UHH_HOSTCONFIG_ENA_INCR_ALIGN_DISABLE
                             | UHH_HOSTCONFIG_ENA_INCR16_ENABLE
                             | UHH_HOSTCONFIG_ENA_INCR8_ENABLE
                             | UHH_HOSTCONFIG_ENA_INCR4_ENABLE
                             | UHH_HOSTCONFIG_AUTOPPD_ON_OVERCUR_EN_ON
                             | UHH_HOSTCONFIG_P1_ULPI_BYPASS_ULPI_MODE);

  // USB reset (GPIO 147 - Port 5 pin 19) output high
  MmioAnd32 (GPIO5_BASE + GPIO_OE, ~BIT19);
  MmioWrite32 (GPIO5_BASE + GPIO_SETDATAOUT, BIT19);

  // Get the Power IC protocol
  Status = gBS->LocateProtocol (&gEmbeddedExternalDeviceProtocolGuid, NULL, (VOID **)&gTPS65950);
  ASSERT_EFI_ERROR (Status);

  // Power the USB PHY
  Data = VAUX_DEV_GRP_P1;
  Status = gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID4, VAUX2_DEV_GRP), 1, &Data);
  ASSERT_EFI_ERROR(Status);

  Data = VAUX_DEDICATED_18V;
  Status = gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID4, VAUX2_DEDICATED), 1, &Data);
  ASSERT_EFI_ERROR (Status);

  // Enable power to the USB hub
  Status = gTPS65950->Read (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID3, LEDEN), 1, &Data);
  ASSERT_EFI_ERROR (Status);

  // LEDAON controls the power to the USB host, PWM is disabled
  Data &= ~LEDAPWM;
  Data |= LEDAON;

  Status = gTPS65950->Write (gTPS65950, EXTERNAL_DEVICE_REGISTER(I2C_ADDR_GRP_ID3, LEDEN), 1, &Data);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciEmulationEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  UINT8                   CapabilityLength;
  UINT8                   PhysicalPorts;
  UINTN                   MemorySize;

  CapabilityLength = MmioRead8 (USB_EHCI_HCCAPBASE);
  PhysicalPorts    = MmioRead32 (USB_EHCI_HCCAPBASE + 0x4) & 0x0000000F;
  MemorySize       = CapabilityLength + HOST_CONTROLLER_OPERATION_REG_SIZE +
                     4 * PhysicalPorts - 1;

  return RegisterNonDiscoverableMmioDevice (
           NonDiscoverableDeviceTypeEhci,
           NonDiscoverableDeviceDmaTypeNonCoherent,
           ConfigureUSBHost,
           NULL,
           1,
           USB_EHCI_HCCAPBASE, MemorySize
           );
}
