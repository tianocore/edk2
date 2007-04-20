/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Package.c

Abstract:

  This file contains the package processing code to the HII database.

--*/


#include "HiiDatabase.h"

EFI_STATUS
GetPackSize (
  IN  VOID                *Pack,
  OUT UINTN               *PackSize,
  OUT UINT32              *NumberOfTokens
  )
/*++

Routine Description:
  Determines the passed in Pack's size and returns the value.
  
Arguments:

Returns: 

--*/
{
  EFI_HII_STRING_PACK *StringPack;
  UINT16              Type;
  UINT32              Length;

  *PackSize = 0;

  Type      = EFI_HII_IFR;
  if (!CompareMem (&((EFI_HII_PACK_HEADER *) Pack)->Type, &Type, sizeof (UINT16))) {
    //
    // The header contains the full IFR length
    //
    CopyMem (&Length, &((EFI_HII_PACK_HEADER *) Pack)->Length, sizeof (Length));
    *PackSize = (UINTN) Length;
    return EFI_SUCCESS;
  }

  Type = EFI_HII_STRING;
  if (!CompareMem (&((EFI_HII_PACK_HEADER *) Pack)->Type, &Type, sizeof (UINT16))) {
    //
    // The header contains the STRING package length
    // The assumption is that the strings for all languages
    // are a contiguous block of data and there is a series of
    // these package instances which will terminate with a NULL package
    // instance.
    //
    StringPack = (EFI_HII_STRING_PACK *) Pack;

    //
    // There may be multiple instances packed together of strings
    // so we must walk the self describing structures until we encounter
    // the NULL structure to determine the full size.
    //
    CopyMem (&Length, &StringPack->Header.Length, sizeof (Length));
    if (NumberOfTokens != NULL) {
      CopyMem (NumberOfTokens, &StringPack->NumStringPointers, sizeof (UINT32));
    }

    while (Length != 0) {
      *PackSize   = *PackSize + Length;
      StringPack  = (EFI_HII_STRING_PACK *) ((CHAR8 *) StringPack + Length);
      CopyMem (&Length, &StringPack->Header.Length, sizeof (Length));
    }
    //
    // Encountered a length of 0, so let's add the space for the NULL terminator
    // pack's length and call it done.
    //
    *PackSize = *PackSize + sizeof (EFI_HII_STRING_PACK);
    return EFI_SUCCESS;
  }
  //
  // We only determine the size of the non-global Package types.
  // If neither IFR or STRING data were found, return an error
  //
  return EFI_NOT_FOUND;
}

EFI_STATUS
ValidatePack (
  IN   EFI_HII_PROTOCOL          *This,
  IN   EFI_HII_PACKAGE_INSTANCE  *PackageInstance,
  OUT  EFI_HII_PACKAGE_INSTANCE  **StringPackageInstance,
  OUT  UINT32                    *TotalStringCount
  )
