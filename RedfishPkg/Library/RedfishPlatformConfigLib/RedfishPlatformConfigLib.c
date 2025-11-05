/** @file
  Wrapper function to support Redfish Platform Config protocol.

  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishPlatformConfigInternal.h"

REDFISH_PLATFORM_CONFIG_LIB_PRIVATE  mRedfishPlatformConfigLibPrivate;

/**
  Get Redfish value with the given Schema and Configure Language.

  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[out]  Value               The returned value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval EFI_NOT_READY            Redfish Platform Config protocol is not ready.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishPlatformConfigGetValue (
  IN     CHAR8                *Schema,
  IN     CHAR8                *Version,
  IN     EFI_STRING           ConfigureLang,
  OUT    EDKII_REDFISH_VALUE  *Value
  )
{
  if (mRedfishPlatformConfigLibPrivate.Protocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishPlatformConfigLibPrivate.Protocol->GetValue (
                                                      mRedfishPlatformConfigLibPrivate.Protocol,
                                                      Schema,
                                                      Version,
                                                      ConfigureLang,
                                                      Value
                                                      );
}

/**
  Get Redfish attribute value with the given Schema and Configure Language.

  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[out]  AttributeValue      The attribute value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishPlatformConfigGetAttribute (
  IN     CHAR8                    *Schema,
  IN     CHAR8                    *Version,
  IN     EFI_STRING               ConfigureLang,
  OUT    EDKII_REDFISH_ATTRIBUTE  *AttributeValue
  )
{
  if (mRedfishPlatformConfigLibPrivate.Protocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishPlatformConfigLibPrivate.Protocol->GetAttribute (
                                                      mRedfishPlatformConfigLibPrivate.Protocol,
                                                      Schema,
                                                      Version,
                                                      ConfigureLang,
                                                      AttributeValue
                                                      );
}

/**
  Get Redfish default value with the given Schema and Configure Language.

  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[in]   DefaultClass        The UEFI defined default class.
                                   Please refer to UEFI spec. 33.2.5.8 "defaults" for details.
  @param[out]  Value               The returned value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishPlatformConfigGetDefaultValue (
  IN     CHAR8                *Schema,
  IN     CHAR8                *Version,
  IN     EFI_STRING           ConfigureLang,
  IN     UINT16               DefaultClass,
  OUT    EDKII_REDFISH_VALUE  *Value
  )
{
  if (mRedfishPlatformConfigLibPrivate.Protocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishPlatformConfigLibPrivate.Protocol->GetDefaultValue (
                                                      mRedfishPlatformConfigLibPrivate.Protocol,
                                                      Schema,
                                                      Version,
                                                      ConfigureLang,
                                                      DefaultClass,
                                                      Value
                                                      );
}

/**
  Set Redfish value with the given Schema and Configure Language.

  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[in]   Value               The value to set.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval EFI_NOT_READY            Redfish Platform Config protocol is not ready.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishPlatformConfigSetValue (
  IN     CHAR8                *Schema,
  IN     CHAR8                *Version,
  IN     EFI_STRING           ConfigureLang,
  IN     EDKII_REDFISH_VALUE  Value
  )
{
  if (mRedfishPlatformConfigLibPrivate.Protocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishPlatformConfigLibPrivate.Protocol->SetValue (
                                                      mRedfishPlatformConfigLibPrivate.Protocol,
                                                      Schema,
                                                      Version,
                                                      ConfigureLang,
                                                      Value
                                                      );
}

/**
  Get the list of Configure Language from platform configuration by the given Schema and Pattern.

  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   Pattern             The target Configure Language pattern.
  @param[out]  ConfigureLangList   The list of Configure Language.
  @param[out]  Count               The number of Configure Language in ConfigureLangList.

  @retval EFI_SUCCESS              ConfigureLangList is returned successfully.
  @retval EFI_NOT_READY            Redfish Platform Config protocol is not ready.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishPlatformConfigGetConfigureLang (
  IN     CHAR8       *Schema,
  IN     CHAR8       *Version,
  IN     EFI_STRING  Pattern,
  OUT    EFI_STRING  **ConfigureLangList,
  OUT    UINTN       *Count
  )
{
  if (mRedfishPlatformConfigLibPrivate.Protocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishPlatformConfigLibPrivate.Protocol->GetConfigureLang (
                                                      mRedfishPlatformConfigLibPrivate.Protocol,
                                                      Schema,
                                                      Version,
                                                      Pattern,
                                                      ConfigureLangList,
                                                      Count
                                                      );
}

/**
  Get the list of supported Redfish schema from platform configuration.

  @param[out]  SupportedSchema     The supported schema list which is separated by ';'.
                                   For example: "x-UEFI-redfish-Memory.v1_7_1;x-UEFI-redfish-Boot.v1_0_1"
                                   The SupportedSchema is allocated by the callee. It's caller's
                                   responsibility to free this buffer using FreePool().

  @retval EFI_SUCCESS              Schema is returned successfully.
  @retval EFI_NOT_READY            Redfish Platform Config protocol is not ready.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigGetSupportedSchema (
  OUT    CHAR8  **SupportedSchema
  )
{
  if (mRedfishPlatformConfigLibPrivate.Protocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishPlatformConfigLibPrivate.Protocol->GetSupportedSchema (
                                                      mRedfishPlatformConfigLibPrivate.Protocol,
                                                      SupportedSchema
                                                      );
}

/**
  This is a EFI_REDFISH_PLATFORM_CONFIG_PROTOCOL notification event handler.

  Install HII package notification.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
RedfishPlatformConfigProtocolInstalled (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;

  //
  // Locate EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL.
  //
  Status = gBS->LocateProtocol (
                  &gEdkIIRedfishPlatformConfigProtocolGuid,
                  NULL,
                  (VOID **)&mRedfishPlatformConfigLibPrivate.Protocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: locate EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL failure: %r\n", __func__, Status));
    return;
  }

  gBS->CloseEvent (Event);
  mRedfishPlatformConfigLibPrivate.ProtocolEvent = NULL;
}

/**

  Create protocol listener and wait for Redfish Platform Config protocol.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCCESS  Protocol listener is registered successfully.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  ZeroMem (&mRedfishPlatformConfigLibPrivate, sizeof (REDFISH_PLATFORM_CONFIG_LIB_PRIVATE));
  mRedfishPlatformConfigLibPrivate.ProtocolEvent = EfiCreateProtocolNotifyEvent (
                                                     &gEdkIIRedfishPlatformConfigProtocolGuid,
                                                     TPL_CALLBACK,
                                                     RedfishPlatformConfigProtocolInstalled,
                                                     NULL,
                                                     &mRedfishPlatformConfigLibPrivate.Registration
                                                     );
  if (mRedfishPlatformConfigLibPrivate.ProtocolEvent == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed to create protocol notify event\n", __func__));
  }

  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param ImageHandle       Handle that identifies the image to be unloaded.
  @param  SystemTable      The system table.

  @retval EFI_SUCCESS      The image has been unloaded.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (mRedfishPlatformConfigLibPrivate.ProtocolEvent != NULL) {
    gBS->CloseEvent (mRedfishPlatformConfigLibPrivate.ProtocolEvent);
    mRedfishPlatformConfigLibPrivate.ProtocolEvent = NULL;
  }

  mRedfishPlatformConfigLibPrivate.Protocol = NULL;

  return EFI_SUCCESS;
}
