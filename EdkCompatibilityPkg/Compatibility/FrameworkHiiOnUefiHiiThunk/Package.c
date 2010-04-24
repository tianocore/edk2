/** @file
  Implement protocol interface related to package registrations.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"
#include "HiiHandle.h"


BOOLEAN mInFrameworkHiiNewPack = FALSE;
BOOLEAN mInFrameworkHiiRemovePack = FALSE;
BOOLEAN mInFrameworkUpdatePakcage = FALSE;
UINT64  mGuidCount = 0;

EFI_GUID mGuidBase = { 0x14f95e01, 0xd562, 0x432e, { 0x84, 0x4a, 0x95, 0xa4, 0x39, 0x5, 0x10, 0x7e }};



/**
  Get the number of Form, STRING and Font packages in the package list passed in.

  @param Packages             Package List.
  @param IfrPackageCount      Number of IFR Packages.
  @param StringPackageCount   Number of String Packages.
  @param FontPackageCount     Number of Font Packages.

  @retval EFI_INVALID_PARAMETER If the Package List has package with type of 
                                EFI_HII_PACKAGE_KEYBOARD_LAYOUT, EFI_HII_PACKAGE_FONTS, EFI_HII_PACKAGE_IMAGES.
  @retval EFI_SUCCESS           Successfully get the number of IFR and STRING package.
                                 

**/
EFI_STATUS
GetPackageCount (
  IN CONST EFI_HII_PACKAGES               *Packages,
  OUT UINTN                               *IfrPackageCount,
  OUT UINTN                               *StringPackageCount,
  OUT UINTN                               *FontPackageCount
  )
{
  UINTN                         Index;
  TIANO_AUTOGEN_PACKAGES_HEADER **TianoAutogenPackageHdrArray;

  ASSERT (Packages != NULL);
  ASSERT (IfrPackageCount != NULL);
  ASSERT (StringPackageCount != NULL);
  ASSERT (FontPackageCount != NULL);

  *IfrPackageCount = 0;
  *StringPackageCount = 0;
  *FontPackageCount = 0;

  TianoAutogenPackageHdrArray = (TIANO_AUTOGEN_PACKAGES_HEADER **) (((UINT8 *) &Packages->GuidId) + sizeof (Packages->GuidId));
  
  for (Index = 0; Index < Packages->NumberOfPackages; Index++) {
    //
    // The current UEFI HII build tool generate a binary in the format defined by 
    // TIANO_AUTOGEN_PACKAGES_HEADER. We assume that all packages generated in
    // this binary is with same package type. So the returned IfrPackageCount and StringPackageCount
    // may not be the exact number of valid package number in the binary generated 
    // by HII Build tool.
    //
    switch (TianoAutogenPackageHdrArray[Index]->FrameworkPackageHeader.Type) {
      case EFI_HII_IFR:
        *IfrPackageCount += 1;
        break;
      case EFI_HII_STRING:
        *StringPackageCount += 1;
        break;

      case EFI_HII_FONT:
        *FontPackageCount += 1;
        break;

      //
      // The following fonts are invalid for a module that using Framework to UEFI thunk layer.
      //
      default:
        ASSERT (FALSE);
        return EFI_INVALID_PARAMETER;
        break;
    }
  }

  return EFI_SUCCESS;
}

