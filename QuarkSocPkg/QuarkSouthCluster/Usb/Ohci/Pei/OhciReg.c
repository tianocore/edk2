/** @file
The OHCI register operation routines.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "OhcPeim.h"

/**

  Get OHCI operational reg value

  @param  Ohc                   UHC private data
  @param  Offset                Offset of the operational reg

  @retval                       Value of the register

**/
UINT32
OhciGetOperationalReg (
  IN USB_OHCI_HC_DEV         *Ohc,
  IN UINT32                  Offset
  )
{

  return MmioRead32 (Ohc->UsbHostControllerBaseAddress + Offset);

}
/**

  Set OHCI operational reg value

  @param  Ohc                   UHC private data
  @param  Offset                 Offset of the operational reg
  @param  Value                  Value to set

  @retval EFI_SUCCESS            Value set to the reg

**/


EFI_STATUS
OhciSetOperationalReg (
  USB_OHCI_HC_DEV         *Ohc,
  IN UINT32               Offset,
  IN UINT32               *Value
  )
{
  MmioWrite32(Ohc->UsbHostControllerBaseAddress + Offset, *Value);
  return EFI_SUCCESS;
}
/**

  Get HcRevision reg value

  @param  Ohc                   UHC private data

  @retval                       Value of the register

**/


UINT32
OhciGetHcRevision (
  USB_OHCI_HC_DEV         *Ohc
  )
{
  return OhciGetOperationalReg (Ohc, HC_REVISION);
}
/**

  Set HcReset reg value


  @param  Ohc                   UHC private data
  @param  Field                 Field to set
  @param  Value                 Value to set

  @retval EFI_SUCCESS           Value set

**/

EFI_STATUS
OhciSetHcReset (
  IN USB_OHCI_HC_DEV            *Ohc,
  IN UINT32                     Field,
  IN UINT32                     Value
  )
{
  HcRESET                       Reset;

  *(UINT32 *) &Reset = OhciGetOperationalReg (Ohc, USBHOST_OFFSET_UHCHR);

  if (Field & RESET_SYSTEM_BUS) {
    Reset.FSBIR = Value;
  }

  if (Field & RESET_HOST_CONTROLLER) {
    Reset.FHR = Value;
  }

  if (Field & RESET_CLOCK_GENERATION) {
    Reset.CGR = Value;
  }

  if (Field & RESET_SSE_GLOBAL) {
    Reset.SSE = Value;
  }

  if (Field & RESET_PSPL) {
    Reset.PSPL = Value;
  }

  if (Field & RESET_PCPL) {
    Reset.PCPL = Value;
  }

  if (Field & RESET_SSEP1) {
    Reset.SSEP1 = Value;
  }

  if (Field & RESET_SSEP2) {
    Reset.SSEP2 = Value;
  }

  if (Field & RESET_SSEP3) {
    Reset.SSEP3 = Value;
  }

  OhciSetOperationalReg (Ohc, USBHOST_OFFSET_UHCHR, (UINT32*)&Reset);

  return EFI_SUCCESS;
}

