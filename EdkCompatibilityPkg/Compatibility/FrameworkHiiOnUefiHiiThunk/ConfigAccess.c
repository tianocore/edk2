/** @file
  This file implements functions related to Config Access Protocols installed by
  by HII Thunk Modules. These Config access Protocols are used to thunk UEFI Config 
  Access Callback to Framework HII Callback and EFI Variable Set/Get operations.
  
Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HiiDatabase.h"
#include "UefiIfrParser.h"

BOOLEAN            mHiiPackageListUpdated = FALSE;

HII_VENDOR_DEVICE_PATH  mUefiHiiVendorDevicePath = {
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8) (sizeof (HII_VENDOR_DEVICE_PATH_NODE)),
          (UINT8) ((sizeof (HII_VENDOR_DEVICE_PATH_NODE)) >> 8)
        }
      },
      EFI_CALLER_ID_GUID
    },
    0,
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { 
      (UINT8) (sizeof (EFI_DEVICE_PATH_PROTOCOL)),
      (UINT8) ((sizeof (EFI_DEVICE_PATH_PROTOCOL)) >> 8)
    }
  }
};

CONFIG_ACCESS_PRIVATE gConfigAccessPrivateTempate = {
  CONFIG_ACCESS_PRIVATE_SIGNATURE,
  {
    ThunkExtractConfig,
    ThunkRouteConfig,
    ThunkCallback
  }, //ConfigAccessProtocol
  NULL, //FormCallbackProtocol
  NULL 
};

/**
  Get the first EFI_IFR_VARSTORE from the FormSet. 
    
  @param FormSet                  The Form Set.
   
  @retval FORMSET_STORAGE *       Return the first EFI_IFR_VARSTORE.
  @retval NULL                    If the Form Set does not have EFI_IFR_VARSTORE.
**/
FORMSET_STORAGE *
GetFirstStorageOfFormSet (
  IN CONST FORM_BROWSER_FORMSET * FormSet
  ) 
{
  LIST_ENTRY             *StorageList;
  FORMSET_STORAGE        *Storage;

  StorageList = GetFirstNode (&FormSet->StorageListHead);

  while (!IsNull (&FormSet->StorageListHead, StorageList)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageList);
    if (Storage->Type == EFI_HII_VARSTORE_BUFFER) {
      return Storage;
    }
    StorageList = GetNextNode (&FormSet->StorageListHead, StorageList);
  }
  
  return NULL;
}

/**
  Get the FORM_BROWSER_STATEMENT that matches the Question's value.
    
  @param FormSet                  The Form Set.
  @param QuestionId               QuestionId
   
  @retval FORM_BROWSER_STATEMENT*   FORM_BROWSER_STATEMENT that match Question's value.
  @retval NULL                      If the Form Set does not have EFI_IFR_VARSTORE.
**/
FORM_BROWSER_STATEMENT *
GetStorageFromQuestionId (
  IN CONST FORM_BROWSER_FORMSET * FormSet,
  IN       EFI_QUESTION_ID        QuestionId
  )
{
  LIST_ENTRY             *FormList;
  LIST_ENTRY             *StatementList;
  FORM_BROWSER_FORM      *Form;
  FORM_BROWSER_STATEMENT *Statement;

  FormList = GetFirstNode (&FormSet->FormListHead);

  while (!IsNull (&FormSet->FormListHead, FormList)) {
    Form = FORM_BROWSER_FORM_FROM_LINK (FormList);

    StatementList = GetFirstNode (&Form->StatementListHead);

    while (!IsNull (&Form->StatementListHead, StatementList)) {
      Statement = FORM_BROWSER_STATEMENT_FROM_LINK (StatementList);
      if ((QuestionId == Statement->QuestionId) && (Statement->Storage != NULL)) {
        //
        // UEFI Question ID is unique in a FormSet.
        //
        ASSERT (Statement->Storage->Type == EFI_HII_VARSTORE_BUFFER);
        return Statement;
      }
      StatementList = GetNextNode (&Form->StatementListHead, StatementList);
    }

    FormList = GetNextNode (&FormSet->FormListHead, FormList);
  }
  
  return NULL;
}

/**
  Get the EFI_IFR_VARSTORE based the <ConfigHdr> string in a <ConfigRequest>
  or a <ConfigResp> string.
    
  @param FormSet                  The Form Set.
  @param ConfigString             The Configuration String which is defined by UEFI HII.
   
  @retval FORMSET_STORAGE *       The EFI_IFR_VARSTORE where the Question's value is stored.
  @retval NULL                    If the Form Set does not have EFI_IFR_VARSTORE with such ID.
**/
FORMSET_STORAGE *
GetStorageFromConfigString (
  IN CONST FORM_BROWSER_FORMSET *FormSet,
  IN  CONST EFI_STRING          ConfigString
  )
{
  LIST_ENTRY             *StorageList;
  FORMSET_STORAGE        *Storage;
  CHAR16                 *Name;

  if (ConfigString == NULL) {
    return NULL;
  }

  StorageList = GetFirstNode (&FormSet->StorageListHead);

  while (!IsNull (&FormSet->StorageListHead, StorageList)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageList);
    StorageList = GetNextNode (&FormSet->StorageListHead, StorageList);
    if (Storage->Type != EFI_HII_VARSTORE_BUFFER) {
      continue;
    }

    if ((Storage->VarStoreId == FormSet->DefaultVarStoreId) && (FormSet->OriginalDefaultVarStoreName != NULL)) {
      Name = FormSet->OriginalDefaultVarStoreName;
    } else {
      Name = Storage->Name;
    }
    
    if (HiiIsConfigHdrMatch (ConfigString, &Storage->Guid, Name)) {
      return Storage;
    }
  }

  return NULL;
}