/**
  Insert the String Package into the Package Lists which has the TAG GUID matching
  the PackageListGuid of the String Package. 

  The Package List must have only IFR Package and no String Package. 
  Otherwise, ASSERT.

  @param Private                      The HII THUNK driver context data.
  @param StringPackageThunkContext    The HII THUNK context data.
  @param StringPackageListHeader      The String Package List Header.
  
**/
VOID
UpdatePackListWithOnlyIfrPack (
  IN       HII_THUNK_PRIVATE_DATA      *Private,
  IN       HII_THUNK_CONTEXT            *StringPackageThunkContext,
  IN CONST EFI_HII_PACKAGE_LIST_HEADER *StringPackageListHeader
  )
{
  EFI_STATUS                 Status;
  LIST_ENTRY                 *Link;
  HII_THUNK_CONTEXT          *ThunkContext;

  Link = GetFirstNode (&Private->ThunkContextListHead);
  while (!IsNull (&Private->ThunkContextListHead, Link)) {

    ThunkContext = HII_THUNK_CONTEXT_FROM_LINK (Link);

    if (StringPackageThunkContext != ThunkContext) {
      //
      // Skip the String Package Thunk Entry itself.
      //
    
      if (CompareGuid (&StringPackageListHeader->PackageListGuid, &ThunkContext->TagGuid)) {

        ASSERT (ThunkContext->StringPackageCount == 0 && ThunkContext->IfrPackageCount == 1);

        ThunkContext->StringPackageCount = GetPackageCountByType (StringPackageListHeader, EFI_HII_PACKAGE_STRINGS);
        
        Status = mHiiDatabase->UpdatePackageList (
                                              mHiiDatabase,
                                              ThunkContext->UefiHiiHandle,
                                              StringPackageListHeader
                                              );
        ASSERT_EFI_ERROR (Status);

        ThunkContext->SharingStringPack = TRUE;
        StringPackageThunkContext->SharingStringPack = TRUE;

      }
    }
    
    Link = GetNextNode (&Private->ThunkContextListHead, Link);
  }

}

/**
  Caculate the size of UEFI Simple Font Package that is needed to 
  convert all the font a Framework Font Paackage.

  ONLY Narrow Font is supported. Wide Font is discarded. 

  If the Package Header is not of EFI_HII_FONT type, then ASSERT.

  @param   PackHeader Pointer to Framework Font Package.
  
  @return  The size of the UEFI Simple Font Package.
  
**/
UINTN
GetUefiSimpleFontPackSize (
  IN CONST EFI_HII_PACK_HEADER * PackHeader
  )
{
  UINTN             Size;
  EFI_HII_FONT_PACK *FwFontPack;

  FwFontPack = (EFI_HII_FONT_PACK *) PackHeader;

  ASSERT (FwFontPack->Header.Type == EFI_HII_FONT);
  
  Size = sizeof (EFI_HII_SIMPLE_FONT_PACKAGE_HDR) 
    + (FwFontPack->NumberOfNarrowGlyphs * sizeof (EFI_NARROW_GLYPH));

   return Size;
}


/**
  Convert Font Package in Framework format to a newly allocated UEFI
  Simple Font Package.

  ONLY Narrow Font is supported. Wide Font is discarded. 

  If memory allocation fails, then ASSERT.

  @param PackHeader        Pointer to Framework Font Package header.

  @return UEFI Simple Font Package.
**/
EFI_HII_SIMPLE_FONT_PACKAGE_HDR *
FrameworkFontPackToUefiSimpliedFont (
  IN CONST EFI_HII_PACK_HEADER * PackHeader
  )
{
  EFI_HII_SIMPLE_FONT_PACKAGE_HDR       *FontPack;
  UINTN                                 Size;
  EFI_NARROW_GLYPH                      *FwNarrowGlyph;
  EFI_NARROW_GLYPH                      *NarrowGlyph;
  UINTN                                 Idx;
  EFI_HII_FONT_PACK                     *FwFontPack;

  Size = GetUefiSimpleFontPackSize (PackHeader);

  FwFontPack = (EFI_HII_FONT_PACK *) PackHeader;

  FontPack      = AllocateZeroPool (Size);
  ASSERT (FontPack != NULL);

  //
  // Prepare the Header information.
  //
  FontPack->Header.Length = (UINT32) Size;
  FontPack->Header.Type = EFI_HII_PACKAGE_SIMPLE_FONTS;

  FontPack->NumberOfNarrowGlyphs = FwFontPack->NumberOfNarrowGlyphs;
  
  //
  // ONLY Narrow Font is supported. Wide Font is discarded. 
  //
  FontPack->NumberOfWideGlyphs = 0;
 
  //
  // Copy Narrow Glyph
  //
  NarrowGlyph   = (EFI_NARROW_GLYPH *) (FontPack + 1);
  FwNarrowGlyph = (EFI_NARROW_GLYPH *) (FwFontPack + 1);
  CopyMem (NarrowGlyph, FwNarrowGlyph, sizeof (EFI_NARROW_GLYPH) * FwFontPack->NumberOfNarrowGlyphs);
  for (Idx = 0; Idx < FwFontPack->NumberOfNarrowGlyphs; Idx++) {
    //
    // Clear the GLYPH_NON_BREAKING (EFI_GLYPH_WIDE is used here as they are all 0x02)
    // attribute which is not defined in UEFI EFI_NARROW_GLYPH
    //
    NarrowGlyph[Idx].Attributes = (UINT8) (NarrowGlyph[Idx].Attributes & ~(EFI_GLYPH_WIDE));
  }

  return FontPack;
}

