/*++
Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  IfrCommon.c

Abstract:

  Common Library Routines to assist in IFR creation on-the-fly

--*/

#include "IfrLibrary.h"

EFI_STATUS
GetCurrentLanguage (
  OUT     CHAR16              *Lang
  )
/*++

Routine Description:

  Determine what is the current language setting
  
Arguments:
  
  Lang      - Pointer of system language
  
Returns: 

  Status code

--*/
{
  EFI_STATUS  Status;
  UINTN       Size;
  UINTN       Index;
  CHAR8       Language[4];

  //
  // Getting the system language and placing it into our Global Data
  //
  Size = sizeof (Language);

  Status = gRT->GetVariable (
                  L"Lang",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &Size,
                  Language
                  );

  if (EFI_ERROR (Status)) {
    EfiAsciiStrCpy (Language, (CHAR8 *) "eng");
  }

  for (Index = 0; Index < 3; Index++) {
    //
    // Bitwise AND ascii value with 0xDF yields an uppercase value.
    // Sign extend into a unicode value
    //
    Lang[Index] = (CHAR16) (Language[Index] & 0xDF);
  }

  //
  // Null-terminate the value
  //
  Lang[3] = (CHAR16) 0;

  return Status;
}


#ifdef SUPPORT_DEPRECATED_IFRSUPPORTLIB_API
EFI_STATUS
AddString (
  IN      VOID                *StringBuffer,
  IN      CHAR16              *Language,
  IN      CHAR16              *String,
  IN OUT  STRING_REF          *StringToken
  )
