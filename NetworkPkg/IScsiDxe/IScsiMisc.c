/** @file
  Miscellaneous routines for iSCSI driver.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "IScsiImpl.h"

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8  IScsiHexString[] = "0123456789ABCDEFabcdef";

/**
  Removes (trims) specified leading and trailing characters from a string.

  @param[in, out] Str   Pointer to the null-terminated string to be trimmed.
                        On return, Str will hold the trimmed string.

  @param[in]      CharC Character will be trimmed from str.

**/
VOID
IScsiStrTrim (
  IN OUT CHAR16   *Str,
  IN     CHAR16   CharC
  )
{
  CHAR16  *Pointer1;
  CHAR16  *Pointer2;

  if (*Str == 0) {
    return ;
  }

  //
  // Trim off the leading and trailing characters c
  //
  for (Pointer1 = Str; (*Pointer1 != 0) && (*Pointer1 == CharC); Pointer1++) {
    ;
  }

  Pointer2 = Str;
  if (Pointer2 == Pointer1) {
    while (*Pointer1 != 0) {
      Pointer2++;
      Pointer1++;
    }
  } else {
    while (*Pointer1 != 0) {
    *Pointer2 = *Pointer1;
    Pointer1++;
    Pointer2++;
    }
    *Pointer2 = 0;
  }


  for (Pointer1 = Str + StrLen(Str) - 1; Pointer1 >= Str && *Pointer1 == CharC; Pointer1--) {
    ;
  }
  if  (Pointer1 !=  Str + StrLen(Str) - 1) {
    *(Pointer1 + 1) = 0;
  }
}

/**
  Calculate the prefix length of the IPv4 subnet mask.

  @param[in]  SubnetMask The IPv4 subnet mask.

  @return     The prefix length of the subnet mask.
  @retval 0   Other errors as indicated.

**/
UINT8
IScsiGetSubnetMaskPrefixLength (
  IN EFI_IPv4_ADDRESS  *SubnetMask
  )
{
  UINT8   Len;
  UINT32  ReverseMask;

  //
  // The SubnetMask is in network byte order.
  //
  ReverseMask = (SubnetMask->Addr[0] << 24) | (SubnetMask->Addr[1] << 16) | (SubnetMask->Addr[2] << 8) | (SubnetMask->Addr[3]);

  //
  // Reverse it.
  //
  ReverseMask = ~ReverseMask;

  if ((ReverseMask & (ReverseMask + 1)) != 0) {
    return 0;
  }

  Len = 0;

  while (ReverseMask != 0) {
    ReverseMask = ReverseMask >> 1;
    Len++;
  }

  return (UINT8) (32 - Len);
}


/**
  Convert the hexadecimal encoded LUN string into the 64-bit LUN.

  @param[in]   Str             The hexadecimal encoded LUN string.
  @param[out]  Lun             Storage to return the 64-bit LUN.

  @retval EFI_SUCCESS            The 64-bit LUN is stored in Lun.
  @retval EFI_INVALID_PARAMETER  The string is malformatted.

**/
EFI_STATUS
IScsiAsciiStrToLun (
  IN  CHAR8  *Str,
  OUT UINT8  *Lun
  )
{
  UINTN   Index, IndexValue, IndexNum, SizeStr;
  CHAR8   TemStr[2];
  UINT8   TemValue;
  UINT16  Value[4];

  ZeroMem (Lun, 8);
  ZeroMem (TemStr, 2);
  ZeroMem ((UINT8 *) Value, sizeof (Value));
  SizeStr    = AsciiStrLen (Str);
  IndexValue = 0;
  IndexNum   = 0;

  for (Index = 0; Index < SizeStr; Index ++) {
    TemStr[0] = Str[Index];
    TemValue = (UINT8) AsciiStrHexToUint64 (TemStr);
    if (TemValue == 0 && TemStr[0] != '0') {
      if ((TemStr[0] != '-') || (IndexNum == 0)) {
        //
        // Invalid Lun Char.
        //
        return EFI_INVALID_PARAMETER;
      }
    }

    if ((TemValue == 0) && (TemStr[0] == '-')) {
      //
      // Next Lun value.
      //
      if (++IndexValue >= 4) {
        //
        // Max 4 Lun value.
        //
        return EFI_INVALID_PARAMETER;
      }
      //
      // Restart str index for the next lun value.
      //
      IndexNum = 0;
      continue;
    }

    if (++IndexNum > 4) {
      //
      // Each Lun Str can't exceed size 4, because it will be as UINT16 value.
      //
      return EFI_INVALID_PARAMETER;
    }

    //
    // Combine UINT16 value.
    //
    Value[IndexValue] = (UINT16) ((Value[IndexValue] << 4) + TemValue);
  }

  for (Index = 0; Index <= IndexValue; Index ++) {
    *((UINT16 *) &Lun[Index * 2]) =  HTONS (Value[Index]);
  }

  return EFI_SUCCESS;
}

/**
  Convert the 64-bit LUN into the hexadecimal encoded LUN string.

  @param[in]   Lun The 64-bit LUN.
  @param[out]  Str The storage to return the hexadecimal encoded LUN string.

**/
VOID
IScsiLunToUnicodeStr (
  IN UINT8    *Lun,
  OUT CHAR16  *Str
  )
{
  UINTN   Index;
  CHAR16  *TempStr;

  TempStr = Str;

  for (Index = 0; Index < 4; Index++) {

    if ((Lun[2 * Index] | Lun[2 * Index + 1]) == 0) {
      CopyMem (TempStr, L"0-", sizeof (L"0-"));
    } else {
      TempStr[0]  = (CHAR16) IScsiHexString[Lun[2 * Index] >> 4];
      TempStr[1]  = (CHAR16) IScsiHexString[Lun[2 * Index] & 0x0F];
      TempStr[2]  = (CHAR16) IScsiHexString[Lun[2 * Index + 1] >> 4];
      TempStr[3]  = (CHAR16) IScsiHexString[Lun[2 * Index + 1] & 0x0F];
      TempStr[4]  = L'-';
      TempStr[5]  = 0;

      IScsiStrTrim (TempStr, L'0');
    }

    TempStr += StrLen (TempStr);
  }
  //
  // Remove the last '-'
  //
  ASSERT (StrLen(Str) >= 1);
  Str[StrLen (Str) - 1] = 0;

  for (Index = StrLen (Str) - 1; Index > 1; Index = Index - 2) {
    if ((Str[Index] == L'0') && (Str[Index - 1] == L'-')) {
      Str[Index - 1] = 0;
    } else {
      break;
    }
  }
}

/**
  Convert the formatted IP address into the binary IP address.

  @param[in]   Str               The UNICODE string.
  @param[in]   IpMode            Indicates whether the IP address is v4 or v6.
  @param[out]  Ip                The storage to return the ASCII string.

  @retval EFI_SUCCESS            The binary IP address is returned in Ip.
  @retval EFI_INVALID_PARAMETER  The IP string is malformatted or IpMode is
                                 invalid.

**/
EFI_STATUS
IScsiAsciiStrToIp (
  IN  CHAR8             *Str,
  IN  UINT8             IpMode,
  OUT EFI_IP_ADDRESS    *Ip
  )
{
  EFI_STATUS            Status;

  if (IpMode == IP_MODE_IP4 || IpMode == IP_MODE_AUTOCONFIG_IP4) {
    return NetLibAsciiStrToIp4 (Str, &Ip->v4);

  } else if (IpMode == IP_MODE_IP6 || IpMode == IP_MODE_AUTOCONFIG_IP6) {
    return NetLibAsciiStrToIp6 (Str, &Ip->v6);

  } else if (IpMode == IP_MODE_AUTOCONFIG) {
    Status = NetLibAsciiStrToIp4 (Str, &Ip->v4);
    if (!EFI_ERROR (Status)) {
      return Status;
    }
    return NetLibAsciiStrToIp6 (Str, &Ip->v6);

  }

  return EFI_INVALID_PARAMETER;
}

/**
  Convert the mac address into a hexadecimal encoded "-" separated string.

  @param[in]  Mac     The mac address.
  @param[in]  Len     Length in bytes of the mac address.
  @param[in]  VlanId  VLAN ID of the network device.
  @param[out] Str     The storage to return the mac string.

**/
VOID
IScsiMacAddrToStr (
  IN  EFI_MAC_ADDRESS  *Mac,
  IN  UINT32           Len,
  IN  UINT16           VlanId,
  OUT CHAR16           *Str
  )
{
  UINT32  Index;
  CHAR16  *String;

  for (Index = 0; Index < Len; Index++) {
    Str[3 * Index]      = (CHAR16) IScsiHexString[(Mac->Addr[Index] >> 4) & 0x0F];
    Str[3 * Index + 1]  = (CHAR16) IScsiHexString[Mac->Addr[Index] & 0x0F];
    Str[3 * Index + 2]  = L':';
  }

  String = &Str[3 * Index - 1] ;
  if (VlanId != 0) {
    String += UnicodeSPrint (String, 6 * sizeof (CHAR16), L"\\%04x", (UINTN) VlanId);
  }

  *String = L'\0';
}

