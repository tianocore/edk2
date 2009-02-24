/** @file
  Miscellaneous routines for iSCSI driver.

Copyright (c) 2004 - 2008, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IScsiImpl.h"

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8  IScsiHexString[] = "0123456789ABCDEFabcdef";

/**
  Removes (trims) specified leading and trailing characters from a string.

  @param[in, out]  Str  Pointer to the null-terminated string to be trimmed. On return, 
                        Str will hold the trimmed string. 

  @param[in]      CharC Character will be trimmed from str.
**/
VOID
StrTrim (
  IN OUT CHAR16   *Str,
  IN     CHAR16   CharC
  )
{
  CHAR16  *Pointer1;
  CHAR16  *Pointer2;
  
  if (*Str == 0) {
    return;
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

  @return The prefix length of the subnet mask.
  @retval 0 Other errors as indicated.
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

  @retval EFI_SUCCESS           The 64-bit LUN is stored in Lun.
  @retval EFI_INVALID_PARAMETER The string is malformatted.
**/
EFI_STATUS
IScsiAsciiStrToLun (
  IN  CHAR8  *Str,
  OUT UINT8  *Lun
  )
{
  UINT32  Index;
  CHAR8   *LunUnitStr[4];
  CHAR8   Digit;
  UINTN   Temp;

  ZeroMem (Lun, 8);
  ZeroMem (LunUnitStr, sizeof (LunUnitStr));

  Index         = 0;
  LunUnitStr[0] = Str;

  if (!IsHexDigit ((UINT8 *) &Digit, *Str)) {
    return EFI_INVALID_PARAMETER;
  }

  while (*Str != '\0') {
    //
    // Legal representations of LUN:
    //   4752-3A4F-6b7e-2F99,
    //   6734-9-156f-127,
    //   4186-9
    //
    if (*Str == '-') {
      *Str = '\0';
      Index++;

      if (*(Str + 1) != '\0') {
        if (!IsHexDigit ((UINT8 *) &Digit, *(Str + 1))) {
          return EFI_INVALID_PARAMETER;
        }

        LunUnitStr[Index] = Str + 1;
      }
    } else if (!IsHexDigit ((UINT8 *) &Digit, *Str)) {
      return EFI_INVALID_PARAMETER;
    }

    Str++;
  }

  for (Index = 0; (Index < 4) && (LunUnitStr[Index] != NULL); Index++) {
    if (AsciiStrLen (LunUnitStr[Index]) > 4) {
      return EFI_INVALID_PARAMETER;
    }

    Temp = AsciiStrHexToUintn (LunUnitStr[Index]);
    *((UINT16 *) &Lun[Index * 2]) = HTONS (Temp);
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
      StrCpy (TempStr, L"0-");
    } else {
      TempStr[0]  = (CHAR16) IScsiHexString[Lun[2 * Index] >> 4];
      TempStr[1]  = (CHAR16) IScsiHexString[Lun[2 * Index] & 0x0F];
      TempStr[2]  = (CHAR16) IScsiHexString[Lun[2 * Index + 1] >> 4];
      TempStr[3]  = (CHAR16) IScsiHexString[Lun[2 * Index + 1] & 0x0F];
      TempStr[4]  = L'-';
      TempStr[5]  = 0;

      StrTrim (TempStr, L'0');
    }

    TempStr += StrLen (TempStr);
  }

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
  Convert the ASCII string into a UNICODE string.

  @param[in]   Source      The ASCII string.
  @param[out]  Destination The storage to return the UNICODE string.

  @return CHAR16 *         Pointer to the UNICODE string.
**/
CHAR16 *
IScsiAsciiStrToUnicodeStr (
  IN  CHAR8   *Source,
  OUT CHAR16  *Destination
  )
{
  ASSERT (Destination != NULL);
  ASSERT (Source != NULL);

  while (*Source != '\0') {
    *(Destination++) = (CHAR16) *(Source++);
  }

  *Destination = '\0';

  return Destination;
}

/**
  Convert the UNICODE string into an ASCII string.

  @param[in]  Source       The UNICODE string.
  @param[out] Destination  The storage to return the ASCII string.

  @return CHAR8 *          Pointer to the ASCII string.
**/
CHAR8 *
IScsiUnicodeStrToAsciiStr (
  IN  CHAR16  *Source,
  OUT CHAR8   *Destination
  )
{
  ASSERT (Destination != NULL);
  ASSERT (Source != NULL);

  while (*Source != '\0') {
    //
    // If any Unicode characters in Source contain
    // non-zero value in the upper 8 bits, then ASSERT().
    //
    ASSERT (*Source < 0x100);
    *(Destination++) = (CHAR8) *(Source++);
  }

  *Destination = '\0';

  return Destination;
}

/**
  Convert the decimal dotted IPv4 address into the binary IPv4 address.

  @param[in]   Str             The UNICODE string.
  @param[out]  Ip              The storage to return the ASCII string.

  @retval EFI_SUCCESS           The binary IP address is returned in Ip.
  @retval EFI_INVALID_PARAMETER The IP string is malformatted.
**/
EFI_STATUS
IScsiAsciiStrToIp (
  IN  CHAR8             *Str,
  OUT EFI_IPv4_ADDRESS  *Ip
  )
{
  UINTN Index;
  UINTN Number;

  Index = 0;

  while (*Str != 0) {

    if (Index > 3) {
      return EFI_INVALID_PARAMETER;
    }

    Number = 0;
    while (NET_IS_DIGIT (*Str)) {
      Number = Number * 10 + (*Str - '0');
      Str++;
    }

    if (Number > 0xFF) {
      return EFI_INVALID_PARAMETER;
    }

    Ip->Addr[Index] = (UINT8) Number;

    if ((*Str != '\0') && (*Str != '.')) {
      //
      // The current character should be either the NULL terminator or
      // the dot delimiter.
      //
      return EFI_INVALID_PARAMETER;
    }

    if (*Str == '.') {
      //
      // Skip the delimiter.
      //
      Str++;
    }

    Index++;
  }

  if (Index != 4) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Convert the mac address into a hexadecimal encoded "-" seperated string.

  @param[in]  Mac The mac address.
  @param[in]  Len  Length in bytes of the mac address.
  @param[out] Str The storage to return the mac string.
**/
VOID
IScsiMacAddrToStr (
  IN  EFI_MAC_ADDRESS  *Mac,
  IN  UINT32           Len,
  OUT CHAR16           *Str
  )
{
  UINT32  Index;

  for (Index = 0; Index < Len; Index++) {
    Str[3 * Index]      = (CHAR16) IScsiHexString[(Mac->Addr[Index] >> 4) & 0x0F];
    Str[3 * Index + 1]  = (CHAR16) IScsiHexString[Mac->Addr[Index] & 0x0F];
    Str[3 * Index + 2]  = L'-';
  }

  Str[3 * Index - 1] = L'\0';
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
  // Prefix for Hex String
  //
  HexStr[0] = '0';
  HexStr[1] = 'x';

  for (Index = 0; Index < BinLength; Index++) {
    HexStr[Index * 2 + 2] = IScsiHexString[BinBuffer[Index] >> 4];
    HexStr[Index * 2 + 3] = IScsiHexString[BinBuffer[Index] & 0x0F];
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
  UINT32  HexCount;
  CHAR8   *HexBuf;
  UINT8   Digit;
  UINT8   Byte;

  Digit = 0;

  //
  // Find out how many hex characters the string has.
  //
  HexBuf = HexStr;
  if ((HexBuf[0] == '0') && ((HexBuf[1] == 'x') || (HexBuf[1] == 'X'))) {
    HexBuf += 2;
  }

  for (Index = 0, HexCount = 0; IsHexDigit (&Digit, HexBuf[Index]); Index++, HexCount++)
    ;

  if (HexCount == 0) {
    *BinLength = 0;
    return EFI_SUCCESS;
  }
  //
  // Test if buffer is passed enough.
  //
  if (((HexCount + 1) / 2) > *BinLength) {
    *BinLength = (HexCount + 1) / 2;
    return EFI_BUFFER_TOO_SMALL;
  }

  *BinLength = (HexCount + 1) / 2;

  for (Index = 0; Index < HexCount; Index++) {

    IsHexDigit (&Digit, HexBuf[HexCount - 1 - Index]);

    if ((Index & 1) == 0) {
      Byte = Digit;
    } else {
      Byte = BinBuffer[*BinLength - 1 - Index / 2];
      Byte &= 0x0F;
      Byte = (UINT8) (Byte | (Digit << 4));
    }

    BinBuffer[*BinLength - 1 - Index / 2] = Byte;
  }

  return EFI_SUCCESS;
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
  Create the iSCSI driver data..

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

  //
  // Create an event to be signal when the BS to RT transition is triggerd so
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
    gBS->FreePool (Private);
    return NULL;
  }

  CopyMem(&Private->IScsiExtScsiPassThru, &gIScsiExtScsiPassThruProtocolTemplate, sizeof(EFI_EXT_SCSI_PASS_THRU_PROTOCOL));

  //
  // 0 is designated to the TargetId, so use another value for the AdapterId.
  //
  Private->ExtScsiPassThruMode.AdapterId = 2;
  Private->ExtScsiPassThruMode.Attributes = EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL | EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL;
  Private->ExtScsiPassThruMode.IoAlign  = 4;
  Private->IScsiExtScsiPassThru.Mode    = &Private->ExtScsiPassThruMode;

  //
  // Install the Ext SCSI PASS THRU protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &Private->ExtScsiPassThruHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->IScsiExtScsiPassThru
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (Private->ExitBootServiceEvent);
    gBS->FreePool (Private);

    return NULL;
  }

  IScsiSessionInit (&Private->Session, FALSE);

  return Private;
}

/**
  Clean the iSCSI driver data.

  @param[in]  Private The iSCSI driver data.
**/
VOID
IScsiCleanDriverData (
  IN ISCSI_DRIVER_DATA  *Private
  )
{
  if (Private->DevicePath != NULL) {
    gBS->UninstallProtocolInterface (
          Private->ExtScsiPassThruHandle,
          &gEfiDevicePathProtocolGuid,
          Private->DevicePath
          );

    gBS->FreePool (Private->DevicePath);
  }

  if (Private->ExtScsiPassThruHandle != NULL) {
    gBS->UninstallProtocolInterface (
          Private->ExtScsiPassThruHandle,
          &gEfiExtScsiPassThruProtocolGuid,
          &Private->IScsiExtScsiPassThru
          );
  }

  gBS->CloseEvent (Private->ExitBootServiceEvent);

  gBS->FreePool (Private);
}

/**
  Get the various configuration data of this iSCSI instance.

  @param[in]  Private   The iSCSI driver data.

  @retval EFI_SUCCESS   The configuration of this instance is got.
  @retval EFI_ABORTED   The operation was aborted.
  @retval Others        Other errors as indicated.
**/
EFI_STATUS
IScsiGetConfigData (
  IN ISCSI_DRIVER_DATA  *Private
  )
{
  EFI_STATUS                  Status;
  ISCSI_SESSION               *Session;
  UINTN                       BufferSize;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  EFI_SIMPLE_NETWORK_MODE     *Mode;
  CHAR16                      MacString[65];

  //
  // get the iSCSI Initiator Name
  //
  Session                       = &Private->Session;
  Session->InitiatorNameLength  = ISCSI_NAME_MAX_SIZE;
  Status = gIScsiInitiatorName.Get (
                                &gIScsiInitiatorName,
                                &Session->InitiatorNameLength,
                                Session->InitiatorName
                                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  Private->Controller,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **)&Snp
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Mode = Snp->Mode;

  //
  // Get the mac string, it's the name of various variable
  //
  IScsiMacAddrToStr (&Mode->PermanentAddress, Mode->HwAddressSize, MacString);

  //
  // Get the normal configuration.
  //
  BufferSize = sizeof (Session->ConfigData.NvData);
  Status = gRT->GetVariable (
                  MacString,
                  &gEfiIScsiInitiatorNameProtocolGuid,
                  NULL,
                  &BufferSize,
                  &Session->ConfigData.NvData
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!Session->ConfigData.NvData.Enabled) {
    return EFI_ABORTED;
  }
  //
  // Get the CHAP Auth information.
  //
  BufferSize = sizeof (Session->AuthData.AuthConfig);
  Status = gRT->GetVariable (
                  MacString,
                  &mIScsiCHAPAuthInfoGuid,
                  NULL,
                  &BufferSize,
                  &Session->AuthData.AuthConfig
                  );

  if (!EFI_ERROR (Status) && Session->ConfigData.NvData.InitiatorInfoFromDhcp) {
    //
    // Start dhcp.
    //
    Status = IScsiDoDhcp (Private->Image, Private->Controller, &Session->ConfigData);
  }

  return Status;
}

/**
  Get the device path of the iSCSI tcp connection and update it.

  @param[in]  Private The iSCSI driver data.

  @return The updated device path.
  @retval NULL Other errors as indicated.
**/
EFI_DEVICE_PATH_PROTOCOL *
IScsiGetTcpConnDevicePath (
  IN ISCSI_DRIVER_DATA  *Private
  )
{
  ISCSI_SESSION             *Session;
  ISCSI_CONNECTION          *Conn;
  TCP4_IO                   *Tcp4Io;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_STATUS                Status;
  EFI_DEV_PATH              *DPathNode;

  Session = &Private->Session;
  if (Session->State != SESSION_STATE_LOGGED_IN) {
    return NULL;
  }

  Conn = NET_LIST_USER_STRUCT_S (
          Session->Conns.ForwardLink,
          ISCSI_CONNECTION,
          Link,
          ISCSI_CONNECTION_SIGNATURE
          );
  Tcp4Io = &Conn->Tcp4Io;

  Status = gBS->HandleProtocol (
                  Tcp4Io->Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  //
  // Duplicate it.
  //
  DevicePath  = DuplicateDevicePath (DevicePath);

  DPathNode   = (EFI_DEV_PATH *) DevicePath;

  while (!IsDevicePathEnd (&DPathNode->DevPath)) {
    if ((DevicePathType (&DPathNode->DevPath) == MESSAGING_DEVICE_PATH) &&
        (DevicePathSubType (&DPathNode->DevPath) == MSG_IPv4_DP)
        ) {

      DPathNode->Ipv4.LocalPort       = 0;
      DPathNode->Ipv4.StaticIpAddress = (BOOLEAN) (!Session->ConfigData.NvData.InitiatorInfoFromDhcp);
      break;
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

  IScsiSessionAbort (&Private->Session);
}
