/** @file -- VariablePolicy.h

This protocol allows communication with Variable Policy Engine.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __EDKII_VARIABLE_POLICY_PROTOCOL__
#define __EDKII_VARIABLE_POLICY_PROTOCOL__

#define EDKII_VARIABLE_POLICY_PROTOCOL_REVISION  0x0000000000010000

#define EDKII_VARIABLE_POLICY_PROTOCOL_GUID \
  { \
    0x81D1675C, 0x86F6, 0x48DF, { 0xBD, 0x95, 0x9A, 0x6E, 0x4F, 0x09, 0x25, 0xC3 } \
  }

#define VARIABLE_POLICY_ENTRY_REVISION  0x00010000

#pragma pack(push, 1)
typedef struct {
  UINT32      Version;
  UINT16      Size;
  UINT16      OffsetToName;
  EFI_GUID    Namespace;
  UINT32      MinSize;
  UINT32      MaxSize;
  UINT32      AttributesMustHave;
  UINT32      AttributesCantHave;
  UINT8       LockPolicyType;
  UINT8       Padding[3];
  // UINT8    LockPolicy[];     // Variable Length Field
  // CHAR16   Name[]            // Variable Length Field
} VARIABLE_POLICY_ENTRY;

#define     VARIABLE_POLICY_NO_MIN_SIZE   0
#define     VARIABLE_POLICY_NO_MAX_SIZE   MAX_UINT32
#define     VARIABLE_POLICY_NO_MUST_ATTR  0
#define     VARIABLE_POLICY_NO_CANT_ATTR  0

#define     VARIABLE_POLICY_TYPE_NO_LOCK            0
#define     VARIABLE_POLICY_TYPE_LOCK_NOW           1
#define     VARIABLE_POLICY_TYPE_LOCK_ON_CREATE     2
#define     VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE  3

typedef struct {
  EFI_GUID    Namespace;
  UINT8       Value;
  UINT8       Padding;
  // CHAR16   Name[];           // Variable Length Field
} VARIABLE_LOCK_ON_VAR_STATE_POLICY;
#pragma pack(pop)

/**
  This API function disables the variable policy enforcement. If it's
  already been called once, will return EFI_ALREADY_STARTED.

  @retval     EFI_SUCCESS
  @retval     EFI_ALREADY_STARTED   Has already been called once this boot.
  @retval     EFI_WRITE_PROTECTED   Interface has been locked until reboot.
  @retval     EFI_WRITE_PROTECTED   Interface option is disabled by platform PCD.

**/
typedef
EFI_STATUS
(EFIAPI *DISABLE_VARIABLE_POLICY)(
  VOID
  );

/**
  This API function returns whether or not the policy engine is
  currently being enforced.

  @param[out]   State       Pointer to a return value for whether the policy enforcement
                            is currently enabled.

  @retval     EFI_SUCCESS
  @retval     Others        An error has prevented this command from completing.

**/
typedef
EFI_STATUS
(EFIAPI *IS_VARIABLE_POLICY_ENABLED)(
  OUT BOOLEAN *State
  );

/**
  This API function validates and registers a new policy with
  the policy enforcement engine.

  @param[in]  NewPolicy     Pointer to the incoming policy structure.

  @retval     EFI_SUCCESS
  @retval     EFI_INVALID_PARAMETER   NewPolicy is NULL or is internally inconsistent.
  @retval     EFI_ALREADY_STARTED     An identical matching policy already exists.
  @retval     EFI_WRITE_PROTECTED     The interface has been locked until the next reboot.
  @retval     EFI_ABORTED             A calculation error has prevented this function from completing.
  @retval     EFI_OUT_OF_RESOURCES    Cannot grow the table to hold any more policies.

**/
typedef
EFI_STATUS
(EFIAPI *REGISTER_VARIABLE_POLICY)(
  IN CONST VARIABLE_POLICY_ENTRY  *PolicyEntry
  );

/**
  This API function will dump the entire contents of the variable policy table.

  Similar to GetVariable, the first call can be made with a 0 size and it will return
  the size of the buffer required to hold the entire table.

  @param[out]     Policy  Pointer to the policy buffer. Can be NULL if Size is 0.
  @param[in,out]  Size    On input, the size of the output buffer. On output, the size
                          of the data returned.

  @retval     EFI_SUCCESS             Policy data is in the output buffer and Size has been updated.
  @retval     EFI_INVALID_PARAMETER   Size is NULL, or Size is non-zero and Policy is NULL.
  @retval     EFI_BUFFER_TOO_SMALL    Size is insufficient to hold policy. Size updated with required size.

**/
typedef
EFI_STATUS
(EFIAPI *DUMP_VARIABLE_POLICY)(
  IN OUT UINT8  *Policy,
  IN OUT UINT32 *Size
  );

/**
  This API function locks the interface so that no more policy updates
  can be performed or changes made to the enforcement until the next boot.

  @retval     EFI_SUCCESS
  @retval     Others        An error has prevented this command from completing.

**/
typedef
EFI_STATUS
(EFIAPI *LOCK_VARIABLE_POLICY)(
  VOID
  );

typedef struct {
  UINT64                        Revision;
  DISABLE_VARIABLE_POLICY       DisableVariablePolicy;
  IS_VARIABLE_POLICY_ENABLED    IsVariablePolicyEnabled;
  REGISTER_VARIABLE_POLICY      RegisterVariablePolicy;
  DUMP_VARIABLE_POLICY          DumpVariablePolicy;
  LOCK_VARIABLE_POLICY          LockVariablePolicy;
} _EDKII_VARIABLE_POLICY_PROTOCOL;

typedef _EDKII_VARIABLE_POLICY_PROTOCOL EDKII_VARIABLE_POLICY_PROTOCOL;

extern EFI_GUID  gEdkiiVariablePolicyProtocolGuid;

#endif
