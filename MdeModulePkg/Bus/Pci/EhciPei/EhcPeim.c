/** @file
PEIM to produce gPeiUsb2HostControllerPpiGuid based on gPeiUsbControllerPpiGuid
which is used to enable recovery function from USB Drivers.

Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EhcPeim.h"

//
// Two arrays used to translate the EHCI port state (change)
// to the UEFI protocol's port state (change).
//
USB_PORT_STATE_MAP  mUsbPortStateMap[] = {
  {PORTSC_CONN,     USB_PORT_STAT_CONNECTION},
  {PORTSC_ENABLED,  USB_PORT_STAT_ENABLE},
  {PORTSC_SUSPEND,  USB_PORT_STAT_SUSPEND},
  {PORTSC_OVERCUR,  USB_PORT_STAT_OVERCURRENT},
  {PORTSC_RESET,    USB_PORT_STAT_RESET},
  {PORTSC_POWER,    USB_PORT_STAT_POWER},
  {PORTSC_OWNER,    USB_PORT_STAT_OWNER}
};

USB_PORT_STATE_MAP  mUsbPortChangeMap[] = {
  {PORTSC_CONN_CHANGE,    USB_PORT_STAT_C_CONNECTION},
  {PORTSC_ENABLE_CHANGE,  USB_PORT_STAT_C_ENABLE},
  {PORTSC_OVERCUR_CHANGE, USB_PORT_STAT_C_OVERCURRENT}
};

/**
  Read Ehc Operation register.
  
  @param  Ehc       The EHCI device.
  @param  Offset    The operation register offset.

  @retval the register content read.

**/
UINT32
EhcReadOpReg (
  IN  PEI_USB2_HC_DEV     *Ehc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
 
  ASSERT (Ehc->CapLen != 0);

  Data = MmioRead32 (Ehc->UsbHostControllerBaseAddress + Ehc->CapLen + Offset);
  
  return Data;
}

/**
  Write the data to the EHCI operation register.
  
  @param  Ehc       The EHCI device.
  @param  Offset    EHCI operation register offset.
  @param  Data      The data to write.

**/
VOID
EhcWriteOpReg (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{

  ASSERT (Ehc->CapLen != 0);

  MmioWrite32(Ehc->UsbHostControllerBaseAddress + Ehc->CapLen + Offset, Data);

}

/**
  Set one bit of the operational register while keeping other bits.
  
  @param  Ehc       The EHCI device.
  @param  Offset    The offset of the operational register.
  @param  Bit       The bit mask of the register to set.

**/
VOID
EhcSetOpRegBit (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = EhcReadOpReg (Ehc, Offset);
  Data |= Bit;
  EhcWriteOpReg (Ehc, Offset, Data);
}

/**
  Clear one bit of the operational register while keeping other bits.
  
  @param  Ehc       The EHCI device.
  @param  Offset    The offset of the operational register.
  @param  Bit       The bit mask of the register to clear.

**/
VOID
EhcClearOpRegBit (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = EhcReadOpReg (Ehc, Offset);
  Data &= ~Bit;
  EhcWriteOpReg (Ehc, Offset, Data);
}

/**
  Wait the operation register's bit as specified by Bit 
  to become set (or clear).
  
  @param  Ehc           The EHCI device.
  @param  Offset        The offset of the operational register.
  @param  Bit           The bit mask of the register to wait for.
  @param  WaitToSet     Wait the bit to set or clear.
  @param  Timeout       The time to wait before abort (in millisecond).

  @retval EFI_SUCCESS   The bit successfully changed by host controller.
  @retval EFI_TIMEOUT   The time out occurred.

**/
EFI_STATUS
EhcWaitOpRegBit (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN UINT32               Offset,
  IN UINT32               Bit,
  IN BOOLEAN              WaitToSet,
  IN UINT32               Timeout
  )
{
  UINT32                  Index;

  for (Index = 0; Index < Timeout / EHC_SYNC_POLL_INTERVAL + 1; Index++) {
    if (EHC_REG_BIT_IS_SET (Ehc, Offset, Bit) == WaitToSet) {
      return EFI_SUCCESS;
    }

    MicroSecondDelay (EHC_SYNC_POLL_INTERVAL);
  }

  return EFI_TIMEOUT;
}

/**
  Read EHCI capability register.
  
  @param  Ehc       The EHCI device.
  @param  Offset    Capability register address.

  @retval the register content read.

**/
UINT32
EhcReadCapRegister (
  IN  PEI_USB2_HC_DEV     *Ehc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  
  Data = MmioRead32(Ehc->UsbHostControllerBaseAddress + Offset);
  
  return Data;
}

/**
  Set door bell and wait it to be ACKed by host controller.
  This function is used to synchronize with the hardware.
  
  @param  Ehc       The EHCI device.
  @param  Timeout   The time to wait before abort (in millisecond, ms).

  @retval EFI_TIMEOUT       Time out happened while waiting door bell to set.
  @retval EFI_SUCCESS       Synchronized with the hardware.

**/
EFI_STATUS
EhcSetAndWaitDoorBell (
  IN  PEI_USB2_HC_DEV     *Ehc,
  IN  UINT32              Timeout
  )
{
  EFI_STATUS              Status;
  UINT32                  Data;

  EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_IAAD);

  Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_IAA, TRUE, Timeout);

  //
  // ACK the IAA bit in USBSTS register. Make sure other
  // interrupt bits are not ACKed. These bits are WC (Write Clean).
  //
  Data  = EhcReadOpReg (Ehc, EHC_USBSTS_OFFSET);
  Data &= ~USBSTS_INTACK_MASK;
  Data |= USBSTS_IAA;

  EhcWriteOpReg (Ehc, EHC_USBSTS_OFFSET, Data);

  return Status;
}

