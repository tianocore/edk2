
/*++

Copyright (c)  2010  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent


Module Name:

  VlvPolicy.h

Abstract:

  Interface definition details between ValleyView MRC and platform drivers during PEI phase.

--*/

#ifndef _VLV_POLICY_PPI_H_
#define _VLV_POLICY_PPI_H_

//
// MRC Policy provided by platform for PEI phase {7D84B2C2-22A1-4372-B12C-EBB232D3A6A3}
//
#define VLV_POLICY_PPI_GUID \
  { \
    0x7D84B2C2, 0x22A1, 0x4372, 0xB1, 0x2C, 0xEB, 0xB2, 0x32, 0xD3, 0xA6, 0xA3 \
  }

//
// Extern the GUID for protocol users.
//
extern EFI_GUID gVlvPolicyPpiGuid;

//
// PPI revision number
// Any backwards compatible changes to this PPI will result in an update in the revision number
// Major changes will require publication of a new PPI
//
#define MRC_PLATFORM_POLICY_PPI_REVISION  1

#ifndef MAX_SOCKETS
#define MAX_SOCKETS 4
#endif

#define S3_TIMING_DATA_LEN          9
#define S3_READ_TRAINING_DATA_LEN   16
#define S3_WRITE_TRAINING_DATA_LEN  12

#ifndef S3_RESTORE_DATA_LEN
#define S3_RESTORE_DATA_LEN (S3_TIMING_DATA_LEN + S3_READ_TRAINING_DATA_LEN + S3_WRITE_TRAINING_DATA_LEN)
#endif // S3_RESTORE_DATA_LEN
#pragma pack(1)
//
// MRC Platform Data Structure
//
typedef struct {
  UINT8   SpdAddressTable[MAX_SOCKETS];
  UINT8   TSonDimmSmbusAddress[MAX_SOCKETS];

  UINT16  SmbusBar;
  UINT32  IchRcba;
  UINT32  WdbBaseAddress; // Write Data Buffer area (WC caching mode)
  UINT32  WdbRegionSize;
  UINT32  SmBusAddress;
  UINT8   UserBd;
  UINT8   PlatformType;
  UINT8   FastBoot;
  UINT8   DynSR;
} VLV_PLATFORM_DATA;


typedef struct {
  UINT16  MmioSize;
  UINT16  GttSize;
  UINT8   IgdDvmt50PreAlloc;
  UINT8   PrimaryDisplay;
  UINT8   PAVPMode;
  UINT8   ApertureSize;
} GT_CONFIGURATION;

typedef struct {
  UINT8   EccSupport;
  UINT16  DdrFreqLimit;
  UINT8   MaxTolud;
} MEMORY_CONFIGURATION;


//
// MRC Platform Policiy PPI
//
typedef struct _VLV_POLICY_PPI {
  UINT8                 Revision;
  VLV_PLATFORM_DATA     PlatformData;
  GT_CONFIGURATION      GtConfig;
  MEMORY_CONFIGURATION  MemConfig;
  VOID                  *S3DataPtr; // was called MRC_PARAMS_SAVE_RESTORE
  UINT8                 ISPEn;            //ISP (IUNIT) Device Enabled
  UINT8                 ISPPciDevConfig;  //ISP (IUNIT) Device Config: 0->B0/D2/F0 for Window OS, 1->B0D3/F0 for Linux OS
} VLV_POLICY_PPI;

#pragma pack()

#endif // _VLV_POLICY_PPI_H_
