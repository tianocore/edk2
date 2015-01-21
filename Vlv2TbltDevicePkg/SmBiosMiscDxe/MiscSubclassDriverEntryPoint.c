/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscSubclassDriverEntryPoint.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"
#include <Protocol/HiiString.h>

EFI_HII_HANDLE  mHiiHandle;
EFI_HII_STRING_PROTOCOL  *mHiiString;


EFI_STRING
EFIAPI
SmbiosMiscGetString (
  IN EFI_STRING_ID   StringId
  ){

  EFI_STATUS  Status;
  UINTN       StringSize;
  CHAR16      TempString;
  EFI_STRING  String;
  String             = NULL;

  //
  // Retrieve the size of the string in the string package for the BestLanguage
  //
  StringSize = 0;
  Status = mHiiString->GetString (
                         mHiiString,
                         "en-US",
                         mHiiHandle,
                         StringId,
                         &TempString,
                         &StringSize,
                         NULL
                         );
  //
  // If GetString() returns EFI_SUCCESS for a zero size,
  // then there are no supported languages registered for HiiHandle.  If GetString()
  // returns an error other than EFI_BUFFER_TOO_SMALL, then HiiHandle is not present
  // in the HII Database
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    goto Error;
  }

  //
  // Allocate a buffer for the return string
  //
  String = AllocateZeroPool (StringSize);
  if (String == NULL) {
    goto Error;
  }

  //
  // Retrieve the string from the string package
  //
  Status = mHiiString->GetString (
                         mHiiString,
                         "en-US",
                         mHiiHandle,
                         StringId,
                         String,
                         &StringSize,
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    //
    // Free the buffer and return NULL if the supported languages can not be retrieved.
    //
    FreePool (String);
    String = NULL;
  }
Error:

  return String;
}

/**
  Standard EFI driver point.  This driver parses the mMiscSubclassDataTable
  structure and reports any generated data to the DataHub.

  @param ImageHandle   - Handle for the image of this driver
  @param SystemTable   - Pointer to the EFI System Table

  @retval EFI_SUCCESS      - The data was successfully reported to the Data Hub.
  @retval EFI_DEVICE_ERROR - Can not locate any protocols

**/
EFI_STATUS
EFIAPI
MiscSubclassDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  UINTN                Index;
  EFI_STATUS           EfiStatus;
  EFI_SMBIOS_PROTOCOL  *Smbios;

  //
  // Retrieve the pointer to the UEFI HII String Protocol
  //
  EfiStatus = gBS->LocateProtocol (
                     &gEfiHiiStringProtocolGuid,
                     NULL,
                     (VOID **) &mHiiString
                     );
  ASSERT_EFI_ERROR (EfiStatus);

  EfiStatus = gBS->LocateProtocol(
                     &gEfiSmbiosProtocolGuid,
                     NULL,
                     (VOID**)&Smbios
                     );

  if (EFI_ERROR(EfiStatus)) {
    DEBUG((EFI_D_ERROR, "Could not locate SMBIOS protocol.  %r\n", EfiStatus));
    return EfiStatus;
  }

  mHiiHandle = HiiAddPackages (
                 &gEfiCallerIdGuid,
                 NULL,
                 MiscSubclassStrings,
                 NULL
                 );
  ASSERT (mHiiHandle != NULL);

  for (Index = 0; Index < mMiscSubclassDataTableEntries; ++Index) {
    //
    // If the entry have a function pointer, just log the data.
    //
    if (mMiscSubclassDataTable[Index].Function != NULL) {
      EfiStatus = (*mMiscSubclassDataTable[Index].Function)(
        mMiscSubclassDataTable[Index].RecordData,
        Smbios
        );

      if (EFI_ERROR(EfiStatus)) {
        DEBUG((EFI_D_ERROR, "Misc smbios store error.  Index=%d, ReturnStatus=%r\n", Index, EfiStatus));
        return EfiStatus;
      }
    }
  }

  return EfiStatus;
}