/**
  Clear all the interrutp status bits, these bits 
  are Write-Clean.
  
  @param  Ehc       The EHCI device.

**/
VOID
EhcAckAllInterrupt (
  IN  PEI_USB2_HC_DEV         *Ehc
  )
{
  EhcWriteOpReg (Ehc, EHC_USBSTS_OFFSET, USBSTS_INTACK_MASK);
}

/**
  Enable the periodic schedule then wait EHC to 
  actually enable it.
  
  @param  Ehc       The EHCI device.
  @param  Timeout   The time to wait before abort (in millisecond, ms).

  @retval EFI_TIMEOUT       Time out happened while enabling periodic schedule.
  @retval EFI_SUCCESS       The periodical schedule is enabled.

**/
EFI_STATUS
EhcEnablePeriodSchd (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_ENABLE_PERIOD);

  Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_PERIOD_ENABLED, TRUE, Timeout);
  return Status;
}

/**
  Enable asynchrounous schedule.
  
  @param  Ehc       The EHCI device.
  @param  Timeout   Time to wait before abort.

  @retval EFI_SUCCESS       The EHCI asynchronous schedule is enabled.
  @retval Others            Failed to enable the asynchronous scheudle.

**/
EFI_STATUS
EhcEnableAsyncSchd (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_ENABLE_ASYNC);

  Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_ASYNC_ENABLED, TRUE, Timeout);
  return Status;
}

/**
  Check whether Ehc is halted.
  
  @param  Ehc       The EHCI device.

  @retval TRUE      The controller is halted.
  @retval FALSE     The controller isn't halted.

**/
BOOLEAN
EhcIsHalt (
  IN PEI_USB2_HC_DEV      *Ehc
  )
{
  return EHC_REG_BIT_IS_SET (Ehc, EHC_USBSTS_OFFSET, USBSTS_HALT);
}

/**
  Check whether system error occurred.
  
  @param  Ehc       The EHCI device.

  @retval TRUE      System error happened.
  @retval FALSE     No system error.

**/
BOOLEAN
EhcIsSysError (
  IN PEI_USB2_HC_DEV      *Ehc
  )
{
  return EHC_REG_BIT_IS_SET (Ehc, EHC_USBSTS_OFFSET, USBSTS_SYS_ERROR);
}

/**
  Reset the host controller.
  
  @param  Ehc             The EHCI device.
  @param  Timeout         Time to wait before abort (in millisecond, ms).

  @retval EFI_TIMEOUT     The transfer failed due to time out.
  @retval Others          Failed to reset the host.

**/
EFI_STATUS
EhcResetHC (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  //
  // Host can only be reset when it is halt. If not so, halt it
  //
  if (!EHC_REG_BIT_IS_SET (Ehc, EHC_USBSTS_OFFSET, USBSTS_HALT)) {
    Status = EhcHaltHC (Ehc, Timeout);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_RESET);
  Status = EhcWaitOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_RESET, FALSE, Timeout);
  return Status;
}

