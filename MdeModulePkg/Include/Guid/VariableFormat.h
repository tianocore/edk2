/** @file
  The variable data structures are related to EDK II-specific implementation of UEFI variables.
  VariableFormat.h defines variable data headers and variable storage region headers.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VARIABLE_FORMAT_H__
#define __VARIABLE_FORMAT_H__

#define EFI_VARIABLE_GUID \
  { 0xddcf3616, 0x3275, 0x4164, { 0x98, 0xb6, 0xfe, 0x85, 0x70, 0x7f, 0xfe, 0x7d } }

#define EFI_AUTHENTICATED_VARIABLE_GUID \
  { 0xaaf32c78, 0x947b, 0x439a, { 0xa1, 0x80, 0x2e, 0x14, 0x4e, 0xc3, 0x77, 0x92 } }

extern EFI_GUID gEfiVariableGuid;
extern EFI_GUID gEfiAuthenticatedVariableGuid;

///
/// Alignment of variable name and data, according to the architecture:
/// * For IA-32 and Intel(R) 64 architectures: 1.
///
#define ALIGNMENT         1

//
// GET_PAD_SIZE calculates the miminal pad bytes needed to make the current pad size satisfy the alignment requirement.
//
#if (ALIGNMENT == 1)
#define GET_PAD_SIZE(a) (0)
#else
#define GET_PAD_SIZE(a) (((~a) + 1) & (ALIGNMENT - 1))
#endif

///
/// Alignment of Variable Data Header in Variable Store region.
///
#define HEADER_ALIGNMENT  4
#define HEADER_ALIGN(Header)  (((UINTN) (Header) + HEADER_ALIGNMENT - 1) & (~(HEADER_ALIGNMENT - 1)))

///
/// Status of Variable Store Region.
///
typedef enum {
  EfiRaw,
  EfiValid,
  EfiInvalid,
  EfiUnknown
} VARIABLE_STORE_STATUS;

#pragma pack(1)

#define VARIABLE_STORE_SIGNATURE  EFI_VARIABLE_GUID
#define AUTHENTICATED_VARIABLE_STORE_SIGNATURE  EFI_AUTHENTICATED_VARIABLE_GUID

///
/// Variable Store Header Format and State.
///
#define VARIABLE_STORE_FORMATTED          0x5a
#define VARIABLE_STORE_HEALTHY            0xfe

///
/// Variable Store region header.
///
typedef struct {
  ///
  /// Variable store region signature.
  ///
  EFI_GUID  Signature;
  ///
  /// Size of entire variable store,
  /// including size of variable store header but not including the size of FvHeader.
  ///
  UINT32  Size;
  ///
  /// Variable region format state.
  ///
  UINT8   Format;
  ///
  /// Variable region healthy state.
  ///
  UINT8   State;
  UINT16  Reserved;
  UINT32  Reserved1;
} VARIABLE_STORE_HEADER;

///
/// Variable data start flag.
///
#define VARIABLE_DATA                     0x55AA

///
/// Variable State flags.
///
#define VAR_IN_DELETED_TRANSITION     0xfe  ///< Variable is in obsolete transition.
#define VAR_DELETED                   0xfd  ///< Variable is obsolete.
#define VAR_HEADER_VALID_ONLY         0x7f  ///< Variable header has been valid.
#define VAR_ADDED                     0x3f  ///< Variable has been completely added.

///
/// Variable Attribute combinations.
///
#define VARIABLE_ATTRIBUTE_NV_BS        (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)
#define VARIABLE_ATTRIBUTE_BS_RT        (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)
#define VARIABLE_ATTRIBUTE_BS_RT_AT     (VARIABLE_ATTRIBUTE_BS_RT | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
#define VARIABLE_ATTRIBUTE_NV_BS_RT     (VARIABLE_ATTRIBUTE_BS_RT | EFI_VARIABLE_NON_VOLATILE)
#define VARIABLE_ATTRIBUTE_NV_BS_RT_HR  (VARIABLE_ATTRIBUTE_NV_BS_RT | EFI_VARIABLE_HARDWARE_ERROR_RECORD)
#define VARIABLE_ATTRIBUTE_NV_BS_RT_AT  (VARIABLE_ATTRIBUTE_NV_BS_RT | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
#define VARIABLE_ATTRIBUTE_AT           EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS
#define VARIABLE_ATTRIBUTE_NV_BS_RT_HR_AT    (VARIABLE_ATTRIBUTE_NV_BS_RT_HR | VARIABLE_ATTRIBUTE_AT)
///
/// EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is deprecated and should be considered as reserved
///
#define VARIABLE_ATTRIBUTE_AT_AW        (EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
#define VARIABLE_ATTRIBUTE_NV_BS_RT_AW  (VARIABLE_ATTRIBUTE_NV_BS_RT | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
#define VARIABLE_ATTRIBUTE_NV_BS_RT_HR_AT_AW    (VARIABLE_ATTRIBUTE_NV_BS_RT_HR | VARIABLE_ATTRIBUTE_AT_AW)

///
/// Single Variable Data Header Structure.
///
typedef struct {
  ///
  /// Variable Data Start Flag.
  ///
  UINT16      StartId;
  ///
  /// Variable State defined above.
  ///
  UINT8       State;
  UINT8       Reserved;
  ///
  /// Attributes of variable defined in UEFI specification.
  ///
  UINT32      Attributes;
  ///
  /// Size of variable null-terminated Unicode string name.
  ///
  UINT32      NameSize;
  ///
  /// Size of the variable data without this header.
  ///
  UINT32      DataSize;
  ///
  /// A unique identifier for the vendor that produces and consumes this varaible.
  ///
  EFI_GUID    VendorGuid;
} VARIABLE_HEADER;

///
/// Single Authenticated Variable Data Header Structure.
///
typedef struct {
  ///
  /// Variable Data Start Flag.
  ///
  UINT16      StartId;
  ///
  /// Variable State defined above.
  ///
  UINT8       State;
  UINT8       Reserved;
  ///
  /// Attributes of variable defined in UEFI specification.
  ///
  UINT32      Attributes;
  ///
  /// Associated monotonic count value against replay attack.
  ///
  UINT64      MonotonicCount;
  ///
  /// Associated TimeStamp value against replay attack.
  ///
  EFI_TIME    TimeStamp;
  ///
  /// Index of associated public key in database.
  ///
  UINT32      PubKeyIndex;
  ///
  /// Size of variable null-terminated Unicode string name.
  ///
  UINT32      NameSize;
  ///
  /// Size of the variable data without this header.
  ///
  UINT32      DataSize;
  ///
  /// A unique identifier for the vendor that produces and consumes this varaible.
  ///
  EFI_GUID    VendorGuid;
} AUTHENTICATED_VARIABLE_HEADER;

typedef struct {
  EFI_GUID    *Guid;
  CHAR16      *Name;
  UINTN       VariableSize;
} VARIABLE_ENTRY_CONSISTENCY;

#pragma pack()

typedef struct _VARIABLE_INFO_ENTRY  VARIABLE_INFO_ENTRY;

///
/// This structure contains the variable list that is put in EFI system table.
/// The variable driver collects all variables that were used at boot service time and produces this list.
/// This is an optional feature to dump all used variables in shell environment.
///
struct _VARIABLE_INFO_ENTRY {
  VARIABLE_INFO_ENTRY *Next;       ///< Pointer to next entry.
  EFI_GUID            VendorGuid;  ///< Guid of Variable.
  CHAR16              *Name;       ///< Name of Variable.
  UINT32              Attributes;  ///< Attributes of variable defined in UEFI specification.
  UINT32              ReadCount;   ///< Number of times to read this variable.
  UINT32              WriteCount;  ///< Number of times to write this variable.
  UINT32              DeleteCount; ///< Number of times to delete this variable.
  UINT32              CacheCount;  ///< Number of times that cache hits this variable.
  BOOLEAN             Volatile;    ///< TRUE if volatile, FALSE if non-volatile.
};

#endif // _EFI_VARIABLE_H_
