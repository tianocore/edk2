/** @file
  Memory profile data structure.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MEMORY_PROFILE_H_
#define _MEMORY_PROFILE_H_

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
#define MEMORY_PROFILE_DRIVER_INFO_REVISION 0x0002

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
} MEMORY_PROFILE_DRIVER_INFO;

typedef enum {
  MemoryProfileActionAllocatePages = 1,
  MemoryProfileActionFreePages = 2,
  MemoryProfileActionAllocatePool = 3,
  MemoryProfileActionFreePool = 4,
} MEMORY_PROFILE_ACTION;

#define MEMORY_PROFILE_ALLOC_INFO_SIGNATURE SIGNATURE_32 ('M','P','A','I')
#define MEMORY_PROFILE_ALLOC_INFO_REVISION 0x0001

typedef struct {
  MEMORY_PROFILE_COMMON_HEADER  Header;
  PHYSICAL_ADDRESS              CallerAddress;
  UINT32                        SequenceId;
  UINT8                         Reserved[4];
  MEMORY_PROFILE_ACTION         Action;
  EFI_MEMORY_TYPE               MemoryType;
  PHYSICAL_ADDRESS              Buffer;
  UINT64                        Size;
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

  @return EFI_SUCCESS           Register success.
  @return EFI_OUT_OF_RESOURCE   No enough resource for this register.

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

  @return EFI_SUCCESS           Unregister success.
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

struct _EDKII_MEMORY_PROFILE_PROTOCOL {
  EDKII_MEMORY_PROFILE_GET_DATA         GetData;
  EDKII_MEMORY_PROFILE_REGISTER_IMAGE   RegisterImage;
  EDKII_MEMORY_PROFILE_UNREGISTER_IMAGE UnregisterImage;
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
#define SMRAM_PROFILE_COMMAND_GET_PROFILE_INFO         0x1
#define SMRAM_PROFILE_COMMAND_GET_PROFILE_DATA         0x2
//
// Below 2 commands are now used by ECP only and only valid before SmmReadyToLock
//
#define SMRAM_PROFILE_COMMAND_REGISTER_IMAGE           0x3
#define SMRAM_PROFILE_COMMAND_UNREGISTER_IMAGE         0x4

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
  0x821c9a09, 0x541a, 0x40f6, 0x9f, 0x43, 0xa, 0xd1, 0x93, 0xa1, 0x2c, 0xfe \
}

extern EFI_GUID gEdkiiMemoryProfileGuid;

#endif

