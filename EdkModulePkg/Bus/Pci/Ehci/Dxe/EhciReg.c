/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    Ehchlp.c

Abstract:


Revision History
--*/

#include "Ehci.h"


VOID
HostReset (
  IN USB2_HC_DEV    *HcDev
  )
{
  UINT32  Value;
  UINT32  TimeOut;

  ReadEhcOperationalReg (
    HcDev,
    USBCMD,
    &Value
    );

  Value = Value & (~USBCMD_RS);
  WriteEhcOperationalReg (
    HcDev,
    USBCMD,
    Value
    );

  TimeOut = 40;
  while (TimeOut --) {
    gBS->Stall (500);
    ReadEhcOperationalReg (
      HcDev,
      USBSTS,
      &Value
      );
    if ((Value & USBSTS_HCH) != 0) {
      break;
    }
  }

  if (TimeOut == 0) {
    DEBUG((gEHCErrorLevel, "TimeOut for clearing Run/Stop bit\n"));
  }

  ReadEhcOperationalReg (
    HcDev,
    USBCMD,
    &Value
    );
  Value = Value | USBCMD_HCRESET;
  WriteEhcOperationalReg (
    HcDev,
    USBCMD,
    Value
    );

  TimeOut = 40;
  while (TimeOut --) {
    gBS->Stall (500);
    ReadEhcOperationalReg (
      HcDev,
      USBCMD,
      &Value
      );
    if ((Value & USBCMD_HCRESET) == 0) {
      break;
    }
  }

  if (TimeOut == 0) {
    DEBUG((gEHCErrorLevel, "TimeOut for Host Reset\n"));
  }

}

EFI_STATUS
ReadEhcCapabiltiyReg (
  IN USB2_HC_DEV             *HcDev,
  IN UINT32                  CapabiltiyRegAddr,
  IN OUT UINT32              *Data
  )