/**
  Prepare a UEFI Package List from a Framework HII package list registered
  from a Framework HII NewPack () function.

  If either Packages or PackageListGuid is NULL, then ASSERT.
  
  @param Packages                     The Framework HII Package List.
  @param PackageListGuid              The Package List GUID.


  @return The UEFI Package List.  
**/
EFI_HII_PACKAGE_LIST_HEADER *
PrepareUefiPackageListFromFrameworkHiiPackages (
  IN CONST EFI_HII_PACKAGES            *Packages,
  IN CONST EFI_GUID                    *PackageListGuid
  )
{
  UINTN                       NumberOfPackages;
  EFI_HII_PACKAGE_LIST_HEADER *PackageListHeader;
  UINT8                       *PackageListData;
  UINT32                      PackageListLength;
  UINT32                      PackageLength;
  EFI_HII_PACKAGE_HEADER      PackageHeader;
  UINTN                       Index;
  TIANO_AUTOGEN_PACKAGES_HEADER   **TianoAutogenPackageHdrArray;
  EFI_HII_SIMPLE_FONT_PACKAGE_HDR *FontPack;
  

  ASSERT (Packages != NULL);
  ASSERT (PackageListGuid != NULL);

  TianoAutogenPackageHdrArray = (TIANO_AUTOGEN_PACKAGES_HEADER **) ((UINT8 *) &Packages->GuidId + sizeof (Packages->GuidId));
  NumberOfPackages = Packages->NumberOfPackages;

  PackageListLength = sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  for (Index = 0; Index < NumberOfPackages; Index++) {
    if (TianoAutogenPackageHdrArray[Index]->FrameworkPackageHeader.Type == EFI_HII_FONT) {
      //
      // There is no tool to generate Font package in Framework HII's implementation.
      // Therefore, Font Package be a C structure defined in Framework HII code. 
      // Therefore, Font Package will be in Framework HII format defined by EFI_HII_FONT_PACK.
      // We need to create a UEFI Simple Font Package and copy over all data. Hence, EFI_HII_FONT
      // is handled differently than EFI_HII_IFR and EFI_HII_STRING.
      //
      PackageListLength = (UINT32) (PackageListLength + GetUefiSimpleFontPackSize (&TianoAutogenPackageHdrArray[Index]->FrameworkPackageHeader));
      
    } else {
      //
      // For EFI_HII_IFR and EFI_HII_STRING, EDK II's VFR Compiler and Build.exe will generate a binary in a format
      // defined by TIANO_AUTOGEN_PACKAGES_HEADER. A Framework HII's EFI_HII_PACK_HEADER is inserted before
      // the UEFI package data.
      //
      CopyMem (&PackageLength, &TianoAutogenPackageHdrArray[Index]->FrameworkPackageHeader.Length, sizeof (UINT32));
      //
      // EFI_HII_PACK_HEADER.FrameworkPackageHeader.Length include the sizeof FrameworkPackageHeader itself.
      //
      PackageListLength += (PackageLength - sizeof(EFI_HII_PACK_HEADER));
      
    }
  }

  //
  // Include the lenght of EFI_HII_PACKAGE_END
  //
  PackageListLength += sizeof (EFI_HII_PACKAGE_HEADER);
  PackageListHeader = AllocateZeroPool (PackageListLength);
  ASSERT (PackageListHeader != NULL);

  CopyMem (&PackageListHeader->PackageListGuid, PackageListGuid, sizeof (EFI_GUID));
  PackageListHeader->PackageLength = PackageListLength;

  PackageListData = ((UINT8 *) PackageListHeader) + sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  //
  // Build the UEFI Package List.
  //
  for (Index = 0; Index < NumberOfPackages; Index++) {
    if (TianoAutogenPackageHdrArray[Index]->FrameworkPackageHeader.Type == EFI_HII_FONT) {
      PackageLength = (UINT32) GetUefiSimpleFontPackSize (&TianoAutogenPackageHdrArray[Index]->FrameworkPackageHeader);
      FontPack = FrameworkFontPackToUefiSimpliedFont (&TianoAutogenPackageHdrArray[Index]->FrameworkPackageHeader);
      CopyMem (PackageListData, FontPack, PackageLength);
      FreePool (FontPack);
      
    } else {
      CopyMem (&PackageLength, &(TianoAutogenPackageHdrArray[Index]->FrameworkPackageHeader.Length), sizeof (UINT32));
      PackageLength  -= sizeof (EFI_HII_PACK_HEADER);
      CopyMem (PackageListData, &(TianoAutogenPackageHdrArray[Index]->PackageHeader), PackageLength);
      
    }
    PackageListData += PackageLength;
  }

  //
  // Append EFI_HII_PACKAGE_END
  //
  PackageHeader.Type = EFI_HII_PACKAGE_END;
  PackageHeader.Length = sizeof (EFI_HII_PACKAGE_HEADER);
  CopyMem (PackageListData, &PackageHeader, PackageHeader.Length);

  return PackageListHeader;  
}