/**

  Get specific field of HcReset reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetHcReset (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Field
  )
{
  HcRESET                 Reset;
  UINT32                  Value;


  *(UINT32 *) &Reset = OhciGetOperationalReg (Ohc, USBHOST_OFFSET_UHCHR);
  Value = 0;

  switch (Field) {
  case RESET_SYSTEM_BUS:
    Value = Reset.FSBIR;
    break;

  case RESET_HOST_CONTROLLER:
    Value = Reset.FHR;
    break;

  case RESET_CLOCK_GENERATION:
    Value = Reset.CGR;
    break;

  case RESET_SSE_GLOBAL:
    Value = Reset.SSE;
    break;

  case RESET_PSPL:
    Value = Reset.PSPL;
    break;

  case RESET_PCPL:
    Value = Reset.PCPL;
    break;

  case RESET_SSEP1:
    Value = Reset.SSEP1;
    break;

  case RESET_SSEP2:
    Value = Reset.SSEP2;
    break;

  case RESET_SSEP3:
    Value = Reset.SSEP3;
    break;

  default:
    ASSERT (FALSE);
  }


  return Value;
}

/**

  Set HcControl reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to set
  @param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetHcControl (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field,
  IN UINT32               Value
  )
{
  EFI_STATUS              Status;
  HcCONTROL               Control;



  *(UINT32 *) &Control = OhciGetOperationalReg (Ohc, HC_CONTROL);

  if (Field & CONTROL_BULK_RATIO) {
    Control.ControlBulkRatio = Value;
  }

  if (Field & HC_FUNCTIONAL_STATE) {
    Control.FunctionalState = Value;
  }

  if (Field & PERIODIC_ENABLE) {
    Control.PeriodicEnable = Value;
  }

  if (Field & CONTROL_ENABLE) {
    Control.ControlEnable = Value;
  }

  if (Field & ISOCHRONOUS_ENABLE) {
    Control.IsochronousEnable = Value;
  }

  if (Field & BULK_ENABLE) {
    Control.BulkEnable = Value;
  }

  if (Field & INTERRUPT_ROUTING) {
    Control.InterruptRouting = Value;
  }

  Status = OhciSetOperationalReg (Ohc, HC_CONTROL, (UINT32*)&Control);

  return Status;
}


/**

  Get specific field of HcControl reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/


UINT32
OhciGetHcControl (
  IN USB_OHCI_HC_DEV   *Ohc,
  IN UINTN             Field
  )
{
  HcCONTROL     Control;

  *(UINT32 *) &Control = OhciGetOperationalReg (Ohc, HC_CONTROL);

  switch (Field) {
  case CONTROL_BULK_RATIO:
    return Control.ControlBulkRatio;
    break;
  case PERIODIC_ENABLE:
    return Control.PeriodicEnable;
    break;
  case CONTROL_ENABLE:
    return Control.ControlEnable;
    break;
  case BULK_ENABLE:
    return Control.BulkEnable;
    break;
  case ISOCHRONOUS_ENABLE:
    return Control.IsochronousEnable;
    break;
  case HC_FUNCTIONAL_STATE:
    return Control.FunctionalState;
    break;
  case INTERRUPT_ROUTING:
    return Control.InterruptRouting;
    break;
  default:
    ASSERT (FALSE);
  }

  return 0;
}

/**

  Set HcCommand reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to set
  @param  Value                 Value to set

  @retval EFI_SUCCESS           Value set

**/

EFI_STATUS
OhciSetHcCommandStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field,
  IN UINT32               Value
  )
{
  EFI_STATUS              Status;
  HcCOMMAND_STATUS        CommandStatus;

  ZeroMem (&CommandStatus, sizeof (HcCOMMAND_STATUS));

  if(Field & HC_RESET){
    CommandStatus.HcReset = Value;
  }

  if(Field & CONTROL_LIST_FILLED){
    CommandStatus.ControlListFilled = Value;
  }

  if(Field & BULK_LIST_FILLED){
    CommandStatus.BulkListFilled = Value;
  }

  if(Field & CHANGE_OWNER_REQUEST){
    CommandStatus.ChangeOwnerRequest = Value;
  }

  if(Field & SCHEDULE_OVERRUN_COUNT){
    CommandStatus.ScheduleOverrunCount = Value;
  }

  Status = OhciSetOperationalReg (Ohc, HC_COMMAND_STATUS, (UINT32*)&CommandStatus);

  return Status;
}

/**

  Get specific field of HcCommand reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetHcCommandStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  )
{
  HcCOMMAND_STATUS        CommandStatus;

  *(UINT32 *) &CommandStatus = OhciGetOperationalReg (Ohc, HC_COMMAND_STATUS);

  switch (Field){
  case HC_RESET:
    return CommandStatus.HcReset;
    break;
  case CONTROL_LIST_FILLED:
    return CommandStatus.ControlListFilled;
    break;
  case BULK_LIST_FILLED:
    return CommandStatus.BulkListFilled;
    break;
  case CHANGE_OWNER_REQUEST:
    return CommandStatus.ChangeOwnerRequest;
    break;
  case SCHEDULE_OVERRUN_COUNT:
    return CommandStatus.ScheduleOverrunCount;
    break;
  default:
    ASSERT (FALSE);
  }

  return 0;
}

/**

  Clear specific fields of Interrupt Status

  @param  Ohc                   UHC private data
  @param  Field                 Field to clear

  @retval EFI_SUCCESS           Fields cleared

**/

