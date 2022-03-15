/** @file
  Definitions shared among different implementation of ProtectedVariableLib.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PROTECTED_VARIABLE_INTERNAL_H_
#define _PROTECTED_VARIABLE_INTERNAL_H_

#include <Guid/VariableFormat.h>
#include <Guid/ProtectedVariable.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/RpmcLib.h>
#include <Library/VariableKeyLib.h>
#include <Library/EncryptionVariableLib.h>
#include <Library/ProtectedVariableLib.h>
#include <Library/HashApiLib.h>

#define VARIABLE_KEY_SIZE                 (256/8)

#define METADATA_HMAC_SIZE                (256/8)
#define METADATA_HMAC_KEY_NAME            L"HMAC_KEY"
#define METADATA_HMAC_KEY_NAME_SIZE       0x10

#define METADATA_HMAC_SEP                 L":"
#define METADATA_HMAC_SEP_SIZE            2

#define METADATA_HMAC_VARIABLE_NAME       L"MetaDataHmacVar"
#define METADATA_HMAC_VARIABLE_NAME_SIZE  sizeof (METADATA_HMAC_VARIABLE_NAME)
#define METADATA_HMAC_VARIABLE_GUID       gEdkiiMetaDataHmacVariableGuid
#define METADATA_HMAC_VARIABLE_ATTR       VARIABLE_ATTRIBUTE_NV_BS_RT

#define DIGEST_CONTEXT_SIZE               (HashApiGetContextSize())

#define MAX_VARIABLE_SIZE                                                       \
  MAX (MAX (PcdGet32 (PcdMaxVariableSize), PcdGet32 (PcdMaxAuthVariableSize)),  \
       PcdGet32 (PcdMaxHardwareErrorVariableSize))

#define IS_VARIABLE(Var, Name, Guid)          \
  (StrCmp ((Var)->VariableName, (Name)) == 0  \
   && CompareGuid ((CONST EFI_GUID *)(Var)->VendorGuid, (CONST EFI_GUID *)(Guid)))

#define VARIABLE_SIZE(VarInfo)                                  \
  (((UINTN)(VarInfo)->Header.Data - (UINTN)(VarInfo)->Buffer)   \
   + (VarInfo)->Header.DataSize                                 \
   + GET_PAD_SIZE ((VarInfo)->Header.DataSize))

#define VARIABLE_HEADER_SIZE(AuthFlag)                    \
  ((AuthFlag) ? sizeof (AUTHENTICATED_VARIABLE_HEADER)    \
              : sizeof (VARIABLE_HEADER))

#define VARIABLE_NAME(Var, AuthFlag)                    \
  ((CHAR16 *)((UINTN)(Var) + VARIABLE_HEADER_SIZE(AuthFlag)))

#define VARIABLE_START(VarStore)  \
   ((VARIABLE_HEADER *)HEADER_ALIGN ((VARIABLE_STORE_HEADER *)(VarStore) + 1))

#define VARIABLE_END(VarStore)                         \
   ((VARIABLE_HEADER *)HEADER_ALIGN ((UINTN)(VarStore) \
     + ((VARIABLE_STORE_HEADER *)(VarStore))->Size))

#define SET_VARIABLE_DATA_SIZE(VarInfo, Size)                                 \
  if ((VarInfo)->Flags.Auth) {                                                \
    ((AUTHENTICATED_VARIABLE_HEADER *)((VarInfo)->Buffer))->DataSize = Size;  \
    (VarInfo)->Header.DataSize = Size;                                        \
  } else {                                                                    \
    ((VARIABLE_HEADER *)((VarInfo)->Buffer))->DataSize = Size;                \
      (VarInfo)->Header.DataSize = Size;                                      \
  }

#define IS_KNOWN_UNPROTECTED_VARIABLE(Global, VarInfo)  \
  (CheckKnownUnprotectedVariable ((Global), (VarInfo)) < UnprotectedVarIndexMax)

#define GET_CNTX(Global)    ((PROTECTED_VARIABLE_CONTEXT_IN *)(UINTN)((Global)->ContextIn))
#define GET_BUFR(Address)   ((VOID *)(UINTN)(Address))
#define GET_ADRS(Buffer)    ((EFI_PHYSICAL_ADDRESS)(UINTN)(Buffer))

typedef struct _VARIABLE_IDENTIFIER {
  CHAR16            *VariableName;
  EFI_GUID          *VendorGuid;
  UINT8             State;
} VARIABLE_IDENTIFIER;

typedef enum {
  IndexHmacInDel = 0,     /// MetaDataHmacVar with state VAR_IN_DELETED_TRANSITION
  IndexHmacAdded,         /// MetaDataHmacVar with state VAR_ADDED
  IndexErrorFlag,         /// VarErrorFlag
  IndexPlatformVar,       /// Platform Variable
  UnprotectedVarIndexMax
} UNPROTECTED_VARIABLE_INDEX;

#pragma pack(1)

#define PROTECTED_VARIABLE_CONTEXT_OUT_STRUCT_VERSION 0x02

typedef struct _PROTECTED_VARIABLE_GLOBAL {
  UINT32                                StructVersion;
  UINT32                                StructSize;

  ///
  /// Variable root key used to derive Encryption key and HMAC key.
  ///
  UINT8                                 RootKey[VARIABLE_KEY_SIZE];
  ///
  /// HMAC key derived from RootKey.
  ///
  UINT8                                 MetaDataHmacKey[VARIABLE_KEY_SIZE];
  ///
  /// Number of variables in linked list pointed by VariableDigests.
  ///
  UINT32                                VariableNumber;
  ///
  /// Size of memory reserved by VariableCache.
  ///
  UINT32                                VariableCacheSize;
  ///
  /// Memory reserved to temporarily hold data of one variable, for integrity
  /// validation purpose.
  ///
  EFI_PHYSICAL_ADDRESS                  VariableCache;
  ///
  /// Pointer to linked list, in which each node holds the digest value of each
  /// variable.
  ///
  EFI_PHYSICAL_ADDRESS                  VariableDigests;
  ///
  /// Memory reserved for Context used in hash API to avoid repeat alloc/free.
  ///
  EFI_PHYSICAL_ADDRESS                  DigestContext;
  ///
  /// Pointer to one of node in linked list pointed by VariableDigests, which
  /// has been just accessed. This is mainly used to facilitate the two calls
  /// use case of GetVariable().
  ///
  EFI_PHYSICAL_ADDRESS                  LastAccessedVariable;
  ///
  /// Cached copy of pointers to nodes of unprotected variables in the linked
  /// list pointed by VariableDigests.
  ///
  EFI_PHYSICAL_ADDRESS                  Unprotected[UnprotectedVarIndexMax];
  ///
  /// Pointer to data structure holding helper functions passed by user of
  /// ProtectedVariableLib, most of which are used to complete operations on
  /// variable storage.
  ///
  EFI_PHYSICAL_ADDRESS                  ContextIn;

  ///
  /// Pointer to Global data structure. This is to hold pre-mem address value.
  /// Later to be used to identify pre-mem to post-mem transition.
  ///
  EFI_PHYSICAL_ADDRESS                  GlobalSelf;

  struct {
    BOOLEAN                               Auth;         /// Authenticated variable format
    BOOLEAN                               WriteInit;    /// Write-init-done
    BOOLEAN                               WriteReady;   /// Ready-to-write
    BOOLEAN                               RecoveryMode; /// Variable storage recovery or provisioning
    BOOLEAN                               CacheReady;   /// Indicates Cache is available
    BOOLEAN                               Reserved;
  }                                     Flags;
} PROTECTED_VARIABLE_GLOBAL;

#pragma pack()

/* Sort method function pointer taking two parameters */
typedef
INTN
(*SORT_METHOD) (
  IN VARIABLE_DIGEST *Variable1,
  IN VARIABLE_DIGEST *Variable2
  );

