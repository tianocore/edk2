/** @file
  The XHCI controller driver.

(C) Copyright 2023 Hewlett Packard Enterprise Development LP<BR>
Copyright (c) 2011 - 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Xhci.h"

//
// Two arrays used to translate the XHCI port state (change)
// to the UEFI protocol's port state (change).
//
USB_PORT_STATE_MAP  mUsbPortStateMap[] = {
  { XHC_PORTSC_CCS,   USB_PORT_STAT_CONNECTION  },
  { XHC_PORTSC_PED,   USB_PORT_STAT_ENABLE      },
  { XHC_PORTSC_OCA,   USB_PORT_STAT_OVERCURRENT },
  { XHC_PORTSC_RESET, USB_PORT_STAT_RESET       }
};

USB_PORT_STATE_MAP  mUsbPortChangeMap[] = {
  { XHC_PORTSC_CSC, USB_PORT_STAT_C_CONNECTION  },
  { XHC_PORTSC_PEC, USB_PORT_STAT_C_ENABLE      },
  { XHC_PORTSC_OCC, USB_PORT_STAT_C_OVERCURRENT },
  { XHC_PORTSC_PRC, USB_PORT_STAT_C_RESET       }
};

USB_CLEAR_PORT_MAP  mUsbClearPortChangeMap[] = {
  { XHC_PORTSC_CSC, EfiUsbPortConnectChange     },
  { XHC_PORTSC_PEC, EfiUsbPortEnableChange      },
  { XHC_PORTSC_OCC, EfiUsbPortOverCurrentChange },
  { XHC_PORTSC_PRC, EfiUsbPortResetChange       }
};

USB_PORT_STATE_MAP  mUsbHubPortStateMap[] = {
  { XHC_HUB_PORTSC_CCS,   USB_PORT_STAT_CONNECTION  },
  { XHC_HUB_PORTSC_PED,   USB_PORT_STAT_ENABLE      },
  { XHC_HUB_PORTSC_OCA,   USB_PORT_STAT_OVERCURRENT },
  { XHC_HUB_PORTSC_RESET, USB_PORT_STAT_RESET       }
};

USB_PORT_STATE_MAP  mUsbHubPortChangeMap[] = {
  { XHC_HUB_PORTSC_CSC, USB_PORT_STAT_C_CONNECTION  },
  { XHC_HUB_PORTSC_PEC, USB_PORT_STAT_C_ENABLE      },
  { XHC_HUB_PORTSC_OCC, USB_PORT_STAT_C_OVERCURRENT },
  { XHC_HUB_PORTSC_PRC, USB_PORT_STAT_C_RESET       }
};

USB_CLEAR_PORT_MAP  mUsbHubClearPortChangeMap[] = {
  { XHC_HUB_PORTSC_CSC,  EfiUsbPortConnectChange     },
  { XHC_HUB_PORTSC_PEC,  EfiUsbPortEnableChange      },
  { XHC_HUB_PORTSC_OCC,  EfiUsbPortOverCurrentChange },
  { XHC_HUB_PORTSC_PRC,  EfiUsbPortResetChange       },
  { XHC_HUB_PORTSC_BHRC, Usb3PortBHPortResetChange   }
};

EFI_DRIVER_BINDING_PROTOCOL  gXhciDriverBinding = {
  XhcDriverBindingSupported,
  XhcDriverBindingStart,
  XhcDriverBindingStop,
  0x30,
  NULL,
  NULL
};

//
// Template for Xhci's Usb2 Host Controller Protocol Instance.
//
EFI_USB2_HC_PROTOCOL  gXhciUsb2HcTemplate = {
  XhcGetCapability,
  XhcReset,
  XhcGetState,
  XhcSetState,
  XhcControlTransfer,
  XhcBulkTransfer,
  XhcAsyncInterruptTransfer,
  XhcSyncInterruptTransfer,
  XhcIsochronousTransfer,
  XhcAsyncIsochronousTransfer,
  XhcGetRootHubPortStatus,
  XhcSetRootHubPortFeature,
  XhcClearRootHubPortFeature,
  0x3,
  0x0
};

static UINT64   mXhciPerformanceCounterStartValue;
static UINT64   mXhciPerformanceCounterEndValue;
static UINT64   mXhciPerformanceCounterFrequency;
static BOOLEAN  mXhciPerformanceCounterValuesCached = FALSE;

/**
  Retrieves the capability of root hub ports.

  @param  This                  The EFI_USB2_HC_PROTOCOL instance.
  @param  MaxSpeed              Max speed supported by the controller.
  @param  PortNumber            Number of the root hub ports.
  @param  Is64BitCapable        Whether the controller supports 64-bit memory
                                addressing.

  @retval EFI_SUCCESS           Host controller capability were retrieved successfully.
  @retval EFI_INVALID_PARAMETER Either of the three capability pointer is NULL.

**/
EFI_STATUS
EFIAPI
XhcGetCapability (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  OUT UINT8                 *MaxSpeed,
  OUT UINT8                 *PortNumber,
  OUT UINT8                 *Is64BitCapable
  )
{
  USB_XHCI_INSTANCE  *Xhc;
  EFI_TPL            OldTpl;

  if ((MaxSpeed == NULL) || (PortNumber == NULL) || (Is64BitCapable == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (XHC_TPL);

  Xhc             = XHC_FROM_THIS (This);
  *MaxSpeed       = EFI_USB_SPEED_SUPER;
  *PortNumber     = (UINT8)(Xhc->HcSParams1.Data.MaxPorts);
  *Is64BitCapable = (UINT8)Xhc->Support64BitDma;
  DEBUG ((DEBUG_INFO, "XhcGetCapability: %d ports, 64 bit %d\n", *PortNumber, *Is64BitCapable));

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

/**
  Provides software reset for the USB host controller.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  Attributes            A bit mask of the reset operation to perform.

  @retval EFI_SUCCESS           The reset operation succeeded.
  @retval EFI_INVALID_PARAMETER Attributes is not valid.
  @retval EFI_UNSUPPOURTED      The type of reset specified by Attributes is
                                not currently supported by the host controller.
  @retval EFI_DEVICE_ERROR      Host controller isn't halted to reset.

**/
EFI_STATUS
EFIAPI
XhcReset (
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN UINT16                Attributes
  )
{
  USB_XHCI_INSTANCE  *Xhc;
  EFI_STATUS         Status;
  EFI_TPL            OldTpl;

  Xhc = XHC_FROM_THIS (This);

  if (Xhc->DevicePath != NULL) {
    //
    // Report Status Code to indicate reset happens
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_PROGRESS_CODE,
      (EFI_IO_BUS_USB | EFI_IOB_PC_RESET),
      Xhc->DevicePath
      );
  }

  OldTpl = gBS->RaiseTPL (XHC_TPL);

  switch (Attributes) {
    case EFI_USB_HC_RESET_GLOBAL:
    //
    // Flow through, same behavior as Host Controller Reset
    //
    case EFI_USB_HC_RESET_HOST_CONTROLLER:
      if ((Xhc->DebugCapSupOffset != 0xFFFFFFFF) && ((XhcReadExtCapReg (Xhc, Xhc->DebugCapSupOffset) & 0xFF) == XHC_CAP_USB_DEBUG) &&
          ((XhcReadExtCapReg (Xhc, Xhc->DebugCapSupOffset + XHC_DC_DCCTRL) & BIT0) != 0))
      {
        Status = EFI_SUCCESS;
        goto ON_EXIT;
      }

      //
      // Host Controller must be Halt when Reset it
      //
      if (!XhcIsHalt (Xhc)) {
        Status = XhcHaltHC (Xhc, XHC_GENERIC_TIMEOUT);

        if (EFI_ERROR (Status)) {
          Status = EFI_DEVICE_ERROR;
          goto ON_EXIT;
        }
      }

      Status = XhcResetHC (Xhc, XHC_RESET_TIMEOUT);
      ASSERT (!(XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_CNR)));

      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }

      //
      // Clean up the asynchronous transfers, currently only
      // interrupt supports asynchronous operation.
      //
      XhciDelAllAsyncIntTransfers (Xhc);
      XhcFreeSched (Xhc);

      XhcInitSched (Xhc);
      break;

    case EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG:
    case EFI_USB_HC_RESET_HOST_WITH_DEBUG:
      Status = EFI_UNSUPPORTED;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
  }

ON_EXIT:
  DEBUG ((DEBUG_INFO, "XhcReset: status %r\n", Status));
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Retrieve the current state of the USB host controller.

  @param  This                   This EFI_USB2_HC_PROTOCOL instance.
  @param  State                  Variable to return the current host controller
                                 state.

  @retval EFI_SUCCESS            Host controller state was returned in State.
  @retval EFI_INVALID_PARAMETER  State is NULL.
  @retval EFI_DEVICE_ERROR       An error was encountered while attempting to
                                 retrieve the host controller's current state.

**/
EFI_STATUS
EFIAPI
XhcGetState (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  OUT EFI_USB_HC_STATE      *State
  )
{
  USB_XHCI_INSTANCE  *Xhc;
  EFI_TPL            OldTpl;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (XHC_TPL);

  Xhc = XHC_FROM_THIS (This);

  if (XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT)) {
    *State = EfiUsbHcStateHalt;
  } else {
    *State = EfiUsbHcStateOperational;
  }

  DEBUG ((DEBUG_INFO, "XhcGetState: current state %d\n", *State));
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

/**
  Sets the USB host controller to a specific state.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  State                 The state of the host controller that will be set.

  @retval EFI_SUCCESS           The USB host controller was successfully placed
                                in the state specified by State.
  @retval EFI_INVALID_PARAMETER State is invalid.
  @retval EFI_DEVICE_ERROR      Failed to set the state due to device error.

**/
EFI_STATUS
EFIAPI
XhcSetState (
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN EFI_USB_HC_STATE      State
  )
{
  USB_XHCI_INSTANCE  *Xhc;
  EFI_STATUS         Status;
  EFI_USB_HC_STATE   CurState;
  EFI_TPL            OldTpl;

  Status = XhcGetState (This, &CurState);

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (CurState == State) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (XHC_TPL);

  Xhc = XHC_FROM_THIS (This);

  switch (State) {
    case EfiUsbHcStateHalt:
      Status = XhcHaltHC (Xhc, XHC_GENERIC_TIMEOUT);
      break;

    case EfiUsbHcStateOperational:
      if (XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HSE)) {
        Status = EFI_DEVICE_ERROR;
        break;
      }

      //
      // Software must not write a one to this field unless the host controller
      // is in the Halted state. Doing so will yield undefined results.
      // refers to Spec[XHCI1.0-2.3.1]
      //
      if (!XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT)) {
        Status = EFI_DEVICE_ERROR;
        break;
      }

      Status = XhcRunHC (Xhc, XHC_GENERIC_TIMEOUT);
      break;

    case EfiUsbHcStateSuspend:
      Status = EFI_UNSUPPORTED;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "XhcSetState: status %r\n", Status));
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Retrieves the current status of a USB root hub port.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber            The root hub port to retrieve the state from.
                                This value is zero-based.
  @param  PortStatus            Variable to receive the port state.

  @retval EFI_SUCCESS           The status of the USB root hub port specified.
                                by PortNumber was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid.
  @retval EFI_DEVICE_ERROR      Can't read register.

