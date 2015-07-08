/** @file
  The implementation of construct ENTRY_INDEXER in IpSecConfig application.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecConfig.h"
#include "Indexer.h"
#include "Helper.h"

/**
  Fill in SPD_ENTRY_INDEXER through ParamPackage list.

  @param[in, out] Indexer         The pointer to the SPD_ENTRY_INDEXER structure.
  @param[in]      ParamPackage    The pointer to the ParamPackage list.

  @retval EFI_SUCCESS    Filled in SPD_ENTRY_INDEXER successfully.
**/
EFI_STATUS
ConstructSpdIndexer (
  IN OUT SPD_ENTRY_INDEXER    *Indexer,
  IN     LIST_ENTRY           *ParamPackage
  )
{
  EFI_STATUS      Status;
  UINT64          Value64;
  CONST CHAR16    *ValueStr;

  ValueStr = NULL;

  if (ShellCommandLineGetFlag (ParamPackage, L"-i")) {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-i");
  } else if (ShellCommandLineGetFlag (ParamPackage, L"-d")) {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-d");
  } else if (ShellCommandLineGetFlag (ParamPackage, L"-e")) {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-e");
  } else {
    ASSERT (FALSE);
  }

  ASSERT (ValueStr != NULL);

  Value64 = StrToUInteger (ValueStr, &Status);
  if (!EFI_ERROR (Status)) {
    Indexer->Index = (UINTN) Value64;
    Indexer->Name  = NULL;
  } else {
    UnicodeStrToAsciiStr (ValueStr, (CHAR8 *) Indexer->Name);
  }

  return EFI_SUCCESS;
}