/**
  This function installs a EFI_CONFIG_ACCESS_PROTOCOL instance for a form package registered
  by a module using Framework HII Protocol Interfaces.

  UEFI HII require EFI_HII_CONFIG_ACCESS_PROTOCOL to be installed on a EFI_HANDLE, so
  that Setup Utility can load the Buffer Storage using this protocol.
   
  @param Packages             The Package List.
  @param ThunkContext         The Thunk Context.
   
  @retval  EFI_SUCCESS        The Config Access Protocol is installed successfully.
  @retval  EFI_OUT_RESOURCE   There is not enough memory.
   
**/
EFI_STATUS
InstallDefaultConfigAccessProtocol (
  IN  CONST EFI_HII_PACKAGES                    *Packages,
  IN  OUT   HII_THUNK_CONTEXT                   *ThunkContext
  )
{
  EFI_STATUS                                  Status;
  CONFIG_ACCESS_PRIVATE                       *ConfigAccessInstance;
  HII_VENDOR_DEVICE_PATH                      *HiiVendorPath;

  ASSERT (ThunkContext->IfrPackageCount != 0);

  ConfigAccessInstance = AllocateCopyPool (
                           sizeof (CONFIG_ACCESS_PRIVATE), 
                           &gConfigAccessPrivateTempate
                           );
  ASSERT (ConfigAccessInstance != NULL);

  //
  // Use memory address as unique ID to distinguish from different device paths
  // This function may be called multi times by the framework HII driver.
  //
  HiiVendorPath = AllocateCopyPool (
                           sizeof (HII_VENDOR_DEVICE_PATH), 
                           &mUefiHiiVendorDevicePath
                           );
  ASSERT (HiiVendorPath != NULL);

  HiiVendorPath->Node.UniqueId = (UINT64) ((UINTN) HiiVendorPath);

  Status = gBS->InstallMultipleProtocolInterfaces (
          &ThunkContext->UefiHiiDriverHandle,
          &gEfiDevicePathProtocolGuid,          
          HiiVendorPath,
          &gEfiHiiConfigAccessProtocolGuid,
          &ConfigAccessInstance->ConfigAccessProtocol,
          NULL
          );
  ASSERT_EFI_ERROR (Status);
  
  ConfigAccessInstance->ThunkContext = ThunkContext;
  
  return EFI_SUCCESS;
}