/* Update variable digest data function pointer */
typedef
BOOLEAN
(*DIGEST_UPDATE) (
  IN OUT  VOID        *Context,
  IN      VOID        *Data,
  IN      UINTN       DataSize
  );

/**

  Print variable information

  @param[in]   Data8      Pointer to data
  @param[out]  DataSize   Size of data

**/
VOID
PrintVariableData (
  IN UINT8   *Data8,
  IN UINTN   DataSize
  );

/**

  Derive HMAC key from given variable root key.

  @param[in]  RootKey       Pointer to root key to derive from.
  @param[in]  RootKeySize   Size of root key.
  @param[out] HmacKey       Pointer to generated HMAC key.
  @param[in]  HmacKeySize   Size of HMAC key.

  @retval TRUE      The HMAC key is derived successfully.
  @retval FALSE     Failed to generate HMAC key from given root key.

**/
BOOLEAN
GenerateMetaDataHmacKey (
  IN   CONST UINT8  *RootKey,
  IN   UINTN        RootKeySize,
  OUT  UINT8        *HmacKey,
  IN   UINTN        HmacKeySize
  );

/**

  Digests the given variable data and updates HMAC context.

  @param[in,out]  Context   Pointer to initialized HMAC context.
  @param[in]      VarInfo   Pointer to variable data.

  @retval TRUE    HMAC context was updated successfully.
  @retval FALSE   Failed to update HMAC context.

**/
BOOLEAN
UpdateVariableMetadataHmac (
  IN  VOID                      *Context,
  IN  PROTECTED_VARIABLE_INFO   *VarInfo
  );