/*++

Routine Description:

  Read  Ehc Capabitlity register

Arguments:

  HcDev              - USB2_HC_DEV
  CapabiltiyRegAddr  - Ehc Capability register address
  Data               - A pointer to data read from register

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  return HcDev->PciIo->Mem.Read (
                             HcDev->PciIo,
                             EfiPciIoWidthUint32,
                             USB_BAR_INDEX,
                             (UINT64) CapabiltiyRegAddr,
                             1,
                             Data
                             );
}

EFI_STATUS
ReadEhcOperationalReg (
  IN USB2_HC_DEV             *HcDev,
  IN UINT32                  OperationalRegAddr,
  IN OUT UINT32              *Data
  )
/*++

Routine Description:

  Read  Ehc Operation register

Arguments:

  HcDev                - USB2_HC_DEV
  OperationalRegAddr   - Ehc Operation register address
  Data                 - A pointer to data read from register

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  ASSERT (HcDev->UsbCapabilityLen);
  return HcDev->PciIo->Mem.Read (
                             HcDev->PciIo,
                             EfiPciIoWidthUint32,
                             USB_BAR_INDEX,
                             (UINT64) (OperationalRegAddr + HcDev->UsbCapabilityLen),
                             1,
                             Data
                             );
}

EFI_STATUS
WriteEhcOperationalReg (
  IN USB2_HC_DEV             *HcDev,
  IN UINT32                  OperationalRegAddr,
  IN UINT32                  Data
  )
/*++

Routine Description:

  Write  Ehc Operation register

Arguments:

  HcDev                - USB2_HC_DEV
  OperationalRegAddr   - Ehc Operation register address
  Data                 - 32bit write to register

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  ASSERT (HcDev->UsbCapabilityLen);
  return HcDev->PciIo->Mem.Write (
                             HcDev->PciIo,
                             EfiPciIoWidthUint32,
                             USB_BAR_INDEX,
                             (UINT64) (OperationalRegAddr + HcDev->UsbCapabilityLen),
                             1,
                             &Data
                             );
}



VOID
ClearLegacySupport (
  IN USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Stop the legacy USB SMI

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  UINT32  EECP;
  UINT32  Value;
  UINT32  TimeOut;

  ReadEhcCapabiltiyReg (
    HcDev,
    HCCPARAMS,
    &EECP
    );

  EECP = (EECP >> 8) & 0xFF;

  DEBUG ((gEHCDebugLevel, "EHCI: EECPBase = 0x%x\n", EECP));


  HcDev->PciIo->Pci.Read (
                     HcDev->PciIo,
                     EfiPciIoWidthUint32,
                     EECP,
                     1,
                     &Value
                     );

  DEBUG((gEHCDebugLevel, "EECP[0] = 0x%x\n", Value));

  HcDev->PciIo->Pci.Read (
                     HcDev->PciIo,
                     EfiPciIoWidthUint32,
                     EECP + 0x4,
                     1,
                     &Value
                     );

  DEBUG((gEHCDebugLevel, "EECP[4] = 0x%x\n", Value));

  HcDev->PciIo->Pci.Read (
                     HcDev->PciIo,
                     EfiPciIoWidthUint32,
                     EECP,
                     1,
                     &Value
                     );

  Value = Value | (0x1 << 24);
  DEBUG((gEHCErrorLevel, "Value Written = 0x%x\n", Value));

  HcDev->PciIo->Pci.Write (
                     HcDev->PciIo,
                     EfiPciIoWidthUint32,
                     EECP,
                     1,
                     &Value
                     );

 TimeOut = 40;
 while (TimeOut --) {
   gBS->Stall (500);

   HcDev->PciIo->Pci.Read (
                      HcDev->PciIo,
                      EfiPciIoWidthUint32,
                      EECP,
                      1,
                      &Value
                      );
  if ((Value & 0x01010000) == 0x01000000) {
    break;
  }
 }

  if (TimeOut == 0) {
    DEBUG((gEHCErrorLevel, "Timeout for getting HC OS Owned Semaphore\n" ));
  }

  DEBUG((gEHCErrorLevel, "After Release Value\n" ));

  HcDev->PciIo->Pci.Read (
                     HcDev->PciIo,
                     EfiPciIoWidthUint32,
                     EECP,
                     1,
                     &Value
                     );

  DEBUG((gEHCDebugLevel, "EECP[0] = 0x%x\n", Value));

  HcDev->PciIo->Pci.Read (
                     HcDev->PciIo,
                     EfiPciIoWidthUint32,
                     EECP + 0x4,
                     1,
                     &Value
                     );

  DEBUG((gEHCDebugLevel, "EECP[4] = 0x%x\n", Value));


}

EFI_STATUS
GetCapabilityLen (
  IN USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Get the length of capability register

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;
  UINT32      CapabilityLenAddr;

  CapabilityLenAddr = CAPLENGTH;

  Status = ReadEhcCapabiltiyReg (
             HcDev,
             CapabilityLenAddr,
             &(HcDev->UsbCapabilityLen)
             );
  HcDev->UsbCapabilityLen = (UINT8) HcDev->UsbCapabilityLen;

  return Status;
}

EFI_STATUS
SetFrameListLen (
  IN USB2_HC_DEV     *HcDev,
  IN UINTN           Length
  )
/*++

Routine Description:

  Set the length of Frame List

Arguments:

  HcDev    - USB2_HC_DEV
  Length   - the required length of frame list

Returns:

  EFI_SUCCESS            Success
  EFI_INVALID_PARAMETER  Invalid parameter
  EFI_DEVICE_ERROR       Fail

--*/
{
  EFI_STATUS  Status;
  UINT32      UsbCommandAddr;
  UINT32      UsbCommandReg;

  UsbCommandAddr = USBCMD;

  if (256 != Length && 512 != Length) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  Status = ReadEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             &UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  if (256 == Length) {
    UsbCommandReg |= USBCMD_FLS_256;
  } else {
    UsbCommandReg |= USBCMD_FLS_512;
  }

  Status = WriteEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

exit:
  return Status;
}

EFI_STATUS
SetFrameListBaseAddr (
  IN USB2_HC_DEV     *HcDev,
  IN UINT32          FrameBuffer
  )