EFI_STATUS
OhciClearInterruptStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  )
{
  EFI_STATUS              Status;
  HcINTERRUPT_STATUS      InterruptStatus;

  ZeroMem (&InterruptStatus, sizeof (HcINTERRUPT_STATUS));

  if(Field & SCHEDULE_OVERRUN){
    InterruptStatus.SchedulingOverrun = 1;
  }

  if(Field & WRITEBACK_DONE_HEAD){
    InterruptStatus.WriteBackDone = 1;
  }
  if(Field & START_OF_FRAME){
    InterruptStatus.Sof = 1;
  }

  if(Field & RESUME_DETECT){
    InterruptStatus.ResumeDetected = 1;
  }

  if(Field & UNRECOVERABLE_ERROR){
    InterruptStatus.UnrecoverableError = 1;
  }

  if(Field & FRAME_NUMBER_OVERFLOW){
    InterruptStatus.FrameNumOverflow = 1;
  }

  if(Field & ROOTHUB_STATUS_CHANGE){
    InterruptStatus.RHStatusChange = 1;
  }

  if(Field & OWNERSHIP_CHANGE){
    InterruptStatus.OwnerChange = 1;
  }

  Status = OhciSetOperationalReg (Ohc, HC_INTERRUPT_STATUS, (UINT32*)&InterruptStatus);

  return Status;
}

/**

  Get fields of HcInterrupt reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetHcInterruptStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  )
{
  HcINTERRUPT_STATUS      InterruptStatus;

  *(UINT32 *) &InterruptStatus = OhciGetOperationalReg (Ohc, HC_INTERRUPT_STATUS);

  switch (Field){
  case SCHEDULE_OVERRUN:
    return InterruptStatus.SchedulingOverrun;
    break;

  case  WRITEBACK_DONE_HEAD:
    return InterruptStatus.WriteBackDone;
    break;

  case START_OF_FRAME:
    return InterruptStatus.Sof;
    break;

  case RESUME_DETECT:
    return InterruptStatus.ResumeDetected;
    break;

  case UNRECOVERABLE_ERROR:
    return InterruptStatus.UnrecoverableError;
    break;

  case FRAME_NUMBER_OVERFLOW:
    return InterruptStatus.FrameNumOverflow;
    break;

  case ROOTHUB_STATUS_CHANGE:
    return InterruptStatus.RHStatusChange;
    break;

  case OWNERSHIP_CHANGE:
    return InterruptStatus.OwnerChange;
    break;

  default:
    ASSERT (FALSE);
  }

  return 0;
}

/**

  Set Interrupt Control reg value

  @param  Ohc                   UHC private data
  @param  StatEnable            Enable or Disable
  @param  Field                 Field to set
  @param  Value                 Value to set

  @retval EFI_SUCCESS           Value set

**/

EFI_STATUS
OhciSetInterruptControl (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN BOOLEAN              StatEnable,
  IN UINTN                Field,
  IN UINT32               Value
  )
{
  EFI_STATUS              Status;
  HcINTERRUPT_CONTROL     InterruptState;


  ZeroMem (&InterruptState, sizeof (HcINTERRUPT_CONTROL));

  if(Field & SCHEDULE_OVERRUN) {
    InterruptState.SchedulingOverrunInt = Value;
  }

  if(Field & WRITEBACK_DONE_HEAD) {
    InterruptState.WriteBackDoneInt = Value;
  }
  if(Field & START_OF_FRAME) {
    InterruptState.SofInt = Value;
  }

  if(Field & RESUME_DETECT) {
    InterruptState.ResumeDetectedInt = Value;
  }

  if(Field & UNRECOVERABLE_ERROR) {
    InterruptState.UnrecoverableErrorInt = Value;
  }

  if(Field & FRAME_NUMBER_OVERFLOW) {
    InterruptState.FrameNumOverflowInt = Value;
  }

  if(Field & ROOTHUB_STATUS_CHANGE) {
    InterruptState.RHStatusChangeInt = Value;
  }

  if(Field & OWNERSHIP_CHANGE) {
    InterruptState.OwnerChangedInt = Value;
  }

  if(Field & MASTER_INTERRUPT) {
    InterruptState.MasterInterruptEnable = Value;
  }

  if (StatEnable) {
    Status = OhciSetOperationalReg (Ohc, HC_INTERRUPT_ENABLE, (UINT32*)&InterruptState);
  } else {
    Status = OhciSetOperationalReg (Ohc, HC_INTERRUPT_DISABLE, (UINT32*)&InterruptState);
  }

  return Status;
}

