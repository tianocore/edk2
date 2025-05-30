/** @file -- Tpm2DebugLibVerbose.c
This file contains helper functions to perform a detailed debugging of
TPM transactions as they go to and from the TPM device.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tpm2DebugLib.h>

#include <IndustryStandard/Tpm20.h>
#include <IndustryStandard/UefiTcgPlatform.h>

#define MAX_TPM_BUFFER_DUMP  240          // If printing an entire buffer, only print up to MAX bytes.

#pragma pack(1)

// TPM2_PCR_COMMON_COMMAND - The common fields shared between the PCR Extend
//                           and PCR Event commands.
typedef struct {
  TPM2_COMMAND_HEADER    Header;
  TPMI_DH_PCR            PcrHandle;
  UINT32                 AuthorizationSize;
  TPMS_AUTH_COMMAND      AuthSessionPcr;
} TPM2_PCR_COMMON_COMMAND;

typedef struct {
  TPM2_COMMAND_HEADER       Header;
  TPMI_RH_HIERARCHY_AUTH    AuthHandle;
  UINT32                    AuthorizationSize;
  TPMS_AUTH_COMMAND         AuthSession;
  TPM2B_AUTH                NewAuth;
} TPM2_HIERARCHY_CHANGE_AUTH_COMMAND;

typedef struct {
  TPM2_COMMAND_HEADER    Header;
  TPMI_SH_POLICY         PolicySession;
  TPML_DIGEST            HashList;
} TPM2_POLICY_OR_COMMAND;

typedef struct {
  TPM2_COMMAND_HEADER       Header;
  TPMI_DH_OBJECT            TpmKey;
  TPMI_DH_ENTITY            Bind;
  TPM2B_NONCE               NonceCaller;
  TPM2B_ENCRYPTED_SECRET    Salt;
  TPM_SE                    SessionType;
  TPMT_SYM_DEF              Symmetric;
  TPMI_ALG_HASH             AuthHash;
} TPM2_START_AUTH_SESSION_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER    Header;
  TPMI_SH_AUTH_SESSION    SessionHandle;
  TPM2B_NONCE             NonceTPM;
} TPM2_START_AUTH_SESSION_RESPONSE;

#pragma pack()

typedef struct {
  UINT32    Code;
  CHAR8     *Text;
} TPM2_CODE_STRING;

TPM2_CODE_STRING  CommandCodeStrings[] = {
  { TPM_CC_NV_UndefineSpaceSpecial,    "TPM_CC_NV_UndefineSpaceSpecial"    },
  { TPM_CC_EvictControl,               "TPM_CC_EvictControl"               },
  { TPM_CC_HierarchyControl,           "TPM_CC_HierarchyControl"           },
  { TPM_CC_NV_UndefineSpace,           "TPM_CC_NV_UndefineSpace"           },
  { TPM_CC_ChangeEPS,                  "TPM_CC_ChangeEPS"                  },
  { TPM_CC_ChangePPS,                  "TPM_CC_ChangePPS"                  },
  { TPM_CC_Clear,                      "TPM_CC_Clear"                      },
  { TPM_CC_ClearControl,               "TPM_CC_ClearControl"               },
  { TPM_CC_ClockSet,                   "TPM_CC_ClockSet"                   },
  { TPM_CC_HierarchyChangeAuth,        "TPM_CC_HierarchyChangeAuth"        },
  { TPM_CC_NV_DefineSpace,             "TPM_CC_NV_DefineSpace"             },
  { TPM_CC_PCR_Allocate,               "TPM_CC_PCR_Allocate"               },
  { TPM_CC_PCR_SetAuthPolicy,          "TPM_CC_PCR_SetAuthPolicy"          },
  { TPM_CC_PP_Commands,                "TPM_CC_PP_Commands"                },
  { TPM_CC_SetPrimaryPolicy,           "TPM_CC_SetPrimaryPolicy"           },
  { TPM_CC_FieldUpgradeStart,          "TPM_CC_FieldUpgradeStart"          },
  { TPM_CC_ClockRateAdjust,            "TPM_CC_ClockRateAdjust"            },
  { TPM_CC_CreatePrimary,              "TPM_CC_CreatePrimary"              },
  { TPM_CC_NV_GlobalWriteLock,         "TPM_CC_NV_GlobalWriteLock"         },
  { TPM_CC_PP_LAST,                    "TPM_CC_PP_LAST"                    },
  { TPM_CC_GetCommandAuditDigest,      "TPM_CC_GetCommandAuditDigest"      },
  { TPM_CC_NV_Increment,               "TPM_CC_NV_Increment"               },
  { TPM_CC_NV_SetBits,                 "TPM_CC_NV_SetBits"                 },
  { TPM_CC_NV_Extend,                  "TPM_CC_NV_Extend"                  },
  { TPM_CC_NV_Write,                   "TPM_CC_NV_Write"                   },
  { TPM_CC_NV_WriteLock,               "TPM_CC_NV_WriteLock"               },
  { TPM_CC_DictionaryAttackLockReset,  "TPM_CC_DictionaryAttackLockReset"  },
  { TPM_CC_DictionaryAttackParameters, "TPM_CC_DictionaryAttackParameters" },
  { TPM_CC_NV_ChangeAuth,              "TPM_CC_NV_ChangeAuth"              },
  { TPM_CC_PCR_Event,                  "TPM_CC_PCR_Event"                  },
  { TPM_CC_PCR_Reset,                  "TPM_CC_PCR_Reset"                  },
  { TPM_CC_SequenceComplete,           "TPM_CC_SequenceComplete"           },
  { TPM_CC_SetAlgorithmSet,            "TPM_CC_SetAlgorithmSet"            },
  { TPM_CC_SetCommandCodeAuditStatus,  "TPM_CC_SetCommandCodeAuditStatus"  },
  { TPM_CC_FieldUpgradeData,           "TPM_CC_FieldUpgradeData"           },
  { TPM_CC_IncrementalSelfTest,        "TPM_CC_IncrementalSelfTest"        },
  { TPM_CC_SelfTest,                   "TPM_CC_SelfTest"                   },
  { TPM_CC_Startup,                    "TPM_CC_Startup"                    },
  { TPM_CC_Shutdown,                   "TPM_CC_Shutdown"                   },
  { TPM_CC_StirRandom,                 "TPM_CC_StirRandom"                 },
  { TPM_CC_ActivateCredential,         "TPM_CC_ActivateCredential"         },
  { TPM_CC_Certify,                    "TPM_CC_Certify"                    },
  { TPM_CC_PolicyNV,                   "TPM_CC_PolicyNV"                   },
  { TPM_CC_CertifyCreation,            "TPM_CC_CertifyCreation"            },
  { TPM_CC_Duplicate,                  "TPM_CC_Duplicate"                  },
  { TPM_CC_GetTime,                    "TPM_CC_GetTime"                    },
  { TPM_CC_GetSessionAuditDigest,      "TPM_CC_GetSessionAuditDigest"      },
  { TPM_CC_NV_Read,                    "TPM_CC_NV_Read"                    },
  { TPM_CC_NV_ReadLock,                "TPM_CC_NV_ReadLock"                },
  { TPM_CC_ObjectChangeAuth,           "TPM_CC_ObjectChangeAuth"           },
  { TPM_CC_PolicySecret,               "TPM_CC_PolicySecret"               },
  { TPM_CC_Rewrap,                     "TPM_CC_Rewrap"                     },
  { TPM_CC_Create,                     "TPM_CC_Create"                     },
  { TPM_CC_ECDH_ZGen,                  "TPM_CC_ECDH_ZGen"                  },
  { TPM_CC_HMAC,                       "TPM_CC_HMAC"                       },
  { TPM_CC_Import,                     "TPM_CC_Import"                     },
  { TPM_CC_Load,                       "TPM_CC_Load"                       },
  { TPM_CC_Quote,                      "TPM_CC_Quote"                      },
  { TPM_CC_RSA_Decrypt,                "TPM_CC_RSA_Decrypt"                },
  { TPM_CC_HMAC_Start,                 "TPM_CC_HMAC_Start"                 },
  { TPM_CC_SequenceUpdate,             "TPM_CC_SequenceUpdate"             },
  { TPM_CC_Sign,                       "TPM_CC_Sign"                       },
  { TPM_CC_Unseal,                     "TPM_CC_Unseal"                     },
  { TPM_CC_PolicySigned,               "TPM_CC_PolicySigned"               },
  { TPM_CC_ContextLoad,                "TPM_CC_ContextLoad"                },
  { TPM_CC_ContextSave,                "TPM_CC_ContextSave"                },
  { TPM_CC_ECDH_KeyGen,                "TPM_CC_ECDH_KeyGen"                },
  { TPM_CC_EncryptDecrypt,             "TPM_CC_EncryptDecrypt"             },
  { TPM_CC_FlushContext,               "TPM_CC_FlushContext"               },
  { TPM_CC_LoadExternal,               "TPM_CC_LoadExternal"               },
  { TPM_CC_MakeCredential,             "TPM_CC_MakeCredential"             },
  { TPM_CC_NV_ReadPublic,              "TPM_CC_NV_ReadPublic"              },
  { TPM_CC_PolicyAuthorize,            "TPM_CC_PolicyAuthorize"            },
  { TPM_CC_PolicyAuthValue,            "TPM_CC_PolicyAuthValue"            },
  { TPM_CC_PolicyCommandCode,          "TPM_CC_PolicyCommandCode"          },
  { TPM_CC_PolicyCounterTimer,         "TPM_CC_PolicyCounterTimer"         },
  { TPM_CC_PolicyCpHash,               "TPM_CC_PolicyCpHash"               },
  { TPM_CC_PolicyLocality,             "TPM_CC_PolicyLocality"             },
  { TPM_CC_PolicyNameHash,             "TPM_CC_PolicyNameHash"             },
  { TPM_CC_PolicyOR,                   "TPM_CC_PolicyOR"                   },
  { TPM_CC_PolicyTicket,               "TPM_CC_PolicyTicket"               },
  { TPM_CC_ReadPublic,                 "TPM_CC_ReadPublic"                 },
  { TPM_CC_RSA_Encrypt,                "TPM_CC_RSA_Encrypt"                },
  { TPM_CC_StartAuthSession,           "TPM_CC_StartAuthSession"           },
  { TPM_CC_VerifySignature,            "TPM_CC_VerifySignature"            },
  { TPM_CC_ECC_Parameters,             "TPM_CC_ECC_Parameters"             },
  { TPM_CC_FirmwareRead,               "TPM_CC_FirmwareRead"               },
  { TPM_CC_GetCapability,              "TPM_CC_GetCapability"              },
  { TPM_CC_GetRandom,                  "TPM_CC_GetRandom"                  },
  { TPM_CC_GetTestResult,              "TPM_CC_GetTestResult"              },
  { TPM_CC_Hash,                       "TPM_CC_Hash"                       },
  { TPM_CC_PCR_Read,                   "TPM_CC_PCR_Read"                   },
  { TPM_CC_PolicyPCR,                  "TPM_CC_PolicyPCR"                  },
  { TPM_CC_PolicyRestart,              "TPM_CC_PolicyRestart"              },
  { TPM_CC_ReadClock,                  "TPM_CC_ReadClock"                  },
  { TPM_CC_PCR_Extend,                 "TPM_CC_PCR_Extend"                 },
  { TPM_CC_PCR_SetAuthValue,           "TPM_CC_PCR_SetAuthValue"           },
  { TPM_CC_NV_Certify,                 "TPM_CC_NV_Certify"                 },
  { TPM_CC_EventSequenceComplete,      "TPM_CC_EventSequenceComplete"      },
  { TPM_CC_HashSequenceStart,          "TPM_CC_HashSequenceStart"          },
  { TPM_CC_PolicyPhysicalPresence,     "TPM_CC_PolicyPhysicalPresence"     },
  { TPM_CC_PolicyDuplicationSelect,    "TPM_CC_PolicyDuplicationSelect"    },
  { TPM_CC_PolicyGetDigest,            "TPM_CC_PolicyGetDigest"            },
  { TPM_CC_TestParms,                  "TPM_CC_TestParms"                  },
  { TPM_CC_Commit,                     "TPM_CC_Commit"                     },
  { TPM_CC_PolicyPassword,             "TPM_CC_PolicyPassword"             },
  { TPM_CC_ZGen_2Phase,                "TPM_CC_ZGen_2Phase"                },
  { TPM_CC_EC_Ephemeral,               "TPM_CC_EC_Ephemeral"               },
};
UINTN             CommandCodeStringsCount = sizeof (CommandCodeStrings) / sizeof (CommandCodeStrings[0]);

TPM2_CODE_STRING  ResponseCodeStrings[] = {
  { TPM_RC_SUCCESS,           "TPM_RC_SUCCESS"           },
  { TPM_RC_BAD_TAG,           "TPM_RC_BAD_TAG"           },
  { TPM_RC_INITIALIZE,        "TPM_RC_INITIALIZE"        },
  { TPM_RC_FAILURE,           "TPM_RC_FAILURE"           },
  { TPM_RC_SEQUENCE,          "TPM_RC_SEQUENCE"          },
  { TPM_RC_PRIVATE,           "TPM_RC_PRIVATE"           },
  { TPM_RC_HMAC,              "TPM_RC_HMAC"              },
  { TPM_RC_DISABLED,          "TPM_RC_DISABLED"          },
  { TPM_RC_EXCLUSIVE,         "TPM_RC_EXCLUSIVE"         },
  { TPM_RC_AUTH_TYPE,         "TPM_RC_AUTH_TYPE"         },
  { TPM_RC_AUTH_MISSING,      "TPM_RC_AUTH_MISSING"      },
  { TPM_RC_POLICY,            "TPM_RC_POLICY"            },
  { TPM_RC_PCR,               "TPM_RC_PCR"               },
  { TPM_RC_PCR_CHANGED,       "TPM_RC_PCR_CHANGED"       },
  { TPM_RC_UPGRADE,           "TPM_RC_UPGRADE"           },
  { TPM_RC_TOO_MANY_CONTEXTS, "TPM_RC_TOO_MANY_CONTEXTS" },
  { TPM_RC_AUTH_UNAVAILABLE,  "TPM_RC_AUTH_UNAVAILABLE"  },
  { TPM_RC_REBOOT,            "TPM_RC_REBOOT"            },
  { TPM_RC_UNBALANCED,        "TPM_RC_UNBALANCED"        },
  { TPM_RC_COMMAND_SIZE,      "TPM_RC_COMMAND_SIZE"      },
  { TPM_RC_COMMAND_CODE,      "TPM_RC_COMMAND_CODE"      },
  { TPM_RC_AUTHSIZE,          "TPM_RC_AUTHSIZE"          },
  { TPM_RC_AUTH_CONTEXT,      "TPM_RC_AUTH_CONTEXT"      },
  { TPM_RC_NV_RANGE,          "TPM_RC_NV_RANGE"          },
  { TPM_RC_NV_SIZE,           "TPM_RC_NV_SIZE"           },
  { TPM_RC_NV_LOCKED,         "TPM_RC_NV_LOCKED"         },
  { TPM_RC_NV_AUTHORIZATION,  "TPM_RC_NV_AUTHORIZATION"  },
  { TPM_RC_NV_UNINITIALIZED,  "TPM_RC_NV_UNINITIALIZED"  },
  { TPM_RC_NV_SPACE,          "TPM_RC_NV_SPACE"          },
  { TPM_RC_NV_DEFINED,        "TPM_RC_NV_DEFINED"        },
  { TPM_RC_BAD_CONTEXT,       "TPM_RC_BAD_CONTEXT"       },
  { TPM_RC_CPHASH,            "TPM_RC_CPHASH"            },
  { TPM_RC_PARENT,            "TPM_RC_PARENT"            },
  { TPM_RC_NEEDS_TEST,        "TPM_RC_NEEDS_TEST"        },
  { TPM_RC_NO_RESULT,         "TPM_RC_NO_RESULT"         },
  { TPM_RC_SENSITIVE,         "TPM_RC_SENSITIVE"         },
};
UINTN             ResponseCodeStringsCount = sizeof (ResponseCodeStrings) / sizeof (ResponseCodeStrings[0]);

UINT32  mLastCommandSent = 0;

/**
  This simple function will dump up to MAX_TPM_BUFFER_DUMP bytes
  of a TPM data buffer and apppend '...' if buffer is larger.

  @param[in]  DebugLevel    The debugging level to use.
  @param[in]  Preamble      [Optional] A string to print before the buffer dump.
  @param[in]  BufferSize    The actual size of the provided buffer.
  @param[in]  Buffer        A pointer to the buffer in question.

**/
VOID
DumpTpmBuffer (
  IN UINTN        DebugLevel,
  IN CHAR8        *Preamble OPTIONAL,
  IN UINTN        BufferSize,
  IN CONST UINT8  *Buffer
  )
{
  UINTN  DebugBufferCount, Index;

  // TODO: Don't even evaluate if below required debugging level.
  // TODO: Pass in max buffer size? Format nicely?

  // Determine max buffer size.
  DebugBufferCount = MIN (BufferSize, MAX_TPM_BUFFER_DUMP);

  // Print the preamble, if supplied.
  if (Preamble) {
    DEBUG ((DebugLevel, "%a", Preamble));
  }

  // Dump them bytes.
  for (Index = 0; Index < DebugBufferCount; Index++) {
    DEBUG ((DebugLevel, "%02X ", Buffer[Index]));
  }

  // FINISH HIM!!!
  if (DebugBufferCount != BufferSize) {
    DEBUG ((DebugLevel, "...\n"));
  } else {
    DEBUG ((DebugLevel, "\n"));
  }

  return;
}

