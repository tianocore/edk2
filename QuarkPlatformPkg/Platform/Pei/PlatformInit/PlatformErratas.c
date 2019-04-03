/** @file
Platform Erratas performed by early init PEIM driver.

Copyright (c) 2013 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"
#include "PlatformEarlyInit.h"

//
// Constants.
//

//
// Platform EHCI Packet Buffer OUT/IN Thresholds, values in number of DWORDs.
//
#define EHCI_OUT_THRESHOLD_VALUE              (0x7f)
#define EHCI_IN_THRESHOLD_VALUE               (0x7f)

//
// Platform init USB device interrupt masks.
//
#define V_IOH_USBDEVICE_D_INTR_MSK_UDC_REG    (0x0000007f)
#define V_IOH_USBDEVICE_EP_INTR_MSK_UDC_REG   (B_IOH_USBDEVICE_EP_INTR_MSK_UDC_REG_OUT_EP_MASK | B_IOH_USBDEVICE_EP_INTR_MSK_UDC_REG_IN_EP_MASK)

//
// Global variables defined within this source module.
//

UINTN IohEhciPciReg[IOH_MAX_EHCI_USB_CONTROLLERS] = {
  PCI_LIB_ADDRESS (IOH_USB_BUS_NUMBER, IOH_USB_EHCI_DEVICE_NUMBER, IOH_EHCI_FUNCTION_NUMBER, 0),
};

UINTN IohUsbDevicePciReg[IOH_MAX_USBDEVICE_USB_CONTROLLERS] = {
  PCI_LIB_ADDRESS (IOH_USB_BUS_NUMBER, IOH_USBDEVICE_DEVICE_NUMBER, IOH_USBDEVICE_FUNCTION_NUMBER, 0),
};

//
// Routines local to this source module.
//

/** Perform USB erratas after MRC init.

**/
VOID
PlatformUsbErratasPostMrc (
  VOID
  )
{
  UINT32                            Index;
  UINT32                            TempBar0Addr;
  UINT16                            SaveCmdReg;
  UINT32                            SaveBar0Reg;

  TempBar0Addr = PcdGet32(PcdPeiQNCUsbControllerMemoryBaseAddress);

  //
  // Apply EHCI controller erratas.
  //
  for (Index = 0; Index < IOH_MAX_EHCI_USB_CONTROLLERS; Index++, TempBar0Addr += IOH_USB_CONTROLLER_MMIO_RANGE) {

    if ((PciRead16 (IohEhciPciReg[Index] + R_IOH_USB_VENDOR_ID)) != V_IOH_USB_VENDOR_ID) {
      continue;  // Device not enabled, skip.
    }

    //
    // Save current settings for PCI CMD/BAR0 registers
    //
    SaveCmdReg = PciRead16 (IohEhciPciReg[Index] + R_IOH_USB_COMMAND);
    SaveBar0Reg = PciRead32 (IohEhciPciReg[Index] + R_IOH_USB_MEMBAR);

    //
    // Temp. assign base address register, Enable Memory Space.
    //
    PciWrite32 ((IohEhciPciReg[Index] + R_IOH_USB_MEMBAR), TempBar0Addr);
    PciWrite16 (IohEhciPciReg[Index] + R_IOH_USB_COMMAND, SaveCmdReg | B_IOH_USB_COMMAND_MSE);


    //
    // Set packet buffer OUT/IN thresholds.
    //
    MmioAndThenOr32 (
      TempBar0Addr + R_IOH_EHCI_INSNREG01,
      (UINT32) (~(B_IOH_EHCI_INSNREG01_OUT_THRESHOLD_MASK | B_IOH_EHCI_INSNREG01_IN_THRESHOLD_MASK)),
      (UINT32) ((EHCI_OUT_THRESHOLD_VALUE << B_IOH_EHCI_INSNREG01_OUT_THRESHOLD_BP) | (EHCI_IN_THRESHOLD_VALUE << B_IOH_EHCI_INSNREG01_IN_THRESHOLD_BP))
      );

    //
    // Restore settings for PCI CMD/BAR0 registers
    //
    PciWrite32 ((IohEhciPciReg[Index] + R_IOH_USB_MEMBAR), SaveBar0Reg);
    PciWrite16 (IohEhciPciReg[Index] + R_IOH_USB_COMMAND, SaveCmdReg);
  }

  //
  // Apply USB device controller erratas.
  //
  for (Index = 0; Index < IOH_MAX_USBDEVICE_USB_CONTROLLERS; Index++, TempBar0Addr += IOH_USB_CONTROLLER_MMIO_RANGE) {

    if ((PciRead16 (IohUsbDevicePciReg[Index] + R_IOH_USB_VENDOR_ID)) != V_IOH_USB_VENDOR_ID) {
      continue;  // Device not enabled, skip.
    }

    //
    // Save current settings for PCI CMD/BAR0 registers
    //
    SaveCmdReg = PciRead16 (IohUsbDevicePciReg[Index] + R_IOH_USB_COMMAND);
    SaveBar0Reg = PciRead32 (IohUsbDevicePciReg[Index] + R_IOH_USB_MEMBAR);

    //
    // Temp. assign base address register, Enable Memory Space.
    //
    PciWrite32 ((IohUsbDevicePciReg[Index] + R_IOH_USB_MEMBAR), TempBar0Addr);
    PciWrite16 (IohUsbDevicePciReg[Index] + R_IOH_USB_COMMAND, SaveCmdReg | B_IOH_USB_COMMAND_MSE);

    //
    // Erratas for USB Device interrupt registers.
    //

    //
    // 1st Mask interrupts.
    //
    MmioWrite32 (
      TempBar0Addr + R_IOH_USBDEVICE_D_INTR_MSK_UDC_REG,
      V_IOH_USBDEVICE_D_INTR_MSK_UDC_REG
      );
    //
    // 2nd RW/1C of equivalent status bits.
    //
    MmioWrite32 (
      TempBar0Addr + R_IOH_USBDEVICE_D_INTR_UDC_REG,
      V_IOH_USBDEVICE_D_INTR_MSK_UDC_REG
      );

    //
    // 1st Mask end point interrupts.
    //
    MmioWrite32 (
      TempBar0Addr + R_IOH_USBDEVICE_EP_INTR_MSK_UDC_REG,
      V_IOH_USBDEVICE_EP_INTR_MSK_UDC_REG
      );
    //
    // 2nd RW/1C of equivalent end point status bits.
    //
    MmioWrite32 (
      TempBar0Addr + R_IOH_USBDEVICE_EP_INTR_UDC_REG,
      V_IOH_USBDEVICE_EP_INTR_MSK_UDC_REG
      );

    //
    // Restore settings for PCI CMD/BAR0 registers
    //
    PciWrite32 ((IohUsbDevicePciReg[Index] + R_IOH_USB_MEMBAR), SaveBar0Reg);
    PciWrite16 (IohUsbDevicePciReg[Index] + R_IOH_USB_COMMAND, SaveCmdReg);
  }
}

//
// Routines exported by this source module.
//

/** Perform Platform Erratas after MRC.

  @retval   EFI_SUCCESS               Operation success.

**/
EFI_STATUS
EFIAPI
PlatformErratasPostMrc (
  VOID
  )
{
  PlatformUsbErratasPostMrc ();
  return EFI_SUCCESS;
}
