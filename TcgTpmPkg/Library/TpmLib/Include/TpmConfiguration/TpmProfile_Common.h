/** @file
  This file is copied from
    - https://github.com/TrustedComputingGroup/TPM/blob/main/TPMCmd/TpmConfiguration/TpmConfiguration/TpmProfile_Common.h
  to set build option for TPM reference library.

  All option is the same to original file except ALG_SHA512.
  it is enabled to support ALG_SHA512 algorithm

**/

#pragma once

// YES & NO defined by TpmBuildSwitches.h
#if (YES != 1 || NO != 0)
#  error YES or NO incorrectly set
#endif
#if defined(ALG_YES) || defined(ALG_NO)
#  error ALG_YES and ALG_NO should only be defined by the TpmProfile_Common.h file
#endif

// Change these definitions to turn all algorithms ON or OFF. That is, to turn
// all algorithms on, set ALG_NO to YES. This is intended as a debug feature.
#define  ALG_YES                    YES
#define  ALG_NO                     NO

// Defines according to the processor being built for.
// Are building for a BIG_ENDIAN processor?
#define  BIG_ENDIAN_TPM             NO
#define  LITTLE_ENDIAN_TPM          !BIG_ENDIAN_TPM
// Does processor support Auto align?
#define  AUTO_ALIGN                 NO

//***********************************************
// Defines for Symmetric Algorithms
//***********************************************

#define ALG_AES                     ALG_YES

#define     AES_128                     (YES * ALG_AES)
#define     AES_192                     (NO  * ALG_AES)
#define     AES_256                     (YES * ALG_AES)

#define ALG_SM4                     ALG_NO

#define     SM4_128                     (NO  * ALG_SM4)

#define ALG_CAMELLIA                ALG_YES

#define     CAMELLIA_128                (YES * ALG_CAMELLIA)
#define     CAMELLIA_192                (NO  * ALG_CAMELLIA)
#define     CAMELLIA_256                (YES * ALG_CAMELLIA)

// must be yes if any above are yes.
#define ALG_SYMCIPHER               (ALG_AES || ALG_SM4 || ALG_CAMELLIA)
#define ALG_CMAC                    (YES * ALG_SYMCIPHER)

// block cipher modes
#define ALG_CTR                     ALG_YES
#define ALG_OFB                     ALG_YES
#define ALG_CBC                     ALG_YES
#define ALG_CFB                     ALG_YES
#define ALG_ECB                     ALG_YES

//***********************************************
// Defines for RSA Asymmetric Algorithms
//***********************************************
#define ALG_RSA                     ALG_YES
#define     RSA_1024                        (YES * ALG_RSA)
#define     RSA_2048                        (YES * ALG_RSA)
#define     RSA_3072                        (YES * ALG_RSA)
#define     RSA_4096                        (YES * ALG_RSA)
#define     RSA_16384                       (NO  * ALG_RSA)

#define     ALG_RSASSA                      (YES * ALG_RSA)
#define     ALG_RSAES                       (YES * ALG_RSA)
#define     ALG_RSAPSS                      (YES * ALG_RSA)
#define     ALG_OAEP                        (YES * ALG_RSA)

// RSA Implementation Styles
// use Chinese Remainder Theorem (5 prime) format for private key ?
#define CRT_FORMAT_RSA                  YES
#define RSA_DEFAULT_PUBLIC_EXPONENT     0x00010001

//***********************************************
// Defines for ECC Asymmetric Algorithms
//***********************************************
#define ALG_ECC                     ALG_YES
#define     ALG_ECDH                        (YES * ALG_ECC)
#define     ALG_ECDSA                       (YES * ALG_ECC)
#define     ALG_ECDAA                       (YES * ALG_ECC)
#define     ALG_SM2                         (YES * ALG_ECC)
#define     ALG_ECSCHNORR                   (YES * ALG_ECC)
#define     ALG_ECMQV                       (YES * ALG_ECC)
#define     ALG_KDF1_SP800_56A              (YES * ALG_ECC)
#define     ALG_EDDSA                       (NO  * ALG_ECC)
#define     ALG_EDDSA_PH                    (NO  * ALG_ECC)

#define     ECC_NIST_P192                   (YES * ALG_ECC)
#define     ECC_NIST_P224                   (YES * ALG_ECC)
#define     ECC_NIST_P256                   (YES * ALG_ECC)
#define     ECC_NIST_P384                   (YES * ALG_ECC)
#define     ECC_NIST_P521                   (YES * ALG_ECC)
#define     ECC_BN_P256                     (YES * ALG_ECC)
#define     ECC_BN_P638                     (YES * ALG_ECC)
#define     ECC_SM2_P256                    (YES * ALG_ECC)

#define     ECC_BP_P256_R1                  (NO * ALG_ECC)
#define     ECC_BP_P384_R1                  (NO * ALG_ECC)
#define     ECC_BP_P512_R1                  (NO * ALG_ECC)
#define     ECC_CURVE_25519                 (NO * ALG_ECC)
#define     ECC_CURVE_448                   (NO * ALG_ECC)