/**
  Halt the host controller.
  
  @param  Ehc             The EHCI device.
  @param  Timeout         Time to wait before abort.

  @retval EFI_TIMEOUT     Failed to halt the controller before Timeout.
  @retval EFI_SUCCESS     The EHCI is halt.

**/
EFI_STATUS
EhcHaltHC (
  IN PEI_USB2_HC_DEV     *Ehc,
  IN UINT32              Timeout
  )
{
  EFI_STATUS              Status;

  EhcClearOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_RUN);
  Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_HALT, TRUE, Timeout);
  return Status;
}

/**
  Set the EHCI to run.
  
  @param  Ehc             The EHCI device.
  @param  Timeout         Time to wait before abort.

  @retval EFI_SUCCESS     The EHCI is running.
  @retval Others          Failed to set the EHCI to run.

**/
EFI_STATUS
EhcRunHC (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_RUN);
  Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_HALT, FALSE, Timeout);
  return Status;
}

/**
  Power On All EHCI Ports.
  
  @param  Ehc             The EHCI device.

**/
VOID
EhcPowerOnAllPorts (
  IN PEI_USB2_HC_DEV          *Ehc
  )
{
  UINT8 PortNumber;
  UINT8 Index;

  PortNumber = (UINT8)(Ehc->HcStructParams & HCSP_NPORTS);
  for (Index = 0; Index < PortNumber; Index++) {
    EhcSetOpRegBit (Ehc, EHC_PORT_STAT_OFFSET + 4 * Index, PORTSC_POWER);
  }
}

/**
  Initialize the HC hardware. 
  EHCI spec lists the five things to do to initialize the hardware.
  1. Program CTRLDSSEGMENT.
  2. Set USBINTR to enable interrupts.
  3. Set periodic list base.
  4. Set USBCMD, interrupt threshold, frame list size etc.
  5. Write 1 to CONFIGFLAG to route all ports to EHCI.
  
  @param  Ehc             The EHCI device.

  @retval EFI_SUCCESS     The EHCI has come out of halt state.
  @retval EFI_TIMEOUT     Time out happened.

**/
EFI_STATUS
EhcInitHC (
  IN PEI_USB2_HC_DEV      *Ehc
  )
{
  EFI_STATUS              Status;
  EFI_PHYSICAL_ADDRESS        TempPtr;
  UINTN               PageNumber;
  
  ASSERT (EhcIsHalt (Ehc));

  //
  // Allocate the periodic frame and associated memeory
  // management facilities if not already done.
  //
  if (Ehc->PeriodFrame != NULL) {
    EhcFreeSched (Ehc);
  }
  PageNumber =  sizeof(PEI_URB)/PAGESIZE +1;
  Status = PeiServicesAllocatePages (
             EfiBootServicesCode,
             PageNumber,
             &TempPtr
             );
  Ehc->Urb = (PEI_URB *) ((UINTN) TempPtr);
  if (Ehc->Urb  == NULL) {
    return Status;
  }

  EhcPowerOnAllPorts (Ehc);  
  MicroSecondDelay (EHC_ROOT_PORT_RECOVERY_STALL);
  
  Status = EhcInitSched (Ehc);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // 1. Program the CTRLDSSEGMENT register with the high 32 bit addr
  //
  EhcWriteOpReg (Ehc, EHC_CTRLDSSEG_OFFSET, Ehc->High32bitAddr);

  //
  // 2. Clear USBINTR to disable all the interrupt. UEFI works by polling
  //
  EhcWriteOpReg (Ehc, EHC_USBINTR_OFFSET, 0);

  //
  // 3. Program periodic frame list, already done in EhcInitSched
  // 4. Start the Host Controller
  //
  EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_RUN);

  //
  // 5. Set all ports routing to EHC
  //
  EhcSetOpRegBit (Ehc, EHC_CONFIG_FLAG_OFFSET, CONFIGFLAG_ROUTE_EHC);

  //
  // Wait roothub port power stable
  //
  MicroSecondDelay (EHC_ROOT_PORT_RECOVERY_STALL);

  Status = EhcEnablePeriodSchd (Ehc, EHC_GENERIC_TIMEOUT);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EhcEnableAsyncSchd (Ehc, EHC_GENERIC_TIMEOUT);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Submits bulk transfer to a bulk endpoint of a USB device.
  
  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB2_HOST_CONTROLLER_PPI.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction in bit 7.
  @param  DeviceSpeed           Device speed, Low speed device doesn't support 
                                bulk transfer.
  @param  MaximumPacketLength   Maximum packet size the endpoint is capable of 
                                sending or receiving.
  @param  Data                  Array of pointers to the buffers of data to transmit 
                                from or receive into.
  @param  DataLength            The lenght of the data buffer.
  @param  DataToggle            On input, the initial data toggle for the transfer;
                                On output, it is updated to to next data toggle to use of 
                                the subsequent bulk transfer.
  @param  TimeOut               Indicates the maximum time, in millisecond, which the
                                transfer is allowed to complete.
                                If Timeout is 0, then the caller must wait for the function
                                to be completed until EFI_SUCCESS or EFI_DEVICE_ERROR is returned.
  @param  Translator            A pointr to the transaction translator data.                                
  @param  TransferResult        A pointer to the detailed result information of the
                                bulk transfer.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @retval EFI_INVALID_PARAMETER Parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
