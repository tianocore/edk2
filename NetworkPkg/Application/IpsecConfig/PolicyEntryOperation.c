/** @file
  The implementation of policy entry operation function in IpSecConfig application.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecConfig.h"
#include "Indexer.h"
#include "Match.h"
#include "Helper.h"
#include "ForEach.h"
#include "PolicyEntryOperation.h"

/**
  Fill in EFI_IPSEC_SPD_SELECTOR through ParamPackage list.

  @param[out]     Selector        The pointer to the EFI_IPSEC_SPD_SELECTOR structure.
  @param[in]      ParamPackage    The pointer to the ParamPackage list.
  @param[in, out] Mask            The pointer to the Mask.

  @retval EFI_SUCCESS              Fill in EFI_IPSEC_SPD_SELECTOR successfully.
  @retval EFI_INVALID_PARAMETER    Invalid user input parameter.

**/
EFI_STATUS
CreateSpdSelector (
     OUT EFI_IPSEC_SPD_SELECTOR    *Selector,
  IN     LIST_ENTRY                *ParamPackage,
  IN OUT UINT32                    *Mask
  )
{
  EFI_STATUS      Status;
  EFI_STATUS      ReturnStatus;
  CONST CHAR16    *ValueStr;

  Status       = EFI_SUCCESS;
  ReturnStatus = EFI_SUCCESS;

  //
  // Convert user imput from string to integer, and fill in the member in EFI_IPSEC_SPD_SELECTOR.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--local");
  if (ValueStr != NULL) {
    Selector->LocalAddressCount = 1;
    Status = EfiInetAddrRange ((CHAR16 *) ValueStr, Selector->LocalAddress);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
        mHiiHandle,
        mAppName,
        L"--local",
        ValueStr
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    } else {
      *Mask |= LOCAL;
    }
  }

  //
  // Convert user imput from string to integer, and fill in the member in EFI_IPSEC_SPD_SELECTOR.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--remote");
  if (ValueStr != NULL) {
    Selector->RemoteAddressCount = 1;
    Status = EfiInetAddrRange ((CHAR16 *) ValueStr, Selector->RemoteAddress);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
        mHiiHandle,
        mAppName,
        L"--remote",
        ValueStr
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    } else {
      *Mask |= REMOTE;
    }
  }

  Selector->NextLayerProtocol = EFI_IPSEC_ANY_PROTOCOL;

  //
  // Convert user imput from string to integer, and fill in the member in EFI_IPSEC_SPD_SELECTOR.
  //
  Status = GetNumber (
             L"--proto",
             (UINT16) -1,
             &Selector->NextLayerProtocol,
             sizeof (UINT16),
             mMapIpProtocol,
             ParamPackage,
             FORMAT_NUMBER | FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= PROTO;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  Selector->LocalPort  = EFI_IPSEC_ANY_PORT;
  Selector->RemotePort = EFI_IPSEC_ANY_PORT;

  //
  // Convert user imput from string to integer, and fill in the member in EFI_IPSEC_SPD_SELECTOR.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--local-port");
  if (ValueStr != NULL) {
    Status = EfiInetPortRange ((CHAR16 *) ValueStr, &Selector->LocalPort, &Selector->LocalPortRange);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
        mHiiHandle,
        mAppName,
        L"--local-port",
        ValueStr
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    } else {
      *Mask |= LOCAL_PORT;
    }
  }

  //
  // Convert user imput from string to integer, and fill in the member in EFI_IPSEC_SPD_SELECTOR.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--remote-port");
  if (ValueStr != NULL) {
    Status = EfiInetPortRange ((CHAR16 *) ValueStr, &Selector->RemotePort, &Selector->RemotePortRange);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
        mHiiHandle,
        mAppName,
        L"--remote-port",
        ValueStr
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    } else {
      *Mask |= REMOTE_PORT;
    }
  }

  //
  // Convert user imput from string to integer, and fill in the member in EFI_IPSEC_SPD_SELECTOR.
  //
  Status = GetNumber (
             L"--icmp-type",
             (UINT8) -1,
             &Selector->LocalPort,
             sizeof (UINT16),
             NULL,
             ParamPackage,
             FORMAT_NUMBER
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= ICMP_TYPE;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  //
  // Convert user imput from string to integer, and fill in the member in EFI_IPSEC_SPD_SELECTOR.
  //
  Status = GetNumber (
             L"--icmp-code",
             (UINT8) -1,
             &Selector->RemotePort,
             sizeof (UINT16),
             NULL,
             ParamPackage,
             FORMAT_NUMBER
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= ICMP_CODE;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  return ReturnStatus;
}

/**
  Fill in EFI_IPSEC_SPD_SELECTOR and EFI_IPSEC_SPD_DATA through ParamPackage list.

  @param[out] Selector        The pointer to the EFI_IPSEC_SPD_SELECTOR structure.
  @param[out] Data            The pointer to the EFI_IPSEC_SPD_DATA structure.
  @param[in]  ParamPackage    The pointer to the ParamPackage list.
  @param[out] Mask            The pointer to the Mask.
  @param[in]  CreateNew       The switch to create new.

  @retval EFI_SUCCESS              Fill in EFI_IPSEC_SPD_SELECTOR and EFI_IPSEC_SPD_DATA successfully.
  @retval EFI_INVALID_PARAMETER    Invalid user input parameter.

**/
EFI_STATUS
CreateSpdEntry (
  OUT EFI_IPSEC_SPD_SELECTOR    **Selector,
  OUT EFI_IPSEC_SPD_DATA        **Data,
  IN  LIST_ENTRY                *ParamPackage,
  OUT UINT32                    *Mask,
  IN  BOOLEAN                   CreateNew
  )
{
  EFI_STATUS      Status;
  EFI_STATUS      ReturnStatus;
  CONST CHAR16    *ValueStr;
  UINTN           DataSize;

  Status    = EFI_SUCCESS;
  *Mask     = 0;

  *Selector = AllocateZeroPool (sizeof (EFI_IPSEC_SPD_SELECTOR) + 2 * sizeof (EFI_IP_ADDRESS_INFO));
  ASSERT (*Selector != NULL);

  (*Selector)->LocalAddress  = (EFI_IP_ADDRESS_INFO *) (*Selector + 1);
  (*Selector)->RemoteAddress = (*Selector)->LocalAddress + 1;

  ReturnStatus = CreateSpdSelector (*Selector, ParamPackage, Mask);

  //
  // SPD DATA
  // NOTE: Allocate enough memory and add padding for different arch.
  //
  DataSize  = ALIGN_VARIABLE (sizeof (EFI_IPSEC_SPD_DATA));
  DataSize  = ALIGN_VARIABLE (DataSize + sizeof (EFI_IPSEC_PROCESS_POLICY));
  DataSize += sizeof (EFI_IPSEC_TUNNEL_OPTION);

  *Data = AllocateZeroPool (DataSize);
  ASSERT (*Data != NULL);

  (*Data)->ProcessingPolicy               = (EFI_IPSEC_PROCESS_POLICY *) ALIGN_POINTER (
                                                                           (*Data + 1),
                                                                           sizeof (UINTN)
                                                                           );
  (*Data)->ProcessingPolicy->TunnelOption = (EFI_IPSEC_TUNNEL_OPTION *) ALIGN_POINTER (
                                                                          ((*Data)->ProcessingPolicy + 1),
                                                                          sizeof (UINTN)
                                                                          );


  //
  // Convert user imput from string to integer, and fill in the Name in EFI_IPSEC_SPD_DATA.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--name");
  if (ValueStr != NULL) {
    UnicodeStrToAsciiStr (ValueStr, (CHAR8 *) (*Data)->Name);
    *Mask |= NAME;
  }

  //
  // Convert user imput from string to integer, and fill in the PackageFlag in EFI_IPSEC_SPD_DATA.
  //
  Status = GetNumber (
             L"--packet-flag",
             (UINT8) -1,
             &(*Data)->PackageFlag,
             sizeof (UINT32),
             NULL,
             ParamPackage,
             FORMAT_NUMBER
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= PACKET_FLAG;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  //
  // Convert user imput from string to integer, and fill in the Action in EFI_IPSEC_SPD_DATA.
  //
  Status = GetNumber (
             L"--action",
             (UINT8) -1,
             &(*Data)->Action,
             sizeof (UINT32),
             mMapIpSecAction,
             ParamPackage,
             FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= ACTION;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  //
  // Convert user imput from string to integer, and fill in the ExtSeqNum in EFI_IPSEC_SPD_DATA.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"--ext-sequence")) {
    (*Data)->ProcessingPolicy->ExtSeqNum   = TRUE;
    *Mask |= EXT_SEQUENCE;
  } else if (ShellCommandLineGetFlag (ParamPackage, L"--ext-sequence-")) {
    (*Data)->ProcessingPolicy->ExtSeqNum   = FALSE;
    *Mask |= EXT_SEQUENCE;
  }

  //
  // Convert user imput from string to integer, and fill in the SeqOverflow in EFI_IPSEC_SPD_DATA.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"--sequence-overflow")) {
    (*Data)->ProcessingPolicy->SeqOverflow = TRUE;
    *Mask |= SEQUENCE_OVERFLOW;
  } else if (ShellCommandLineGetFlag (ParamPackage, L"--sequence-overflow-")) {
    (*Data)->ProcessingPolicy->SeqOverflow = FALSE;
    *Mask |= SEQUENCE_OVERFLOW;
  }

  //
  // Convert user imput from string to integer, and fill in the FragCheck in EFI_IPSEC_SPD_DATA.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"--fragment-check")) {
    (*Data)->ProcessingPolicy->FragCheck   = TRUE;
    *Mask |= FRAGMENT_CHECK;
  } else if (ShellCommandLineGetFlag (ParamPackage, L"--fragment-check-")) {
    (*Data)->ProcessingPolicy->FragCheck   = FALSE;
    *Mask |= FRAGMENT_CHECK;
  }

  //
  // Convert user imput from string to integer, and fill in the ProcessingPolicy in EFI_IPSEC_SPD_DATA.
  //
  Status = GetNumber (
             L"--lifebyte",
             (UINT64) -1,
             &(*Data)->ProcessingPolicy->SaLifetime.ByteCount,
             sizeof (UINT64),
             NULL,
             ParamPackage,
             FORMAT_NUMBER
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= LIFEBYTE;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  Status = GetNumber (
             L"--lifetime",
             (UINT64) -1,
             &(*Data)->ProcessingPolicy->SaLifetime.HardLifetime,
             sizeof (UINT64),
             NULL,
             ParamPackage,
             FORMAT_NUMBER
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= LIFETIME;
  }
  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  Status = GetNumber (
             L"--lifetime-soft",
             (UINT64) -1,
             &(*Data)->ProcessingPolicy->SaLifetime.SoftLifetime,
             sizeof (UINT64),
             NULL,
             ParamPackage,
             FORMAT_NUMBER
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= LIFETIME_SOFT;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  (*Data)->ProcessingPolicy->Mode = EfiIPsecTransport;
  Status = GetNumber (
             L"--mode",
             0,
             &(*Data)->ProcessingPolicy->Mode,
             sizeof (UINT32),
             mMapIpSecMode,
             ParamPackage,
             FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= MODE;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--tunnel-local");
  if (ValueStr != NULL) {
    Status = EfiInetAddr2 ((CHAR16 *) ValueStr, &(*Data)->ProcessingPolicy->TunnelOption->LocalTunnelAddress);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
        mHiiHandle,
        mAppName,
        L"--tunnel-local",
        ValueStr
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    } else {
      *Mask |= TUNNEL_LOCAL;
    }
  }

  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--tunnel-remote");
  if (ValueStr != NULL) {
    Status = EfiInetAddr2 ((CHAR16 *) ValueStr, &(*Data)->ProcessingPolicy->TunnelOption->RemoteTunnelAddress);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
        mHiiHandle,
        mAppName,
        L"--tunnel-remote",
        ValueStr
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    } else {
      *Mask |= TUNNEL_REMOTE;
    }
  }

  (*Data)->ProcessingPolicy->TunnelOption->DF = EfiIPsecTunnelCopyDf;
  Status = GetNumber (
             L"--dont-fragment",
             0,
             &(*Data)->ProcessingPolicy->TunnelOption->DF,
             sizeof (UINT32),
             mMapDfOption,
             ParamPackage,
             FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= DONT_FRAGMENT;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  (*Data)->ProcessingPolicy->Proto = EfiIPsecESP;
  Status = GetNumber (
             L"--ipsec-proto",
             0,
             &(*Data)->ProcessingPolicy->Proto,
             sizeof (UINT32),
             mMapIpSecProtocol,
             ParamPackage,
             FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= IPSEC_PROTO;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  Status = GetNumber (
             L"--encrypt-algo",
             0,
             &(*Data)->ProcessingPolicy->EncAlgoId,
             sizeof (UINT8),
             mMapEncAlgo,
             ParamPackage,
             FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= ENCRYPT_ALGO;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  Status = GetNumber (
             L"--auth-algo",
             0,
             &(*Data)->ProcessingPolicy->AuthAlgoId,
             sizeof (UINT8),
             mMapAuthAlgo,
             ParamPackage,
             FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= AUTH_ALGO;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  //
  // Cannot check Mode against EfiIPsecTunnel, because user may want to change tunnel_remote only so the Mode is not set.
  //
  if ((*Mask & (TUNNEL_LOCAL | TUNNEL_REMOTE | DONT_FRAGMENT)) == 0) {
    (*Data)->ProcessingPolicy->TunnelOption = NULL;
  }

  if ((*Mask & (EXT_SEQUENCE | SEQUENCE_OVERFLOW | FRAGMENT_CHECK | LIFEBYTE |
                LIFETIME_SOFT | LIFETIME | MODE | TUNNEL_LOCAL | TUNNEL_REMOTE |
                DONT_FRAGMENT | IPSEC_PROTO | AUTH_ALGO | ENCRYPT_ALGO)) == 0) {
    if ((*Data)->Action != EfiIPsecActionProtect) {
      //
      // User may not provide additional parameter for Protect action, so we cannot simply set ProcessingPolicy to NULL.
      //
      (*Data)->ProcessingPolicy = NULL;
    }
  }

  if (CreateNew) {
    if ((*Mask & (LOCAL | REMOTE | PROTO | ACTION)) != (LOCAL | REMOTE | PROTO | ACTION)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_ONE_OF_PARAMETERS),
        mHiiHandle,
        mAppName,
        L"--local --remote --proto --action"
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    } else if (((*Data)->Action == EfiIPsecActionProtect) &&
               ((*Data)->ProcessingPolicy->Mode == EfiIPsecTunnel) &&
               ((*Mask & (TUNNEL_LOCAL | TUNNEL_REMOTE)) != (TUNNEL_LOCAL | TUNNEL_REMOTE))) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_ONE_OF_PARAMETERS),
        mHiiHandle,
        mAppName,
        L"--tunnel-local --tunnel-remote"
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    }
  }

  return ReturnStatus;
}

/**
  Fill in EFI_IPSEC_SA_ID and EFI_IPSEC_SA_DATA2 through ParamPackage list.

  @param[out] SaId            The pointer to the EFI_IPSEC_SA_ID structure.
  @param[out] Data            The pointer to the EFI_IPSEC_SA_DATA2 structure.
  @param[in]  ParamPackage    The pointer to the ParamPackage list.
  @param[out] Mask            The pointer to the Mask.
  @param[in]  CreateNew       The switch to create new.

  @retval EFI_SUCCESS              Fill in EFI_IPSEC_SA_ID and EFI_IPSEC_SA_DATA2 successfully.
  @retval EFI_INVALID_PARAMETER    Invalid user input parameter.

**/
EFI_STATUS
CreateSadEntry (
  OUT EFI_IPSEC_SA_ID      **SaId,
  OUT EFI_IPSEC_SA_DATA2   **Data,
  IN  LIST_ENTRY           *ParamPackage,
  OUT UINT32               *Mask,
  IN  BOOLEAN              CreateNew
  )
{
  EFI_STATUS      Status;
  EFI_STATUS      ReturnStatus;
  UINTN           AuthKeyLength;
  UINTN           EncKeyLength;
  CONST CHAR16    *ValueStr;
  CHAR8           *AsciiStr;
  UINTN           DataSize;

  Status        = EFI_SUCCESS;
  ReturnStatus  = EFI_SUCCESS;
  *Mask         = 0;
  AuthKeyLength = 0;
  EncKeyLength  = 0;

  *SaId = AllocateZeroPool (sizeof (EFI_IPSEC_SA_ID));
  ASSERT (*SaId != NULL);

  //
  // Convert user imput from string to integer, and fill in the Spi in EFI_IPSEC_SA_ID.
  //
  Status = GetNumber (L"--spi", (UINT32) -1, &(*SaId)->Spi, sizeof (UINT32), NULL, ParamPackage, FORMAT_NUMBER);
  if (!EFI_ERROR (Status)) {
    *Mask |= SPI;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  //
  // Convert user imput from string to integer, and fill in the Proto in EFI_IPSEC_SA_ID.
  //
  Status = GetNumber (
             L"--ipsec-proto",
             0,
             &(*SaId)->Proto,
             sizeof (EFI_IPSEC_PROTOCOL_TYPE),
             mMapIpSecProtocol,
             ParamPackage,
             FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= IPSEC_PROTO;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  //
  // Convert user imput from string to integer, and fill in EFI_IPSEC_SA_DATA2.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--auth-key");
  if (ValueStr != NULL) {
    AuthKeyLength = StrLen (ValueStr);
  }

  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--encrypt-key");
  if (ValueStr != NULL) {
    EncKeyLength = StrLen (ValueStr);
  }

  //
  // EFI_IPSEC_SA_DATA2:
  //   +------------
  //   | EFI_IPSEC_SA_DATA2
  //   +-----------------------
  //   | AuthKey
  //   +-------------------------
  //   | EncKey
  //   +-------------------------
  //   | SpdSelector
  //
  // Notes: To make sure the address alignment add padding after each data if needed.
  //
  DataSize  = ALIGN_VARIABLE (sizeof (EFI_IPSEC_SA_DATA2));
  DataSize  = ALIGN_VARIABLE (DataSize + AuthKeyLength);
  DataSize  = ALIGN_VARIABLE (DataSize + EncKeyLength);
  DataSize  = ALIGN_VARIABLE (DataSize + sizeof (EFI_IPSEC_SPD_SELECTOR));
  DataSize  = ALIGN_VARIABLE (DataSize + sizeof (EFI_IP_ADDRESS_INFO));
  DataSize += sizeof (EFI_IP_ADDRESS_INFO);



  *Data = AllocateZeroPool (DataSize);
  ASSERT (*Data != NULL);

  (*Data)->ManualSet                    = TRUE;
  (*Data)->AlgoInfo.EspAlgoInfo.AuthKey = (VOID *) ALIGN_POINTER (((*Data) + 1), sizeof (UINTN));
  (*Data)->AlgoInfo.EspAlgoInfo.EncKey  = (VOID *) ALIGN_POINTER (
                                                     ((UINT8 *) (*Data)->AlgoInfo.EspAlgoInfo.AuthKey + AuthKeyLength),
                                                     sizeof (UINTN)
                                                     );
  (*Data)->SpdSelector                  = (EFI_IPSEC_SPD_SELECTOR *) ALIGN_POINTER (
                                                                       ((UINT8 *) (*Data)->AlgoInfo.EspAlgoInfo.EncKey + EncKeyLength),
                                                                       sizeof (UINTN)
                                                                       );
  (*Data)->SpdSelector->LocalAddress    = (EFI_IP_ADDRESS_INFO *) ALIGN_POINTER (
                                                                    ((UINT8 *) (*Data)->SpdSelector + sizeof (EFI_IPSEC_SPD_SELECTOR)),
                                                                    sizeof (UINTN));
  (*Data)->SpdSelector->RemoteAddress   = (EFI_IP_ADDRESS_INFO *) ALIGN_POINTER (
                                                                    (*Data)->SpdSelector->LocalAddress + 1,
                                                                    sizeof (UINTN)
                                                                    );

  (*Data)->Mode = EfiIPsecTransport;
  Status = GetNumber (
             L"--mode",
             0,
             &(*Data)->Mode,
             sizeof (EFI_IPSEC_MODE),
             mMapIpSecMode,
             ParamPackage,
             FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= MODE;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  //
  // According to RFC 4303-3.3.3. The first packet sent using a given SA
  // will contain a sequence number of 1.
  //
  (*Data)->SNCount = 1;
  Status = GetNumber (
             L"--sequence-number",
             (UINT64) -1,
             &(*Data)->SNCount,
             sizeof (UINT64),
             NULL,
             ParamPackage,
             FORMAT_NUMBER
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= SEQUENCE_NUMBER;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  (*Data)->AntiReplayWindows = 0;
  Status = GetNumber (
             L"--antireplay-window",
             (UINT8) -1,
             &(*Data)->AntiReplayWindows,
             sizeof (UINT8),
             NULL,
             ParamPackage,
             FORMAT_NUMBER
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= SEQUENCE_NUMBER;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  Status = GetNumber (
             L"--encrypt-algo",
             0,
             &(*Data)->AlgoInfo.EspAlgoInfo.EncAlgoId,
             sizeof (UINT8),
             mMapEncAlgo,
             ParamPackage,
             FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= ENCRYPT_ALGO;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--encrypt-key");
  if (ValueStr != NULL ) {
    (*Data)->AlgoInfo.EspAlgoInfo.EncKeyLength = EncKeyLength;
    AsciiStr = AllocateZeroPool (EncKeyLength + 1);
    ASSERT (AsciiStr != NULL);
    UnicodeStrToAsciiStr (ValueStr, AsciiStr);
    CopyMem ((*Data)->AlgoInfo.EspAlgoInfo.EncKey,  AsciiStr, EncKeyLength);
    FreePool (AsciiStr);
    *Mask |= ENCRYPT_KEY;
  } else {
    (*Data)->AlgoInfo.EspAlgoInfo.EncKey = NULL;
  }

  Status = GetNumber (
             L"--auth-algo",
             0,
             &(*Data)->AlgoInfo.EspAlgoInfo.AuthAlgoId,
             sizeof (UINT8),
             mMapAuthAlgo,
             ParamPackage,
             FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= AUTH_ALGO;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--auth-key");
  if (ValueStr != NULL) {
    (*Data)->AlgoInfo.EspAlgoInfo.AuthKeyLength = AuthKeyLength;
    AsciiStr = AllocateZeroPool (AuthKeyLength + 1);
    ASSERT (AsciiStr != NULL);
    UnicodeStrToAsciiStr (ValueStr, AsciiStr);
    CopyMem ((*Data)->AlgoInfo.EspAlgoInfo.AuthKey, AsciiStr, AuthKeyLength);
    FreePool (AsciiStr);
    *Mask |= AUTH_KEY;
  } else {
    (*Data)->AlgoInfo.EspAlgoInfo.AuthKey = NULL;
  }

  Status = GetNumber (
             L"--lifebyte",
             (UINT64) -1,
             &(*Data)->SaLifetime.ByteCount,
             sizeof (UINT64),
             NULL,
             ParamPackage,
             FORMAT_NUMBER
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= LIFEBYTE;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  Status = GetNumber (
             L"--lifetime",
             (UINT64) -1,
             &(*Data)->SaLifetime.HardLifetime,
             sizeof (UINT64),
             NULL,
             ParamPackage,
             FORMAT_NUMBER
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= LIFETIME;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  Status = GetNumber (
             L"--lifetime-soft",
             (UINT64) -1,
             &(*Data)->SaLifetime.SoftLifetime,
             sizeof (UINT64),
             NULL,
             ParamPackage,
             FORMAT_NUMBER
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= LIFETIME_SOFT;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  Status = GetNumber (
             L"--path-mtu",
             (UINT32) -1,
             &(*Data)->PathMTU,
             sizeof (UINT32),
             NULL,
             ParamPackage,
             FORMAT_NUMBER
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= PATH_MTU;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  //
  // Convert user imput from string to integer, and fill in the DestAddress in EFI_IPSEC_SA_ID.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--tunnel-dest");
  if (ValueStr != NULL) {
    Status = EfiInetAddr2 ((CHAR16 *) ValueStr, &(*Data)->TunnelDestinationAddress);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
        mHiiHandle,
        mAppName,
        L"--tunnel-dest",
        ValueStr
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    } else {
      *Mask |= DEST;
    }
  }

  //
  // Convert user input from string to integer, and fill in the DestAddress in EFI_IPSEC_SA_ID.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--tunnel-source");
  if (ValueStr != NULL) {
    Status = EfiInetAddr2 ((CHAR16 *) ValueStr, &(*Data)->TunnelSourceAddress);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
        mHiiHandle,
        mAppName,
        L"--tunnel-source",
        ValueStr
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    } else {
      *Mask |= SOURCE;
    }
  }

  //
  // If it is TunnelMode, then check if the tunnel-source and --tunnel-dest are set
  //
  if ((*Data)->Mode == EfiIPsecTunnel) {
    if ((*Mask & (DEST|SOURCE)) != (DEST|SOURCE)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_ONE_OF_PARAMETERS),
        mHiiHandle,
        mAppName,
        L"--tunnel-source --tunnel-dest"
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    }
  }
  ReturnStatus = CreateSpdSelector ((*Data)->SpdSelector, ParamPackage, Mask);

  if (CreateNew) {
    if ((*Mask & (SPI|IPSEC_PROTO|LOCAL|REMOTE)) != (SPI|IPSEC_PROTO|LOCAL|REMOTE)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_ONE_OF_PARAMETERS),
        mHiiHandle,
        mAppName,
        L"--spi --ipsec-proto --local --remote"
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    } else {
      if ((*SaId)->Proto == EfiIPsecAH) {
        if ((*Mask & AUTH_ALGO) == 0) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_PARAMETER),
            mHiiHandle,
            mAppName,
            L"--auth-algo"
            );
          ReturnStatus = EFI_INVALID_PARAMETER;
        } else if ((*Data)->AlgoInfo.EspAlgoInfo.AuthAlgoId != IPSEC_AALG_NONE && (*Mask & AUTH_KEY) == 0) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_PARAMETER),
            mHiiHandle,
            mAppName,
            L"--auth-key"
            );
          ReturnStatus = EFI_INVALID_PARAMETER;
        }
      } else {
        if ((*Mask & (ENCRYPT_ALGO|AUTH_ALGO)) != (ENCRYPT_ALGO|AUTH_ALGO) ) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_PARAMETER),
            mHiiHandle,
            mAppName,
            L"--encrypt-algo --auth-algo"
            );
          ReturnStatus = EFI_INVALID_PARAMETER;
        } else if ((*Data)->AlgoInfo.EspAlgoInfo.EncAlgoId != IPSEC_EALG_NONE && (*Mask & ENCRYPT_KEY) == 0) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_PARAMETER),
            mHiiHandle,
            mAppName,
            L"--encrypt-key"
            );
          ReturnStatus = EFI_INVALID_PARAMETER;
        } else if ((*Data)->AlgoInfo.EspAlgoInfo.AuthAlgoId != IPSEC_AALG_NONE && (*Mask & AUTH_KEY) == 0) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_PARAMETER),
            mHiiHandle,
            mAppName,
            L"--auth-key"
            );
          ReturnStatus = EFI_INVALID_PARAMETER;
        }
      }
    }
  }

  return ReturnStatus;
}