/**
  This abstract function takes in a list of codes and strings and returns either
  a string matching the code or the supplied "default" string.

  @param[in]  Code            TPM2 Command or Response Code in little-endian format.
  @param[in]  List            Pointer to the start of a TPM2_CODE_STRING list.
  @param[in]  ListCount       Number of items in the given list.
  @param[in]  DefaultString   A string to be returned if no matching string is found.

  @retval     CHAR8* of a human-readable string for the code (as defined in the spec).

**/
STATIC
CHAR8 *
GetTpmCodeString (
  IN UINT32            Code,
  IN TPM2_CODE_STRING  *List,
  IN UINTN             ListCount,
  IN CHAR8             *DefaultString
  )
{
  UINTN  Index;
  CHAR8  *Result = DefaultString;

  // Find the code in the string array.
  for (Index = 0; Index < ListCount; Index++) {
    if (List[Index].Code == Code) {
      Result = List[Index].Text;
      break;
    }
  }

  return Result;
}

/**
  Simple wrapper function to use GetTpmCodeString() to search for a command code.

  @param[in]  Code    TPM2 Command Code in marshalled (generally big-endian) format.

  @retval     CHAR8* of a human-readable string for the code (as defined in the spec).

**/
CHAR8 *
GetTpm2CommandString (
  IN TPM_CC  Code
  )
{
  return GetTpmCodeString (SwapBytes32 (Code), CommandCodeStrings, CommandCodeStringsCount, "TPM_CC_UNKNOWN");
}

