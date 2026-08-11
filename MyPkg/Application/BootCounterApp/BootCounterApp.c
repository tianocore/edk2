#include <Uefi.h>

#include <Guid/BootTracker.h>

#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC
EFI_STATUS
ReadBootCount (
  OUT UINT32  *BootCount
  )
{
  UINTN  DataSize;

  *BootCount = 0;
  DataSize    = sizeof (*BootCount);

  return gRT->GetVariable (
                BOOT_COUNT_VARIABLE_NAME,
                &gMyBootTrackerGuid,
                NULL,
                &DataSize,
                BootCount
                );
}

STATIC
EFI_STATUS
ResetBootCount (
  VOID
  )
{
  UINT32  BootCount;

  BootCount = 0;

  return gRT->SetVariable (
                BOOT_COUNT_VARIABLE_NAME,
                &gMyBootTrackerGuid,
                EFI_VARIABLE_NON_VOLATILE |
                EFI_VARIABLE_BOOTSERVICE_ACCESS |
                EFI_VARIABLE_RUNTIME_ACCESS,
                sizeof (BootCount),
                &BootCount
                );
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS     Status;
  UINT32         BootCount;
  EFI_INPUT_KEY  Key;
  UINTN          EventIndex;

  (VOID)ImageHandle;

  while (TRUE) {
    SystemTable->ConOut->ClearScreen (SystemTable->ConOut);

    Print (L"====================================\r\n");
    Print (L"          UEFI Boot Tracker\r\n");
    Print (L"====================================\r\n");

    Status = ReadBootCount (&BootCount);

    if (Status == EFI_NOT_FOUND) {
      Print (L"BootCount variable does not exist.\r\n");
      Print (L"Total boot count: 0\r\n");
    } else if (EFI_ERROR (Status)) {
      Print (L"Read BootCount failed: %r\r\n", Status);
    } else {
      Print (L"Total boot count: %u\r\n", BootCount);
    }

    Print (L"\r\n");
    Print (L"[R] Reset boot count\r\n");
    Print (L"[E] Exit\r\n");
    Print (L"\r\nSelect: ");

    Status = SystemTable->BootServices->WaitForEvent (
                                          1,
                                          &SystemTable->ConIn->WaitForKey,
                                          &EventIndex
                                          );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = SystemTable->ConIn->ReadKeyStroke (
                                  SystemTable->ConIn,
                                  &Key
                                  );

    if (EFI_ERROR (Status)) {
      continue;
    }

    if ((Key.UnicodeChar == L'e') ||
        (Key.UnicodeChar == L'E'))
    {
      break;
    }

    if ((Key.UnicodeChar == L'r') ||
        (Key.UnicodeChar == L'R'))
    {
      Status = ResetBootCount ();

      if (EFI_ERROR (Status)) {
        Print (L"\r\nReset failed: %r\r\n", Status);
        SystemTable->BootServices->Stall (2000000);
      }
    }
  }

  return EFI_SUCCESS;
}