/**
  Convert the binary encoded buffer into a hexadecimal encoded string.

  @param[in]       BinBuffer   The buffer containing the binary data.
  @param[in]       BinLength   Length of the binary buffer.
  @param[in, out]  HexStr      Pointer to the string.
  @param[in, out]  HexLength   The length of the string.

  @retval EFI_SUCCESS          The binary data is converted to the hexadecimal string
                               and the length of the string is updated.
  @retval EFI_BUFFER_TOO_SMALL The string is too small.
  @retval EFI_INVALID_PARAMETER The IP string is malformatted.

**/
EFI_STATUS
IScsiBinToHex (
  IN     UINT8  *BinBuffer,
  IN     UINT32 BinLength,
  IN OUT CHAR8  *HexStr,
  IN OUT UINT32 *HexLength
  )
{
  UINTN Index;

  if ((HexStr == NULL) || (BinBuffer == NULL) || (BinLength == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (((*HexLength) - 3) < BinLength * 2) {
    *HexLength = BinLength * 2 + 3;
    return EFI_BUFFER_TOO_SMALL;
  }

  *HexLength = BinLength * 2 + 3;
  //
  // Prefix for Hex String.
  //
  HexStr[0] = '0';
  HexStr[1] = 'x';

  for (Index = 0; Index < BinLength; Index++) {
    HexStr[Index * 2 + 2] = IScsiHexString[BinBuffer[Index] >> 4];
    HexStr[Index * 2 + 3] = IScsiHexString[BinBuffer[Index] & 0xf];
  }

  HexStr[Index * 2 + 2] = '\0';

  return EFI_SUCCESS;
}


/**
  Convert the hexadecimal string into a binary encoded buffer.

  @param[in, out]  BinBuffer   The binary buffer.
  @param[in, out]  BinLength   Length of the binary buffer.
  @param[in]       HexStr      The hexadecimal string.

  @retval EFI_SUCCESS          The hexadecimal string is converted into a binary
                               encoded buffer.
  @retval EFI_BUFFER_TOO_SMALL The binary buffer is too small to hold the converted data.

**/
EFI_STATUS
IScsiHexToBin (
  IN OUT UINT8  *BinBuffer,
  IN OUT UINT32 *BinLength,
  IN     CHAR8  *HexStr
  )
{
  UINTN   Index;
  UINTN   Length;
  UINT8   Digit;
  CHAR8   TemStr[2];

  ZeroMem (TemStr, sizeof (TemStr));

  //
  // Find out how many hex characters the string has.
  //
  if ((HexStr[0] == '0') && ((HexStr[1] == 'x') || (HexStr[1] == 'X'))) {
    HexStr += 2;
  }

  Length = AsciiStrLen (HexStr);

  for (Index = 0; Index < Length; Index ++) {
    TemStr[0] = HexStr[Index];
    Digit = (UINT8) AsciiStrHexToUint64 (TemStr);
    if (Digit == 0 && TemStr[0] != '0') {
      //
      // Invalid Lun Char.
      //
      break;
    }
    if ((Index & 1) == 0) {
      BinBuffer [Index/2] = Digit;
    } else {
      BinBuffer [Index/2] = (UINT8) ((BinBuffer [Index/2] << 4) + Digit);
    }
  }

  *BinLength = (UINT32) ((Index + 1)/2);

  return EFI_SUCCESS;
}


/**
  Convert the decimal-constant string or hex-constant string into a numerical value.

  @param[in] Str                    String in decimal or hex.

  @return The numerical value.

**/
UINTN
IScsiNetNtoi (
  IN     CHAR8  *Str
  )
{
  if ((Str[0] == '0') && ((Str[1] == 'x') || (Str[1] == 'X'))) {
    Str += 2;

    return AsciiStrHexToUintn (Str);
  }

  return AsciiStrDecimalToUintn (Str);
}


/**
  Generate random numbers.

  @param[in, out]  Rand       The buffer to contain random numbers.
  @param[in]       RandLength The length of the Rand buffer.

**/
VOID
IScsiGenRandom (
  IN OUT UINT8  *Rand,
  IN     UINTN  RandLength
  )
{
  UINT32  Random;

  while (RandLength > 0) {
    Random  = NET_RANDOM (NetRandomInitSeed ());
    *Rand++ = (UINT8) (Random);
    RandLength--;
  }
}


/**
  Check whether UNDI protocol supports IPv6.

  @param[in]   ControllerHandle    Controller handle.
  @param[in]   Image               Handle of the image.
  @param[out]  Ipv6Support         TRUE if UNDI supports IPv6.

  @retval EFI_SUCCESS            Get the result whether UNDI supports IPv6 by NII or AIP protocol successfully.
  @retval EFI_NOT_FOUND          Don't know whether UNDI supports IPv6 since NII or AIP is not available.

**/
EFI_STATUS
IScsiCheckIpv6Support (
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   Image,
  OUT BOOLEAN                      *Ipv6Support
  )
{
  EFI_HANDLE                       Handle;
  EFI_ADAPTER_INFORMATION_PROTOCOL *Aip;
  EFI_STATUS                       Status;
  EFI_GUID                         *InfoTypesBuffer;
  UINTN                            InfoTypeBufferCount;
  UINTN                            TypeIndex;
  BOOLEAN                          Supported;
  VOID                             *InfoBlock;
  UINTN                            InfoBlockSize;

  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL *Nii;

  ASSERT (Ipv6Support != NULL);

  //
  // Check whether the UNDI supports IPv6 by NII protocol.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  (VOID **) &Nii,
                  Image,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (Status == EFI_SUCCESS) {
    *Ipv6Support = Nii->Ipv6Supported;
    return EFI_SUCCESS;
  }

  //
  // Get the NIC handle by SNP protocol.
  //
  Handle = NetLibGetSnpHandle (ControllerHandle, NULL);
  if (Handle == NULL) {
    return EFI_NOT_FOUND;
  }

  Aip    = NULL;
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiAdapterInformationProtocolGuid,
                  (VOID *) &Aip
                  );
  if (EFI_ERROR (Status) || Aip == NULL) {
    return EFI_NOT_FOUND;
  }

  InfoTypesBuffer     = NULL;
  InfoTypeBufferCount = 0;
  Status = Aip->GetSupportedTypes (Aip, &InfoTypesBuffer, &InfoTypeBufferCount);
  if (EFI_ERROR (Status) || InfoTypesBuffer == NULL) {
    FreePool (InfoTypesBuffer);
    return EFI_NOT_FOUND;
  }

  Supported = FALSE;
  for (TypeIndex = 0; TypeIndex < InfoTypeBufferCount; TypeIndex++) {
    if (CompareGuid (&InfoTypesBuffer[TypeIndex], &gEfiAdapterInfoUndiIpv6SupportGuid)) {
      Supported = TRUE;
      break;
    }
  }

  FreePool (InfoTypesBuffer);
  if (!Supported) {
    return EFI_NOT_FOUND;
  }

  //
  // We now have adapter information block.
  //
  InfoBlock     = NULL;
  InfoBlockSize = 0;
  Status = Aip->GetInformation (Aip, &gEfiAdapterInfoUndiIpv6SupportGuid, &InfoBlock, &InfoBlockSize);
  if (EFI_ERROR (Status) || InfoBlock == NULL) {
    FreePool (InfoBlock);
    return EFI_NOT_FOUND;
  }

  *Ipv6Support = ((EFI_ADAPTER_INFO_UNDI_IPV6_SUPPORT *) InfoBlock)->Ipv6Support;
  FreePool (InfoBlock);

  return EFI_SUCCESS;
}

/**
  Record the NIC info in global structure.

  @param[in]  Controller         The handle of the controller.
  @param[in]  Image              Handle of the image.

  @retval EFI_SUCCESS            The operation is completed.
  @retval EFI_OUT_OF_RESOURCES   Do not have sufficient resources to finish this
                                 operation.

**/
EFI_STATUS
IScsiAddNic (
  IN EFI_HANDLE  Controller,
  IN EFI_HANDLE  Image
  )
{
  EFI_STATUS                  Status;
  ISCSI_NIC_INFO              *NicInfo;
  LIST_ENTRY                  *Entry;
  EFI_MAC_ADDRESS             MacAddr;
  UINTN                       HwAddressSize;
  UINT16                      VlanId;

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
  // Check whether the NIC info already exists. Return directly if so.
  //
  NET_LIST_FOR_EACH (Entry, &mPrivate->NicInfoList) {
    NicInfo = NET_LIST_USER_STRUCT (Entry, ISCSI_NIC_INFO, Link);
    if (NicInfo->HwAddressSize == HwAddressSize &&
        CompareMem (&NicInfo->PermanentAddress, MacAddr.Addr, HwAddressSize) == 0 &&
        NicInfo->VlanId == VlanId) {
      mPrivate->CurrentNic = NicInfo->NicIndex;

      //
      // Set IPv6 available flag.
      //
      Status = IScsiCheckIpv6Support (Controller, Image, &NicInfo->Ipv6Available);
      if (EFI_ERROR (Status)) {
        //
        // Fail to get the data whether UNDI supports IPv6.
        // Set default value to TRUE.
        //
        NicInfo->Ipv6Available = TRUE;
      }

      return EFI_SUCCESS;
    }

    if (mPrivate->MaxNic < NicInfo->NicIndex) {
      mPrivate->MaxNic = NicInfo->NicIndex;
    }
  }

  //
  // Record the NIC info in private structure.
  //
  NicInfo = AllocateZeroPool (sizeof (ISCSI_NIC_INFO));
  if (NicInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (&NicInfo->PermanentAddress, MacAddr.Addr, HwAddressSize);
  NicInfo->HwAddressSize  = (UINT32) HwAddressSize;
  NicInfo->VlanId         = VlanId;
  NicInfo->NicIndex       = (UINT8) (mPrivate->MaxNic + 1);
  mPrivate->MaxNic        = NicInfo->NicIndex;

  //
  // Set IPv6 available flag.
  //
  Status = IScsiCheckIpv6Support (Controller, Image, &NicInfo->Ipv6Available);
  if (EFI_ERROR (Status)) {
    //
    // Fail to get the data whether UNDI supports IPv6.
    // Set default value to TRUE.
    //
    NicInfo->Ipv6Available = TRUE;
  }

  //
  // Get the PCI location.
  //
  IScsiGetNICPciLocation (
    Controller,
    &NicInfo->BusNumber,
    &NicInfo->DeviceNumber,
    &NicInfo->FunctionNumber
    );

  InsertTailList (&mPrivate->NicInfoList, &NicInfo->Link);
  mPrivate->NicCount++;

  mPrivate->CurrentNic = NicInfo->NicIndex;
  return EFI_SUCCESS;
}


/**
  Delete the recorded NIC info from global structure. Also delete corresponding
  attempts.

  @param[in]  Controller         The handle of the controller.

  @retval EFI_SUCCESS            The operation is completed.
  @retval EFI_NOT_FOUND          The NIC info to be deleted is not recorded.

**/
EFI_STATUS
IScsiRemoveNic (
  IN EFI_HANDLE  Controller
  )
{
  EFI_STATUS                  Status;
  ISCSI_NIC_INFO              *NicInfo;
  LIST_ENTRY                  *Entry;
  LIST_ENTRY                  *NextEntry;
  ISCSI_ATTEMPT_CONFIG_NVDATA *AttemptConfigData;
  ISCSI_NIC_INFO              *ThisNic;
  EFI_MAC_ADDRESS             MacAddr;
  UINTN                       HwAddressSize;
  UINT16                      VlanId;

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
    return EFI_NOT_FOUND;
  }

  mPrivate->CurrentNic = ThisNic->NicIndex;

  RemoveEntryList (&ThisNic->Link);
  FreePool (ThisNic);
  mPrivate->NicCount--;

  //
  // Remove all attempts related to this NIC.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &mPrivate->AttemptConfigs) {
    AttemptConfigData = NET_LIST_USER_STRUCT (Entry, ISCSI_ATTEMPT_CONFIG_NVDATA, Link);
    if (AttemptConfigData->NicIndex == mPrivate->CurrentNic) {
      RemoveEntryList (&AttemptConfigData->Link);
      mPrivate->AttemptCount--;

      if (AttemptConfigData->SessionConfigData.Enabled == ISCSI_ENABLED_FOR_MPIO && mPrivate->MpioCount > 0) {
        if (--mPrivate->MpioCount == 0) {
          mPrivate->EnableMpio = FALSE;
        }

        if (AttemptConfigData->AuthenticationType == ISCSI_AUTH_TYPE_KRB && mPrivate->Krb5MpioCount > 0) {
          mPrivate->Krb5MpioCount--;
        }

      } else if (AttemptConfigData->SessionConfigData.Enabled == ISCSI_ENABLED && mPrivate->SinglePathCount > 0) {
        mPrivate->SinglePathCount--;

        if (mPrivate->ValidSinglePathCount > 0) {
          mPrivate->ValidSinglePathCount--;
        }
      }

      FreePool (AttemptConfigData);
    }
  }

  return EFI_SUCCESS;
}