/**
  This function un-installs the EFI_CONFIG_ACCESS_PROTOCOL instance for a form package registered
  by a module using Framework HII Protocol Interfaces.

  ASSERT if no Config Access is found for such pakcage list or failed to uninstall the protocol.

  @param ThunkContext         The Thunk Context.
   
**/
VOID
UninstallDefaultConfigAccessProtocol (
  IN  HII_THUNK_CONTEXT                   *ThunkContext
  )
{
  EFI_STATUS                      Status;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  HII_VENDOR_DEVICE_PATH          *HiiVendorPath;

  Status = gBS->HandleProtocol (
                  ThunkContext->UefiHiiDriverHandle,
                  &gEfiHiiConfigAccessProtocolGuid,
                  (VOID **) &ConfigAccess
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->HandleProtocol (
                  ThunkContext->UefiHiiDriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &HiiVendorPath
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ThunkContext->UefiHiiDriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  HiiVendorPath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

}
  

/**
   Wrap the EFI_HII_CONFIG_ACCESS_PROTOCOL.ExtractConfig to a call to EFI_FORM_CALLBACK_PROTOCOL.NvRead.
   
   @param BufferStorage         The key with all attributes needed to call EFI_FORM_CALLBACK_PROTOCOL.NvRead.
   @param FwFormCallBack    The EFI_FORM_CALLBACK_PROTOCOL registered by Framework HII module.
   @param Data                     The data read.
   @param DataSize                 The size of data.
   
   @retval EFI_STATUS              The status returned by the EFI_FORM_CALLBACK_PROTOCOL.NvWrite.
   @retval EFI_INVALID_PARAMETER   If the EFI_FORM_CALLBACK_PROTOCOL.NvRead return the size information of the data
                                   does not match what has been recorded early in he BUFFER_STORAGE_ENTRY.
 **/
EFI_STATUS
CallFormCallBack (
  IN       FORMSET_STORAGE                            *BufferStorage,
  IN       EFI_FORM_CALLBACK_PROTOCOL                 *FwFormCallBack,
  OUT      VOID                                       **Data,
  OUT      UINTN                                      *DataSize
  )
{
  EFI_STATUS          Status;

  *DataSize = 0;
  *Data     = NULL;
  
  Status = FwFormCallBack->NvRead (
              FwFormCallBack,  
              BufferStorage->Name,
              &BufferStorage->Guid,
              NULL,
              DataSize,
              *Data
              );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    if (BufferStorage->Size != *DataSize) {
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    }

    *Data = AllocateZeroPool (*DataSize);
    if (*Data == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = FwFormCallBack->NvRead (
                  FwFormCallBack,  
                  BufferStorage->Name,
                  &BufferStorage->Guid,
                  NULL,
                  DataSize,
                  *Data
                  );
  }

  return Status;
}


/**
   Wrap the EFI_HII_CONFIG_ACCESS_PROTOCOL.ExtractConfig to a call to UEFI Variable Get Service.
   
   @param BufferStorage        The key with all attributes needed to call a UEFI Variable Get Service.
   @param Data                    The data read.
   @param DataSize                The size of data.

   If the UEFI Variable Get Service return the size information of the data
   does not match what has been recorded early in he BUFFER_STORAGE_ENTRY.
   then ASSERT.
                                                        
   @retval EFI_STATUS              The status returned by the UEFI Variable Get Service.
   @retval EFI_INVALID_PARAMETER   If the UEFI Variable Get Service return the size information of the data
                                   does not match what has been recorded early in he BUFFER_STORAGE_ENTRY.
 **/
EFI_STATUS
GetUefiVariable (
  IN       FORMSET_STORAGE                            *BufferStorage,
  OUT      VOID                                       **Data,
  OUT      UINTN                                      *DataSize
  )
{
  EFI_STATUS          Status;

  *DataSize = 0;
  *Data = NULL;
  Status = gRT->GetVariable (
              BufferStorage->Name,
              &BufferStorage->Guid,
              NULL,
              DataSize,
              *Data
              );
  if (Status == EFI_BUFFER_TOO_SMALL) {

    if (BufferStorage->Size != *DataSize) {
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    }

    *Data = AllocateZeroPool (*DataSize);
    if (*Data == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gRT->GetVariable (
                BufferStorage->Name,
                &BufferStorage->Guid,
                NULL,
                DataSize,
                *Data
                );
  }

  return Status;
}

/**

  This function implement the EFI_HII_CONFIG_ACCESS_PROTOCOL.ExtractConfig
  so that data can be read from the data storage such as UEFI Variable or module's
  customized storage exposed by EFI_FRAMEWORK_CALLBACK.

   @param This        Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL
   @param Request     A null-terminated Unicode string in <ConfigRequest> format. Note that this
                      includes the routing information as well as the configurable name / value pairs. It is
                      invalid for this string to be in <MultiConfigRequest> format.

   @param Progress    On return, points to a character in the Request string. Points to the string's null
                      terminator if request was successful. Points to the most recent '&' before the first
                      failing name / value pair (or the beginning of the string if the failure is in the first
                      name / value pair) if the request was not successful
   @param Results     A null-terminated Unicode string in <ConfigAltResp> format which has all
                      values filled in for the names in the Request string. String to be allocated by the called
                      function.
   
   @retval EFI_INVALID_PARAMETER   If there is no Buffer Storage for this Config Access instance.
   @retval EFI_SUCCESS             The setting is retrived successfully.
   @retval !EFI_SUCCESS            The error returned by UEFI Get Variable or Framework Form Callback Nvread.
 **/
EFI_STATUS
EFIAPI
ThunkExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;
  CONFIG_ACCESS_PRIVATE            *ConfigAccess;
  FORMSET_STORAGE                  *BufferStorage;
  VOID                             *Data;
  UINTN                            DataSize;
  FORM_BROWSER_FORMSET             *FormSetContext;
  CHAR16                           *VarStoreName;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  UINTN                            Size;
  BOOLEAN                          AllocatedRequest;
  LIST_ENTRY                       *StorageList;
  EFI_STRING                       SingleResult;
  EFI_STRING                       FinalResults;
  EFI_STRING                       StrPointer;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Request;

  Status         = EFI_SUCCESS;
  Data           = NULL;
  StrPointer     = NULL;
  SingleResult   = NULL;
  FinalResults   = NULL;
  ConfigAccess   = CONFIG_ACCESS_PRIVATE_FROM_PROTOCOL (This);
  FormSetContext = ConfigAccess->ThunkContext->FormSet;
  if (IsListEmpty (&FormSetContext->StorageListHead)) {
    //
    // No VarStorage does exist in this form.
    //
    return EFI_NOT_FOUND;
  }
  StorageList    = GetFirstNode (&FormSetContext->StorageListHead);

  do {
    if (Request != NULL) {
      BufferStorage = GetStorageFromConfigString (ConfigAccess->ThunkContext->FormSet, Request);
      if (BufferStorage == NULL) {
        return EFI_NOT_FOUND;
      }
    } else {
      if (IsNull (&FormSetContext->StorageListHead, StorageList)) {
        //
        // No Storage to be extracted into the results.
        //
        break;
      }
      BufferStorage = FORMSET_STORAGE_FROM_LINK (StorageList);
      StorageList = GetNextNode (&FormSetContext->StorageListHead, StorageList);
      if (BufferStorage->Type != EFI_HII_VARSTORE_BUFFER) {
        //
        // BufferStorage type should be EFI_HII_VARSTORE_BUFFER
        //
        continue;
      }
    }
  
    VarStoreName     = NULL;
    ConfigRequestHdr = NULL;
    ConfigRequest    = NULL;
    Size             = 0;
    AllocatedRequest = FALSE;

    if (ConfigAccess->ThunkContext->NvMapOverride == NULL) {
      //
      // NvMapOverride is not used. Get the Storage data from EFI Variable or Framework Form Callback.
      //
      if (ConfigAccess->FormCallbackProtocol == NULL ||
          ConfigAccess->FormCallbackProtocol->NvRead == NULL) {
        Status = GetUefiVariable (
                   BufferStorage,
                   &Data,
                   &DataSize
                   );
      } else {
        Status = CallFormCallBack (
                   BufferStorage,
                   ConfigAccess->FormCallbackProtocol,
                    &Data,
                    &DataSize
                   );
      }
    } else {
      //
      // Use the NvMapOverride.
      //
      DataSize = BufferStorage->Size;
      Data = AllocateCopyPool (DataSize, ConfigAccess->ThunkContext->NvMapOverride);
      
      if (Data != NULL) {
        Status = EFI_SUCCESS;
      } else {
        Status = EFI_OUT_OF_RESOURCES;
      }
    }
    
    if (!EFI_ERROR (Status)) {
      ConfigRequest = Request;
      if (Request == NULL || (StrStr (Request, L"OFFSET") == NULL)) {
        //
        // Request is without any request element, construct full request string.
        //

        if ((BufferStorage->VarStoreId == FormSetContext->DefaultVarStoreId) && (FormSetContext->OriginalDefaultVarStoreName != NULL)) {
          VarStoreName = FormSetContext->OriginalDefaultVarStoreName;
        } else {
          VarStoreName = BufferStorage->Name;
        }

        //
        // First Set ConfigRequestHdr string.
        //
        ConfigRequestHdr = HiiConstructConfigHdr (&BufferStorage->Guid, VarStoreName, ConfigAccess->ThunkContext->UefiHiiDriverHandle);
        ASSERT (ConfigRequestHdr != NULL);

        //
        // Allocate and fill a buffer large enough to hold the <ConfigHdr> template 
        // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
        //
        Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
        ConfigRequest = AllocateZeroPool (Size);
        ASSERT (ConfigRequest != NULL);
        AllocatedRequest = TRUE;
        UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)DataSize);
        FreePool (ConfigRequestHdr);
      }
      Status = mHiiConfigRoutingProtocol->BlockToConfig (
                                              mHiiConfigRoutingProtocol,
                                              ConfigRequest,
                                              Data,
                                              DataSize,
                                              &SingleResult,
                                              Progress
                                              );
      //
      // Free the allocated config request string.
      //
      if (AllocatedRequest) {
        FreePool (ConfigRequest);
        ConfigRequest = NULL;
      }
    }
    //
    // Free the allocated Data
    //
    if (Data != NULL) {
      FreePool (Data);
    }
    //
    // Directly return when meet with error
    //
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Merge result into the final results.
    //
    if (FinalResults == NULL) {
      FinalResults = SingleResult;
      SingleResult = NULL;
    } else {
      Size = StrLen (FinalResults);
      Size = Size + 1;
      Size = Size + StrLen (SingleResult) + 1;
      StrPointer = AllocateZeroPool (Size * sizeof (CHAR16));
      ASSERT (StrPointer != NULL);
      StrCpy (StrPointer, FinalResults);
      FreePool (FinalResults);
      FinalResults = StrPointer;
      StrPointer  = StrPointer + StrLen (StrPointer);
      *StrPointer = L'&';
      StrCpy (StrPointer + 1, SingleResult);
      FreePool (SingleResult);
    }
  } while (Request == NULL);

  if (!EFI_ERROR (Status)) {
    *Results = FinalResults;
  } else {
    if (FinalResults != NULL) {
      FreePool (FinalResults);
    }
  }
  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function implement the EFI_HII_CONFIG_ACCESS_PROTOCOL.RouteConfig
  so that data can be written to the data storage such as UEFI Variable or module's
  customized storage exposed by EFI_FRAMEWORK_CALLBACK.
   
   @param This             Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL
   @param Configuration    A null-terminated Unicode string in <ConfigResp> format.
   @param Progress         A pointer to a string filled in with the offset of the most recent '&' before the first
                           failing name / value pair (or the beginning of the string if the failure is in the first
                           name / value pair) or the terminating NULL if all was successful.
   
   @retval EFI_INVALID_PARAMETER   If there is no Buffer Storage for this Config Access instance.
   @retval EFI_SUCCESS             The setting is saved successfully.
   @retval !EFI_SUCCESS            The error returned by UEFI Set Variable or Framework Form Callback Nvwrite.
**/   
EFI_STATUS
EFIAPI
ThunkRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS                                  Status;
  CONFIG_ACCESS_PRIVATE                       *ConfigAccess;
  FORMSET_STORAGE                             *BufferStorage;
  VOID                                        *Data;
  UINTN                                       DataSize;
  UINTN                                       DataSize2;
  BOOLEAN                                     ResetRequired;
  BOOLEAN                                     DataAllocated;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data = NULL;
  ConfigAccess = CONFIG_ACCESS_PRIVATE_FROM_PROTOCOL (This);

  BufferStorage = GetStorageFromConfigString (ConfigAccess->ThunkContext->FormSet, Configuration);

  if (BufferStorage == NULL) {
    *Progress = Configuration;
    return EFI_NOT_FOUND;
  }

  DataSize2     = BufferStorage->Size;
  if (ConfigAccess->ThunkContext->NvMapOverride == NULL) {
    DataAllocated = TRUE;
    if (ConfigAccess->FormCallbackProtocol == NULL ||
        ConfigAccess->FormCallbackProtocol->NvRead == NULL) {
      Status = GetUefiVariable (
                 BufferStorage,
                 &Data,
                 &DataSize
                 );
    } else {
      Status = CallFormCallBack (
                 BufferStorage,
                 ConfigAccess->FormCallbackProtocol,
                  &Data,
                  &DataSize
                 );
    }
  } else {
    //
    // ConfigToBlock will convert the Config String and update the NvMapOverride accordingly.
    //
    Status = EFI_SUCCESS;
    Data = ConfigAccess->ThunkContext->NvMapOverride;
    DataSize      = DataSize2;
    DataAllocated = FALSE;
  }  
  if (EFI_ERROR (Status) || (DataSize2 != DataSize)) {
    if (Data == NULL) {
      Data = AllocateZeroPool (DataSize2);
    }
  }

  DataSize = DataSize2;
  Status = mHiiConfigRoutingProtocol->ConfigToBlock (
                                          mHiiConfigRoutingProtocol,
                                          Configuration,
                                          Data,
                                          &DataSize,
                                          Progress
                                          );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  if (ConfigAccess->ThunkContext->NvMapOverride == NULL) {
    if (ConfigAccess->FormCallbackProtocol == NULL ||
        ConfigAccess->FormCallbackProtocol->NvWrite == NULL) {
      Status = gRT->SetVariable (
                    BufferStorage->Name,
                    &BufferStorage->Guid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    DataSize2,
                    Data
                    );
    } else {
      Status = ConfigAccess->FormCallbackProtocol->NvWrite (
                    ConfigAccess->FormCallbackProtocol,  
                    BufferStorage->Name,
                    &BufferStorage->Guid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    DataSize2,
                    Data,
                    &ResetRequired
                    );
    }
  }

Done: 
  if (DataAllocated && (Data != NULL)) {
    FreePool (Data);
  }
  
  return Status;
}