/*++

Routine Description:

  Set base address of frame list first entry

Arguments:

  HcDev        - USB2_HC_DEV
  FrameBuffer  - base address of first entry of frame list

Returns:

--*/
{
  EFI_STATUS  Status;
  UINT32      PeriodicListBaseAddr;
  UINT32      PeriodicListBaseReg;

  Status                = EFI_SUCCESS;
  PeriodicListBaseAddr  = PERIODICLISTBASE;
  PeriodicListBaseReg   = FrameBuffer & 0xfffff000;

  if (IsEhcHalted (HcDev)) {

    Status = WriteEhcOperationalReg (
               HcDev,
               PeriodicListBaseAddr,
               PeriodicListBaseReg
               );
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }

  }

exit:
  return Status;
}

EFI_STATUS
SetAsyncListAddr (
  IN USB2_HC_DEV        *HcDev,
  IN EHCI_QH_ENTITY     *QhPtr
  )
/*++

Routine Description:

  Set address of first Async schedule Qh

Arguments:

  HcDev   - USB2_HC_DEV
  QhPtr   - A pointer to first Qh in the Async schedule

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;
  UINT32      AsyncListAddr;
  UINT32      AsyncListReg;

  AsyncListAddr = ASYNCLISTADDR;
  AsyncListReg  = (UINT32) GET_0B_TO_31B (&(QhPtr->Qh));

  Status = WriteEhcOperationalReg (
             HcDev,
             AsyncListAddr,
             AsyncListReg
             );

  return Status;
}

EFI_STATUS
SetCtrlDataStructSeg (
  IN USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Set register of control and data structure segment

Arguments:

  HcDev  - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail


--*/
{
  EFI_STATUS  Status;
  UINT32      CtrlDsSegmentAddr;
  UINT32      CtrlDsSegmentReg;

  CtrlDsSegmentAddr = CTRLDSSGMENT;
  CtrlDsSegmentReg  = HcDev->High32BitAddr;

  Status = WriteEhcOperationalReg (
             HcDev,
             CtrlDsSegmentAddr,
             CtrlDsSegmentReg
             );

  return Status;
}

