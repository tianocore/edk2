/** @file
  Memory profile data structure.

  Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MEMORY_PROFILE_H_
#define _MEMORY_PROFILE_H_

#include <Pi/PiFirmwareFile.h>

//
// For BIOS MemoryType (0 ~ EfiMaxMemoryType - 1), it is recorded in UsageByType[MemoryType]. (Each valid entry has one entry)
// For OS MemoryType (0x80000000 ~ 0xFFFFFFFF), it is recorded in UsageByType[EfiMaxMemoryType]. (All types are combined into one entry)
// For OEM MemoryType (0x70000000 ~ 0x7FFFFFFF), it is recorded in UsageByType[EfiMaxMemoryType + 1]. (All types are combined into one entry)
//

typedef struct {
  UINT32                       Signature;
  UINT16                       Length;
  UINT16                       Revision;
} MEMORY_PROFILE_COMMON_HEADER;

#define MEMORY_PROFILE_CONTEXT_SIGNATURE SIGNATURE_32 ('M','P','C','T')
#define MEMORY_PROFILE_CONTEXT_REVISION 0x0002

typedef struct {
  MEMORY_PROFILE_COMMON_HEADER  Header;
  UINT64                        CurrentTotalUsage;
  UINT64                        PeakTotalUsage;
  UINT64                        CurrentTotalUsageByType[EfiMaxMemoryType + 2];
  UINT64                        PeakTotalUsageByType[EfiMaxMemoryType + 2];
  UINT64                        TotalImageSize;
  UINT32                        ImageCount;
  UINT32                        SequenceCount;
} MEMORY_PROFILE_CONTEXT;

#define MEMORY_PROFILE_DRIVER_INFO_SIGNATURE SIGNATURE_32 ('M','P','D','I')
#define MEMORY_PROFILE_DRIVER_INFO_REVISION 0x0003

typedef struct {
  MEMORY_PROFILE_COMMON_HEADER  Header;
  EFI_GUID                      FileName;
  PHYSICAL_ADDRESS              ImageBase;
  UINT64                        ImageSize;
  PHYSICAL_ADDRESS              EntryPoint;
  UINT16                        ImageSubsystem;
  EFI_FV_FILETYPE               FileType;
  UINT8                         Reserved[1];
  UINT32                        AllocRecordCount;
  UINT64                        CurrentUsage;
  UINT64                        PeakUsage;
  UINT64                        CurrentUsageByType[EfiMaxMemoryType + 2];
  UINT64                        PeakUsageByType[EfiMaxMemoryType + 2];
  UINT16                        PdbStringOffset;
  UINT8                         Reserved2[6];
//CHAR8                         PdbString[];
} MEMORY_PROFILE_DRIVER_INFO;

typedef enum {
  MemoryProfileActionAllocatePages = 1,
  MemoryProfileActionFreePages = 2,
  MemoryProfileActionAllocatePool = 3,
  MemoryProfileActionFreePool = 4,
} MEMORY_PROFILE_ACTION;

//
// Below is the detailed MEMORY_PROFILE_ACTION definition.
//
//  31       15      9  8  8 7  7 6   6 5-4  3 - 0
// +----------------------------------------------+
// |User |  |Lib|   |Re|Copy|Zero|Align|Type|Basic|
// +----------------------------------------------+
//

//
// Basic Action
//      1 : AllocatePages
//      2 : FreePages
//      3 : AllocatePool
//      4 : FreePool
//
#define MEMORY_PROFILE_ACTION_BASIC_MASK 0xF

//
// Extension
//
#define MEMORY_PROFILE_ACTION_EXTENSION_MASK               0xFFF0
#define MEMORY_PROFILE_ACTION_EXTENSION_LIB_MASK           0x8000
#define MEMORY_PROFILE_ACTION_EXTENSION_REALLOC_MASK       0x0200
#define MEMORY_PROFILE_ACTION_EXTENSION_COPY_MASK          0x0100
#define MEMORY_PROFILE_ACTION_EXTENSION_ZERO_MASK          0x0080
#define MEMORY_PROFILE_ACTION_EXTENSION_ALIGN_MASK         0x0040
#define MEMORY_PROFILE_ACTION_EXTENSION_MEM_TYPE_MASK      0x0030
#define MEMORY_PROFILE_ACTION_EXTENSION_MEM_TYPE_BASIC     0x0000
#define MEMORY_PROFILE_ACTION_EXTENSION_MEM_TYPE_RUNTIME   0x0010
#define MEMORY_PROFILE_ACTION_EXTENSION_MEM_TYPE_RESERVED  0x0020

//
// Extension (used by memory allocation lib)
//
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_PAGES                    0x8001
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_PAGES            0x8011
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RESERVED_PAGES           0x8021
#define MEMORY_PROFILE_ACTION_LIB_FREE_PAGES                        0x8002
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ALIGNED_PAGES            0x8041
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ALIGNED_RUNTIME_PAGES    0x8051
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ALIGNED_RESERVED_PAGES   0x8061
#define MEMORY_PROFILE_ACTION_LIB_FREE_ALIGNED_PAGES                0x8042
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_POOL                     0x8003
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_POOL             0x8013
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RESERVED_POOL            0x8023
#define MEMORY_PROFILE_ACTION_LIB_FREE_POOL                         0x8004
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ZERO_POOL                0x8083
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_ZERO_POOL        0x8093
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RESERVED_ZERO_POOL       0x80a3
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_COPY_POOL                0x8103
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_COPY_POOL        0x8113
#define MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RESERVED_COPY_POOL       0x8123
#define MEMORY_PROFILE_ACTION_LIB_REALLOCATE_POOL                   0x8203
#define MEMORY_PROFILE_ACTION_LIB_REALLOCATE_RUNTIME_POOL           0x8213
#define MEMORY_PROFILE_ACTION_LIB_REALLOCATE_RESERVED_POOL          0x8223

//
// User defined: 0x80000000~0xFFFFFFFF
//
// NOTE: User defined action MUST OR the basic action,
//       so that core can know the action is allocate or free,
//       and the type is pages (can be freed partially)
//       or pool (cannot be freed partially).
//
#define MEMORY_PROFILE_ACTION_USER_DEFINED_MASK           0x80000000

#define MEMORY_PROFILE_ALLOC_INFO_SIGNATURE SIGNATURE_32 ('M','P','A','I')
#define MEMORY_PROFILE_ALLOC_INFO_REVISION 0x0002

typedef struct {
  MEMORY_PROFILE_COMMON_HEADER  Header;
  PHYSICAL_ADDRESS              CallerAddress;
  UINT32                        SequenceId;
  UINT8                         Reserved[2];
  UINT16                        ActionStringOffset;
  MEMORY_PROFILE_ACTION         Action;
  EFI_MEMORY_TYPE               MemoryType;
  PHYSICAL_ADDRESS              Buffer;
  UINT64                        Size;
//CHAR8                         ActionString[];
} MEMORY_PROFILE_ALLOC_INFO;

#define MEMORY_PROFILE_DESCRIPTOR_SIGNATURE SIGNATURE_32 ('M','P','D','R')
#define MEMORY_PROFILE_DESCRIPTOR_REVISION 0x0001

typedef struct {
  MEMORY_PROFILE_COMMON_HEADER  Header;
  PHYSICAL_ADDRESS              Address;
  UINT64                        Size;
} MEMORY_PROFILE_DESCRIPTOR;

#define MEMORY_PROFILE_FREE_MEMORY_SIGNATURE SIGNATURE_32 ('M','P','R','M')
#define MEMORY_PROFILE_FREE_MEMORY_REVISION 0x0001

typedef struct {
  MEMORY_PROFILE_COMMON_HEADER  Header;
  UINT64                        TotalFreeMemoryPages;
  UINT32                        FreeMemoryEntryCount;
  UINT8                         Reserved[4];
  //MEMORY_PROFILE_DESCRIPTOR     MemoryDescriptor[FreeMemoryEntryCount];
} MEMORY_PROFILE_FREE_MEMORY;

#define MEMORY_PROFILE_MEMORY_RANGE_SIGNATURE SIGNATURE_32 ('M','P','M','R')
#define MEMORY_PROFILE_MEMORY_RANGE_REVISION 0x0001

typedef struct {
  MEMORY_PROFILE_COMMON_HEADER  Header;
  UINT32                        MemoryRangeCount;
  UINT8                         Reserved[4];
  //MEMORY_PROFILE_DESCRIPTOR     MemoryDescriptor[MemoryRangeCount];
} MEMORY_PROFILE_MEMORY_RANGE;

//
// UEFI memory profile layout:
// +--------------------------------+
// | CONTEXT                        |
// +--------------------------------+
// | DRIVER_INFO(1)                 |
// +--------------------------------+
// | ALLOC_INFO(1, 1)               |
// +--------------------------------+
// | ALLOC_INFO(1, m1)              |
// +--------------------------------+
// | DRIVER_INFO(n)                 |
// +--------------------------------+
// | ALLOC_INFO(n, 1)               |
// +--------------------------------+
// | ALLOC_INFO(n, mn)              |
// +--------------------------------+
//

typedef struct _EDKII_MEMORY_PROFILE_PROTOCOL EDKII_MEMORY_PROFILE_PROTOCOL;

/**
  Get memory profile data.

  @param[in]      This              The EDKII_MEMORY_PROFILE_PROTOCOL instance.
  @param[in, out] ProfileSize       On entry, points to the size in bytes of the ProfileBuffer.
                                    On return, points to the size of the data returned in ProfileBuffer.
  @param[out]     ProfileBuffer     Profile buffer.

  @return EFI_SUCCESS               Get the memory profile data successfully.
  @return EFI_UNSUPPORTED           Memory profile is unsupported.
  @return EFI_BUFFER_TO_SMALL       The ProfileSize is too small for the resulting data.
                                    ProfileSize is updated with the size required.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_MEMORY_PROFILE_GET_DATA)(
  IN     EDKII_MEMORY_PROFILE_PROTOCOL  *This,
  IN OUT UINT64                         *ProfileSize,
     OUT VOID                           *ProfileBuffer
  );

/**
  Register image to memory profile.

  @param[in] This               The EDKII_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] FilePath           File path of the image.
  @param[in] ImageBase          Image base address.
  @param[in] ImageSize          Image size.
  @param[in] FileType           File type of the image.

  @return EFI_SUCCESS           Register successfully.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required.
  @return EFI_OUT_OF_RESOURCES  No enough resource for this register.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_MEMORY_PROFILE_REGISTER_IMAGE)(
  IN EDKII_MEMORY_PROFILE_PROTOCOL      *This,
  IN EFI_DEVICE_PATH_PROTOCOL           *FilePath,
  IN PHYSICAL_ADDRESS                   ImageBase,
  IN UINT64                             ImageSize,
  IN EFI_FV_FILETYPE                    FileType
  );

/**
  Unregister image from memory profile.

  @param[in] This               The EDKII_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] FilePath           File path of the image.
  @param[in] ImageBase          Image base address.
  @param[in] ImageSize          Image size.

  @return EFI_SUCCESS           Unregister successfully.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required.
  @return EFI_NOT_FOUND         The image is not found.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_MEMORY_PROFILE_UNREGISTER_IMAGE)(
  IN EDKII_MEMORY_PROFILE_PROTOCOL      *This,
  IN EFI_DEVICE_PATH_PROTOCOL           *FilePath,
  IN PHYSICAL_ADDRESS                   ImageBase,
  IN UINT64                             ImageSize
  );

#define MEMORY_PROFILE_RECORDING_ENABLE     TRUE
#define MEMORY_PROFILE_RECORDING_DISABLE    FALSE

/**
  Get memory profile recording state.

  @param[in]  This              The EDKII_MEMORY_PROFILE_PROTOCOL instance.
  @param[out] RecordingState    Recording state.

  @return EFI_SUCCESS           Memory profile recording state is returned.
  @return EFI_UNSUPPORTED       Memory profile is unsupported.
  @return EFI_INVALID_PARAMETER RecordingState is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_MEMORY_PROFILE_GET_RECORDING_STATE) (
  IN EDKII_MEMORY_PROFILE_PROTOCOL      *This,
  OUT BOOLEAN                           *RecordingState
  );

/**
  Set memory profile recording state.

  @param[in] This               The EDKII_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] RecordingState     Recording state.

  @return EFI_SUCCESS           Set memory profile recording state successfully.
  @return EFI_UNSUPPORTED       Memory profile is unsupported.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_MEMORY_PROFILE_SET_RECORDING_STATE) (
  IN EDKII_MEMORY_PROFILE_PROTOCOL      *This,
  IN BOOLEAN                            RecordingState
  );

/**
  Record memory profile of multilevel caller.

  @param[in] This               The EDKII_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] CallerAddress      Address of caller.
  @param[in] Action             Memory profile action.
  @param[in] MemoryType         Memory type.
                                EfiMaxMemoryType means the MemoryType is unknown.
  @param[in] Buffer             Buffer address.
  @param[in] Size               Buffer size.
  @param[in] ActionString       String for memory profile action.
                                Only needed for user defined allocate action.

  @return EFI_SUCCESS           Memory profile is updated.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required,
                                or memory profile for the memory type is not required.
  @return EFI_ACCESS_DENIED     It is during memory profile data getting.
  @return EFI_ABORTED           Memory profile recording is not enabled.
  @return EFI_OUT_OF_RESOURCES  No enough resource to update memory profile for allocate action.
  @return EFI_NOT_FOUND         No matched allocate info found for free action.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_MEMORY_PROFILE_RECORD) (
  IN EDKII_MEMORY_PROFILE_PROTOCOL      *This,
  IN PHYSICAL_ADDRESS                   CallerAddress,
  IN MEMORY_PROFILE_ACTION              Action,
  IN EFI_MEMORY_TYPE                    MemoryType,
  IN VOID                               *Buffer,
  IN UINTN                              Size,
  IN CHAR8                              *ActionString OPTIONAL
  );

struct _EDKII_MEMORY_PROFILE_PROTOCOL {
  EDKII_MEMORY_PROFILE_GET_DATA             GetData;
  EDKII_MEMORY_PROFILE_REGISTER_IMAGE       RegisterImage;
  EDKII_MEMORY_PROFILE_UNREGISTER_IMAGE     UnregisterImage;
  EDKII_MEMORY_PROFILE_GET_RECORDING_STATE  GetRecordingState;
  EDKII_MEMORY_PROFILE_SET_RECORDING_STATE  SetRecordingState;
  EDKII_MEMORY_PROFILE_RECORD               Record;
};

//
// SMRAM profile layout:
// +--------------------------------+
// | CONTEXT                        |
// +--------------------------------+
// | DRIVER_INFO(1)                 |
// +--------------------------------+
// | ALLOC_INFO(1, 1)               |
// +--------------------------------+
// | ALLOC_INFO(1, m1)              |
// +--------------------------------+
// | DRIVER_INFO(n)                 |
// +--------------------------------+
// | ALLOC_INFO(n, 1)               |
// +--------------------------------+
// | ALLOC_INFO(n, mn)              |
// +--------------------------------+
// | FREE_MEMORY                    |
// +--------------------------------+
// | FREE MEMORY DESCRIPTOR(1)      |
// +--------------------------------+
// | FREE MEMORY DESCRIPTOR(p)      |
// +--------------------------------+
// | MEMORY_RANGE                   |
// +--------------------------------+
// | MEMORY RANGE DESCRIPTOR(1)     |
// +--------------------------------+
// | MEMORY RANGE DESCRIPTOR(q)     |
// +--------------------------------+
//

//
// SMRAM profile command
//
#define SMRAM_PROFILE_COMMAND_GET_PROFILE_INFO           0x1
#define SMRAM_PROFILE_COMMAND_GET_PROFILE_DATA           0x2
//
// Below 2 commands have been deprecated. They may not be re-used.
//
#define SMRAM_PROFILE_COMMAND_DEPRECATED1                0x3
#define SMRAM_PROFILE_COMMAND_DEPRECATED2                0x4

#define SMRAM_PROFILE_COMMAND_GET_PROFILE_DATA_BY_OFFSET 0x5
#define SMRAM_PROFILE_COMMAND_GET_RECORDING_STATE        0x6
#define SMRAM_PROFILE_COMMAND_SET_RECORDING_STATE        0x7

typedef struct {
  UINT32                            Command;
  UINT32                            DataLength;
  UINT64                            ReturnStatus;
} SMRAM_PROFILE_PARAMETER_HEADER;

typedef struct {
  SMRAM_PROFILE_PARAMETER_HEADER    Header;
  UINT64                            ProfileSize;
} SMRAM_PROFILE_PARAMETER_GET_PROFILE_INFO;

typedef struct {
  SMRAM_PROFILE_PARAMETER_HEADER    Header;
  UINT64                            ProfileSize;
  PHYSICAL_ADDRESS                  ProfileBuffer;
} SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA;

typedef struct {
  SMRAM_PROFILE_PARAMETER_HEADER    Header;
  //
  // On input, profile buffer size.
  // On output, actual profile data size copied.
  //
  UINT64                            ProfileSize;
  PHYSICAL_ADDRESS                  ProfileBuffer;
  //
  // On input, profile buffer offset to copy.
  // On output, next time profile buffer offset to copy.
  //
  UINT64                            ProfileOffset;
} SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA_BY_OFFSET;

typedef struct {
  SMRAM_PROFILE_PARAMETER_HEADER    Header;
  BOOLEAN                           RecordingState;
} SMRAM_PROFILE_PARAMETER_RECORDING_STATE;

typedef struct {
  SMRAM_PROFILE_PARAMETER_HEADER    Header;
  EFI_GUID                          FileName;
  PHYSICAL_ADDRESS                  ImageBuffer;
  UINT64                            NumberOfPage;
} SMRAM_PROFILE_PARAMETER_REGISTER_IMAGE;

typedef struct {
  SMRAM_PROFILE_PARAMETER_HEADER    Header;
  EFI_GUID                          FileName;
  PHYSICAL_ADDRESS                  ImageBuffer;
  UINT64                            NumberOfPage;
} SMRAM_PROFILE_PARAMETER_UNREGISTER_IMAGE;


#define EDKII_MEMORY_PROFILE_GUID { \
  0x821c9a09, 0x541a, 0x40f6, { 0x9f, 0x43, 0xa, 0xd1, 0x93, 0xa1, 0x2c, 0xfe } \
}

extern EFI_GUID gEdkiiMemoryProfileGuid;

typedef EDKII_MEMORY_PROFILE_PROTOCOL EDKII_SMM_MEMORY_PROFILE_PROTOCOL;

#define EDKII_SMM_MEMORY_PROFILE_GUID { \
  0xe22bbcca, 0x516a, 0x46a8, { 0x80, 0xe2, 0x67, 0x45, 0xe8, 0x36, 0x93, 0xbd } \
}

extern EFI_GUID gEdkiiSmmMemoryProfileGuid;

#endif

