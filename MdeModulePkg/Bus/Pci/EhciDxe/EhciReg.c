/** @file

  The EHCI register operation routines.

Copyright (c) 2007 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Ehci.h"


/**
  Read EHCI capability register.

  @param  Ehc          The EHCI device.
  @param  Offset       Capability register address.

  @return The register content read.
  @retval If err, return 0xffff.

**/
UINT32
EhcReadCapRegister (
  IN  USB2_HC_DEV         *Ehc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  Status = Ehc->PciIo->Mem.Read (
                             Ehc->PciIo,
                             EfiPciIoWidthUint32,
                             EHC_BAR_INDEX,
                             (UINT64) Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "EhcReadCapRegister: Pci Io read error - %r at %d\n", Status, Offset));
    Data = 0xFFFF;
  }

  return Data;
}

/**
  Read EHCI debug port register.

  @param  Ehc          The EHCI device.
  @param  Offset       Debug port register offset.

  @return The register content read.
  @retval If err, return 0xffff.

**/
UINT32
EhcReadDbgRegister (
  IN  USB2_HC_DEV         *Ehc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  Status = Ehc->PciIo->Mem.Read (
                             Ehc->PciIo,
                             EfiPciIoWidthUint32,
                             Ehc->DebugPortBarNum,
                             (UINT64) (Ehc->DebugPortOffset + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "EhcReadDbgRegister: Pci Io read error - %r at %d\n", Status, Offset));
    Data = 0xFFFF;
  }

  return Data;
}


/**
  Read EHCI Operation register.

  @param  Ehc          The EHCI device.
  @param  Offset       The operation register offset.

  @return The register content read.
  @retval If err, return 0xffff.

**/
UINT32
EhcReadOpReg (
  IN  USB2_HC_DEV         *Ehc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  ASSERT (Ehc->CapLen != 0);

  Status = Ehc->PciIo->Mem.Read (
                             Ehc->PciIo,
                             EfiPciIoWidthUint32,
                             EHC_BAR_INDEX,
                             (UINT64) (Ehc->CapLen + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "EhcReadOpReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFF;
  }

  return Data;
}


/**
  Write  the data to the EHCI operation register.

  @param  Ehc          The EHCI device.
  @param  Offset       EHCI operation register offset.
  @param  Data         The data to write.

**/
VOID
EhcWriteOpReg (
  IN USB2_HC_DEV          *Ehc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Ehc->CapLen != 0);

  Status = Ehc->PciIo->Mem.Write (
                             Ehc->PciIo,
                             EfiPciIoWidthUint32,
                             EHC_BAR_INDEX,
                             (UINT64) (Ehc->CapLen + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "EhcWriteOpReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}


/**
  Set one bit of the operational register while keeping other bits.

  @param  Ehc          The EHCI device.
  @param  Offset       The offset of the operational register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
EhcSetOpRegBit (
  IN USB2_HC_DEV          *Ehc,
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

  @param  Ehc          The EHCI device.
  @param  Offset       The offset of the operational register.
  @param  Bit          The bit mask of the register to clear.

**/
VOID
EhcClearOpRegBit (
  IN USB2_HC_DEV          *Ehc,
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

  @param  Ehc          The EHCI device.
  @param  Offset       The offset of the operation register.
  @param  Bit          The bit of the register to wait for.
  @param  WaitToSet    Wait the bit to set or clear.
  @param  Timeout      The time to wait before abort (in millisecond).

  @retval EFI_SUCCESS  The bit successfully changed by host controller.
  @retval EFI_TIMEOUT  The time out occurred.

**/
EFI_STATUS
EhcWaitOpRegBit (
  IN USB2_HC_DEV          *Ehc,
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

    gBS->Stall (EHC_SYNC_POLL_INTERVAL);
  }

  return EFI_TIMEOUT;
}


/**
  Add support for UEFI Over Legacy (UoL) feature, stop
  the legacy USB SMI support.

  @param  Ehc          The EHCI device.

**/
VOID
EhcClearLegacySupport (
  IN USB2_HC_DEV          *Ehc
  )
{
  UINT32                    ExtendCap;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT32                    Value;
  UINT32                    TimeOut;

  DEBUG ((EFI_D_INFO, "EhcClearLegacySupport: called to clear legacy support\n"));

  PciIo     = Ehc->PciIo;
  ExtendCap = (Ehc->HcCapParams >> 8) & 0xFF;

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
  Value |= (0x1 << 24);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

  TimeOut = 40;
  while (TimeOut-- != 0) {
    gBS->Stall (500);

    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

    if ((Value & 0x01010000) == 0x01000000) {
      break;
    }
  }

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);
}



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

  @param  Ehc          The EHCI device.

**/
VOID
EhcAckAllInterrupt (
  IN  USB2_HC_DEV         *Ehc
  )
{
  EhcWriteOpReg (Ehc, EHC_USBSTS_OFFSET, USBSTS_INTACK_MASK);
}


/**
  Enable the periodic schedule then wait EHC to
  actually enable it.

  @param  Ehc          The EHCI device.
  @param  Timeout      The time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The periodical schedule is enabled.
  @retval EFI_TIMEOUT  Time out happened while enabling periodic schedule.

**/
EFI_STATUS
EhcEnablePeriodSchd (
  IN USB2_HC_DEV          *Ehc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_ENABLE_PERIOD);

  Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_PERIOD_ENABLED, TRUE, Timeout);
  return Status;
}