/**
  Create and initialize the Attempts.

  @param[in]  AttemptNum          The number of Attempts will be created.

  @retval EFI_SUCCESS             The Attempts have been created successfully.
  @retval Others                  Failed to create the Attempt.

**/
EFI_STATUS
IScsiCreateAttempts (
  IN UINTN            AttemptNum
)
{
  ISCSI_ATTEMPT_CONFIG_NVDATA   *AttemptConfigData;
  ISCSI_SESSION_CONFIG_NVDATA   *ConfigData;
  UINT8                         *AttemptConfigOrder;
  UINTN                         AttemptConfigOrderSize;
  UINT8                         *AttemptOrderTmp;
  UINTN                         TotalNumber;
  UINT8                         Index;
  EFI_STATUS                    Status;

  for (Index = 1; Index <= AttemptNum; Index ++) {
    //
    // Get the initialized attempt order. This is used to essure creating attempts by order.
    //
    AttemptConfigOrder = IScsiGetVariableAndSize (
                           L"InitialAttemptOrder",
                           &gIScsiConfigGuid,
                           &AttemptConfigOrderSize
                           );
    TotalNumber = AttemptConfigOrderSize / sizeof (UINT8);
    if (TotalNumber == AttemptNum) {
      Status = EFI_SUCCESS;
      break;
    }
    TotalNumber++;

    //
    // Append the new created attempt to the end.
    //
    AttemptOrderTmp = AllocateZeroPool (TotalNumber * sizeof (UINT8));
    if (AttemptOrderTmp == NULL) {
      if (AttemptConfigOrder != NULL) {
        FreePool (AttemptConfigOrder);
      }
      return EFI_OUT_OF_RESOURCES;
    }

    if (AttemptConfigOrder != NULL) {
      CopyMem (AttemptOrderTmp, AttemptConfigOrder, AttemptConfigOrderSize);
      FreePool (AttemptConfigOrder);
    }

    AttemptOrderTmp[TotalNumber - 1] = Index;
    AttemptConfigOrder               = AttemptOrderTmp;
    AttemptConfigOrderSize           = TotalNumber * sizeof (UINT8);

    Status = gRT->SetVariable (
                    L"InitialAttemptOrder",
                    &gIScsiConfigGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    AttemptConfigOrderSize,
                    AttemptConfigOrder
                    );
    FreePool (AttemptConfigOrder);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR,
        "%a: Failed to set 'InitialAttemptOrder' with Guid (%g): "
        "%r\n",
        __FUNCTION__, &gIScsiConfigGuid, Status));
      return Status;
    }

    //
    // Create new Attempt
    //
    AttemptConfigData = AllocateZeroPool (sizeof (ISCSI_ATTEMPT_CONFIG_NVDATA));
    if (AttemptConfigData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    ConfigData                    = &AttemptConfigData->SessionConfigData;
    ConfigData->TargetPort        = ISCSI_WELL_KNOWN_PORT;
    ConfigData->ConnectTimeout    = CONNECT_DEFAULT_TIMEOUT;
    ConfigData->ConnectRetryCount = CONNECT_MIN_RETRY;

    AttemptConfigData->AuthenticationType           = ISCSI_AUTH_TYPE_CHAP;
    AttemptConfigData->AuthConfigData.CHAP.CHAPType = ISCSI_CHAP_UNI;
    //
    // Configure the Attempt index and set variable.
    //
    AttemptConfigData->AttemptConfigIndex = Index;

    //
    // Set the attempt name according to the order.
    //
    UnicodeSPrint (
      mPrivate->PortString,
      (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
      L"Attempt %d",
      (UINTN) AttemptConfigData->AttemptConfigIndex
      );
    UnicodeStrToAsciiStrS (mPrivate->PortString, AttemptConfigData->AttemptName, ATTEMPT_NAME_SIZE);

    Status = gRT->SetVariable (
                    mPrivate->PortString,
                    &gEfiIScsiInitiatorNameProtocolGuid,
                    ISCSI_CONFIG_VAR_ATTR,
                    sizeof (ISCSI_ATTEMPT_CONFIG_NVDATA),
                    AttemptConfigData
                    );
    FreePool (AttemptConfigData);
    if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR,
          "%a: Failed to set variable (mPrivate->PortString) with Guid (%g): "
          "%r\n",
          __FUNCTION__, &gEfiIScsiInitiatorNameProtocolGuid, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Create the iSCSI configuration Keywords for each attempt. You can find the keywords
  defined in the "x-UEFI-ns" namespace (http://www.uefi.org/confignamespace).

  @param[in]  KeywordNum          The number Sets of Keywords will be created.

  @retval EFI_SUCCESS             The operation is completed.
  @retval Others                  Failed to create the Keywords.

**/
EFI_STATUS
IScsiCreateKeywords (
  IN UINTN            KeywordNum
)
{
  VOID                          *StartOpCodeHandle;
  EFI_IFR_GUID_LABEL            *StartLabel;
  VOID                          *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL            *EndLabel;
  UINTN                         Index;
  EFI_STRING_ID                 StringToken;
  CHAR16                        StringId[64];
  CHAR16                        KeywordId[32];
  EFI_STATUS                    Status;

  Status = IScsiCreateOpCode (
             KEYWORD_ENTRY_LABEL,
             &StartOpCodeHandle,
             &StartLabel,
             &EndOpCodeHandle,
             &EndLabel
             );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 1; Index <= KeywordNum; Index ++) {
    //
    // Create iSCSIAttemptName Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_ATTEMPTT_NAME_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIAttemptName:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateStringOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_ATTEMPT_NAME_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_ATTEMPT_NAME_VAR_OFFSET + ATTEMPT_NAME_SIZE * (Index - 1) * sizeof (CHAR16)),
      StringToken,
      StringToken,
      EFI_IFR_FLAG_READ_ONLY,
      0,
      0,
      ATTEMPT_NAME_SIZE,
      NULL
      );

    //
    // Create iSCSIBootEnable Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_MODE_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIBootEnable:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateNumericOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_BOOTENABLE_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_BOOTENABLE_VAR_OFFSET + (Index - 1)),
      StringToken,
      StringToken,
      0,
      EFI_IFR_NUMERIC_SIZE_1,
      0,
      2,
      0,
      NULL
      );

    //
    // Create iSCSIIpAddressType Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_IP_MODE_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIIpAddressType:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateNumericOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_ADDRESS_TYPE_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_ADDRESS_TYPE_VAR_OFFSET + (Index - 1)),
      StringToken,
      StringToken,
      0,
      EFI_IFR_NUMERIC_SIZE_1,
      0,
      2,
      0,
      NULL
      );

    //
    // Create iSCSIConnectRetry Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_CONNECT_RETRY_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIConnectRetry:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateNumericOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_CONNECT_RETRY_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_CONNECT_RETRY_VAR_OFFSET + (Index - 1)),
      StringToken,
      StringToken,
      0,
      EFI_IFR_NUMERIC_SIZE_1,
      0,
      16,
      0,
      NULL
      );

    //
    // Create iSCSIConnectTimeout Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_CONNECT_TIMEOUT_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIConnectTimeout:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateNumericOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_CONNECT_TIMEOUT_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_CONNECT_TIMEOUT_VAR_OFFSET + 2 * (Index - 1)),
      StringToken,
      StringToken,
      0,
      EFI_IFR_NUMERIC_SIZE_2,
      CONNECT_MIN_TIMEOUT,
      CONNECT_MAX_TIMEOUT,
      0,
      NULL
      );

    //
    // Create ISID Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_ISID_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIISID:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateStringOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_ISID_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_ISID_VAR_OFFSET + sizeof (KEYWORD_STR) * (Index - 1)),
      StringToken,
      STRING_TOKEN (STR_ISCSI_ISID_HELP),
      0,
      0,
      ISID_CONFIGURABLE_MIN_LEN,
      ISID_CONFIGURABLE_STORAGE,
      NULL
      );

    //
    // Create iSCSIInitiatorInfoViaDHCP Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_INITIATOR_VIA_DHCP_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIInitiatorInfoViaDHCP:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateNumericOpCode (
    StartOpCodeHandle,
    (EFI_QUESTION_ID) (ATTEMPT_INITIATOR_VIA_DHCP_QUESTION_ID + (Index - 1)),
    CONFIGURATION_VARSTORE_ID,
    (UINT16) (ATTEMPT_INITIATOR_VIA_DHCP_VAR_OFFSET + (Index - 1)),
    StringToken,
    StringToken,
    0,
    0,
    0,
    1,
    0,
    NULL
    );

    //
    // Create iSCSIInitiatorIpAddress Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_INITIATOR_IP_ADDRESS_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIInitiatorIpAddress:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateStringOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_INITIATOR_IP_ADDRESS_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_INITIATOR_IP_ADDRESS_VAR_OFFSET + sizeof (KEYWORD_STR) * (Index - 1)),
      StringToken,
      StringToken,
      0,
      0,
      IP4_MIN_SIZE,
      IP4_STR_MAX_SIZE,
      NULL
      );

    //
    // Create iSCSIInitiatorNetmask Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_INITIATOR_NET_MASK_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIInitiatorNetmask:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateStringOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_INITIATOR_NET_MASK_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_INITIATOR_NET_MASK_VAR_OFFSET + sizeof (KEYWORD_STR) * (Index - 1)),
      StringToken,
      StringToken,
      0,
      0,
      IP4_MIN_SIZE,
      IP4_STR_MAX_SIZE,
      NULL
      );

    //
    // Create iSCSIInitiatorGateway Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_INITIATOR_GATE_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIInitiatorGateway:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateStringOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_INITIATOR_GATE_WAY_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_INITIATOR_GATE_WAY_VAR_OFFSET + sizeof (KEYWORD_STR) * (Index - 1)),
      StringToken,
      StringToken,
      0,
      0,
      IP4_MIN_SIZE,
      IP4_STR_MAX_SIZE,
      NULL
      );

    //
    // Create iSCSITargetInfoViaDHCP Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_TARGET_VIA_DHCP_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSITargetInfoViaDHCP:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateNumericOpCode (
    StartOpCodeHandle,
    (EFI_QUESTION_ID) (ATTEMPT_TARGET_VIA_DHCP_QUESTION_ID + (Index - 1)),
    CONFIGURATION_VARSTORE_ID,
    (UINT16) (ATTEMPT_TARGET_VIA_DHCP_VAR_OFFSET + (Index - 1)),
    StringToken,
    StringToken,
    0,
    0,
    0,
    1,
    0,
    NULL
    );

    //
    // Create iSCSITargetTcpPort Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_TARGET_TCP_PORT_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSITargetTcpPort:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateNumericOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_TARGET_TCP_PORT_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_TARGET_TCP_PORT_VAR_OFFSET + 2 * (Index - 1)),
      StringToken,
      StringToken,
      0,
      EFI_IFR_NUMERIC_SIZE_2,
      TARGET_PORT_MIN_NUM,
      TARGET_PORT_MAX_NUM,
      0,
      NULL
      );

    //
    // Create iSCSITargetName Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_TARGET_NAME_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSITargetName:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateStringOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_TARGET_NAME_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_TARGET_NAME_VAR_OFFSET + sizeof (KEYWORD_STR) * (Index - 1)),
      StringToken,
      StringToken,
      0,
      0,
      ISCSI_NAME_IFR_MIN_SIZE,
      ISCSI_NAME_IFR_MAX_SIZE,
      NULL
      );

    //
    // Create iSCSITargetIpAddress Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_TARGET_IP_ADDRESS_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSITargetIpAddress:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateStringOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_TARGET_IP_ADDRESS_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_TARGET_IP_ADDRESS_VAR_OFFSET + sizeof (KEYWORD_STR) * (Index - 1)),
      StringToken,
      StringToken,
      0,
      0,
      IP_MIN_SIZE,
      IP_STR_MAX_SIZE,
      NULL
      );

    //
    // Create iSCSILUN Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_LUN_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSILUN:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateStringOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_LUN_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_LUN_VAR_OFFSET + sizeof (KEYWORD_STR) * (Index - 1)),
      StringToken,
      StringToken,
      0,
      0,
      LUN_MIN_SIZE,
      LUN_MAX_SIZE,
      NULL
      );

    //
    // Create iSCSIAuthenticationMethod Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_AUTHENTICATION_METHOD_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIAuthenticationMethod:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateNumericOpCode (
    StartOpCodeHandle,
    (EFI_QUESTION_ID) (ATTEMPT_AUTHENTICATION_METHOD_QUESTION_ID + (Index - 1)),
    CONFIGURATION_VARSTORE_ID,
    (UINT16) (ATTEMPT_AUTHENTICATION_METHOD_VAR_OFFSET + (Index - 1)),
    StringToken,
    StringToken,
    0,
    0,
    0,
    1,
    0,
    NULL
    );

    //
    // Create iSCSIChapType Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_CHARTYPE_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIChapType:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateNumericOpCode (
    StartOpCodeHandle,
    (EFI_QUESTION_ID) (ATTEMPT_CHARTYPE_QUESTION_ID + (Index - 1)),
    CONFIGURATION_VARSTORE_ID,
    (UINT16) (ATTEMPT_CHARTYPE_VAR_OFFSET + (Index - 1)),
    StringToken,
    StringToken,
    0,
    0,
    0,
    1,
    0,
    NULL
    );

    //
    // Create iSCSIChapUsername Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_CHAR_USER_NAME_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIChapUsername:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateStringOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_CHAR_USER_NAME_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_CHAR_USER_NAME_VAR_OFFSET + sizeof (KEYWORD_STR) * (Index - 1)),
      StringToken,
      StringToken,
      0,
      0,
      0,
      ISCSI_CHAP_NAME_MAX_LEN,
      NULL
      );

    //
    // Create iSCSIChapSecret Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_CHAR_SECRET_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIChapSecret:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateStringOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_CHAR_SECRET_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_CHAR_SECRET_VAR_OFFSET + sizeof (KEYWORD_STR) * (Index - 1)),
      StringToken,
      StringToken,
      0,
      0,
      ISCSI_CHAP_SECRET_MIN_LEN,
      ISCSI_CHAP_SECRET_MAX_LEN,
      NULL
      );

    //
    // Create iSCSIReverseChapUsername Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_CHAR_REVERSE_USER_NAME_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIReverseChapUsername:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateStringOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_CHAR_REVERSE_USER_NAME_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_CHAR_REVERSE_USER_NAME_VAR_OFFSET + sizeof (KEYWORD_STR) * (Index - 1)),
      StringToken,
      StringToken,
      0,
      0,
      0,
      ISCSI_CHAP_NAME_MAX_LEN,
      NULL
      );

    //
    // Create iSCSIReverseChapSecret Keyword.
    //
    UnicodeSPrint (StringId, sizeof (StringId), L"STR_ISCSI_CHAR_REVERSE_SECRET_PROMPT%d", Index);
    StringToken =  HiiSetString (
                     mCallbackInfo->RegisteredHandle,
                     0,
                     StringId,
                     NULL
                     );
    UnicodeSPrint (KeywordId, sizeof (KeywordId), L"iSCSIReverseChapSecret:%d", Index);
    HiiSetString (mCallbackInfo->RegisteredHandle, StringToken, KeywordId, "x-UEFI-ns");
    HiiCreateStringOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (ATTEMPT_CHAR_REVERSE_SECRET_QUESTION_ID + (Index - 1)),
      CONFIGURATION_VARSTORE_ID,
      (UINT16) (ATTEMPT_CHAR_REVERSE_SECRET_VAR_OFFSET + sizeof (KEYWORD_STR) * (Index - 1)),
      StringToken,
      StringToken,
      0,
      0,
      ISCSI_CHAP_SECRET_MIN_LEN,
      ISCSI_CHAP_SECRET_MAX_LEN,
      NULL
      );
  }

  Status = HiiUpdateForm (
             mCallbackInfo->RegisteredHandle, // HII handle
             &gIScsiConfigGuid,               // Formset GUID
             FORMID_ATTEMPT_FORM,             // Form ID
             StartOpCodeHandle,               // Label for where to insert opcodes
             EndOpCodeHandle                  // Replace data
             );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return Status;
}