**/
EFI_STATUS
EFIAPI
XhcGetRootHubPortStatus (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  IN  UINT8                 PortNumber,
  OUT EFI_USB_PORT_STATUS   *PortStatus
  )
{
  USB_XHCI_INSTANCE  *Xhc;
  UINT32             Offset;
  UINT32             State;
  UINT32             TotalPort;
  UINTN              Index;
  UINTN              MapSize;
  UINT8              PortSpeed;
  EFI_STATUS         Status;
  USB_DEV_ROUTE      ParentRouteChart;
  EFI_TPL            OldTpl;

  if (PortStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (XHC_TPL);

  Xhc    = XHC_FROM_THIS (This);
  Status = EFI_SUCCESS;

  TotalPort = Xhc->HcSParams1.Data.MaxPorts;

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset                       = (UINT32)(XHC_PORTSC_OFFSET + (0x10 * PortNumber));
  PortStatus->PortStatus       = 0;
  PortStatus->PortChangeStatus = 0;

  State = XhcReadOpReg (Xhc, Offset);

  PortSpeed = (State & XHC_PORTSC_PS) >> 10;

  //
  // According to XHCI 1.1 spec November 2017,
  // Section 7.2 xHCI Support Protocol Capability
  //
  if (PortSpeed > 0) {
    PortStatus->PortStatus = XhcCheckUsbPortSpeedUsedPsic (Xhc, PortSpeed, PortNumber);
    // If no match found in ext cap reg, fall back to PORTSC
    if (PortStatus->PortStatus == 0) {
      //
      // According to XHCI 1.1 spec November 2017,
      // bit 10~13 of the root port status register identifies the speed of the attached device.
      //
      switch (PortSpeed) {
        case 2:
          PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;
          break;

        case 3:
          PortStatus->PortStatus |= USB_PORT_STAT_HIGH_SPEED;
          break;

        case 4:
        case 5:
          PortStatus->PortStatus |= USB_PORT_STAT_SUPER_SPEED;
          break;

        default:
          break;
      }
    }
  }

  //
  // Convert the XHCI port/port change state to UEFI status
  //
  MapSize = sizeof (mUsbPortStateMap) / sizeof (USB_PORT_STATE_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (XHC_BIT_IS_SET (State, mUsbPortStateMap[Index].HwState)) {
      PortStatus->PortStatus = (UINT16)(PortStatus->PortStatus | mUsbPortStateMap[Index].UefiState);
    }
  }

  //
  // Bit5~8 reflects its current link state.
  //
  if ((State & XHC_PORTSC_PLS) >> 5 == 3) {
    PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
  }

  MapSize = sizeof (mUsbPortChangeMap) / sizeof (USB_PORT_STATE_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (XHC_BIT_IS_SET (State, mUsbPortChangeMap[Index].HwState)) {
      PortStatus->PortChangeStatus = (UINT16)(PortStatus->PortChangeStatus | mUsbPortChangeMap[Index].UefiState);
    }
  }

  MapSize = sizeof (mUsbClearPortChangeMap) / sizeof (USB_CLEAR_PORT_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (XHC_BIT_IS_SET (State, mUsbClearPortChangeMap[Index].HwState)) {
      XhcClearRootHubPortFeature (This, PortNumber, (EFI_USB_PORT_FEATURE)mUsbClearPortChangeMap[Index].Selector);
    }
  }

  //
  // Poll the root port status register to enable/disable corresponding device slot if there is a device attached/detached.
  // For those devices behind hub, we get its attach/detach event by hooking Get_Port_Status request at control transfer for those hub.
  //
  ParentRouteChart.Dword = 0;
  Status                 = XhcPollPortStatusChange (Xhc, ParentRouteChart, PortNumber, PortStatus);

  //
  // Force resetting the port by clearing the USB_PORT_STAT_C_RESET bit in PortChangeStatus
  // when XhcPollPortStatusChange fails
  //
  if (EFI_ERROR (Status)) {
    PortStatus->PortChangeStatus &= ~(USB_PORT_STAT_C_RESET);
    Status                        = EFI_SUCCESS;
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Sets a feature for the specified root hub port.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber            Root hub port to set.
  @param  PortFeature           Feature to set.

  @retval EFI_SUCCESS           The feature specified by PortFeature was set.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
  @retval EFI_DEVICE_ERROR      Can't read register.

**/
EFI_STATUS
EFIAPI
XhcSetRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
  )
{
  USB_XHCI_INSTANCE  *Xhc;
  UINT32             Offset;
  UINT32             State;
  UINT32             TotalPort;
  EFI_STATUS         Status;
  EFI_TPL            OldTpl;

  OldTpl = gBS->RaiseTPL (XHC_TPL);

  Xhc    = XHC_FROM_THIS (This);
  Status = EFI_SUCCESS;

  TotalPort = (Xhc->HcSParams1.Data.MaxPorts);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset = (UINT32)(XHC_PORTSC_OFFSET + (0x10 * PortNumber));
  State  = XhcReadOpReg (Xhc, Offset);

  //
  // Mask off the port status change bits, these bits are
  // write clean bit
  //
  State &= ~(BIT1 | BIT17 | BIT18 | BIT19 | BIT20 | BIT21 | BIT22 | BIT23);

  switch (PortFeature) {
    case EfiUsbPortEnable:
      //
      // Ports may only be enabled by the xHC. Software cannot enable a port by writing a '1' to this flag.
      // A port may be disabled by software writing a '1' to this flag.
      //
      Status = EFI_SUCCESS;
      break;

    case EfiUsbPortSuspend:
      State |= XHC_PORTSC_LWS;
      XhcWriteOpReg (Xhc, Offset, State);
      State &= ~XHC_PORTSC_PLS;
      State |= (3 << 5);
      XhcWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortReset:
      DEBUG ((DEBUG_INFO, "XhcUsbPortReset!\n"));
      //
      // Make sure Host Controller not halt before reset it
      //
      if (XhcIsHalt (Xhc)) {
        Status = XhcRunHC (Xhc, XHC_GENERIC_TIMEOUT);

        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "XhcSetRootHubPortFeature :failed to start HC - %r\n", Status));
          break;
        }
      }

      //
      // 4.3.1 Resetting a Root Hub Port
      // 1) Write the PORTSC register with the Port Reset (PR) bit set to '1'.
      //
      State |= XHC_PORTSC_RESET;
      XhcWriteOpReg (Xhc, Offset, State);
      XhcWaitOpRegBit (Xhc, Offset, XHC_PORTSC_PRC, TRUE, XHC_GENERIC_TIMEOUT);
      break;

    case EfiUsbPortPower:
      //
      // Not supported, ignore the operation
      //
      Status = EFI_SUCCESS;
      break;

    case EfiUsbPortOwner:
      //
      // XHCI root hub port don't has the owner bit, ignore the operation
      //
      Status = EFI_SUCCESS;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
  }