EFI_STATUS
SetPortRoutingEhc (
  IN USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Set Ehc port routing bit

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;
  UINT32      ConfigFlagAddr;
  UINT32      ConfigFlagReg;

  ConfigFlagAddr = CONFIGFLAG;

  Status = ReadEhcOperationalReg (
             HcDev,
             ConfigFlagAddr,
             &ConfigFlagReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  ConfigFlagReg |= CONFIGFLAG_CF;
  Status = WriteEhcOperationalReg (
             HcDev,
             ConfigFlagAddr,
             ConfigFlagReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

exit:
  return Status;
}

EFI_STATUS
SetEhcDoorbell (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Set Ehc door bell bit

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;
  UINT32      UsbCommandAddr;
  UINT32      UsbCommandReg;

  UsbCommandAddr = USBCMD;

  Status = ReadEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             &UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  UsbCommandReg |= USBCMD_IAAD;
  Status = WriteEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

exit:
  return Status;
}

EFI_STATUS
ClearEhcAllStatus (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Clear Ehc all status bits

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  UINT32  UsbStatusAddr;

  UsbStatusAddr = USBSTS;

  return WriteEhcOperationalReg (
           HcDev,
           UsbStatusAddr,
           0x003F
           );
}

EFI_STATUS
EnablePeriodicSchedule (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Enable periodic schedule

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;
  UINT32      UsbCommandAddr;
  UINT32      UsbCommandReg;

  UsbCommandAddr = USBCMD;

  Status = ReadEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             &UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  UsbCommandReg |= USBCMD_PSE;
  Status = WriteEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

exit:
  return Status;
}

EFI_STATUS
DisablePeriodicSchedule (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Disable periodic schedule

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;
  UINT32      UsbCommandAddr;
  UINT32      UsbCommandReg;

  UsbCommandAddr = USBCMD;

  Status = ReadEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             &UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  UsbCommandReg &= ~USBCMD_PSE;
  Status = WriteEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return Status;
}

EFI_STATUS
EnableAsynchronousSchedule (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Enable asynchrounous schedule

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;
  UINT32      UsbCommandAddr;
  UINT32      UsbCommandReg;

  UsbCommandAddr = USBCMD;

  Status = ReadEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             &UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  UsbCommandReg |= USBCMD_ASE;
  Status = WriteEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

exit:
  return Status;
}

EFI_STATUS
DisableAsynchronousSchedule (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Disable asynchrounous schedule

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;
  UINT32      UsbCommandAddr;
  UINT32      UsbCommandReg;

  UsbCommandAddr = USBCMD;

  Status = ReadEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             &UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  UsbCommandReg &= ~USBCMD_ASE;
  Status = WriteEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return Status;
}

EFI_STATUS
ResetEhc (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Reset Ehc

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;
  UINT32      UsbCommandAddr;
  UINT32      UsbCommandReg;

  UsbCommandAddr = USBCMD;

  Status = ReadEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             &UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  UsbCommandReg |= USBCMD_HCRESET;
  Status = WriteEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

exit:
  return Status;
}

EFI_STATUS
StartScheduleExecution (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Start Ehc schedule execution

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;
  UINT32      UsbCommandAddr;
  UINT32      UsbCommandReg;

  UsbCommandAddr = USBCMD;

  Status = ReadEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             &UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  UsbCommandReg |= USBCMD_RS;
  Status = WriteEhcOperationalReg (
             HcDev,
             UsbCommandAddr,
             UsbCommandReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

exit:
  return Status;
}

BOOLEAN
IsFrameListProgrammable (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Whether frame list is programmable

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   Programmable
  FALSE  Unprogrammable

--*/
{
  BOOLEAN Value;
  UINT32  HcCapParamsAddr;
  UINT32  HcCapParamsReg;

  HcCapParamsAddr = HCCPARAMS;

  ReadEhcCapabiltiyReg(
    HcDev,
    HcCapParamsAddr,
    &HcCapParamsReg
    );

  if (HcCapParamsReg & HCCP_PFLF) {
    Value = TRUE;
  } else {
    Value = FALSE;
  }

  return Value;
}

BOOLEAN
IsPeriodicScheduleEnabled (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Whether periodic schedule is enabled

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE    Enabled
  FALSE   Disabled

--*/
{
  BOOLEAN Value;
  UINT32  UsbStatusAddr;
  UINT32  UsbStatusReg;

  UsbStatusAddr = USBSTS;

  ReadEhcOperationalReg (
    HcDev,
    UsbStatusAddr,
    &UsbStatusReg
    );

  if (UsbStatusReg & USBSTS_PSS) {
    Value = TRUE;
  } else {
    Value = FALSE;
  }

  return Value;
}

BOOLEAN
IsAsyncScheduleEnabled (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Whether asynchronous schedule is enabled

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   Enabled
  FALSE  Disabled

--*/
{
  BOOLEAN Value;
  UINT32  UsbStatusAddr;
  UINT32  UsbStatusReg;

  UsbStatusAddr = USBSTS;

  ReadEhcOperationalReg (
    HcDev,
    UsbStatusAddr,
    &UsbStatusReg
    );

  if (UsbStatusReg & USBSTS_ASS) {
    Value = TRUE;
  } else {
    Value = FALSE;
  }

  return Value;
}

BOOLEAN
IsEhcPortEnabled (
  IN  USB2_HC_DEV     *HcDev,
  IN  UINT8           PortNum
  )
/*++

Routine Description:

  Whether port is enabled

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   Enabled
  FALSE  Disabled

--*/
{
  UINT32  PortStatusControlAddr;
  UINT32  PortStatusControlReg;

  PortStatusControlAddr = (UINT32) (PORTSC + (4 * PortNum));

  ReadEhcOperationalReg (
    HcDev,
    PortStatusControlAddr,
    &PortStatusControlReg
    );

  return ((BOOLEAN) ((PortStatusControlReg & PORTSC_PED) ? TRUE : FALSE));
}

BOOLEAN
IsEhcReseted (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Whether Ehc is reseted

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   Reseted
  FALSE  Unreseted

--*/
{
  BOOLEAN Value;
  UINT32  UsbCommandAddr;
  UINT32  UsbCommandReg;

  UsbCommandAddr = USBCMD;

  ReadEhcOperationalReg (
    HcDev,
    UsbCommandAddr,
    &UsbCommandReg
    );

  if (UsbCommandReg & USBCMD_HCRESET) {
    Value = FALSE;
  } else {
    Value = TRUE;
  }

  return Value;
}

BOOLEAN
IsEhcHalted (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Whether Ehc is halted

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   Halted
  FALSE  Not halted

--*/
{
  BOOLEAN Value;
  UINT32  UsbStatusAddr;
  UINT32  UsbStatusReg;

  UsbStatusAddr = USBSTS;

  ReadEhcOperationalReg (
    HcDev,
    UsbStatusAddr,
    &UsbStatusReg
    );

  if (UsbStatusReg & USBSTS_HCH) {
    Value = TRUE;
  } else {
    Value = FALSE;
  }

  return Value;
}

BOOLEAN
IsEhcSysError (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Whether Ehc is system error

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   System error
  FALSE  No system error

--*/
{
  BOOLEAN Value;
  UINT32  UsbStatusAddr;
  UINT32  UsbStatusReg;

  UsbStatusAddr = USBSTS;

  ReadEhcOperationalReg (
    HcDev,
    UsbStatusAddr,
    &UsbStatusReg
    );

  if (UsbStatusReg & USBSTS_HSE) {
    Value = TRUE;
  } else {
    Value = FALSE;
  }

  return Value;
}

BOOLEAN
IsHighSpeedDevice (
  IN EFI_USB2_HC_PROTOCOL *This,
  IN UINT8                PortNum
  )
/*++

Routine Description:

  Whether high speed device attached

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  TRUE   High speed
  FALSE  Full speed

--*/
{
  USB2_HC_DEV          *HcDev;
  UINT32               PortStatusControlAddr;
  UINT32               PortStatusControlReg;

  HcDev = USB2_HC_DEV_FROM_THIS (This);
  PortStatusControlAddr = (UINT32) (PORTSC + (4 * PortNum));

  //
  // Set port reset bit
  //
  ReadEhcOperationalReg (
    HcDev,
    PortStatusControlAddr,
    &PortStatusControlReg
    );
  //
  // Make sure Host Controller not halt before reset it
  //
  if (IsEhcHalted (HcDev)) {
    StartScheduleExecution (HcDev);
    WaitForEhcNotHalt (HcDev, EHCI_GENERIC_TIMEOUT);
  }
  PortStatusControlReg &= 0xffffffd5;
  PortStatusControlReg |= PORTSC_PR;
  //
  // Set one to PortReset bit must also set zero to PortEnable bit
  //
  PortStatusControlReg &= ~PORTSC_PED;
  WriteEhcOperationalReg (
    HcDev,
    PortStatusControlAddr,
    PortStatusControlReg
    );

  //
  // Set Port reset recovery time
  //
  gBS->Stall (EHCI_SET_PORT_RESET_RECOVERY_TIME);

  //
  // Clear port reset bit
  //
  ReadEhcOperationalReg (
    HcDev,
    PortStatusControlAddr,
    &PortStatusControlReg
    );
  PortStatusControlReg &= 0xffffffd5;
  PortStatusControlReg &= ~PORTSC_PR;
  WriteEhcOperationalReg (
    HcDev,
    PortStatusControlAddr,
    PortStatusControlReg
    );

  //
  // Clear port reset recovery time
  //
  gBS->Stall (EHCI_CLEAR_PORT_RESET_RECOVERY_TIME);

  return ((BOOLEAN) (IsEhcPortEnabled (HcDev, PortNum) ? TRUE : FALSE));
}

EFI_STATUS
WaitForEhcReset (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  wait for Ehc reset or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
{
  EFI_STATUS  Status;
  UINTN       Delay;

  //
  // Timeout is in US unit
  //
  Delay = (Timeout / 50) + 1;
  do {

    if (IsEhcReseted (HcDev)) {
      Status = EFI_SUCCESS;
      goto exit;
    }
    gBS->Stall (EHCI_GENERIC_RECOVERY_TIME);

  } while (Delay--);

  Status = EFI_TIMEOUT;

exit:
  return Status;
}

EFI_STATUS
WaitForEhcHalt (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  wait for Ehc halt or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
{
  EFI_STATUS  Status;
  UINTN       Delay;

  //
  // Timeout is in US unit
  //
  Delay = (Timeout / 50) + 1;
  do {

    if (IsEhcHalted (HcDev)) {
      Status = EFI_SUCCESS;
      goto exit;
    }
    gBS->Stall (EHCI_GENERIC_RECOVERY_TIME);

  } while (Delay--);

  Status = EFI_TIMEOUT;

exit:
  return Status;
}

EFI_STATUS
WaitForEhcNotHalt (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  wait for Ehc not halt or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
{
  EFI_STATUS  Status;
  UINTN       Delay;

  //
  // Timeout is in US unit
  //
  Delay = (Timeout / 50) + 1;
  do {

    if (!IsEhcHalted (HcDev)) {
      Status = EFI_SUCCESS;
      goto exit;
    }
    gBS->Stall (EHCI_GENERIC_RECOVERY_TIME);

  } while (Delay--);

  Status = EFI_TIMEOUT;

exit:
  return Status;
}

EFI_STATUS
WaitForAsyncScheduleEnable (
  IN  USB2_HC_DEV            *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  Wait for Ehc asynchronous schedule enable or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
{
  EFI_STATUS  Status;
  UINTN       Delay;

  //
  // Timeout is in US unit
  //
  Delay = (Timeout / 50) + 1;
  do {

    if (IsAsyncScheduleEnabled (HcDev)) {
      Status = EFI_SUCCESS;
      goto exit;
    }
    gBS->Stall (EHCI_GENERIC_RECOVERY_TIME);

  } while (Delay--);

  Status = EFI_TIMEOUT;

exit:
  return Status;
}

EFI_STATUS
WaitForAsyncScheduleDisable (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  Wait for Ehc asynchronous schedule disable or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
{
  EFI_STATUS  Status;
  UINTN       Delay;

  //
  // Timeout is in US unit
  //
  Delay = (Timeout / 50) + 1;
  do {

    if (!IsAsyncScheduleEnabled (HcDev)) {
      Status = EFI_SUCCESS;
      goto exit;
    }
    gBS->Stall (EHCI_GENERIC_RECOVERY_TIME);

  } while (Delay--);

  Status = EFI_TIMEOUT;

exit:
  return Status;
}

EFI_STATUS
WaitForPeriodicScheduleEnable (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  Wait for Ehc periodic schedule enable or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
{
  EFI_STATUS  Status;
  UINTN       Delay;

  //
  // Timeout is in US unit
  //
  Delay = (Timeout / 50) + 1;
  do {

    if (IsPeriodicScheduleEnabled (HcDev)) {
      Status = EFI_SUCCESS;
      goto exit;
    }
    gBS->Stall (EHCI_GENERIC_RECOVERY_TIME);

  } while (Delay--);

  Status = EFI_TIMEOUT;

exit:
  return Status;
}

EFI_STATUS
WaitForPeriodicScheduleDisable (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  Wait for periodic schedule disable or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
{
  EFI_STATUS  Status;
  UINTN       Delay;

  //
  // Timeout is in US unit
  //
  Delay = (Timeout / 50) + 1;
  do {

    if (!IsPeriodicScheduleEnabled (HcDev)) {
      Status = EFI_SUCCESS;
      goto exit;
    }
    gBS->Stall (EHCI_GENERIC_RECOVERY_TIME);

  } while (Delay--);

  Status = EFI_TIMEOUT;

exit:
  return Status;
}

EFI_STATUS
WaitForEhcDoorbell (
  IN USB2_HC_DEV             *HcDev,
  IN UINTN                   Timeout
  )
/*++

Routine Description:

  Wait for periodic schedule disable or timeout

Arguments:

  HcDev   - USB2_HC_DEV
  Timeout - timeout threshold

Returns:

  EFI_SUCCESS    Success
  EFI_TIMEOUT    Timeout

--*/
{
  EFI_STATUS  Status;
  UINT32      UsbCommandAddr;
  UINT32      UsbCommandReg;
  UINTN       Delay;

  UsbCommandAddr  = USBCMD;
  Delay           = (Timeout / 50) + 1;

  do {
    Status = ReadEhcOperationalReg (
               HcDev,
               UsbCommandAddr,
               &UsbCommandReg
               );
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }
    if (!(UsbCommandReg & USBCMD_IAAD)) {
      break;
    }
	
  } while (--Delay);

  if (0 == Delay) {
    Status = EFI_TIMEOUT;
  }

exit:
  return Status;
}