/**

  Get field of HcInterruptControl reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetHcInterruptControl (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  )
{
  HcINTERRUPT_CONTROL     InterruptState;

  *(UINT32 *) &InterruptState = OhciGetOperationalReg (Ohc, HC_INTERRUPT_ENABLE);

  switch (Field){
    case SCHEDULE_OVERRUN:
      return InterruptState.SchedulingOverrunInt;
      break;

    case WRITEBACK_DONE_HEAD:
      return InterruptState.WriteBackDoneInt;
      break;

    case START_OF_FRAME:
      return InterruptState.SofInt;
      break;

    case RESUME_DETECT:
      return InterruptState.ResumeDetectedInt;
      break;

    case UNRECOVERABLE_ERROR:
      return InterruptState.UnrecoverableErrorInt;
      break;

    case FRAME_NUMBER_OVERFLOW:
      return InterruptState.FrameNumOverflowInt;
      break;

    case ROOTHUB_STATUS_CHANGE:
      return InterruptState.RHStatusChangeInt;
      break;

    case OWNERSHIP_CHANGE:
      return InterruptState.OwnerChangedInt;
      break;

    case MASTER_INTERRUPT:
      return InterruptState.MasterInterruptEnable;
      break;

    default:
      ASSERT (FALSE);
  }

  return 0;
}

/**

  Set memory pointer of specific type

  @param  Ohc                   UHC private data
  @param  PointerType           Type of the pointer to set
  @param  Value                 Value to set

  @retval EFI_SUCCESS           Memory pointer set

**/

EFI_STATUS
OhciSetMemoryPointer(
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                PointerType,
  IN VOID                 *Value
  )
{
  EFI_STATUS              Status;
  UINT32                  Verify;

  Status = OhciSetOperationalReg (Ohc, PointerType, (UINT32*)&Value);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Verify = OhciGetOperationalReg (Ohc, PointerType);

  while (Verify != (UINT32) Value) {
    MicroSecondDelay (HC_1_MILLISECOND);
    Verify = OhciGetOperationalReg (Ohc, PointerType);
  };


  return Status;
}

/**

  Get memory pointer of specific type

  @param  Ohc                   UHC private data
  @param  PointerType           Type of pointer

  @retval                       Memory pointer of the specific type

**/

VOID *
OhciGetMemoryPointer (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                PointerType
  )
{

  return (VOID *) OhciGetOperationalReg (Ohc, PointerType);
}


/**

  Set Frame Interval value

  @param  Ohc                   UHC private data
  @param  Field                 Field to set
  @param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetFrameInterval (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field,
  IN UINT32               Value
  )
{
  EFI_STATUS              Status;
  HcFRM_INTERVAL          FrameInterval;


  *(UINT32 *) &FrameInterval = OhciGetOperationalReg(Ohc, HC_FRM_INTERVAL);

  if (Field & FRAME_INTERVAL) {
    FrameInterval.FrmIntervalToggle = !FrameInterval.FrmIntervalToggle;
    FrameInterval.FrameInterval = Value;
  }

  if (Field & FS_LARGEST_DATA_PACKET) {
    FrameInterval.FSMaxDataPacket = Value;
  }

  if (Field & FRMINT_TOGGLE) {
    FrameInterval.FrmIntervalToggle = Value;
  }

  Status = OhciSetOperationalReg (
             Ohc,
             HC_FRM_INTERVAL,
             (UINT32*)&FrameInterval
             );

  return Status;
}


/**

  Get field of frame interval reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetFrameInterval (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  )
{
  HcFRM_INTERVAL          FrameInterval;

  *(UINT32 *) &FrameInterval = OhciGetOperationalReg (Ohc, HC_FRM_INTERVAL);

  switch (Field){
    case FRAME_INTERVAL:
      return FrameInterval.FrameInterval;
      break;

    case FS_LARGEST_DATA_PACKET:
      return FrameInterval.FSMaxDataPacket;
      break;

    case FRMINT_TOGGLE:
      return FrameInterval.FrmIntervalToggle;
      break;

    default:
      ASSERT (FALSE);
  }

  return 0;
}

/**

  Set Frame Remaining reg value

  @param  Ohc                   UHC private data
  @param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetFrameRemaining (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Value
  )
{
  EFI_STATUS              Status;
  HcFRAME_REMAINING       FrameRemaining;


  *(UINT32 *) &FrameRemaining = OhciGetOperationalReg (Ohc, HC_FRM_REMAINING);

  FrameRemaining.FrameRemaining = Value;
  FrameRemaining.FrameRemainingToggle = !FrameRemaining.FrameRemainingToggle;

  Status = OhciSetOperationalReg (Ohc, HC_FRM_REMAINING, (UINT32*)&FrameRemaining);

  return Status;
}
/**

  Get value of frame remaining reg

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of frame remaining reg

**/
UINT32
OhciGetFrameRemaining (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  )