ON_EXIT:
  DEBUG ((DEBUG_INFO, "XhcSetRootHubPortFeature: status %r\n", Status));
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Clears a feature for the specified root hub port.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber            Specifies the root hub port whose feature is
                                requested to be cleared.
  @param  PortFeature           Indicates the feature selector associated with the
                                feature clear request.

  @retval EFI_SUCCESS           The feature specified by PortFeature was cleared
                                for the USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
  @retval EFI_DEVICE_ERROR      Can't read register.

**/
EFI_STATUS
EFIAPI
XhcClearRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
  )
{
  USB_XHCI_INSTANCE  *Xhc;
  UINT32             Offset;
  UINT32             State;
  UINT32             TotalPort;
  EFI_STATUS         Status;
  EFI_TPL            OldTpl;

  OldTpl = gBS->RaiseTPL (XHC_TPL);

  Xhc    = XHC_FROM_THIS (This);
  Status = EFI_SUCCESS;

  TotalPort = (Xhc->HcSParams1.Data.MaxPorts);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset = XHC_PORTSC_OFFSET + (0x10 * PortNumber);

  //
  // Mask off the port status change bits, these bits are
  // write clean bit
  //
  State  = XhcReadOpReg (Xhc, Offset);
  State &= ~(BIT1 | BIT17 | BIT18 | BIT19 | BIT20 | BIT21 | BIT22 | BIT23);

  switch (PortFeature) {
    case EfiUsbPortEnable:
      //
      // Ports may only be enabled by the xHC. Software cannot enable a port by writing a '1' to this flag.
      // A port may be disabled by software writing a '1' to this flag.
      //
      State |= XHC_PORTSC_PED;
      State &= ~XHC_PORTSC_RESET;
      XhcWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortSuspend:
      State |= XHC_PORTSC_LWS;
      XhcWriteOpReg (Xhc, Offset, State);
      State &= ~XHC_PORTSC_PLS;
      XhcWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortReset:
      //
      // PORTSC_RESET BIT(4) bit is RW1S attribute, which means Write-1-to-set status:
      // Register bits indicate status when read, a clear bit may be set by
      // writing a '1'. Writing a '0' to RW1S bits has no effect.
      //
      break;

    case EfiUsbPortOwner:
      //
      // XHCI root hub port don't has the owner bit, ignore the operation
      //
      break;

    case EfiUsbPortConnectChange:
      //
      // Clear connect status change
      //
      State |= XHC_PORTSC_CSC;
      XhcWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortEnableChange:
      //
      // Clear enable status change
      //
      State |= XHC_PORTSC_PEC;
      XhcWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortOverCurrentChange:
      //
      // Clear PortOverCurrent change
      //
      State |= XHC_PORTSC_OCC;
      XhcWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortResetChange:
      //
      // Clear Port Reset change
      //
      State |= XHC_PORTSC_PRC;
      XhcWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortPower:
    case EfiUsbPortSuspendChange:
      //
      // Not supported or not related operation
      //
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      break;
  }

ON_EXIT:
  DEBUG ((DEBUG_INFO, "XhcClearRootHubPortFeature: status %r\n", Status));
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Submits a new transaction to a target USB device.

  @param  Xhc                   The XHCI Instance.
  @param  DeviceAddress         The target device address.
  @param  EndPointAddress       Endpoint number and its direction encoded in bit 7
  @param  DeviceSpeed           Target device speed.
  @param  MaximumPacketLength   Maximum packet size the default control transfer
                                endpoint is capable of sending or receiving.
  @param  Type                  The transaction type.
  @param  Request               USB device request to send.
  @param  Data                  Data buffer to be transmitted or received from USB
                                device.
  @param  DataLength            The size (in bytes) of the data buffer.
  @param  Timeout               Indicates the maximum timeout, in millisecond.
  @param  TransferResult        Return the result of this control transfer.

  @retval EFI_SUCCESS           Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      Transfer failed due to host controller or device error.
**/
EFI_STATUS
XhcTransfer (
  IN     USB_XHCI_INSTANCE       *Xhc,
  IN     UINT8                   DeviceAddress,
  IN     UINT8                   EndPointAddress,
  IN     UINT8                   DeviceSpeed,
  IN     UINTN                   MaximumPacketLength,
  IN     UINTN                   Type,
  IN     EFI_USB_DEVICE_REQUEST  *Request,
  IN OUT VOID                    *Data,
  IN OUT UINTN                   *DataLength,
  IN     UINTN                   Timeout,
  OUT    UINT32                  *TransferResult
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  RecoveryStatus;
  URB         *Urb;

  ASSERT ((Type == XHC_CTRL_TRANSFER) || (Type == XHC_BULK_TRANSFER) || (Type == XHC_INT_TRANSFER_SYNC));
  Urb = XhcCreateUrb (
          Xhc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          MaximumPacketLength,
          Type,
          Request,
          Data,
          *DataLength,
          NULL,
          NULL
          );

  if (Urb == NULL) {
    DEBUG ((DEBUG_ERROR, "XhcTransfer[Type=%d]: failed to create URB!\n", Type));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = XhcExecTransfer (Xhc, FALSE, Urb, Timeout);

  if (Status == EFI_TIMEOUT) {
    //
    // The transfer timed out. Abort the transfer by dequeueing of the TD.
    //
    RecoveryStatus = XhcDequeueTrbFromEndpoint (Xhc, Urb);
    if (RecoveryStatus == EFI_ALREADY_STARTED) {
      //
      // The URB is finished just before stopping endpoint.
      // Change returning status from EFI_TIMEOUT to EFI_SUCCESS.
      //
      ASSERT (Urb->Result == EFI_USB_NOERROR);
      Status = EFI_SUCCESS;
      DEBUG ((DEBUG_ERROR, "XhcTransfer[Type=%d]: pending URB is finished, Length = %d.\n", Type, Urb->Completed));
    } else if (EFI_ERROR (RecoveryStatus)) {
      DEBUG ((DEBUG_ERROR, "XhcTransfer[Type=%d]: XhcDequeueTrbFromEndpoint failed!\n", Type));
    }
  }

  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;

  //
  // Based on XHCI spec 4.8.3, software should do the reset endpoint while USB Transaction occur.
  //
  if ((*TransferResult == EFI_USB_ERR_STALL) || (*TransferResult == EFI_USB_ERR_BABBLE) || (*TransferResult == EDKII_USB_ERR_TRANSACTION)) {
    ASSERT (Status == EFI_DEVICE_ERROR);
    RecoveryStatus = XhcRecoverHaltedEndpoint (Xhc, Urb);
    if (EFI_ERROR (RecoveryStatus)) {
      DEBUG ((DEBUG_ERROR, "XhcTransfer[Type=%d]: XhcRecoverHaltedEndpoint failed!\n", Type));
    }
  }

  Xhc->PciIo->Flush (Xhc->PciIo);
  //
  // Do not free URB data, since it is passed in as an external argument
  // and allocated and managed by the caller.
  //
  XhcFreeUrb (Xhc, Urb);
  return Status;
}

/**
  Submits control transfer to a target USB device.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         The target device address.
  @param  DeviceSpeed           Target device speed.
  @param  MaximumPacketLength   Maximum packet size the default control transfer
                                endpoint is capable of sending or receiving.
  @param  Request               USB device request to send.
  @param  TransferDirection     Specifies the data direction for the data stage
  @param  Data                  Data buffer to be transmitted or received from USB
                                device.
  @param  DataLength            The size (in bytes) of the data buffer.
  @param  Timeout               Indicates the maximum timeout, in millisecond.
  @param  Translator            Transaction translator to be used by this device.
  @param  TransferResult        Return the result of this control transfer.

  @retval EFI_SUCCESS           Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      Transfer failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
XhcControlTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     EFI_USB_DEVICE_REQUEST              *Request,
  IN     EFI_USB_DATA_DIRECTION              TransferDirection,
  IN OUT VOID                                *Data,
  IN OUT UINTN                               *DataLength,
  IN     UINTN                               Timeout,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT    UINT32                              *TransferResult
  )
{
  USB_XHCI_INSTANCE       *Xhc;
  UINT8                   Endpoint;
  UINT8                   Index;
  UINT8                   DescriptorType;
  UINT8                   SlotId;
  UINT8                   TTT;
  UINT8                   MTT;
  UINT32                  MaxPacket0;
  EFI_USB_HUB_DESCRIPTOR  *HubDesc;
  EFI_TPL                 OldTpl;
  EFI_STATUS              Status;
  UINTN                   MapSize;
  EFI_USB_PORT_STATUS     PortStatus;
  UINT32                  State;
  EFI_USB_DEVICE_REQUEST  ClearPortRequest;
  UINTN                   Len;

  //
  // Validate parameters
  //
  if ((Request == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbDataIn) &&
      (TransferDirection != EfiUsbDataOut) &&
      (TransferDirection != EfiUsbNoData))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection == EfiUsbNoData) &&
      ((Data != NULL) || (*DataLength != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbNoData) &&
      ((Data == NULL) || (*DataLength == 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((MaximumPacketLength != 8)  && (MaximumPacketLength != 16) &&
      (MaximumPacketLength != 32) && (MaximumPacketLength != 64) &&
      (MaximumPacketLength != 512)
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_LOW) && (MaximumPacketLength != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_SUPER) && (MaximumPacketLength != 512)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (XHC_TPL);

  Xhc = XHC_FROM_THIS (This);

  Status          = EFI_DEVICE_ERROR;
  *TransferResult = EFI_USB_ERR_SYSTEM;
  Len             = 0;

  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    DEBUG ((DEBUG_ERROR, "XhcControlTransfer: HC halted at entrance\n"));
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = XhcBusDevAddrToSlotId (Xhc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  //
  // Hook the Set_Address request from UsbBus.
  // According to XHCI 1.0 spec, the Set_Address request is replaced by XHCI's Address_Device cmd.
  //
  if ((Request->Request     == USB_REQ_SET_ADDRESS) &&
      (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD, USB_TARGET_DEVICE)))
  {
    //
    // Reset the BusDevAddr field of all disabled entries in UsbDevContext array firstly.
    // This way is used to clean the history to avoid using wrong device address by XhcAsyncInterruptTransfer().
    //
    for (Index = 0; Index < 255; Index++) {
      if (!Xhc->UsbDevContext[Index + 1].Enabled &&
          (Xhc->UsbDevContext[Index + 1].SlotId == 0) &&
          (Xhc->UsbDevContext[Index + 1].BusDevAddr == (UINT8)Request->Value))
      {
        Xhc->UsbDevContext[Index + 1].BusDevAddr = 0;
      }
    }

    if (Xhc->UsbDevContext[SlotId].XhciDevAddr == 0) {
      Status = EFI_DEVICE_ERROR;
      goto ON_EXIT;
    }

    //
    // The actual device address has been assigned by XHCI during initializing the device slot.
    // So we just need establish the mapping relationship between the device address requested from UsbBus
    // and the actual device address assigned by XHCI. The the following invocations through EFI_USB2_HC_PROTOCOL interface
    // can find out the actual device address by it.
    //
    Xhc->UsbDevContext[SlotId].BusDevAddr = (UINT8)Request->Value;
    Status                                = EFI_SUCCESS;
    goto ON_EXIT;
  }

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  // Note that we encode the direction in address although default control
  // endpoint is bidirectional. XhcCreateUrb expects this
  // combination of Ep addr and its direction.
  //
  Endpoint = (UINT8)(0 | ((TransferDirection == EfiUsbDataIn) ? 0x80 : 0));
  Status   = XhcTransfer (
               Xhc,
               DeviceAddress,
               Endpoint,
               DeviceSpeed,
               MaximumPacketLength,
               XHC_CTRL_TRANSFER,
               Request,
               Data,
               DataLength,
               Timeout,
               TransferResult
               );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Hook Get_Descriptor request from UsbBus as we need evaluate context and configure endpoint.
  // Hook Get_Status request form UsbBus as we need trace device attach/detach event happened at hub.
  // Hook Set_Config request from UsbBus as we need configure device endpoint.
  //
  if ((Request->Request     == USB_REQ_GET_DESCRIPTOR) &&
      ((Request->RequestType == USB_REQUEST_TYPE (EfiUsbDataIn, USB_REQ_TYPE_STANDARD, USB_TARGET_DEVICE)) ||
       ((Request->RequestType == USB_REQUEST_TYPE (EfiUsbDataIn, USB_REQ_TYPE_CLASS, USB_TARGET_DEVICE)))))
  {
    DescriptorType = (UINT8)(Request->Value >> 8);
    if ((DescriptorType == USB_DESC_TYPE_DEVICE) && ((*DataLength == sizeof (EFI_USB_DEVICE_DESCRIPTOR)) || ((DeviceSpeed == EFI_USB_SPEED_FULL) && (*DataLength == 8)))) {
      ASSERT (Data != NULL);
      //
      // Store a copy of device scriptor as hub device need this info to configure endpoint.
      //
      CopyMem (&Xhc->UsbDevContext[SlotId].DevDesc, Data, *DataLength);
      if (Xhc->UsbDevContext[SlotId].DevDesc.BcdUSB >= 0x0300) {
        //
        // If it's a usb3.0 device, then its max packet size is a 2^n.
        //
        MaxPacket0 = 1 << Xhc->UsbDevContext[SlotId].DevDesc.MaxPacketSize0;
      } else {
        MaxPacket0 = Xhc->UsbDevContext[SlotId].DevDesc.MaxPacketSize0;
      }

      Xhc->UsbDevContext[SlotId].ConfDesc = AllocateZeroPool (Xhc->UsbDevContext[SlotId].DevDesc.NumConfigurations * sizeof (EFI_USB_CONFIG_DESCRIPTOR *));
      if (Xhc->HcCParams.Data.Csz == 0) {
        Status = XhcEvaluateContext (Xhc, SlotId, MaxPacket0);
      } else {
        Status = XhcEvaluateContext64 (Xhc, SlotId, MaxPacket0);
      }
    } else if (DescriptorType == USB_DESC_TYPE_CONFIG) {
      ASSERT (Data != NULL);
      if (*DataLength == ((UINT16 *)Data)[1]) {
        //
        // Get configuration value from request, Store the configuration descriptor for Configure_Endpoint cmd.
        //
        Index = (UINT8)Request->Value;
        ASSERT (Index < Xhc->UsbDevContext[SlotId].DevDesc.NumConfigurations);
        Xhc->UsbDevContext[SlotId].ConfDesc[Index] = AllocateZeroPool (*DataLength);
        CopyMem (Xhc->UsbDevContext[SlotId].ConfDesc[Index], Data, *DataLength);
        //
        // Default to use AlternateSetting 0 for all interfaces.
        //
        Xhc->UsbDevContext[SlotId].ActiveAlternateSetting = AllocateZeroPool (Xhc->UsbDevContext[SlotId].ConfDesc[Index]->NumInterfaces * sizeof (UINT8));
      }
    } else if (((DescriptorType == USB_DESC_TYPE_HUB) ||
                (DescriptorType == USB_DESC_TYPE_HUB_SUPER_SPEED)) && (*DataLength > 2))
    {
      ASSERT (Data != NULL);
      HubDesc = (EFI_USB_HUB_DESCRIPTOR *)Data;
      ASSERT (HubDesc->NumPorts <= 15);
      //
      // The bit 5,6 of HubCharacter field of Hub Descriptor is TTT.
      //
      TTT = (UINT8)((HubDesc->HubCharacter & (BIT5 | BIT6)) >> 5);
      if (Xhc->UsbDevContext[SlotId].DevDesc.DeviceProtocol == 2) {
        //
        // Don't support multi-TT feature for super speed hub now.
        //
        MTT = 0;
        DEBUG ((DEBUG_ERROR, "XHCI: Don't support multi-TT feature for Hub now. (force to disable MTT)\n"));
      } else {
        MTT = 0;
      }

      if (Xhc->HcCParams.Data.Csz == 0) {
        Status = XhcConfigHubContext (Xhc, SlotId, HubDesc->NumPorts, TTT, MTT);
      } else {
        Status = XhcConfigHubContext64 (Xhc, SlotId, HubDesc->NumPorts, TTT, MTT);
      }
    }
  } else if ((Request->Request     == USB_REQ_SET_CONFIG) &&
             (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD, USB_TARGET_DEVICE)))
  {
    //
    // Hook Set_Config request from UsbBus as we need configure device endpoint.
    //
    for (Index = 0; Index < Xhc->UsbDevContext[SlotId].DevDesc.NumConfigurations; Index++) {
      if (Xhc->UsbDevContext[SlotId].ConfDesc[Index]->ConfigurationValue == (UINT8)Request->Value) {
        if (Xhc->HcCParams.Data.Csz == 0) {
          Status = XhcSetConfigCmd (Xhc, SlotId, DeviceSpeed, Xhc->UsbDevContext[SlotId].ConfDesc[Index]);
        } else {
          Status = XhcSetConfigCmd64 (Xhc, SlotId, DeviceSpeed, Xhc->UsbDevContext[SlotId].ConfDesc[Index]);
        }

        break;
      }
    }
  } else if ((Request->Request     == USB_REQ_SET_INTERFACE) &&
             (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD, USB_TARGET_INTERFACE)))
  {
    //
    // Hook Set_Interface request from UsbBus as we need configure interface setting.
    // Request->Value indicates AlterlateSetting to set
    // Request->Index indicates Interface to set
    //
    if (Xhc->UsbDevContext[SlotId].ActiveAlternateSetting[(UINT8)Request->Index] != (UINT8)Request->Value) {
      if (Xhc->HcCParams.Data.Csz == 0) {
        Status = XhcSetInterface (Xhc, SlotId, DeviceSpeed, Xhc->UsbDevContext[SlotId].ConfDesc[Xhc->UsbDevContext[SlotId].ActiveConfiguration - 1], Request);
      } else {
        Status = XhcSetInterface64 (Xhc, SlotId, DeviceSpeed, Xhc->UsbDevContext[SlotId].ConfDesc[Xhc->UsbDevContext[SlotId].ActiveConfiguration - 1], Request);
      }
    }
  } else if ((Request->Request     == USB_REQ_GET_STATUS) &&
             (Request->RequestType == USB_REQUEST_TYPE (EfiUsbDataIn, USB_REQ_TYPE_CLASS, USB_TARGET_OTHER)))
  {
    ASSERT (Data != NULL);
    //
    // Hook Get_Status request from UsbBus to keep track of the port status change.
    //
    State                       = *(UINT32 *)Data;
    PortStatus.PortStatus       = 0;
    PortStatus.PortChangeStatus = 0;

    if (DeviceSpeed == EFI_USB_SPEED_SUPER) {
      //
      // For super speed hub, its bit10~12 presents the attached device speed.
      //
      if ((State & XHC_PORTSC_PS) >> 10 == 0) {
        PortStatus.PortStatus |= USB_PORT_STAT_SUPER_SPEED;
      }
    } else {
      //
      // For high or full/low speed hub, its bit9~10 presents the attached device speed.
      //
      if (XHC_BIT_IS_SET (State, BIT9)) {
        PortStatus.PortStatus |= USB_PORT_STAT_LOW_SPEED;
      } else if (XHC_BIT_IS_SET (State, BIT10)) {
        PortStatus.PortStatus |= USB_PORT_STAT_HIGH_SPEED;
      }
    }

    //
    // Convert the XHCI port/port change state to UEFI status
    //
    MapSize = sizeof (mUsbHubPortStateMap) / sizeof (USB_PORT_STATE_MAP);
    for (Index = 0; Index < MapSize; Index++) {
      if (XHC_BIT_IS_SET (State, mUsbHubPortStateMap[Index].HwState)) {
        PortStatus.PortStatus = (UINT16)(PortStatus.PortStatus | mUsbHubPortStateMap[Index].UefiState);
      }
    }

    MapSize = sizeof (mUsbHubPortChangeMap) / sizeof (USB_PORT_STATE_MAP);
    for (Index = 0; Index < MapSize; Index++) {
      if (XHC_BIT_IS_SET (State, mUsbHubPortChangeMap[Index].HwState)) {
        PortStatus.PortChangeStatus = (UINT16)(PortStatus.PortChangeStatus | mUsbHubPortChangeMap[Index].UefiState);
      }
    }

    MapSize = sizeof (mUsbHubClearPortChangeMap) / sizeof (USB_CLEAR_PORT_MAP);

    for (Index = 0; Index < MapSize; Index++) {
      if (XHC_BIT_IS_SET (State, mUsbHubClearPortChangeMap[Index].HwState)) {
        ZeroMem (&ClearPortRequest, sizeof (EFI_USB_DEVICE_REQUEST));
        ClearPortRequest.RequestType = USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_CLASS, USB_TARGET_OTHER);
        ClearPortRequest.Request     = (UINT8)USB_REQ_CLEAR_FEATURE;
        ClearPortRequest.Value       = mUsbHubClearPortChangeMap[Index].Selector;
        ClearPortRequest.Index       = Request->Index;
        ClearPortRequest.Length      = 0;

        XhcControlTransfer (
          This,
          DeviceAddress,
          DeviceSpeed,
          MaximumPacketLength,
          &ClearPortRequest,
          EfiUsbNoData,
          NULL,
          &Len,
          Timeout,
          Translator,
          TransferResult
          );
      }
    }

    XhcPollPortStatusChange (Xhc, Xhc->UsbDevContext[SlotId].RouteString, (UINT8)Request->Index, &PortStatus);

    *(UINT32 *)Data = *(UINT32 *)&PortStatus;
  }

ON_EXIT:
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcControlTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction in bit 7.
  @param  DeviceSpeed           Device speed, Low speed device doesn't support bulk
                                transfer.
  @param  MaximumPacketLength   Maximum packet size the endpoint is capable of
                                sending or receiving.
  @param  DataBuffersNumber     Number of data buffers prepared for the transfer.
  @param  Data                  Array of pointers to the buffers of data to transmit
                                from or receive into. The caller is responsible for freeing
                                the buffers after the transfers are completed.
  @param  DataLength            The length of the data buffer.
  @param  DataToggle            On input, the initial data toggle for the transfer;
                                On output, it is updated to to next data toggle to
                                use of the subsequent bulk transfer.
  @param  Timeout               Indicates the maximum time, in millisecond, which
                                the transfer is allowed to complete.
  @param  Translator            A pointr to the transaction translator data.
  @param  TransferResult        A pointer to the detailed result information of the
                                bulk transfer.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
XhcBulkTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     UINT8                               DataBuffersNumber,
  IN OUT VOID                                *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN OUT UINTN                               *DataLength,
  IN OUT UINT8                               *DataToggle,
  IN     UINTN                               Timeout,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT    UINT32                              *TransferResult
  )
{
  USB_XHCI_INSTANCE  *Xhc;
  UINT8              SlotId;
  EFI_STATUS         Status;
  EFI_TPL            OldTpl;
  UINTN              DebugErrorLevel;

  //
  // Validate the parameters
  //
  if ((DataLength == NULL) || (*DataLength == 0) ||
      (Data == NULL) || (Data[0] == NULL) || (TransferResult == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 0) && (*DataToggle != 1)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_LOW) ||
      ((DeviceSpeed == EFI_USB_SPEED_FULL) && (MaximumPacketLength > 64)) ||
      ((EFI_USB_SPEED_HIGH == DeviceSpeed) && (MaximumPacketLength > 512)) ||
      ((EFI_USB_SPEED_SUPER == DeviceSpeed) && (MaximumPacketLength > 1024)))
  {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (XHC_TPL);

  Xhc = XHC_FROM_THIS (This);

  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;

  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    DEBUG ((DEBUG_ERROR, "XhcBulkTransfer: HC is halted\n"));
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = XhcBusDevAddrToSlotId (Xhc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  //
  Status = XhcTransfer (
             Xhc,
             DeviceAddress,
             EndPointAddress,
             DeviceSpeed,
             MaximumPacketLength,
             XHC_BULK_TRANSFER,
             NULL,
             Data[0],
             DataLength,
             Timeout,
             TransferResult
             );

ON_EXIT:
  if (EFI_ERROR (Status)) {
    if (Status == EFI_TIMEOUT) {
      DebugErrorLevel = DEBUG_VERBOSE;
    } else {
      DebugErrorLevel = DEBUG_ERROR;
    }

    DEBUG ((DebugErrorLevel, "XhcBulkTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Submits an asynchronous interrupt transfer to an
  interrupt endpoint of a USB device.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction encoded in bit 7
  @param  DeviceSpeed           Indicates device speed.
  @param  MaximumPacketLength   Maximum packet size the target endpoint is capable
  @param  IsNewTransfer         If TRUE, to submit an new asynchronous interrupt
                                transfer If FALSE, to remove the specified
                                asynchronous interrupt.
  @param  DataToggle            On input, the initial data toggle to use; on output,
                                it is updated to indicate the next data toggle.
  @param  PollingInterval       The he interval, in milliseconds, that the transfer
                                is polled.
  @param  DataLength            The length of data to receive at the rate specified
                                by  PollingInterval.
  @param  Translator            Transaction translator to use.
  @param  CallBackFunction      Function to call at the rate specified by
                                PollingInterval.
  @param  Context               Context to CallBackFunction.

  @retval EFI_SUCCESS           The request has been successfully submitted or canceled.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request failed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
XhcAsyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     BOOLEAN                             IsNewTransfer,
  IN OUT UINT8                               *DataToggle,
  IN     UINTN                               PollingInterval,
  IN     UINTN                               DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK     CallBackFunction,
  IN     VOID                                *Context OPTIONAL
  )
{
  USB_XHCI_INSTANCE  *Xhc;
  URB                *Urb;
  EFI_STATUS         Status;
  UINT8              SlotId;
  UINT8              Index;
  EFI_TPL            OldTpl;

  //
  // Validate parameters
  //
  if (!XHCI_IS_DATAIN (EndPointAddress)) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsNewTransfer) {
    if (DataLength == 0) {
      return EFI_INVALID_PARAMETER;
    }

    if ((*DataToggle != 1) && (*DataToggle != 0)) {
      return EFI_INVALID_PARAMETER;
    }

    if ((PollingInterval > 255) || (PollingInterval < 1)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  OldTpl = gBS->RaiseTPL (XHC_TPL);

  Xhc = XHC_FROM_THIS (This);

  //
  // Delete Async interrupt transfer request.
  //
  if (!IsNewTransfer) {
    //
    // The delete request may happen after device is detached.
    //
    for (Index = 0; Index < 255; Index++) {
      if (Xhc->UsbDevContext[Index + 1].BusDevAddr == DeviceAddress) {
        break;
      }
    }

    if (Index == 255) {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    Status = XhciDelAsyncIntTransfer (Xhc, DeviceAddress, EndPointAddress);
    DEBUG ((DEBUG_INFO, "XhcAsyncInterruptTransfer: remove old transfer for addr %d, Status = %r\n", DeviceAddress, Status));
    goto ON_EXIT;
  }

  Status = EFI_SUCCESS;

  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    DEBUG ((DEBUG_ERROR, "XhcAsyncInterruptTransfer: HC is halt\n"));
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = XhcBusDevAddrToSlotId (Xhc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  Urb = XhciInsertAsyncIntTransfer (
          Xhc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          MaximumPacketLength,
          DataLength,
          CallBackFunction,
          Context
          );
  if (Urb == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Ring the doorbell
  //
  Status = RingIntTransferDoorBell (Xhc, Urb);

ON_EXIT:
  Xhc->PciIo->Flush (Xhc->PciIo);
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Submits synchronous interrupt transfer to an interrupt endpoint
  of a USB device.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction encoded in bit 7
  @param  DeviceSpeed           Indicates device speed.
  @param  MaximumPacketLength   Maximum packet size the target endpoint is capable
                                of sending or receiving.
  @param  Data                  Buffer of data that will be transmitted to  USB
                                device or received from USB device.
  @param  DataLength            On input, the size, in bytes, of the data buffer; On
                                output, the number of bytes transferred.
  @param  DataToggle            On input, the initial data toggle to use; on output,
                                it is updated to indicate the next data toggle.
  @param  Timeout               Maximum time, in second, to complete.
  @param  Translator            Transaction translator to use.
  @param  TransferResult        Variable to receive the transfer result.

  @return EFI_SUCCESS           The transfer was completed successfully.
  @return EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @return EFI_INVALID_PARAMETER Some parameters are invalid.
  @return EFI_TIMEOUT           The transfer failed due to timeout.
  @return EFI_DEVICE_ERROR      The failed due to host controller or device error

**/
EFI_STATUS
EFIAPI
XhcSyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN OUT VOID                                *Data,
  IN OUT UINTN                               *DataLength,
  IN OUT UINT8                               *DataToggle,
  IN     UINTN                               Timeout,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT    UINT32                              *TransferResult
  )
{
  USB_XHCI_INSTANCE  *Xhc;
  UINT8              SlotId;
  EFI_STATUS         Status;
  EFI_TPL            OldTpl;

  //
  // Validates parameters
  //
  if ((DataLength == NULL) || (*DataLength == 0) ||
      (Data == NULL) || (TransferResult == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 1) && (*DataToggle != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (((DeviceSpeed == EFI_USB_SPEED_LOW) && (MaximumPacketLength != 8))  ||
      ((DeviceSpeed == EFI_USB_SPEED_FULL) && (MaximumPacketLength > 64)) ||
      ((DeviceSpeed == EFI_USB_SPEED_HIGH) && (MaximumPacketLength > 3072)))
  {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (XHC_TPL);

  Xhc = XHC_FROM_THIS (This);

  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;

  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    DEBUG ((DEBUG_ERROR, "EhcSyncInterruptTransfer: HC is halt\n"));
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = XhcBusDevAddrToSlotId (Xhc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  Status = XhcTransfer (
             Xhc,
             DeviceAddress,
             EndPointAddress,
             DeviceSpeed,
             MaximumPacketLength,
             XHC_INT_TRANSFER_SYNC,
             NULL,
             Data,
             DataLength,
             Timeout,
             TransferResult
             );

ON_EXIT:
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcSyncInterruptTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Submits isochronous transfer to a target USB device.

  @param  This                 This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress        Target device address.
  @param  EndPointAddress      End point address with its direction.
  @param  DeviceSpeed          Device speed, Low speed device doesn't support this
                               type.
  @param  MaximumPacketLength  Maximum packet size that the endpoint is capable of
                               sending or receiving.
  @param  DataBuffersNumber    Number of data buffers prepared for the transfer.
  @param  Data                 Array of pointers to the buffers of data that will
                               be transmitted to USB device or received from USB
                               device.
  @param  DataLength           The size, in bytes, of the data buffer.
  @param  Translator           Transaction translator to use.
  @param  TransferResult       Variable to receive the transfer result.

  @return EFI_UNSUPPORTED      Isochronous transfer is unsupported.

**/
EFI_STATUS
EFIAPI
XhcIsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     UINT8                               DataBuffersNumber,
  IN OUT VOID                                *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                               DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT    UINT32                              *TransferResult
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Submits Async isochronous transfer to a target USB device.

  @param  This                 This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress        Target device address.
  @param  EndPointAddress      End point address with its direction.
  @param  DeviceSpeed          Device speed, Low speed device doesn't support this
                               type.
  @param  MaximumPacketLength  Maximum packet size that the endpoint is capable of
                               sending or receiving.
  @param  DataBuffersNumber    Number of data buffers prepared for the transfer.
  @param  Data                 Array of pointers to the buffers of data that will
                               be transmitted to USB device or received from USB
                               device.
  @param  DataLength           The size, in bytes, of the data buffer.
  @param  Translator           Transaction translator to use.
  @param  IsochronousCallBack  Function to be called when the transfer complete.
  @param  Context              Context passed to the call back function as
                               parameter.

  @return EFI_UNSUPPORTED      Isochronous transfer isn't supported.

**/
EFI_STATUS
EFIAPI
XhcAsyncIsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     UINT8                               DataBuffersNumber,
  IN OUT VOID                                *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                               DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN     VOID                                *Context
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Entry point for EFI drivers.

  @param  ImageHandle       EFI_HANDLE.
  @param  SystemTable       EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS       Success.
  @retval Others            Fail.

**/
EFI_STATUS
EFIAPI
XhcDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gXhciDriverBinding,
           ImageHandle,
           &gXhciComponentName,
           &gXhciComponentName2
           );
}

/**
  Test to see if this driver supports ControllerHandle. Any
  ControllerHandle that has Usb2HcProtocol installed will
  be supported.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          This driver supports this device.
  @return EFI_UNSUPPORTED      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
XhcDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  USB_CLASSC           UsbClassCReg;

  //
  // Test whether there is PCI IO Protocol attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (USB_CLASSC) / sizeof (UINT8),
                        &UsbClassCReg
                        );

  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  //
  // Test whether the controller belongs to Xhci type
  //
  if ((UsbClassCReg.BaseCode != PCI_CLASS_SERIAL) ||
      (UsbClassCReg.SubClassCode != PCI_CLASS_SERIAL_USB) ||
      (UsbClassCReg.ProgInterface != PCI_IF_XHCI))
  {
    Status = EFI_UNSUPPORTED;
  }

ON_EXIT:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Create and initialize a USB_XHCI_INSTANCE structure.

  @param  PciIo                  The PciIo on this device.
  @param  DevicePath             The device path of host controller.
  @param  OriginalPciAttributes  Original PCI attributes.

  @return The allocated and initialized USB_XHCI_INSTANCE structure if created,
          otherwise NULL.

**/
USB_XHCI_INSTANCE *
XhcCreateUsbHc (
  IN EFI_PCI_IO_PROTOCOL       *PciIo,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN UINT64                    OriginalPciAttributes
  )
{
  USB_XHCI_INSTANCE  *Xhc;
  EFI_STATUS         Status;
  UINT32             PageSize;
  UINT16             ExtCapReg;
  UINT8              ReleaseNumber;

  Xhc = AllocateZeroPool (sizeof (USB_XHCI_INSTANCE));

  if (Xhc == NULL) {
    return NULL;
  }

  //
  // Initialize private data structure
  //
  Xhc->Signature             = XHCI_INSTANCE_SIG;
  Xhc->PciIo                 = PciIo;
  Xhc->DevicePath            = DevicePath;
  Xhc->OriginalPciAttributes = OriginalPciAttributes;
  CopyMem (&Xhc->Usb2Hc, &gXhciUsb2HcTemplate, sizeof (EFI_USB2_HC_PROTOCOL));

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        XHC_PCI_SBRN_OFFSET,
                        1,
                        &ReleaseNumber
                        );

  if (!EFI_ERROR (Status)) {
    Xhc->Usb2Hc.MajorRevision = (ReleaseNumber & 0xF0) >> 4;
    Xhc->Usb2Hc.MinorRevision = (ReleaseNumber & 0x0F);
  }

  InitializeListHead (&Xhc->AsyncIntTransfers);

  //
  // Be caution that the Offset passed to XhcReadCapReg() should be Dword align
  //
  Xhc->CapLength        = XhcReadCapReg8 (Xhc, XHC_CAPLENGTH_OFFSET);
  Xhc->HcSParams1.Dword = XhcReadCapReg (Xhc, XHC_HCSPARAMS1_OFFSET);
  Xhc->HcSParams2.Dword = XhcReadCapReg (Xhc, XHC_HCSPARAMS2_OFFSET);
  Xhc->HcCParams.Dword  = XhcReadCapReg (Xhc, XHC_HCCPARAMS_OFFSET);
  Xhc->DBOff            = XhcReadCapReg (Xhc, XHC_DBOFF_OFFSET);
  Xhc->RTSOff           = XhcReadCapReg (Xhc, XHC_RTSOFF_OFFSET);

  //
  // This PageSize field defines the page size supported by the xHC implementation.
  // This xHC supports a page size of 2^(n+12) if bit n is Set. For example,
  // if bit 0 is Set, the xHC supports 4k byte page sizes.
  //
  PageSize = XhcReadOpReg (Xhc, XHC_PAGESIZE_OFFSET);
  if ((PageSize & (~XHC_PAGESIZE_MASK)) != 0) {
    DEBUG ((DEBUG_ERROR, "XhcCreateUsb3Hc: Reserved bits are not 0 for PageSize\n"));
    goto ON_ERROR;
  }

  PageSize     &= XHC_PAGESIZE_MASK;
  Xhc->PageSize = 1 << (HighBitSet32 (PageSize) + 12);

  ExtCapReg              = (UINT16)(Xhc->HcCParams.Data.ExtCapReg);
  Xhc->ExtCapRegBase     = ExtCapReg << 2;
  Xhc->UsbLegSupOffset   = XhcGetCapabilityAddr (Xhc, XHC_CAP_USB_LEGACY);
  Xhc->DebugCapSupOffset = XhcGetCapabilityAddr (Xhc, XHC_CAP_USB_DEBUG);
  Xhc->Usb2SupOffset     = XhcGetSupportedProtocolCapabilityAddr (Xhc, XHC_SUPPORTED_PROTOCOL_DW0_MAJOR_REVISION_USB2);
  Xhc->Usb3SupOffset     = XhcGetSupportedProtocolCapabilityAddr (Xhc, XHC_SUPPORTED_PROTOCOL_DW0_MAJOR_REVISION_USB3);

  DEBUG ((DEBUG_INFO, "XhcCreateUsb3Hc: Capability length 0x%x\n", Xhc->CapLength));
  DEBUG ((DEBUG_INFO, "XhcCreateUsb3Hc: HcSParams1 0x%x\n", Xhc->HcSParams1));
  DEBUG ((DEBUG_INFO, "XhcCreateUsb3Hc: HcSParams2 0x%x\n", Xhc->HcSParams2));
  DEBUG ((DEBUG_INFO, "XhcCreateUsb3Hc: HcCParams 0x%x\n", Xhc->HcCParams));
  DEBUG ((DEBUG_INFO, "XhcCreateUsb3Hc: DBOff 0x%x\n", Xhc->DBOff));
  DEBUG ((DEBUG_INFO, "XhcCreateUsb3Hc: RTSOff 0x%x\n", Xhc->RTSOff));
  DEBUG ((DEBUG_INFO, "XhcCreateUsb3Hc: UsbLegSupOffset 0x%x\n", Xhc->UsbLegSupOffset));
  DEBUG ((DEBUG_INFO, "XhcCreateUsb3Hc: DebugCapSupOffset 0x%x\n", Xhc->DebugCapSupOffset));
  DEBUG ((DEBUG_INFO, "XhcCreateUsb3Hc: Usb2SupOffset 0x%x\n", Xhc->Usb2SupOffset));
  DEBUG ((DEBUG_INFO, "XhcCreateUsb3Hc: Usb3SupOffset 0x%x\n", Xhc->Usb3SupOffset));

  //
  // Create AsyncRequest Polling Timer
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  XhcMonitorAsyncRequests,
                  Xhc,
                  &Xhc->PollTimer
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return Xhc;

ON_ERROR:
  FreePool (Xhc);
  return NULL;
}

/**
  One notified function to stop the Host Controller when gBS->ExitBootServices() called.

  @param  Event                   Pointer to this event
  @param  Context                 Event handler private data

**/
VOID
EFIAPI
XhcExitBootService (
  EFI_EVENT  Event,
  VOID       *Context
  )

{
  USB_XHCI_INSTANCE    *Xhc;
  EFI_PCI_IO_PROTOCOL  *PciIo;

  Xhc   = (USB_XHCI_INSTANCE *)Context;
  PciIo = Xhc->PciIo;

  //
  // Stop AsyncRequest Polling timer then stop the XHCI driver
  // and uninstall the XHCI protocl.
  //
  gBS->SetTimer (Xhc->PollTimer, TimerCancel, 0);
  XhcHaltHC (Xhc, XHC_GENERIC_TIMEOUT);

  if (Xhc->PollTimer != NULL) {
    gBS->CloseEvent (Xhc->PollTimer);
  }

  XhcClearBiosOwnership (Xhc);

  //
  // Restore original PCI attributes
  //
  PciIo->Attributes (
           PciIo,
           EfiPciIoAttributeOperationSet,
           Xhc->OriginalPciAttributes,
           NULL
           );
}

/**
  Starting the Usb XHCI Driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          supports this device.
  @return EFI_UNSUPPORTED      do not support this device.
  @return EFI_DEVICE_ERROR     cannot be started due to device Error.
  @return EFI_OUT_OF_RESOURCES cannot allocate resources.

**/
EFI_STATUS
EFIAPI
XhcDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT64                    Supports;
  UINT64                    OriginalPciAttributes;
  BOOLEAN                   PciAttributesSaved;
  USB_XHCI_INSTANCE         *Xhc;
  EFI_DEVICE_PATH_PROTOCOL  *HcDevicePath;

  //
  // Open the PciIo Protocol, then enable the USB host controller
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open Device Path Protocol for on USB host controller
  //
  HcDevicePath = NULL;
  Status       = gBS->OpenProtocol (
                        Controller,
                        &gEfiDevicePathProtocolGuid,
                        (VOID **)&HcDevicePath,
                        This->DriverBindingHandle,
                        Controller,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );

  PciAttributesSaved = FALSE;
  //
  // Save original PCI attributes
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationGet,
                    0,
                    &OriginalPciAttributes
                    );

  if (EFI_ERROR (Status)) {
    goto CLOSE_PCIIO;
  }

  PciAttributesSaved = TRUE;

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );
  if (!EFI_ERROR (Status)) {
    Supports &= (UINT64)EFI_PCI_DEVICE_ENABLE;
    Status    = PciIo->Attributes (
                         PciIo,
                         EfiPciIoAttributeOperationEnable,
                         Supports,
                         NULL
                         );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcDriverBindingStart: failed to enable controller\n"));
    goto CLOSE_PCIIO;
  }

  //
  // Create then install USB2_HC_PROTOCOL
  //
  Xhc = XhcCreateUsbHc (PciIo, HcDevicePath, OriginalPciAttributes);

  if (Xhc == NULL) {
    DEBUG ((DEBUG_ERROR, "XhcDriverBindingStart: failed to create USB2_HC\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Enable 64-bit DMA support in the PCI layer if this controller
  // supports it.
  //
  if (Xhc->HcCParams.Data.Ac64 != 0) {
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationEnable,
                      EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE,
                      NULL
                      );
    if (!EFI_ERROR (Status)) {
      Xhc->Support64BitDma = TRUE;
    } else {
      DEBUG ((
        DEBUG_WARN,
        "%a: failed to enable 64-bit DMA on 64-bit capable controller @ %p (%r)\n",
        __func__,
        Controller,
        Status
        ));
    }
  }

  XhcSetBiosOwnership (Xhc);

  Status = XhcResetHC (Xhc, XHC_RESET_TIMEOUT);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to reset HC\n", __func__));
    goto FREE_POOL;
  }

  ASSERT (XhcIsHalt (Xhc));

  //
  // After Chip Hardware Reset wait until the Controller Not Ready (CNR) flag
  // in the USBSTS is '0' before writing any xHC Operational or Runtime registers.
  //
  ASSERT (!(XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_CNR)));

  //
  // Initialize the schedule
  //
  XhcInitSched (Xhc);

  //
  // Start the Host Controller
  //
  XhcRunHC (Xhc, XHC_GENERIC_TIMEOUT);

  //
  // Start the asynchronous interrupt monitor
  //
  Status = gBS->SetTimer (Xhc->PollTimer, TimerPeriodic, XHC_ASYNC_TIMER_INTERVAL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcDriverBindingStart: failed to start async interrupt monitor\n"));
    XhcHaltHC (Xhc, XHC_GENERIC_TIMEOUT);
    goto FREE_POOL;
  }

  //
  // Create event to stop the HC when exit boot service.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  XhcExitBootService,
                  Xhc,
                  &gEfiEventExitBootServicesGuid,
                  &Xhc->ExitBootServiceEvent
                  );
  if (EFI_ERROR (Status)) {
    goto FREE_POOL;
  }

  //
  // Install the component name protocol, don't fail the start
  // because of something for display.
  //
  AddUnicodeString2 (
    "eng",
    gXhciComponentName.SupportedLanguages,
    &Xhc->ControllerNameTable,
    L"eXtensible Host Controller (USB 3.0)",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gXhciComponentName2.SupportedLanguages,
    &Xhc->ControllerNameTable,
    L"eXtensible Host Controller (USB 3.0)",
    FALSE
    );

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiUsb2HcProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Xhc->Usb2Hc
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcDriverBindingStart: failed to install USB2_HC Protocol\n"));
    goto FREE_POOL;
  }

  DEBUG ((DEBUG_INFO, "XhcDriverBindingStart: XHCI started for controller @ %x\n", Controller));
  return EFI_SUCCESS;