/**

  Free the attempt configure data variable.

**/
VOID
IScsiCleanAttemptVariable (
  IN   VOID
)
{
  ISCSI_ATTEMPT_CONFIG_NVDATA   *AttemptConfigData;
  UINT8                         *AttemptConfigOrder;
  UINTN                         AttemptConfigOrderSize;
  UINTN                         Index;

  //
  // Get the initialized attempt order.
  //
  AttemptConfigOrder = IScsiGetVariableAndSize (
                         L"InitialAttemptOrder",
                         &gIScsiConfigGuid,
                         &AttemptConfigOrderSize
                         );
  if (AttemptConfigOrder == NULL || AttemptConfigOrderSize == 0) {
    return;
  }

  for (Index = 1; Index < AttemptConfigOrderSize / sizeof (UINT8); Index++) {
    UnicodeSPrint (
      mPrivate->PortString,
      (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
      L"Attempt %d",
      Index
      );

    GetVariable2 (
      mPrivate->PortString,
      &gEfiIScsiInitiatorNameProtocolGuid,
      (VOID**)&AttemptConfigData,
      NULL
      );

    if (AttemptConfigData != NULL) {
      gRT->SetVariable (
             mPrivate->PortString,
             &gEfiIScsiInitiatorNameProtocolGuid,
             0,
             0,
             NULL
             );
    }
  }
  return;
}

/**
  Get the recorded NIC info from global structure by the Index.

  @param[in]  NicIndex          The index indicates the position of NIC info.

  @return Pointer to the NIC info, or NULL if not found.

**/
ISCSI_NIC_INFO *
IScsiGetNicInfoByIndex (
  IN UINT8      NicIndex
  )
{
  LIST_ENTRY        *Entry;
  ISCSI_NIC_INFO    *NicInfo;

  NET_LIST_FOR_EACH (Entry, &mPrivate->NicInfoList) {
    NicInfo = NET_LIST_USER_STRUCT (Entry, ISCSI_NIC_INFO, Link);
    if (NicInfo->NicIndex == NicIndex) {
      return NicInfo;
    }
  }

  return NULL;
}


/**
  Get the NIC's PCI location and return it according to the composited
  format defined in iSCSI Boot Firmware Table.

  @param[in]   Controller        The handle of the controller.
  @param[out]  Bus               The bus number.
  @param[out]  Device            The device number.
  @param[out]  Function          The function number.

  @return      The composited representation of the NIC PCI location.

**/
UINT16
IScsiGetNICPciLocation (
  IN EFI_HANDLE  Controller,
  OUT UINTN      *Bus,
  OUT UINTN      *Device,
  OUT UINTN      *Function
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_HANDLE                PciIoHandle;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINTN                     Segment;

  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return 0;
  }

  Status = gBS->LocateDevicePath (
                  &gEfiPciIoProtocolGuid,
                  &DevicePath,
                  &PciIoHandle
                  );
  if (EFI_ERROR (Status)) {
    return 0;
  }

  Status = gBS->HandleProtocol (PciIoHandle, &gEfiPciIoProtocolGuid, (VOID **) &PciIo);
  if (EFI_ERROR (Status)) {
    return 0;
  }

  Status = PciIo->GetLocation (PciIo, &Segment, Bus, Device, Function);
  if (EFI_ERROR (Status)) {
    return 0;
  }

  return (UINT16) ((*Bus << 8) | (*Device << 3) | *Function);
}