/**
  Fill in EFI_IPSEC_PAD_ID and EFI_IPSEC_PAD_DATA through ParamPackage list.

  @param[out] PadId           The pointer to the EFI_IPSEC_PAD_ID structure.
  @param[out] Data            The pointer to the EFI_IPSEC_PAD_DATA structure.
  @param[in]  ParamPackage    The pointer to the ParamPackage list.
  @param[out] Mask            The pointer to the Mask.
  @param[in]  CreateNew       The switch to create new.

  @retval EFI_SUCCESS              Fill in EFI_IPSEC_PAD_ID and EFI_IPSEC_PAD_DATA successfully.
  @retval EFI_INVALID_PARAMETER    Invalid user input parameter.

**/
EFI_STATUS
CreatePadEntry (
  OUT EFI_IPSEC_PAD_ID      **PadId,
  OUT EFI_IPSEC_PAD_DATA    **Data,
  IN  LIST_ENTRY            *ParamPackage,
  OUT UINT32                *Mask,
  IN  BOOLEAN               CreateNew
  )
{
  EFI_STATUS         Status;
  EFI_STATUS         ReturnStatus;
  SHELL_FILE_HANDLE  FileHandle;
  UINT64             FileSize;
  UINTN              AuthDataLength;
  UINTN              RevocationDataLength;
  UINTN              DataLength;
  UINTN              Index;
  CONST CHAR16       *ValueStr;
  UINTN              DataSize;

  Status               = EFI_SUCCESS;
  ReturnStatus         = EFI_SUCCESS;
  *Mask                = 0;
  AuthDataLength       = 0;
  RevocationDataLength = 0;

  *PadId = AllocateZeroPool (sizeof (EFI_IPSEC_PAD_ID));
  ASSERT (*PadId != NULL);

  //
  // Convert user imput from string to integer, and fill in EFI_IPSEC_PAD_ID.
  //
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--peer-address");
  if (ValueStr != NULL) {
    (*PadId)->PeerIdValid = FALSE;
    Status = EfiInetAddrRange ((CHAR16 *) ValueStr, &(*PadId)->Id.IpAddress);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
        mHiiHandle,
        mAppName,
        L"--peer-address",
        ValueStr
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    } else {
      *Mask |= PEER_ADDRESS;
    }
  }

  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--peer-id");
  if (ValueStr != NULL) {
    (*PadId)->PeerIdValid = TRUE;
    StrnCpy ((CHAR16 *) (*PadId)->Id.PeerId, ValueStr, ARRAY_SIZE ((*PadId)->Id.PeerId) - 1);
    *Mask |= PEER_ID;
  }

  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--auth-data");
  if (ValueStr != NULL) {
    if (ValueStr[0] == L'@') {
      //
      // Input is a file: --auth-data "@fs1:\My Certificates\tom.dat"
      //
      Status = ShellOpenFileByName (&ValueStr[1], &FileHandle, EFI_FILE_MODE_READ, 0);
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_IPSEC_CONFIG_FILE_OPEN_FAILED),
          mHiiHandle,
          mAppName,
          &ValueStr[1]
          );
        ReturnStatus = EFI_INVALID_PARAMETER;
      } else {
        Status = ShellGetFileSize (FileHandle, &FileSize);
        ShellCloseFile (&FileHandle);
        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_IPSEC_CONFIG_FILE_OPEN_FAILED),
            mHiiHandle,
            mAppName,
            &ValueStr[1]
            );
          ReturnStatus = EFI_INVALID_PARAMETER;
        } else {
          AuthDataLength = (UINTN) FileSize;
        }
      }
    } else {
      AuthDataLength = StrLen (ValueStr);
    }
  }

  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--revocation-data");
  if (ValueStr != NULL) {
    RevocationDataLength = (StrLen (ValueStr) + 1) * sizeof (CHAR16);
  }

  //
  // Allocate Buffer for Data. Add padding after each struct to make sure the alignment
  // in different Arch.
  //
  DataSize  = ALIGN_VARIABLE (sizeof (EFI_IPSEC_PAD_DATA));
  DataSize  = ALIGN_VARIABLE (DataSize + AuthDataLength);
  DataSize += RevocationDataLength;

  *Data = AllocateZeroPool (DataSize);
  ASSERT (*Data != NULL);

  (*Data)->AuthData       = (VOID *) ALIGN_POINTER ((*Data + 1), sizeof (UINTN));
  (*Data)->RevocationData = (VOID *) ALIGN_POINTER (((UINT8 *) (*Data + 1) + AuthDataLength), sizeof (UINTN));
  (*Data)->AuthProtocol   = EfiIPsecAuthProtocolIKEv1;

  //
  // Convert user imput from string to integer, and fill in EFI_IPSEC_PAD_DATA.
  //
  Status = GetNumber (
             L"--auth-proto",
             0,
             &(*Data)->AuthProtocol,
             sizeof (EFI_IPSEC_AUTH_PROTOCOL_TYPE),
             mMapAuthProto,
             ParamPackage,
             FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= AUTH_PROTO;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  Status = GetNumber (
             L"--auth-method",
             0,
             &(*Data)->AuthMethod,
             sizeof (EFI_IPSEC_AUTH_METHOD),
             mMapAuthMethod,
             ParamPackage,
             FORMAT_STRING
             );
  if (!EFI_ERROR (Status)) {
    *Mask |= AUTH_METHOD;
  }

  if (Status == EFI_INVALID_PARAMETER) {
    ReturnStatus = EFI_INVALID_PARAMETER;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"--ike-id")) {
    (*Data)->IkeIdFlag = TRUE;
    *Mask |= IKE_ID;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"--ike-id-")) {
    (*Data)->IkeIdFlag = FALSE;
    *Mask |= IKE_ID;
  }

  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--auth-data");
  if (ValueStr != NULL) {
    if (ValueStr[0] == L'@') {
      //
      // Input is a file: --auth-data "@fs1:\My Certificates\tom.dat"
      //

      Status = ShellOpenFileByName (&ValueStr[1], &FileHandle, EFI_FILE_MODE_READ, 0);
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_IPSEC_CONFIG_FILE_OPEN_FAILED),
          mHiiHandle,
          mAppName,
          &ValueStr[1]
          );
        ReturnStatus = EFI_INVALID_PARAMETER;
        (*Data)->AuthData = NULL;
      } else {
        DataLength = AuthDataLength;
        Status     = ShellReadFile (FileHandle, &DataLength, (*Data)->AuthData);
        ShellCloseFile (&FileHandle);
        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_IPSEC_CONFIG_FILE_OPEN_FAILED),
            mHiiHandle,
            mAppName,
            &ValueStr[1]
            );
          ReturnStatus = EFI_INVALID_PARAMETER;
          (*Data)->AuthData = NULL;
        } else {
          ASSERT (DataLength == AuthDataLength);
          *Mask |= AUTH_DATA;
        }
      }
    } else {
      for (Index = 0; Index < AuthDataLength; Index++) {
        ((CHAR8 *) (*Data)->AuthData)[Index] = (CHAR8) ValueStr[Index];
      }
      (*Data)->AuthDataSize = AuthDataLength;
      *Mask |= AUTH_DATA;
    }
  }

  ValueStr = ShellCommandLineGetValue (ParamPackage, L"--revocation-data");
  if (ValueStr != NULL) {
    CopyMem ((*Data)->RevocationData, ValueStr, RevocationDataLength);
    (*Data)->RevocationDataSize = RevocationDataLength;
    *Mask |= REVOCATION_DATA;
  } else {
    (*Data)->RevocationData = NULL;
  }

  if (CreateNew) {
    if ((*Mask & (PEER_ID | PEER_ADDRESS)) == 0) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_ONE_OF_PARAMETERS),
        mHiiHandle,
        mAppName,
        L"--peer-id --peer-address"
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    } else if ((*Mask & (AUTH_METHOD | AUTH_DATA)) != (AUTH_METHOD | AUTH_DATA)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_ONE_OF_PARAMETERS),
        mHiiHandle,
        mAppName,
        L"--auth-method --auth-data"
        );
      ReturnStatus = EFI_INVALID_PARAMETER;
    }
  }

  return ReturnStatus;
}