FREE_POOL:
  gBS->CloseEvent (Xhc->PollTimer);
  XhcFreeSched (Xhc);
  FreePool (Xhc);

CLOSE_PCIIO:
  if (PciAttributesSaved) {
    //
    // Restore original PCI attributes
    //
    PciIo->Attributes (
             PciIo,
             EfiPciIoAttributeOperationSet,
             OriginalPciAttributes,
             NULL
             );
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to stop driver on.
  @param  NumberOfChildren     Number of Children in the ChildHandleBuffer.
  @param  ChildHandleBuffer    List of handles for the children we need to stop.

  @return EFI_SUCCESS          Success.
  @return EFI_DEVICE_ERROR     Fail.

**/
EFI_STATUS
EFIAPI
XhcDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS            Status;
  EFI_USB2_HC_PROTOCOL  *Usb2Hc;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  USB_XHCI_INSTANCE     *Xhc;
  UINT8                 Index;

  //
  // Test whether the Controller handler passed in is a valid
  // Usb controller handle that should be supported, if not,
  // return the error status directly
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  (VOID **)&Usb2Hc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  Usb2Hc
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Xhc   = XHC_FROM_THIS (Usb2Hc);
  PciIo = Xhc->PciIo;

  //
  // Stop AsyncRequest Polling timer then stop the XHCI driver
  // and uninstall the XHCI protocl.
  //
  gBS->SetTimer (Xhc->PollTimer, TimerCancel, 0);

  //
  // Disable the device slots occupied by these devices on its downstream ports.
  // Entry 0 is reserved.
  //
  for (Index = 0; Index < 255; Index++) {
    if (!Xhc->UsbDevContext[Index + 1].Enabled ||
        (Xhc->UsbDevContext[Index + 1].SlotId == 0))
    {
      continue;
    }

    if (Xhc->HcCParams.Data.Csz == 0) {
      XhcDisableSlotCmd (Xhc, Xhc->UsbDevContext[Index + 1].SlotId);
    } else {
      XhcDisableSlotCmd64 (Xhc, Xhc->UsbDevContext[Index + 1].SlotId);
    }
  }

  if (Xhc->PollTimer != NULL) {
    gBS->CloseEvent (Xhc->PollTimer);
  }

  if (Xhc->ExitBootServiceEvent != NULL) {
    gBS->CloseEvent (Xhc->ExitBootServiceEvent);
  }

  XhcHaltHC (Xhc, XHC_GENERIC_TIMEOUT);
  XhcClearBiosOwnership (Xhc);
  XhciDelAllAsyncIntTransfers (Xhc);
  XhcFreeSched (Xhc);

  if (Xhc->ControllerNameTable) {
    FreeUnicodeStringTable (Xhc->ControllerNameTable);
  }

  //
  // Restore original PCI attributes
  //
  PciIo->Attributes (
           PciIo,
           EfiPciIoAttributeOperationSet,
           Xhc->OriginalPciAttributes,
           NULL
           );

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  FreePool (Xhc);

  return EFI_SUCCESS;
}