/**
  Simple wrapper function to use GetTpmCodeString() to search for a response code.

  @param[in]  Code    TPM2 Response Code in marshalled (generally big-endian) format.

  @retval     CHAR8* of a human-readable string for the code (as defined in the spec).

**/
CHAR8 *
GetTpm2ResponseString (
  IN TPM_RC  Code
  )
{
  return GetTpmCodeString (SwapBytes32 (Code), ResponseCodeStrings, ResponseCodeStringsCount, "TPM_RC_UNKNOWN");
}

/**
  This function dumps a TPM2B_DIGEST structure.

  @param[in]  IsCommand       Indicates that the auth session structure starts with a session handle.
  @param[in]  AuthSessionPtr  Pointer to the auth session to be printed.

**/
STATIC
VOID
Dump2bDigest (
  IN UINTN               DebugLevel,
  IN CHAR8               *Preamble OPTIONAL,
  IN CONST TPM2B_DIGEST  *Digest
  )
{
  UINT16  DigestSize;

  DigestSize = SwapBytes16 (Digest->size);
  if (DigestSize) {
    DumpTpmBuffer (DebugLevel, Preamble, SwapBytes16 (Digest->size), Digest->buffer);
  } else {
    DEBUG ((DebugLevel, "%a<EMPTY>\n", Preamble));
  }

  return;
}