/**
  Build the EFI_IFR_DATA_ARRAY which will be used to pass to 
  EFI_FORM_CALLBACK_PROTOCOL.Callback. Check definition of EFI_IFR_DATA_ARRAY
  for details.

  ASSERT if the Question Type is not EFI_IFR_TYPE_NUM_SIZE_* or EFI_IFR_TYPE_STRING.
  
   @param ConfigAccess     Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL
   @param QuestionId       The Question ID.
   @param Type             The Question Type.
   @param Value            The Question Value.
   @param NvMapAllocated   On output indicates if a buffer is allocated for NvMap.
   
   @return A pointer to EFI_IFR_DATA_ARRAY. The caller must free this buffer allocated.
**/   
EFI_IFR_DATA_ARRAY *
CreateIfrDataArray (
  IN    CONFIG_ACCESS_PRIVATE         *ConfigAccess,
  IN    EFI_QUESTION_ID               QuestionId,
  IN    UINT8                         Type,
  IN    EFI_IFR_TYPE_VALUE            *Value,
  OUT   BOOLEAN                       *NvMapAllocated
  )
{
  EFI_IFR_DATA_ARRAY                *IfrDataArray;
  EFI_IFR_DATA_ENTRY                *IfrDataEntry;
  UINTN                             BrowserDataSize;
  FORMSET_STORAGE                   *BufferStorage;
  UINTN                             Size;
  EFI_STRING                        String;
  FORM_BROWSER_STATEMENT            *Statement;

  *NvMapAllocated = FALSE;

  String = NULL;

  switch (Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
    case EFI_IFR_TYPE_NUM_SIZE_16:
    case EFI_IFR_TYPE_NUM_SIZE_32:
    case EFI_IFR_TYPE_NUM_SIZE_64:
    case EFI_IFR_TYPE_BOOLEAN:
      Size = sizeof (*Value);
      break;

    case EFI_IFR_TYPE_STRING:
      if (Value->string == 0) {
        Size = 0;
      } else {
        String = HiiGetString (ConfigAccess->ThunkContext->UefiHiiHandle, Value->string, NULL);
        ASSERT (String != NULL);

        Size = StrSize (String);
      }
      break;

    case EFI_IFR_TYPE_ACTION:
    case EFI_IFR_TYPE_UNDEFINED:
      Size = 0;
      break;
      
    default:
      ASSERT (FALSE);
      Size = 0;
      break;
  }

  IfrDataArray = AllocateZeroPool (sizeof (EFI_IFR_DATA_ARRAY) + sizeof (EFI_IFR_DATA_ENTRY) + Size);
  ASSERT (IfrDataArray != NULL);
  IfrDataArray->EntryCount = 1;
  IfrDataEntry             = (EFI_IFR_DATA_ENTRY *) (IfrDataArray + 1);

  Statement = GetStorageFromQuestionId (ConfigAccess->ThunkContext->FormSet, QuestionId);

  if (Statement == NULL || Statement->Storage == NULL) {
    //
    // The QuestionId is not associated with a Buffer Storage.
    // Try to get the first Buffer Storage then.
    //
    BufferStorage = GetFirstStorageOfFormSet (ConfigAccess->ThunkContext->FormSet);
  } else {
    BufferStorage        = Statement->Storage;
    IfrDataEntry->OpCode = Statement->Operand;
  }
  
  if (BufferStorage != NULL) {
    BrowserDataSize      = BufferStorage->Size;
    IfrDataEntry->Length = (UINT8) (sizeof (EFI_IFR_DATA_ENTRY) + Size);

    if (ConfigAccess->ThunkContext->NvMapOverride == NULL) {
      *NvMapAllocated = TRUE;
      IfrDataArray->NvRamMap = AllocateZeroPool (BrowserDataSize);
    } else {
      *NvMapAllocated = FALSE;
      IfrDataArray->NvRamMap = ConfigAccess->ThunkContext->NvMapOverride;
    }
    
    ASSERT (HiiGetBrowserData (&BufferStorage->Guid, BufferStorage->Name, BrowserDataSize, (UINT8 *) IfrDataArray->NvRamMap));

    switch (Type) {
      case EFI_IFR_TYPE_NUM_SIZE_8:
      case EFI_IFR_TYPE_NUM_SIZE_16:
      case EFI_IFR_TYPE_NUM_SIZE_32:
      case EFI_IFR_TYPE_NUM_SIZE_64:
      case EFI_IFR_TYPE_BOOLEAN:
        CopyMem (&IfrDataEntry->Data, &(Value->u8), sizeof (*Value));
        break;

      case EFI_IFR_TYPE_STRING:
        if (Size != 0) {
          ASSERT (String != NULL);
          StrCpy ((CHAR16 *) &IfrDataEntry->Data, String);
          FreePool (String);
        }
        break;

      case EFI_IFR_TYPE_ACTION:
      case EFI_IFR_TYPE_UNDEFINED:
        break;

      default:
        ASSERT (FALSE);
        break;
    }

    //
    // Need to fiil in the information for the rest of field for EFI_IFR_DATA_ENTRY.
    // It seems that no implementation is found to use other fields. Leave them uninitialized for now.
    //
    //UINT8   OpCode;           // Likely a string, numeric, or one-of
    //UINT8   Length;           // Length of the EFI_IFR_DATA_ENTRY packet
    //UINT16  Flags;            // Flags settings to determine what behavior is desired from the browser after the callback
    //VOID    *Data;            // The data in the form based on the op-code type - this is not a pointer to the data, the data follows immediately
    // If the OpCode is a OneOf or Numeric type - Data is a UINT16 value
    // If the OpCode is a String type - Data is a CHAR16[x] type
    // If the OpCode is a Checkbox type - Data is a UINT8 value
    // If the OpCode is a NV Access type - Data is a FRAMEWORK_EFI_IFR_NV_DATA structure
  }

  return IfrDataArray;
}

