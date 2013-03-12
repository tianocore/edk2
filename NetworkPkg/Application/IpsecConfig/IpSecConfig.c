/** @file
  The main process for IpSecConfig application.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HiiLib.h>

#include <Protocol/IpSec.h>

#include "IpSecConfig.h"
#include "Dump.h"
#include "Indexer.h"
#include "PolicyEntryOperation.h"
#include "Delete.h"
#include "Helper.h"

//
// Used for ShellCommandLineParseEx only
// and to ensure user inputs are in valid format
//
SHELL_PARAM_ITEM    mIpSecConfigParamList[] = {
  { L"-p",                    TypeValue },
  { L"-a",                    TypeValue },
  { L"-i",                    TypeValue },
  { L"-e",                    TypeValue },
  { L"-d",                    TypeValue },
  { L"-f",                    TypeFlag },
  { L"-l",                    TypeFlag },
  { L"-enable",               TypeFlag },
  { L"-disable",              TypeFlag },
  { L"-status",               TypeFlag },
  { L"-?",                    TypeFlag },

  //
  // SPD Selector
  //
  { L"--local",               TypeValue },
  { L"--remote",              TypeValue },
  { L"--proto",               TypeValue },
  { L"--local-port",          TypeValue },
  { L"--remote-port",         TypeValue },
  { L"--icmp-type",           TypeValue },
  { L"--icmp-code",           TypeValue },

  //
  // SPD Data
  //
  { L"--name",                TypeValue },
  { L"--packet-flag",         TypeValue },
  { L"--action",              TypeValue },
  { L"--lifebyte",            TypeValue },
  { L"--lifetime-soft",       TypeValue },
  { L"--lifetime",            TypeValue },
  { L"--mode",                TypeValue },
  { L"--tunnel-local",        TypeValue },
  { L"--tunnel-remote",       TypeValue },
  { L"--dont-fragment",       TypeValue },
  { L"--ipsec-proto",         TypeValue },
  { L"--auth-algo",           TypeValue },
  { L"--encrypt-algo",        TypeValue },

  { L"--ext-sequence",        TypeFlag  },
  { L"--sequence-overflow",   TypeFlag  },
  { L"--fragment-check",      TypeFlag  },
  { L"--ext-sequence-",       TypeFlag  },
  { L"--sequence-overflow-",  TypeFlag  },
  { L"--fragment-check-",     TypeFlag  },

  //
  // SA ID
  // --ipsec-proto
  //
  { L"--spi",                 TypeValue },
  { L"--tunnel-dest",         TypeValue },
  { L"--tunnel-source",       TypeValue },
  { L"--lookup-spi",          TypeValue },
  { L"--lookup-ipsec-proto",  TypeValue },
  { L"--lookup-dest",         TypeValue },

  //
  // SA DATA
  // --mode
  // --auth-algo
  // --encrypt-algo
  //
  { L"--sequence-number",     TypeValue },
  { L"--antireplay-window",   TypeValue },
  { L"--auth-key",            TypeValue },
  { L"--encrypt-key",         TypeValue },
  { L"--path-mtu",            TypeValue },

  //
  // PAD ID
  //
  { L"--peer-id",             TypeValue },
  { L"--peer-address",        TypeValue },
  { L"--auth-proto",          TypeValue },
  { L"--auth-method",         TypeValue },
  { L"--ike-id",              TypeValue },
  { L"--ike-id-",             TypeValue },
  { L"--auth-data",           TypeValue },
  { L"--revocation-data",     TypeValue },
  { L"--lookup-peer-id",      TypeValue },
  { L"--lookup-peer-address", TypeValue },

  { NULL,                     TypeMax   },
};

//
// -P
//
STR2INT mMapPolicy[] = {
  { L"SPD",       IPsecConfigDataTypeSpd },
  { L"SAD",       IPsecConfigDataTypeSad },
  { L"PAD",       IPsecConfigDataTypePad },
  { NULL,         0 },
};

//
// --proto
//
STR2INT mMapIpProtocol[] = {
  { L"TCP",       EFI_IP4_PROTO_TCP },
  { L"UDP",       EFI_IP4_PROTO_UDP },
  { L"ICMP",      EFI_IP4_PROTO_ICMP },
  { NULL,         0 },
};

//
// --action
//
STR2INT mMapIpSecAction[] = {
  { L"Bypass",    EfiIPsecActionBypass },
  { L"Discard",   EfiIPsecActionDiscard },
  { L"Protect",   EfiIPsecActionProtect },
  { NULL,         0 },
};

//
// --mode
//
STR2INT mMapIpSecMode[] = {
  { L"Transport", EfiIPsecTransport },
  { L"Tunnel",    EfiIPsecTunnel },
  { NULL,         0 },
};

//
// --dont-fragment
//
STR2INT mMapDfOption[] = {
  { L"clear",     EfiIPsecTunnelClearDf },
  { L"set",       EfiIPsecTunnelSetDf },
  { L"copy",      EfiIPsecTunnelCopyDf },
  { NULL,         0 },
};

//
// --ipsec-proto
//
STR2INT mMapIpSecProtocol[] = {
  { L"AH",        EfiIPsecAH },
  { L"ESP",       EfiIPsecESP },
  { NULL,         0 },
};

//
// --auth-algo
//
STR2INT mMapAuthAlgo[] = {
  { L"NONE",         IPSEC_AALG_NONE },
  { L"MD5HMAC",      IPSEC_AALG_MD5HMAC },
  { L"SHA1HMAC",     IPSEC_AALG_SHA1HMAC },
  { L"SHA2-256HMAC", IPSEC_AALG_SHA2_256HMAC },
  { L"SHA2-384HMAC", IPSEC_AALG_SHA2_384HMAC },
  { L"SHA2-512HMAC", IPSEC_AALG_SHA2_512HMAC },
  { L"AES-XCBC-MAC", IPSEC_AALG_AES_XCBC_MAC },
  { L"NULL",         IPSEC_AALG_NULL },
  { NULL,            0 },
};

//
// --encrypt-algo
//
STR2INT mMapEncAlgo[] = {
  { L"NONE",         IPSEC_EALG_NONE },
  { L"DESCBC",       IPSEC_EALG_DESCBC },
  { L"3DESCBC",      IPSEC_EALG_3DESCBC },
  { L"CASTCBC",      IPSEC_EALG_CASTCBC },
  { L"BLOWFISHCBC",  IPSEC_EALG_BLOWFISHCBC },
  { L"NULL",         IPSEC_EALG_NULL },
  { L"AESCBC",       IPSEC_EALG_AESCBC },
  { L"AESCTR",       IPSEC_EALG_AESCTR },
  { L"AES-CCM-ICV8", IPSEC_EALG_AES_CCM_ICV8 },
  { L"AES-CCM-ICV12",IPSEC_EALG_AES_CCM_ICV12 },
  { L"AES-CCM-ICV16",IPSEC_EALG_AES_CCM_ICV16 },
  { L"AES-GCM-ICV8", IPSEC_EALG_AES_GCM_ICV8 },
  { L"AES-GCM-ICV12",IPSEC_EALG_AES_GCM_ICV12 },
  { L"AES-GCM-ICV16",IPSEC_EALG_AES_GCM_ICV16 },
  { NULL,            0 },
};

//
// --auth-proto
//
STR2INT mMapAuthProto[] = {
  { L"IKEv1",        EfiIPsecAuthProtocolIKEv1 },
  { L"IKEv2",        EfiIPsecAuthProtocolIKEv2 },
  { NULL,            0 },
};

//
// --auth-method
//
STR2INT mMapAuthMethod[] = {
  { L"PreSharedSecret", EfiIPsecAuthMethodPreSharedSecret },
  { L"Certificates",    EfiIPsecAuthMethodCertificates },
  { NULL,               0 },
};

EFI_IPSEC2_PROTOCOL          *mIpSec;
EFI_IPSEC_CONFIG_PROTOCOL    *mIpSecConfig;
EFI_HII_HANDLE               mHiiHandle;
CHAR16                       mAppName[]          = L"IpSecConfig";

//
// Used for IpSecConfigRetriveCheckListByName only to check the validation of user input
//
VAR_CHECK_ITEM    mIpSecConfigVarCheckList[] = {
  { L"-enable",              BIT(1)|BIT(0),  BIT(1),  BIT(2)|BIT(1)|BIT(0), 0 },
  { L"-disable",             BIT(1)|BIT(0),  BIT(1),  BIT(2)|BIT(1)|BIT(0), 0 },
  { L"-status",              BIT(1)|BIT(0),  BIT(1),  BIT(2)|BIT(1)|BIT(0), 0 },
  { L"-p",                   BIT(1),         0,       BIT(2)|BIT(1)|BIT(0), 0 },

  { L"-a",                   BIT(0),         0,       BIT(2)|BIT(1)|BIT(0), 0 },
  { L"-i",                   BIT(0),         0,       BIT(2)|BIT(1)|BIT(0), 0 },
  { L"-d",                   BIT(0),         0,       BIT(2)|BIT(1)|BIT(0), 0 },
  { L"-e",                   BIT(0),         0,       BIT(2)|BIT(1)|BIT(0), 0 },
  { L"-l",                   BIT(0),         0,       BIT(2)|BIT(1)|BIT(0), 0 },
  { L"-f",                   BIT(0),         0,       BIT(2)|BIT(1)|BIT(0), 0 },

  { L"-?",                   BIT(0),         BIT(0),  BIT(2)|BIT(1)|BIT(0), 0 },

  //
  // SPD Selector
  //
  { L"--local",              0,              0,       BIT(2)|BIT(1),        0 },
  { L"--remote",             0,              0,       BIT(2)|BIT(1),        0 },
  { L"--proto",              0,              0,       BIT(2)|BIT(1),        0 },
  { L"--local-port",         0,              0,       BIT(2)|BIT(1),        BIT(0) },
  { L"--remote-port",        0,              0,       BIT(2)|BIT(1),        BIT(0) },
  { L"--icmp-type",          0,              0,       BIT(2)|BIT(1),        BIT(1) },
  { L"--icmp-code",          0,              0,       BIT(2)|BIT(1),        BIT(1) },

  //
  // SPD Data
  //
  { L"--name",               0,              0,       BIT(2),               0 },
  { L"--packet-flag",        0,              0,       BIT(2),               0 },
  { L"--action",             0,              0,       BIT(2)|BIT(1),        0 },
  { L"--lifebyte",           0,              0,       BIT(2)|BIT(1),        0 },
  { L"--lifetime-soft",      0,              0,       BIT(2)|BIT(1),        0 },
  { L"--lifetime",           0,              0,       BIT(2)|BIT(1),        0 },
  { L"--mode",               0,              0,       BIT(2)|BIT(1),        0 },
  { L"--tunnel-local",       0,              0,       BIT(2),               0 },
  { L"--tunnel-remote",      0,              0,       BIT(2),               0 },
  { L"--dont-fragment",      0,              0,       BIT(2),               0 },
  { L"--ipsec-proto",        0,              0,       BIT(2)|BIT(1),        0 },
  { L"--auth-algo",          0,              0,       BIT(2)|BIT(1),        0 },
  { L"--encrypt-algo",       0,              0,       BIT(2)|BIT(1),        0 },

  { L"--ext-sequence",       0,              0,       BIT(2),               BIT(2) },
  { L"--sequence-overflow",  0,              0,       BIT(2),               BIT(2) },
  { L"--fragment-check",     0,              0,       BIT(2),               BIT(2) },
  { L"--ext-sequence-",      0,              0,       BIT(2),               BIT(3) },
  { L"--sequence-overflow-", 0,              0,       BIT(2),               BIT(3) },
  { L"--fragment-check-",    0,              0,       BIT(2),               BIT(3) },

  //
  // SA ID
  // --ipsec-proto
  //
  { L"--spi",                0,              0,       BIT(1),               0 },
  { L"--tunnel-dest",        0,              0,       BIT(1),               0 },
  { L"--tunnel-source",      0,              0,       BIT(1),               0 },
  { L"--lookup-spi",         0,              0,       BIT(1),               0 },
  { L"--lookup-ipsec-proto", 0,              0,       BIT(1),               0 },
  { L"--lookup-dest",        0,              0,       BIT(1),               0 },

  //
  // SA DATA
  // --mode
  // --auth-algo
  // --encrypt-algo
  //
  { L"--sequence-number",    0,              0,       BIT(1),               0 },
  { L"--antireplay-window",  0,              0,       BIT(1),               0 },
  { L"--auth-key",           0,              0,       BIT(1),               0 },
  { L"--encrypt-key",        0,              0,       BIT(1),               0 },
  { L"--path-mtu",           0,              0,       BIT(1),               0 },

  //
  // The example to add a PAD:
  // "-A --peer-id Mike [--peer-address 10.23.2.2] --auth-proto IKE1/IKE2
  //     --auth-method PreSharedSeceret/Certificate --ike-id
  //     --auth-data 343343 --revocation-data 2342432"
  // The example to delete a PAD:
  // "-D * --lookup-peer-id Mike [--lookup-peer-address 10.23.2.2]"
  // "-D 1"
  // The example to edit a PAD:
  // "-E * --lookup-peer-id Mike --auth-method Certificate"

  //
  // PAD ID
  //
  { L"--peer-id",            0,              0,       BIT(0),               BIT(4) },
  { L"--peer-address",       0,              0,       BIT(0),               BIT(5) },
  { L"--auth-proto",         0,              0,       BIT(0),               0 },
  { L"--auth-method",        0,              0,       BIT(0),               0 },
  { L"--IKE-ID",             0,              0,       BIT(0),               BIT(6) },
  { L"--IKE-ID-",            0,              0,       BIT(0),               BIT(7) },
  { L"--auth-data",          0,              0,       BIT(0),               0 },
  { L"--revocation-data",    0,              0,       BIT(0),               0 },
  { L"--lookup-peer-id",     0,              0,       BIT(0),               BIT(4) },
  { L"--lookup-peer-address",0,              0,       BIT(0),               BIT(5) },

  { NULL,                    0,              0,       0,                    0 },
};

/**
  The function to allocate the proper sized buffer for various
  EFI interfaces.

  @param[in, out] Status        Current status.
  @param[in, out] Buffer        Current allocated buffer, or NULL.
  @param[in]      BufferSize    Current buffer size needed

  @retval TRUE     If the buffer was reallocated and the caller should try the API again.
  @retval FALSE    If the buffer was not reallocated successfully.
**/
BOOLEAN
GrowBuffer (
  IN OUT EFI_STATUS    *Status,
  IN OUT VOID          **Buffer,
  IN     UINTN         BufferSize
  )
{
  BOOLEAN    TryAgain;

  ASSERT (Status != NULL);
  ASSERT (Buffer != NULL);

  //
  // If this is an initial request, buffer will be null with a new buffer size.
  //
  if ((NULL == *Buffer) && (BufferSize != 0)) {
    *Status = EFI_BUFFER_TOO_SMALL;
  }

  //
  // If the status code is "buffer too small", resize the buffer.
  //
  TryAgain = FALSE;
  if (*Status == EFI_BUFFER_TOO_SMALL) {

    if (*Buffer != NULL) {
      FreePool (*Buffer);
    }

    *Buffer = AllocateZeroPool (BufferSize);

    if (*Buffer != NULL) {
      TryAgain = TRUE;
    } else {
      *Status = EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // If there's an error, free the buffer.
  //
  if (!TryAgain && EFI_ERROR (*Status) && (*Buffer != NULL)) {
    FreePool (*Buffer);
    *Buffer = NULL;
  }

  return TryAgain;
}

/**
  Function returns an array of handles that support the requested protocol
  in a buffer allocated from a pool.

  @param[in]      SearchType    Specifies which handle(s) are to be returned.
  @param[in]      Protocol      Provides the protocol to search by.
                                This parameter is only valid for SearchType ByProtocol.

  @param[in]      SearchKey     Supplies the search key depending on the SearchType.
  @param[in, out] NoHandles     The number of handles returned in Buffer.
  @param[out]     Buffer        A pointer to the buffer to return the requested array of
                                handles that support Protocol.

  @retval EFI_SUCCESS    The resulting array of handles was returned.
  @retval Others         Other mistake case.
**/
EFI_STATUS
LocateHandle (
  IN     EFI_LOCATE_SEARCH_TYPE    SearchType,
  IN     EFI_GUID                  *Protocol  OPTIONAL,
  IN     VOID                      *SearchKey OPTIONAL,
  IN OUT UINTN                     *NoHandles,
     OUT EFI_HANDLE                **Buffer
  )
{
  EFI_STATUS    Status;
  UINTN         BufferSize;

  ASSERT (NoHandles != NULL);
  ASSERT (Buffer != NULL);

  //
  // Initialize for GrowBuffer loop.
  //
  Status      = EFI_SUCCESS;
  *Buffer     = NULL;
  BufferSize  = 50 * sizeof (EFI_HANDLE);

  //
  // Call the real function.
  //
  while (GrowBuffer (&Status, (VOID **) Buffer, BufferSize)) {
    Status = gBS->LocateHandle (
                    SearchType,
                    Protocol,
                    SearchKey,
                    &BufferSize,
                    *Buffer
                    );
  }

  *NoHandles = BufferSize / sizeof (EFI_HANDLE);
  if (EFI_ERROR (Status)) {
    *NoHandles = 0;
  }

  return Status;
}

/**
  Find the first instance of this protocol in the system and return its interface.

  @param[in]  ProtocolGuid    The guid of the protocol.
  @param[out] Interface       The pointer to the first instance of the protocol.

  @retval EFI_SUCCESS    A protocol instance matching ProtocolGuid was found.
  @retval Others         A protocol instance matching ProtocolGuid was not found.
**/
EFI_STATUS
LocateProtocol (
  IN  EFI_GUID    *ProtocolGuid,
  OUT VOID        **Interface
  )

{
  EFI_STATUS    Status;
  UINTN         NumberHandles;
  UINTN         Index;
  EFI_HANDLE    *Handles;

  *Interface    = NULL;
  Handles       = NULL;
  NumberHandles = 0;

  Status        = LocateHandle (ByProtocol, ProtocolGuid, NULL, &NumberHandles, &Handles);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "LibLocateProtocol: Handle not found\n"));
    return Status;
  }

  for (Index = 0; Index < NumberHandles; Index++) {
    ASSERT (Handles != NULL);
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    ProtocolGuid,
                    Interface
                    );

    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  if (Handles != NULL) {
    FreePool (Handles);
  }

  return Status;
}