/**
  Generate a Random GUID.
  
  @param Guid On output, a Random GUID will be filled.

**/
VOID
GenerateRandomGuid (
  OUT           EFI_GUID * Guid
  )
{
  CopyGuid (Guid, &mGuidBase);

  mGuidCount++;  
  *((UINT64 *) Guid) = *((UINT64 *) Guid) + mGuidCount;
}

/**
  Given a Package List with only a IFR package, find the Package List that only has a String Package based on
  the TAG GUID. Then export the String Package from the Package List and insert it
  to the given IFR package.

  This is to handle the case of Framework HII interface which allow String Package
  and IFR package to be registered using two different NewPack () calls.

  @param Private                      The HII THUNK driver context data.
  @param IfrThunkContext              Package List with only a IFR package.

  @retval EFI_SUCCESS                 If the String Package is found and inserted to the
                                      Package List with only a IFR package.
  @retval EFI_NOT_FOUND               No String Package matching the TAG GUID is found.
**/
EFI_STATUS
FindStringPackAndUpdatePackListWithOnlyIfrPack (
  IN HII_THUNK_PRIVATE_DATA          *Private,
  IN HII_THUNK_CONTEXT                *IfrThunkContext
  )
{
  EFI_STATUS                      Status;
  LIST_ENTRY                      *Link;
  EFI_HII_PACKAGE_LIST_HEADER     *StringPackageListHeader;
  UINTN                           Size;
  HII_THUNK_CONTEXT               *ThunkContext;
  
  Link = GetFirstNode (&Private->ThunkContextListHead);

  while (!IsNull (&Private->ThunkContextListHead, Link)) {

    ThunkContext = HII_THUNK_CONTEXT_FROM_LINK (Link);

    if (ThunkContext != IfrThunkContext) {
      if (CompareGuid (&IfrThunkContext->TagGuid, &ThunkContext->TagGuid) && (ThunkContext->IfrPackageCount == 0)) {
        StringPackageListHeader = NULL;
        Status = ExportPackageLists (ThunkContext->UefiHiiHandle, &StringPackageListHeader, &Size);
        ASSERT_EFI_ERROR (Status);
        if (StringPackageListHeader == NULL) {
          return EFI_NOT_FOUND;
        }

        IfrThunkContext->StringPackageCount = GetPackageCountByType (StringPackageListHeader, EFI_HII_PACKAGE_STRINGS);
        //
        // Add Function to only get only String Packages from the Package List
        //
        Status = mHiiDatabase->UpdatePackageList (
                                  mHiiDatabase,
                                  IfrThunkContext->UefiHiiHandle,
                                  StringPackageListHeader
                                  );
        ASSERT_EFI_ERROR (Status);
        
        FreePool (StringPackageListHeader);

        IfrThunkContext->SharingStringPack = TRUE;
        ThunkContext->SharingStringPack = TRUE;
        
        return EFI_SUCCESS;

      }
    }

    Link = GetNextNode (&Private->ThunkContextListHead, Link);
  }

  //
  // A Form Package must have a String Package to function.
  // If ASSERT here, check the sequence of call to Hii->NewPack. 
  // String Pack must be registered before Ifr Package is registered.
  //
  ASSERT (FALSE);
  return EFI_NOT_FOUND;
  
}