/*++

Routine Description:
  Verifies that the package instance is using the correct handle for string operations.
  
Arguments:

Returns: 

--*/
{
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_PACKAGE_INSTANCE  *HandlePackageInstance;
  UINT8                     *RawData;
  EFI_GUID                  Guid;
  EFI_HII_IFR_PACK          *FormPack;
  UINTN                     Index;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData         = EFI_HII_DATA_FROM_THIS (This);

  HandleDatabase  = HiiData->DatabaseHead;
  ZeroMem (&Guid, sizeof (EFI_GUID));

  *StringPackageInstance = PackageInstance;

  //
  // Based on if there is IFR data in this package instance, determine
  // what the location is of the beginning of the string data.
  //
  if (PackageInstance->IfrSize > 0) {
    FormPack = (EFI_HII_IFR_PACK *) ((CHAR8 *) (&PackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER));
  } else {
    //
    // If there is no IFR data assume the caller knows what they are doing.
    //
    return EFI_SUCCESS;
  }

  RawData = (UINT8 *) FormPack;

  for (Index = 0; RawData[Index] != EFI_IFR_END_FORM_SET_OP;) {
    if (RawData[Index] == EFI_IFR_FORM_SET_OP) {
      //
      // Cache the guid for this formset
      //
      CopyMem (&Guid, &((EFI_IFR_FORM_SET *) &RawData[Index])->Guid, sizeof (EFI_GUID));
      break;
    }

    Index = RawData[Index + 1] + Index;
  }
  //
  // If there is no string package, and the PackageInstance->IfrPack.Guid and PackageInstance->Guid are
  // different, we should return the correct handle for the caller to use for strings.
  //
  if ((PackageInstance->StringSize == 0) && (!CompareGuid (&Guid, &PackageInstance->Guid))) {
    //
    // Search the database for a handle that matches the PackageInstance->Guid
    //
    for (; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
      //
      // Get Ifrdata and extract the Guid for it
      //
      HandlePackageInstance = HandleDatabase->Buffer;

      ASSERT (HandlePackageInstance->IfrSize != 0);

      FormPack  = (EFI_HII_IFR_PACK *) ((CHAR8 *) (&HandlePackageInstance->IfrData) + sizeof (EFI_HII_PACK_HEADER));
      RawData   = (UINT8 *) FormPack;

      for (Index = 0; RawData[Index] != EFI_IFR_END_FORM_SET_OP;) {
        if (RawData[Index] == EFI_IFR_FORM_SET_OP) {
          //
          // Cache the guid for this formset
          //
          CopyMem (&Guid, &((EFI_IFR_FORM_SET *) &RawData[Index])->Guid, sizeof (EFI_GUID));
          break;
        }

        Index = RawData[Index + 1] + Index;
      }
      //
      // If the Guid from the new handle matches the original Guid referenced in the original package data
      // return the appropriate package instance data to use.
      //
      if (CompareGuid (&Guid, &PackageInstance->Guid)) {
        if (TotalStringCount != NULL) {
          *TotalStringCount = HandleDatabase->NumberOfTokens;
        }

        *StringPackageInstance = HandlePackageInstance;
      }
    }
    //
    // end for
    //
  } else {
    return EFI_SUCCESS;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiNewPack (
  IN  EFI_HII_PROTOCOL    *This,
  IN  EFI_HII_PACKAGES    *Packages,
  OUT EFI_HII_HANDLE      *Handle
  )
/*++

Routine Description:

  Extracts the various packs from a package list.
  
Arguments:

  This      - Pointer of HII protocol.
  Packages  - Pointer of HII packages.
  Handle    - Handle value to be returned.

Returns: 

  EFI_SUCCESS           - Pacakges has added to HII database successfully.
  EFI_INVALID_PARAMETER - Invalid parameter.

--*/
{
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_HANDLE_DATABASE   *Database;
  EFI_HII_PACK_HEADER       *PackageHeader;
  EFI_HII_GLOBAL_DATA       *GlobalData;
  EFI_HII_IFR_PACK          *IfrPack;
  EFI_HII_STRING_PACK       *StringPack;
  EFI_HII_FONT_PACK         *FontPack;
  EFI_HII_KEYBOARD_PACK     *KeyboardPack;
  EFI_STATUS                Status;
  UINTN                     IfrSize;
  UINTN                     StringSize;
  UINTN                     TotalStringSize;
  UINTN                     InstanceSize;
  UINTN                     Count;
  UINTN                     Index;
  UINT16                    Member;
  EFI_GUID                  Guid;
  EFI_FORM_SET_STUB         FormSetStub;
  UINT8                     *Location;
  UINT16                    Unicode;
  UINT16                    NumWideGlyphs;
  UINT16                    NumNarrowGlyphs;
  UINT32                    NumberOfTokens;
  UINT32                    TotalTokenNumber;
  UINT8                     *Local;
  EFI_NARROW_GLYPH          *NarrowGlyph;

  if (Packages->NumberOfPackages == 0 || This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData           = EFI_HII_DATA_FROM_THIS (This);

  GlobalData        = HiiData->GlobalData;

  Database          = HiiData->DatabaseHead;

  PackageInstance   = NULL;
  IfrPack           = NULL;
  StringPack        = NULL;
  InstanceSize      = 0;
  IfrSize           = 0;
  StringSize        = 0;
  TotalStringSize   = 0;
  NumberOfTokens    = 0;
  TotalTokenNumber  = 0;

  //
  // Search through the passed in Packages for the IfrPack and any StringPack.
  //
  for (Index = 0; Index < Packages->NumberOfPackages; Index++) {

    PackageHeader = *(EFI_HII_PACK_HEADER **) (((UINT8 *) Packages) + sizeof (EFI_HII_PACKAGES) + Index * sizeof (VOID *));

    switch (PackageHeader->Type) {
    case EFI_HII_IFR:
      //
      // There shoule be only one Ifr package.
      //
      ASSERT (IfrPack == NULL);
      IfrPack = (EFI_HII_IFR_PACK *) PackageHeader;
      break;

    case EFI_HII_STRING:
      StringPack = (EFI_HII_STRING_PACK *) PackageHeader;
      //
      // Sending me a String Package. Get its size.
      //
      Status = GetPackSize ((VOID *) StringPack, &StringSize, &NumberOfTokens);
      ASSERT (!EFI_ERROR (Status));

      //
      // The size which GetPackSize() returns include the null terminator. So if multiple
      // string packages are passed in, merge all these packages, and only pad one null terminator.
      //
      if (TotalStringSize > 0) {
        TotalStringSize -= sizeof (EFI_HII_STRING_PACK);
      }

      TotalStringSize += StringSize;
      TotalTokenNumber += NumberOfTokens;
      break;
    }
  }
  //
  // If sending a StringPack without an IfrPack, you must include a GuidId
  //
  if ((StringPack != NULL) && (IfrPack == NULL)) {
    if (Packages->GuidId == NULL) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // If passing in an IfrPack and a GuidId is provided, ensure they are the same value.
  //
  if ((IfrPack != NULL) && (Packages->GuidId != NULL)) {
    Location  = ((UINT8 *) IfrPack);
    Location  = (UINT8 *) (((UINTN) Location) + sizeof (EFI_HII_PACK_HEADER));

    //
    // Advance to the Form Set Op-code
    //
    for (Count = 0; ((EFI_IFR_OP_HEADER *) &Location[Count])->OpCode != EFI_IFR_FORM_SET_OP;) {
      Count = Count + ((EFI_IFR_OP_HEADER *) &Location[Count])->Length;
    }
    //
    // Copy to local variable
    //
    CopyMem (&Guid, &((EFI_IFR_FORM_SET *) &Location[Count])->Guid, sizeof (EFI_GUID));

    //
    // Check to see if IfrPack->Guid != GuidId
    //
    if (!CompareGuid (&Guid, Packages->GuidId)) {
      //
      // If a string package is present, the GUIDs should have agreed.  Return an error
      //
      if (StringPack != NULL) {
        return EFI_INVALID_PARAMETER;
      }
    }
  }
  //
  // If someone is passing in a string only, create a dummy IfrPack with a Guid
  // to enable future searching of this data.
  //
  if ((IfrPack == NULL) && (StringPack != NULL)) {
    ZeroMem (&FormSetStub, sizeof (FormSetStub));

    FormSetStub.Header.Type           = EFI_HII_IFR;
    FormSetStub.Header.Length         = sizeof (EFI_FORM_SET_STUB);

    FormSetStub.FormSet.Header.OpCode = EFI_IFR_FORM_SET_OP;
    FormSetStub.FormSet.Header.Length = (UINT8) sizeof (EFI_IFR_FORM_SET);
    //
    // Dummy string
    //
    FormSetStub.FormSet.FormSetTitle  = 0x02;
    CopyMem (&FormSetStub.FormSet.Guid, Packages->GuidId, sizeof (EFI_GUID));

    FormSetStub.EndFormSet.Header.OpCode  = EFI_IFR_END_FORM_SET_OP;
    FormSetStub.EndFormSet.Header.Length  = (UINT8) sizeof (EFI_IFR_END_FORM_SET);
    IfrPack = (EFI_HII_IFR_PACK *) &FormSetStub;
  }

  if (IfrPack != NULL) {
    //
    // Sending me an IFR Package. Get its size.
    //
    Status = GetPackSize ((VOID *) IfrPack, &IfrSize, NULL);
    ASSERT (!EFI_ERROR (Status));
  }
  //
  // Prepare the internal package instace buffer to store package data.
  //
  InstanceSize = IfrSize + TotalStringSize;

  if (InstanceSize != 0) {
    PackageInstance = AllocateZeroPool (InstanceSize + sizeof (EFI_HII_PACKAGE_INSTANCE));

    ASSERT (PackageInstance);

    //
    // If there is no DatabaseHead allocated - allocate one
    //
    if (HiiData->DatabaseHead == NULL) {
      HiiData->DatabaseHead = AllocateZeroPool (sizeof (EFI_HII_HANDLE_DATABASE));
      ASSERT (HiiData->DatabaseHead);
    }
    //
    // If the head is being used (Handle is non-zero), allocate next Database and
    // add it to the linked-list
    //
    if (HiiData->DatabaseHead->Handle != 0) {
      HandleDatabase = AllocateZeroPool (sizeof (EFI_HII_HANDLE_DATABASE));

      ASSERT (HandleDatabase);

      for (; Database->NextHandleDatabase != NULL; Database = Database->NextHandleDatabase)
        ;

      //
      // We are sitting on the Database entry which contains the null Next pointer.  Fix it.
      //
      Database->NextHandleDatabase = HandleDatabase;

    }

    Database = HiiData->DatabaseHead;

    //
    // Initialize this instance data
    //
    for (*Handle = 1; Database->NextHandleDatabase != NULL; Database = Database->NextHandleDatabase) {
      //
      // Since the first Database instance will have a passed back handle of 1, we will continue
      // down the linked list of entries until we encounter the end of the linked list.  Each time
      // we go down one level deeper, increment the handle value that will be passed back.
      //
      if (Database->Handle >= *Handle) {
        *Handle = (EFI_HII_HANDLE) (Database->Handle + 1);
      }
    }

    PackageInstance->Handle     = *Handle;
    PackageInstance->IfrSize    = IfrSize;
    PackageInstance->StringSize = TotalStringSize;
    if (Packages->GuidId != NULL) {
      CopyMem (&PackageInstance->Guid, Packages->GuidId, sizeof (EFI_GUID));
    }

    Database->Buffer              = PackageInstance;
    Database->Handle              = PackageInstance->Handle;
    Database->NumberOfTokens      = TotalTokenNumber;
    Database->NextHandleDatabase  = NULL;
  }
  //
  // Copy the Ifr package data into package instance.
  //
  if (IfrSize > 0) {
    CopyMem (&PackageInstance->IfrData, IfrPack, IfrSize);
  }
  //
  // Main loop to store package data into HII database.
  //
  StringSize      = 0;
  TotalStringSize = 0;

  for (Index = 0; Index < Packages->NumberOfPackages; Index++) {

    PackageHeader = *(EFI_HII_PACK_HEADER **) (((UINT8 *) Packages) + sizeof (EFI_HII_PACKAGES) + Index * sizeof (VOID *));

    switch (PackageHeader->Type) {
    case EFI_HII_STRING:
      StringPack = (EFI_HII_STRING_PACK *) PackageHeader;
      //
      // The size which GetPackSize() returns include the null terminator. So if multiple
      // string packages are passed in, merge all these packages, and only pad one null terminator.
      //
      if (TotalStringSize > 0) {
        TotalStringSize -= sizeof (EFI_HII_STRING_PACK);
      }

      GetPackSize ((VOID *) StringPack, &StringSize, &NumberOfTokens);
      CopyMem ((CHAR8 *) (&PackageInstance->IfrData) + IfrSize + TotalStringSize, StringPack, StringSize);

      TotalStringSize += StringSize;
      break;

    case EFI_HII_HANDLES:
      CopyMem (&PackageInstance->HandlePack, PackageHeader, sizeof (EFI_HII_HANDLE_PACK));
      break;

    case EFI_HII_FONT:
      FontPack = (EFI_HII_FONT_PACK *) PackageHeader;
      //
      // Add whatever narrow glyphs were passed to us if undefined
      //
      CopyMem (&NumNarrowGlyphs, &FontPack->NumberOfNarrowGlyphs, sizeof (UINT16));
      for (Count = 0; Count <= NumNarrowGlyphs; Count++) {
        Local       = (UINT8 *) (&FontPack->NumberOfWideGlyphs + sizeof (UINT8)) + (sizeof (EFI_NARROW_GLYPH)) * Count;
        NarrowGlyph = (EFI_NARROW_GLYPH *) Local;
        CopyMem (&Member, &NarrowGlyph->UnicodeWeight, sizeof (UINT16));
        //
        // If the glyph is already defined, do not overwrite it.  It is what it is.
        //
        CopyMem (&Unicode, &GlobalData->NarrowGlyphs[Member].UnicodeWeight, sizeof (UINT16));
        if (Unicode == 0) {
          CopyMem (&GlobalData->NarrowGlyphs[Member], Local, sizeof (EFI_NARROW_GLYPH));
        }
      }
      //
      // Add whatever wide glyphs were passed to us if undefined
      //
      CopyMem (&NumWideGlyphs, &FontPack->NumberOfWideGlyphs, sizeof (UINT16));
      for (Count = 0; Count <= NumWideGlyphs; Count++) {
        Local = (UINT8 *) (&FontPack->NumberOfWideGlyphs + sizeof (UINT8)) +
          (sizeof (EFI_NARROW_GLYPH)) *
          NumNarrowGlyphs;
        CopyMem (
          &Member,
          (UINTN *) (Local + sizeof (EFI_WIDE_GLYPH) * Count),
          sizeof (UINT16)
          );
        //
        // If the glyph is already defined, do not overwrite it.  It is what it is.
        //
        CopyMem (&Unicode, &GlobalData->WideGlyphs[Member].UnicodeWeight, sizeof (UINT16));
        if (Unicode == 0) {
          Local = (UINT8*)(&FontPack->NumberOfWideGlyphs + sizeof(UINT8)) + (sizeof(EFI_NARROW_GLYPH)) * NumNarrowGlyphs;
          CopyMem (
            &GlobalData->WideGlyphs[Member],
            (UINTN *) (Local + sizeof (EFI_WIDE_GLYPH) * Count),
            sizeof (EFI_WIDE_GLYPH)
            );
        }
      }
      break;

    case EFI_HII_KEYBOARD:
      KeyboardPack = (EFI_HII_KEYBOARD_PACK *) PackageHeader;
      //
      // Sending me a Keyboard Package
      //
      if (KeyboardPack->DescriptorCount > 105) {
        return EFI_INVALID_PARAMETER;
      }
      //
      // If someone updates the Descriptors with a count of 0, blow aware the overrides.
      //
      if (KeyboardPack->DescriptorCount == 0) {
        ZeroMem (GlobalData->OverrideKeyboardLayout, sizeof (EFI_KEY_DESCRIPTOR) * 106);
      }

      if (KeyboardPack->DescriptorCount < 106 && KeyboardPack->DescriptorCount > 0) {
        //
        // If SystemKeyboard was updated already, then steer changes to the override database
        //
        if (GlobalData->SystemKeyboardUpdate) {
          ZeroMem (GlobalData->OverrideKeyboardLayout, sizeof (EFI_KEY_DESCRIPTOR) * 106);
          for (Count = 0; Count < KeyboardPack->DescriptorCount; Count++) {
            CopyMem (&Member, &KeyboardPack->Descriptor[Count].Key, sizeof (UINT16));
            CopyMem (
              &GlobalData->OverrideKeyboardLayout[Member],
              &KeyboardPack->Descriptor[Count],
              sizeof (EFI_KEY_DESCRIPTOR)
              );
          }
        } else {
          //
          // SystemKeyboard was never updated, so this is likely the keyboard driver setting the System database.
          //
          ZeroMem (GlobalData->SystemKeyboardLayout, sizeof (EFI_KEY_DESCRIPTOR) * 106);
          for (Count = 0; Count < KeyboardPack->DescriptorCount; Count++) {
            CopyMem (&Member, &KeyboardPack->Descriptor->Key, sizeof (UINT16));
            CopyMem (
              &GlobalData->SystemKeyboardLayout[Member],
              &KeyboardPack->Descriptor[Count],
              sizeof (EFI_KEY_DESCRIPTOR)
              );
          }
          //
          // Just updated the system keyboard database, reflect that in the global flag.
          //
          GlobalData->SystemKeyboardUpdate = TRUE;
        }
      }
      break;

    default:
      break;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiRemovePack (
  IN EFI_HII_PROTOCOL    *This,
  IN EFI_HII_HANDLE      Handle
  )
/*++

Routine Description:
  Removes the various packs from a Handle
  
Arguments:

Returns: 

--*/
{
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_HANDLE_DATABASE   *PreviousHandleDatabase;

  if (This == NULL || Handle == 0) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData         = EFI_HII_DATA_FROM_THIS (This);

  HandleDatabase  = HiiData->DatabaseHead;
  PackageInstance = NULL;

  //
  // Initialize the Previous with the Head of the Database
  //
  PreviousHandleDatabase = HandleDatabase;

  for (; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
    //
    // Match the numeric value with the database entry - if matched,
    // free the package instance and apply fix-up to database linked list
    //
    if (Handle == HandleDatabase->Handle) {
      PackageInstance = HandleDatabase->Buffer;

      //
      // Free the Package Instance
      //
      FreePool (PackageInstance);

      //
      // If this was the only Handle in the database
      //
      if (HiiData->DatabaseHead == HandleDatabase) {
        HiiData->DatabaseHead = NULL;
      }
      //
      // Make the parent->Next point to the current->Next
      //
      PreviousHandleDatabase->NextHandleDatabase = HandleDatabase->NextHandleDatabase;
      FreePool (HandleDatabase);
      return EFI_SUCCESS;
    }
    //
    // If this was not the HandleDatabase entry we were looking for, cache it just in case the next one is
    //
    PreviousHandleDatabase = HandleDatabase;
  }
  //
  // No handle was found - error condition
  //
  if (PackageInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