/**
  If a NvMapOverride is passed in to EFI_FORM_BROWSER_PROTOCOL.SendForm, the Form Browser
  needs to be informed when data changed in NvMapOverride. This function will invoke
  SetBrowserData () to set internal data of Form Browser.

  @param  ConfigAccess   The Config Access Private Context.
  @param  QuestionId     The Question Id that invokes the callback.
  

**/
VOID
SyncBrowserDataForNvMapOverride (
  IN    CONST CONFIG_ACCESS_PRIVATE         *ConfigAccess,
  IN          EFI_QUESTION_ID               QuestionId
  )
{
  FORMSET_STORAGE   *BufferStorage;
  BOOLEAN           CheckFlag;
  UINTN             BrowserDataSize;
  FORM_BROWSER_STATEMENT *Statement;

  if (ConfigAccess->ThunkContext->NvMapOverride != NULL) {

    Statement = GetStorageFromQuestionId (ConfigAccess->ThunkContext->FormSet, QuestionId);

    if (Statement == NULL || Statement->Storage == NULL) {
      //
      // QuestionId is a statement without Storage.
      // 1) It is a Goto. 
      // 
      //
      BufferStorage = GetFirstStorageOfFormSet (ConfigAccess->ThunkContext->FormSet);
    } else {
      BufferStorage = Statement->Storage;
    }

    //
    // If NvMapOverride is not NULL, this Form must have at least one Buffer Type Variable Storage.
    //
    ASSERT (BufferStorage != NULL);
    
    BrowserDataSize = BufferStorage->Size;

    CheckFlag = HiiSetBrowserData (&BufferStorage->Guid, BufferStorage->Name, BrowserDataSize, ConfigAccess->ThunkContext->NvMapOverride, NULL);
    ASSERT (CheckFlag);
  }

}

