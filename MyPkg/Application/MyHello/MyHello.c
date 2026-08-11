#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_INPUT_KEY  Key;
  EFI_STATUS     Status;

  (VOID)ImageHandle;

  Print (L"\r\n");
  Print (L"Ahihiiii");
  Print (L"====================================\r\n");
  Print (L" Hello from Thanh Lam's UEFI App!\r\n");
  Print (L"====================================\r\n");
  Print (L"Press any key to exit...\r\n");

  do {
    Status = SystemTable->ConIn->ReadKeyStroke (
                                   SystemTable->ConIn,
                                   &Key
                                   );
  } while (Status == EFI_NOT_READY);

  return EFI_SUCCESS;
}