/**
  This function dumps an AuthSession structure from a command or response.

  @param[in]  IsCommand       Indicates that the auth session structure starts with a session handle.
  @param[in]  AuthSessionPtr  Pointer to the auth session to be printed.

**/
STATIC
VOID
DumpAuthSession (
  IN BOOLEAN      IsCommand,
  IN CONST UINT8  *AuthSession
  )
{
  CONST UINT8         *FloatingPtr;
  CONST TPM2B_DIGEST  *Digest;

  FloatingPtr = AuthSession;

  DEBUG ((DEBUG_INFO, "== Auth Session ==\n"));

  // If we're pointing to a command auth session,
  // we need to print the session handle.
  if (IsCommand) {
    DEBUG ((DEBUG_INFO, "- SHand:  0x%8X\n", SwapBytes32 (*(UINT32 *)FloatingPtr)));
    FloatingPtr += sizeof (UINT32);
  }

  // Print the common parts of an auth session.
  Digest = (TPM2B_DIGEST *)FloatingPtr;
  Dump2bDigest (DEBUG_INFO, "- Nonce:  ", Digest);
  FloatingPtr += sizeof (UINT16) + SwapBytes16 (Digest->size);        // Add the size field and the digest size.

  DEBUG ((DEBUG_INFO, "- Attr:   0x%2X\n", *FloatingPtr));
  FloatingPtr++;

  Digest = (TPM2B_DIGEST *)FloatingPtr;
  Dump2bDigest (DEBUG_INFO, "- HMAC:   ", Digest);

  return;
}

/**
  This function dumps additional information about TPM PCR Extend
  and Event operations.

  @param[in]  Command         Little-endian format of the command being issued.
  @param[in]  InputBlockSize  Size of the input buffer.
  @param[in]  InputBlock      Pointer to the input buffer itself.

**/
STATIC
VOID
DumpTpmPcrCommand (
  IN TPM_CC       Command,
  IN UINT32       InputBlockSize,
  IN CONST UINT8  *InputBlock
  )
{
  CONST TPM2_PCR_COMMON_COMMAND  *PcrHeader;
  TPMI_DH_PCR                    PcrHandle;

  // If this is an unrecognized command, we can't go much further.
  if ((Command != TPM_CC_PCR_Extend) && (Command != TPM_CC_PCR_Event)) {
    DEBUG ((DEBUG_WARN, "%a - Unrecognized command! 0x%X\n", __func__, Command));
    return;
  }

  // Start the debugging by mapping some stuff.
  PcrHeader = (TPM2_PCR_COMMON_COMMAND *)InputBlock;
  PcrHandle = SwapBytes32 (PcrHeader->PcrHandle);
  DEBUG ((DEBUG_INFO, "- PCR:    %d (0x%08X)\n", PcrHandle, PcrHandle));

  // Now handle any command-specific debugging.
  if (Command == TPM_CC_PCR_Extend) {
    CONST TPML_DIGEST_VALUES  *DigestValues;
    CONST TPMT_HA             *CurrentDigest;
    UINT32                    DigestCount;

    // Move the dump data to the start of the digest section.
    DigestValues = (TPML_DIGEST_VALUES *)(InputBlock + OFFSET_OF (TPM2_PCR_COMMON_COMMAND, AuthSessionPcr) + SwapBytes32 (PcrHeader->AuthorizationSize));

    // Determine the digest count and locate the first digest.
    DigestCount   = SwapBytes32 (DigestValues->count);
    CurrentDigest = &DigestValues->digests[0];

    // Print each digest.
    do {
      // Print the current digest.
      switch (SwapBytes16 (CurrentDigest->hashAlg)) {
        case TPM_ALG_SHA1:
          DumpTpmBuffer (DEBUG_INFO, "- SHA1:   ", SHA1_DIGEST_SIZE, CurrentDigest->digest.sha1);
          CurrentDigest = (TPMT_HA *)((UINT8 *)CurrentDigest + OFFSET_OF (TPMT_HA, digest) + SHA1_DIGEST_SIZE);
          DigestCount--;    // Account for this digest.
          break;

        case TPM_ALG_SHA256:
          DumpTpmBuffer (DEBUG_INFO, "- SHA256: ", SHA256_DIGEST_SIZE, CurrentDigest->digest.sha256);
          CurrentDigest = (TPMT_HA *)((UINT8 *)CurrentDigest + OFFSET_OF (TPMT_HA, digest) + SHA256_DIGEST_SIZE);
          DigestCount--;    // Account for this digest.
          break;

        default:
          // This algorithm hasn't been programmed yet. We need to bail.
          DEBUG ((DEBUG_WARN, "%a - Unknown hash algorithm! 0x%04X\n", __func__, SwapBytes16 (CurrentDigest->hashAlg)));
          // Zero the count so we can get out of here.
          DigestCount = 0;
          break;
      }
    } while (DigestCount > 0);
  }

  return;
}

/**
  This function dumps additional information about TPM Hierarchy Change Auth operations.

  @param[in]  InputBlockSize  Size of the input buffer.
  @param[in]  InputBlock      Pointer to the input buffer itself.

**/
STATIC
VOID
DumpTpmHierarchyChangeAuthCommand (
  IN UINT32       InputBlockSize,
  IN CONST UINT8  *InputBlock
  )
{
  CONST TPM2_HIERARCHY_CHANGE_AUTH_COMMAND  *ChangeAuthCommand;
  CONST TPM2B_DIGEST                        *Digest;

  // Start the debugging by mapping some stuff.
  ChangeAuthCommand = (TPM2_HIERARCHY_CHANGE_AUTH_COMMAND *)InputBlock;

  // Print the auth handle (the handle being authorized against).
  DEBUG ((DEBUG_INFO, "- AHand:  0x%8X\n", SwapBytes32 (ChangeAuthCommand->AuthHandle)));

  // Print the auth session.
  DumpAuthSession (TRUE, (UINT8 *)&ChangeAuthCommand->AuthSession);

  // Print the new auth.
  Digest = (TPM2B_DIGEST *)(InputBlock + OFFSET_OF (TPM2_HIERARCHY_CHANGE_AUTH_COMMAND, AuthSession) + SwapBytes32 (ChangeAuthCommand->AuthorizationSize));
  Dump2bDigest (DEBUG_INFO, "- NAuth:  ", Digest);

  return;
}

