#include <Uefi.h>

#include <Guid/BootTracker.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC EFI_EVENT  mReadyToBootEvent;
STATIC BOOLEAN    mBootCountUpdated = FALSE;

STATIC
VOID
EFIAPI
BootCounterReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;
  UINT32      BootCount;
  UINTN       DataSize;

  (VOID)Event;
  (VOID)Context;

  if (mBootCountUpdated) {
    return;
  }

  mBootCountUpdated = TRUE;
  BootCount         = 0;
  DataSize          = sizeof (BootCount);

  Status = gRT->GetVariable (
                  BOOT_COUNT_VARIABLE_NAME,
                  &gMyBootTrackerGuid,
                  NULL,
                  &DataSize,
                  &BootCount
                  );

  if (Status == EFI_NOT_FOUND) {
    BootCount = 0;
  } else if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[BootTracker] GetVariable failed: %r\n",
      Status
      ));

    return;
  }

  BootCount++;

  Status = gRT->SetVariable (
                  BOOT_COUNT_VARIABLE_NAME,
                  &gMyBootTrackerGuid,
                  EFI_VARIABLE_NON_VOLATILE |
                  EFI_VARIABLE_BOOTSERVICE_ACCESS |
                  EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (BootCount),
                  &BootCount
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[BootTracker] SetVariable failed: %r\n",
      Status
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "[BootTracker] BootCount = %u\n",
      BootCount
      ));
  }
}

EFI_STATUS
EFIAPI
BootCounterDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  (VOID)ImageHandle;
  (VOID)SystemTable;

  DEBUG ((DEBUG_INFO, "[BootTracker] Driver loaded\n"));

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             BootCounterReadyToBoot,
             NULL,
             &mReadyToBootEvent
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[BootTracker] ReadyToBoot event failed: %r\n",
      Status
      ));
  }

  return Status;
}