EhcBulkTransfer (
  IN EFI_PEI_SERVICES                     **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI         *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  OUT VOID                            *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN  OUT UINTN                           *DataLength,
  IN  OUT UINT8                           *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  )
{
  PEI_USB2_HC_DEV         *Ehc;
  PEI_URB                 *Urb;
  EFI_STATUS              Status;

  //
  // Validate the parameters
  //
  if ((DataLength == NULL) || (*DataLength == 0) || 
      (Data == NULL) || (Data[0] == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 0) && (*DataToggle != 1)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_LOW) ||
      ((DeviceSpeed == EFI_USB_SPEED_FULL) && (MaximumPacketLength > 64)) ||
      ((EFI_USB_SPEED_HIGH == DeviceSpeed) && (MaximumPacketLength > 512))) {
    return EFI_INVALID_PARAMETER;
  }

  Ehc =PEI_RECOVERY_USB_EHC_DEV_FROM_EHCI_THIS(This);
  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;

  if (EhcIsHalt (Ehc) || EhcIsSysError (Ehc)) {
    EhcAckAllInterrupt (Ehc);
    goto ON_EXIT;
  }

  EhcAckAllInterrupt (Ehc);

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  //
  Urb = EhcCreateUrb (
          Ehc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          *DataToggle,
          MaximumPacketLength,
          Translator,
          EHC_BULK_TRANSFER,
          NULL,
          Data[0],
          *DataLength,
          NULL,
          NULL,
          1
          );

  if (Urb == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  EhcLinkQhToAsync (Ehc, Urb->Qh);
  Status = EhcExecTransfer (Ehc, Urb, TimeOut);
  EhcUnlinkQhFromAsync (Ehc, Urb->Qh);

  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;
  *DataToggle     = Urb->DataToggle;

  if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  }

  EhcAckAllInterrupt (Ehc);
  EhcFreeUrb (Ehc, Urb);

ON_EXIT:
  return Status;
}

/**
  Retrieves the number of root hub ports.

  @param[in]  PeiServices   The pointer to the PEI Services Table.
  @param[in]  This          The pointer to this instance of the 
                            PEI_USB2_HOST_CONTROLLER_PPI.
  @param[out] PortNumber    The pointer to the number of the root hub ports.                                
                                
  @retval EFI_SUCCESS           The port number was retrieved successfully.
  @retval EFI_INVALID_PARAMETER PortNumber is NULL.

**/
EFI_STATUS
EFIAPI
EhcGetRootHubPortNumber (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI           *This,
  OUT UINT8                                 *PortNumber
  )
{

  PEI_USB2_HC_DEV             *EhcDev;
  EhcDev = PEI_RECOVERY_USB_EHC_DEV_FROM_EHCI_THIS (This);
  
  if (PortNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }  
  
  *PortNumber = (UINT8)(EhcDev->HcStructParams & HCSP_NPORTS);
  return EFI_SUCCESS;
  
}