/**
  This function dumps additional information about Policy OR operations.

  @param[in]  InputBlockSize  Size of the input buffer.
  @param[in]  InputBlock      Pointer to the input buffer itself.

**/
STATIC
VOID
DumpTpmPolicyOrCommand (
  IN UINT32       InputBlockSize,
  IN CONST UINT8  *InputBlock
  )
{
  CONST TPM2_POLICY_OR_COMMAND  *PolicyOrCommand;
  CONST TPM2B_DIGEST            *Digest;
  UINT32                        Index;

  // Start the debugging by mapping some stuff.
  PolicyOrCommand = (TPM2_POLICY_OR_COMMAND *)InputBlock;

  // Print the policy session token.
  DEBUG ((DEBUG_INFO, "- PSess:  0x%8X\n", SwapBytes32 (PolicyOrCommand->PolicySession)));

  // Print out all of the digests.
  Digest = &PolicyOrCommand->HashList.digests[0];
  for (Index = 0; Index < SwapBytes32 (PolicyOrCommand->HashList.count); Index++) {
    Dump2bDigest (DEBUG_INFO, "- Digest: ", Digest);
    // Add the size field and the digest size.
    Digest = (TPM2B_DIGEST *)((UINT8 *)Digest + sizeof (UINT16) + SwapBytes16 (Digest->size));
  }

  return;
}

/**
  This function dumps additional information about Start Auth Session operations.

  @param[in]  InputBlockSize  Size of the input buffer.
  @param[in]  InputBlock      Pointer to the input buffer itself.

**/
STATIC
VOID
DumpTpmStartAuthSessionCommand (
  IN UINT32       InputBlockSize,
  IN CONST UINT8  *InputBlock
  )
{
  CONST UINT8                            *FloatingPtr;
  CONST TPM2_START_AUTH_SESSION_COMMAND  *StartAuthSessionCommand;
  CONST TPM2B_DIGEST                     *Digest;

  // Start the debugging by mapping some stuff.
  StartAuthSessionCommand = (TPM2_START_AUTH_SESSION_COMMAND *)InputBlock;

  // Print as many of the simple fields as possible.
  DEBUG ((DEBUG_INFO, "- TpmKey: 0x%8X\n", SwapBytes32 (StartAuthSessionCommand->TpmKey)));
  DEBUG ((DEBUG_INFO, "- Bind:   0x%8X\n", SwapBytes32 (StartAuthSessionCommand->Bind)));
  Digest = (TPM2B_DIGEST *)&StartAuthSessionCommand->NonceCaller;
  Dump2bDigest (DEBUG_INFO, "- NonceC: ", Digest);
  // Add the size field and the digest size.
  FloatingPtr = (UINT8 *)Digest + sizeof (UINT16) + SwapBytes16 (Digest->size);

  // Skip the Salt for now.
  Digest = (TPM2B_DIGEST *)FloatingPtr;
  Dump2bDigest (DEBUG_INFO, "- Salt:  ", Digest);
  FloatingPtr += sizeof (UINT16) + SwapBytes16 (Digest->size);

  // Print the remaining things.
  DEBUG ((DEBUG_INFO, "- SesTyp: 0x%2X\n", *FloatingPtr));
  FloatingPtr++;

  // Skip Symmetric for now.
  // NOTE: Symmetric is an interpreted structure, so just jump to the end.
  FloatingPtr = InputBlock + (InputBlockSize - sizeof (UINT16));
  DEBUG ((DEBUG_INFO, "- AHash:  0x%4X\n", SwapBytes16 (*(UINT16 *)FloatingPtr)));

  return;
}

/**
  This function dumps additional information about Start Auth Session operations.

  @param[in]  OutputBlockSize  Size of the output buffer.
  @param[in]  OutputBlock      Pointer to the output buffer itself.

**/
STATIC
VOID
DumpTpmStartAuthSessionResponse (
  IN UINT32       OutputBlockSize,
  IN CONST UINT8  *OutputBlock
  )
{
  CONST TPM2_START_AUTH_SESSION_RESPONSE  *StartAuthSessionResponse;
  CONST TPM2B_DIGEST                      *Digest;

  // Start the debugging by mapping some stuff.
  StartAuthSessionResponse = (TPM2_START_AUTH_SESSION_RESPONSE *)OutputBlock;

  // Dump the handle and the nonce.
  DEBUG ((DEBUG_INFO, "- SHand:  0x%8X\n", SwapBytes32 (StartAuthSessionResponse->SessionHandle)));
  Digest = (TPM2B_DIGEST *)&StartAuthSessionResponse->NonceTPM;
  Dump2bDigest (DEBUG_INFO, "- NonceT: ", Digest);

  return;
}

/**
  This function dumps as much information as possible about
  a command being sent to the TPM for maximum user-readability.

  @param[in]  InputBlockSize  Size of the input buffer.
  @param[in]  InputBlock      Pointer to the input buffer itself.

**/
VOID
EFIAPI
DumpTpmInputBlock (
  IN UINT32       InputBlockSize,
  IN CONST UINT8  *InputBlock
  )
{
  CONST TPM2_COMMAND_HEADER  *CommHeader;
  TPM_ST                     NativeTag;
  UINT32                     NativeSize;
  TPM_CC                     NativeCode;

  DEBUG ((DEBUG_INFO, "\n=== BEGIN TPM COMMAND ===\n"));
  DEBUG ((DEBUG_VERBOSE, "Size:     %d (0x%X), Address:  0x%X\n", InputBlockSize, InputBlockSize, InputBlock));

  // Make sure we've got at least enough data for a valid header.
  if (InputBlockSize < sizeof (*CommHeader)) {
    DEBUG ((DEBUG_WARN, "%a - Invalid buffer size!\n", __func__));
    return;
  }

  // Start the debugging by mapping some stuff.
  CommHeader = (TPM2_COMMAND_HEADER *)InputBlock;
  NativeTag  = SwapBytes16 (CommHeader->tag);
  NativeSize = SwapBytes32 (CommHeader->paramSize);
  NativeCode = SwapBytes32 (CommHeader->commandCode);
  DEBUG ((DEBUG_INFO, "Command:  %a (0x%08X)\n", GetTpm2CommandString (CommHeader->commandCode), NativeCode));
  DEBUG ((DEBUG_INFO, "Tag:      0x%04X\n", NativeTag));
  DEBUG ((DEBUG_INFO, "Size:     %d (0x%X)\n", NativeSize, NativeSize));

  // Debug anything else.
  switch (NativeCode) {
    case TPM_CC_PCR_Event:
    case TPM_CC_PCR_Extend:
      DumpTpmPcrCommand (NativeCode, InputBlockSize, InputBlock);
      break;

    case TPM_CC_HierarchyChangeAuth:
      DumpTpmHierarchyChangeAuthCommand (InputBlockSize, InputBlock);
      break;

    case TPM_CC_PolicyOR:
      DumpTpmPolicyOrCommand (InputBlockSize, InputBlock);
      break;

    case TPM_CC_StartAuthSession:
      DumpTpmStartAuthSessionCommand (InputBlockSize, InputBlock);
      break;

    default:
      break;
  }

  // If verbose, dump all of the buffer contents for deeper analysis.
  DumpTpmBuffer (DEBUG_INFO, "DATA:     ", MIN (InputBlockSize, NativeSize), InputBlock);

  // Update the last command sent so that response parsing can have some context.
  mLastCommandSent = NativeCode;

  return;
}