/**
  Read the EFI variable (VendorGuid/Name) and return a dynamically allocated
  buffer, and the size of the buffer. If failure, return NULL.

  @param[in]   Name                   String part of EFI variable name.
  @param[in]   VendorGuid             GUID part of EFI variable name.
  @param[out]  VariableSize           Returns the size of the EFI variable that was read.

  @return Dynamically allocated memory that contains a copy of the EFI variable.
  @return Caller is responsible freeing the buffer.
  @retval NULL                   Variable was not read.

**/
VOID *
IScsiGetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *VendorGuid,
  OUT UINTN               *VariableSize
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;
  VOID        *Buffer;

  Buffer = NULL;

  //
  // Pass in a zero size buffer to find the required buffer size.
  //
  BufferSize  = 0;
  Status      = gRT->GetVariable (Name, VendorGuid, NULL, &BufferSize, Buffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Allocate the buffer to return
    //
    Buffer = AllocateZeroPool (BufferSize);
    if (Buffer == NULL) {
      return NULL;
    }
    //
    // Read variable into the allocated buffer.
    //
    Status = gRT->GetVariable (Name, VendorGuid, NULL, &BufferSize, Buffer);
    if (EFI_ERROR (Status)) {
      BufferSize = 0;
    }
  }

  *VariableSize = BufferSize;
  return Buffer;
}


/**
  Create the iSCSI driver data.

  @param[in] Image      The handle of the driver image.
  @param[in] Controller The handle of the controller.

  @return The iSCSI driver data created.
  @retval NULL Other errors as indicated.

**/
ISCSI_DRIVER_DATA *
IScsiCreateDriverData (
  IN EFI_HANDLE  Image,
  IN EFI_HANDLE  Controller
  )
{
  ISCSI_DRIVER_DATA *Private;
  EFI_STATUS        Status;

  Private = AllocateZeroPool (sizeof (ISCSI_DRIVER_DATA));
  if (Private == NULL) {
    return NULL;
  }

  Private->Signature  = ISCSI_DRIVER_DATA_SIGNATURE;
  Private->Image      = Image;
  Private->Controller = Controller;
  Private->Session    = NULL;

  //
  // Create an event to be signaled when the BS to RT transition is triggerd so
  // as to abort the iSCSI session.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  IScsiOnExitBootService,
                  Private,
                  &gEfiEventExitBootServicesGuid,
                  &Private->ExitBootServiceEvent
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Private);
    return NULL;
  }

  Private->ExtScsiPassThruHandle = NULL;
  CopyMem(&Private->IScsiExtScsiPassThru, &gIScsiExtScsiPassThruProtocolTemplate, sizeof(EFI_EXT_SCSI_PASS_THRU_PROTOCOL));

  //
  // 0 is designated to the TargetId, so use another value for the AdapterId.
  //
  Private->ExtScsiPassThruMode.AdapterId  = 2;
  Private->ExtScsiPassThruMode.Attributes = EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL | EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL;
  Private->ExtScsiPassThruMode.IoAlign    = 4;
  Private->IScsiExtScsiPassThru.Mode      = &Private->ExtScsiPassThruMode;

  return Private;
}


/**
  Clean the iSCSI driver data.

  @param[in]              Private The iSCSI driver data.

  @retval EFI_SUCCESS     The clean operation is successful.
  @retval Others          Other errors as indicated.

**/
EFI_STATUS
IScsiCleanDriverData (
  IN ISCSI_DRIVER_DATA  *Private
  )
{
  EFI_STATUS            Status;

  Status = EFI_SUCCESS;

  if (Private->DevicePath != NULL) {
    Status = gBS->UninstallProtocolInterface (
                    Private->ExtScsiPassThruHandle,
                    &gEfiDevicePathProtocolGuid,
                    Private->DevicePath
                    );
    if (EFI_ERROR (Status)) {
      goto EXIT;
    }

    FreePool (Private->DevicePath);
  }

  if (Private->ExtScsiPassThruHandle != NULL) {
    Status = gBS->UninstallProtocolInterface (
                    Private->ExtScsiPassThruHandle,
                    &gEfiExtScsiPassThruProtocolGuid,
                    &Private->IScsiExtScsiPassThru
                    );
    if (!EFI_ERROR (Status)) {
      mPrivate->OneSessionEstablished = FALSE;
    }
  }

EXIT:
  if (Private->ExitBootServiceEvent != NULL) {
    gBS->CloseEvent (Private->ExitBootServiceEvent);
  }

  mCallbackInfo->Current = NULL;

  FreePool (Private);
  return Status;
}

