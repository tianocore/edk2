/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiCapsule.h

Abstract:

  Defines for the EFI Capsule functionality
  
--*/

#ifndef _EFI_CAPSULE_H_
#define _EFI_CAPSULE_H_


#define CAPSULE_BLOCK_DESCRIPTOR_SIGNATURE  EFI_SIGNATURE_32 ('C', 'B', 'D', 'S')

typedef struct {
  EFI_GUID  OemGuid;
  UINT32    HeaderSize;
  //
  // UINT8                       OemHdrData[];
  //
} EFI_CAPSULE_OEM_HEADER;

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)

#define MAX_SUPPORT_CAPSULE_NUM               50
#define CAPSULE_FLAGS_PERSIST_ACROSS_RESET    0x00010000
#define CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE   0x00020000 

typedef struct {
  UINT64                   Length;                    
  union { 
    EFI_PHYSICAL_ADDRESS   DataBlock;                 
    EFI_PHYSICAL_ADDRESS   ContinuationPointer;  
  } Union;
} EFI_CAPSULE_BLOCK_DESCRIPTOR;

typedef struct {
  EFI_GUID  CapsuleGuid;
  UINT32    HeaderSize;
  UINT32    Flags;
  UINT32    CapsuleImageSize;
} EFI_CAPSULE_HEADER;

typedef struct {
  UINT32   CapsuleArrayNumber;
  VOID*    CapsulePtr[1];
} EFI_CAPSULE_TABLE;

//
// This struct is deprecated because VendorTable entries physical address will not be fixed up when 
// transitioning from preboot to runtime phase. So we don't need CapsuleInfoTable to record capsule
// GUIDs any more for runtime convert.
//
typedef struct {
  UINT32      CapsuleGuidNumber;
  EFI_GUID    CapsuleGuidPtr[1];
} EFI_CAPSULE_INFO_TABLE;

//
// This GUID is used for collecting all capsules' Guids who install in ConfigTable.
// This GUID is deprecated as well.
//
#define EFI_CAPSULE_INFO_GUID \
  { \
    0x8B34EAC7, 0x2690, 0x460B, {0x8B, 0xA5, 0xD5, 0xCF, 0x32, 0x83, 0x17, 0x35} \
  }

#else

typedef struct {
  EFI_GUID  CapsuleGuid;
  UINT32    HeaderSize;
  UINT32    Flags;
  UINT32    CapsuleImageSize;
  UINT32    SequenceNumber;
  EFI_GUID  InstanceId;
  UINT32    OffsetToSplitInformation;
  UINT32    OffsetToCapsuleBody;
  UINT32    OffsetToOemDefinedHeader;
  UINT32    OffsetToAuthorInformation;
  UINT32    OffsetToRevisionInformation;
  UINT32    OffsetToShortDescription;
  UINT32    OffsetToLongDescription;
  UINT32    OffsetToApplicableDevices;
} EFI_CAPSULE_HEADER;

//
// An array of these describe the blocks that make up a capsule for
// a capsule update.
//
typedef struct {
  UINT64                Length;     // length of the data block
  EFI_PHYSICAL_ADDRESS  Data;       // physical address of the data block
  UINT32                Signature;  // CBDS
  UINT32                CheckSum;   // to sum this structure to 0
} EFI_CAPSULE_BLOCK_DESCRIPTOR;

#endif

//
// Bits in the flags field of the capsule header
//
#define EFI_CAPSULE_HEADER_FLAG_SETUP 0x00000001  // supports setup changes
//
// This is the GUID of the capsule header of the image on disk.
//
#define EFI_CAPSULE_GUID \
  { \
    0x3B6686BD, 0x0D76, 0x4030, {0xB7, 0x0E, 0xB5, 0x51, 0x9E, 0x2F, 0xC5, 0xA0} \
  }

//
// This is the GUID of the file created by the capsule application that contains
// the path to the device(s) to update.
//
#define EFI_PATH_FILE_NAME_GUID \
  { \
    0x7644C181, 0xFA6E, 0x46DA, {0x80, 0xCB, 0x04, 0xB9, 0x90, 0x40, 0x62, 0xE8} \
  }
//
// This is the GUID of the configuration results file created by the capsule
// application.
//
#define EFI_CONFIG_FILE_NAME_GUID \
  { \
    0x98B8D59B, 0xE8BA, 0x48EE, {0x98, 0xDD, 0xC2, 0x95, 0x39, 0x2F, 0x1E, 0xDB} \
  }

#endif // #ifndef _EFI_CAPSULE_H_