/**
  This function dumps as much information as possible about
  a response from the TPM for maximum user-readability.

  @param[in]  OutputBlockSize  Size of the output buffer.
  @param[in]  OutputBlock      Pointer to the output buffer itself.

**/
VOID
EFIAPI
DumpTpmOutputBlock (
  IN UINT32       OutputBlockSize,
  IN CONST UINT8  *OutputBlock
  )
{
  CONST TPM2_RESPONSE_HEADER  *RespHeader;
  TPM_ST                      NativeTag;
  UINT32                      NativeSize;
  TPM_CC                      NativeCode;

  DEBUG ((DEBUG_VERBOSE, "Size:     %d (0x%X), Address:  0x%X\n", OutputBlockSize, OutputBlockSize, OutputBlock));

  // Start the debugging by mapping some stuff.
  RespHeader = (TPM2_RESPONSE_HEADER *)OutputBlock;
  NativeTag  = SwapBytes16 (RespHeader->tag);
  NativeSize = SwapBytes32 (RespHeader->paramSize);
  NativeCode = SwapBytes32 (RespHeader->responseCode);
  DEBUG ((DEBUG_INFO, "Response: %a (0x%08X)\n", GetTpm2ResponseString (RespHeader->responseCode), NativeCode));
  DEBUG ((DEBUG_INFO, "Tag:      0x%04X\n", NativeTag));
  DEBUG ((DEBUG_INFO, "Size:     %d (0x%X)\n", NativeSize, NativeSize));

  // Debug anything else based on the Command context.
  if (mLastCommandSent != 0x00) {
    switch (mLastCommandSent) {
      case TPM_CC_StartAuthSession:
        DumpTpmStartAuthSessionResponse (OutputBlockSize, OutputBlock);
        break;

      default:
        break;
    }
  }

  // If verbose, dump all of the buffer contents for deeper analysis.
  DumpTpmBuffer (DEBUG_INFO, "DATA:     ", MIN (OutputBlockSize, NativeSize), OutputBlock);

  DEBUG ((DEBUG_INFO, "=== END TPM COMMAND ===\n\n"));

  return;
}

/**

  This function dump raw data.

  @param  Data  raw data
  @param  Size  raw data size

**/
STATIC
VOID
InternalDumpData (
  IN UINT8  *Data,
  IN UINTN  Size
  )
{
  UINTN  Index;

  for (Index = 0; Index < Size; Index++) {
    DEBUG ((DEBUG_INFO, "%02x", (UINTN)Data[Index]));
  }
}

/**

  This function dump raw data with colume format.

  @param  Data  raw data
  @param  Size  raw data size

**/
STATIC
VOID
InternalDumpHex (
  IN UINT8  *Data,
  IN UINTN  Size
  )
{
  UINTN  Index;
  UINTN  Count;
  UINTN  Left;

  #define COLUME_SIZE  (16 * 2)

  Count = Size / COLUME_SIZE;
  Left  = Size % COLUME_SIZE;
  for (Index = 0; Index < Count; Index++) {
    DEBUG ((DEBUG_INFO, "%04x: ", Index * COLUME_SIZE));
    InternalDumpData (Data + Index * COLUME_SIZE, COLUME_SIZE);
    DEBUG ((DEBUG_INFO, "\n"));
  }

  if (Left != 0) {
    DEBUG ((DEBUG_INFO, "%04x: ", Index * COLUME_SIZE));
    InternalDumpData (Data + Index * COLUME_SIZE, Left);
    DEBUG ((DEBUG_INFO, "\n"));
  }
}

/**
  This function dump PCR event.

  @param[in]  EventHdr     TCG PCR event structure.
**/
STATIC
VOID
DumpEvent (
  IN TCG_PCR_EVENT_HDR  *EventHdr
  )
{
  UINTN  Index;

  DEBUG ((DEBUG_INFO, "  Event:\n"));
  DEBUG ((DEBUG_INFO, "    PCRIndex  - %d\n", EventHdr->PCRIndex));
  DEBUG ((DEBUG_INFO, "    EventType - 0x%08x\n", EventHdr->EventType));
  DEBUG ((DEBUG_INFO, "    Digest    - "));
  for (Index = 0; Index < sizeof (TCG_DIGEST); Index++) {
    DEBUG ((DEBUG_INFO, "%02x ", EventHdr->Digest.digest[Index]));
  }

  DEBUG ((DEBUG_INFO, "\n"));
  DEBUG ((DEBUG_INFO, "    EventSize - 0x%08x\n", EventHdr->EventSize));
  InternalDumpHex ((UINT8 *)(EventHdr + 1), EventHdr->EventSize);
}

