#include <Uefi.h>
#include  <Library/UefiLib.h>

/***
  Print a welcoming message.

  Establishes the main structure of the application.

  @retval  0         The application exited normally.
  @retval  Other     An error occurred.
***/
#define  count 10
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{

  for (UINTN i = 0; i < count; i++)
  {
    Print(L"Hello there fellow Programmer.\n");
    Print(L"Welcome to the world of EDK II.\n");
  }
  


  return 0;
}