/**
  Clears a feature for the specified root hub port.
  
  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB2_HOST_CONTROLLER_PPI.
  @param  PortNumber            Specifies the root hub port whose feature
                                is requested to be cleared.
  @param  PortFeature           Indicates the feature selector associated with the
                                feature clear request.

  @retval EFI_SUCCESS            The feature specified by PortFeature was cleared 
                                 for the USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER  PortNumber is invalid or PortFeature is invalid.

**/
EFI_STATUS
EFIAPI
EhcClearRootHubPortFeature (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI           *This,
  IN  UINT8                 PortNumber,
  IN  EFI_USB_PORT_FEATURE  PortFeature
  )
{
  PEI_USB2_HC_DEV         *Ehc;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  EFI_STATUS              Status;

  Ehc       = PEI_RECOVERY_USB_EHC_DEV_FROM_EHCI_THIS (This);
  Status    = EFI_SUCCESS;

  TotalPort = (Ehc->HcStructParams & HCSP_NPORTS);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset  = EHC_PORT_STAT_OFFSET + (4 * PortNumber);
  State   = EhcReadOpReg (Ehc, Offset);
  State &= ~PORTSC_CHANGE_MASK;

  switch (PortFeature) {
  case EfiUsbPortEnable:
    //
    // Clear PORT_ENABLE feature means disable port.
    //
    State &= ~PORTSC_ENABLED;
    EhcWriteOpReg (Ehc, Offset, State);
    break;

  case EfiUsbPortSuspend:
    //
    // A write of zero to this bit is ignored by the host
    // controller. The host controller will unconditionally
    // set this bit to a zero when:
    //   1. software sets the Forct Port Resume bit to a zero from a one.
    //   2. software sets the Port Reset bit to a one frome a zero.
    //
    State &= ~PORSTSC_RESUME;
    EhcWriteOpReg (Ehc, Offset, State);
    break;

  case EfiUsbPortReset:
    //
    // Clear PORT_RESET means clear the reset signal.
    //
    State &= ~PORTSC_RESET;
    EhcWriteOpReg (Ehc, Offset, State);
    break;

  case EfiUsbPortOwner:
    //
    // Clear port owner means this port owned by EHC
    //
    State &= ~PORTSC_OWNER;
    EhcWriteOpReg (Ehc, Offset, State);
    break;

  case EfiUsbPortConnectChange:
    //
    // Clear connect status change
    //
    State |= PORTSC_CONN_CHANGE;
    EhcWriteOpReg (Ehc, Offset, State);
    break;

  case EfiUsbPortEnableChange:
    //
    // Clear enable status change
    //
    State |= PORTSC_ENABLE_CHANGE;
    EhcWriteOpReg (Ehc, Offset, State);
    break;

  case EfiUsbPortOverCurrentChange:
    //
    // Clear PortOverCurrent change
    //
    State |= PORTSC_OVERCUR_CHANGE;
    EhcWriteOpReg (Ehc, Offset, State);
    break;

  case EfiUsbPortPower:
  case EfiUsbPortSuspendChange:
  case EfiUsbPortResetChange:
    //
    // Not supported or not related operation
    //
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

ON_EXIT:
  return Status;
}

/**
  Sets a feature for the specified root hub port.
  
  @param  PeiServices           The pointer of EFI_PEI_SERVICES
  @param  This                  The pointer of PEI_USB2_HOST_CONTROLLER_PPI
  @param  PortNumber            Root hub port to set.
  @param  PortFeature           Feature to set.

  @retval EFI_SUCCESS            The feature specified by PortFeature was set.
  @retval EFI_INVALID_PARAMETER  PortNumber is invalid or PortFeature is invalid.
  @retval EFI_TIMEOUT            The time out occurred.

**/
EFI_STATUS
EFIAPI
EhcSetRootHubPortFeature (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI           *This,
  IN UINT8                                  PortNumber,
  IN EFI_USB_PORT_FEATURE                   PortFeature
  )
{
  PEI_USB2_HC_DEV         *Ehc;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  EFI_STATUS              Status;

  Ehc       = PEI_RECOVERY_USB_EHC_DEV_FROM_EHCI_THIS (This);
  Status    = EFI_SUCCESS;

  TotalPort = (Ehc->HcStructParams & HCSP_NPORTS);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset  = (UINT32) (EHC_PORT_STAT_OFFSET + (4 * PortNumber));
  State   = EhcReadOpReg (Ehc, Offset);

  //
  // Mask off the port status change bits, these bits are
  // write clean bit
  //
  State &= ~PORTSC_CHANGE_MASK;

  switch (PortFeature) {
  case EfiUsbPortEnable:
    //
    // Sofeware can't set this bit, Port can only be enable by
    // EHCI as a part of the reset and enable
    //
    State |= PORTSC_ENABLED;
    EhcWriteOpReg (Ehc, Offset, State);
    break;

  case EfiUsbPortSuspend:
    State |= PORTSC_SUSPEND;
    EhcWriteOpReg (Ehc, Offset, State);
    break;

  case EfiUsbPortReset:
    //
    // Make sure Host Controller not halt before reset it
    //
    if (EhcIsHalt (Ehc)) {
      Status = EhcRunHC (Ehc, EHC_GENERIC_TIMEOUT);

      if (EFI_ERROR (Status)) {
        break;
      }
    }
    
    //
    // Set one to PortReset bit must also set zero to PortEnable bit
    //
    State |= PORTSC_RESET;
    State &= ~PORTSC_ENABLED;
    EhcWriteOpReg (Ehc, Offset, State);
    break;

  case EfiUsbPortPower:
    //
    // Not supported, ignore the operation
    //
    Status = EFI_SUCCESS;
    break;

  case EfiUsbPortOwner:
    State |= PORTSC_OWNER;
    EhcWriteOpReg (Ehc, Offset, State);
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  }

ON_EXIT:
  return Status;
}

/**
  Retrieves the current status of a USB root hub port.
  
  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  This                   The pointer of PEI_USB2_HOST_CONTROLLER_PPI.
  @param  PortNumber             The root hub port to retrieve the state from.  
  @param  PortStatus             Variable to receive the port state.

  @retval EFI_SUCCESS            The status of the USB root hub port specified.
                                 by PortNumber was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER  PortNumber is invalid.

**/
EFI_STATUS
EFIAPI
EhcGetRootHubPortStatus (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI           *This,
  IN  UINT8                                 PortNumber,
  OUT EFI_USB_PORT_STATUS                   *PortStatus
  )
{
  PEI_USB2_HC_DEV         *Ehc;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  UINTN                   Index;
  UINTN                   MapSize;
  EFI_STATUS              Status;

  if (PortStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Ehc       =  PEI_RECOVERY_USB_EHC_DEV_FROM_EHCI_THIS(This);
  Status    = EFI_SUCCESS;

  TotalPort = (Ehc->HcStructParams & HCSP_NPORTS);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset                        = (UINT32) (EHC_PORT_STAT_OFFSET + (4 * PortNumber));
  PortStatus->PortStatus        = 0;
  PortStatus->PortChangeStatus  = 0;

  State                         = EhcReadOpReg (Ehc, Offset);

  //
  // Identify device speed. If in K state, it is low speed.
  // If the port is enabled after reset, the device is of 
  // high speed. The USB bus driver should retrieve the actual
  // port speed after reset. 
  //
  if (EHC_BIT_IS_SET (State, PORTSC_LINESTATE_K)) {
    PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;

  } else if (EHC_BIT_IS_SET (State, PORTSC_ENABLED)) {
    PortStatus->PortStatus |= USB_PORT_STAT_HIGH_SPEED;
  }
  
  //
  // Convert the EHCI port/port change state to UEFI status
  //
  MapSize = sizeof (mUsbPortStateMap) / sizeof (USB_PORT_STATE_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (EHC_BIT_IS_SET (State, mUsbPortStateMap[Index].HwState)) {
      PortStatus->PortStatus = (UINT16) (PortStatus->PortStatus | mUsbPortStateMap[Index].UefiState);
    }
  }

  MapSize = sizeof (mUsbPortChangeMap) / sizeof (USB_PORT_STATE_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (EHC_BIT_IS_SET (State, mUsbPortChangeMap[Index].HwState)) {
      PortStatus->PortChangeStatus = (UINT16) (PortStatus->PortChangeStatus | mUsbPortChangeMap[Index].UefiState);
    }
  }

ON_EXIT:
  return Status;
}

/**
  Submits control transfer to a target USB device.
  
  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  This                   The pointer of PEI_USB2_HOST_CONTROLLER_PPI.
  @param  DeviceAddress          The target device address.
  @param  DeviceSpeed            Target device speed.
  @param  MaximumPacketLength    Maximum packet size the default control transfer 
                                 endpoint is capable of sending or receiving.
  @param  Request                USB device request to send.
  @param  TransferDirection      Specifies the data direction for the data stage.
  @param  Data                   Data buffer to be transmitted or received from USB device.
  @param  DataLength             The size (in bytes) of the data buffer.
  @param  TimeOut                Indicates the maximum timeout, in millisecond.
                                 If Timeout is 0, then the caller must wait for the function
                                 to be completed until EFI_SUCCESS or EFI_DEVICE_ERROR is returned.
  @param  Translator             Transaction translator to be used by this device.
  @param  TransferResult         Return the result of this control transfer.

  @retval EFI_SUCCESS            Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES   The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval EFI_TIMEOUT            Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR       Transfer failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
EhcControlTransfer (
  IN  EFI_PEI_SERVICES                    **PeiServices,
  IN  PEI_USB2_HOST_CONTROLLER_PPI        *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  EFI_USB_DEVICE_REQUEST              *Request,
  IN  EFI_USB_DATA_DIRECTION              TransferDirection,
  IN  OUT VOID                            *Data,
  IN  OUT UINTN                           *DataLength,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  )
{
  PEI_USB2_HC_DEV         *Ehc;
  PEI_URB                 *Urb;
  UINT8                   Endpoint;
  EFI_STATUS              Status;

  //
  // Validate parameters
  //
  if ((Request == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbDataIn) &&
      (TransferDirection != EfiUsbDataOut) &&
      (TransferDirection != EfiUsbNoData)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection == EfiUsbNoData) && 
      ((Data != NULL) || (*DataLength != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbNoData) && 
     ((Data == NULL) || (*DataLength == 0))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MaximumPacketLength != 8)  && (MaximumPacketLength != 16) &&
      (MaximumPacketLength != 32) && (MaximumPacketLength != 64)) {
    return EFI_INVALID_PARAMETER;
  }


  if ((DeviceSpeed == EFI_USB_SPEED_LOW) ||
      ((DeviceSpeed == EFI_USB_SPEED_FULL) && (MaximumPacketLength > 64)) ||
      ((EFI_USB_SPEED_HIGH == DeviceSpeed) && (MaximumPacketLength > 512))) {
    return EFI_INVALID_PARAMETER;
  }

  Ehc             = PEI_RECOVERY_USB_EHC_DEV_FROM_EHCI_THIS (This);

  Status          = EFI_DEVICE_ERROR;
  *TransferResult = EFI_USB_ERR_SYSTEM;

  if (EhcIsHalt (Ehc) || EhcIsSysError (Ehc)) {
    EhcAckAllInterrupt (Ehc);
    goto ON_EXIT;
  }

  EhcAckAllInterrupt (Ehc);

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  //
  //
  // Encode the direction in address, although default control
  // endpoint is bidirectional. EhcCreateUrb expects this
  // combination of Ep addr and its direction.
  //
  Endpoint = (UINT8) (0 | ((TransferDirection == EfiUsbDataIn) ? 0x80 : 0));
  Urb = EhcCreateUrb (
          Ehc,
          DeviceAddress,
          Endpoint,
          DeviceSpeed,
          0,
          MaximumPacketLength,
          Translator,
          EHC_CTRL_TRANSFER,
          Request,
          Data,
          *DataLength,
          NULL,
          NULL,
          1
          );

  if (Urb == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  EhcLinkQhToAsync (Ehc, Urb->Qh);
  Status = EhcExecTransfer (Ehc, Urb, TimeOut);
  EhcUnlinkQhFromAsync (Ehc, Urb->Qh);

  //
  // Get the status from URB. The result is updated in EhcCheckUrbResult
  // which is called by EhcExecTransfer
  //
  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;

  if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  }

  EhcAckAllInterrupt (Ehc);
  EhcFreeUrb (Ehc, Urb);

ON_EXIT:
  return Status;
}

/**
  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS            PPI successfully installed.

**/
EFI_STATUS
EFIAPI
EhcPeimEntry (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  PEI_USB_CONTROLLER_PPI      *ChipSetUsbControllerPpi;
  EFI_STATUS                  Status;
  UINT8                       Index;
  UINTN                       ControllerType;
  UINTN                       BaseAddress;
  UINTN                       MemPages;
  PEI_USB2_HC_DEV             *EhcDev;
  EFI_PHYSICAL_ADDRESS        TempPtr;

  //
  // Shadow this PEIM to run from memory
  //
  if (!EFI_ERROR (PeiServicesRegisterForShadow (FileHandle))) {
    return EFI_SUCCESS;
  }

  Status = PeiServicesLocatePpi (
             &gPeiUsbControllerPpiGuid,
             0,
             NULL,
             (VOID **) &ChipSetUsbControllerPpi
             );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Index = 0;
  while (TRUE) {
    Status = ChipSetUsbControllerPpi->GetUsbController (
                                        (EFI_PEI_SERVICES **) PeiServices,
                                        ChipSetUsbControllerPpi,
                                        Index,
                                        &ControllerType,
                                        &BaseAddress
                                        );
    //
    // When status is error, meant no controller is found
    //
    if (EFI_ERROR (Status)) {
      break;
    }
    
    //
    // This PEIM is for UHC type controller.
    //
    if (ControllerType != PEI_EHCI_CONTROLLER) {
      Index++;
      continue;
    }

    MemPages = sizeof (PEI_USB2_HC_DEV) / PAGESIZE + 1;
    Status = PeiServicesAllocatePages (
               EfiBootServicesCode,
               MemPages,
               &TempPtr
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem((VOID *)(UINTN)TempPtr, MemPages*PAGESIZE);
    EhcDev = (PEI_USB2_HC_DEV *) ((UINTN) TempPtr);

    EhcDev->Signature = USB2_HC_DEV_SIGNATURE;

    EhcDev->UsbHostControllerBaseAddress = (UINT32) BaseAddress;


    EhcDev->HcStructParams = EhcReadCapRegister (EhcDev, EHC_HCSPARAMS_OFFSET);
    EhcDev->HcCapParams    = EhcReadCapRegister (EhcDev, EHC_HCCPARAMS_OFFSET);
    EhcDev->CapLen         = EhcReadCapRegister (EhcDev, EHC_CAPLENGTH_OFFSET) & 0x0FF;
    //
    // Initialize Uhc's hardware
    //
    Status = InitializeUsbHC (EhcDev);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    EhcDev->Usb2HostControllerPpi.ControlTransfer          = EhcControlTransfer;
    EhcDev->Usb2HostControllerPpi.BulkTransfer             = EhcBulkTransfer;
    EhcDev->Usb2HostControllerPpi.GetRootHubPortNumber     = EhcGetRootHubPortNumber;
    EhcDev->Usb2HostControllerPpi.GetRootHubPortStatus     = EhcGetRootHubPortStatus;
    EhcDev->Usb2HostControllerPpi.SetRootHubPortFeature    = EhcSetRootHubPortFeature;
    EhcDev->Usb2HostControllerPpi.ClearRootHubPortFeature  = EhcClearRootHubPortFeature;

    EhcDev->PpiDescriptor.Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
    EhcDev->PpiDescriptor.Guid = &gPeiUsb2HostControllerPpiGuid;
    EhcDev->PpiDescriptor.Ppi = &EhcDev->Usb2HostControllerPpi;

    Status = PeiServicesInstallPpi (&EhcDev->PpiDescriptor);
    if (EFI_ERROR (Status)) {
      Index++;
      continue;
    }

    Index++;
  }

  return EFI_SUCCESS;
}

/**
  @param  EhcDev                 EHCI Device.

  @retval EFI_SUCCESS            EHCI successfully initialized.
  @retval EFI_ABORTED            EHCI was failed to be initialized.

**/
EFI_STATUS
InitializeUsbHC (
  IN PEI_USB2_HC_DEV      *EhcDev  
  )
{
  EFI_STATUS  Status;

   	
  EhcResetHC (EhcDev, EHC_RESET_TIMEOUT);

  Status = EhcInitHC (EhcDev);

  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;      
  }
  
  return EFI_SUCCESS;
}