/**
  This function dump PCR event 2.

  @param[in]  TcgPcrEvent2     TCG PCR event 2 structure.
**/
STATIC
VOID
DumpEvent2 (
  IN TCG_PCR_EVENT2  *TcgPcrEvent2
  )
{
  UINTN          Index;
  UINT32         DigestIndex;
  UINT32         DigestCount;
  TPMI_ALG_HASH  HashAlgo;
  UINT32         DigestSize;
  UINT8          *DigestBuffer;
  UINT32         EventSize;
  UINT8          *EventBuffer;

  DEBUG ((DEBUG_INFO, "  Event:\n"));
  DEBUG ((DEBUG_INFO, "    PCRIndex  - %d\n", TcgPcrEvent2->PCRIndex));
  DEBUG ((DEBUG_INFO, "    EventType - 0x%08x\n", TcgPcrEvent2->EventType));

  DEBUG ((DEBUG_INFO, "    DigestCount: 0x%08x\n", TcgPcrEvent2->Digest.count));

  DigestCount  = TcgPcrEvent2->Digest.count;
  HashAlgo     = TcgPcrEvent2->Digest.digests[0].hashAlg;
  DigestBuffer = (UINT8 *)&TcgPcrEvent2->Digest.digests[0].digest;
  for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
    DEBUG ((DEBUG_INFO, "      HashAlgo : 0x%04x\n", HashAlgo));
    DEBUG ((DEBUG_INFO, "      Digest(%d): ", DigestIndex));
    switch (HashAlgo) {
      case TPM_ALG_SHA1:
        DigestSize = SHA1_DIGEST_SIZE;
        break;
      case TPM_ALG_SHA256:
        DigestSize = SHA256_DIGEST_SIZE;
        break;
      case TPM_ALG_SHA384:
        DigestSize = SHA384_DIGEST_SIZE;
        break;
      case TPM_ALG_SHA512:
        DigestSize = SHA512_DIGEST_SIZE;
        break;
      default:
        DEBUG ((DEBUG_ERROR, "Unknown Hash Algorithm\n"));
        continue;
    }

    for (Index = 0; Index < DigestSize; Index++) {
      DEBUG ((DEBUG_INFO, "%02x ", DigestBuffer[Index]));
    }

    DEBUG ((DEBUG_INFO, "\n"));
    //
    // Prepare next
    //
    CopyMem (&HashAlgo, DigestBuffer + DigestSize, sizeof (TPMI_ALG_HASH));
    DigestBuffer = DigestBuffer + DigestSize + sizeof (TPMI_ALG_HASH);
  }

  DEBUG ((DEBUG_INFO, "\n"));
  DigestBuffer = DigestBuffer - sizeof (TPMI_ALG_HASH);

  CopyMem (&EventSize, DigestBuffer, sizeof (TcgPcrEvent2->EventSize));
  DEBUG ((DEBUG_INFO, "    EventSize - 0x%08x\n", EventSize));
  EventBuffer = DigestBuffer + sizeof (TcgPcrEvent2->EventSize);
  InternalDumpHex (EventBuffer, EventSize);
}

/**
  This function dump TCG_EfiSpecIDEventStruct.

  @param[in]  TcgEfiSpecIdEventStruct     A pointer to TCG_EfiSpecIDEventStruct.
**/
STATIC
VOID
DumpTcgEfiSpecIdEventStruct (
  IN TCG_EfiSpecIDEventStruct  *TcgEfiSpecIdEventStruct
  )
{
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINTN                            Index;
  UINT8                            *VendorInfoSize;
  UINT8                            *VendorInfo;
  UINT32                           NumberOfAlgorithms;

  DEBUG ((DEBUG_INFO, "  TCG_EfiSpecIDEventStruct:\n"));
  DEBUG ((DEBUG_INFO, "    signature          - '"));
  for (Index = 0; Index < sizeof (TcgEfiSpecIdEventStruct->signature); Index++) {
    DEBUG ((DEBUG_INFO, "%c", TcgEfiSpecIdEventStruct->signature[Index]));
  }

  DEBUG ((DEBUG_INFO, "'\n"));
  DEBUG ((DEBUG_INFO, "    platformClass      - 0x%08x\n", TcgEfiSpecIdEventStruct->platformClass));
  DEBUG ((DEBUG_INFO, "    specVersion        - %d.%d%d\n", TcgEfiSpecIdEventStruct->specVersionMajor, TcgEfiSpecIdEventStruct->specVersionMinor, TcgEfiSpecIdEventStruct->specErrata));
  DEBUG ((DEBUG_INFO, "    uintnSize          - 0x%02x\n", TcgEfiSpecIdEventStruct->uintnSize));

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));
  DEBUG ((DEBUG_INFO, "    NumberOfAlgorithms - 0x%08x\n", NumberOfAlgorithms));

  DigestSize = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));
  for (Index = 0; Index < NumberOfAlgorithms; Index++) {
    DEBUG ((DEBUG_INFO, "    digest(%d)\n", Index));
    DEBUG ((DEBUG_INFO, "      algorithmId      - 0x%04x\n", DigestSize[Index].algorithmId));
    DEBUG ((DEBUG_INFO, "      digestSize       - 0x%04x\n", DigestSize[Index].digestSize));
  }

  VendorInfoSize = (UINT8 *)&DigestSize[NumberOfAlgorithms];
  DEBUG ((DEBUG_INFO, "    VendorInfoSize     - 0x%02x\n", *VendorInfoSize));
  VendorInfo = VendorInfoSize + 1;
  DEBUG ((DEBUG_INFO, "    VendorInfo         - "));
  for (Index = 0; Index < *VendorInfoSize; Index++) {
    DEBUG ((DEBUG_INFO, "%02x ", VendorInfo[Index]));
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

/**
  This function gets the size of TCG_EfiSpecIDEventStruct. It is copied here as it is a simple calculation and to
  avoid library dependencies within TCG2 code.

  @param[in]  TcgEfiSpecIdEventStruct     A pointer to TCG_EfiSpecIDEventStruct.
**/
STATIC
UINTN
Tpm2DebugLibGetTcgEfiSpecIdEventStructSize (
  IN TCG_EfiSpecIDEventStruct  *TcgEfiSpecIdEventStruct
  )
{
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINT8                            *VendorInfoSize;
  UINT32                           NumberOfAlgorithms;

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));

  DigestSize     = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));
  VendorInfoSize = (UINT8 *)&DigestSize[NumberOfAlgorithms];
  return sizeof (TCG_EfiSpecIDEventStruct) + sizeof (UINT32) + (NumberOfAlgorithms * sizeof (TCG_EfiSpecIdEventAlgorithmSize)) + sizeof (UINT8) + (*VendorInfoSize);
}

/**
  This function returns size of TCG PCR event 2.

  @param[in]  TcgPcrEvent2     TCG PCR event 2 structure.

  @return size of TCG PCR event 2.
**/
STATIC
UINTN
GetPcrEvent2Size (
  IN TCG_PCR_EVENT2  *TcgPcrEvent2
  )
{
  UINT32         DigestIndex;
  UINT32         DigestCount;
  TPMI_ALG_HASH  HashAlgo;
  UINT32         DigestSize;
  UINT8          *DigestBuffer;
  UINT32         EventSize;
  UINT8          *EventBuffer;

  DigestCount  = TcgPcrEvent2->Digest.count;
  HashAlgo     = TcgPcrEvent2->Digest.digests[0].hashAlg;
  DigestBuffer = (UINT8 *)&TcgPcrEvent2->Digest.digests[0].digest;
  for (DigestIndex = 0; DigestIndex < DigestCount; DigestIndex++) {
    switch (HashAlgo) {
      case TPM_ALG_SHA1:
        DigestSize = SHA1_DIGEST_SIZE;
        break;
      case TPM_ALG_SHA256:
        DigestSize = SHA256_DIGEST_SIZE;
        break;
      case TPM_ALG_SHA384:
        DigestSize = SHA384_DIGEST_SIZE;
        break;
      case TPM_ALG_SHA512:
        DigestSize = SHA512_DIGEST_SIZE;
        break;
      default:
        DEBUG ((DEBUG_ERROR, "Unknown Hash Algorithm\n"));
        continue;
    }

    //
    // Prepare next
    //
    CopyMem (&HashAlgo, DigestBuffer + DigestSize, sizeof (TPMI_ALG_HASH));
    DigestBuffer = DigestBuffer + DigestSize + sizeof (TPMI_ALG_HASH);
  }

  DigestBuffer = DigestBuffer - sizeof (TPMI_ALG_HASH);

  CopyMem (&EventSize, DigestBuffer, sizeof (TcgPcrEvent2->EventSize));
  EventBuffer = DigestBuffer + sizeof (TcgPcrEvent2->EventSize);

  return (UINTN)EventBuffer + EventSize - (UINTN)TcgPcrEvent2;
}

