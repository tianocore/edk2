/** @file
  Implement EFI RealTimeClock runtime services via RTC Lib.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#include "LsRealTimeClock.h"

STATIC BOOLEAN    mInitialized = FALSE;
STATIC EFI_EVENT  mRtcVirtualAddrChangeEvent;
STATIC UINTN      mRtcBase;

/*
  Enable Real-time clock.

  @param VOID

  @retval  VOID
 */
STATIC
VOID
InitRtc (
  VOID
  )
{
  UINTN              Val;
  EFI_HOB_GUID_TYPE  *GuidHob   = NULL;
  VOID               *DataInHob = NULL;

  if (!mInitialized) {
    /* Enable rtc */
    GuidHob = GetFirstGuidHob (&gRtcRegisterBaseAddressHobGuid);
    if (GuidHob) {
      DataInHob = GET_GUID_HOB_DATA (GuidHob);
      mRtcBase  = (UINT64)(*(UINTN *)DataInHob);
      Val       = MmioRead32 (mRtcBase + RTC_CTRL_REG);
      Val      |= TOY_ENABLE_BIT | OSC_ENABLE_BIT;
      MmioWrite32 (mRtcBase + RTC_CTRL_REG, Val);
      mInitialized = TRUE;
    } else {
      DebugPrint (DEBUG_INFO, "RTC register address not found!\n");
      ASSERT (FALSE);
    }
  }
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
  UINT32  Val;

  // Ensure Time is a valid pointer
  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Val        = MmioRead32 (mRtcBase + TOY_READ1_REG);
  Time->Year = Val + 1900;

  Val              = MmioRead32 (mRtcBase + TOY_READ0_REG);
  Time->Month      = (Val >> TOY_MON_SHIFT) & TOY_MON_MASK;
  Time->Day        = (Val >> TOY_DAY_SHIFT) & TOY_DAY_MASK;
  Time->Hour       = (Val >> TOY_HOUR_SHIFT) & TOY_HOUR_MASK;
  Time->Minute     = (Val >> TOY_MIN_SHIFT) & TOY_MIN_MASK;
  Time->Second     = (Val >> TOY_SEC_SHIFT) & TOY_SEC_MASK;
  Time->Nanosecond = 0;
  return EFI_SUCCESS;
}

/**
  Sets the current local time and date information.

  @param  Time                  A pointer to the current time.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER A time field is out of range.
  @retval EFI_DEVICE_ERROR      The time could not be set due due to hardware error.
**/
EFI_STATUS
EFIAPI
LibSetTime (
  IN  EFI_TIME  *Time
  )
{
  UINT32  Val;

  // Initialize the hardware if not already done

  Val  = 0;
  Val |= (Time->Second << TOY_SEC_SHIFT);
  Val |= (Time->Minute << TOY_MIN_SHIFT);
  Val |= (Time->Hour   << TOY_HOUR_SHIFT);
  Val |= (Time->Day    << TOY_DAY_SHIFT);
  Val |= (Time->Month  << TOY_MON_SHIFT);
  MmioWrite32 (mRtcBase + TOY_WRITE0_REG, Val);

  Val = Time->Year - 1900;
  MmioWrite32 (mRtcBase + TOY_WRITE1_REG, Val);
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
STATIC
VOID
EFIAPI
VirtualNotifyEvent (
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
  EfiConvertPointer (0x0, (VOID **)&mRtcBase);
  return;
}

/** Add the RTC controller address range to the memory map.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  RtcPageBase  Base address of the RTC controller.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           Flash device not found.
**/
STATIC
EFI_STATUS
MapRtcResources (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_PHYSICAL_ADDRESS  RtcPageBase
  )
{
  EFI_STATUS  Status;

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  RtcPageBase,
                  EFI_PAGE_SIZE,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to add memory space. Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = gDS->AllocateMemorySpace (
                  EfiGcdAllocateAddress,
                  EfiGcdMemoryTypeMemoryMappedIo,
                  0,
                  EFI_PAGE_SIZE,
                  &RtcPageBase,
                  ImageHandle,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to allocate memory space. Status = %r\n",
      Status
      ));
    gDS->RemoveMemorySpace (
           RtcPageBase,
           EFI_PAGE_SIZE
           );
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  RtcPageBase,
                  EFI_PAGE_SIZE,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to set memory attributes. Status = %r\n",
      Status
      ));

    gDS->FreeMemorySpace (
           RtcPageBase,
           EFI_PAGE_SIZE
           );

    gDS->RemoveMemorySpace (
           RtcPageBase,
           EFI_PAGE_SIZE
           );
  }

  return Status;
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

  InitRtc ();
  Status = MapRtcResources (ImageHandle, (mRtcBase & ~EFI_PAGE_MASK));
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to map memory for loongson 7A RTC. Status = %r\n",
      Status
      ));
    return Status;
  }

  //
  // Register for the virtual address change event
  //
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