/**
  Converts a time in nanoseconds to a performance counter tick count.

  @param  Time  The time in nanoseconds to be converted to performance counter ticks.
  @return Time in nanoseconds converted to ticks.
**/
UINT64
XhcConvertTimeToTicks (
  IN UINT64  Time
  )
{
  UINT64  Ticks;
  UINT64  Remainder;
  UINT64  Divisor;
  UINTN   Shift;

  // Cache the return values to avoid repeated calls to GetPerformanceCounterProperties ()
  if (!mXhciPerformanceCounterValuesCached) {
    mXhciPerformanceCounterFrequency = GetPerformanceCounterProperties (
                                         &mXhciPerformanceCounterStartValue,
                                         &mXhciPerformanceCounterEndValue
                                         );

    mXhciPerformanceCounterValuesCached = TRUE;
  }

  // Prevent returning a tick value of 0, unless Time is already 0
  if (0 == mXhciPerformanceCounterFrequency) {
    return Time;
  }

  // Nanoseconds per second
  Divisor = 1000000000;

  //
  //           Frequency
  // Ticks = ------------- x Time
  //         1,000,000,000
  //
  Ticks = MultU64x64 (
            DivU64x64Remainder (
              mXhciPerformanceCounterFrequency,
              Divisor,
              &Remainder
              ),
            Time
            );

  //
  // Ensure (Remainder * Time) will not overflow 64-bit.
  //
  // HighBitSet64 (Remainder) + 1 + HighBitSet64 (Time) + 1 <= 64
  //
  Shift     = MAX (0, HighBitSet64 (Remainder) + HighBitSet64 (Time) - 62);
  Remainder = RShiftU64 (Remainder, (UINTN)Shift);
  Divisor   = RShiftU64 (Divisor, (UINTN)Shift);
  Ticks    += DivU64x64Remainder (MultU64x64 (Remainder, Time), Divisor, NULL);

  return Ticks;
}

