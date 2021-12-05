/** @file

  The implementation of EFI REST Resource JSON to C structure convertor
  Protocol.

  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Protocol/RestJsonStructure.h>
#include "RestJsonStructureInternal.h"

LIST_ENTRY  mRestJsonStructureList;
EFI_HANDLE  mProtocolHandle;

/**
  This function registers Restful resource interpreter for the
  specific schema.

  @param[in]    This                     This is the EFI_REST_JSON_STRUCTURE_PROTOCOL instance.
  @param[in]    JsonStructureSupported   The type and version of REST JSON resource which this converter
                                         supports.
  @param[in]    ToStructure              The function to convert REST JSON resource to structure.
  @param[in]    ToJson                   The function to convert REST JSON structure to JSON in text format.
  @param[in]    DestroyStructure         Destroy REST JSON structure returned in ToStructure()  function.

  @retval EFI_SUCCESS             Register successfully.
  @retval Others                  Fail to register.

**/
EFI_STATUS
EFIAPI
RestJsonStructureRegister (
  IN EFI_REST_JSON_STRUCTURE_PROTOCOL           *This,
  IN EFI_REST_JSON_STRUCTURE_SUPPORTED          *JsonStructureSupported,
  IN EFI_REST_JSON_STRUCTURE_TO_STRUCTURE       ToStructure,
  IN EFI_REST_JSON_STRUCTURE_TO_JSON            ToJson,
  IN EFI_REST_JSON_STRUCTURE_DESTORY_STRUCTURE  DestroyStructure
  )
{
  UINTN                                   NumberOfNS;
  UINTN                                   Index;
  LIST_ENTRY                              *ThisList;
  REST_JSON_STRUCTURE_INSTANCE            *Instance;
  EFI_REST_JSON_RESOURCE_TYPE_IDENTIFIER  *CloneSupportedInterpId;
  EFI_REST_JSON_STRUCTURE_SUPPORTED       *ThisSupportedInterp;

  if ((This == NULL) ||
      (ToStructure == NULL) ||
      (ToJson == NULL) ||
      (DestroyStructure == NULL) ||
      (JsonStructureSupported == NULL)
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check how many name space interpreter can interpret.
  //
  ThisList   = &JsonStructureSupported->NextSupportedRsrcInterp;
  NumberOfNS = 1;
  while (TRUE) {
    if (ThisList->ForwardLink == &JsonStructureSupported->NextSupportedRsrcInterp) {
      break;
    } else {
      ThisList = ThisList->ForwardLink;
      NumberOfNS++;
    }
  }

  Instance =
    (REST_JSON_STRUCTURE_INSTANCE *)AllocateZeroPool (sizeof (REST_JSON_STRUCTURE_INSTANCE) + NumberOfNS * sizeof (EFI_REST_JSON_RESOURCE_TYPE_IDENTIFIER));
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&Instance->NextRestJsonStructureInstance);
  Instance->NumberOfNameSpaceToConvert = NumberOfNS;
  Instance->SupportedRsrcIndentifier   = (EFI_REST_JSON_RESOURCE_TYPE_IDENTIFIER *)((REST_JSON_STRUCTURE_INSTANCE *)Instance + 1);
  //
  // Copy supported resource identifer interpreter.
  //
  CloneSupportedInterpId = Instance->SupportedRsrcIndentifier;
  ThisSupportedInterp    = JsonStructureSupported;
  for (Index = 0; Index < NumberOfNS; Index++) {
    CopyMem ((VOID *)CloneSupportedInterpId, (VOID *)&ThisSupportedInterp->RestResourceInterp, sizeof (EFI_REST_JSON_RESOURCE_TYPE_IDENTIFIER));
    ThisSupportedInterp = (EFI_REST_JSON_STRUCTURE_SUPPORTED *)ThisSupportedInterp->NextSupportedRsrcInterp.ForwardLink;
    CloneSupportedInterpId++;
  }

  Instance->JsonToStructure  = ToStructure;
  Instance->StructureToJson  = ToJson;
  Instance->DestroyStructure = DestroyStructure;
  InsertTailList (&mRestJsonStructureList, &Instance->NextRestJsonStructureInstance);
  return EFI_SUCCESS;
}