CREATE_POLICY_ENTRY mCreatePolicyEntry[] = {
  (CREATE_POLICY_ENTRY) CreateSpdEntry,
  (CREATE_POLICY_ENTRY) CreateSadEntry,
  (CREATE_POLICY_ENTRY) CreatePadEntry
};

/**
  Combine old SPD entry with new SPD entry.

  @param[in, out] OldSelector    The pointer to the EFI_IPSEC_SPD_SELECTOR structure.
  @param[in, out] OldData        The pointer to the EFI_IPSEC_SPD_DATA structure.
  @param[in]      NewSelector    The pointer to the EFI_IPSEC_SPD_SELECTOR structure.
  @param[in]      NewData        The pointer to the EFI_IPSEC_SPD_DATA structure.
  @param[in]      Mask           The pointer to the Mask.
  @param[out]     CreateNew      The switch to create new.

  @retval EFI_SUCCESS              Combined successfully.
  @retval EFI_INVALID_PARAMETER    Invalid user input parameter.

**/
EFI_STATUS
CombineSpdEntry (
  IN OUT EFI_IPSEC_SPD_SELECTOR    *OldSelector,
  IN OUT EFI_IPSEC_SPD_DATA        *OldData,
  IN     EFI_IPSEC_SPD_SELECTOR    *NewSelector,
  IN     EFI_IPSEC_SPD_DATA        *NewData,
  IN     UINT32                    Mask,
     OUT BOOLEAN                   *CreateNew
  )
{

  //
  // Process Selector
  //
  *CreateNew = FALSE;
  if ((Mask & LOCAL) == 0) {
    NewSelector->LocalAddressCount = OldSelector->LocalAddressCount;
    NewSelector->LocalAddress      = OldSelector->LocalAddress;
  } else if ((NewSelector->LocalAddressCount != OldSelector->LocalAddressCount) ||
             (CompareMem (NewSelector->LocalAddress, OldSelector->LocalAddress, NewSelector->LocalAddressCount * sizeof (EFI_IP_ADDRESS_INFO)) != 0)) {
    *CreateNew = TRUE;
  }

  if ((Mask & REMOTE) == 0) {
    NewSelector->RemoteAddressCount = OldSelector->RemoteAddressCount;
    NewSelector->RemoteAddress      = OldSelector->RemoteAddress;
  } else if ((NewSelector->RemoteAddressCount != OldSelector->RemoteAddressCount) ||
             (CompareMem (NewSelector->RemoteAddress, OldSelector->RemoteAddress, NewSelector->RemoteAddressCount * sizeof (EFI_IP_ADDRESS_INFO)) != 0)) {
    *CreateNew = TRUE;
  }

  if ((Mask & PROTO) == 0) {
    NewSelector->NextLayerProtocol = OldSelector->NextLayerProtocol;
  } else if (NewSelector->NextLayerProtocol != OldSelector->NextLayerProtocol) {
    *CreateNew = TRUE;
  }

  switch (NewSelector->NextLayerProtocol) {
    case EFI_IP4_PROTO_TCP:
    case EFI_IP4_PROTO_UDP:
      if ((Mask & LOCAL_PORT) == 0) {
        NewSelector->LocalPort      = OldSelector->LocalPort;
        NewSelector->LocalPortRange = OldSelector->LocalPortRange;
      } else if ((NewSelector->LocalPort != OldSelector->LocalPort) ||
        (NewSelector->LocalPortRange != OldSelector->LocalPortRange)) {
        *CreateNew = TRUE;
      }

      if ((Mask & REMOTE_PORT) == 0) {
        NewSelector->RemotePort      = OldSelector->RemotePort;
        NewSelector->RemotePortRange = OldSelector->RemotePortRange;
      } else if ((NewSelector->RemotePort != OldSelector->RemotePort) ||
        (NewSelector->RemotePortRange != OldSelector->RemotePortRange)) {
        *CreateNew = TRUE;
      }
      break;

    case EFI_IP4_PROTO_ICMP:
      if ((Mask & ICMP_TYPE) == 0) {
        NewSelector->LocalPort = OldSelector->LocalPort;
      } else if (NewSelector->LocalPort != OldSelector->LocalPort) {
        *CreateNew = TRUE;
      }

      if ((Mask & ICMP_CODE) == 0) {
        NewSelector->RemotePort = OldSelector->RemotePort;
      } else if (NewSelector->RemotePort != OldSelector->RemotePort) {
        *CreateNew = TRUE;
      }
      break;
  }
  //
  // Process Data
  //
  if ((Mask & NAME) != 0) {
    AsciiStrCpy ((CHAR8 *) OldData->Name, (CHAR8 *) NewData->Name);
  }

  if ((Mask & PACKET_FLAG) != 0) {
    OldData->PackageFlag = NewData->PackageFlag;
  }

  if ((Mask & ACTION) != 0) {
    OldData->Action = NewData->Action;
  }

  if (OldData->Action != EfiIPsecActionProtect) {
    OldData->ProcessingPolicy = NULL;
  } else {
    //
    // Protect
    //
    if (OldData->ProcessingPolicy == NULL) {
      //
      // Just point to new data if originally NULL.
      //
      OldData->ProcessingPolicy = NewData->ProcessingPolicy;
      if (OldData->ProcessingPolicy->Mode == EfiIPsecTunnel &&
          (Mask & (TUNNEL_LOCAL | TUNNEL_REMOTE)) != (TUNNEL_LOCAL | TUNNEL_REMOTE)
        ) {
        //
        // Change to Protect action and Tunnel mode, but without providing local/remote tunnel address.
        //
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_ONE_OF_PARAMETERS),
          mHiiHandle,
          mAppName,
          L"--tunnel-local --tunnel-remote"
          );
        return EFI_INVALID_PARAMETER;
      }
    } else {
      //
      // Modify some of the data.
      //
      if ((Mask & EXT_SEQUENCE) != 0) {
        OldData->ProcessingPolicy->ExtSeqNum = NewData->ProcessingPolicy->ExtSeqNum;
      }

      if ((Mask & SEQUENCE_OVERFLOW) != 0) {
        OldData->ProcessingPolicy->SeqOverflow = NewData->ProcessingPolicy->SeqOverflow;
      }

      if ((Mask & FRAGMENT_CHECK) != 0) {
        OldData->ProcessingPolicy->FragCheck = NewData->ProcessingPolicy->FragCheck;
      }

      if ((Mask & LIFEBYTE) != 0) {
        OldData->ProcessingPolicy->SaLifetime.ByteCount = NewData->ProcessingPolicy->SaLifetime.ByteCount;
      }

      if ((Mask & LIFETIME_SOFT) != 0) {
        OldData->ProcessingPolicy->SaLifetime.SoftLifetime = NewData->ProcessingPolicy->SaLifetime.SoftLifetime;
      }

      if ((Mask & LIFETIME) != 0) {
        OldData->ProcessingPolicy->SaLifetime.HardLifetime = NewData->ProcessingPolicy->SaLifetime.HardLifetime;
      }

      if ((Mask & MODE) != 0) {
        OldData->ProcessingPolicy->Mode = NewData->ProcessingPolicy->Mode;
      }

      if ((Mask & IPSEC_PROTO) != 0) {
        OldData->ProcessingPolicy->Proto = NewData->ProcessingPolicy->Proto;
      }

      if ((Mask & AUTH_ALGO) != 0) {
        OldData->ProcessingPolicy->AuthAlgoId = NewData->ProcessingPolicy->AuthAlgoId;
      }

      if ((Mask & ENCRYPT_ALGO) != 0) {
        OldData->ProcessingPolicy->EncAlgoId = NewData->ProcessingPolicy->EncAlgoId;
      }

      if (OldData->ProcessingPolicy->Mode != EfiIPsecTunnel) {
        OldData->ProcessingPolicy->TunnelOption = NULL;
      } else {
        if (OldData->ProcessingPolicy->TunnelOption == NULL) {
          //
          // Set from Transport mode to Tunnel mode, should ensure TUNNEL_LOCAL & TUNNEL_REMOTE both exists.
          //
          if ((Mask & (TUNNEL_LOCAL | TUNNEL_REMOTE)) != (TUNNEL_LOCAL | TUNNEL_REMOTE)) {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_ONE_OF_PARAMETERS),
              mHiiHandle,
              mAppName,
              L"--tunnel-local --tunnel-remote"
              );
            return EFI_INVALID_PARAMETER;
          }

          OldData->ProcessingPolicy->TunnelOption = NewData->ProcessingPolicy->TunnelOption;
        } else {
          if ((Mask & TUNNEL_LOCAL) != 0) {
            CopyMem (
              &OldData->ProcessingPolicy->TunnelOption->LocalTunnelAddress,
              &NewData->ProcessingPolicy->TunnelOption->LocalTunnelAddress,
              sizeof (EFI_IP_ADDRESS)
              );
          }

          if ((Mask & TUNNEL_REMOTE) != 0) {
            CopyMem (
              &OldData->ProcessingPolicy->TunnelOption->RemoteTunnelAddress,
              &NewData->ProcessingPolicy->TunnelOption->RemoteTunnelAddress,
              sizeof (EFI_IP_ADDRESS)
              );
          }

          if ((Mask & DONT_FRAGMENT) != 0) {
            OldData->ProcessingPolicy->TunnelOption->DF = NewData->ProcessingPolicy->TunnelOption->DF;
          }
        }
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Combine old SAD entry with new SAD entry.

  @param[in, out] OldSaId      The pointer to the EFI_IPSEC_SA_ID structure.
  @param[in, out] OldData      The pointer to the EFI_IPSEC_SA_DATA2 structure.
  @param[in]      NewSaId      The pointer to the EFI_IPSEC_SA_ID structure.
  @param[in]      NewData      The pointer to the EFI_IPSEC_SA_DATA2 structure.
  @param[in]      Mask         The pointer to the Mask.
  @param[out]     CreateNew    The switch to create new.

  @retval EFI_SUCCESS              Combined successfully.
  @retval EFI_INVALID_PARAMETER    Invalid user input parameter.