/**
  Computes and returns the elapsed ticks since PreviousTick. The
  value of PreviousTick is overwritten with the current performance
  counter value.

  @param  PreviousTick    Pointer to PreviousTick count.
  @return The elapsed ticks since PreviousCount. PreviousCount is
          overwritten with the current performance counter value.
**/
UINT64
XhcGetElapsedTicks (
  IN OUT UINT64  *PreviousTick
  )
{
  UINT64  CurrentTick;
  UINT64  Delta;

  CurrentTick = GetPerformanceCounter ();

  //
  // Determine if the counter is counting up or down
  //
  if (mXhciPerformanceCounterStartValue < mXhciPerformanceCounterEndValue) {
    //
    // Counter counts upwards, check for an overflow condition
    //
    if (*PreviousTick > CurrentTick) {
      Delta = (CurrentTick - mXhciPerformanceCounterStartValue) + (mXhciPerformanceCounterEndValue - *PreviousTick);
    } else {
      Delta = CurrentTick - *PreviousTick;
    }
  } else {
    //
    // Counter counts downwards, check for an underflow condition
    //
    if (*PreviousTick < CurrentTick) {
      Delta = (mXhciPerformanceCounterStartValue - CurrentTick) + (*PreviousTick - mXhciPerformanceCounterEndValue);
    } else {
      Delta = *PreviousTick - CurrentTick;
    }
  }

  //
  // Set PreviousTick to CurrentTick
  //
  *PreviousTick = CurrentTick;

  return Delta;
}
