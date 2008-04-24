#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>


CHAR16            *gEmptyString = L" ";

/**
  Get the string based on the StringId and HII Package List Handle.

  @param  Token                  The String's ID.
  @param  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
GetToken (
  IN  EFI_STRING_ID                Token,
  IN  EFI_HII_HANDLE               HiiHandle
  )
{
  EFI_STATUS  Status;
  CHAR16      *String;
  UINTN       BufferLength;

  //
  // Set default string size assumption at no more than 256 bytes
  //
  BufferLength = 0x100;
  String = AllocateZeroPool (BufferLength);
  ASSERT (String != NULL);

  Status = HiiLibGetString (HiiHandle, Token, String, &BufferLength);

  if (Status == EFI_BUFFER_TOO_SMALL) {
    gBS->FreePool (String);
    String = AllocateZeroPool (BufferLength);
    ASSERT (String != NULL);

    Status = HiiLibGetString (HiiHandle, Token, String, &BufferLength);
  }
  ASSERT_EFI_ERROR (Status);

  return String;
}

/**
  Create a new string in HII Package List.

  @param  String                 The String to be added
  @param  HiiHandle              The package list in the HII database to insert the
                                 specified string.

  @return The output string.

**/
EFI_STRING_ID
NewString (
  IN  CHAR16                   *String,
  IN  EFI_HII_HANDLE           HiiHandle
  )
{
  EFI_STRING_ID  StringId;
  EFI_STATUS     Status;

  StringId = 0;
  Status = HiiLibNewString (HiiHandle, &StringId, String);
  ASSERT_EFI_ERROR (Status);

  return StringId;
}