/**
  Check wheather the Controller handle is configured to use DHCP protocol.

  @param[in]  Controller           The handle of the controller.
  @param[in]  IpVersion            IP_VERSION_4 or IP_VERSION_6.

  @retval TRUE                     The handle of the controller need the Dhcp protocol.
  @retval FALSE                    The handle of the controller does not need the Dhcp protocol.

**/
BOOLEAN
IScsiDhcpIsConfigured (
  IN EFI_HANDLE  Controller,
  IN UINT8       IpVersion
  )
{
  ISCSI_ATTEMPT_CONFIG_NVDATA *AttemptTmp;
  UINT8                       *AttemptConfigOrder;
  UINTN                       AttemptConfigOrderSize;
  UINTN                       Index;
  EFI_STATUS                  Status;
  EFI_MAC_ADDRESS             MacAddr;
  UINTN                       HwAddressSize;
  UINT16                      VlanId;
  CHAR16                      MacString[ISCSI_MAX_MAC_STRING_LEN];
  CHAR16                      AttemptMacString[ISCSI_MAX_MAC_STRING_LEN];
  CHAR16                      AttemptName[ISCSI_NAME_IFR_MAX_SIZE];

  AttemptConfigOrder = IScsiGetVariableAndSize (
                         L"AttemptOrder",
                         &gIScsiConfigGuid,
                         &AttemptConfigOrderSize
                         );
  if (AttemptConfigOrder == NULL || AttemptConfigOrderSize == 0) {
    return FALSE;
  }

  //
  // Get MAC address of this network device.
  //
  Status = NetLibGetMacAddress (Controller, &MacAddr, &HwAddressSize);
  if(EFI_ERROR (Status)) {
    return FALSE;
  }
  //
  // Get VLAN ID of this network device.
  //
  VlanId = NetLibGetVlanId (Controller);
  IScsiMacAddrToStr (&MacAddr, (UINT32) HwAddressSize, VlanId, MacString);

  for (Index = 0; Index < AttemptConfigOrderSize / sizeof (UINT8); Index++) {
    UnicodeSPrint (
      AttemptName,
      (UINTN) 128,
      L"Attempt %d",
      (UINTN) AttemptConfigOrder[Index]
      );
    Status = GetVariable2 (
               AttemptName,
               &gEfiIScsiInitiatorNameProtocolGuid,
               (VOID**)&AttemptTmp,
               NULL
               );
    if(AttemptTmp == NULL || EFI_ERROR (Status)) {
      continue;
    }

    ASSERT (AttemptConfigOrder[Index] == AttemptTmp->AttemptConfigIndex);

    if (AttemptTmp->SessionConfigData.Enabled == ISCSI_DISABLED) {
      FreePool (AttemptTmp);
      continue;
    }

    if (AttemptTmp->SessionConfigData.IpMode != IP_MODE_AUTOCONFIG &&
        AttemptTmp->SessionConfigData.IpMode != ((IpVersion == IP_VERSION_4) ? IP_MODE_IP4 : IP_MODE_IP6)) {
      FreePool (AttemptTmp);
      continue;
    }

    AsciiStrToUnicodeStrS (AttemptTmp->MacString, AttemptMacString, sizeof (AttemptMacString) / sizeof (AttemptMacString[0]));

    if (AttemptTmp->Actived == ISCSI_ACTIVE_DISABLED || StrCmp (MacString, AttemptMacString)) {
      continue;
    }

    if(AttemptTmp->SessionConfigData.IpMode == IP_MODE_AUTOCONFIG ||
       AttemptTmp->SessionConfigData.InitiatorInfoFromDhcp == TRUE ||
       AttemptTmp->SessionConfigData.TargetInfoFromDhcp == TRUE) {
      FreePool (AttemptTmp);
      FreePool (AttemptConfigOrder);
      return TRUE;
    }

    FreePool (AttemptTmp);
  }

  FreePool (AttemptConfigOrder);
  return FALSE;
}

/**
  Check whether the Controller handle is configured to use DNS protocol.

  @param[in]  Controller           The handle of the controller.

  @retval TRUE                     The handle of the controller need the Dns protocol.
  @retval FALSE                    The handle of the controller does not need the Dns protocol.

**/
BOOLEAN
IScsiDnsIsConfigured (
  IN EFI_HANDLE  Controller
  )
{
  ISCSI_ATTEMPT_CONFIG_NVDATA *AttemptTmp;
  UINT8                       *AttemptConfigOrder;
  UINTN                       AttemptConfigOrderSize;
  UINTN                       Index;
  EFI_STATUS                  Status;
  EFI_MAC_ADDRESS             MacAddr;
  UINTN                       HwAddressSize;
  UINT16                      VlanId;
  CHAR16                      AttemptMacString[ISCSI_MAX_MAC_STRING_LEN];
  CHAR16                      MacString[ISCSI_MAX_MAC_STRING_LEN];
  CHAR16                      AttemptName[ISCSI_NAME_IFR_MAX_SIZE];

  AttemptConfigOrder = IScsiGetVariableAndSize (
                         L"AttemptOrder",
                         &gIScsiConfigGuid,
                         &AttemptConfigOrderSize
                         );
  if (AttemptConfigOrder == NULL || AttemptConfigOrderSize == 0) {
    return FALSE;
  }

  //
  // Get MAC address of this network device.
  //
  Status = NetLibGetMacAddress (Controller, &MacAddr, &HwAddressSize);
  if(EFI_ERROR (Status)) {
    return FALSE;
  }
  //
  // Get VLAN ID of this network device.
  //
  VlanId = NetLibGetVlanId (Controller);
  IScsiMacAddrToStr (&MacAddr, (UINT32) HwAddressSize, VlanId, MacString);

  for (Index = 0; Index < AttemptConfigOrderSize / sizeof (UINT8); Index++) {
    UnicodeSPrint (
      AttemptName,
      (UINTN) 128,
      L"Attempt %d",
      (UINTN) AttemptConfigOrder[Index]
      );

    Status = GetVariable2 (
               AttemptName,
               &gEfiIScsiInitiatorNameProtocolGuid,
               (VOID**)&AttemptTmp,
               NULL
               );
    if(AttemptTmp == NULL || EFI_ERROR (Status)) {
      continue;
    }

    ASSERT (AttemptConfigOrder[Index] == AttemptTmp->AttemptConfigIndex);

    AsciiStrToUnicodeStrS (AttemptTmp->MacString, AttemptMacString, sizeof (AttemptMacString) / sizeof (AttemptMacString[0]));

    if (AttemptTmp->SessionConfigData.Enabled == ISCSI_DISABLED || StrCmp (MacString, AttemptMacString)) {
      FreePool (AttemptTmp);
      continue;
    }

    if (AttemptTmp->SessionConfigData.DnsMode || AttemptTmp->SessionConfigData.TargetInfoFromDhcp) {
      FreePool (AttemptTmp);
      FreePool (AttemptConfigOrder);
      return TRUE;
    } else {
      FreePool (AttemptTmp);
      continue;
    }

  }

  FreePool (AttemptConfigOrder);
  return FALSE;

}

