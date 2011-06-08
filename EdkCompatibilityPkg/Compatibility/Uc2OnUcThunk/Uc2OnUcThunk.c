/** @file
Module produce UC2 on top of UC.

UEFI 2.1 specification supersedes Inte's EFI Specification 1.10.
EFI_UNICODE_COLLATION_PROTOCOL defined in Inte's EFI Specification 1.10 is replaced by
EFI_UNICODE_COLLATION_PROTOCOL in UEFI 2.1.
This module produces UC2 on top of UC. This module is used on platform when both of
these two conditions are true:
1) EFI 1.10 module producing UC present
2) And the rest of modules on the platform consume UC2

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:

**/

#include <PiDxe.h>
#include <Protocol/UnicodeCollation.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/LanguageLib.h>

/**
  Performs a case-insensitive comparison of two Null-terminated strings.

  @param  This Protocol instance pointer.
  @param  Str1 A pointer to a Null-terminated string.
  @param  Str2 A pointer to a Null-terminated string.

  @retval 0   Str1 is equivalent to Str2
  @retval > 0 Str1 is lexically greater than Str2
  @retval < 0 Str1 is lexically less than Str2

**/
INTN
EFIAPI
StriColl (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *Str1,
  IN CHAR16                           *Str2
  );

/**
  Converts all the characters in a Null-terminated string to 
  lower case characters.

  @param  This   Protocol instance pointer.
  @param  Str    A pointer to a Null-terminated string.

**/
VOID
EFIAPI
StrLwr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN OUT CHAR16                       *Str
  );

/**
  Converts all the characters in a Null-terminated string to upper
  case characters.

  @param  This   Protocol instance pointer.
  @param  Str    A pointer to a Null-terminated string.

**/
VOID
EFIAPI
StrUpr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN OUT CHAR16                       *Str
  );

/**
  Performs a case-insensitive comparison of a Null-terminated
  pattern string and a Null-terminated string.

  @param  This    Protocol instance pointer.
  @param  String  A pointer to a Null-terminated string.
  @param  Pattern A pointer to a Null-terminated pattern string.

  @retval TRUE    Pattern was found in String.
  @retval FALSE   Pattern was not found in String.

**/
BOOLEAN
EFIAPI
MetaiMatch (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *String,
  IN CHAR16                           *Pattern
  );

/**
  Converts an 8.3 FAT file name in an OEM character set to a Null-terminated string.

  @param  This    Protocol instance pointer.
  @param  FatSize The size of the string Fat in bytes.
  @param  Fat     A pointer to a Null-terminated string that contains an 8.3 file
                  name using an OEM character set.
  @param  String  A pointer to a Null-terminated string. The string must
                  be preallocated to hold FatSize characters.

**/
VOID
EFIAPI
FatToStr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN UINTN                            FatSize,
  IN CHAR8                            *Fat,
  OUT CHAR16                          *String
  );

/**
  Converts a Null-terminated string to legal characters in a FAT 
  filename using an OEM character set. 

  @param  This    Protocol instance pointer.
  @param  String  A pointer to a Null-terminated string. The string must
                  be preallocated to hold FatSize characters.
  @param  FatSize The size of the string Fat in bytes.
  @param  Fat     A pointer to a Null-terminated string that contains an 8.3 file
                  name using an OEM character set.

  @retval TRUE    Fat is a Long File Name
  @retval FALSE   Fat is an 8.3 file name

**/
BOOLEAN
EFIAPI
StrToFat (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *String,
  IN UINTN                            FatSize,
  OUT CHAR8                           *Fat
  );

#define UC2_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('_', 'U', 'C', '2')

typedef struct {
  UINT32                          Signature;
  EFI_UNICODE_COLLATION_PROTOCOL  Uc2;
  EFI_UNICODE_COLLATION_PROTOCOL *Uc;
} UC2_PRIVATE_DATA;

#define UC2_PRIVATE_DATA_FROM_THIS(a) CR (a, UC2_PRIVATE_DATA, Uc2, UC2_PRIVATE_DATA_SIGNATURE)

//
// Firmware Volume Protocol template
//
EFI_EVENT  mUc2Registration;

UC2_PRIVATE_DATA gUC2PrivateDataTemplate = {
  UC2_PRIVATE_DATA_SIGNATURE,
  {
    StriColl,
    MetaiMatch,
    StrLwr,
    StrUpr,
    FatToStr,
    StrToFat,
    NULL
  },
  NULL
};

//
// Module globals
//
/**
  This notification function is invoked when an instance of the
  EFI_UNICODE_COLLATION_PROTOCOL is produced. It installs another instance of the
  EFI_UNICODE_COLLATION_PROTOCOL2 on the same handle.

  @param  Event                 The event that occured
  @param  Context               Context of event. Not used in this nofication function.

**/
VOID
EFIAPI
UcNotificationEvent (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS                     Status;
  UINTN                          BufferSize;
  EFI_HANDLE                     Handle;
  UC2_PRIVATE_DATA               *Private;
  EFI_UNICODE_COLLATION_PROTOCOL *Uc2;

  while (TRUE) {
    BufferSize = sizeof (Handle);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    &gEfiUnicodeCollationProtocolGuid,
                    mUc2Registration,
                    &BufferSize,
                    &Handle
                    );
    if (EFI_ERROR (Status)) {
      //
      // Exit Path of While Loop....
      //
      break;
    }

    //
    // Skip this handle if the Firmware Volume Protocol is already installed
    //
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiUnicodeCollation2ProtocolGuid,
                    (VOID **)&Uc2
                    );
    if (!EFI_ERROR (Status)) {
      continue;
    }

    //
    // Allocate private data structure
    //
    Private = AllocateCopyPool (sizeof (UC2_PRIVATE_DATA), &gUC2PrivateDataTemplate);
    if (Private == NULL) {
      continue;
    }

    //
    // Retrieve the UC Protocol
    //
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiUnicodeCollationProtocolGuid,
                    (VOID **)&Private->Uc
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Fill in rest of private data structure
    //
    Private->Uc2.SupportedLanguages = ConvertLanguagesIso639ToRfc4646 (Private->Uc->SupportedLanguages);
    if (Private->Uc2.SupportedLanguages != NULL) {

      //
      // Install UC2 Protocol onto same handle
      //
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Handle,
                      &gEfiUnicodeCollation2ProtocolGuid,
                      &Private->Uc2,
                      NULL
                      );
      ASSERT_EFI_ERROR (Status);
    }
  }
}