/**
  Free up resource allocated for a EFI_IFR_DATA_ARRAY by CreateIfrDataArray ().

  @param Array              The EFI_IFR_DATA_ARRAY allocated.
  @param NvMapAllocated     If the NvRamMap is allocated for EFI_IFR_DATA_ARRAY.

**/
VOID
DestroyIfrDataArray (
  IN  EFI_IFR_DATA_ARRAY           *Array,
  IN  BOOLEAN                      NvMapAllocated
  )
{
  if (Array != NULL) {
    if (NvMapAllocated) {
      FreePool (Array->NvRamMap);
    }

    FreePool (Array);
  }
}

/**
  Get the ONE_OF_OPTION_MAP_ENTRY for a QuestionId that invokes the 
  EFI_FORM_CALLBACK_PROTOCOL.Callback. The information is needed as
  the callback mechanism for EFI_IFR_ONE_OF_OPTION is changed from 
  EFI_IFR_ONE_OF_OPTION in Framework IFR. Check EFI_IFR_GUID_OPTIONKEY
  for detailed information.

  @param ThunkContext   The Thunk Context.
  @param QuestionId     The Question Id.
  @param Type           The Question Type.
  @param Value          The One Of Option's value.

  @return The ONE_OF_OPTION_MAP_ENTRY found.
  @retval NULL If no entry is found.
**/
ONE_OF_OPTION_MAP_ENTRY *
GetOneOfOptionMapEntry (
  IN  HII_THUNK_CONTEXT              *ThunkContext,
  IN  EFI_QUESTION_ID                QuestionId,
  IN  UINT8                          Type,
  IN  EFI_IFR_TYPE_VALUE             *Value
  )
{
  LIST_ENTRY              *Link;
  LIST_ENTRY              *Link2;
  ONE_OF_OPTION_MAP_ENTRY *OneOfOptionMapEntry;
  ONE_OF_OPTION_MAP       *OneOfOptionMap;
  FORM_BROWSER_FORMSET    *FormSet;

  FormSet = ThunkContext->FormSet;

  Link = GetFirstNode (&FormSet->OneOfOptionMapListHead);

  while (!IsNull (&FormSet->OneOfOptionMapListHead, Link)) {
    OneOfOptionMap = ONE_OF_OPTION_MAP_FROM_LINK(Link);
    if (OneOfOptionMap->QuestionId == QuestionId) {
      ASSERT (OneOfOptionMap->ValueType == Type);

      Link2 = GetFirstNode (&OneOfOptionMap->OneOfOptionMapEntryListHead);

      while (!IsNull (&OneOfOptionMap->OneOfOptionMapEntryListHead, Link2)) {
        OneOfOptionMapEntry = ONE_OF_OPTION_MAP_ENTRY_FROM_LINK (Link2);

        if (CompareMem (Value, &OneOfOptionMapEntry->Value, sizeof (EFI_IFR_TYPE_VALUE)) == 0) {
          return OneOfOptionMapEntry;
        }

        Link2 = GetNextNode (&OneOfOptionMap->OneOfOptionMapEntryListHead, Link2);
      }
    }

    Link = GetNextNode (&FormSet->OneOfOptionMapListHead, Link);
  }


  return NULL;
}

