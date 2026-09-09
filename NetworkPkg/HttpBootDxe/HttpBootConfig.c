/** @file
  Helper functions for configuring or getting the parameters relating to HTTP Boot.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpBootDxe.h"
#include <Library/UefiBootManagerLib.h>

CHAR16  mHttpBootConfigStorageName[] = L"HTTP_BOOT_CONFIG_IFR_NVDATA";

/**
  Add new boot option for HTTP boot.

  @param[in]  Private             Pointer to the driver private data.
  @param[in]  UsingIpv6           Set to TRUE if creating boot option for IPv6.
  @param[in]  Description         The description text of the boot option.
  @param[in]  Uri                 The URI string of the boot file.
  @param[in]  ProxyUri            The Proxy URI string for the boot path.

  @retval EFI_SUCCESS             The boot option is created successfully.
  @retval Others                  Failed to create new boot option.

**/
EFI_STATUS
HttpBootAddBootOption (
  IN   HTTP_BOOT_PRIVATE_DATA  *Private,
  IN   BOOLEAN                 UsingIpv6,
  IN   CHAR16                  *Description,
  IN   CHAR16                  *Uri,
  IN   CHAR16                  *ProxyUri
  )
{
  EFI_DEV_PATH                  *Node;
  EFI_DEVICE_PATH_PROTOCOL      *TmpDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *FinalDevicePath;
  UINTN                         Length;
  CHAR8                         AsciiUri[URI_STR_MAX_SIZE];
  CHAR8                         AsciiProxyUri[URI_STR_MAX_SIZE];
  UINTN                         AsciiProxyUriSize;
  EFI_STATUS                    Status;
  EFI_BOOT_MANAGER_LOAD_OPTION  NewOption;

  NewDevicePath   = NULL;
  Node            = NULL;
  TmpDevicePath   = NULL;
  FinalDevicePath = NULL;

  if (StrLen (Description) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check the URI Scheme
  //
  UnicodeStrToAsciiStrS (Uri, AsciiUri, sizeof (AsciiUri));
  UnicodeStrToAsciiStrS (ProxyUri, AsciiProxyUri, sizeof (AsciiProxyUri));
  Status = HttpBootCheckUriScheme (AsciiUri);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_INVALID_PARAMETER) {
      DEBUG ((DEBUG_ERROR, "Error: Invalid URI address.\n"));
    } else if (Status == EFI_ACCESS_DENIED) {
      DEBUG ((DEBUG_ERROR, "Error: Access forbidden, only HTTPS connection is allowed.\n"));
    }

    return Status;
  }

  if (StrLen (ProxyUri) != 0) {
    Status = HttpBootCheckUriScheme (AsciiProxyUri);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_INVALID_PARAMETER) {
        DEBUG ((DEBUG_ERROR, "Error: Invalid URI address.\n"));
      } else if (Status == EFI_ACCESS_DENIED) {
        DEBUG ((DEBUG_ERROR, "Error: Access forbidden, only HTTPS connection is allowed.\n"));
      }

      return Status;
    }
  }

  //
  // Create a new device path by appending the IP node, Proxy node and URI node to
  // the driver's parent device path
  //
  if (!UsingIpv6) {
    Node = AllocateZeroPool (sizeof (IPv4_DEVICE_PATH));
    if (Node == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    Node->Ipv4.Header.Type    = MESSAGING_DEVICE_PATH;
    Node->Ipv4.Header.SubType = MSG_IPv4_DP;
    SetDevicePathNodeLength (Node, sizeof (IPv4_DEVICE_PATH));
  } else {
    Node = AllocateZeroPool (sizeof (IPv6_DEVICE_PATH));
    if (Node == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    Node->Ipv6.Header.Type    = MESSAGING_DEVICE_PATH;
    Node->Ipv6.Header.SubType = MSG_IPv6_DP;
    SetDevicePathNodeLength (Node, sizeof (IPv6_DEVICE_PATH));
  }

  TmpDevicePath = AppendDevicePathNode (Private->ParentDevicePath, (EFI_DEVICE_PATH_PROTOCOL *)Node);
  FreePool (Node);
  if (TmpDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Update the Proxy node with the input Proxy URI
  //
  if (StrLen (ProxyUri) != 0) {
    AsciiProxyUriSize = AsciiStrSize (AsciiProxyUri);
    Length            = sizeof (EFI_DEVICE_PATH_PROTOCOL) + AsciiProxyUriSize;
    Node              = AllocatePool (Length);
    if (Node == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    Node->DevPath.Type    = MESSAGING_DEVICE_PATH;
    Node->DevPath.SubType = MSG_URI_DP;
    SetDevicePathNodeLength (Node, Length);
    CopyMem (
      (UINT8 *)Node + sizeof (EFI_DEVICE_PATH_PROTOCOL),
      AsciiProxyUri,
      AsciiProxyUriSize
      );
    NewDevicePath = AppendDevicePathNode (TmpDevicePath, (EFI_DEVICE_PATH_PROTOCOL *)Node);
    FreePool (Node);
    if (NewDevicePath == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }
  } else {
    NewDevicePath = TmpDevicePath;
  }

  //
  // Update the URI node with the input boot file URI.
  //
  Length = sizeof (EFI_DEVICE_PATH_PROTOCOL) + AsciiStrSize (AsciiUri);
  Node   = AllocatePool (Length);
  if (Node == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Node->DevPath.Type    = MESSAGING_DEVICE_PATH;
  Node->DevPath.SubType = MSG_URI_DP;
  SetDevicePathNodeLength (Node, Length);
  CopyMem ((UINT8 *)Node + sizeof (EFI_DEVICE_PATH_PROTOCOL), AsciiUri, AsciiStrSize (AsciiUri));
  FinalDevicePath = AppendDevicePathNode (NewDevicePath, (EFI_DEVICE_PATH_PROTOCOL *)Node);
  FreePool (Node);
  if (FinalDevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Add a new load option.
  //
  Status = EfiBootManagerInitializeLoadOption (
             &NewOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             LOAD_OPTION_ACTIVE,
             Description,
             FinalDevicePath,
             NULL,
             0
             );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = EfiBootManagerAddLoadOptionVariable (&NewOption, (UINTN)-1);
  EfiBootManagerFreeLoadOption (&NewOption);

ON_EXIT:

  if (TmpDevicePath != NULL) {
    if (TmpDevicePath == NewDevicePath) {
      NewDevicePath = NULL;
    }

    FreePool (TmpDevicePath);
  }

  if (NewDevicePath != NULL) {
    FreePool (NewDevicePath);
  }

  if (FinalDevicePath != NULL) {
    FreePool (FinalDevicePath);
  }

  return Status;
}

/**

  This function allows the caller to request the current
  configuration for one or more named elements. The resulting
  string is in <ConfigAltResp> format. Also, any and all alternative
  configuration strings shall be appended to the end of the
  current configuration string. If they are, they must appear
  after the current configuration. They must contain the same
  routing (GUID, NAME, PATH) as the current configuration string.
  They must have an additional description indicating the type of
  alternative configuration the string represents,
  "ALTCFG=<StringToken>". That <StringToken> (when
  converted from Hex UNICODE to binary) is a reference to a
  string in the associated string pack.

  @param[in]  This       Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.

  @param[in]  Request    A null-terminated Unicode string in
                         <ConfigRequest> format. Note that this
                         includes the routing information as well as
                         the configurable name / value pairs. It is
                         invalid for this string to be in
                         <MultiConfigRequest> format.

  @param[out] Progress   On return, points to a character in the
                         Request string. Points to the string's null
                         terminator if request was successful. Points
                         to the most recent "&" before the first
                         failing name / value pair (or the beginning
                         of the string if the failure is in the first
                         name / value pair) if the request was not successful.

  @param[out] Results    A null-terminated Unicode string in
                         <ConfigAltResp> format which has all values
                         filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval EFI_SUCCESS             The Results string is filled with the
                                  values corresponding to all requested
                                  names.

  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.

  @retval EFI_INVALID_PARAMETER   For example, passing in a NULL
                                  for the Request parameter
                                  would result in this type of
                                  error. In this case, the
                                  Progress parameter would be
                                  set to NULL.

  @retval EFI_NOT_FOUND           Routing data doesn't match any
                                  known driver. Progress set to the
                                  first character in the routing header.
                                  Note: There is no requirement that the
                                  driver validate the routing data. It
                                  must skip the <ConfigHdr> in order to
                                  process the names.

  @retval EFI_INVALID_PARAMETER   Illegal syntax. Progress set
                                  to most recent "&" before the
                                  error or the beginning of the
                                  string.

  @retval EFI_INVALID_PARAMETER   Unknown name. Progress points
                                  to the & before the name in
                                  question.

**/
EFI_STATUS
EFIAPI
HttpBootFormExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  EFI_STATUS                    Status;
  UINTN                         BufferSize;
  HTTP_BOOT_FORM_CALLBACK_INFO  *CallbackInfo;
  EFI_STRING                    ConfigRequestHdr;
  EFI_STRING                    ConfigRequest;
  BOOLEAN                       AllocatedRequest;
  UINTN                         Size;

  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gHttpBootConfigGuid, mHttpBootConfigStorageName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  CallbackInfo = HTTP_BOOT_FORM_CALLBACK_INFO_FROM_CONFIG_ACCESS (This);
  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize = sizeof (HTTP_BOOT_CONFIG_IFR_NVDATA);
  ZeroMem (&CallbackInfo->HttpBootNvData, BufferSize);
  StrCpyS (CallbackInfo->HttpBootNvData.Description, DESCRIPTION_STR_MAX_SIZE / sizeof (CHAR16), HTTP_BOOT_DEFAULT_DESCRIPTION_STR);

  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gHttpBootConfigGuid, mHttpBootConfigStorageName, CallbackInfo->ChildHandle);
    if (ConfigRequestHdr == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Size          = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    if (ConfigRequest == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *)&CallbackInfo->HttpBootNvData,
                                BufferSize,
                                Results,
                                Progress
                                );

  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
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

  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver's name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.

  @param[in]  Configuration  A null-terminated Unicode string in
                             <ConfigString> format.

  @param[out] Progress       A pointer to a string filled in with the
                             offset of the most recent '&' before the
                             first failing name / value pair (or the
                             beginning of the string if the failure
                             is in the first name / value pair) or
                             the terminating NULL if all was
                             successful.

  @retval EFI_SUCCESS             The results have been distributed or are
                                  awaiting distribution.

  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.

  @retval EFI_INVALID_PARAMETERS  Passing in a NULL for the
                                  Results parameter would result
                                  in this type of error.

  @retval EFI_NOT_FOUND           Target for the specified routing data
                                  was not found.

**/
EFI_STATUS
EFIAPI
HttpBootFormRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  EFI_STATUS                    Status;
  UINTN                         BufferSize;
  HTTP_BOOT_FORM_CALLBACK_INFO  *CallbackInfo;
  HTTP_BOOT_PRIVATE_DATA        *Private;

  if (Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: there is no name for Name/Value storage, only GUID will be checked
  //
  if (!HiiIsConfigHdrMatch (Configuration, &gHttpBootConfigGuid, mHttpBootConfigStorageName)) {
    return EFI_NOT_FOUND;
  }

  CallbackInfo = HTTP_BOOT_FORM_CALLBACK_INFO_FROM_CONFIG_ACCESS (This);
  Private      = HTTP_BOOT_PRIVATE_DATA_FROM_CALLBACK_INFO (CallbackInfo);

  BufferSize = sizeof (HTTP_BOOT_CONFIG_IFR_NVDATA);
  ZeroMem (&CallbackInfo->HttpBootNvData, BufferSize);

  Status = gHiiConfigRouting->ConfigToBlock (
                                gHiiConfigRouting,
                                Configuration,
                                (UINT8 *)&CallbackInfo->HttpBootNvData,
                                &BufferSize,
                                Progress
                                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create a new boot option according to the configuration data.
  //
  HttpBootAddBootOption (
    Private,
    (CallbackInfo->HttpBootNvData.IpVersion == HTTP_BOOT_IP_VERSION_6) ? TRUE : FALSE,
    CallbackInfo->HttpBootNvData.Description,
    CallbackInfo->HttpBootNvData.Uri,
    CallbackInfo->HttpBootNvData.ProxyUri
    );

  return EFI_SUCCESS;
}

/**

  This function is called to provide results data to the driver.
  This data consists of a unique key that is used to identify
  which data is either being passed back or being asked for.

  @param[in]       This          Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]       Action        Specifies the type of action taken by the browser.
  @param[in]       QuestionId    A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect. The format of the data tends to
                                 vary based on the opcode that generated the callback.
  @param[in]       Type          The type of value for the question.
  @param[in, out]  Value         A pointer to the data being sent to the original
                                 exporting driver.
  @param[out]      ActionRequest On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.
**/
EFI_STATUS
EFIAPI
HttpBootFormCallback (
  IN CONST  EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN        EFI_BROWSER_ACTION              Action,
  IN        EFI_QUESTION_ID                 QuestionId,
  IN        UINT8                           Type,
  IN OUT    EFI_IFR_TYPE_VALUE              *Value,
  OUT       EFI_BROWSER_ACTION_REQUEST      *ActionRequest
  )
{
  EFI_INPUT_KEY                 Key;
  CHAR16                        *Uri;
  UINTN                         UriLen;
  CHAR8                         *AsciiUri;
  HTTP_BOOT_FORM_CALLBACK_INFO  *CallbackInfo;
  EFI_STATUS                    Status;

  Uri      = NULL;
  UriLen   = 0;
  AsciiUri = NULL;
  Status   = EFI_SUCCESS;

  if ((This == NULL) || (Value == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  CallbackInfo = HTTP_BOOT_FORM_CALLBACK_INFO_FROM_CONFIG_ACCESS (This);

  if (Action != EFI_BROWSER_ACTION_CHANGING) {
    return EFI_UNSUPPORTED;
  }

  switch (QuestionId) {
    case KEY_INITIATOR_URI:
    case KEY_INITIATOR_PROXY_URI:
      //
      // Get user input URI string
      //
      Uri = HiiGetString (CallbackInfo->RegisteredHandle, Value->string, NULL);
      if (Uri == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      //
      // The URI should be either an empty string (for corporate environment) ,or http(s) for home environment.
      // Pop up a message box for the unsupported URI.
      //
      if (StrLen (Uri) != 0) {
        UriLen   = StrLen (Uri) + 1;
        AsciiUri = AllocateZeroPool (UriLen);
        if (AsciiUri == NULL) {
          FreePool (Uri);
          return EFI_OUT_OF_RESOURCES;
        }

        UnicodeStrToAsciiStrS (Uri, AsciiUri, UriLen);

        Status = HttpBootCheckUriScheme (AsciiUri);

        if (Status == EFI_INVALID_PARAMETER) {
          DEBUG ((DEBUG_ERROR, "HttpBootFormCallback: %r.\n", Status));

          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"ERROR: Unsupported URI!",
            L"Only supports HTTP and HTTPS",
            NULL
            );
        } else if (Status == EFI_ACCESS_DENIED) {
          DEBUG ((DEBUG_ERROR, "HttpBootFormCallback: %r.\n", Status));

          CreatePopUp (
            EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
            &Key,
            L"ERROR: Unsupported URI!",
            L"HTTP is disabled",
            NULL
            );
        }
      }

      if (Uri != NULL) {
        FreePool (Uri);
      }

      if (AsciiUri != NULL) {
        FreePool (AsciiUri);
      }

      break;

    default:
      break;
  }

  return Status;
}

/**
  Initialize the configuration form.

  @param[in]  Private             Pointer to the driver private data.

  @retval EFI_SUCCESS             The configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.

**/
EFI_STATUS
HttpBootConfigFormInit (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS                    Status;
  HTTP_BOOT_FORM_CALLBACK_INFO  *CallbackInfo;
  VENDOR_DEVICE_PATH            VendorDeviceNode;
  CHAR16                        *MacString;
  CHAR16                        *OldMenuString;
  CHAR16                        MenuString[128];

  CallbackInfo = &Private->CallbackInfo;

  if (CallbackInfo->Initialized) {
    return EFI_SUCCESS;
  }

  CallbackInfo->Signature = HTTP_BOOT_FORM_CALLBACK_INFO_SIGNATURE;

  //
  // Construct device path node for EFI HII Config Access protocol,
  // which consists of controller physical device path and one hardware
  // vendor guid node.
  //
  ZeroMem (&VendorDeviceNode, sizeof (VENDOR_DEVICE_PATH));
  VendorDeviceNode.Header.Type    = HARDWARE_DEVICE_PATH;
  VendorDeviceNode.Header.SubType = HW_VENDOR_DP;
  CopyGuid (&VendorDeviceNode.Guid, &gEfiCallerIdGuid);
  SetDevicePathNodeLength (&VendorDeviceNode.Header, sizeof (VENDOR_DEVICE_PATH));
  CallbackInfo->HiiVendorDevicePath = AppendDevicePathNode (
                                        Private->ParentDevicePath,
                                        (EFI_DEVICE_PATH_PROTOCOL *)&VendorDeviceNode
                                        );
  if (CallbackInfo->HiiVendorDevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  CallbackInfo->ConfigAccess.ExtractConfig = HttpBootFormExtractConfig;
  CallbackInfo->ConfigAccess.RouteConfig   = HttpBootFormRouteConfig;
  CallbackInfo->ConfigAccess.Callback      = HttpBootFormCallback;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &CallbackInfo->ChildHandle,
                  &gEfiDevicePathProtocolGuid,
                  CallbackInfo->HiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &CallbackInfo->ConfigAccess,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Publish our HII data.
  //
  CallbackInfo->RegisteredHandle = HiiAddPackages (
                                     &gHttpBootConfigGuid,
                                     CallbackInfo->ChildHandle,
                                     HttpBootDxeStrings,
                                     HttpBootConfigVfrBin,
                                     NULL
                                     );
  if (CallbackInfo->RegisteredHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  //
  // Append MAC string in the menu help string
  //
  Status = NetLibGetMacString (Private->Controller, NULL, &MacString);
  if (!EFI_ERROR (Status)) {
    OldMenuString = HiiGetString (
                      CallbackInfo->RegisteredHandle,
                      STRING_TOKEN (STR_HTTP_BOOT_CONFIG_FORM_HELP),
                      NULL
                      );
    if (OldMenuString != NULL) {
      UnicodeSPrint (MenuString, 128, L"%s (MAC:%s)", OldMenuString, MacString);
      HiiSetString (
        CallbackInfo->RegisteredHandle,
        STRING_TOKEN (STR_HTTP_BOOT_CONFIG_FORM_HELP),
        MenuString,
        NULL
        );

      FreePool (OldMenuString);
    }

    FreePool (MacString);

    CallbackInfo->Initialized = TRUE;
    return EFI_SUCCESS;
  }

Error:

  HttpBootConfigFormUnload (Private);
  return Status;
}

/**
  Unload the configuration form, this includes: delete all the configuration
  entries, uninstall the form callback protocol, and free the resources used.
  The form will only be unload completely when both IP4 and IP6 stack are stopped.

  @param[in]  Private             Pointer to the driver private data.

  @retval EFI_SUCCESS             The configuration form is unloaded.
  @retval Others                  Failed to unload the form.

**/
EFI_STATUS
HttpBootConfigFormUnload (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  )
{
  HTTP_BOOT_FORM_CALLBACK_INFO  *CallbackInfo;

  if ((Private->Ip4Nic != NULL) || (Private->Ip6Nic != NULL)) {
    //
    // Only unload the configuration form when both IP4 and IP6 stack are stopped.
    //
    return EFI_SUCCESS;
  }

  CallbackInfo = &Private->CallbackInfo;
  if (CallbackInfo->ChildHandle != NULL) {
    //
    // Uninstall EFI_HII_CONFIG_ACCESS_PROTOCOL
    //
    gBS->UninstallMultipleProtocolInterfaces (
           CallbackInfo->ChildHandle,
           &gEfiDevicePathProtocolGuid,
           CallbackInfo->HiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &CallbackInfo->ConfigAccess,
           NULL
           );
    CallbackInfo->ChildHandle = NULL;
  }

  if (CallbackInfo->HiiVendorDevicePath != NULL) {
    FreePool (CallbackInfo->HiiVendorDevicePath);
    CallbackInfo->HiiVendorDevicePath = NULL;
  }

  if (CallbackInfo->RegisteredHandle != NULL) {
    //
    // Remove HII package list
    //
    HiiRemovePackages (CallbackInfo->RegisteredHandle);
    CallbackInfo->RegisteredHandle = NULL;
  }

  return EFI_SUCCESS;
}
