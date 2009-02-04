/** @file

  This file contains the definination for host controller register operation routines.

Copyright (c) 2007 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_EHCI_REG_H_
#define _EFI_EHCI_REG_H_


typedef enum {
  //
  // Capability register offset
  //
  EHC_CAPLENGTH_OFFSET    = 0,    // Capability register length offset
  EHC_HCSPARAMS_OFFSET    = 0x04, // Structural Parameters 04-07h
  EHC_HCCPARAMS_OFFSET    = 0x08, // Capability parameters offset

  //
  // Capability register bit definition
  //
  HCSP_NPORTS             = 0x0F, // Number of root hub port
  HCCP_64BIT              = 0x01, // 64-bit addressing capability

  //
  // Operational register offset
  //
  EHC_USBCMD_OFFSET       = 0x0,  // USB command register offset
  EHC_USBSTS_OFFSET       = 0x04, // Statue register offset
  EHC_USBINTR_OFFSET      = 0x08, // USB interrutp offset
  EHC_FRINDEX_OFFSET      = 0x0C, // Frame index offset
  EHC_CTRLDSSEG_OFFSET    = 0x10, // Control data structure segment offset
  EHC_FRAME_BASE_OFFSET   = 0x14, // Frame list base address offset
  EHC_ASYNC_HEAD_OFFSET   = 0x18, // Next asynchronous list address offset
  EHC_CONFIG_FLAG_OFFSET  = 0x40, // Configure flag register offset
  EHC_PORT_STAT_OFFSET    = 0x44, // Port status/control offset

  EHC_FRAME_LEN           = 1024,

  //
  // Register bit definition
  //
  CONFIGFLAG_ROUTE_EHC    = 0x01, // Route port to EHC

  USBCMD_RUN              = 0x01,   // Run/stop
  USBCMD_RESET            = 0x02,   // Start the host controller reset
  USBCMD_ENABLE_PERIOD    = 0x10,   // Enable periodic schedule
  USBCMD_ENABLE_ASYNC     = 0x20,   // Enable asynchronous schedule
  USBCMD_IAAD             = 0x40,   // Interrupt on async advance doorbell

  USBSTS_IAA              = 0x20,   // Interrupt on async advance
  USBSTS_PERIOD_ENABLED   = 0x4000, // Periodic schedule status
  USBSTS_ASYNC_ENABLED    = 0x8000, // Asynchronous schedule status
  USBSTS_HALT             = 0x1000, // Host controller halted
  USBSTS_SYS_ERROR        = 0x10,   // Host system error
  USBSTS_INTACK_MASK      = 0x003F, // Mask for the interrupt ACK, the WC
                                    // (write clean) bits in USBSTS register

  PORTSC_CONN             = 0x01,   // Current Connect Status
  PORTSC_CONN_CHANGE      = 0x02,   // Connect Status Change
  PORTSC_ENABLED          = 0x04,   // Port Enable / Disable
  PORTSC_ENABLE_CHANGE    = 0x08,   // Port Enable / Disable Change
  PORTSC_OVERCUR          = 0x10,   // Over current Active
  PORTSC_OVERCUR_CHANGE   = 0x20,   // Over current Change
  PORSTSC_RESUME          = 0x40,   // Force Port Resume
  PORTSC_SUSPEND          = 0x80,   // Port Suspend State
  PORTSC_RESET            = 0x100,  // Port Reset
  PORTSC_LINESTATE_K      = 0x400,  // Line Status K-state
  PORTSC_LINESTATE_J      = 0x800,  // Line Status J-state
  PORTSC_POWER            = 0x1000, // Port Power
  PORTSC_OWNER            = 0x2000, // Port Owner
  PORTSC_CHANGE_MASK      = 0x2A,   // Mask of the port change bits,
                                    // they are WC (write clean)
  //
  // PCI Configuration Registers
  //
  EHC_PCI_CLASSC          = 0x09,
  EHC_PCI_CLASSC_PI       = 0x20,
  EHC_BAR_INDEX           = 0 /* how many bytes away from USB_BASE to 0x10 */
}EHCI_REGISTER_OFFSET;

#define EHC_LINK_TERMINATED(Link) (((Link) & 0x01) != 0)

#define EHC_ADDR(High, QhHw32)   \
        ((VOID *) (UINTN) (LShiftU64 ((High), 32) | ((QhHw32) & 0xFFFFFFF0)))

#define EHCI_IS_DATAIN(EndpointAddr) EHC_BIT_IS_SET((EndpointAddr), 0x80)