/**
  This function check if this interpreter instance support the given namesapce.

  @param[in]    This                EFI_REST_JSON_STRUCTURE_PROTOCOL instance.
  @param[in]    InterpreterInstance REST_JSON_STRUCTURE_INSTANCE
  @param[in]    RsrcTypeIdentifier  Resource type identifier.
  @param[in]    ResourceRaw         Given Restful resource.
  @param[out]   RestJSonHeader      Property interpreted from given ResourceRaw.

  @retval EFI_SUCCESS
  @retval Others.

**/
EFI_STATUS
InterpreterInstanceToStruct (
  IN EFI_REST_JSON_STRUCTURE_PROTOCOL        *This,
  IN REST_JSON_STRUCTURE_INSTANCE            *InterpreterInstance,
  IN EFI_REST_JSON_RESOURCE_TYPE_IDENTIFIER  *RsrcTypeIdentifier OPTIONAL,
  IN CHAR8                                   *ResourceRaw,
  OUT EFI_REST_JSON_STRUCTURE_HEADER         **RestJSonHeader
  )
{
  UINTN                                   Index;
  EFI_STATUS                              Status;
  EFI_REST_JSON_RESOURCE_TYPE_IDENTIFIER  *ThisSupportedRsrcTypeId;

  if ((This == NULL) ||
      (InterpreterInstance == NULL) ||
      (ResourceRaw == NULL) ||
      (RestJSonHeader == NULL)
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_UNSUPPORTED;
  if (RsrcTypeIdentifier == NULL) {
    //
    // No resource type identifier, send to intepreter anyway.
    // Interpreter may recognize this resource.
    //
    Status = InterpreterInstance->JsonToStructure (
                                    This,
                                    NULL,
                                    ResourceRaw,
                                    RestJSonHeader
                                    );
  } else {
    //
    // Check if the namesapce and version is supported by this interpreter.
    //
    ThisSupportedRsrcTypeId = InterpreterInstance->SupportedRsrcIndentifier;
    for (Index = 0; Index < InterpreterInstance->NumberOfNameSpaceToConvert; Index++) {
      if (AsciiStrCmp (
            RsrcTypeIdentifier->NameSpace.ResourceTypeName,
            ThisSupportedRsrcTypeId->NameSpace.ResourceTypeName
            ) == 0)
      {
        if ((RsrcTypeIdentifier->NameSpace.MajorVersion == NULL) &&
            (RsrcTypeIdentifier->NameSpace.MinorVersion == NULL) &&
            (RsrcTypeIdentifier->NameSpace.ErrataVersion == NULL)
            )
        {
          //
          // Don't check version of this resource type identifier.
          //
          Status = InterpreterInstance->JsonToStructure (
                                          This,
                                          RsrcTypeIdentifier,
                                          ResourceRaw,
                                          RestJSonHeader
                                          );
          break;
        } else {
          //
          // Check version.
          //
          if ((AsciiStrCmp (
                 RsrcTypeIdentifier->NameSpace.MajorVersion,
                 ThisSupportedRsrcTypeId->NameSpace.MajorVersion
                 ) == 0) &&
              (AsciiStrCmp (
                 RsrcTypeIdentifier->NameSpace.MinorVersion,
                 ThisSupportedRsrcTypeId->NameSpace.MinorVersion
                 ) == 0) &&
              (AsciiStrCmp (
                 RsrcTypeIdentifier->NameSpace.ErrataVersion,
                 ThisSupportedRsrcTypeId->NameSpace.ErrataVersion
                 ) == 0))
          {
            Status = InterpreterInstance->JsonToStructure (
                                            This,
                                            RsrcTypeIdentifier,
                                            ResourceRaw,
                                            RestJSonHeader
                                            );
            break;
          }
        }
      }

      ThisSupportedRsrcTypeId++;
    }
  }

  return Status;
}

/**
  This function converts JSON C structure to JSON property.

  @param[in]    This                 EFI_REST_JSON_STRUCTURE_PROTOCOL instance.
  @param[in]    InterpreterInstance  REST_JSON_STRUCTURE_INSTANCE
  @param[in]    RestJSonHeader       Resource type identifier.
  @param[out]   ResourceRaw          Output in JSON text format.

  @retval EFI_SUCCESS
  @retval Others.

**/
EFI_STATUS
InterpreterEfiStructToInstance (
  IN EFI_REST_JSON_STRUCTURE_PROTOCOL  *This,
  IN REST_JSON_STRUCTURE_INSTANCE      *InterpreterInstance,
  IN EFI_REST_JSON_STRUCTURE_HEADER    *RestJSonHeader,
  OUT CHAR8                            **ResourceRaw
  )
{
  UINTN                                   Index;
  EFI_STATUS                              Status;
  EFI_REST_JSON_RESOURCE_TYPE_IDENTIFIER  *ThisSupportedRsrcTypeId;
  EFI_REST_JSON_RESOURCE_TYPE_IDENTIFIER  *RsrcTypeIdentifier;

  if ((This == NULL) ||
      (InterpreterInstance == NULL) ||
      (RestJSonHeader == NULL) ||
      (ResourceRaw == NULL)
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  RsrcTypeIdentifier = &RestJSonHeader->JsonRsrcIdentifier;
  if ((RsrcTypeIdentifier == NULL) ||
      (RsrcTypeIdentifier->NameSpace.ResourceTypeName == NULL) ||
      (RsrcTypeIdentifier->NameSpace.MajorVersion == NULL) ||
      (RsrcTypeIdentifier->NameSpace.MinorVersion == NULL) ||
      (RsrcTypeIdentifier->NameSpace.ErrataVersion == NULL)
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check if the namesapce and version is supported by this interpreter.
  //
  Status                  = EFI_UNSUPPORTED;
  ThisSupportedRsrcTypeId = InterpreterInstance->SupportedRsrcIndentifier;
  for (Index = 0; Index < InterpreterInstance->NumberOfNameSpaceToConvert; Index++) {
    if (AsciiStrCmp (
          RsrcTypeIdentifier->NameSpace.ResourceTypeName,
          ThisSupportedRsrcTypeId->NameSpace.ResourceTypeName
          ) == 0)
    {
      //
      // Check version.
      //
      if ((AsciiStrCmp (
             RsrcTypeIdentifier->NameSpace.MajorVersion,
             ThisSupportedRsrcTypeId->NameSpace.MajorVersion
             ) == 0) &&
          (AsciiStrCmp (
             RsrcTypeIdentifier->NameSpace.MinorVersion,
             ThisSupportedRsrcTypeId->NameSpace.MinorVersion
             ) == 0) &&
          (AsciiStrCmp (
             RsrcTypeIdentifier->NameSpace.ErrataVersion,
             ThisSupportedRsrcTypeId->NameSpace.ErrataVersion
             ) == 0))
      {
        Status = InterpreterInstance->StructureToJson (
                                        This,
                                        RestJSonHeader,
                                        ResourceRaw
                                        );
        break;
      }
    }

    ThisSupportedRsrcTypeId++;
  }

  return Status;
}

/**
  This function destory REST property structure.

  @param[in]    This                 EFI_REST_JSON_STRUCTURE_PROTOCOL instance.
  @param[in]    InterpreterInstance  REST_JSON_STRUCTURE_INSTANCE
  @param[in]    RestJSonHeader       Property interpreted from given ResourceRaw.

  @retval EFI_SUCCESS
  @retval Others.

**/
EFI_STATUS
InterpreterInstanceDestoryJsonStruct (
  IN EFI_REST_JSON_STRUCTURE_PROTOCOL  *This,
  IN REST_JSON_STRUCTURE_INSTANCE      *InterpreterInstance,
  IN EFI_REST_JSON_STRUCTURE_HEADER    *RestJSonHeader
  )
{
  UINTN                                   Index;
  EFI_STATUS                              Status;
  EFI_REST_JSON_RESOURCE_TYPE_IDENTIFIER  *ThisSupportedRsrcTypeId;

  if ((This == NULL) ||
      (InterpreterInstance == NULL) ||
      (RestJSonHeader == NULL)
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_UNSUPPORTED;
  //
  // Check if the namesapce and version is supported by this interpreter.
  //
  ThisSupportedRsrcTypeId = InterpreterInstance->SupportedRsrcIndentifier;
  for (Index = 0; Index < InterpreterInstance->NumberOfNameSpaceToConvert; Index++) {
    if (AsciiStrCmp (
          RestJSonHeader->JsonRsrcIdentifier.NameSpace.ResourceTypeName,
          ThisSupportedRsrcTypeId->NameSpace.ResourceTypeName
          ) == 0)
    {
      if ((RestJSonHeader->JsonRsrcIdentifier.NameSpace.MajorVersion == NULL) &&
          (RestJSonHeader->JsonRsrcIdentifier.NameSpace.MinorVersion == NULL) &&
          (RestJSonHeader->JsonRsrcIdentifier.NameSpace.ErrataVersion == NULL)
          )
      {
        //
        // Don't check version of this resource type identifier.
        //
        Status = InterpreterInstance->DestroyStructure (
                                        This,
                                        RestJSonHeader
                                        );
        break;
      } else {
        //
        // Check version.
        //
        if ((AsciiStrCmp (
               RestJSonHeader->JsonRsrcIdentifier.NameSpace.MajorVersion,
               ThisSupportedRsrcTypeId->NameSpace.MajorVersion
               ) == 0) &&
            (AsciiStrCmp (
               RestJSonHeader->JsonRsrcIdentifier.NameSpace.MinorVersion,
               ThisSupportedRsrcTypeId->NameSpace.MinorVersion
               ) == 0) &&
            (AsciiStrCmp (
               RestJSonHeader->JsonRsrcIdentifier.NameSpace.ErrataVersion,
               ThisSupportedRsrcTypeId->NameSpace.ErrataVersion
               ) == 0))
        {
          Status = InterpreterInstance->DestroyStructure (
                                          This,
                                          RestJSonHeader
                                          );
          break;
        }
      }
    }

    ThisSupportedRsrcTypeId++;
  }

  return Status;
}

/**
  This function translates the given JSON text to JSON C Structure.

  @param[in]    This                EFI_REST_JSON_STRUCTURE_PROTOCOL instance.
  @param[in]    RsrcTypeIdentifier  Resource type identifier.
  @param[in]    ResourceJsonText    Given Restful resource.
  @param[out]   JsonStructure       Property interpreted from given ResourceRaw.

  @retval EFI_SUCCESS
  @retval Others.

**/
EFI_STATUS
EFIAPI
RestJsonStructureToStruct (
  IN EFI_REST_JSON_STRUCTURE_PROTOCOL        *This,
  IN EFI_REST_JSON_RESOURCE_TYPE_IDENTIFIER  *RsrcTypeIdentifier OPTIONAL,
  IN CHAR8                                   *ResourceJsonText,
  OUT EFI_REST_JSON_STRUCTURE_HEADER         **JsonStructure
  )
{
  EFI_STATUS                    Status;
  REST_JSON_STRUCTURE_INSTANCE  *Instance;

  if ((This == NULL) ||
      (ResourceJsonText == NULL) ||
      (JsonStructure == NULL)
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  if (IsListEmpty (&mRestJsonStructureList)) {
    return EFI_UNSUPPORTED;
  }

  Status   = EFI_SUCCESS;
  Instance = (REST_JSON_STRUCTURE_INSTANCE *)GetFirstNode (&mRestJsonStructureList);
  while (TRUE) {
    Status = InterpreterInstanceToStruct (
               This,
               Instance,
               RsrcTypeIdentifier,
               ResourceJsonText,
               JsonStructure
               );
    if (!EFI_ERROR (Status)) {
      break;
    }

    if (IsNodeAtEnd (&mRestJsonStructureList, &Instance->NextRestJsonStructureInstance)) {
      Status = EFI_UNSUPPORTED;
      break;
    }

    Instance = (REST_JSON_STRUCTURE_INSTANCE *)GetNextNode (&mRestJsonStructureList, &Instance->NextRestJsonStructureInstance);
  }

  return Status;
}

/**
  This function destory REST property EFI structure which returned in
  JsonToStructure().

  @param[in]    This            EFI_REST_JSON_STRUCTURE_PROTOCOL instance.
  @param[in]    RestJSonHeader  Property to destory.

  @retval EFI_SUCCESS
  @retval Others

**/
EFI_STATUS
EFIAPI
RestJsonStructureDestroyStruct (
  IN EFI_REST_JSON_STRUCTURE_PROTOCOL  *This,
  IN EFI_REST_JSON_STRUCTURE_HEADER    *RestJSonHeader
  )
{
  EFI_STATUS                    Status;
  REST_JSON_STRUCTURE_INSTANCE  *Instance;

  if ((This == NULL) || (RestJSonHeader == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsListEmpty (&mRestJsonStructureList)) {
    return EFI_UNSUPPORTED;
  }

  Status   = EFI_SUCCESS;
  Instance = (REST_JSON_STRUCTURE_INSTANCE *)GetFirstNode (&mRestJsonStructureList);
  while (TRUE) {
    Status = InterpreterInstanceDestoryJsonStruct (
               This,
               Instance,
               RestJSonHeader
               );
    if (!EFI_ERROR (Status)) {
      break;
    }

    if (IsNodeAtEnd (&mRestJsonStructureList, &Instance->NextRestJsonStructureInstance)) {
      Status = EFI_UNSUPPORTED;
      break;
    }

    Instance = (REST_JSON_STRUCTURE_INSTANCE *)GetNextNode (&mRestJsonStructureList, &Instance->NextRestJsonStructureInstance);
  }

  return Status;
}

/**
  This function translates the given JSON C Structure to JSON text.

  @param[in]    This            EFI_REST_JSON_STRUCTURE_PROTOCOL instance.
  @param[in]    RestJSonHeader  Given Restful resource.
  @param[out]   ResourceRaw     Resource in RESTfuls service oriented.

  @retval EFI_SUCCESS
  @retval Others             Fail to remove the entry

**/
EFI_STATUS
EFIAPI
RestJsonStructureToJson (
  IN EFI_REST_JSON_STRUCTURE_PROTOCOL  *This,
  IN EFI_REST_JSON_STRUCTURE_HEADER    *RestJSonHeader,
  OUT CHAR8                            **ResourceRaw
  )
{
  EFI_STATUS                    Status;
  REST_JSON_STRUCTURE_INSTANCE  *Instance;

  if ((This == NULL) || (RestJSonHeader == NULL) || (ResourceRaw == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsListEmpty (&mRestJsonStructureList)) {
    return EFI_UNSUPPORTED;
  }

  Status   = EFI_SUCCESS;
  Instance = (REST_JSON_STRUCTURE_INSTANCE *)GetFirstNode (&mRestJsonStructureList);
  while (TRUE) {
    Status = InterpreterEfiStructToInstance (
               This,
               Instance,
               RestJSonHeader,
               ResourceRaw
               );
    if (!EFI_ERROR (Status)) {
      break;
    }

    if (IsNodeAtEnd (&mRestJsonStructureList, &Instance->NextRestJsonStructureInstance)) {
      Status = EFI_UNSUPPORTED;
      break;
    }

    Instance = (REST_JSON_STRUCTURE_INSTANCE *)GetNextNode (&mRestJsonStructureList, &Instance->NextRestJsonStructureInstance);
  }

  return Status;
}

EFI_REST_JSON_STRUCTURE_PROTOCOL  mRestJsonStructureProtocol = {
  RestJsonStructureRegister,
  RestJsonStructureToStruct,
  RestJsonStructureToJson,
  RestJsonStructureDestroyStruct
};

/**
  This is the declaration of an EFI image entry point.

  @param  ImageHandle           The firmware allocated handle for the UEFI image.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
RestJsonStructureEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  InitializeListHead (&mRestJsonStructureList);
  //
  // Install the Restful Resource Interpreter Protocol.
  //
  mProtocolHandle = NULL;
  Status          = gBS->InstallProtocolInterface (
                           &mProtocolHandle,
                           &gEfiRestJsonStructureProtocolGuid,
                           EFI_NATIVE_INTERFACE,
                           (VOID *)&mRestJsonStructureProtocol
                           );
  return Status;
}

/**
  This is the unload handle for Redfish discover module.

  Disconnect the driver specified by ImageHandle from all the devices in the handle database.
  Uninstall all the protocols installed in the driver entry point.

  @param[in] ImageHandle           The drivers' driver image.

  @retval    EFI_SUCCESS           The image is unloaded.
  @retval    Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
RestJsonStructureUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                    Status;
  REST_JSON_STRUCTURE_INSTANCE  *Instance;
  REST_JSON_STRUCTURE_INSTANCE  *NextInstance;

  Status = gBS->UninstallProtocolInterface (
                  mProtocolHandle,
                  &gEfiRestJsonStructureProtocolGuid,
                  (VOID *)&mRestJsonStructureProtocol
                  );

  if (IsListEmpty (&mRestJsonStructureList)) {
    return Status;
  }

  //
  // Free memory of REST_JSON_STRUCTURE_INSTANCE instance.
  //
  Instance = (REST_JSON_STRUCTURE_INSTANCE *)GetFirstNode (&mRestJsonStructureList);
  do {
    NextInstance = NULL;
    if (!IsNodeAtEnd (&mRestJsonStructureList, &Instance->NextRestJsonStructureInstance)) {
      NextInstance = (REST_JSON_STRUCTURE_INSTANCE *)GetNextNode (&mRestJsonStructureList, &Instance->NextRestJsonStructureInstance);
    }

    FreePool ((VOID *)Instance);
    Instance = NextInstance;
  } while (Instance != NULL);

  return Status;
}