{
  HcFRAME_REMAINING       FrameRemaining;


  *(UINT32 *) &FrameRemaining = OhciGetOperationalReg (Ohc, HC_FRM_REMAINING);

  switch (Field){
    case FRAME_REMAINING:
      return FrameRemaining.FrameRemaining;
      break;

    case FRAME_REMAIN_TOGGLE:
      return FrameRemaining.FrameRemainingToggle;
      break;

    default:
      ASSERT (FALSE);
  }

  return 0;
}

/**

  Set frame number reg value

  @param  Ohc                   UHC private data
  @param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetFrameNumber(
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Value
  )
{
  EFI_STATUS              Status;

  Status = OhciSetOperationalReg (Ohc, HC_FRM_NUMBER, &Value);

  return Status;
}

/**

  Get frame number reg value

  @param  Ohc                   UHC private data

  @retval                       Value of frame number reg

**/

UINT32
OhciGetFrameNumber (
  IN USB_OHCI_HC_DEV      *Ohc
  )
{
  return OhciGetOperationalReg(Ohc, HC_FRM_NUMBER);
}

/**

  Set period start reg value

  @param  Ohc                   UHC private data
  @param  Value                 Value to set

  @retval EFI_SUCCESS           Value set

**/

EFI_STATUS
OhciSetPeriodicStart (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Value
  )
{
  EFI_STATUS              Status;


  Status = OhciSetOperationalReg (Ohc, HC_PERIODIC_START, &Value);

  return Status;
}


/**

  Get periodic start reg value

  @param  Ohc                   UHC private data

  @param                        Value of periodic start reg

**/

UINT32
OhciGetPeriodicStart (
  IN USB_OHCI_HC_DEV      *Ohc
  )
{
  return OhciGetOperationalReg(Ohc, HC_PERIODIC_START);
}


/**

  Set Ls Threshold reg value

  @param  Ohc                   UHC private data
  @param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetLsThreshold (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Value
  )
{
  EFI_STATUS              Status;


  Status = OhciSetOperationalReg (Ohc, HC_LS_THREASHOLD, &Value);

  return Status;
}


/**

  Get Ls Threshold reg value

  @param  Ohc                   UHC private data

  @retval                       Value of Ls Threshold reg

**/

UINT32
OhciGetLsThreshold (
  IN USB_OHCI_HC_DEV      *Ohc
  )
{
  return OhciGetOperationalReg(Ohc, HC_LS_THREASHOLD);
}

/**

  Set Root Hub Descriptor reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to set
  @param  Value                 Value to set

  @retval  EFI_SUCCESS          Value set

**/
EFI_STATUS
OhciSetRootHubDescriptor (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field,
  IN UINT32               Value
  )
{
  EFI_STATUS              Status;
  HcRH_DESC_A             DescriptorA;
  HcRH_DESC_B             DescriptorB;


  if (Field & (RH_DEV_REMOVABLE || RH_PORT_PWR_CTRL_MASK)) {
    *(UINT32 *) &DescriptorB = OhciGetOperationalReg (Ohc, HC_RH_DESC_B);

    if(Field & RH_DEV_REMOVABLE) {
      DescriptorB.DeviceRemovable = Value;
    }
    if(Field & RH_PORT_PWR_CTRL_MASK) {
      DescriptorB.PortPowerControlMask = Value;
    }

    Status = OhciSetOperationalReg (Ohc, HC_RH_DESC_B, (UINT32*)&DescriptorB);

    return Status;
  }

  *(UINT32 *)&DescriptorA = OhciGetOperationalReg (Ohc, HC_RH_DESC_A);

  if(Field & RH_NUM_DS_PORTS) {
    DescriptorA.NumDownStrmPorts = Value;
  }
  if(Field & RH_NO_PSWITCH) {
    DescriptorA.NoPowerSwitch = Value;
  }
  if(Field & RH_PSWITCH_MODE) {
    DescriptorA.PowerSwitchMode = Value;
  }
  if(Field & RH_DEVICE_TYPE) {
    DescriptorA.DeviceType = Value;
  }
  if(Field & RH_OC_PROT_MODE) {
    DescriptorA.OverCurrentProtMode = Value;
  }
  if(Field & RH_NOC_PROT) {
    DescriptorA.NoOverCurrentProtMode = Value;
  }
  if(Field & RH_NO_POTPGT) {
    DescriptorA.PowerOnToPowerGoodTime = Value;
  }

  Status = OhciSetOperationalReg (Ohc, HC_RH_DESC_A, (UINT32*)&DescriptorA);

  return Status;
}