/**
  Get the various configuration data.

  @param[in]  Private   The iSCSI driver data.

  @retval EFI_SUCCESS            The configuration data is retrieved.
  @retval EFI_NOT_FOUND          This iSCSI driver is not configured yet.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.

**/
EFI_STATUS
IScsiGetConfigData (
  IN ISCSI_DRIVER_DATA  *Private
  )
{
  EFI_STATUS                  Status;
  CHAR16                      MacString[ISCSI_MAX_MAC_STRING_LEN];
  CHAR16                      AttemptMacString[ISCSI_MAX_MAC_STRING_LEN];
  UINTN                       Index;
  ISCSI_NIC_INFO              *NicInfo;
  ISCSI_ATTEMPT_CONFIG_NVDATA *AttemptConfigData;
  ISCSI_ATTEMPT_CONFIG_NVDATA *AttemptTmp;
  UINT8                       *AttemptConfigOrder;
  UINTN                       AttemptConfigOrderSize;
  CHAR16                      IScsiMode[64];
  CHAR16                      IpMode[64];

  //
  // There should be at least one attempt configured.
  //
  AttemptConfigOrder = IScsiGetVariableAndSize (
                         L"AttemptOrder",
                         &gIScsiConfigGuid,
                         &AttemptConfigOrderSize
                         );
  if (AttemptConfigOrder == NULL || AttemptConfigOrderSize == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Get the iSCSI Initiator Name.
  //
  mPrivate->InitiatorNameLength  = ISCSI_NAME_MAX_SIZE;
  Status = gIScsiInitiatorName.Get (
                                 &gIScsiInitiatorName,
                                 &mPrivate->InitiatorNameLength,
                                 mPrivate->InitiatorName
                                 );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the normal configuration.
  //
  for (Index = 0; Index < AttemptConfigOrderSize / sizeof (UINT8); Index++) {

    //
    // Check whether the attempt exists in AttemptConfig.
    //
    AttemptTmp = IScsiConfigGetAttemptByConfigIndex (AttemptConfigOrder[Index]);
    if (AttemptTmp != NULL && AttemptTmp->SessionConfigData.Enabled == ISCSI_DISABLED) {
      continue;
    } else if (AttemptTmp != NULL && AttemptTmp->SessionConfigData.Enabled != ISCSI_DISABLED) {
      //
      // Check the autoconfig path to see whether it should be retried.
      //
      if (AttemptTmp->SessionConfigData.IpMode == IP_MODE_AUTOCONFIG &&
          !AttemptTmp->AutoConfigureSuccess) {
        if (mPrivate->Ipv6Flag &&
            AttemptTmp->AutoConfigureMode == IP_MODE_AUTOCONFIG_IP6) {
          //
          // Autoconfigure for IP6 already attempted but failed. Do not try again.
          //
          continue;
        } else if (!mPrivate->Ipv6Flag &&
                   AttemptTmp->AutoConfigureMode == IP_MODE_AUTOCONFIG_IP4) {
          //
          // Autoconfigure for IP4  already attempted but failed. Do not try again.
          //
          continue;
        } else {
          //
          // Try another approach for this autoconfigure path.
          //
          AttemptTmp->AutoConfigureMode =
            (UINT8) (mPrivate->Ipv6Flag ? IP_MODE_AUTOCONFIG_IP6 : IP_MODE_AUTOCONFIG_IP4);
          AttemptTmp->SessionConfigData.InitiatorInfoFromDhcp = TRUE;
          AttemptTmp->SessionConfigData.TargetInfoFromDhcp    = TRUE;
          AttemptTmp->DhcpSuccess                             = FALSE;

          //
          // Get some information from the dhcp server.
          //
          if (!mPrivate->Ipv6Flag) {
            Status = IScsiDoDhcp (Private->Image, Private->Controller, AttemptTmp);
            if (!EFI_ERROR (Status)) {
              AttemptTmp->DhcpSuccess = TRUE;
            }
          } else {
            Status = IScsiDoDhcp6 (Private->Image, Private->Controller, AttemptTmp);
            if (!EFI_ERROR (Status)) {
              AttemptTmp->DhcpSuccess = TRUE;
            }
          }

          //
          // Refresh the state of this attempt to NVR.
          //
          UnicodeSPrint (
            mPrivate->PortString,
            (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
            L"Attempt %d",
            (UINTN) AttemptTmp->AttemptConfigIndex
            );

          gRT->SetVariable (
                 mPrivate->PortString,
                 &gEfiIScsiInitiatorNameProtocolGuid,
                 ISCSI_CONFIG_VAR_ATTR,
                 sizeof (ISCSI_ATTEMPT_CONFIG_NVDATA),
                 AttemptTmp
                 );

          continue;
        }
      } else if (AttemptTmp->SessionConfigData.InitiatorInfoFromDhcp &&
                 !AttemptTmp->ValidPath &&
                 AttemptTmp->NicIndex == mPrivate->CurrentNic) {
        //
        // If the attempt associates with the current NIC, we can
        // get DHCP information for already added, but failed, attempt.
        //
        AttemptTmp->DhcpSuccess = FALSE;
        if (!mPrivate->Ipv6Flag && (AttemptTmp->SessionConfigData.IpMode == IP_MODE_IP4)) {
          Status = IScsiDoDhcp (Private->Image, Private->Controller, AttemptTmp);
          if (!EFI_ERROR (Status)) {
            AttemptTmp->DhcpSuccess = TRUE;
          }
        } else if (mPrivate->Ipv6Flag && (AttemptTmp->SessionConfigData.IpMode == IP_MODE_IP6)) {
          Status = IScsiDoDhcp6 (Private->Image, Private->Controller, AttemptTmp);
          if (!EFI_ERROR (Status)) {
            AttemptTmp->DhcpSuccess = TRUE;
          }
        }

        //
        // Refresh the state of this attempt to NVR.
        //
        UnicodeSPrint (
          mPrivate->PortString,
          (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
          L"Attempt %d",
          (UINTN) AttemptTmp->AttemptConfigIndex
          );

        gRT->SetVariable (
               mPrivate->PortString,
               &gEfiIScsiInitiatorNameProtocolGuid,
               ISCSI_CONFIG_VAR_ATTR,
               sizeof (ISCSI_ATTEMPT_CONFIG_NVDATA),
               AttemptTmp
               );

        continue;

      } else {
        continue;
      }
    }

    //
    // This attempt does not exist in AttemptConfig. Try to add a new one.
    //

    NicInfo = IScsiGetNicInfoByIndex (mPrivate->CurrentNic);
    ASSERT (NicInfo != NULL);
    IScsiMacAddrToStr (&NicInfo->PermanentAddress, NicInfo->HwAddressSize, NicInfo->VlanId, MacString);
    UnicodeSPrint (
      mPrivate->PortString,
      (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
      L"Attempt %d",
      (UINTN) AttemptConfigOrder[Index]
      );

    GetVariable2 (
      mPrivate->PortString,
      &gEfiIScsiInitiatorNameProtocolGuid,
      (VOID**)&AttemptConfigData,
      NULL
      );
    AsciiStrToUnicodeStrS (AttemptConfigData->MacString, AttemptMacString, sizeof (AttemptMacString) / sizeof (AttemptMacString[0]));

    if (AttemptConfigData == NULL || AttemptConfigData->Actived == ISCSI_ACTIVE_DISABLED ||
        StrCmp (MacString, AttemptMacString)) {
      continue;
    }

    ASSERT (AttemptConfigOrder[Index] == AttemptConfigData->AttemptConfigIndex);

    AttemptConfigData->NicIndex      = NicInfo->NicIndex;
    AttemptConfigData->DhcpSuccess   = FALSE;
    AttemptConfigData->ValidiBFTPath = (BOOLEAN) (mPrivate->EnableMpio ? TRUE : FALSE);
    AttemptConfigData->ValidPath     = FALSE;

    if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_AUTOCONFIG) {
      AttemptConfigData->SessionConfigData.InitiatorInfoFromDhcp = TRUE;
      AttemptConfigData->SessionConfigData.TargetInfoFromDhcp    = TRUE;

      AttemptConfigData->AutoConfigureMode =
        (UINT8) (mPrivate->Ipv6Flag ? IP_MODE_AUTOCONFIG_IP6 : IP_MODE_AUTOCONFIG_IP4);
      AttemptConfigData->AutoConfigureSuccess = FALSE;
    }

    //
    // Get some information from dhcp server.
    //
    if (AttemptConfigData->SessionConfigData.Enabled != ISCSI_DISABLED &&
        AttemptConfigData->SessionConfigData.InitiatorInfoFromDhcp) {

      if (!mPrivate->Ipv6Flag &&
          (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_IP4 ||
           AttemptConfigData->AutoConfigureMode == IP_MODE_AUTOCONFIG_IP4)) {
        Status = IScsiDoDhcp (Private->Image, Private->Controller, AttemptConfigData);
        if (!EFI_ERROR (Status)) {
          AttemptConfigData->DhcpSuccess = TRUE;
        }
      } else if (mPrivate->Ipv6Flag &&
                (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_IP6 ||
                 AttemptConfigData->AutoConfigureMode == IP_MODE_AUTOCONFIG_IP6)) {
        Status = IScsiDoDhcp6 (Private->Image, Private->Controller, AttemptConfigData);
        if (!EFI_ERROR (Status)) {
          AttemptConfigData->DhcpSuccess = TRUE;
        }
      }

      //
      // Refresh the state of this attempt to NVR.
      //
      UnicodeSPrint (
        mPrivate->PortString,
        (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
        L"Attempt %d",
        (UINTN) AttemptConfigData->AttemptConfigIndex
        );

      gRT->SetVariable (
             mPrivate->PortString,
             &gEfiIScsiInitiatorNameProtocolGuid,
             ISCSI_CONFIG_VAR_ATTR,
             sizeof (ISCSI_ATTEMPT_CONFIG_NVDATA),
             AttemptConfigData
             );
    }

    //
    // Update Attempt Help Info.
    //

    if (AttemptConfigData->SessionConfigData.Enabled == ISCSI_DISABLED) {
      UnicodeSPrint (IScsiMode, 64, L"Disabled");
    } else if (AttemptConfigData->SessionConfigData.Enabled == ISCSI_ENABLED) {
      UnicodeSPrint (IScsiMode, 64, L"Enabled");
    } else if (AttemptConfigData->SessionConfigData.Enabled == ISCSI_ENABLED_FOR_MPIO) {
      UnicodeSPrint (IScsiMode, 64, L"Enabled for MPIO");
    }

    if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_IP4) {
      UnicodeSPrint (IpMode, 64, L"IP4");
    } else if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_IP6) {
      UnicodeSPrint (IpMode, 64, L"IP6");
    } else if (AttemptConfigData->SessionConfigData.IpMode == IP_MODE_AUTOCONFIG) {
      UnicodeSPrint (IpMode, 64, L"Autoconfigure");
    }

    UnicodeSPrint (
      mPrivate->PortString,
      (UINTN) ISCSI_NAME_IFR_MAX_SIZE,
      L"MAC: %s, PFA: Bus %d | Dev %d | Func %d, iSCSI mode: %s, IP version: %s",
      MacString,
      NicInfo->BusNumber,
      NicInfo->DeviceNumber,
      NicInfo->FunctionNumber,
      IScsiMode,
      IpMode
      );

    AttemptConfigData->AttemptTitleHelpToken = HiiSetString (
                                                 mCallbackInfo->RegisteredHandle,
                                                 0,
                                                 mPrivate->PortString,
                                                 NULL
                                                 );
    if (AttemptConfigData->AttemptTitleHelpToken == 0) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Record the attempt in global link list.
    //
    InsertTailList (&mPrivate->AttemptConfigs, &AttemptConfigData->Link);
    mPrivate->AttemptCount++;

    if (AttemptConfigData->SessionConfigData.Enabled == ISCSI_ENABLED_FOR_MPIO) {
      mPrivate->MpioCount++;
      mPrivate->EnableMpio = TRUE;

      if (AttemptConfigData->AuthenticationType == ISCSI_AUTH_TYPE_KRB) {
        mPrivate->Krb5MpioCount++;
      }
    } else if (AttemptConfigData->SessionConfigData.Enabled == ISCSI_ENABLED) {
      mPrivate->SinglePathCount++;
    }
  }

  //
  // Reorder the AttemptConfig by the configured order.
  //
  for (Index = 0; Index < AttemptConfigOrderSize / sizeof (UINT8); Index++) {
    AttemptConfigData = IScsiConfigGetAttemptByConfigIndex (AttemptConfigOrder[Index]);
    if (AttemptConfigData == NULL) {
      continue;
    }

    RemoveEntryList (&AttemptConfigData->Link);
    InsertTailList (&mPrivate->AttemptConfigs, &AttemptConfigData->Link);
  }

  //
  // Update the Main Form.
  //
  IScsiConfigUpdateAttempt ();

  FreePool (AttemptConfigOrder);

  //
  //  There should be at least one attempt configuration.
  //
  if (!mPrivate->EnableMpio) {
    if (mPrivate->SinglePathCount == 0) {
      return EFI_NOT_FOUND;
    }
    mPrivate->ValidSinglePathCount = mPrivate->SinglePathCount;
  }

  return EFI_SUCCESS;
}


/**
  Get the device path of the iSCSI tcp connection and update it.

  @param  Session                The iSCSI session.

  @return The updated device path.
  @retval NULL Other errors as indicated.

**/
EFI_DEVICE_PATH_PROTOCOL *
IScsiGetTcpConnDevicePath (
  IN ISCSI_SESSION      *Session
  )
{
  ISCSI_CONNECTION          *Conn;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_STATUS                Status;
  EFI_DEV_PATH              *DPathNode;
  UINTN                     PathLen;

  if (Session->State != SESSION_STATE_LOGGED_IN) {
    return NULL;
  }

  Conn = NET_LIST_USER_STRUCT_S (
           Session->Conns.ForwardLink,
           ISCSI_CONNECTION,
           Link,
           ISCSI_CONNECTION_SIGNATURE
           );

  Status = gBS->HandleProtocol (
                  Conn->TcpIo.Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  //
  // Duplicate it.
  //
  DevicePath  = DuplicateDevicePath (DevicePath);
  if (DevicePath == NULL) {
    return NULL;
  }

  DPathNode   = (EFI_DEV_PATH *) DevicePath;

  while (!IsDevicePathEnd (&DPathNode->DevPath)) {
    if (DevicePathType (&DPathNode->DevPath) == MESSAGING_DEVICE_PATH) {
      if (!Conn->Ipv6Flag && DevicePathSubType (&DPathNode->DevPath) == MSG_IPv4_DP) {
        DPathNode->Ipv4.LocalPort       = 0;

        DPathNode->Ipv4.StaticIpAddress =
          (BOOLEAN) (!Session->ConfigData->SessionConfigData.InitiatorInfoFromDhcp);

        //
        //  Add a judgement here to support previous versions of IPv4_DEVICE_PATH.
        //  In previous versions of IPv4_DEVICE_PATH, GatewayIpAddress and SubnetMask
        //  do not exist.
        //  In new version of IPv4_DEVICE_PATH, structure length is 27.
        //

        PathLen = DevicePathNodeLength (&DPathNode->Ipv4);

        if (PathLen == IP4_NODE_LEN_NEW_VERSIONS) {

          IP4_COPY_ADDRESS (
            &DPathNode->Ipv4.GatewayIpAddress,
            &Session->ConfigData->SessionConfigData.Gateway
            );

          IP4_COPY_ADDRESS (
            &DPathNode->Ipv4.SubnetMask,
            &Session->ConfigData->SessionConfigData.SubnetMask
            );
        }

        break;
      } else if (Conn->Ipv6Flag && DevicePathSubType (&DPathNode->DevPath) == MSG_IPv6_DP) {
        DPathNode->Ipv6.LocalPort       = 0;

        //
        //  Add a judgement here to support previous versions of IPv6_DEVICE_PATH.
        //  In previous versions of IPv6_DEVICE_PATH, IpAddressOrigin, PrefixLength
        //  and GatewayIpAddress do not exist.
        //  In new version of IPv6_DEVICE_PATH, structure length is 60, while in
        //  old versions, the length is 43.
        //

        PathLen = DevicePathNodeLength (&DPathNode->Ipv6);

        if (PathLen == IP6_NODE_LEN_NEW_VERSIONS ) {

          DPathNode->Ipv6.IpAddressOrigin = 0;
          DPathNode->Ipv6.PrefixLength    = IP6_PREFIX_LENGTH;
          ZeroMem (&DPathNode->Ipv6.GatewayIpAddress, sizeof (EFI_IPv6_ADDRESS));
        }
        else if (PathLen == IP6_NODE_LEN_OLD_VERSIONS) {

          //
          //  StaticIPAddress is a field in old versions of IPv6_DEVICE_PATH, while ignored in new
          //  version. Set StaticIPAddress through its' offset in old IPv6_DEVICE_PATH.
          //
          *((UINT8 *)(&DPathNode->Ipv6) + IP6_OLD_IPADDRESS_OFFSET) =
            (BOOLEAN) (!Session->ConfigData->SessionConfigData.InitiatorInfoFromDhcp);
        }

        break;
      }
    }

    DPathNode = (EFI_DEV_PATH *) NextDevicePathNode (&DPathNode->DevPath);
  }

  return DevicePath;
}


/**
  Abort the session when the transition from BS to RT is initiated.

  @param[in]   Event  The event signaled.
  @param[in]  Context The iSCSI driver data.

**/
VOID
EFIAPI
IScsiOnExitBootService (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  ISCSI_DRIVER_DATA *Private;

  Private = (ISCSI_DRIVER_DATA *) Context;

  gBS->CloseEvent (Private->ExitBootServiceEvent);
  Private->ExitBootServiceEvent = NULL;

  if (Private->Session != NULL) {
    IScsiSessionAbort (Private->Session);
  }
}

/**
  Tests whether a controller handle is being managed by IScsi driver.

  This function tests whether the driver specified by DriverBindingHandle is
  currently managing the controller specified by ControllerHandle.  This test
  is performed by evaluating if the protocol specified by ProtocolGuid is
  present on ControllerHandle and is was opened by DriverBindingHandle and Nic
  Device handle with an attribute of EFI_OPEN_PROTOCOL_BY_DRIVER.
  If ProtocolGuid is NULL, then ASSERT().

  @param  ControllerHandle     A handle for a controller to test.
  @param  DriverBindingHandle  Specifies the driver binding handle for the
                               driver.
  @param  ProtocolGuid         Specifies the protocol that the driver specified
                               by DriverBindingHandle opens in its Start()
                               function.

  @retval EFI_SUCCESS          ControllerHandle is managed by the driver
                               specified by DriverBindingHandle.
  @retval EFI_UNSUPPORTED      ControllerHandle is not managed by the driver
                               specified by DriverBindingHandle.

**/
EFI_STATUS
EFIAPI
IScsiTestManagedDevice (
  IN  EFI_HANDLE       ControllerHandle,
  IN  EFI_HANDLE       DriverBindingHandle,
  IN  EFI_GUID         *ProtocolGuid
  )
{
  EFI_STATUS     Status;
  VOID           *ManagedInterface;
  EFI_HANDLE     NicControllerHandle;

  ASSERT (ProtocolGuid != NULL);

  NicControllerHandle = NetLibGetNicHandle (ControllerHandle, ProtocolGuid);
  if (NicControllerHandle == NULL) {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  (EFI_GUID *) ProtocolGuid,
                  &ManagedInterface,
                  DriverBindingHandle,
                  NicControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           ControllerHandle,
           (EFI_GUID *) ProtocolGuid,
           DriverBindingHandle,
           NicControllerHandle
           );
    return EFI_UNSUPPORTED;
  }

  if (Status != EFI_ALREADY_STARTED) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}
