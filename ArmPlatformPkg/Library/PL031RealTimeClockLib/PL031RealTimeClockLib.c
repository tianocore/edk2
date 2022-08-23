/** @file
  Implement EFI RealTimeClock runtime services via RTC Lib.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2020, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2019, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Guid/EventGroup.h>
#include <Guid/GlobalVariable.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/RealTimeClockLib.h>
#include <Library/TimeBaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#include <Protocol/RealTimeClock.h>

#include "PL031RealTimeClock.h"

STATIC BOOLEAN    mPL031Initialized = FALSE;
STATIC EFI_EVENT  mRtcVirtualAddrChangeEvent;
STATIC UINTN      mPL031RtcBase;

EFI_STATUS
IdentifyPL031 (
  VOID
  )
{
  EFI_STATUS  Status;

  // Check if this is a PrimeCell Peripheral
  if (  (MmioRead8 (mPL031RtcBase + PL031_RTC_PCELL_ID0) != 0x0D)
     || (MmioRead8 (mPL031RtcBase + PL031_RTC_PCELL_ID1) != 0xF0)
     || (MmioRead8 (mPL031RtcBase + PL031_RTC_PCELL_ID2) != 0x05)
     || (MmioRead8 (mPL031RtcBase + PL031_RTC_PCELL_ID3) != 0xB1))
  {
    Status = EFI_NOT_FOUND;
    goto EXIT;
  }

  // Check if this PrimeCell Peripheral is the PL031 Real Time Clock
  if (  (MmioRead8 (mPL031RtcBase + PL031_RTC_PERIPH_ID0) != 0x31)
     || (MmioRead8 (mPL031RtcBase + PL031_RTC_PERIPH_ID1) != 0x10)
     || ((MmioRead8 (mPL031RtcBase + PL031_RTC_PERIPH_ID2) & 0xF) != 0x04)
     || (MmioRead8 (mPL031RtcBase + PL031_RTC_PERIPH_ID3) != 0x00))
  {
    Status = EFI_NOT_FOUND;
    goto EXIT;
  }

  Status = EFI_SUCCESS;

EXIT:
  return Status;
}

EFI_STATUS
InitializePL031 (
  VOID
  )
{
  EFI_STATUS  Status;

  // Prepare the hardware
  Status = IdentifyPL031 ();
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  // Ensure interrupts are masked. We do not want RTC interrupts in UEFI
  if ((MmioRead32 (mPL031RtcBase + PL031_RTC_IMSC_IRQ_MASK_SET_CLEAR_REGISTER) & PL031_SET_IRQ_MASK) != 0) {
    MmioWrite32 (mPL031RtcBase + PL031_RTC_IMSC_IRQ_MASK_SET_CLEAR_REGISTER, 0);
  }

  // Clear any existing interrupts
  if ((MmioRead32 (mPL031RtcBase + PL031_RTC_RIS_RAW_IRQ_STATUS_REGISTER) & PL031_IRQ_TRIGGERED) == PL031_IRQ_TRIGGERED) {
    MmioOr32 (mPL031RtcBase + PL031_RTC_ICR_IRQ_CLEAR_REGISTER, PL031_CLEAR_IRQ);
  }

  // Start the clock counter
  if ((MmioRead32 (mPL031RtcBase + PL031_RTC_CR_CONTROL_REGISTER) & PL031_RTC_ENABLED) != PL031_RTC_ENABLED) {
    MmioOr32 (mPL031RtcBase + PL031_RTC_CR_CONTROL_REGISTER, PL031_RTC_ENABLED);
  }

  mPL031Initialized = TRUE;

EXIT:
  return Status;
}

/**
  Returns the current time and date information, and the time-keeping capabilities
  of the hardware platform.

  @param  Time                   A pointer to storage to receive a snapshot of the current time.
  @param  Capabilities           An optional pointer to a buffer to receive the real time clock
                                 device's capabilities.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Time is NULL.
  @retval EFI_DEVICE_ERROR       The time could not be retrieved due to hardware error.
  @retval EFI_SECURITY_VIOLATION The time could not be retrieved due to an authentication failure.

**/
EFI_STATUS
EFIAPI
LibGetTime (
  OUT EFI_TIME               *Time,
  OUT EFI_TIME_CAPABILITIES  *Capabilities
  )
{
  EFI_STATUS  Status;
  UINT32      EpochSeconds;

  // Ensure Time is a valid pointer
  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Initialize the hardware if not already done
  if (!mPL031Initialized) {
    Status = InitializePL031 ();
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  EpochSeconds = MmioRead32 (mPL031RtcBase + PL031_RTC_DR_DATA_REGISTER);

  // Adjust for the correct time zone
  // The timezone setting also reflects the DST setting of the clock
  if (Time->TimeZone != EFI_UNSPECIFIED_TIMEZONE) {
    EpochSeconds += Time->TimeZone * SEC_PER_MIN;
  } else if ((Time->Daylight & EFI_TIME_IN_DAYLIGHT) == EFI_TIME_IN_DAYLIGHT) {
    // Convert to adjusted time, i.e. spring forwards one hour
    EpochSeconds += SEC_PER_HOUR;
  }

  // Convert from internal 32-bit time to UEFI time
  EpochToEfiTime (EpochSeconds, Time);

  // Update the Capabilities info
  if (Capabilities != NULL) {
    // PL031 runs at frequency 1Hz
    Capabilities->Resolution = PL031_COUNTS_PER_SECOND;
    // Accuracy in ppm multiplied by 1,000,000, e.g. for 50ppm set 50,000,000
    Capabilities->Accuracy = (UINT32)PcdGet32 (PcdPL031RtcPpmAccuracy);
    // FALSE: Setting the time does not clear the values below the resolution level
    Capabilities->SetsToZero = FALSE;
  }

  return EFI_SUCCESS;
}

/**
  Sets the current local time and date information.

  @param  Time                  A pointer to the current time.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER A time field is out of range.
  @retval EFI_DEVICE_ERROR      The time could not be set due to hardware error.

**/
EFI_STATUS
EFIAPI
LibSetTime (
  IN  EFI_TIME  *Time
  )
{
  EFI_STATUS  Status;
  UINT32      EpochSeconds;

  // Because the PL031 is a 32-bit counter counting seconds,
  // the maximum time span is just over 136 years.
  // Time is stored in Unix Epoch format, so it starts in 1970,
  // Therefore it can not exceed the year 2106.
  if ((Time->Year < 1970) || (Time->Year >= 2106)) {
    return EFI_UNSUPPORTED;
  }

  // Initialize the hardware if not already done
  if (!mPL031Initialized) {
    Status = InitializePL031 ();
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  EpochSeconds = (UINT32)EfiTimeToEpoch (Time);

  // Adjust for the correct time zone, i.e. convert to UTC time zone
  // The timezone setting also reflects the DST setting of the clock
  if (Time->TimeZone != EFI_UNSPECIFIED_TIMEZONE) {
    EpochSeconds -= Time->TimeZone * SEC_PER_MIN;
  } else if ((Time->Daylight & EFI_TIME_IN_DAYLIGHT) == EFI_TIME_IN_DAYLIGHT) {
    // Convert to un-adjusted time, i.e. fall back one hour
    EpochSeconds -= SEC_PER_HOUR;
  }

  // Set the PL031
  MmioWrite32 (mPL031RtcBase + PL031_RTC_LR_LOAD_REGISTER, EpochSeconds);

  return EFI_SUCCESS;
}

/**
  Returns the current wakeup alarm clock setting.

  @param  Enabled               Indicates if the alarm is currently enabled or disabled.
  @param  Pending               Indicates if the alarm signal is pending and requires acknowledgement.
  @param  Time                  The current alarm setting.

  @retval EFI_SUCCESS           The alarm settings were returned.
  @retval EFI_INVALID_PARAMETER Any parameter is NULL.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be retrieved due to a hardware error.

**/
EFI_STATUS
EFIAPI
LibGetWakeupTime (
  OUT BOOLEAN   *Enabled,
  OUT BOOLEAN   *Pending,
  OUT EFI_TIME  *Time
  )
{
  // Not a required feature
  return EFI_UNSUPPORTED;
}

/**
  Sets the system wakeup alarm clock time.

  @param  Enabled               Enable or disable the wakeup alarm.
  @param  Time                  If Enable is TRUE, the time to set the wakeup alarm for.

  @retval EFI_SUCCESS           If Enable is TRUE, then the wakeup alarm was enabled. If
                                Enable is FALSE, then the wakeup alarm was disabled.
  @retval EFI_INVALID_PARAMETER A time field is out of range.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be set due to a hardware error.
  @retval EFI_UNSUPPORTED       A wakeup timer is not supported on this platform.

**/
EFI_STATUS
EFIAPI
LibSetWakeupTime (
  IN BOOLEAN    Enabled,
  OUT EFI_TIME  *Time
  )
{
  // Not a required feature
  return EFI_UNSUPPORTED;
}

/**
  Fixup internal data so that EFI can be call in virtual mode.
  Call the passed in Child Notify event and convert any pointers in
  lib to virtual mode.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
VOID
EFIAPI
LibRtcVirtualNotifyEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // Only needed if you are going to support the OS calling RTC functions in virtual mode.
  // You will need to call EfiConvertPointer (). To convert any stored physical addresses
  // to virtual address. After the OS transitions to calling in virtual mode, all future
  // runtime calls will be made in virtual mode.
  //
  EfiConvertPointer (0x0, (VOID **)&mPL031RtcBase);
  return;
}

/**
  This is the declaration of an EFI image entry point. This can be the entry point to an application
  written to this specification, an EFI boot service driver, or an EFI runtime driver.

  @param  ImageHandle           Handle that identifies the loaded image.
  @param  SystemTable           System Table for this image.

  @retval EFI_SUCCESS           The operation completed successfully.

**/
EFI_STATUS
EFIAPI
LibRtcInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  // Initialize RTC Base Address
  mPL031RtcBase = PcdGet32 (PcdPL031RtcBase);

  // Declare the controller as EFI_MEMORY_RUNTIME
  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  mPL031RtcBase,
                  SIZE_4KB,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (mPL031RtcBase, SIZE_4KB, EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Install the protocol
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiRealTimeClockArchProtocolGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  LibRtcVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mRtcVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