**/
EFI_STATUS
CombineSadEntry (
  IN OUT EFI_IPSEC_SA_ID      *OldSaId,
  IN OUT EFI_IPSEC_SA_DATA2   *OldData,
  IN     EFI_IPSEC_SA_ID      *NewSaId,
  IN     EFI_IPSEC_SA_DATA2   *NewData,
  IN     UINT32               Mask,
     OUT BOOLEAN              *CreateNew
  )
{

  *CreateNew = FALSE;

  if ((Mask & SPI) == 0) {
    NewSaId->Spi = OldSaId->Spi;
  } else if (NewSaId->Spi != OldSaId->Spi) {
    *CreateNew = TRUE;
  }

  if ((Mask & IPSEC_PROTO) == 0) {
    NewSaId->Proto = OldSaId->Proto;
  } else if (NewSaId->Proto != OldSaId->Proto) {
    *CreateNew = TRUE;
  }

  if ((Mask & DEST) == 0) {
    CopyMem (&NewData->TunnelDestinationAddress, &OldData->TunnelDestinationAddress, sizeof (EFI_IP_ADDRESS));
  } else if (CompareMem (&NewData->TunnelDestinationAddress, &OldData->TunnelDestinationAddress, sizeof (EFI_IP_ADDRESS)) != 0) {
    *CreateNew = TRUE;
  }

  if ((Mask & SOURCE) == 0) {
    CopyMem (&NewData->TunnelSourceAddress, &OldData->TunnelSourceAddress, sizeof (EFI_IP_ADDRESS));
  } else if (CompareMem (&NewData->TunnelSourceAddress, &OldData->TunnelSourceAddress, sizeof (EFI_IP_ADDRESS)) != 0) {
    *CreateNew = TRUE;
  }
  //
  // Process SA_DATA.
  //
  if ((Mask & MODE) != 0) {
    OldData->Mode = NewData->Mode;
  }

  if ((Mask & SEQUENCE_NUMBER) != 0) {
    OldData->SNCount = NewData->SNCount;
  }

  if ((Mask & ANTIREPLAY_WINDOW) != 0) {
    OldData->AntiReplayWindows = NewData->AntiReplayWindows;
  }

  if ((Mask & AUTH_ALGO) != 0) {
    OldData->AlgoInfo.EspAlgoInfo.AuthAlgoId    = NewData->AlgoInfo.EspAlgoInfo.AuthAlgoId;
  }

  if ((Mask & AUTH_KEY) != 0) {
    OldData->AlgoInfo.EspAlgoInfo.AuthKey       = NewData->AlgoInfo.EspAlgoInfo.AuthKey;
    OldData->AlgoInfo.EspAlgoInfo.AuthKeyLength = NewData->AlgoInfo.EspAlgoInfo.AuthKeyLength;
  }

  if ((Mask & ENCRYPT_ALGO) != 0) {
    OldData->AlgoInfo.EspAlgoInfo.EncAlgoId     = NewData->AlgoInfo.EspAlgoInfo.EncAlgoId;
  }

  if ((Mask & ENCRYPT_KEY) != 0) {
    OldData->AlgoInfo.EspAlgoInfo.EncKey        = NewData->AlgoInfo.EspAlgoInfo.EncKey;
    OldData->AlgoInfo.EspAlgoInfo.EncKeyLength  = NewData->AlgoInfo.EspAlgoInfo.EncKeyLength;
  }

  if (NewSaId->Proto == EfiIPsecAH) {
    if ((Mask & (ENCRYPT_ALGO | ENCRYPT_KEY)) != 0) {
      //
      // Should not provide encrypt_* if AH.
      //
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_UNWANTED_PARAMETER),
        mHiiHandle,
        mAppName,
        L"--encrypt-algo --encrypt-key"
        );
      return EFI_INVALID_PARAMETER;
    }
  }

  if (NewSaId->Proto == EfiIPsecESP && OldSaId->Proto == EfiIPsecAH) {
    //
    // AH -> ESP
    // Should provide encrypt_algo at least.
    //
    if ((Mask & ENCRYPT_ALGO) == 0) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_PARAMETER),
        mHiiHandle,
        mAppName,
        L"--encrypt-algo"
        );
      return EFI_INVALID_PARAMETER;
    }

    //
    // Encrypt_key should be provided if algorithm is not NONE.
    //
    if (NewData->AlgoInfo.EspAlgoInfo.EncAlgoId != IPSEC_EALG_NONE && (Mask & ENCRYPT_KEY) == 0) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_PARAMETER),
        mHiiHandle,
        mAppName,
        L"--encrypt-algo"
        );
      return EFI_INVALID_PARAMETER;
    }
  }

  if ((Mask & LIFEBYTE) != 0) {
    OldData->SaLifetime.ByteCount    = NewData->SaLifetime.ByteCount;
  }

  if ((Mask & LIFETIME_SOFT) != 0) {
    OldData->SaLifetime.SoftLifetime = NewData->SaLifetime.SoftLifetime;
  }

  if ((Mask & LIFETIME) != 0) {
    OldData->SaLifetime.HardLifetime = NewData->SaLifetime.HardLifetime;
  }

  if ((Mask & PATH_MTU) != 0) {
    OldData->PathMTU                 = NewData->PathMTU;
  }
  //
  // Process SpdSelector.
  //
  if (OldData->SpdSelector == NULL) {
    if ((Mask & (LOCAL | REMOTE | PROTO | LOCAL_PORT | REMOTE_PORT | ICMP_TYPE | ICMP_CODE)) != 0) {
      if ((Mask & (LOCAL | REMOTE | PROTO)) != (LOCAL | REMOTE | PROTO)) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_ONE_OF_PARAMETERS),
          mHiiHandle,
          mAppName,
          L"--local --remote --proto"
          );
        return EFI_INVALID_PARAMETER;
      }

      OldData->SpdSelector = NewData->SpdSelector;
    }
  } else {
    if ((Mask & LOCAL) != 0) {
      OldData->SpdSelector->LocalAddressCount  = NewData->SpdSelector->LocalAddressCount;
      OldData->SpdSelector->LocalAddress       = NewData->SpdSelector->LocalAddress;
    }

    if ((Mask & REMOTE) != 0) {
      OldData->SpdSelector->RemoteAddressCount = NewData->SpdSelector->RemoteAddressCount;
      OldData->SpdSelector->RemoteAddress      = NewData->SpdSelector->RemoteAddress;
    }

    if ((Mask & PROTO) != 0) {
      OldData->SpdSelector->NextLayerProtocol  = NewData->SpdSelector->NextLayerProtocol;
    }

    if (OldData->SpdSelector != NULL) {
      switch (OldData->SpdSelector->NextLayerProtocol) {
        case EFI_IP4_PROTO_TCP:
        case EFI_IP4_PROTO_UDP:
          if ((Mask & LOCAL_PORT) != 0) {
            OldData->SpdSelector->LocalPort  = NewData->SpdSelector->LocalPort;
          }

          if ((Mask & REMOTE_PORT) != 0) {
            OldData->SpdSelector->RemotePort = NewData->SpdSelector->RemotePort;
          }
          break;

        case EFI_IP4_PROTO_ICMP:
          if ((Mask & ICMP_TYPE) != 0) {
            OldData->SpdSelector->LocalPort  = (UINT8) NewData->SpdSelector->LocalPort;
          }

          if ((Mask & ICMP_CODE) != 0) {
            OldData->SpdSelector->RemotePort = (UINT8) NewData->SpdSelector->RemotePort;
          }
          break;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Combine old PAD entry with new PAD entry.

  @param[in, out] OldPadId     The pointer to the EFI_IPSEC_PAD_ID structure.
  @param[in, out] OldData      The pointer to the EFI_IPSEC_PAD_DATA structure.
  @param[in]      NewPadId     The pointer to the EFI_IPSEC_PAD_ID structure.
  @param[in]      NewData      The pointer to the EFI_IPSEC_PAD_DATA structure.
  @param[in]      Mask         The pointer to the Mask.
  @param[out]     CreateNew    The switch to create new.

  @retval EFI_SUCCESS              Combined successfully.
  @retval EFI_INVALID_PARAMETER    Invalid user input parameter.

**/
EFI_STATUS
CombinePadEntry (
  IN OUT EFI_IPSEC_PAD_ID      *OldPadId,
  IN OUT EFI_IPSEC_PAD_DATA    *OldData,
  IN     EFI_IPSEC_PAD_ID      *NewPadId,
  IN     EFI_IPSEC_PAD_DATA    *NewData,
  IN     UINT32                Mask,
     OUT BOOLEAN               *CreateNew
  )
{

  *CreateNew = FALSE;

  if ((Mask & (PEER_ID | PEER_ADDRESS)) == 0) {
    CopyMem (NewPadId, OldPadId, sizeof (EFI_IPSEC_PAD_ID));
  } else {
    if ((Mask & PEER_ID) != 0) {
      if (OldPadId->PeerIdValid) {
        if (StrCmp ((CONST CHAR16 *) OldPadId->Id.PeerId, (CONST CHAR16 *) NewPadId->Id.PeerId) != 0) {
          *CreateNew = TRUE;
        }
      } else {
        *CreateNew = TRUE;
      }
    } else {
      //
      // MASK & PEER_ADDRESS
      //
      if (OldPadId->PeerIdValid) {
        *CreateNew = TRUE;
      } else {
        if ((CompareMem (&OldPadId->Id.IpAddress.Address, &NewPadId->Id.IpAddress.Address, sizeof (EFI_IP_ADDRESS)) != 0) ||
            (OldPadId->Id.IpAddress.PrefixLength != NewPadId->Id.IpAddress.PrefixLength)) {
          *CreateNew = TRUE;
        }
      }
    }
  }

  if ((Mask & AUTH_PROTO) != 0) {
    OldData->AuthProtocol = NewData->AuthProtocol;
  }

  if ((Mask & AUTH_METHOD) != 0) {
    OldData->AuthMethod = NewData->AuthMethod;
  }

  if ((Mask & IKE_ID) != 0) {
    OldData->IkeIdFlag = NewData->IkeIdFlag;
  }

  if ((Mask & AUTH_DATA) != 0) {
    OldData->AuthDataSize = NewData->AuthDataSize;
    OldData->AuthData     = NewData->AuthData;
  }

  if ((Mask & REVOCATION_DATA) != 0) {
    OldData->RevocationDataSize = NewData->RevocationDataSize;
    OldData->RevocationData     = NewData->RevocationData;
  }

  return EFI_SUCCESS;
}

COMBINE_POLICY_ENTRY mCombinePolicyEntry[] = {
  (COMBINE_POLICY_ENTRY) CombineSpdEntry,
  (COMBINE_POLICY_ENTRY) CombineSadEntry,
  (COMBINE_POLICY_ENTRY) CombinePadEntry
};

/**
  Edit entry information in the database.

  @param[in] Selector    The pointer to the EFI_IPSEC_CONFIG_SELECTOR structure.
  @param[in] Data        The pointer to the data.
  @param[in] Context     The pointer to the INSERT_POLICY_ENTRY_CONTEXT structure.

  @retval EFI_SUCCESS    Continue the iteration.
  @retval EFI_ABORTED    Abort the iteration.
**/
EFI_STATUS
EditOperatePolicyEntry (
  IN EFI_IPSEC_CONFIG_SELECTOR    *Selector,
  IN VOID                         *Data,
  IN EDIT_POLICY_ENTRY_CONTEXT    *Context
  )
{
  EFI_STATUS    Status;
  BOOLEAN       CreateNew;

  if (mMatchPolicyEntry[Context->DataType] (Selector, Data, &Context->Indexer)) {
    ASSERT (Context->DataType < 3);

    Status = mCombinePolicyEntry[Context->DataType] (
               Selector,
               Data,
               Context->Selector,
               Context->Data,
               Context->Mask,
               &CreateNew
               );
    if (!EFI_ERROR (Status)) {
      if (CreateNew) {
        //
        // Insert new entry before old entry
        //
        Status = mIpSecConfig->SetData (
                                 mIpSecConfig,
                                 Context->DataType,
                                 Context->Selector,
                                 Data,
                                 Selector
                                 );
        ASSERT_EFI_ERROR (Status);
        //
        // Delete old entry
        //
        Status = mIpSecConfig->SetData (
                                 mIpSecConfig,
                                 Context->DataType,
                                 Selector,
                                 NULL,
                                 NULL
                                 );
        ASSERT_EFI_ERROR (Status);
      } else {
        Status = mIpSecConfig->SetData (
                                 mIpSecConfig,
                                 Context->DataType,
                                 Context->Selector,
                                 Data,
                                 NULL
                                 );
      }
    }

    Context->Status = Status;
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Edit entry information in database according to datatype.

  @param[in] DataType        The value of EFI_IPSEC_CONFIG_DATA_TYPE.
  @param[in] ParamPackage    The pointer to the ParamPackage list.

  @retval EFI_SUCCESS             Edit entry information successfully.
  @retval EFI_NOT_FOUND           Can't find the specified entry.
  @retval Others                  Some mistaken case.
**/
EFI_STATUS
EditPolicyEntry (
  IN EFI_IPSEC_CONFIG_DATA_TYPE    DataType,
  IN LIST_ENTRY                    *ParamPackage
  )
{
  EFI_STATUS                   Status;
  EDIT_POLICY_ENTRY_CONTEXT    Context;
  CONST CHAR16                 *ValueStr;

  ValueStr = ShellCommandLineGetValue (ParamPackage, L"-e");
  if (ValueStr == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_INDEX_NOT_SPECIFIED), mHiiHandle, mAppName, ValueStr);
    return EFI_NOT_FOUND;
  }

  Status = mConstructPolicyEntryIndexer[DataType] (&Context.Indexer, ParamPackage);
  if (!EFI_ERROR (Status)) {
    Context.DataType = DataType;
    Context.Status   = EFI_NOT_FOUND;
    Status = mCreatePolicyEntry[DataType] (&Context.Selector, &Context.Data, ParamPackage, &Context.Mask, FALSE);
    if (!EFI_ERROR (Status)) {
      ForeachPolicyEntry (DataType, (VISIT_POLICY_ENTRY) EditOperatePolicyEntry, &Context);
      Status = Context.Status;
    }

    if (Context.Selector != NULL) {
      gBS->FreePool (Context.Selector);
    }

    if (Context.Data != NULL) {
      gBS->FreePool (Context.Data);
    }
  }

  if (Status == EFI_NOT_FOUND) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_INDEX_NOT_FOUND), mHiiHandle, mAppName, ValueStr);
  } else if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_EDIT_FAILED), mHiiHandle, mAppName);
  }

  return Status;

}

