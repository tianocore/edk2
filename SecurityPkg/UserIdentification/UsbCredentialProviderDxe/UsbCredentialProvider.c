/** @file
  Usb Credential Provider driver implemenetation.
    
Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UsbCredentialProvider.h"

CREDENTIAL_TABLE            *mUsbTable       = NULL;
USB_PROVIDER_CALLBACK_INFO  *mCallbackInfo   = NULL;
USB_CREDENTIAL_INFO         *mUsbInfoHandle  = NULL;

EFI_USER_CREDENTIAL2_PROTOCOL  gUsbCredentialProviderDriver = {
  USB_CREDENTIAL_PROVIDER_GUID,
  EFI_USER_CREDENTIAL_CLASS_SECURE_CARD,
  CredentialEnroll,
  CredentialForm,
  CredentialTile,
  CredentialTitle,
  CredentialUser,
  CredentialSelect,
  CredentialDeselect,
  CredentialDefault,
  CredentialGetInfo,
  CredentialGetNextInfo,
  EFI_CREDENTIAL_CAPABILITIES_ENROLL,
  CredentialDelete
};


/**
  Get string by string id from HII Interface.


  @param[in] Id      String ID to get the string from.

  @retval  CHAR16 *  String from ID.
  @retval  NULL      If error occurs.

**/
CHAR16 *
GetStringById (
  IN EFI_STRING_ID             Id
  )
{
  //
  // Get the current string for the current Language
  //
  return HiiGetString (mCallbackInfo->HiiHandle, Id, NULL);
}


/**
  Expand password table size.

**/
VOID
ExpandTableSize (
  VOID
  )
{
  CREDENTIAL_TABLE  *NewTable;
  UINTN             Count;

  Count = mUsbTable->MaxCount + USB_TABLE_INC;
  //
  // Create new credential table.
  //
  NewTable = AllocateZeroPool (
               sizeof (CREDENTIAL_TABLE) - sizeof (USB_INFO) +
               Count * sizeof (USB_INFO)
               );
  ASSERT (NewTable != NULL); 

  NewTable->MaxCount = Count;
  NewTable->Count    = mUsbTable->Count;

  //
  // Copy old entries.
  //
  CopyMem (
    &NewTable->UserInfo, 
    &mUsbTable->UserInfo, 
    mUsbTable->Count * sizeof (USB_INFO)
    );
  FreePool (mUsbTable);
  mUsbTable = NewTable;
}


/**
  Add, update or delete info in table, and sync with NV variable.

  @param[in]  Index     The index of the password in table. If index is found in
                        table, update the info, else add the into to table. 
  @param[in]  Info      The new credential info to add into table. If Info is NULL, 
                        delete the info by Index.

  @retval EFI_INVALID_PARAMETER  Info is NULL when save the info.
  @retval EFI_SUCCESS            Modify the table successfully.
  @retval Others                 Failed to modify the table.

**/
EFI_STATUS
ModifyTable (
  IN  UINTN                                Index,
  IN  USB_INFO                             * Info OPTIONAL
  )
{
  EFI_STATUS  Status;
  USB_INFO    *NewUsbInfo;
  
  NewUsbInfo = NULL;
  if (Index < mUsbTable->Count) {
    if (Info == NULL) {
      //
      // Delete the specified entry.
      //
      mUsbTable->Count--;
      if (Index != mUsbTable->Count) {
        NewUsbInfo = &mUsbTable->UserInfo[mUsbTable->Count];
      } 
    } else {
      //
      // Update the specified entry.
      //
      NewUsbInfo = Info;
    }
  } else {
    //
    // Add a new entry
    //
    if (Info == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    if (mUsbTable->Count >= mUsbTable->MaxCount) {
      ExpandTableSize ();
    }

    NewUsbInfo = Info;
    mUsbTable->Count++;
  }

  if (NewUsbInfo != NULL) {
    CopyMem (&mUsbTable->UserInfo[Index], NewUsbInfo, sizeof (USB_INFO));
  }

  //
  // Save the credential table.
  //
  Status = gRT->SetVariable (
                  L"UsbCredential",
                  &gUsbCredentialProviderGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  mUsbTable->Count * sizeof (USB_INFO),
                  &mUsbTable->UserInfo
                  );
  return Status;
}


/**
  Create a credential table

  @retval EFI_SUCCESS      Create a credential table successfully.
  @retval Others           Failed to create a password.

**/
EFI_STATUS
InitCredentialTable (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       *Var;
  UINTN       VarSize;

  //
  // Get Usb credential data from NV variable.
  //
  VarSize = 0;
  Var     = NULL;
  Status  = gRT->GetVariable (
                   L"UsbCredential", 
                   &gUsbCredentialProviderGuid, 
                   NULL, 
                   &VarSize,
                   Var
                   );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Var = AllocateZeroPool (VarSize);
    if (Var == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Status = gRT->GetVariable (
                    L"UsbCredential", 
                    &gUsbCredentialProviderGuid, 
                    NULL, 
                    &VarSize,
                    Var
                    );
  }
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    return Status;
  }
  
  //
  // Init Usb credential table.
  //
  mUsbTable = AllocateZeroPool (
                sizeof (CREDENTIAL_TABLE) - sizeof (USB_INFO) +
                USB_TABLE_INC * sizeof (USB_INFO) + 
                VarSize
                );
  if (mUsbTable == NULL) {
    FreePool (Var);
    return EFI_OUT_OF_RESOURCES;
  }

  mUsbTable->Count      = VarSize / sizeof (USB_INFO);
  mUsbTable->MaxCount   = mUsbTable->Count + USB_TABLE_INC;
  if (Var != NULL) {
    CopyMem (mUsbTable->UserInfo, Var, VarSize);
    FreePool (Var);
  }
  return EFI_SUCCESS;
}