/**
  Functions which are registered to receive notification of
  database events have this prototype. The actual event is encoded
  in NotifyType. The following table describes how PackageType,
  PackageGuid, Handle, and Package are used for each of the
  notification types.

  If any Pakcage List in database is updated, mHiiPackageListUpdated
  will be set. If mHiiPackageListUpdated is set, Framework ThunkCallback()
  will force the UEFI Setup Browser to save the uncommitted data. This
  is needed as Framework's Callback function may dynamically update
  opcode in a Package List. UEFI Setup Browser will quit itself and reparse
  the Package List's IFR and display it. UEFI Config Access's implementation
  is required to save the modified (SetBrowserData or directly save the data
  to NV storage). But Framework HII Modules is not aware of this rule. Therefore,
  we will enforce the rule in ThunkCallback (). The side effect of force saving
  of NV data is the NV flag in browser may not flag a update as data has already
  been saved to NV storage.

  @param PackageType  Package type of the notification.

  @param PackageGuid  If PackageType is
                      EFI_HII_PACKAGE_TYPE_GUID, then this is
                      the pointer to the GUID from the Guid
                      field of EFI_HII_PACKAGE_GUID_HEADER.
                      Otherwise, it must be NULL.

  @param Package  Points to the package referred to by the
                  notification Handle The handle of the package
                  list which contains the specified package.

  @param Handle       The HII handle.

  @param NotifyType   The type of change concerning the
                      database. See
                      EFI_HII_DATABASE_NOTIFY_TYPE.

**/
EFI_STATUS
EFIAPI
FormUpdateNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
{
  mHiiPackageListUpdated = TRUE;

  return EFI_SUCCESS;
}

/**
  Wrap the EFI_HII_CONFIG_ACCESS_PROTOCOL.CallBack to EFI_FORM_CALLBACK_PROTOCOL.Callback. Therefor,
  the framework HII module willl do no porting and work with a UEFI HII SetupBrowser.
   
   @param This                      Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
   @param Action                    Specifies the type of action taken by the browser. See EFI_BROWSER_ACTION_x.
   @param QuestionId                A unique value which is sent to the original exporting driver so that it can identify the
                                    type of data to expect. The format of the data tends to vary based on the opcode that
                                    generated the callback.
   @param Type                      The type of value for the question. See EFI_IFR_TYPE_x in
                                    EFI_IFR_ONE_OF_OPTION.
   @param Value                     A pointer to the data being sent to the original exporting driver. The type is specified
                                    by Type. Type EFI_IFR_TYPE_VALUE is defined in
                                    EFI_IFR_ONE_OF_OPTION.
   @param ActionRequest             On return, points to the action requested by the callback function. Type
                                    EFI_BROWSER_ACTION_REQUEST is specified in SendForm() in the Form
                                    Browser Protocol.
   
   @retval EFI_UNSUPPORTED        If the Framework HII module does not register Callback although it specify the opcode under
                                  focuse to be INTERRACTIVE.
   @retval EFI_SUCCESS            The callback complete successfully.
   @retval !EFI_SUCCESS           The error code returned by EFI_FORM_CALLBACK_PROTOCOL.Callback.
   
 **/