/**
  Helper function called to check the conflicted flags.

  @param[in] CheckList       The pointer to the VAR_CHECK_ITEM table.
  @param[in] ParamPackage    The pointer to the ParamPackage list.

  @retval EFI_SUCCESS              No conflicted flags.
  @retval EFI_INVALID_PARAMETER    The input parameter is erroroneous or there are some conflicted flags.
**/
EFI_STATUS
IpSecConfigRetriveCheckListByName (
  IN VAR_CHECK_ITEM    *CheckList,
  IN LIST_ENTRY        *ParamPackage
)
{

  LIST_ENTRY        *Node;
  VAR_CHECK_ITEM    *Item;
  UINT32            Attribute1;
  UINT32            Attribute2;
  UINT32            Attribute3;
  UINT32            Attribute4;
  UINT32            Index;

  Attribute1 = 0;
  Attribute2 = 0;
  Attribute3 = 0;
  Attribute4 = 0;
  Index      = 0;
  Item       = mIpSecConfigVarCheckList;

  if ((ParamPackage == NULL) || (CheckList == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Enumerate through the list of parameters that are input by user.
  //
  for (Node = GetFirstNode (ParamPackage); !IsNull (ParamPackage, Node); Node = GetNextNode (ParamPackage, Node)) {
    if (((SHELL_PARAM_PACKAGE *) Node)->Name != NULL) {
      //
      // Enumerate the check list that defines the conflicted attributes of each flag.
      //
      for (; Item->VarName != NULL; Item++) {
        if (StrCmp (((SHELL_PARAM_PACKAGE *) Node)->Name, Item->VarName) == 0) {
          Index++;
          if (Index == 1) {
            Attribute1 = Item->Attribute1;
            Attribute2 = Item->Attribute2;
            Attribute3 = Item->Attribute3;
            Attribute4 = Item->Attribute4;
          } else {
            Attribute1 &= Item->Attribute1;
            Attribute2 |= Item->Attribute2;
            Attribute3 &= Item->Attribute3;
            Attribute4 |= Item->Attribute4;
            if (Attribute1 != 0) {
              return EFI_INVALID_PARAMETER;
            }

            if (Attribute2 != 0) {
              if ((Index == 2) && (StrCmp (Item->VarName, L"-p") == 0)) {
                continue;
              }

              return EFI_INVALID_PARAMETER;
            }

            if (Attribute3 == 0) {
              return EFI_INVALID_PARAMETER;
            }
            if (((Attribute4 & 0xFF) == 0x03) || ((Attribute4 & 0xFF) == 0x0C) ||
                ((Attribute4 & 0xFF) == 0x30) || ((Attribute4 & 0xFF) == 0xC0)) {
              return EFI_INVALID_PARAMETER;
            }
          }
          break;
        }
      }

      Item = mIpSecConfigVarCheckList;
    }
  }

  return EFI_SUCCESS;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers, including
  both device drivers and bus drivers.

  The entry point for IpSecConfig application that parse the command line input and call an IpSecConfig process.

  @param[in] ImageHandle    The image handle of this application.
  @param[in] SystemTable    The pointer to the EFI System Table.

  @retval EFI_SUCCESS    The operation completed successfully.

**/
EFI_STATUS
EFIAPI
InitializeIpSecConfig (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_IPSEC_CONFIG_DATA_TYPE    DataType;
  UINT8                         Value;
  LIST_ENTRY                    *ParamPackage;
  CONST CHAR16                  *ValueStr;
  CHAR16                        *ProblemParam;
  UINTN                         NonOptionCount;

  //
  // Register our string package with HII and return the handle to it.
  //
  mHiiHandle = HiiAddPackages (&gEfiCallerIdGuid, ImageHandle, IpSecConfigStrings, NULL);
  ASSERT (mHiiHandle != NULL);

  Status = ShellCommandLineParseEx (mIpSecConfigParamList, &ParamPackage, &ProblemParam, TRUE, FALSE);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_UNKNOWN_OPERATION), mHiiHandle, ProblemParam);
    goto Done;
  }

  Status = IpSecConfigRetriveCheckListByName (mIpSecConfigVarCheckList, ParamPackage);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_MISTAKEN_OPTIONS), mHiiHandle);
    goto Done;
  }

  Status = LocateProtocol (&gEfiIpSecConfigProtocolGuid, (VOID **) &mIpSecConfig);
  if (EFI_ERROR (Status) || mIpSecConfig == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_PROTOCOL_INEXISTENT), mHiiHandle, mAppName);
    goto Done;
  }

  Status = LocateProtocol (&gEfiIpSec2ProtocolGuid, (VOID **) &mIpSec);
  if (EFI_ERROR (Status) || mIpSec == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_PROTOCOL_INEXISTENT), mHiiHandle, mAppName);
    goto Done;
  }

  //
  // Enable IPsec.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-enable")) {
    if (!(mIpSec->DisabledFlag)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_ALREADY_ENABLE), mHiiHandle, mAppName);
    } else {
      //
      // Set enable flag.
      //
      Value  = IPSEC_STATUS_ENABLED;
      Status = gRT->SetVariable (
                      IPSECCONFIG_STATUS_NAME,
                      &gEfiIpSecConfigProtocolGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      sizeof (Value),
                      &Value
                      );
      if (!EFI_ERROR (Status)) {
        mIpSec->DisabledFlag = FALSE;
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_ENABLE_SUCCESS), mHiiHandle, mAppName);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_ENABLE_FAILED), mHiiHandle, mAppName);
      }
    }

    goto Done;
  }

  //
  // Disable IPsec.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-disable")) {
    if (mIpSec->DisabledFlag) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_ALREADY_DISABLE), mHiiHandle, mAppName);
    } else {
      //
      // Set disable flag; however, leave it to be disabled in the callback function of DisabledEvent.
      //
      gBS->SignalEvent (mIpSec->DisabledEvent);
      if (mIpSec->DisabledFlag) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_DISABLE_SUCCESS), mHiiHandle, mAppName);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_DISABLE_FAILED), mHiiHandle, mAppName);
      }
    }

    goto Done;
  }

  //
  //IPsec Status.
  //
  if (ShellCommandLineGetFlag (ParamPackage, L"-status")) {
    if (mIpSec->DisabledFlag) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_STATUS_DISABLE), mHiiHandle, mAppName);
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_STATUS_ENABLE), mHiiHandle, mAppName);
    }
    goto Done;
  }

  //
  // Try to get policy database type.
  //
  DataType = (EFI_IPSEC_CONFIG_DATA_TYPE) - 1;
  ValueStr = ShellCommandLineGetValue (ParamPackage, L"-p");
  if (ValueStr != NULL) {
    DataType = (EFI_IPSEC_CONFIG_DATA_TYPE) MapStringToInteger (ValueStr, mMapPolicy);
    if (DataType == -1) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_DB), mHiiHandle, mAppName, ValueStr);
      goto Done;
    }
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-?")) {
    if (DataType == -1) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_HELP), mHiiHandle);
      goto Done;
    }

    switch (DataType) {
      case IPsecConfigDataTypeSpd:
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_SPD_HELP), mHiiHandle);
        break;

      case IPsecConfigDataTypeSad:
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_SAD_HELP), mHiiHandle);
        break;

      case IPsecConfigDataTypePad:
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_PAD_HELP), mHiiHandle);
        break;

      default:
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_DB), mHiiHandle);
        break;
    }

    goto Done;
  }

  NonOptionCount = ShellCommandLineGetCount (ParamPackage);
  if ((NonOptionCount - 1) > 0) {
    ValueStr = ShellCommandLineGetRawValue (ParamPackage, (UINT32) (NonOptionCount - 1));
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_REDUNDANCY_MANY), mHiiHandle, mAppName, ValueStr);
    goto Done;
  }

  if (DataType == -1) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_MISSING_DB), mHiiHandle, mAppName);
    goto Done;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-a")) {
    Status = AddOrInsertPolicyEntry (DataType, ParamPackage);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  } else if (ShellCommandLineGetFlag (ParamPackage, L"-i")) {
    Status = AddOrInsertPolicyEntry (DataType, ParamPackage);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  } else if (ShellCommandLineGetFlag (ParamPackage, L"-e")) {
    Status = EditPolicyEntry (DataType, ParamPackage);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  } else if (ShellCommandLineGetFlag (ParamPackage, L"-d")) {
    Status = FlushOrDeletePolicyEntry (DataType, ParamPackage);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  } else if (ShellCommandLineGetFlag (ParamPackage, L"-f")) {
    Status = FlushOrDeletePolicyEntry (DataType, ParamPackage);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  } else if (ShellCommandLineGetFlag (ParamPackage, L"-l")) {
    Status = ListPolicyEntry (DataType, ParamPackage);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_UNKNOWN_OPERATION), mHiiHandle, mAppName);
    goto Done;
  }

Done:
  ShellCommandLineFreeVarList (ParamPackage);
  HiiRemovePackages (mHiiHandle);

  return EFI_SUCCESS;
}