//
// Structure to map the hardware port states to the
// UEFI's port states.
//
typedef struct {
  UINT16                  HwState;
  UINT16                  UefiState;
} USB_PORT_STATE_MAP;

//
// Ehci Data and Ctrl Structures
//
#pragma pack(1)
typedef struct {
  UINT8                   PI;
  UINT8                   SubClassCode;
  UINT8                   BaseCode;
} USB_CLASSC;
#pragma pack()

/**
  Read EHCI capability register.

  @param  Ehc     The EHCI device.
  @param  Offset  Capability register address.

  @return The register content.

**/
UINT32
EhcReadCapRegister (
  IN  USB2_HC_DEV         *Ehc,
  IN  UINT32              Offset
  );


/**
  Read EHCI Operation register.

  @param  Ehc      The EHCI device.
  @param  Offset   The operation register offset.

  @return The register content.

**/
UINT32
EhcReadOpReg (
  IN  USB2_HC_DEV         *Ehc,
  IN  UINT32              Offset
  );


/**
  Write  the data to the EHCI operation register.

  @param  Ehc      The EHCI device.
  @param  Offset   EHCI operation register offset.
  @param  Data     The data to write.

**/
VOID
EhcWriteOpReg (
  IN USB2_HC_DEV          *Ehc,
  IN UINT32               Offset,
  IN UINT32               Data
  );


/**
  Add support for UEFI Over Legacy (UoL) feature, stop
  the legacy USB SMI support.

  @param  Ehc      The EHCI device.

**/
VOID
EhcClearLegacySupport (
  IN USB2_HC_DEV          *Ehc
  );



/**
  Set door bell and wait it to be ACKed by host controller.
  This function is used to synchronize with the hardware.

  @param  Ehc          The EHCI device.
  @param  Timeout      The time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  Synchronized with the hardware.
  @retval EFI_TIMEOUT  Time out happened while waiting door bell to set.

**/
EFI_STATUS
EhcSetAndWaitDoorBell (
  IN  USB2_HC_DEV         *Ehc,
  IN  UINT32               Timeout
  );


/**
  Clear all the interrutp status bits, these bits are Write-Clean.

  @param  Ehc      The EHCI device.

**/
VOID
EhcAckAllInterrupt (
  IN  USB2_HC_DEV         *Ehc
  );



/**
  Whether Ehc is halted.

  @param  Ehc     The EHCI device.

  @retval TRUE    The controller is halted.
  @retval FALSE   It isn't halted.

**/
BOOLEAN
EhcIsHalt (
  IN USB2_HC_DEV          *Ehc
  );


/**
  Whether system error occurred.

  @param  Ehc      The EHCI device.

  @retval TRUE     System error happened.
  @retval FALSE    No system error.

**/
BOOLEAN
EhcIsSysError (
  IN USB2_HC_DEV          *Ehc
  );


/**
  Reset the host controller.

  @param  Ehc          The EHCI device.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The host controller is reset.
  @return Others       Failed to reset the host.

**/
EFI_STATUS
EhcResetHC (
  IN USB2_HC_DEV          *Ehc,
  IN UINT32               Timeout
  );


/**
  Halt the host controller.

  @param  Ehc          The EHCI device.
  @param  Timeout      Time to wait before abort.

  @return EFI_SUCCESS  The EHCI is halt.
  @return EFI_TIMEOUT  Failed to halt the controller before Timeout.

**/
EFI_STATUS
EhcHaltHC (
  IN USB2_HC_DEV         *Ehc,
  IN UINT32              Timeout
  );


/**
  Set the EHCI to run.

  @param  Ehc          The EHCI device.
  @param  Timeout      Time to wait before abort.

  @return EFI_SUCCESS  The EHCI is running.
  @return Others       Failed to set the EHCI to run.

**/
EFI_STATUS
EhcRunHC (
  IN USB2_HC_DEV          *Ehc,
  IN UINT32               Timeout
  );



/**
  Initialize the HC hardware.
  EHCI spec lists the five things to do to initialize the hardware:
  1. Program CTRLDSSEGMENT
  2. Set USBINTR to enable interrupts
  3. Set periodic list base
  4. Set USBCMD, interrupt threshold, frame list size etc
  5. Write 1 to CONFIGFLAG to route all ports to EHCI

  @param  Ehc          The EHCI device.

  @return EFI_SUCCESS  The EHCI has come out of halt state.
  @return EFI_TIMEOUT  Time out happened.

**/
EFI_STATUS
EhcInitHC (
  IN USB2_HC_DEV          *Ehc
  );

#endif
