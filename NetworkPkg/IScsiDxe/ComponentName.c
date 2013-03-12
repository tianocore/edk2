/** @file
  UEFI Component Name(2) protocol implementation for iSCSI.

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IScsiImpl.h"

//
// EFI Component Name Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL gIScsiComponentName = {
  IScsiComponentNameGetDriverName,
  IScsiComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL gIScsiComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME) IScsiComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) IScsiComponentNameGetControllerName,
  "en"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE     mIScsiDriverNameTable[] = {
  {
    "eng;en",
    L"iSCSI Driver"
  },
  {
    NULL,
    NULL
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  *gIScsiControllerNameTable = NULL;

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param[in]  This              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param[in]  Language          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param[out]  DriverName       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
IScsiComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mIScsiDriverNameTable,
           DriverName,
           (BOOLEAN) (This == &gIScsiComponentName)
           );
}

/**
  Update the component name for the iSCSI NIC handle.

  @param[in]  Controller            The handle of the NIC controller.
  @param[in]  Ipv6Flag              TRUE if IP6 network stack is used.
  
  @retval EFI_SUCCESS               Update the ControllerNameTable of this instance successfully.
  @retval EFI_INVALID_PARAMETER     The input parameter is invalid.
  @retval EFI_UNSUPPORTED           Can't get the corresponding NIC info from the Controller handle.
  
**/
EFI_STATUS
UpdateName (
  IN   EFI_HANDLE  Controller,
  IN   BOOLEAN     Ipv6Flag
  )
{
  EFI_STATUS                  Status;
  EFI_MAC_ADDRESS             MacAddr;
  UINTN                       HwAddressSize;
  UINT16                      VlanId;
  ISCSI_NIC_INFO              *ThisNic;
  ISCSI_NIC_INFO              *NicInfo;
  LIST_ENTRY                  *Entry;
  CHAR16                      HandleName[80];

  //
  // Get MAC address of this network device.
  //
  Status = NetLibGetMacAddress (Controller, &MacAddr, &HwAddressSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get VLAN ID of this network device.
  //
  VlanId = NetLibGetVlanId (Controller);

  //
  // Check whether the NIC information exists.
  //
  ThisNic = NULL;

  NET_LIST_FOR_EACH (Entry, &mPrivate->NicInfoList) {
    NicInfo = NET_LIST_USER_STRUCT (Entry, ISCSI_NIC_INFO, Link);
    if (NicInfo->HwAddressSize == HwAddressSize &&
        CompareMem (&NicInfo->PermanentAddress, MacAddr.Addr, HwAddressSize) == 0 &&
        NicInfo->VlanId == VlanId) {

      ThisNic = NicInfo;
      break;
    }
  }

  if (ThisNic == NULL) {
    return EFI_UNSUPPORTED;
  }

  UnicodeSPrint (
    HandleName,
    sizeof (HandleName),
    L"iSCSI (%s, NicIndex=%d)",
    Ipv6Flag ? L"IPv6" : L"IPv4",
    ThisNic->NicIndex
  );

  if (gIScsiControllerNameTable != NULL) {
    FreeUnicodeStringTable (gIScsiControllerNameTable);
    gIScsiControllerNameTable = NULL;
  }

  Status = AddUnicodeString2 (
             "eng",
             gIScsiComponentName.SupportedLanguages,
             &gIScsiControllerNameTable,
             HandleName,
             TRUE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AddUnicodeString2 (
           "en",
           gIScsiComponentName2.SupportedLanguages,
           &gIScsiControllerNameTable,
           HandleName,
           FALSE
           );
}


/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param[in]  This              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param[in]  ControllerHandle  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param[in]  ChildHandle       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param[in]  Language          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param[out]  ControllerName   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language, from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL, and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
IScsiComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    ChildHandle        OPTIONAL,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **ControllerName
  )
{
  EFI_HANDLE                      IScsiController;
  BOOLEAN                         Ipv6Flag;
  EFI_STATUS                      Status;
  EFI_GUID                        *IScsiPrivateGuid;
  ISCSI_PRIVATE_PROTOCOL          *IScsiIdentifier;

  //
  // Get the handle of the controller we are controling.
  //
  IScsiController = NetLibGetNicHandle (ControllerHandle, &gEfiTcp4ProtocolGuid);
  if (IScsiController != NULL) {
    IScsiPrivateGuid = &gIScsiV4PrivateGuid;
    Ipv6Flag = FALSE;
  } else {
    IScsiController = NetLibGetNicHandle (ControllerHandle, &gEfiTcp6ProtocolGuid);
    if (IScsiController != NULL) {
      IScsiPrivateGuid = &gIScsiV6PrivateGuid;
      Ipv6Flag = TRUE;
    } else {
      return EFI_UNSUPPORTED;
    }
  }

  // 
  // Retrieve an instance of a produced protocol from IScsiController  
  // 
  Status = gBS->OpenProtocol (
                  IScsiController,
                  IScsiPrivateGuid,
                  (VOID **) &IScsiIdentifier,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UpdateName(IScsiController, Ipv6Flag);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           gIScsiControllerNameTable,
           ControllerName,
           (BOOLEAN)(This == &gIScsiComponentName)
           );
}