/**
  The user Entry Point for DXE driver. The user code starts with this function
  as the real entry point for the image goes into a library that calls this 
  function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeUC2 (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EfiCreateProtocolNotifyEvent (
    &gEfiUnicodeCollationProtocolGuid,
    TPL_CALLBACK,
    UcNotificationEvent,
    NULL,
    &mUc2Registration
    );
  return EFI_SUCCESS;
}


/**
  Performs a case-insensitive comparison of two Null-terminated strings.

  @param  This Protocol instance pointer.
  @param  Str1 A pointer to a Null-terminated string.
  @param  Str2 A pointer to a Null-terminated string.

  @retval 0   Str1 is equivalent to Str2
  @retval > 0 Str1 is lexically greater than Str2
  @retval < 0 Str1 is lexically less than Str2

**/
INTN
EFIAPI
StriColl (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *Str1,
  IN CHAR16                           *Str2
  )
{
  UC2_PRIVATE_DATA *Private;
  
  Private = UC2_PRIVATE_DATA_FROM_THIS (This);
  
  return Private->Uc->StriColl (Private->Uc, Str1, Str2);
}


/**
  Converts all the characters in a Null-terminated string to 
  lower case characters.

  @param  This   Protocol instance pointer.
  @param  Str    A pointer to a Null-terminated string.

**/
VOID
EFIAPI
StrLwr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN OUT CHAR16                       *Str
  )
{
  UC2_PRIVATE_DATA *Private;
  
  Private = UC2_PRIVATE_DATA_FROM_THIS (This);
  
  Private->Uc->StrLwr (Private->Uc, Str);
}


/**
  Converts all the characters in a Null-terminated string to upper
  case characters.

  @param  This   Protocol instance pointer.
  @param  Str    A pointer to a Null-terminated string.

**/
VOID
EFIAPI
StrUpr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN OUT CHAR16                       *Str
  )
{
  UC2_PRIVATE_DATA *Private;
  
  Private = UC2_PRIVATE_DATA_FROM_THIS (This);
  
  Private->Uc->StrUpr (Private->Uc, Str);
}

/**
  Performs a case-insensitive comparison of a Null-terminated
  pattern string and a Null-terminated string.

  @param  This    Protocol instance pointer.
  @param  String  A pointer to a Null-terminated string.
  @param  Pattern A pointer to a Null-terminated pattern string.

  @retval TRUE    Pattern was found in String.
  @retval FALSE   Pattern was not found in String.

**/
BOOLEAN
EFIAPI
MetaiMatch (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *String,
  IN CHAR16                           *Pattern
  )
{
  UC2_PRIVATE_DATA *Private;
  
  Private = UC2_PRIVATE_DATA_FROM_THIS (This);
  
  return Private->Uc->MetaiMatch (Private->Uc, String, Pattern);
}


/**
  Converts an 8.3 FAT file name in an OEM character set to a Null-terminated string.

  @param  This    Protocol instance pointer.
  @param  FatSize The size of the string Fat in bytes.
  @param  Fat     A pointer to a Null-terminated string that contains an 8.3 file
                  name using an 8-bit OEM character set.
  @param  String  A pointer to a Null-terminated string. The string must
                  be preallocated to hold FatSize characters.

**/
VOID
EFIAPI
FatToStr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN UINTN                            FatSize,
  IN CHAR8                            *Fat,
  OUT CHAR16                          *String
  )
{
  UC2_PRIVATE_DATA *Private;
  
  Private = UC2_PRIVATE_DATA_FROM_THIS (This);
  
  Private->Uc->FatToStr (Private->Uc, FatSize, Fat, String);
}


/**
  Converts a Null-terminated string to legal characters in a FAT 
  filename using an OEM character set. 

  @param  This    Protocol instance pointer.
  @param  String  A pointer to a Null-terminated string. The string must
                  be preallocated to hold FatSize characters.
  @param  FatSize The size of the string Fat in bytes.
  @param  Fat     A pointer to a Null-terminated string that contains an 8.3 file
                  name using an OEM character set.

  @retval TRUE    Fat is a Long File Name
  @retval FALSE   Fat is an 8.3 file name

**/
BOOLEAN
EFIAPI
StrToFat (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *String,
  IN UINTN                            FatSize,
  OUT CHAR8                           *Fat
  )
{
  UC2_PRIVATE_DATA *Private;
  
  Private = UC2_PRIVATE_DATA_FROM_THIS (This);
  
  return Private->Uc->StrToFat (Private->Uc, String, FatSize, Fat);
}