/**

  Retrieve the HMAC variable

  @param[in]     Global    Pointer to global configuration data.
  @param[in]     VarInfo   Pointer to variable information data.
  @param[in,out] HmacData  Pointer to a buffer where the
                           HMAC-SHA256 digest value is located.

  @retval TRUE    HMAC context was updated successfully.
  @retval FALSE   Failed to update HMAC context.

**/
STATIC
EFI_STATUS
GetVariableHmacInternal (
  IN      PROTECTED_VARIABLE_GLOBAL       *Global,
  IN      PROTECTED_VARIABLE_INFO         *VarInfo,
  IN  OUT UINT8                           *HmacData
  );
  
/**

  Re-calculate HMAC based on new variable data and re-generate MetaDataHmacVar.

  @param[in]      Global          Pointer to global configuration data.
  @param[in]      NewVarInfo      Pointer to buffer of new variable data.
  @param[in,out]  NewHmacVarInfo  Pointer to buffer of new MetaDataHmacVar.

  @return EFI_SUCCESS           The HMAC value was updated successfully.
  @return EFI_ABORTED           Failed to calculate the HMAC value.
  @return EFI_OUT_OF_RESOURCES  Not enough resource to calculate HMC value.
  @return EFI_NOT_FOUND         The MetaDataHmacVar was not found in storage.

**/
EFI_STATUS
RefreshVariableMetadataHmac (
  IN      PROTECTED_VARIABLE_GLOBAL         *Global,
  IN      PROTECTED_VARIABLE_INFO           *NewVarInfo,
  IN  OUT PROTECTED_VARIABLE_INFO           *NewHmacVarInfo
  );

/**

  Retrieve the context and global configuration data structure from HOB.

  Once protected NV variable storage is cached and verified in PEI phase,
  all related information are stored in a HOB which can be used by PEI variable
  service itself and passed to SMM along with the boot flow, which can avoid
  many duplicate works, like generating HMAC key, verifying NV variable storage,
  etc.

  The HOB can be identified by gEdkiiProtectedVariableGlobalGuid.

  @param[out]   Global      Pointer to global configuration data from PEI phase.

  @retval EFI_SUCCESS     The HOB was found, and Context and Global are retrieved.
  @retval EFI_NOT_FOUND   The HOB was not found.

**/
EFI_STATUS
GetProtectedVariableGlobalFromHob (
  OUT PROTECTED_VARIABLE_GLOBAL       **Global OPTIONAL
  );

/**

  Get context and/or global data structure used to process protected variable.

  @param[out]   Global      Pointer to global configuration data.

  @retval EFI_SUCCESS         Get requested structure successfully.

**/
EFI_STATUS
GetProtectedVariableGlobal (
  OUT PROTECTED_VARIABLE_GLOBAL       **Global OPTIONAL
  );

/**

  Get context data structure used to process protected variable.

  @param[out]   ContextIn   Pointer to context provided by variable runtime services.

  @retval EFI_SUCCESS         Get requested structure successfully.

**/
EFI_STATUS
EFIAPI
GetProtectedVariableContext (
  PROTECTED_VARIABLE_CONTEXT_IN       **ContextIn  OPTIONAL
  );

