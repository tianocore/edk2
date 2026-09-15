/** @file
  Implement Goldfish RealTimeClock runtime services via RTC Lib.

  Copyright (C) 2007 Google, Inc.
  Copyright (C) 2017 Imagination Technologies Ltd.
  Copyright (C) 2026 ZTE Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/RealTimeClockLib.h>
#include <Library/TimeBaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#include <Guid/EventGroup.h>
#include <Guid/FdtHob.h>

#include "GoldfishRealTimeClock.h"

STATIC EFI_EVENT  mRtcVirtualAddrChangeEvent;
STATIC UINTN      mRtcRegisterBase;

/**
  Returns the current time and date information, and the time-keeping capabilities
  of the hardware platform.

  @param  Time                  A pointer to storage to receive a snapshot of the current time.
  @param  Capabilities          An optional pointer to a buffer to receive the real time clock
                                device's capabilities.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER Time is NULL.
  @retval EFI_DEVICE_ERROR      The time could not be retrieved due to hardware error.

**/
EFI_STATUS
EFIAPI
LibGetTime (
  OUT EFI_TIME                *Time,
  OUT  EFI_TIME_CAPABILITIES  *Capabilities
  )
{
  UINTN   Base;
  UINT32  Low;
  UINT32  High;
  UINT64  EpochSeconds;

  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Base = mRtcRegisterBase;

  Low  = MmioRead32 (Base + GOLDFISH_RTC_TIME_LOW);
  High = MmioRead32 (Base + GOLDFISH_RTC_TIME_HIGH);

  EpochSeconds = ((UINT64)High << 32) | Low;
  EpochSeconds = EpochSeconds / NANO_SEC;

  // Adjust for the correct time zone
  // The timezone setting also reflects the DST setting of the clock
  if (Time->TimeZone != EFI_UNSPECIFIED_TIMEZONE) {
    EpochSeconds += Time->TimeZone * SEC_PER_MIN;
  } else if ((Time->Daylight & EFI_TIME_IN_DAYLIGHT) == EFI_TIME_IN_DAYLIGHT) {
    // Convert to adjusted time, i.e. spring forwards one hour
    EpochSeconds += SEC_PER_HOUR;
  }

  // Convert from internal time to UEFI time
  EpochToEfiTime (EpochSeconds, Time);

  // Upadte the Capability info
  if (Capabilities != NULL) {
    // runs at frequency 1Hz
    Capabilities->Resolution = GOLDFISH_COUNTS_PER_SECOND;
    // Accuracy in ppm multiplied by 1,000,000,
    Capabilities->Accuracy = GOLDFISH_ACCURACY;
    // FALSE: Setting the time does not clear the vlaues below the resolution level
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
  IN EFI_TIME  *Time
  )
{
  UINTN   Base;
  UINT64  EpochSeconds;

  Base = mRtcRegisterBase;

  //
  // Use Time, to set the time in your RTC hardware
  //
  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EpochSeconds = (UINT64)EfiTimeToEpoch (Time);
  EpochSeconds = EpochSeconds * NANO_SEC;

  // Adjust for the correct time zone, i.e. convert to UTC time zone
  // The timezone setting also reflects the DST setting of the clock
  if (Time->TimeZone != EFI_UNSPECIFIED_TIMEZONE) {
    EpochSeconds -= Time->TimeZone * SEC_PER_MIN;
  } else if ((Time->Daylight & EFI_TIME_IN_DAYLIGHT) == EFI_TIME_IN_DAYLIGHT) {
    // Convert to un-adjusted time, i.e. fall back one hour
    EpochSeconds -= SEC_PER_HOUR;
  }

  MmioWrite32 (Base + GOLDFISH_RTC_TIME_LOW, (UINT32)(EpochSeconds & 0xFFFFFFFF));
  MmioWrite32 (Base + GOLDFISH_RTC_TIME_HIGH, (UINT32)(EpochSeconds >> 32));

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
  Converts the base address of the RTC registers for runtime calls.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
STATIC
VOID
EFIAPI
VirtualNotifyEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  // This call will convert the physical address (maintained in the library)
  // to a virtual address that can be used during runtime.
  EfiConvertPointer (0x0, (VOID **)&mRtcRegisterBase);
  return;
}

EFI_STATUS
GetGoldfishRtcBase (
  OUT  UINTN  *BaseAddress
  )
{
  VOID        *Hob;
  VOID        *Base;
  INT32       Node;
  INT32       SubNode;
  INT32       Len;
  CONST VOID  *Data;

  Hob = GetFirstGuidHob (&gFdtHobGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64))) {
    return EFI_NOT_FOUND;
  }

  Base = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);
  if (FdtCheckHeader (Base) != 0) {
    return EFI_NOT_FOUND;
  }

  Node = FdtPathOffset (Base, "/soc");
  if (Node < 0 ) {
    return EFI_NOT_FOUND;
  }

  SubNode = FdtNodeOffsetByCompatible (Base, Node, "google,goldfish-rtc");
  if (SubNode < 0 ) {
    return EFI_NOT_FOUND;
  }

  Data = FdtGetProp (Base, SubNode, "reg", &Len);
  if (Data == NULL) {
    return EFI_LOAD_ERROR;
  }

  *BaseAddress = (UINTN)Fdt64ToCpu (*(UINT64 *)Data);
  return EFI_SUCCESS;
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

  // initial RTC Base Address
  Status = GetGoldfishRtcBase (&mRtcRegisterBase);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Use Default Rtc Reg Base!\n"));
    mRtcRegisterBase = (UINTN)PcdGet32 (PcdGoldfishRtcRegBase);
  }

  // Declare the controller as EFI_MEMORY_RUNTIME
  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  mRtcRegisterBase,
                  SIZE_4KB,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME | EFI_MEMORY_XP
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  mRtcRegisterBase,
                  SIZE_4KB,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME | EFI_MEMORY_XP
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // register for the virtual address change event
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mRtcVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