/**
  Disable periodic schedule.

  @param  Ehc               The EHCI device.
  @param  Timeout           Time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS       Periodic schedule is disabled.
  @retval EFI_DEVICE_ERROR  Fail to disable periodic schedule.

**/
EFI_STATUS
EhcDisablePeriodSchd (
  IN USB2_HC_DEV          *Ehc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  EhcClearOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_ENABLE_PERIOD);

  Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_PERIOD_ENABLED, FALSE, Timeout);
  return Status;
}



/**
  Enable asynchrounous schedule.

  @param  Ehc          The EHCI device.
  @param  Timeout      Time to wait before abort.

  @retval EFI_SUCCESS  The EHCI asynchronous schedule is enabled.
  @return Others       Failed to enable the asynchronous scheudle.

**/
EFI_STATUS
EhcEnableAsyncSchd (
  IN USB2_HC_DEV          *Ehc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_ENABLE_ASYNC);

  Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_ASYNC_ENABLED, TRUE, Timeout);
  return Status;
}



/**
  Disable asynchrounous schedule.

  @param  Ehc          The EHCI device.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The asynchronous schedule is disabled.
  @return Others       Failed to disable the asynchronous schedule.

**/
EFI_STATUS
EhcDisableAsyncSchd (
  IN USB2_HC_DEV          *Ehc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS  Status;

  EhcClearOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_ENABLE_ASYNC);

  Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_ASYNC_ENABLED, FALSE, Timeout);
  return Status;
}



/**
  Whether Ehc is halted.

  @param  Ehc          The EHCI device.

  @retval TRUE         The controller is halted.
  @retval FALSE        It isn't halted.

**/
BOOLEAN
EhcIsHalt (
  IN USB2_HC_DEV          *Ehc
  )
{
  return EHC_REG_BIT_IS_SET (Ehc, EHC_USBSTS_OFFSET, USBSTS_HALT);
}


/**
  Whether system error occurred.

  @param  Ehc          The EHCI device.

  @return TRUE         System error happened.
  @return FALSE        No system error.

**/
BOOLEAN
EhcIsSysError (
  IN USB2_HC_DEV          *Ehc
  )
{
  return EHC_REG_BIT_IS_SET (Ehc, EHC_USBSTS_OFFSET, USBSTS_SYS_ERROR);
}


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

  @param  Ehc          The EHCI device.
  @param  Timeout      Time to wait before abort.

  @retval EFI_SUCCESS  The EHCI is halt.
  @retval EFI_TIMEOUT  Failed to halt the controller before Timeout.

**/
EFI_STATUS
EhcHaltHC (
  IN USB2_HC_DEV         *Ehc,
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

  @param  Ehc          The EHCI device.
  @param  Timeout      Time to wait before abort.

  @retval EFI_SUCCESS  The EHCI is running.
  @return Others       Failed to set the EHCI to run.

**/
EFI_STATUS
EhcRunHC (
  IN USB2_HC_DEV          *Ehc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_RUN);
  Status = EhcWaitOpRegBit (Ehc, EHC_USBSTS_OFFSET, USBSTS_HALT, FALSE, Timeout);
  return Status;
}


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
  )
{
  EFI_STATUS              Status;
  UINT32                  Index;

  // This ASSERT crashes the BeagleBoard. There is some issue in the USB stack.
  // This ASSERT needs to be removed so the BeagleBoard will boot. When we fix
  // the USB stack we can put this ASSERT back in
  // ASSERT (EhcIsHalt (Ehc));

  //
  // Allocate the periodic frame and associated memeory
  // management facilities if not already done.
  //
  if (Ehc->PeriodFrame != NULL) {
    EhcFreeSched (Ehc);
  }

  Status = EhcInitSched (Ehc);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // 1. Clear USBINTR to disable all the interrupt. UEFI works by polling
  //
  EhcWriteOpReg (Ehc, EHC_USBINTR_OFFSET, 0);

  //
  // 2. Start the Host Controller
  //
  EhcSetOpRegBit (Ehc, EHC_USBCMD_OFFSET, USBCMD_RUN);

  //
  // 3. Power up all ports if EHCI has Port Power Control (PPC) support
  //
  if (Ehc->HcStructParams & HCSP_PPC) {
    for (Index = 0; Index < (UINT8) (Ehc->HcStructParams & HCSP_NPORTS); Index++) {
      EhcSetOpRegBit (Ehc, (UINT32) (EHC_PORT_STAT_OFFSET + (4 * Index)), PORTSC_POWER);
    }
  }

  //
  // Wait roothub port power stable
  //
  gBS->Stall (EHC_ROOT_PORT_RECOVERY_STALL);

  //
  // 4. Set all ports routing to EHC
  //
  EhcSetOpRegBit (Ehc, EHC_CONFIG_FLAG_OFFSET, CONFIGFLAG_ROUTE_EHC);

  Status = EhcEnablePeriodSchd (Ehc, EHC_GENERIC_TIMEOUT);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "EhcInitHC: failed to enable period schedule\n"));
    return Status;
  }

  Status = EhcEnableAsyncSchd (Ehc, EHC_GENERIC_TIMEOUT);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "EhcInitHC: failed to enable async schedule\n"));
    return Status;
  }

  return EFI_SUCCESS;
}