/*++

Routine Description:

  Add a string to the incoming buffer and return the token and offset data
  
Arguments:
  
  StringBuffer      - The incoming buffer
  
  Language          - Currrent language
  
  String            - The string to be added
  
  StringToken       - The index where the string placed
  
Returns: 

  EFI_OUT_OF_RESOURCES    - No enough buffer to allocate
  
  EFI_SUCCESS             - String successfully added to the incoming buffer

--*/
{
  EFI_HII_STRING_PACK *StringPack;
  EFI_HII_STRING_PACK *StringPackBuffer;
  VOID                *NewBuffer;
  RELOFST             *PackSource;
  RELOFST             *PackDestination;
  UINT8               *Source;
  UINT8               *Destination;
  UINTN               Index;
  BOOLEAN             Finished;
  UINTN               SizeofLanguage;
  UINTN               SizeofString;

  StringPack  = (EFI_HII_STRING_PACK *) StringBuffer;
  Finished    = FALSE;

  //
  // Pre-allocate a buffer sufficient for us to work on.
  // We will use it as a destination scratch pad to build data on
  // and when complete shift the data back to the original buffer
  //
  NewBuffer = EfiLibAllocateZeroPool (DEFAULT_STRING_BUFFER_SIZE);
  if (NewBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StringPackBuffer = (EFI_HII_STRING_PACK *) NewBuffer;

  //
  // StringPack is terminated with a length 0 entry
  //
  for (; StringPack->Header.Length != 0;) {
    //
    // If this stringpack's language is same as CurrentLanguage, use it
    //
    if (EfiCompareMem ((VOID *) ((CHAR8 *) (StringPack) + StringPack->LanguageNameString), Language, 3) == 0) {
      //
      // We have some data in this string pack, copy the string package up to the string data
      //
      EfiCopyMem (&StringPackBuffer->Header, &StringPack->Header, sizeof (StringPack));

      //
      // These are references in the structure to tokens, need to increase them by the space occupied by an additional StringPointer
      //
      StringPackBuffer->LanguageNameString = (UINT16) (StringPackBuffer->LanguageNameString + (UINT16) sizeof (RELOFST));
      StringPackBuffer->PrintableLanguageName = (UINT16) (StringPackBuffer->PrintableLanguageName + (UINT16) sizeof (RELOFST));

      PackSource      = (RELOFST *) (StringPack + 1);
      PackDestination = (RELOFST *) (StringPackBuffer + 1);
      for (Index = 0; PackSource[Index] != 0x0000; Index++) {
        //
        // Copy the stringpointers from old to new buffer
        // remember that we are adding a string, so the string offsets will all go up by sizeof (RELOFST)
        //
        PackDestination[Index] = (UINT16) (PackDestination[Index] + sizeof (RELOFST));
      }
      
      //
      // Add a new stringpointer in the new buffer since we are adding a string.  Null terminate it
      //
      PackDestination[Index] = (UINT16)(PackDestination[Index-1] + 
                                        EfiStrSize((CHAR16 *)((CHAR8 *)(StringPack) + PackSource[Index-1])));
      PackDestination[Index + 1] = (UINT16) 0;

      //
      // Index is the token value for the new string
      //
      *StringToken = (UINT16) Index;

      //
      // Source now points to the beginning of the old buffer strings
      // Destination now points to the beginning of the new buffer strings
      //
      Source      = (UINT8 *) &PackSource[Index + 1];
      Destination = (UINT8 *) &PackDestination[Index + 2];

      //
      // This should copy all the strings from the old buffer to the new buffer
      //
      for (; Index != 0; Index--) {
        //
        // Copy Source string to destination buffer
        //
        EfiStrCpy ((CHAR16 *) Destination, (CHAR16 *) Source);

        //
        // Adjust the source/destination to the next string location
        //
        Destination = Destination + EfiStrSize ((CHAR16 *) Source);
        Source      = Source + EfiStrSize ((CHAR16 *) Source);
      }
      
      //
      // This copies the new string to the destination buffer
      //
      EfiStrCpy ((CHAR16 *) Destination, (CHAR16 *) String);

      //
      // Adjust the size of the changed string pack by adding the size of the new string
      // along with the size of the additional offset entry for the new string
      //
      StringPackBuffer->Header.Length = (UINT32) ((UINTN) StringPackBuffer->Header.Length + EfiStrSize (String) + sizeof (RELOFST));

      //
      // Advance the buffers to point to the next spots.
      //
      StringPackBuffer  = (EFI_HII_STRING_PACK *) ((CHAR8 *) (StringPackBuffer) + StringPackBuffer->Header.Length);
      StringPack        = (EFI_HII_STRING_PACK *) ((CHAR8 *) (StringPack) + StringPack->Header.Length);
      Finished          = TRUE;
      continue;
    }
    //
    // This isn't the language of the stringpack we were asked to add a string to
    // so we need to copy it to the new buffer.
    //
    EfiCopyMem (&StringPackBuffer->Header, &StringPack->Header, StringPack->Header.Length);

    //
    // Advance the buffers to point to the next spots.
    //
    StringPackBuffer  = (EFI_HII_STRING_PACK *) ((CHAR8 *) (StringPackBuffer) + StringPack->Header.Length);
    StringPack        = (EFI_HII_STRING_PACK *) ((CHAR8 *) (StringPack) + StringPack->Header.Length);
  }
  
  //
  // If we didn't copy the new data to a stringpack yet
  //
  if (!Finished) {
    PackDestination = (RELOFST *) (StringPackBuffer + 1);
    //
    // Pointing to a new string pack location
    //
    SizeofLanguage = EfiStrSize (Language);
    SizeofString   = EfiStrSize (String);
    StringPackBuffer->Header.Length = (UINT32)
      (
        sizeof (EFI_HII_STRING_PACK) -
        sizeof (EFI_STRING) +
        sizeof (RELOFST) +
        sizeof (RELOFST) +
        SizeofLanguage +
        SizeofString
      );
    StringPackBuffer->Header.Type           = EFI_HII_STRING;
    StringPackBuffer->LanguageNameString    = (UINT16) ((UINTN) &PackDestination[3] - (UINTN) StringPackBuffer);
    StringPackBuffer->PrintableLanguageName = (UINT16) ((UINTN) &PackDestination[3] - (UINTN) StringPackBuffer);
    StringPackBuffer->Attributes            = 0;
    PackDestination[0]                      = (UINT16) ((UINTN) &PackDestination[3] - (UINTN) StringPackBuffer);
    PackDestination[1]                      = (UINT16) (PackDestination[0] + EfiStrSize (Language));
    PackDestination[2]                      = (UINT16) 0;

    //
    // The first string location will be set to destination.  The minimum number of strings
    // associated with a stringpack will always be token 0 stored as the languagename (e.g. ENG, SPA, etc)
    // and token 1 as the new string being added and and null entry for the stringpointers
    //
    Destination = (CHAR8 *) &PackDestination[3];

    //
    // Copy the language name string to the new buffer
    //
    EfiStrCpy ((CHAR16 *) Destination, Language);

    //
    // Advance the destination to the new empty spot
    //
    Destination = Destination + EfiStrSize (Language);

    //
    // Copy the string to the new buffer
    //
    EfiStrCpy ((CHAR16 *) Destination, String);

    //
    // Since we are starting with a new string pack - we know the new string is token 1
    //
    *StringToken = (UINT16) 1;
  }

  //
  // Zero out the original buffer and copy the updated data in the new buffer to the old buffer
  //
  EfiZeroMem (StringBuffer, DEFAULT_STRING_BUFFER_SIZE);
  EfiCopyMem (StringBuffer, NewBuffer, DEFAULT_STRING_BUFFER_SIZE);

  //
  // Free the newly created buffer since we don't need it anymore
  //
  gBS->FreePool (NewBuffer);
  return EFI_SUCCESS;
}


EFI_STATUS
AddOpCode (
  IN      VOID                *FormBuffer,
  IN OUT  VOID                *OpCodeData
  )
/*++

Routine Description:

  Add op-code data to the FormBuffer
  
Arguments:
  
  FormBuffer      - Form buffer to be inserted to
  
  OpCodeData      - Op-code data to be inserted
  
Returns: 

  EFI_OUT_OF_RESOURCES    - No enough buffer to allocate
  
  EFI_SUCCESS             - Op-code data successfully inserted

--*/
{
  EFI_HII_PACK_HEADER *NewBuffer;
  UINT8               *Source;
  UINT8               *Destination;

  //
  // Pre-allocate a buffer sufficient for us to work on.
  // We will use it as a destination scratch pad to build data on
  // and when complete shift the data back to the original buffer
  //
  NewBuffer = EfiLibAllocateZeroPool (DEFAULT_FORM_BUFFER_SIZE);
  if (NewBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Source      = (UINT8 *) FormBuffer;
  Destination = (UINT8 *) NewBuffer;

  //
  // Copy the IFR Package header to the new buffer
  //
  EfiCopyMem (Destination, Source, sizeof (EFI_HII_PACK_HEADER));

  //
  // Advance Source and Destination to next op-code
  //
  Source      = Source + sizeof (EFI_HII_PACK_HEADER);
  Destination = Destination + sizeof (EFI_HII_PACK_HEADER);

  //
  // Copy data to the new buffer until we run into the end_form
  //
  for (; ((EFI_IFR_OP_HEADER *) Source)->OpCode != EFI_IFR_END_FORM_OP;) {
    //
    // If the this opcode is an end_form_set we better be creating and endform
    // Nonetheless, we will add data before the end_form_set.  This also provides
    // for interesting behavior in the code we will run, but has no bad side-effects
    // since we will possibly do a 0 byte copy in this particular end-case.
    //
    if (((EFI_IFR_OP_HEADER *) Source)->OpCode == EFI_IFR_END_FORM_SET_OP) {
      break;
    }

    //
    // Copy data to new buffer
    //
    EfiCopyMem (Destination, Source, ((EFI_IFR_OP_HEADER *) Source)->Length);

    //
    // Adjust Source/Destination to next op-code location
    //
    Destination = Destination + (UINTN) ((EFI_IFR_OP_HEADER *) Source)->Length;
    Source      = Source + (UINTN) ((EFI_IFR_OP_HEADER *) Source)->Length;
  }

  //
  // Prior to the end_form is where we insert the new op-code data
  //
  EfiCopyMem (Destination, OpCodeData, ((EFI_IFR_OP_HEADER *) OpCodeData)->Length);
  Destination       = Destination + (UINTN) ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;

  NewBuffer->Length = (UINT32) (NewBuffer->Length + (UINT32) (((EFI_IFR_OP_HEADER *) OpCodeData)->Length));

  //
  // Copy end-form data to new buffer
  //
  EfiCopyMem (Destination, Source, ((EFI_IFR_OP_HEADER *) Source)->Length);

  //
  // Adjust Source/Destination to next op-code location
  //
  Destination = Destination + (UINTN) ((EFI_IFR_OP_HEADER *) Source)->Length;
  Source      = Source + (UINTN) ((EFI_IFR_OP_HEADER *) Source)->Length;

  //
  // Copy end-formset data to new buffer
  //
  EfiCopyMem (Destination, Source, ((EFI_IFR_OP_HEADER *) Source)->Length);

  //
  // Zero out the original buffer and copy the updated data in the new buffer to the old buffer
  //
  EfiZeroMem (FormBuffer, DEFAULT_FORM_BUFFER_SIZE);
  EfiCopyMem (FormBuffer, NewBuffer, DEFAULT_FORM_BUFFER_SIZE);

  //
  // Free the newly created buffer since we don't need it anymore
  //
  gBS->FreePool (NewBuffer);
  return EFI_SUCCESS;
}
#endif


EFI_STATUS
GetHiiInterface (
  OUT     EFI_HII_PROTOCOL    **Hii
  )
/*++

Routine Description:

  Get the HII protocol interface
  
Arguments:
  
  Hii     - HII protocol interface
  
Returns: 

  Status code

--*/
{
  EFI_STATUS  Status;

  //
  // There should only be one HII protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiProtocolGuid,
                  NULL,
                  (VOID **) Hii
                  );

  return Status;;
}


EFI_STATUS
ExtractDataFromHiiHandle (
  IN      EFI_HII_HANDLE      HiiHandle,
  IN OUT  UINT16              *ImageLength,
  OUT     UINT8               *DefaultImage,
  OUT     EFI_GUID            *Guid
  )
/*++

Routine Description:

  Extract information pertaining to the HiiHandle
  
Arguments:
  
  HiiHandle       - Hii handle
  
  ImageLength     - For input, length of DefaultImage;
                    For output, length of actually required
                    
  DefaultImage    - Image buffer prepared by caller
  
  Guid            - Guid information about the form
  
Returns: 

  EFI_OUT_OF_RESOURCES    - No enough buffer to allocate
  
  EFI_BUFFER_TOO_SMALL    - DefualtImage has no enough ImageLength
  
  EFI_SUCCESS             - Successfully extract data from Hii database.
  
  
--*/
{
  EFI_STATUS        Status;
  EFI_HII_PROTOCOL  *Hii;
  UINTN             DataLength;
  UINT8             *RawData;
  UINT8             *OldData;
  UINTN             Index;
  UINTN             Temp;
  UINTN             SizeOfNvStore;
  UINTN             CachedStart;

  DataLength    = DEFAULT_FORM_BUFFER_SIZE;
  SizeOfNvStore = 0;
  CachedStart   = 0;

  Status        = GetHiiInterface (&Hii);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate space for retrieval of IFR data
  //
  RawData = EfiLibAllocateZeroPool ((UINTN) DataLength);
  if (RawData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get all the forms associated with this HiiHandle
  //
  Status = Hii->GetForms (Hii, HiiHandle, 0, &DataLength, RawData);

  if (EFI_ERROR (Status)) {
    gBS->FreePool (RawData);

    //
    // Allocate space for retrieval of IFR data
    //
    RawData = EfiLibAllocateZeroPool ((UINTN) DataLength);
    if (RawData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Get all the forms associated with this HiiHandle
    //
    Status = Hii->GetForms (Hii, HiiHandle, 0, &DataLength, RawData);
  }

  OldData = RawData;

  //
  // Point RawData to the beginning of the form data
  //
  RawData = (UINT8 *) ((UINTN) RawData + sizeof (EFI_HII_PACK_HEADER));

  for (Index = 0; RawData[Index] != EFI_IFR_END_FORM_SET_OP;) {
    switch (RawData[Index]) {
    case EFI_IFR_FORM_SET_OP:
      //
      // Copy the GUID information from this handle
      //
      EfiCopyMem (Guid, &((EFI_IFR_FORM_SET *) &RawData[Index])->Guid, sizeof (EFI_GUID));
      break;

    case EFI_IFR_ONE_OF_OP:
    case EFI_IFR_CHECKBOX_OP:
    case EFI_IFR_NUMERIC_OP:
    case EFI_IFR_DATE_OP:
    case EFI_IFR_TIME_OP:
    case EFI_IFR_PASSWORD_OP:
    case EFI_IFR_STRING_OP:
      //
      // Remember, multiple op-codes may reference the same item, so let's keep a running
      // marker of what the highest QuestionId that wasn't zero length.  This will accurately
      // maintain the Size of the NvStore
      //
      if (((EFI_IFR_ONE_OF *) &RawData[Index])->Width != 0) {
        Temp = ((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId + ((EFI_IFR_ONE_OF *) &RawData[Index])->Width;
        if (SizeOfNvStore < Temp) {
          SizeOfNvStore = ((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId + ((EFI_IFR_ONE_OF *) &RawData[Index])->Width;
        }
      }
    }

    Index = RawData[Index + 1] + Index;
  }
    
  //
  // Return an error if buffer is too small
  //
  if (SizeOfNvStore > *ImageLength || DefaultImage == NULL) {
    gBS->FreePool (OldData);
    *ImageLength = (UINT16) SizeOfNvStore;
    return EFI_BUFFER_TOO_SMALL;
  }

  EfiZeroMem (DefaultImage, SizeOfNvStore);

  //
  // Copy the default image information to the user's buffer
  //
  for (Index = 0; RawData[Index] != EFI_IFR_END_FORM_SET_OP;) {
    switch (RawData[Index]) {
    case EFI_IFR_ONE_OF_OP:
      CachedStart = ((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId;
      break;

    case EFI_IFR_ONE_OF_OPTION_OP:
      if (((EFI_IFR_ONE_OF_OPTION *) &RawData[Index])->Flags & EFI_IFR_FLAG_DEFAULT) {
        EfiCopyMem (&DefaultImage[CachedStart], &((EFI_IFR_ONE_OF_OPTION *) &RawData[Index])->Value, 2);
      }
      break;

    case EFI_IFR_CHECKBOX_OP:
      DefaultImage[((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId] = ((EFI_IFR_CHECK_BOX *) &RawData[Index])->Flags;
      break;

    case EFI_IFR_NUMERIC_OP:
      EfiCopyMem (
        &DefaultImage[((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId],
        &((EFI_IFR_NUMERIC *) &RawData[Index])->Default,
        2
        );
      break;

    }

    Index = RawData[Index + 1] + Index;
  }

  *ImageLength = (UINT16) SizeOfNvStore;

  //
  // Free our temporary repository of form data
  //
  gBS->FreePool (OldData);

  return EFI_SUCCESS;
}


EFI_HII_HANDLE
FindHiiHandle (
  IN OUT EFI_HII_PROTOCOL    **HiiProtocol, OPTIONAL
  IN     EFI_GUID            *Guid
  )
/*++

Routine Description:
  Finds HII handle for given pack GUID previously registered with the HII.

Arguments:
  HiiProtocol - pointer to pointer to HII protocol interface. 
                If NULL, the interface will be found but not returned.
                If it points to NULL, the interface will be found and 
                written back to the pointer that is pointed to.
  Guid        - The GUID of the pack that registered with the HII.

Returns:
  Handle to the HII pack previously registered by the memory driver.

--*/
{
  EFI_STATUS        Status;

  EFI_HII_HANDLE    *HiiHandleBuffer;
  EFI_HII_HANDLE    HiiHandle;
  UINT16            HiiHandleBufferLength;
  UINT32            NumberOfHiiHandles;
  EFI_GUID          HiiGuid;
  EFI_HII_PROTOCOL  *HiiProt;
  UINT32            Index;
  UINT16            Length;

  HiiHandle = 0;
  if ((HiiProtocol != NULL) && (*HiiProtocol != NULL)) {
    //
    // The protocol has been passed in
    //
    HiiProt = *HiiProtocol;
  } else {
    gBS->LocateProtocol (
          &gEfiHiiProtocolGuid,
          NULL,
          (VOID **) &HiiProt
          );
    if (HiiProt == NULL) {
      return HiiHandle;
    }

    if (HiiProtocol != NULL) {
      //
      // Return back the HII protocol for the caller as promissed
      //
      *HiiProtocol = HiiProt;
    }
  }
  //
  // Allocate buffer
  //
  HiiHandleBufferLength = 10;
  HiiHandleBuffer       = EfiLibAllocatePool (HiiHandleBufferLength);
  ASSERT (HiiHandleBuffer != NULL);

  //
  // Get the Handles of the packages that were registered with Hii
  //
  Status = HiiProt->FindHandles (
                      HiiProt,
                      &HiiHandleBufferLength,
                      HiiHandleBuffer
                      );

  //
  // Get a bigger bugffer if this one is to small, and try again
  //
  if (Status == EFI_BUFFER_TOO_SMALL) {

    gBS->FreePool (HiiHandleBuffer);

    HiiHandleBuffer = EfiLibAllocatePool (HiiHandleBufferLength);
    ASSERT (HiiHandleBuffer != NULL);

    Status = HiiProt->FindHandles (
                        HiiProt,
                        &HiiHandleBufferLength,
                        HiiHandleBuffer
                        );
  }

  if (EFI_ERROR (Status)) {
    goto lbl_exit;
  }

  NumberOfHiiHandles = HiiHandleBufferLength / sizeof (EFI_HII_HANDLE);

  //
  // Iterate Hii handles and look for the one that matches our Guid
  //
  for (Index = 0; Index < NumberOfHiiHandles; Index++) {

    Length = 0;
    ExtractDataFromHiiHandle (HiiHandleBuffer[Index], &Length, NULL, &HiiGuid);

    if (EfiCompareGuid (&HiiGuid, Guid)) {

      HiiHandle = HiiHandleBuffer[Index];
      break;
    }
  }

lbl_exit:
  gBS->FreePool (HiiHandleBuffer);
  return HiiHandle;
}

#ifdef SUPPORT_DEPRECATED_IFRSUPPORTLIB_API
EFI_STATUS
ValidateDataFromHiiHandle (
  IN      EFI_HII_HANDLE      HiiHandle,
  OUT     BOOLEAN             *Results
  )
/*++

Routine Description:

  Validate that the data associated with the HiiHandle in NVRAM is within
  the reasonable parameters for that FormSet.  Values for strings and passwords
  are not verified due to their not having the equivalent of valid range settings.
  
Arguments:

  HiiHandle -   Handle of the HII database entry to query

  Results -     If return Status is EFI_SUCCESS, Results provides valid data
                TRUE  = NVRAM Data is within parameters
                FALSE = NVRAM Data is NOT within parameters
  
Returns: 

  EFI_OUT_OF_RESOURCES      - No enough buffer to allocate
  
  EFI_SUCCESS               - Data successfully validated
--*/
{
  EFI_STATUS        Status;
  EFI_HII_PROTOCOL  *Hii;
  EFI_GUID          Guid;
  UINT8             *RawData;
  UINT8             *OldData;
  UINTN             RawDataLength;
  UINT8             *VariableData;
  UINTN             Index;
  UINTN             Temp;
  UINTN             SizeOfNvStore;
  UINTN             CachedStart;
  BOOLEAN           GotMatch;

  RawDataLength = DEFAULT_FORM_BUFFER_SIZE;
  SizeOfNvStore = 0;
  CachedStart   = 0;
  GotMatch      = FALSE;
  *Results      = TRUE;

  Status        = GetHiiInterface (&Hii);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate space for retrieval of IFR data
  //
  RawData = EfiLibAllocateZeroPool (RawDataLength);
  if (RawData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get all the forms associated with this HiiHandle
  //
  Status = Hii->GetForms (Hii, HiiHandle, 0, &RawDataLength, RawData);

  if (EFI_ERROR (Status)) {
    gBS->FreePool (RawData);

    //
    // Allocate space for retrieval of IFR data
    //
    RawData = EfiLibAllocateZeroPool (RawDataLength);
    if (RawData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Get all the forms associated with this HiiHandle
    //
    Status = Hii->GetForms (Hii, HiiHandle, 0, &RawDataLength, RawData);
  }

  OldData = RawData;

  //
  // Point RawData to the beginning of the form data
  //
  RawData = (UINT8 *) ((UINTN) RawData + sizeof (EFI_HII_PACK_HEADER));

  for (Index = 0; RawData[Index] != EFI_IFR_END_FORM_SET_OP;) {
    if (RawData[Index] == EFI_IFR_FORM_SET_OP) {
      EfiCopyMem (&Guid, &((EFI_IFR_FORM_SET *) &RawData[Index])->Guid, sizeof (EFI_GUID));
      break;
    }

    Index = RawData[Index + 1] + Index;
  }

  for (Index = 0; RawData[Index] != EFI_IFR_END_FORM_SET_OP;) {
    switch (RawData[Index]) {
    case EFI_IFR_FORM_SET_OP:
      break;

    case EFI_IFR_ONE_OF_OP:
    case EFI_IFR_CHECKBOX_OP:
    case EFI_IFR_NUMERIC_OP:
    case EFI_IFR_DATE_OP:
    case EFI_IFR_TIME_OP:
    case EFI_IFR_PASSWORD_OP:
    case EFI_IFR_STRING_OP:
      //
      // Remember, multiple op-codes may reference the same item, so let's keep a running
      // marker of what the highest QuestionId that wasn't zero length.  This will accurately
      // maintain the Size of the NvStore
      //
      if (((EFI_IFR_ONE_OF *) &RawData[Index])->Width != 0) {
        Temp = ((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId + ((EFI_IFR_ONE_OF *) &RawData[Index])->Width;
        if (SizeOfNvStore < Temp) {
          SizeOfNvStore = ((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId + ((EFI_IFR_ONE_OF *) &RawData[Index])->Width;
        }
      }
    }

    Index = RawData[Index + 1] + Index;
  }
    
  //
  // Allocate memory for our File Form Tags
  //
  VariableData = EfiLibAllocateZeroPool (SizeOfNvStore);
  if (VariableData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gRT->GetVariable (
                  L"Setup",
                  &Guid,
                  NULL,
                  &SizeOfNvStore,
                  (VOID *) VariableData
                  );

  if (EFI_ERROR (Status)) {

    //
    // If there is a variable that exists already and it is larger than what we calculated the
    // storage needs to be, we must assume the variable size from GetVariable is correct and not
    // allow the truncation of the variable.  It is very possible that the user who created the IFR
    // we are cracking is not referring to a variable that was in a previous map, however we cannot
    // allow it's truncation.
    //
    if (Status == EFI_BUFFER_TOO_SMALL) {
      //
      // Free the buffer that was allocated that was too small
      //
      gBS->FreePool (VariableData);

      VariableData = EfiLibAllocatePool (SizeOfNvStore);
      if (VariableData == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Status = gRT->GetVariable (
                      L"Setup",
                      &Guid,
                      NULL,
                      &SizeOfNvStore,
                      (VOID *) VariableData
                      );
    }
  }

  //
  // Walk through the form and see that the variable data it refers to is ok.
  // This allows for the possibility of stale (obsoleted) data in the variable
  // can be overlooked without causing an error
  //
  for (Index = 0; RawData[Index] != EFI_IFR_END_FORM_SET_OP;) {
    switch (RawData[Index]) {
    case EFI_IFR_ONE_OF_OP:
      //
      // A one_of has no data, its the option that does - cache the storage Id
      //
      CachedStart = ((EFI_IFR_ONE_OF *) &RawData[Index])->QuestionId;
      break;

    case EFI_IFR_ONE_OF_OPTION_OP:
      //
      // A one_of_option can be any value
      //
      if (VariableData[CachedStart] == ((EFI_IFR_ONE_OF_OPTION *) &RawData[Index])->Value) {
        GotMatch = TRUE;
      }
      break;

    case EFI_IFR_END_ONE_OF_OP:
      //
      // At this point lets make sure that the data value in the NVRAM matches one of the options
      //
      if (!GotMatch) {
        *Results = FALSE;
        return EFI_SUCCESS;
      }
      break;

    case EFI_IFR_CHECKBOX_OP:
      //
      // A checkbox is a boolean, so 0 and 1 are valid
      // Remember, QuestionId corresponds to the offset location of the data in the variable
      //
      if (VariableData[((EFI_IFR_CHECK_BOX *) &RawData[Index])->QuestionId] > 1) {
        *Results = FALSE;
        return EFI_SUCCESS;
      }
      break;

    case EFI_IFR_NUMERIC_OP:
        if ((VariableData[((EFI_IFR_NUMERIC *)&RawData[Index])->QuestionId] < ((EFI_IFR_NUMERIC *)&RawData[Index])->Minimum) ||
            (VariableData[((EFI_IFR_NUMERIC *)&RawData[Index])->QuestionId] > ((EFI_IFR_NUMERIC *)&RawData[Index])->Maximum)) {
        *Results = FALSE;
        return EFI_SUCCESS;
      }
      break;

    }

    Index = RawData[Index + 1] + Index;
  }

  //
  // Free our temporary repository of form data
  //
  gBS->FreePool (OldData);
  gBS->FreePool (VariableData);

  return EFI_SUCCESS;
}
#endif

EFI_HII_PACKAGES *
PreparePackages (
  IN      UINTN               NumberOfPackages,
  IN      EFI_GUID            *GuidId,
  ...
  )
/*++

Routine Description:
  
  Assemble EFI_HII_PACKAGES according to the passed in packages.
  
Arguments:
  
  NumberOfPackages  -  Number of packages.
  GuidId            -  Package GUID.
  
Returns: 
  
  Pointer of EFI_HII_PACKAGES.
  
--*/
{
  VA_LIST           args;
  EFI_HII_PACKAGES  *HiiPackages;
  VOID              **Package;
  UINTN             Index;

  ASSERT (NumberOfPackages > 0);

  HiiPackages                   = EfiLibAllocateZeroPool (sizeof (EFI_HII_PACKAGES) + NumberOfPackages * sizeof (VOID *));

  HiiPackages->GuidId           = GuidId;
  HiiPackages->NumberOfPackages = NumberOfPackages;
  Package                       = (VOID **) (((UINT8 *) HiiPackages) + sizeof (EFI_HII_PACKAGES));

  VA_START (args, GuidId);

  for (Index = 0; Index < NumberOfPackages; Index++) {
    *Package = VA_ARG (args, VOID *);
    Package++;
  }

  VA_END (args);

  return HiiPackages;
}