EFI_STATUS
EFIAPI
ThunkCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_STATUS                                  Status;
  CONFIG_ACCESS_PRIVATE                       *ConfigAccess;
  EFI_FORM_CALLBACK_PROTOCOL                  *FormCallbackProtocol;
  EFI_HII_CALLBACK_PACKET                     *Packet;
  EFI_IFR_DATA_ARRAY                          *Data;
  EFI_IFR_DATA_ENTRY                          *DataEntry;
  UINT16                                      KeyValue;
  ONE_OF_OPTION_MAP_ENTRY                     *OneOfOptionMapEntry;
  EFI_HANDLE                                  NotifyHandle;
  EFI_INPUT_KEY                               Key;  
  BOOLEAN                                     NvMapAllocated;

  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    ASSERT (This != NULL);
    ASSERT (Value != NULL);
    ASSERT (ActionRequest != NULL);

    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;

    ConfigAccess = CONFIG_ACCESS_PRIVATE_FROM_PROTOCOL (This);

    FormCallbackProtocol = ConfigAccess->FormCallbackProtocol;
    if (FormCallbackProtocol == NULL) {
      ASSERT (FALSE);
      return EFI_UNSUPPORTED;
    }

    //
    // Check if the QuestionId match a OneOfOption.
    //
    OneOfOptionMapEntry = GetOneOfOptionMapEntry (ConfigAccess->ThunkContext, QuestionId, Type, Value);

    if (OneOfOptionMapEntry == NULL) {
      //
      // This is not a One-Of-Option opcode. QuestionId is the KeyValue
      //
      KeyValue = QuestionId;
    } else {
      //
      // Otherwise, use the original Key specified in One Of Option in the Framework VFR syntax.
      //
      KeyValue = OneOfOptionMapEntry->FwKey;
    }

    //
    // Build the EFI_IFR_DATA_ARRAY
    //
    Data = CreateIfrDataArray (ConfigAccess, QuestionId, Type, Value, &NvMapAllocated);

    Status = mHiiDatabase->RegisterPackageNotify (
                             mHiiDatabase,
                             EFI_HII_PACKAGE_FORMS,
                             NULL,
                             FormUpdateNotify,
                             EFI_HII_DATABASE_NOTIFY_REMOVE_PACK,
                             &NotifyHandle
                             );
    //
    //Call the Framework Callback function.
    //
    Packet =  NULL;
    Status =  FormCallbackProtocol->Callback (
                FormCallbackProtocol,
                KeyValue,
                Data,
                &Packet
                );
    SyncBrowserDataForNvMapOverride (ConfigAccess, QuestionId);

    //
    // Callback require browser to perform action
    //
    if (EFI_ERROR (Status)) {
      if (Packet != NULL) {
        do {
          CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, Packet->String, NULL);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
      }
      //
      // Error Code in Status is discarded.
      //
    } else {
      if (Packet != NULL) {
          if (Packet->DataArray.EntryCount  == 1 && Packet->DataArray.NvRamMap == NULL) {
            DataEntry = (EFI_IFR_DATA_ENTRY *) ((UINT8 *) Packet + sizeof (EFI_IFR_DATA_ARRAY));
            if ((DataEntry->Flags & EXIT_REQUIRED) == EXIT_REQUIRED) {
                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
            }

            if ((DataEntry->Flags & SAVE_REQUIRED) == SAVE_REQUIRED) {
              Status = ConfigAccess->ConfigAccessProtocol.RouteConfig (
                                        &ConfigAccess->ConfigAccessProtocol,
                                        NULL,
                                        NULL
                                        );
            }
          }
          FreePool (Packet);
      }
    }

    //
    // Unregister notify for Form package update
    //
    Status = mHiiDatabase->UnregisterPackageNotify (
                             mHiiDatabase,
                             NotifyHandle
                             );
    //
    // UEFI SetupBrowser behaves differently with Framework SetupBrowser when call back function 
    // update any forms in HII database. UEFI SetupBrowser will re-parse the displaying form package and load
    // the values from variable storages. Framework SetupBrowser will only re-parse the displaying form packages.
    // To make sure customer's previous changes is saved and the changing question behaves as expected, we
    // issue a EFI_BROWSER_ACTION_REQUEST_SUBMIT to ask UEFI SetupBrowser to save the changes proceed to re-parse
    // the form and load all the variable storages.
    //
    if (*ActionRequest == EFI_BROWSER_ACTION_REQUEST_NONE && mHiiPackageListUpdated) {
      mHiiPackageListUpdated= FALSE;
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
    } else {
      if (ConfigAccess->ThunkContext->FormSet->SubClass == EFI_FRONT_PAGE_SUBCLASS ||
          ConfigAccess->ThunkContext->FormSet->SubClass == EFI_SINGLE_USE_SUBCLASS) {
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
      }
    }

    //
    // Clean up.
    //
    DestroyIfrDataArray (Data, NvMapAllocated);

    return Status;
  }

  //
  // All other action return unsupported.
  //
  return EFI_UNSUPPORTED;
}