/**

  Get Root Hub Descriptor reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetRootHubDescriptor (
  IN USB_OHCI_HC_DEV     *Ohc,
  IN UINTN               Field
  )
{
  HcRH_DESC_A             DescriptorA;
  HcRH_DESC_B             DescriptorB;


  *(UINT32 *) &DescriptorA = OhciGetOperationalReg (Ohc, HC_RH_DESC_A);
  *(UINT32 *) &DescriptorB = OhciGetOperationalReg (Ohc, HC_RH_DESC_B);

  switch (Field){
    case RH_DEV_REMOVABLE:
      return DescriptorB.DeviceRemovable;
      break;

    case RH_PORT_PWR_CTRL_MASK:
      return DescriptorB.PortPowerControlMask;
      break;

    case RH_NUM_DS_PORTS:
      return DescriptorA.NumDownStrmPorts;
      break;

    case RH_NO_PSWITCH:
      return DescriptorA.NoPowerSwitch;
      break;

    case RH_PSWITCH_MODE:
      return DescriptorA.PowerSwitchMode;
      break;

    case RH_DEVICE_TYPE:
      return DescriptorA.DeviceType;
      break;

    case RH_OC_PROT_MODE:
      return DescriptorA.OverCurrentProtMode;
      break;

    case RH_NOC_PROT:
      return DescriptorA.NoOverCurrentProtMode;
      break;

    case RH_NO_POTPGT:
      return DescriptorA.PowerOnToPowerGoodTime;
      break;

    default:
      ASSERT (FALSE);
  }

  return 0;
}


/**

  Set Root Hub Status reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetRootHubStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  )
{
  EFI_STATUS              Status;
  HcRH_STATUS             RootHubStatus;


  ZeroMem (&RootHubStatus, sizeof(HcRH_STATUS));

  if(Field & RH_LOCAL_PSTAT){
    RootHubStatus.LocalPowerStat = 1;
  }
  if(Field & RH_OC_ID){
    RootHubStatus.OverCurrentIndicator = 1;
  }
  if(Field & RH_REMOTE_WK_ENABLE){
    RootHubStatus.DevRemoteWakeupEnable = 1;
  }
  if(Field & RH_LOCAL_PSTAT_CHANGE){
    RootHubStatus.LocalPowerStatChange = 1;
  }
  if(Field & RH_OC_ID_CHANGE){
    RootHubStatus.OverCurrentIndicatorChange = 1;
  }
  if(Field & RH_CLR_RMT_WK_ENABLE){
    RootHubStatus.ClearRemoteWakeupEnable = 1;
  }

  Status = OhciSetOperationalReg (Ohc, HC_RH_STATUS, (UINT32*)&RootHubStatus);

  return Status;
}


/**

  Get Root Hub Status reg value

  @param  Ohc                   UHC private data
  @param  Field                 Field to get

  @retval                       Value of the field

**/

UINT32
OhciGetRootHubStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINTN                Field
  )
{
  HcRH_STATUS             RootHubStatus;


  *(UINT32 *) &RootHubStatus = OhciGetOperationalReg (Ohc, HC_RH_STATUS);

  switch (Field) {
    case RH_LOCAL_PSTAT:
      return RootHubStatus.LocalPowerStat;
      break;
    case RH_OC_ID:
      return RootHubStatus.OverCurrentIndicator;
      break;
    case RH_REMOTE_WK_ENABLE:
      return RootHubStatus.DevRemoteWakeupEnable;
      break;
    case RH_LOCAL_PSTAT_CHANGE:
      return RootHubStatus.LocalPowerStatChange;
      break;
    case RH_OC_ID_CHANGE:
      return RootHubStatus.OverCurrentIndicatorChange;
      break;
    case RH_CLR_RMT_WK_ENABLE:
      return RootHubStatus.ClearRemoteWakeupEnable;
      break;
    default:
      ASSERT (FALSE);
  }

  return 0;
}