//***********************************************
// Defines for Hash/XOF Algorithms
//***********************************************
#define ALG_MGF1                            ALG_YES
#define ALG_SHA1                            ALG_YES
#define ALG_SHA256                          ALG_YES
#define ALG_SHA256_192                      ALG_NO
#define ALG_SHA384                          ALG_YES
#define ALG_SHA512                          ALG_YES

#define ALG_SHA3_256                        ALG_NO
#define ALG_SHA3_384                        ALG_NO
#define ALG_SHA3_512                        ALG_NO

#define ALG_SM3_256                         ALG_NO

#define ALG_SHAKE256_192                    ALG_NO
#define ALG_SHAKE256_256                    ALG_NO
#define ALG_SHAKE256_512                    ALG_NO

//***********************************************
// Defines for Stateful Signature Algorithms
//***********************************************
#define ALG_LMS                             ALG_NO
#define ALG_XMSS                            ALG_NO

//***********************************************
// Defines for Keyed Hashes
//***********************************************
#define ALG_KEYEDHASH                       ALG_YES
#define ALG_HMAC                            ALG_YES

//***********************************************
// Defines for KDFs
//***********************************************
#define ALG_KDF2                            ALG_YES
#define ALG_KDF1_SP800_108                  ALG_YES

//***********************************************
// Defines for Obscuration/MISC/compatibility
//***********************************************
#define ALG_XOR                             ALG_YES

//***********************************************
// Defines controlling ACT
//***********************************************
#define ACT_SUPPORT                         YES
#define RH_ACT_0                                (YES * ACT_SUPPORT)
#define RH_ACT_1                                ( NO * ACT_SUPPORT)
#define RH_ACT_2                                ( NO * ACT_SUPPORT)
#define RH_ACT_3                                ( NO * ACT_SUPPORT)
#define RH_ACT_4                                ( NO * ACT_SUPPORT)
#define RH_ACT_5                                ( NO * ACT_SUPPORT)
#define RH_ACT_6                                ( NO * ACT_SUPPORT)
#define RH_ACT_7                                ( NO * ACT_SUPPORT)
#define RH_ACT_8                                ( NO * ACT_SUPPORT)
#define RH_ACT_9                                ( NO * ACT_SUPPORT)
#define RH_ACT_A                                (YES * ACT_SUPPORT)
#define RH_ACT_B                                ( NO * ACT_SUPPORT)
#define RH_ACT_C                                ( NO * ACT_SUPPORT)
#define RH_ACT_D                                ( NO * ACT_SUPPORT)
#define RH_ACT_E                                ( NO * ACT_SUPPORT)
#define RH_ACT_F                                ( NO * ACT_SUPPORT)

// number of vendor properties, must currently be 1.
#define MAX_VENDOR_PROPERTY                 (1)

//***********************************************
// Enable VENDOR_PERMANENT_AUTH_HANDLE?
//***********************************************
#define VENDOR_PERMANENT_AUTH_ENABLED       NO
// if YES, this must be valid per Part2 (TPM_RH_AUTH_00 - TPM_RH_AUTH_FF)
// if NO, this must be #undef
#undef  VENDOR_PERMANENT_AUTH_HANDLE

//***********************************************
// Defines controlling optional implementation
//***********************************************
#define FIELD_UPGRADE_IMPLEMENTED           NO

//***********************************************
// Buffer Sizes based on implementation
//***********************************************
// When using PC CRB, the page size for both commands and
// control registers is 4k.  The command buffer starts at
// offset 0x80, so the net size available is:
#define  MAX_COMMAND_SIZE               (4096-0x80)
#define  MAX_RESPONSE_SIZE              (4096-0x80)

//***********************************************
// Vendor Info
//***********************************************
// max buffer for vendor commands
// Max data buffer leaving space for TPM2B size prefix
#define VENDOR_COMMAND_COUNT          0
#define MAX_VENDOR_BUFFER_SIZE         (MAX_RESPONSE_SIZE-2)
#define PRIVATE_VENDOR_SPECIFIC_BYTES RSA_PRIVATE_SIZE

//***********************************************
// Defines controlling Firmware- and SVN-limited objects
//***********************************************
#define FW_LIMITED_SUPPORT                    YES
#define SVN_LIMITED_SUPPORT                   YES

//***********************************************
// Defines controlling External NV
//***********************************************
// This is a software reference implementation of the TPM: there is no
// "external NV" as such. This #define configures the TPM to implement
// "external NV" that is stored in the same place as "internal NV."
// NOTE: enabling this doesn't necessarily mean that the expanded
// (external-NV-specific) attributes are supported.
#define EXTERNAL_NV                           YES

//***********************************************
// Defines controlling secure channel functionality
//***********************************************
// This flag enables support for PolicyTransportSPDM.
// See CC_PolicyTransportSPDM.
#define SEC_CHANNEL_SUPPORT                   YES