/**
  Register the Package List passed from the Framework HII NewPack () interface.
  The FRAMEWORK_EFI_HII_HANDLE will be returned.

  @param This                         The EFI_HII_PROTOCOL context data. Only used
                                      to call HiiRemovePack.
  @param Private                      The HII THUNK driver context data.
  @param Packages                     Package List.
  @param Handle                       On output, a FRAMEWORK_EFI_HII_HANDLE number is
                                      returned.

  @retval EFI_SUCCESS                 The Package List is registered successfull in 
                                      the database.
  @retval EFI_UNSUPPORTED             The number of IFR package in the package list
                                      is greater than 1.
  @retval EFI_OUT_OF_RESOURCE         Not enough resouce.
  
**/
EFI_STATUS
UefiRegisterPackageList (
  IN  EFI_HII_PROTOCOL            *This,
  IN  HII_THUNK_PRIVATE_DATA      *Private,
  IN  EFI_HII_PACKAGES            *Packages,
  OUT FRAMEWORK_EFI_HII_HANDLE    *Handle
  )
{
  EFI_STATUS                  Status;
  UINTN                       StringPackageCount;
  UINTN                       IfrPackageCount;
  UINTN                       FontPackageCount;
  EFI_HII_PACKAGE_LIST_HEADER *PackageListHeader;
  HII_THUNK_CONTEXT           *ThunkContext;
  HII_THUNK_CONTEXT           *ThunkContextToRemove;
  EFI_GUID                    GuidId;
  EFI_HII_PACKAGE_HEADER      *IfrPackage;

  PackageListHeader = NULL;

  Status = GetPackageCount (Packages, &IfrPackageCount, &StringPackageCount, &FontPackageCount);
  ASSERT_EFI_ERROR (Status);
  
  if (IfrPackageCount > 1) {
    //
    // HII Thunk only handle package with 0 or 1 IFR package. 
    //
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  ThunkContext = CreateThunkContext (Private, StringPackageCount, IfrPackageCount);
  if (ThunkContext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  ThunkContext->ByFrameworkHiiNewPack = TRUE;
  
  if (Packages->GuidId == NULL) {
    //
    // UEFI HII Database require Package List GUID must be unique.
    //
    // If Packages->GuidId is NULL, the caller of FramworkHii->NewPack is registering
    // packages with at least 1 StringPack and 1 IfrPack. Therefore, Packages->GuidId is
    // not used as the name of the package list.  Formset GUID is used as the Package List
    // GUID instead.
    //
    ASSERT ((StringPackageCount >=1 && IfrPackageCount == 1) || (FontPackageCount > 0));
    if (IfrPackageCount > 0) {
      IfrPackage = GetIfrPackage (Packages);
      if (IfrPackage == NULL) {
        Status = EFI_NOT_FOUND;
        goto Done;
      }
      GetFormSetGuid (IfrPackage, &ThunkContext->TagGuid);
    } else {
      ASSERT (FontPackageCount > 0);
      GenerateRandomGuid (&ThunkContext->TagGuid);
    }
    
  } else {
    ThunkContextToRemove = TagGuidToIfrPackThunkContext (Private, Packages->GuidId);
    
    if (IfrPackageCount > 0 && 
        StringPackageCount > 0 && 
        (ThunkContextToRemove != NULL)) {
        DEBUG((EFI_D_WARN, "Framework code registers HII package list with the same GUID more than once.\n"));
        DEBUG((EFI_D_WARN, "Remove the previously registered package list and register the new one.\n"));
        HiiRemovePack (This, ThunkContextToRemove->FwHiiHandle);
    }
    CopyGuid (&ThunkContext->TagGuid, Packages->GuidId);
    
  }

  //
  // UEFI HII require EFI_HII_CONFIG_ACCESS_PROTOCOL to be installed on a EFI_HANDLE, so
  // that Setup Utility can load the Buffer Storage using this protocol. An UEFI VFR can only
  // produce IFR package generated with Buffer Storage type and EFI Variable Storage.
  // The default EFI_HII_CONFIG_ACCESS_PROTOCOL is used to Get/Set the Buffer Storage.
  //
  if (IfrPackageCount != 0) {
    InstallDefaultConfigAccessProtocol (Packages, ThunkContext);
  }
  
  PackageListHeader = PrepareUefiPackageListFromFrameworkHiiPackages (Packages, &ThunkContext->TagGuid);
  Status = mHiiDatabase->NewPackageList (
              mHiiDatabase,
              PackageListHeader,  
              ThunkContext->UefiHiiDriverHandle,
              &ThunkContext->UefiHiiHandle
              );
  if (Status == EFI_INVALID_PARAMETER) {
    FreePool (PackageListHeader);
    
    //
    // UEFI HII database does not allow two package list with the same GUID.
    // In Framework HII implementation, Packages->GuidId is used as an identifier to associate 
    // a PackageList with only IFR to a Package list the with String package.
    //
    GenerateRandomGuid (&GuidId);

    PackageListHeader = PrepareUefiPackageListFromFrameworkHiiPackages (Packages, &GuidId);
    Status = mHiiDatabase->NewPackageList (
                mHiiDatabase,
                PackageListHeader,  
                ThunkContext->UefiHiiDriverHandle,
                &ThunkContext->UefiHiiHandle
                );
  }

  //
  // BUGBUG: Remove when development is done
  //
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  if (IfrPackageCount == 0) {
    if (StringPackageCount != 0) {
      //
      // Look for a Package List with only IFR Package with the same TAG GUID name.
      // If found one, add the String Packages to the found Package List.
      // This is needed because Framework HII Module may not register the String Package
      // and IFR Package in one NewPack () call.
      //
      UpdatePackListWithOnlyIfrPack (
          Private,
          ThunkContext,
          PackageListHeader
      );
    }
  } else {
    if (StringPackageCount == 0) {
      //
      // Look for the String Package with the same TAG GUID name and add
      // the found String Package to this Package List.
      // This is needed because Framework HII Module may not register the String Package
      // and IFR Package in one NewPack () call.
      //
      Status = FindStringPackAndUpdatePackListWithOnlyIfrPack (
                  Private,
                  ThunkContext
                  );

      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }
    
    //
    // Parse the Formset. Must be called after FindStringPackAndUpdatePackListWithOnlyIfrPack is called so
    // that String Package is ready.
    //
    ThunkContext->FormSet = ParseFormSet (ThunkContext->UefiHiiHandle);
    ASSERT (ThunkContext->FormSet != NULL);
        
  }

Done:
  if (EFI_ERROR (Status)) {
    DestroyThunkContext (ThunkContext);
  } else {
    InsertTailList (&Private->ThunkContextListHead, &ThunkContext->Link);
    *Handle = ThunkContext->FwHiiHandle;
  }

	if (PackageListHeader != NULL) {
    FreePool (PackageListHeader);
  }
  
  return Status;
}


/**

  Registers the various packages that are passed in a Package List.

  @param This      Pointer of Frameowk HII protocol instance.
  @param Packages  Pointer of HII packages.
  @param Handle    Handle value to be returned.

  @retval EFI_SUCCESS           Pacakges has added to HII database successfully.
  @retval EFI_INVALID_PARAMETER If Handle or Packages is NULL.

**/
EFI_STATUS
EFIAPI
HiiNewPack (
  IN  EFI_HII_PROTOCOL               *This,
  IN  EFI_HII_PACKAGES               *Packages,
  OUT FRAMEWORK_EFI_HII_HANDLE       *Handle
  )
{
  EFI_STATUS                 Status;
  HII_THUNK_PRIVATE_DATA *Private;
  EFI_TPL                    OldTpl;

  if (Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Packages == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  
  //
  // We use a simple Global variable to inform NewOrAddPackNotify()
  // that the package list registered here is already registered
  // in the HII Thunk Layer. So NewOrAddPackNotify () does not need to
  // call registered the Package List again.
  //
  mInFrameworkHiiNewPack = TRUE;

  Private = HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  Status = UefiRegisterPackageList (
              This,
              Private,
              Packages,
              Handle
            );

  mInFrameworkHiiNewPack = FALSE;

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**

  Remove a package from the HII database.

  @param This      Pointer of Frameowk HII protocol instance.
  @param Handle    Handle value to be removed.

  @retval EFI_SUCCESS           Pacakges has added to HII database successfully.
  @retval EFI_INVALID_PARAMETER If Handle or Packages is NULL.

**/
EFI_STATUS
EFIAPI
HiiRemovePack (
  IN EFI_HII_PROTOCOL               *This,
  IN FRAMEWORK_EFI_HII_HANDLE       Handle
  )
{
  EFI_STATUS                 Status;
  HII_THUNK_PRIVATE_DATA     *Private;
  HII_THUNK_CONTEXT          *ThunkContext;
  EFI_TPL                    OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  mInFrameworkHiiRemovePack = TRUE;

  Private = HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  ThunkContext = FwHiiHandleToThunkContext (Private, Handle);

  if (ThunkContext != NULL) {
    Status = mHiiDatabase->RemovePackageList (
                                          mHiiDatabase,
                                          ThunkContext->UefiHiiHandle
                                          );
    ASSERT_EFI_ERROR (Status);

    if (ThunkContext->IfrPackageCount != 0) {
      UninstallDefaultConfigAccessProtocol (ThunkContext);
    }

    DestroyThunkContext (ThunkContext);
  }else {
    Status = EFI_NOT_FOUND;
  }

  mInFrameworkHiiRemovePack = FALSE;
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  This notification function will be called when a Package List is registered
  using UEFI HII interface. The Package List registered need to be recorded in
  Framework Thunk module as Thunk Module may need to look for String Package in
  the package registered.

  If the Package List registered is not either Sting Package or IFR package, 
  then ASSERT. If the NotifyType is not ADD_PACK or NEW_PACK, then ASSERT.
  Both cases means UEFI HII Database itself is buggy. 

  @param PackageType The Package Type.
  @param PackageGuid The Package GUID.
  @param Package     The Package Header.
  @param Handle      The HII Handle of this Package List.
  @param NotifyType  The reason of the notification. 

  @retval EFI_SUCCESS The notification function is successful.
  
**/
EFI_STATUS
EFIAPI
NewOrAddPackNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
{
  EFI_STATUS              Status;
  HII_THUNK_PRIVATE_DATA  *Private;
  HII_THUNK_CONTEXT       *ThunkContext;

  ASSERT (PackageType == EFI_HII_PACKAGE_STRINGS || PackageType == EFI_HII_PACKAGE_FORMS);
  ASSERT (NotifyType == EFI_HII_DATABASE_NOTIFY_ADD_PACK || NotifyType == EFI_HII_DATABASE_NOTIFY_NEW_PACK);

  Status  = EFI_SUCCESS;
  Private = mHiiThunkPrivateData;

  if (mInFrameworkHiiNewPack || mInFrameworkUpdatePakcage) {
    return EFI_SUCCESS;
  }

  //
  // We will create a ThunkContext to log the package list only if the
  // package is not registered with by Framework HII Thunk module yet.
  //
  ThunkContext = UefiHiiHandleToThunkContext (Private, Handle);
  if (ThunkContext == NULL) {
    ThunkContext = CreateThunkContextForUefiHiiHandle (Handle);
    ASSERT (ThunkContext != NULL);

    InsertTailList (&Private->ThunkContextListHead, &ThunkContext->Link);
  } 

  if (PackageType == EFI_HII_PACKAGE_FORMS) {
    if (ThunkContext->FormSet != NULL) {
      DestroyFormSet (ThunkContext->FormSet);
    }

    //
    // Reparse the FormSet.
    //
    ThunkContext->FormSet = ParseFormSet (ThunkContext->UefiHiiHandle);
  }

  return Status;  
}

/**
  This notification function will be called when a Package List is removed
  using UEFI HII interface. The Package List removed need to be removed from
  Framework Thunk module too.

  If the Package List registered is not Sting Package, 
  then ASSERT. If the NotifyType is not REMOVE_PACK, then ASSERT.
  Both cases means UEFI HII Database itself is buggy. 

  @param PackageType The Package Type.
  @param PackageGuid The Package GUID.
  @param Package     The Package Header.
  @param Handle      The HII Handle of this Package List.
  @param NotifyType  The reason of the notification. 

  @retval EFI_SUCCESS The notification function is successful.
  
**/
EFI_STATUS
EFIAPI
RemovePackNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
{
  EFI_STATUS                  Status;
  HII_THUNK_PRIVATE_DATA      *Private;
  HII_THUNK_CONTEXT           *ThunkContext;
  EFI_HII_PACKAGE_LIST_HEADER *HiiPackageList;
  UINTN                        BufferSize;

  Status = EFI_SUCCESS;

  ASSERT (PackageType == EFI_HII_PACKAGE_STRINGS);
  ASSERT (NotifyType == EFI_HII_DATABASE_NOTIFY_REMOVE_PACK);

  if (mInFrameworkHiiRemovePack || mInFrameworkUpdatePakcage) {
    return EFI_SUCCESS;
  }

  Private = mHiiThunkPrivateData;

  ThunkContext = UefiHiiHandleToThunkContext (Private, Handle);

  //
  // BugBug: Change to ASSERT if HII Database fix the bug and to also invoke 
  // NEW_PACK_NOTIFY for package (String Package) created internally.
  //
  if (ThunkContext != NULL) {
    if (!ThunkContext->ByFrameworkHiiNewPack) {
      HiiPackageList = NULL;
      Status = ExportPackageLists (Handle, &HiiPackageList, &BufferSize);
      ASSERT_EFI_ERROR (Status);
      if (HiiPackageList == NULL) {
        return EFI_NOT_FOUND;
      }

      if (GetPackageCountByType (HiiPackageList, EFI_HII_PACKAGE_STRINGS) == 1) {
        //
        // If the string package will be removed is the last string package
        // in the package list, we will remove the HII Thunk entry from the
        // database.
        //
        DestroyThunkContextForUefiHiiHandle (Private, Handle);
      }

      FreePool (HiiPackageList);
    }
  }

  
  return Status;
}