/**

  Check if a given variable is unprotected variable specified in advance
  and return its index ID.

  @param[in] Global     Pointer to global configuration data.
  @param[in] VarInfo    Pointer to variable information data.

  @retval IndexHmacInDel    Variable is MetaDataHmacVar in delete-transition state.
  @retval IndexHmacAdded    Variable is MetaDataHmacVar in valid state.
  @retval IndexErrorFlag    Variable is VarErrorLog.
  @retval Others            Variable is not any known unprotected variables.

**/
UNPROTECTED_VARIABLE_INDEX
CheckKnownUnprotectedVariable (
  IN  PROTECTED_VARIABLE_GLOBAL     *Global,
  IN  PROTECTED_VARIABLE_INFO       *VarInfo
  );

/**

  Return the size of variable MetaDataHmacVar.

  @param[in] AuthFlag         Auth-variable indicator.

  @retval size of variable MetaDataHmacVar.

**/
UINTN
GetMetaDataHmacVarSize (
  IN      BOOLEAN     AuthFlag
  );

/**

  Fix state of MetaDataHmacVar on NV variable storage, if there's failure at
  last boot during updating variable.

  This must be done before the first writing of variable in current boot,
  including storage reclaim.

  @retval EFI_UNSUPPORTED        Updating NV variable storage is not supported.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource to complete the operation.
  @retval EFI_SUCCESS            Variable store was successfully updated.

**/
EFI_STATUS
FixupHmacVariable (
  VOID
  );

/**

  Verify the variable digest.

  @param[in]  Global      Pointer to global configuration data.
  @param[in]  VarInfo     Pointer to verified copy of protected variables.
  @param[in]  VarDig      Pointer to variable digest data.

  @retval EFI_INVALID_PARAMETER  Invalid parameter was passed in.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource to calculate hash.
  @retval EFI_ABORTED            An error was encountered.
  @retval EFI_COMPROMISED_DATA   The data was compromised.
  @retval EFI_SUCCESS            Variable digest was successfully verified.

**/
EFI_STATUS
VerifyVariableDigest (
  IN  PROTECTED_VARIABLE_GLOBAL       *Global,
  IN  PROTECTED_VARIABLE_INFO         *VarInfo,
  IN  VARIABLE_DIGEST                 *VarDig
  );

/**

  Get the variable digest.

  @param[in]      Global        Pointer to global configuration data.
  @param[in]      VarInfo       Pointer to verified copy of protected variables.
  @param[in,out]  DigestValue   Pointer to variable digest value.

  @retval EFI_INVALID_PARAMETER  Invalid parameter was passed in.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource to calculate hash.
  @retval EFI_ABORTED            An error was encountered.
  @retval EFI_COMPROMISED_DATA   The data was compromised.
  @retval EFI_SUCCESS            Variable digest was successfully verified.

**/
EFI_STATUS
GetVariableDigest (
  IN      PROTECTED_VARIABLE_GLOBAL       *Global,
  IN      PROTECTED_VARIABLE_INFO         *VarInfo,
  IN  OUT UINT8                           *DigestValue
  );

/**

  Compare variable name and Guid

  @param[in]  Name1      Name of first variable.
  @param[in]  Name1Size  Size of first variable.
  @param[in]  Name2      Name of second variable.
  @param[in]  Name2Size  Size of second variable.
  @param[in]  Guid1      Guid for first variable.
  @param[in]  Guid2      Guid for second variable.

  @retval 0         First name is identical to Second name.
  @return others    First name is not identical to Second name.

**/
INTN
CompareVariableNameAndGuid (
  IN CONST CHAR16           *Name1,
  IN UINTN                  Name1Size,
  IN CONST CHAR16           *Name2,
  IN UINTN                  Name2Size,
  IN EFI_GUID               *Guid1,
  IN EFI_GUID               *Guid2
  );

/**

  Compare variable digest.

  @param[in]  Variable1     Pointer to first variable digest.
  @param[in]  Variable2     Pointer to second variable digest.

  @retval 0         Variables are identical.
  @return others    Variables are not identical.

**/
INTN
CompareVariableDigestInfo (
  IN  VARIABLE_DIGEST     *Variable1,
  IN  VARIABLE_DIGEST     *Variable2
  );

/**

  Move a node backward in the order controlled by SortMethod.

  @param[in]  Node          Pointer to node to be moved.
  @param[in]  SortMethod    Method used to compare node in list.

**/
VOID
MoveNodeBackward (
  IN  OUT VARIABLE_DIGEST  *Node,
  IN  SORT_METHOD          SortMethod
  );