/**
  Fill in SAD_ENTRY_INDEXER through ParamPackage list.

  @param[in, out] Indexer         The pointer to the SAD_ENTRY_INDEXER structure.
  @param[in]      ParamPackage    The pointer to the ParamPackage list.

  @retval EFI_SUCCESS              Filled in SPD_ENTRY_INDEXER successfully.
  @retval EFI_INVALID_PARAMETER    The mistaken user input in ParamPackage list.
**/
EFI_STATUS
ConstructSadIndexer (
  IN OUT SAD_ENTRY_INDEXER    *Indexer,
  IN     LIST_ENTRY           *ParamPackage
  )
{
  EFI_STATUS      Status;
  EFI_STATUS      Status1;
  UINT64          Value64;
  CONST CHAR16    *ValueStr;

  ValueStr = NULL;

  if (ShellCommandLineGetFlag (ParamPackage, L"-i")) {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-i");
  } else if (ShellCommandLineGetFlag (ParamPackage, L"-d")) {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-d");
  } else if (ShellCommandLineGetFlag (ParamPackage, L"-e")) {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-e");
  } else {
    ASSERT (FALSE);
  }

  ASSERT (ValueStr != NULL);

  Value64 = StrToUInteger (ValueStr, &Status);
  if (!EFI_ERROR (Status)) {
    Indexer->Index = (UINTN) Value64;
    ZeroMem (&Indexer->SaId, sizeof (EFI_IPSEC_SA_ID));
  } else {
    if ((!ShellCommandLineGetFlag (ParamPackage, L"--lookup-spi")) ||
        (!ShellCommandLineGetFlag (ParamPackage, L"--lookup-ipsec-proto")) ||
        (!ShellCommandLineGetFlag (ParamPackage, L"--lookup-dest"))) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_ONE_OF_PARAMETERS),
        mHiiHandle,
        mAppName,
        L"--lookup-spi --lookup-ipsec-proto --lookup-dest"
        );
      return EFI_INVALID_PARAMETER;
    }

    Status = GetNumber (
              L"--lookup-spi",
              (UINT32) -1,
              &Indexer->SaId.Spi,
              sizeof (UINT32),
              NULL,
              ParamPackage,
              FORMAT_NUMBER
              );
    Status1 = GetNumber (
                L"--lookup-ipsec-proto",
                0,
                &Indexer->SaId.Proto,
                sizeof (EFI_IPSEC_PROTOCOL_TYPE),
                mMapIpSecProtocol,
                ParamPackage,
                FORMAT_STRING
                );

    if (EFI_ERROR (Status) || EFI_ERROR (Status1)) {
      return EFI_INVALID_PARAMETER;
    }

    ValueStr = ShellCommandLineGetValue (ParamPackage, L"--lookup-dest");
    ASSERT (ValueStr != NULL);

    Status = EfiInetAddr2 ((CHAR16 *) ValueStr, &Indexer->SaId.DestAddress);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
        mHiiHandle,
        mAppName,
        L"--lookup-dest",
        ValueStr
        );
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/**
  Fill in PAD_ENTRY_INDEXER through ParamPackage list.

  @param[in, out] Indexer         The pointer to the PAD_ENTRY_INDEXER structure.
  @param[in]      ParamPackage    The pointer to the ParamPackage list.

  @retval EFI_SUCCESS              Filled in PAD_ENTRY_INDEXER successfully.
  @retval EFI_INVALID_PARAMETER    The mistaken user input in ParamPackage list.
**/
EFI_STATUS
ConstructPadIndexer (
  IN OUT PAD_ENTRY_INDEXER    *Indexer,
  IN     LIST_ENTRY           *ParamPackage
  )
{
  EFI_STATUS      Status;
  UINT64          Value64;
  CONST CHAR16    *ValueStr;

  ValueStr = NULL;

  if (ShellCommandLineGetFlag (ParamPackage, L"-i")) {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-i");
  } else if (ShellCommandLineGetFlag (ParamPackage, L"-d")) {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-d");
  } else if (ShellCommandLineGetFlag (ParamPackage, L"-e")) {
    ValueStr = ShellCommandLineGetValue (ParamPackage, L"-e");
  } else {
    ASSERT (FALSE);
  }

  ASSERT (ValueStr != NULL);

  Value64 = StrToUInteger (ValueStr, &Status);

  if (!EFI_ERROR (Status)) {
    Indexer->Index = (UINTN) Value64;
    ZeroMem (&Indexer->PadId, sizeof (EFI_IPSEC_PAD_ID));
  } else {

    if (ShellCommandLineGetFlag (ParamPackage, L"--lookup-peer-address")) {
      ValueStr = ShellCommandLineGetValue (ParamPackage, L"--lookup-peer-address");
      ASSERT (ValueStr != NULL);

      Indexer->PadId.PeerIdValid = FALSE;
      Status = EfiInetAddrRange ((CHAR16 *) ValueStr, &Indexer->PadId.Id.IpAddress);
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
          mHiiHandle,
          mAppName,
          L"--lookup-peer-address",
          ValueStr
          );
        return EFI_INVALID_PARAMETER;
      }
    } else {
      ValueStr = ShellCommandLineGetValue (ParamPackage, L"--lookup-peer-id");
      if (ValueStr == NULL) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_ONE_OF_PARAMETERS),
          mHiiHandle,
          mAppName,
          L"--lookup-peer-address --lookup-peer-id"
          );
        return EFI_INVALID_PARAMETER;
      }

      Indexer->PadId.PeerIdValid = TRUE;
      ZeroMem (Indexer->PadId.Id.PeerId, MAX_PEERID_LEN);
      StrnCpyS ((CHAR16 *) Indexer->PadId.Id.PeerId, MAX_PEERID_LEN / sizeof (CHAR16), ValueStr, MAX_PEERID_LEN / sizeof (CHAR16) - 1);
    }
  }

  return EFI_SUCCESS;
}

CONSTRUCT_POLICY_ENTRY_INDEXER mConstructPolicyEntryIndexer[] = {
  (CONSTRUCT_POLICY_ENTRY_INDEXER) ConstructSpdIndexer,
  (CONSTRUCT_POLICY_ENTRY_INDEXER) ConstructSadIndexer,
  (CONSTRUCT_POLICY_ENTRY_INDEXER) ConstructPadIndexer
};