/**
  This function dump event log.

  @param[in]  EventLogFormat     The type of the event log for which the information is requested.
  @param[in]  EventLogLocation   A pointer to the memory address of the event log.
  @param[in]  EventLogLastEntry  If the Event Log contains more than one entry, this is a pointer to the
                                 address of the start of the last entry in the event log in memory.
  @param[in]  FinalEventsTable   A pointer to the memory address of the final event table.
**/
VOID
EFIAPI
DumpEventLog (
  IN EFI_TCG2_EVENT_LOG_FORMAT    EventLogFormat,
  IN EFI_PHYSICAL_ADDRESS         EventLogLocation,
  IN EFI_PHYSICAL_ADDRESS         EventLogLastEntry,
  IN EFI_TCG2_FINAL_EVENTS_TABLE  *FinalEventsTable
  )
{
  TCG_PCR_EVENT_HDR         *EventHdr;
  TCG_PCR_EVENT2            *TcgPcrEvent2;
  TCG_EfiSpecIDEventStruct  *TcgEfiSpecIdEventStruct;
  UINT64                    NumberOfEvents;

  DEBUG ((DEBUG_INFO, "EventLogFormat: (0x%x)\n", EventLogFormat));

  switch (EventLogFormat) {
    case EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2:
      EventHdr = (TCG_PCR_EVENT_HDR *)(UINTN)EventLogLocation;
      while ((EFI_PHYSICAL_ADDRESS)(UINTN)EventHdr <= EventLogLastEntry) {
        DumpEvent (EventHdr);
        EventHdr = (TCG_PCR_EVENT_HDR *)((UINTN)EventHdr + sizeof (TCG_PCR_EVENT_HDR) + EventHdr->EventSize);
      }

      if (FinalEventsTable == NULL) {
        DEBUG ((DEBUG_INFO, "FinalEventsTable: NOT FOUND\n"));
      } else {
        DEBUG ((DEBUG_INFO, "FinalEventsTable:    (0x%x)\n", FinalEventsTable));
        DEBUG ((DEBUG_INFO, "  Version:           (0x%x)\n", FinalEventsTable->Version));
        DEBUG ((DEBUG_INFO, "  NumberOfEvents:    (0x%x)\n", FinalEventsTable->NumberOfEvents));

        EventHdr = (TCG_PCR_EVENT_HDR *)(UINTN)(FinalEventsTable + 1);
        for (NumberOfEvents = 0; NumberOfEvents < FinalEventsTable->NumberOfEvents; NumberOfEvents++) {
          DumpEvent (EventHdr);
          EventHdr = (TCG_PCR_EVENT_HDR *)((UINTN)EventHdr + sizeof (TCG_PCR_EVENT_HDR) + EventHdr->EventSize);
        }
      }

      break;
    case EFI_TCG2_EVENT_LOG_FORMAT_TCG_2:
      //
      // Dump first event
      //
      EventHdr = (TCG_PCR_EVENT_HDR *)(UINTN)EventLogLocation;
      DumpEvent (EventHdr);

      TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)(EventHdr + 1);
      DumpTcgEfiSpecIdEventStruct (TcgEfiSpecIdEventStruct);

      TcgPcrEvent2 = (TCG_PCR_EVENT2 *)((UINTN)TcgEfiSpecIdEventStruct + Tpm2DebugLibGetTcgEfiSpecIdEventStructSize (TcgEfiSpecIdEventStruct));
      while ((EFI_PHYSICAL_ADDRESS)(UINTN)TcgPcrEvent2 <= EventLogLastEntry) {
        DumpEvent2 (TcgPcrEvent2);
        TcgPcrEvent2 = (TCG_PCR_EVENT2 *)((UINTN)TcgPcrEvent2 + GetPcrEvent2Size (TcgPcrEvent2));
      }

      if (FinalEventsTable == NULL) {
        DEBUG ((DEBUG_INFO, "FinalEventsTable: NOT FOUND\n"));
      } else {
        DEBUG ((DEBUG_INFO, "FinalEventsTable:    (0x%x)\n", FinalEventsTable));
        DEBUG ((DEBUG_INFO, "  Version:           (0x%x)\n", FinalEventsTable->Version));
        DEBUG ((DEBUG_INFO, "  NumberOfEvents:    (0x%x)\n", FinalEventsTable->NumberOfEvents));

        TcgPcrEvent2 = (TCG_PCR_EVENT2 *)(UINTN)(FinalEventsTable + 1);
        for (NumberOfEvents = 0; NumberOfEvents < FinalEventsTable->NumberOfEvents; NumberOfEvents++) {
          DumpEvent2 (TcgPcrEvent2);
          TcgPcrEvent2 = (TCG_PCR_EVENT2 *)((UINTN)TcgPcrEvent2 + GetPcrEvent2Size (TcgPcrEvent2));
        }
      }

      break;
  }

  return;
}

/**
  This function dumps the provided PCR digest.

  @param[in]  PcrIndex   The index of the PCR.
  @param[in]  HashAlg    The hash algorithm used.
  @param[in]  PcrValues  The digest to be dumped.
**/
VOID
EFIAPI
DumpPcrDigest (
  IN UINT32         PcrIndex,
  IN TPMI_ALG_HASH  HashAlg,
  IN TPML_DIGEST    *PcrValues
  )
{
  UINTN  Index;
  UINTN  Index2;

  if (PcrValues == NULL) {
    DEBUG ((DEBUG_INFO, "DumpPcrDigest - PcrValues is NULL\n"));
    return;
  }

  for (Index = 0; Index < PcrValues->count; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "ReadPcr - HashAlg = 0x%04x, Pcr[%02d], digest = ",
      HashAlg,
      PcrIndex
      ));

    for (Index2 = 0; Index2 < PcrValues->digests[Index].size; Index2++) {
      if ((PcrValues->digests == NULL) || (PcrValues->digests[Index].buffer == NULL)) {
        DEBUG ((DEBUG_INFO, "NULL\n"));
      } else {
        DEBUG ((DEBUG_INFO, "%02x ", PcrValues->digests[Index].buffer[Index2]));
      }
    }

    DEBUG ((DEBUG_INFO, "\n"));
  }
}