/**
  Read the specified file by FileName in the Usb key and return the file size in BufferSize
  and file content in Buffer.
  Note: the caller is responsible to free the buffer memory.

  @param  FileName               File to read.
  @param  Buffer                 Returned with data read from the file.
  @param  BufferSize             Size of the data buffer.

  @retval EFI_SUCCESS            The command completed successfully.
  @retval EFI_OUT_OF_RESOURCES   Resource allocation failed.
  @retval EFI_NOT_FOUND          File not found.
  @retval EFI_DEVICE_ERROR       Device I/O error.

**/
EFI_STATUS
GetFileData (
  IN     CHAR16                                 *FileName,
  OUT VOID                                      **Buffer,
  OUT UINTN                                     *BufferSize
  )
{
  EFI_STATUS                      Status;
  UINTN                           Index;
  UINTN                           HandleCount;
  UINTN                           ScratchBufferSize;
  EFI_HANDLE                      *HandleBuffer;
  EFI_FILE                        *RootFs;
  EFI_FILE                        *FileHandle;
  EFI_FILE_INFO                   *FileInfo;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;
  EFI_BLOCK_IO_PROTOCOL           *BlkIo;

  FileInfo      = NULL;
  FileHandle    = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Can not Locate SimpleFileSystemProtocol\n"));
    goto Done;
  }

  //
  // Find and open the file in removable media disk.
  //
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlkIo
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (BlkIo->Media->RemovableMedia) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiSimpleFileSystemProtocolGuid,
                      (VOID **) &SimpleFileSystem
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }
    
      Status = SimpleFileSystem->OpenVolume (
                                   SimpleFileSystem,
                                   &RootFs
                                   );
      if (EFI_ERROR (Status)) {
        continue;
      }
                                   
      Status = RootFs->Open (
                         RootFs,
                         &FileHandle,
                         FileName,
                         EFI_FILE_MODE_READ,
                         0
                         );
      if (!EFI_ERROR (Status)) {
        break;
      }      
    }
  }

  FreePool (HandleBuffer);

  if (Index >= HandleCount) {
    DEBUG ((DEBUG_ERROR, "Can not found the token file!\n"));
    Status = EFI_NOT_FOUND;
    goto Done;
  }
  
  //
  // Figure out how big the file is.
  //
  ScratchBufferSize = 0;
  Status = FileHandle->GetInfo (
                        FileHandle,
                        &gEfiFileInfoGuid,
                        &ScratchBufferSize,
                        NULL
                        );
  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    DEBUG ((DEBUG_ERROR, "Can not obtain file size info!\n"));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  FileInfo = AllocateZeroPool (ScratchBufferSize);                                    
  if (FileInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "Can not allocate enough memory for the token file!\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = FileHandle->GetInfo (
                        FileHandle,
                        &gEfiFileInfoGuid,
                        &ScratchBufferSize,
                        FileInfo
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Can not obtain file info from the token file!\n"));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }
  
  //
  // Allocate a buffer for the file.
  //
  *BufferSize = (UINT32) FileInfo->FileSize;
  *Buffer     = AllocateZeroPool (*BufferSize); 
  if (*Buffer == NULL) {
    DEBUG ((DEBUG_ERROR, "Can not allocate a buffer for the file!\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
  
  //
  // Load file into the allocated memory.
  //
  Status = FileHandle->Read (FileHandle, BufferSize, *Buffer);
  if (EFI_ERROR (Status)) {
    FreePool (*Buffer);
    DEBUG ((DEBUG_ERROR, "Can not read the token file!\n"));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }
  
  //
  // Close file.
  //
  Status = FileHandle->Close (FileHandle);
  if (EFI_ERROR (Status)) {
    FreePool (*Buffer);
    DEBUG ((DEBUG_ERROR, "Can not close the token file !\n"));
    Status = EFI_DEVICE_ERROR;
  }

Done:

  if (FileInfo != NULL) {
    FreePool (FileInfo);
  }

  return Status;
}


/**
  Hash the data to get credential.

  @param[in]   Buffer         Points to the data buffer 
  @param[in]   BufferSize     The size of data in buffer, in bytes.
  @param[out]  Credential     Points to the hashed result

  @retval      TRUE           Hash the data successfully.
  @retval      FALSE          Failed to hash the data.
                 
**/
BOOLEAN
GenerateCredential (
  IN      UINT8                               *Buffer,
  IN      UINTN                               BufferSize,
     OUT  UINT8                               *Credential
  )
{
  BOOLEAN           Status;
  UINTN             HashSize;
  VOID              *Hash;
  
  HashSize = Sha1GetContextSize ();
  Hash     = AllocatePool (HashSize);
  ASSERT (Hash != NULL);
  
  Status = Sha1Init (Hash);
  if (!Status) {
    goto Done;
  }
  
  Status = Sha1Update (Hash, Buffer, BufferSize);
  if (!Status) {
    goto Done;
  }
  
  Status = Sha1Final (Hash, Credential);
  
Done:
  FreePool (Hash);
  return Status;
}


/**
  Read the token file, and default the Token is saved at the begining of the file.

  @param[out]  Token            Token read from a Token file.

  @retval EFI_SUCCESS           Read a Token successfully.
  @retval Others                Fails to read a Token.
  
**/
EFI_STATUS
GetToken (
  OUT UINT8                                 *Token
  )
{
  EFI_STATUS  Status;
  UINT8       *Buffer;
  UINTN       BufSize;
  CHAR16      *TokenFile;

  BufSize = 0;
  Buffer  = NULL;
  TokenFile = FixedPcdGetPtr (PcdFixedUsbCredentialProviderTokenFileName);
  Status = GetFileData (TokenFile, (VOID *)&Buffer, &BufSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Read file %s from USB error! Status=(%r)\n", TokenFile, Status));
    return Status;
  }
  
  if (!GenerateCredential (Buffer, BufSize, Token)) {
    DEBUG ((DEBUG_ERROR, "Generate credential from read data failed!\n"));
    FreePool (Buffer);
    return EFI_SECURITY_VIOLATION;
  }
  
  FreePool (Buffer);  
  return EFI_SUCCESS;
}


/**
  Find a user infomation record by the information record type.

  This function searches all user information records of User from beginning 
  until either the information is found or there are no more user infomation
  record. A match occurs when a Info.InfoType field matches the user information
  record type.

  @param[in]     User      Points to the user profile record to search.                          
  @param[in]     InfoType  The infomation type to be searched.
  @param[out]    Info      Points to the user info found, the caller is responsible
                           to free.
  
  @retval EFI_SUCCESS      Find the user information successfully.
  @retval Others           Fail to find the user information.

**/
EFI_STATUS
FindUserInfoByType (
  IN      EFI_USER_PROFILE_HANDLE               User,
  IN      UINT8                                 InfoType,
  OUT     EFI_USER_INFO                         **Info
  )
{
  EFI_STATUS                 Status;
  EFI_USER_INFO              *UserInfo;
  UINTN                      UserInfoSize;
  EFI_USER_INFO_HANDLE       UserInfoHandle;
  EFI_USER_MANAGER_PROTOCOL  *UserManager;
  
  //
  // Find user information by information type.
  //
  if (Info == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol (
                  &gEfiUserManagerProtocolGuid,
                  NULL,
                  (VOID **) &UserManager
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get each user information.
  //

  UserInfoHandle = NULL;
  UserInfo       = NULL;
  UserInfoSize   = 0;
  while (TRUE) {
    Status = UserManager->GetNextInfo (UserManager, User, &UserInfoHandle);
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Get information.
    //
    Status = UserManager->GetInfo (
                            UserManager,
                            User,
                            UserInfoHandle,
                            UserInfo,
                            &UserInfoSize
                            );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      if (UserInfo != NULL) {
        FreePool (UserInfo);
      }
      UserInfo = AllocateZeroPool (UserInfoSize);
      if (UserInfo == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      Status = UserManager->GetInfo (
                              UserManager,
                              User,
                              UserInfoHandle,
                              UserInfo,
                              &UserInfoSize
                              );
    }
    if (EFI_ERROR (Status)) {
      break;
    }

    ASSERT (UserInfo != NULL);
    if (UserInfo->InfoType == InfoType) {
      *Info = UserInfo;
      return EFI_SUCCESS;
    }    
  }

  if (UserInfo != NULL) {
    FreePool (UserInfo);
  }
  return Status;
}


/**
  This function initialize the data mainly used in form browser.

  @retval EFI_SUCCESS          Initialize form data successfully.
  @retval Others               Fail to Initialize form data.

**/
EFI_STATUS
InitFormBrowser (
  VOID
  )
{
  USB_PROVIDER_CALLBACK_INFO  *CallbackInfo;
    
  //
  // Initialize driver private data.
  //
  CallbackInfo = AllocateZeroPool (sizeof (USB_PROVIDER_CALLBACK_INFO));
  if (CallbackInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
 
  CallbackInfo->DriverHandle  = NULL;

  //
  // Publish HII data.
  //
  CallbackInfo->HiiHandle = HiiAddPackages (
                              &gUsbCredentialProviderGuid,
                              CallbackInfo->DriverHandle,
                              UsbCredentialProviderStrings,
                              NULL
                              );
  if (CallbackInfo->HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  mCallbackInfo = CallbackInfo;

  return EFI_SUCCESS;
}


/**
  Enroll a user on a credential provider.

  This function enrolls a user on this credential provider. If the user exists on 
  this credential provider, update the user information on this credential provider; 
  otherwise add the user information on credential provider.
  
  @param[in] This                Points to this instance of EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[in] User                The user profile to enroll.
 
  @retval EFI_SUCCESS            User profile was successfully enrolled.
  @retval EFI_ACCESS_DENIED      Current user profile does not permit enrollment on the
                                 user profile handle. Either the user profile cannot enroll
                                 on any user profile or cannot enroll on a user profile 
                                 other than the current user profile.
  @retval EFI_UNSUPPORTED        This credential provider does not support enrollment in
                                 the pre-OS.
  @retval EFI_DEVICE_ERROR       The new credential could not be created because of a device
                                 error.
  @retval EFI_INVALID_PARAMETER  User does not refer to a valid user profile handle.
  
**/
EFI_STATUS
EFIAPI
CredentialEnroll (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  IN        EFI_USER_PROFILE_HANDLE             User
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  USB_INFO                  UsbInfo;
  EFI_USER_INFO             *UserInfo;
  EFI_INPUT_KEY             Key;
  UINT8                     *UserId;
  CHAR16                    *QuestionStr;
  CHAR16                    *PromptStr;

  if ((This == NULL) || (User == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
 
  //
  // Get User Identifier
  //
  UserInfo = NULL;
  Status = FindUserInfoByType (
             User,
             EFI_USER_INFO_IDENTIFIER_RECORD,
             &UserInfo
             );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (UsbInfo.UserId, (UINT8 *) (UserInfo + 1), sizeof (EFI_USER_INFO_IDENTIFIER)); 
  FreePool (UserInfo);
  
  //
  // Get Token and User ID to UsbInfo.
  //
  Status = GetToken (UsbInfo.Token);
  if (EFI_ERROR (Status)) {
    QuestionStr = GetStringById (STRING_TOKEN (STR_READ_USB_TOKEN_ERROR));
    PromptStr   = GetStringById (STRING_TOKEN (STR_INSERT_USB_TOKEN)); 
    CreatePopUp (
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      &Key,
      QuestionStr,
      L"",
      PromptStr,
      NULL
      );
    FreePool (QuestionStr);
    FreePool (PromptStr);
    return Status;
  } 

  //
  // Check whether User is ever enrolled in the provider.
  // 
  for (Index = 0; Index < mUsbTable->Count; Index++) {
    UserId = (UINT8 *) &mUsbTable->UserInfo[Index].UserId;
    if (CompareMem (UserId, (UINT8 *) &UsbInfo.UserId, sizeof (EFI_USER_INFO_IDENTIFIER)) == 0) {
      //
      // User already exists, update the password.
      //      
      break;
    }
  }
  
  //
  // Enroll the User to the provider.
  //
  Status = ModifyTable (Index, &UsbInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


/**
  Returns the user interface information used during user identification.

  This function returns information about the form used when interacting with the
  user during user identification. The form is the first enabled form in the form-set
  class EFI_HII_USER_CREDENTIAL_FORMSET_GUID installed on the HII handle HiiHandle. If 
  the user credential provider does not require a form to identify the user, then this
  function should return EFI_NOT_FOUND.

  @param[in]  This       Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[out] Hii        On return, holds the HII database handle.
  @param[out] FormSetId  On return, holds the identifier of the form set which contains
                         the form used during user identification.
  @param[out] FormId     On return, holds the identifier of the form used during user 
                         identification.
                         
  @retval EFI_SUCCESS            Form returned successfully.
  @retval EFI_NOT_FOUND          Form not returned.
  @retval EFI_INVALID_PARAMETER  Hii is NULL or FormSetId is NULL or FormId is NULL.
  
**/
EFI_STATUS
EFIAPI
CredentialForm (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  OUT       EFI_HII_HANDLE                      *Hii,
  OUT       EFI_GUID                            *FormSetId,
  OUT       EFI_FORM_ID                         *FormId
  )
{
  if ((This == NULL) || (Hii == NULL) || 
      (FormSetId == NULL) || (FormId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_NOT_FOUND;
}


/**
  Returns bitmap used to describe the credential provider type.

  This optional function returns a bitmap which is less than or equal to the number
  of pixels specified by Width and Height. If no such bitmap exists, then EFI_NOT_FOUND
  is returned. 

  @param[in]     This    Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[in, out] Width  On entry, points to the desired bitmap width. If NULL then no 
                         bitmap information will be returned. On exit, points to the 
                         width of the bitmap returned.
  @param[in, out] Height On entry, points to the desired bitmap height. If NULL then no
                         bitmap information will be returned. On exit, points to the 
                         height of the bitmap returned.
  @param[out]    Hii     On return, holds the HII database handle. 
  @param[out]    Image   On return, holds the HII image identifier. 
 
  @retval EFI_SUCCESS            Image identifier returned successfully.
  @retval EFI_NOT_FOUND          Image identifier not returned.
  @retval EFI_INVALID_PARAMETER  Hii is NULL or Image is NULL.
  
**/
EFI_STATUS
EFIAPI
CredentialTile (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  IN OUT    UINTN                               *Width,
  IN OUT    UINTN                               *Height,
  OUT       EFI_HII_HANDLE                      *Hii,
  OUT       EFI_IMAGE_ID                        *Image
  )
{
  if ((This == NULL) || (Hii == NULL) || (Image == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_NOT_FOUND;
}


/**
  Returns string used to describe the credential provider type.

  This function returns a string which describes the credential provider. If no
  such string exists, then EFI_NOT_FOUND is returned. 

  @param[in]  This       Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[out] Hii        On return, holds the HII database handle.
  @param[out] String     On return, holds the HII string identifier.
 
  @retval EFI_SUCCESS            String identifier returned successfully.
  @retval EFI_NOT_FOUND          String identifier not returned.
  @retval EFI_INVALID_PARAMETER  Hii is NULL or String is NULL.
  
**/
EFI_STATUS
EFIAPI
CredentialTitle (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  OUT       EFI_HII_HANDLE                      *Hii,
  OUT       EFI_STRING_ID                       *String
  )
{
  if ((This == NULL) || (Hii == NULL) || (String == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Set Hii handle and String ID.
  //
  *Hii    = mCallbackInfo->HiiHandle;
  *String = STRING_TOKEN (STR_CREDENTIAL_TITLE);

  return EFI_SUCCESS;
}


/**
  Return the user identifier associated with the currently authenticated user.

  This function returns the user identifier of the user authenticated by this credential
  provider. This function is called after the credential-related information has been 
  submitted on a form OR after a call to Default() has returned that this credential is
  ready to log on.

  @param[in]  This           Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[in]  User           The user profile handle of the user profile currently being 
                             considered by the user identity manager. If NULL, then no user
                             profile is currently under consideration.
  @param[out] Identifier     On return, points to the user identifier. 
 
  @retval EFI_SUCCESS            User identifier returned successfully.
  @retval EFI_NOT_READY          No user identifier can be returned.
  @retval EFI_ACCESS_DENIED      The user has been locked out of this user credential.
  @retval EFI_INVALID_PARAMETER  This is NULL, or Identifier is NULL.
  @retval EFI_NOT_FOUND          User is not NULL, and the specified user handle can't be
                                 found in user profile database.
  
**/
EFI_STATUS
EFIAPI
CredentialUser (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  IN        EFI_USER_PROFILE_HANDLE             User,
  OUT       EFI_USER_INFO_IDENTIFIER            *Identifier
  )
{
  EFI_STATUS    Status;
  UINTN         Index;
  EFI_USER_INFO *UserInfo;
  UINT8         *UserId;
  UINT8         *NewUserId;
  UINT8         *UserToken; 
  UINT8         ReadToken[HASHED_CREDENTIAL_LEN];
  EFI_INPUT_KEY Key;
  CHAR16        *QuestionStr;
  CHAR16        *PromptStr;
  
  if ((This == NULL) || (Identifier == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
 
  if (User == NULL) {
    //
    // Verify the auto logon user, get user id by matched token.
    //
    if (mUsbTable->Count == 0) {
      return EFI_NOT_READY;
    }
    
    //
    // No user selected, get token first and verify the user existed in user database.
    //
    Status = GetToken (ReadToken);
    if (EFI_ERROR (Status)) {
      return EFI_NOT_READY;
    }
    
    for (Index = 0; Index < mUsbTable->Count; Index++) {
      //
      // find the specified credential in the Usb credential database.
      //
      UserToken = mUsbTable->UserInfo[Index].Token;
      if (CompareMem (UserToken, ReadToken, HASHED_CREDENTIAL_LEN) == 0) {
        UserId  = (UINT8 *) &mUsbTable->UserInfo[Index].UserId;
        CopyMem (Identifier, UserId, sizeof (EFI_USER_INFO_IDENTIFIER));
        return EFI_SUCCESS;
      }
    }

    return EFI_NOT_READY;  
  }
   
  //  
  // User is not NULL here. Read a token, and check whether the token matches with 
  // the selected user's Token. If not, try to find a token in token DB to matches 
  // with read token.
  // 
  
  Status = GetToken (ReadToken);
  if (EFI_ERROR (Status)) {
    QuestionStr = GetStringById (STRING_TOKEN (STR_READ_USB_TOKEN_ERROR));
    PromptStr   = GetStringById (STRING_TOKEN (STR_INSERT_USB_TOKEN));
    CreatePopUp (
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      &Key,
      QuestionStr,
      L"",
      PromptStr,
      NULL
      );
    FreePool (QuestionStr);
    FreePool (PromptStr);
    return EFI_NOT_FOUND;
  }

  //
  // Get the selected user's identifier.
  //
  Status = FindUserInfoByType (User, EFI_USER_INFO_IDENTIFIER_RECORD, &UserInfo);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  } 
  
  //
  // Check the selected user's Token with the read token.
  //
  for (Index = 0; Index < mUsbTable->Count; Index++) {
    UserId    = (UINT8 *) &mUsbTable->UserInfo[Index].UserId;
    NewUserId = (UINT8 *) (UserInfo + 1);
    if (CompareMem (UserId, NewUserId, sizeof (EFI_USER_INFO_IDENTIFIER)) == 0) {
      //
      // The user's ID is found in the UsbTable.
      //
      UserToken = mUsbTable->UserInfo[Index].Token;
      if (CompareMem (UserToken, ReadToken, HASHED_CREDENTIAL_LEN) == 0) {
        //
        // The read token matches with the one in UsbTable.
        //
        CopyMem (Identifier, UserId, sizeof (EFI_USER_INFO_IDENTIFIER));
        FreePool (UserInfo);
        return EFI_SUCCESS;
      } 
    }
  }

  FreePool (UserInfo); 
    
  return EFI_NOT_READY;
}


/**
  Indicate that user interface interaction has begun for the specified credential.

  This function is called when a credential provider is selected by the user. If 
  AutoLogon returns FALSE, then the user interface will be constructed by the User
  Identity Manager. 

  @param[in]  This       Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[out] AutoLogon  On return, points to the credential provider's capabilities 
                         after the credential provider has been selected by the user. 
 
  @retval EFI_SUCCESS            Credential provider successfully selected.
  @retval EFI_INVALID_PARAMETER  AutoLogon is NULL.
  
**/
EFI_STATUS
EFIAPI
CredentialSelect (
  IN  CONST  EFI_USER_CREDENTIAL2_PROTOCOL   *This,
  OUT        EFI_CREDENTIAL_LOGON_FLAGS      *AutoLogon
  )
{
  if ((This == NULL) || (AutoLogon == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *AutoLogon = EFI_CREDENTIAL_LOGON_FLAG_DEFAULT | EFI_CREDENTIAL_LOGON_FLAG_AUTO;

  return EFI_SUCCESS;
}


/**
  Indicate that user interface interaction has ended for the specified credential.

  This function is called when a credential provider is deselected by the user.

  @param[in] This        Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
 
  @retval EFI_SUCCESS    Credential provider successfully deselected.
  
**/
EFI_STATUS
EFIAPI
CredentialDeselect (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This
  )
{
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}


/**
  Return the default logon behavior for this user credential.

  This function reports the default login behavior regarding this credential provider.  

  @param[in]  This       Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[out] AutoLogon  On return, holds whether the credential provider should be used
                         by default to automatically log on the user.  
 
  @retval EFI_SUCCESS            Default information successfully returned.
  @retval EFI_INVALID_PARAMETER  AutoLogon is NULL.
  
**/
EFI_STATUS
EFIAPI
CredentialDefault (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  OUT       EFI_CREDENTIAL_LOGON_FLAGS          *AutoLogon
  )
{
  if ((This == NULL) || (AutoLogon == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *AutoLogon = EFI_CREDENTIAL_LOGON_FLAG_DEFAULT | EFI_CREDENTIAL_LOGON_FLAG_AUTO;
  return EFI_SUCCESS;
}


/**
  Return information attached to the credential provider.

  This function returns user information. 

  @param[in]      This          Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[in]      UserInfo      Handle of the user information data record. 
  @param[out]     Info          On entry, points to a buffer of at least *InfoSize bytes. On
                                exit, holds the user information. If the buffer is too small
                                to hold the information, then EFI_BUFFER_TOO_SMALL is returned
                                and InfoSize is updated to contain the number of bytes actually
                                required.
  @param[in, out] InfoSize      On entry, points to the size of Info. On return, points to the 
                                size of the user information. 
 
  @retval EFI_SUCCESS           Information returned successfully.
  @retval EFI_BUFFER_TOO_SMALL  The size specified by InfoSize is too small to hold all of the
                                user information. The size required is returned in *InfoSize.
  @retval EFI_INVALID_PARAMETER Info is NULL or InfoSize is NULL.
  @retval EFI_NOT_FOUND         The specified UserInfo does not refer to a valid user info handle. 
                                
**/
EFI_STATUS
EFIAPI
CredentialGetInfo (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  IN        EFI_USER_INFO_HANDLE                UserInfo,
  OUT       EFI_USER_INFO                       *Info,
  IN OUT    UINTN                               *InfoSize
  )
{
  EFI_USER_INFO            *CredentialInfo;
  UINTN                    Index;
  
  if ((This == NULL) || (InfoSize == NULL) || (Info == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UserInfo == NULL) || (mUsbInfoHandle == NULL)) {
    return EFI_NOT_FOUND;
  }
  
  //
  // Find information handle in credential info table.
  //
  for (Index = 0; Index < mUsbInfoHandle->Count; Index++) {
    CredentialInfo = mUsbInfoHandle->Info[Index];
    if (UserInfo == (EFI_USER_INFO_HANDLE)CredentialInfo) {
      //
      // The handle is found, copy the user info.
      //
      if (CredentialInfo->InfoSize > *InfoSize) {
        *InfoSize = CredentialInfo->InfoSize;
        return EFI_BUFFER_TOO_SMALL;
      }
      
      CopyMem (Info, CredentialInfo, CredentialInfo->InfoSize);      
      return EFI_SUCCESS;            
    }
  }
  
  return EFI_NOT_FOUND;
}


/**
  Enumerate all of the user informations on the credential provider.

  This function returns the next user information record. To retrieve the first user
  information record handle, point UserInfo at a NULL. Each subsequent call will retrieve
  another user information record handle until there are no more, at which point UserInfo
  will point to NULL. 

  @param[in]      This     Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[in, out] UserInfo On entry, points to the previous user information handle or NULL
                           to start enumeration. On exit, points to the next user information
                           handle or NULL if there is no more user information.
 
  @retval EFI_SUCCESS            User information returned.
  @retval EFI_NOT_FOUND          No more user information found.
  @retval EFI_INVALID_PARAMETER  UserInfo is NULL.
  
**/
EFI_STATUS
EFIAPI
CredentialGetNextInfo (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  IN OUT    EFI_USER_INFO_HANDLE                *UserInfo
  )
{
  EFI_USER_INFO            *Info;
  CHAR16                   *ProvNameStr;
  UINTN                    InfoLen;
  UINTN                    Index;
  UINTN                    ProvStrLen;
    
  if ((This == NULL) || (UserInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mUsbInfoHandle == NULL) {
    //
    // Initilized user info table. There are 4 user info records in the table.
    //
    InfoLen  = sizeof (USB_CREDENTIAL_INFO) + (4 - 1) * sizeof (EFI_USER_INFO *);
    mUsbInfoHandle = AllocateZeroPool (InfoLen);
    if (mUsbInfoHandle == NULL) {
      *UserInfo = NULL;
      return EFI_NOT_FOUND;
    }

    //
    // The first information, Credential Provider info.
    //
    InfoLen = sizeof (EFI_USER_INFO) + sizeof (EFI_GUID);
    Info    = AllocateZeroPool (InfoLen);
    ASSERT (Info != NULL);
    
    Info->InfoType    = EFI_USER_INFO_CREDENTIAL_PROVIDER_RECORD;
    Info->InfoSize    = (UINT32) InfoLen;
    Info->InfoAttribs = EFI_USER_INFO_PROTECTED;
    CopyGuid (&Info->Credential, &gUsbCredentialProviderGuid);
    CopyGuid ((EFI_GUID *)(Info + 1), &gUsbCredentialProviderGuid);
    
    mUsbInfoHandle->Info[0] = Info;
    mUsbInfoHandle->Count++;

    //
    // The second information, Credential Provider name info.
    //
    ProvNameStr = GetStringById (STRING_TOKEN (STR_PROVIDER_NAME));
    ProvStrLen  = StrSize (ProvNameStr);
    InfoLen     = sizeof (EFI_USER_INFO) + ProvStrLen;
    Info        = AllocateZeroPool (InfoLen);
    ASSERT (Info != NULL);
    
    Info->InfoType    = EFI_USER_INFO_CREDENTIAL_PROVIDER_NAME_RECORD;
    Info->InfoSize    = (UINT32) InfoLen;
    Info->InfoAttribs = EFI_USER_INFO_PROTECTED;
    CopyGuid (&Info->Credential, &gUsbCredentialProviderGuid);
    CopyMem ((UINT8*)(Info + 1), ProvNameStr, ProvStrLen);
    FreePool (ProvNameStr);
    
    mUsbInfoHandle->Info[1] = Info;
    mUsbInfoHandle->Count++;

    //
    // The third information, Credential Provider type info.
    //
    InfoLen = sizeof (EFI_USER_INFO) + sizeof (EFI_GUID);
    Info    = AllocateZeroPool (InfoLen);
    ASSERT (Info != NULL);
      
    Info->InfoType    = EFI_USER_INFO_CREDENTIAL_TYPE_RECORD;
    Info->InfoSize    = (UINT32) InfoLen;
    Info->InfoAttribs = EFI_USER_INFO_PROTECTED;
    CopyGuid (&Info->Credential, &gUsbCredentialProviderGuid);
    CopyGuid ((EFI_GUID *)(Info + 1), &gEfiUserCredentialClassSecureCardGuid);
    
    mUsbInfoHandle->Info[2] = Info;
    mUsbInfoHandle->Count++;
 
    //
    // The fourth information, Credential Provider type name info.
    //
    ProvNameStr = GetStringById (STRING_TOKEN (STR_PROVIDER_TYPE_NAME));
    ProvStrLen  = StrSize (ProvNameStr);
    InfoLen     = sizeof (EFI_USER_INFO) + ProvStrLen;
    Info        = AllocateZeroPool (InfoLen);
    ASSERT (Info != NULL);
    
    Info->InfoType    = EFI_USER_INFO_CREDENTIAL_PROVIDER_NAME_RECORD;
    Info->InfoSize    = (UINT32) InfoLen;
    Info->InfoAttribs = EFI_USER_INFO_PROTECTED;
    CopyGuid (&Info->Credential, &gUsbCredentialProviderGuid);
    CopyMem ((UINT8*)(Info + 1), ProvNameStr, ProvStrLen);
    FreePool (ProvNameStr);
    
    mUsbInfoHandle->Info[3] = Info;
    mUsbInfoHandle->Count++;
  }
  
  if (*UserInfo == NULL) {
    //
    // Return the first info handle.
    //
    *UserInfo = (EFI_USER_INFO_HANDLE) mUsbInfoHandle->Info[0];
    return EFI_SUCCESS;
  }
  
  //
  // Find information handle in credential info table.
  //
  for (Index = 0; Index < mUsbInfoHandle->Count; Index++) {
    Info = mUsbInfoHandle->Info[Index];
    if (*UserInfo == (EFI_USER_INFO_HANDLE)Info) {
      //
      // The handle is found, get the next one.
      //
      if (Index == mUsbInfoHandle->Count - 1) {
        //
        // Already last one.
        //
        *UserInfo = NULL;
        return EFI_NOT_FOUND;
      }
      Index++;
      *UserInfo = (EFI_USER_INFO_HANDLE)mUsbInfoHandle->Info[Index];
      return EFI_SUCCESS; 
    }
  }

  *UserInfo = NULL;
  return EFI_NOT_FOUND;
}


/**
  Delete a user on this credential provider.

  This function deletes a user on this credential provider. 

  @param[in]     This            Points to this instance of the EFI_USER_CREDENTIAL2_PROTOCOL.
  @param[in]     User            The user profile handle to delete.

  @retval EFI_SUCCESS            User profile was successfully deleted.
  @retval EFI_ACCESS_DENIED      Current user profile does not permit deletion on the user profile handle. 
                                 Either the user profile cannot delete on any user profile or cannot delete 
                                 on a user profile other than the current user profile. 
  @retval EFI_UNSUPPORTED        This credential provider does not support deletion in the pre-OS.
  @retval EFI_DEVICE_ERROR       The new credential could not be deleted because of a device error.
  @retval EFI_INVALID_PARAMETER  User does not refer to a valid user profile handle.
**/
EFI_STATUS
EFIAPI
CredentialDelete (
  IN CONST  EFI_USER_CREDENTIAL2_PROTOCOL       *This,
  IN        EFI_USER_PROFILE_HANDLE             User
  )
{
  EFI_STATUS                Status;
  EFI_USER_INFO             *UserInfo;
  UINT8                     *UserId;
  UINT8                     *NewUserId;
  UINTN                     Index;
  
  if ((This == NULL) || (User == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get User Identifier.
  //
  UserInfo = NULL;
  Status = FindUserInfoByType (
             User,
             EFI_USER_INFO_IDENTIFIER_RECORD,
             &UserInfo
             );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the user by user identifier in mPwdTable.
  // 
  for (Index = 0; Index < mUsbTable->Count; Index++) {
    UserId    = (UINT8 *) &mUsbTable->UserInfo[Index].UserId;
    NewUserId = (UINT8 *) (UserInfo + 1);
    if (CompareMem (UserId, NewUserId, sizeof (EFI_USER_INFO_IDENTIFIER)) == 0) {
      //
      // Found the user, delete it.
      //
      ModifyTable (Index, NULL);
      break;
    }
  }

  FreePool (UserInfo);
  return EFI_SUCCESS;
}


/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
UsbProviderInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Init credential table.
  //
  Status = InitCredentialTable ();
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Init Form Browser
  //
  Status = InitFormBrowser ();
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Install protocol interfaces for the Usb Credential Provider.
  //
  Status = gBS->InstallProtocolInterface (
                  &mCallbackInfo->DriverHandle,
                  &gEfiUserCredential2ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gUsbCredentialProviderDriver
                  );
  return Status;
}