/**

  Remove variable digest node.

  @param[in,out]  Global        Pointer to global configuration data.
  @param[in,out]  VarDig        Pointer to variable digest value.
  @param[in]      FreeResource  Flag to indicate whether to free resource.

**/
VOID
RemoveVariableDigestNode (
  IN  OUT PROTECTED_VARIABLE_GLOBAL     *Global,
  IN  OUT VARIABLE_DIGEST               *VarDig,
  IN      BOOLEAN                       FreeResource
  );

/**

  Insert variable digest node.

  @param[in,out]  Global        Pointer to global configuration data.
  @param[in]      VarDig        Pointer to variable digest value.
  @param[in]      SortMethod    Method for sorting.

**/
VOID
InsertVariableDigestNode (
  IN  OUT PROTECTED_VARIABLE_GLOBAL     *Global,
  IN      VARIABLE_DIGEST               *VarDig,
  IN      SORT_METHOD                   SortMethod
  );

/**

  Create variable digest node.

  @param[in]  VariableName      Name of variable.
  @param[in]  VendorGuid        Guid of variable.
  @param[in]  NameSize          Size of variable name.
  @param[in]  DataSize          Size of variable data.
  @param[in]  AuthVar           Authenticated variable flag.
  @param[in]  Global            Pointer to global configuration data.

  @retval Ptr   Pointer to variable digest

**/
VARIABLE_DIGEST*
CreateVariableDigestNode (
  IN CHAR16                       *VariableName,
  IN EFI_GUID                     *VendorGuid,
  IN UINT16                       NameSize,
  IN UINT32                       DataSize,
  IN BOOLEAN                      AuthVar,
  IN PROTECTED_VARIABLE_GLOBAL    *Global
  );

/**

  This function is used to enumerate the variables managed by current
  ProtectedVariableLib.

  If the VarInfo->StoreIndex is invalid (VAR_INDEX_INVALID), the first variable
  with the smallest StoreIndex will be returned. Otherwise, the variable with
  StoreIndex just after than the VarInfo->StoreIndex will be returned.

  @param[in,out]      VarInfo     Pointer to structure containing variable information.

  @retval EFI_SUCCESS               Found the specified variable.
  @retval EFI_INVALID_PARAMETER     VarInfo is NULL.
  @retval EFI_NOT_FOUND             The specified variable could not be found.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetNextInternal (
  IN  OUT PROTECTED_VARIABLE_INFO       *VarInfo
  );

/**

  Find the specified variable digest

  @param[in]  Global        Pointer to global configuration data.
  @param[in]  VarInfo       Pointer to variable data.
  @param[in]  FindNext      Flag to continue looking for variable.

**/
VARIABLE_DIGEST*
FindVariableInternal (
  IN PROTECTED_VARIABLE_GLOBAL       *Global,
  IN PROTECTED_VARIABLE_INFO         *VarInfo,
  IN BOOLEAN                         FindNext
  );

/**

  Synchronize the RPMC counters

  @param[in]  Global      Pointer to global configuration data.
  @param[in]  VarInfo     Pointer to variable data.
  @param[in]  FindNext    Flag to continue looking for variable.
  
  @retval EFI_SUCCESS     Successfully sync RPMC counters.
  @return others          Failed to sync RPMC counters.

**/
EFI_STATUS
SyncRpmcCounter (
  VOID
  );

/**

  Perform for protected variable integrity check.

  If this initialization failed upon any error, the whole variable services
  should not be used.  A system reset might be needed to re-construct NV
  variable storage to be the default state.

  @param[in]  ContextIn   Pointer to variable service context needed by
                          protected variable.

  @retval EFI_SUCCESS               Protected variable services are ready.
  @retval EFI_INVALID_PARAMETER     If ContextIn == NULL or something missing or
                                    mismatching in the content in ContextIn.
  @retval EFI_COMPROMISED_DATA      If failed to check integrity of protected variables.
  @retval EFI_OUT_OF_RESOURCES      Fail to allocate enough resource.
  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
PerformVariableIntegrityCheck (
  IN  PROTECTED_VARIABLE_CONTEXT_IN   *ContextIn
  );

extern EFI_TIME                       mDefaultTimeStamp;
extern VARIABLE_IDENTIFIER            mUnprotectedVariables[UnprotectedVarIndexMax];
extern PROTECTED_VARIABLE_CONTEXT_IN  mVariableContextIn;
extern PROTECTED_VARIABLE_GLOBAL      mProtectedVariableGlobal;

#endif