/**

  Set Root Hub Port Status reg value

  @param  Ohc                   UHC private data
  @param  Index                 Index of the port
  @param  Field                 Field to set

  @retval  EFI_SUCCESS          Value set

**/

EFI_STATUS
OhciSetRootHubPortStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Index,
  IN UINTN                Field
  )
{
  EFI_STATUS              Status;
  HcRHPORT_STATUS         PortStatus;


  ZeroMem (&PortStatus, sizeof(HcRHPORT_STATUS));

  if (Field & RH_CLEAR_PORT_ENABLE) {
    PortStatus.CurrentConnectStat = 1;
  }
  if (Field & RH_SET_PORT_ENABLE) {
    PortStatus.EnableStat = 1;
  }
  if (Field & RH_SET_PORT_SUSPEND) {
    PortStatus.SuspendStat = 1;
  }
  if (Field & RH_CLEAR_SUSPEND_STATUS) {
    PortStatus.OCIndicator = 1;
  }
  if (Field & RH_SET_PORT_RESET) {
    PortStatus.ResetStat = 1;
  }
  if (Field & RH_SET_PORT_POWER) {
    PortStatus.PowerStat = 1;
  }
  if (Field & RH_CLEAR_PORT_POWER) {
    PortStatus.LsDeviceAttached = 1;
  }
  if (Field & RH_CONNECT_STATUS_CHANGE) {
    PortStatus.ConnectStatChange = 1;
  }
  if (Field & RH_PORT_ENABLE_STAT_CHANGE) {
    PortStatus.EnableStatChange = 1;
  }
  if (Field & RH_PORT_SUSPEND_STAT_CHANGE) {
    PortStatus.SuspendStatChange = 1;
  }
  if (Field & RH_OC_INDICATOR_CHANGE) {
    PortStatus.OCIndicatorChange = 1;
  }
  if (Field & RH_PORT_RESET_STAT_CHANGE ) {
    PortStatus.ResetStatChange = 1;
  }

  Status = OhciSetOperationalReg (Ohc, HC_RH_PORT_STATUS + (Index * 4), (UINT32*)&PortStatus);

  return Status;
}


/**

  Get Root Hub Port Status reg value

  @param  Ohc                   UHC private data
  @param  Index                 Index of the port
  @param  Field                 Field to get

  @retval                       Value of the field and index

**/

UINT32
OhciReadRootHubPortStatus (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN UINT32               Index,
  IN UINTN                Field
  )
{
  HcRHPORT_STATUS         PortStatus;

  *(UINT32 *) &PortStatus = OhciGetOperationalReg (
                              Ohc,
                              HC_RH_PORT_STATUS + (Index * 4)
                              );

  switch (Field){
  case RH_CURR_CONNECT_STAT:
    return PortStatus.CurrentConnectStat;
    break;
  case RH_PORT_ENABLE_STAT:
    return PortStatus.EnableStat;
    break;
  case RH_PORT_SUSPEND_STAT:
    return PortStatus.SuspendStat;
    break;
  case RH_PORT_OC_INDICATOR:
    return PortStatus.OCIndicator;
    break;
  case RH_PORT_RESET_STAT:
    return PortStatus.ResetStat;
    break;
  case RH_PORT_POWER_STAT:
    return PortStatus.PowerStat;
    break;
  case RH_LSDEVICE_ATTACHED:
    return PortStatus.LsDeviceAttached;
    break;
  case RH_CONNECT_STATUS_CHANGE:
    return PortStatus.ConnectStatChange;
    break;
  case RH_PORT_ENABLE_STAT_CHANGE:
    return PortStatus.EnableStatChange;
    break;
  case RH_PORT_SUSPEND_STAT_CHANGE:
    return PortStatus.SuspendStatChange;
    break;
  case RH_OC_INDICATOR_CHANGE:
    return PortStatus.OCIndicatorChange;
    break;
  case RH_PORT_RESET_STAT_CHANGE:
    return PortStatus.ResetStatChange;
    break;
  default:
    ASSERT (FALSE);
  }

  return 0;
}