/**
  Insert entry information in database.

  @param[in] Selector    The pointer to the EFI_IPSEC_CONFIG_SELECTOR structure.
  @param[in] Data        The pointer to the data.
  @param[in] Context     The pointer to the INSERT_POLICY_ENTRY_CONTEXT structure.

  @retval EFI_SUCCESS    Continue the iteration.
  @retval EFI_ABORTED    Abort the iteration.
**/
EFI_STATUS
InsertPolicyEntry (
  IN EFI_IPSEC_CONFIG_SELECTOR      *Selector,
  IN VOID                           *Data,
  IN INSERT_POLICY_ENTRY_CONTEXT    *Context
  )
{
  //
  // Found the entry which we want to insert before.
  //
  if (mMatchPolicyEntry[Context->DataType] (Selector, Data, &Context->Indexer)) {

    Context->Status = mIpSecConfig->SetData (
                                      mIpSecConfig,
                                      Context->DataType,
                                      Context->Selector,
                                      Context->Data,
                                      Selector
                                      );
    //
    // Abort the iteration after the insertion.
    //
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Insert or add entry information in database according to datatype.

  @param[in] DataType        The value of EFI_IPSEC_CONFIG_DATA_TYPE.
  @param[in] ParamPackage    The pointer to the ParamPackage list.

  @retval EFI_SUCCESS             Insert or add entry information successfully.
  @retval EFI_NOT_FOUND           Can't find the specified entry.
  @retval EFI_BUFFER_TOO_SMALL    The entry already existed.
  @retval EFI_UNSUPPORTED         The operation is not supported.
  @retval Others                  Some mistaken case.
**/
EFI_STATUS
AddOrInsertPolicyEntry (
  IN EFI_IPSEC_CONFIG_DATA_TYPE    DataType,
  IN LIST_ENTRY                    *ParamPackage
  )
{
  EFI_STATUS                     Status;
  EFI_IPSEC_CONFIG_SELECTOR      *Selector;
  VOID                           *Data;
  INSERT_POLICY_ENTRY_CONTEXT    Context;
  UINT32                         Mask;
  UINTN                          DataSize;
  CONST CHAR16                   *ValueStr;

  Status = mCreatePolicyEntry[DataType] (&Selector, &Data, ParamPackage, &Mask, TRUE);
  if (!EFI_ERROR (Status)) {
    //
    // Find if the Selector to be inserted already exists.
    //
    DataSize = 0;
    Status = mIpSecConfig->GetData (
                             mIpSecConfig,
                             DataType,
                             Selector,
                             &DataSize,
                             NULL
                             );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_ALREADY_EXISTS), mHiiHandle, mAppName);
    } else if (ShellCommandLineGetFlag (ParamPackage, L"-a")) {
      Status = mIpSecConfig->SetData (
                               mIpSecConfig,
                               DataType,
                               Selector,
                               Data,
                               NULL
                               );
    } else {
      ValueStr = ShellCommandLineGetValue (ParamPackage, L"-i");
      if (ValueStr == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_INDEX_NOT_SPECIFIED), mHiiHandle, mAppName, ValueStr);
        return EFI_NOT_FOUND;
      }

      Status = mConstructPolicyEntryIndexer[DataType] (&Context.Indexer, ParamPackage);
      if (!EFI_ERROR (Status)) {
        Context.DataType  = DataType;
        Context.Status    = EFI_NOT_FOUND;
        Context.Selector  = Selector;
        Context.Data      = Data;

        ForeachPolicyEntry (DataType, (VISIT_POLICY_ENTRY) InsertPolicyEntry, &Context);
        Status = Context.Status;
        if (Status == EFI_NOT_FOUND) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_INDEX_NOT_FOUND), mHiiHandle, mAppName, ValueStr);
        }
      }
    }

    gBS->FreePool (Selector);
    gBS->FreePool (Data);
  }

  if (Status == EFI_UNSUPPORTED) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_INSERT_UNSUPPORT), mHiiHandle, mAppName);
  } else if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_INSERT_FAILED), mHiiHandle, mAppName);
  }

  return Status;
}
