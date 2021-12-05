/** @file

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>*
(C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  NVDataStruc.h

Abstract:

  NVData structure used by the sample driver

Revision History:


**/

#ifndef _NVDATASTRUC_H_
#define _NVDATASTRUC_H_

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/HiiFormMapMethodGuid.h>
#include <Guid/DriverSampleHii.h>
#include <Guid/ZeroGuid.h>

#define CONFIGURATION_VARSTORE_ID  0x1234
#define BITS_VARSTORE_ID           0x2345

#pragma pack(1)

//
// !!! For a structure with a series of bit fields and used as a storage in vfr file, and if the bit fields do not add up to the size of the defined type.
// In the C code use sizeof() to get the size the strucure, the results may vary form the compiler(VS,GCC...).
// But the size of the storage calculated by VfrCompiler is fixed (calculate with alignment).
// To avoid above case, we need to make the total bit width in the structure aligned with the size of the defined type for these bit fields. We can:
// 1. Add bit field (with/without name) with remianing with for padding.
// 2. Add unnamed bit field with 0 for padding, the amount of padding is determined by the alignment characteristics of the members of the structure.
//
typedef struct {
  UINT16    NestByteField;
  UINT8                     : 1; // unamed field can be used for padding
  UINT8     NestBitCheckbox : 1;
  UINT8     NestBitOneof    : 2;
  UINT8                     : 0; // Special width 0 can be used to force alignment at the next word boundary
  UINT8     NestBitNumeric  : 4;
} MY_BITS_DATA;

typedef union {
  UINT8    UnionNumeric;
  UINT8    UnionNumericAlias;
} MY_EFI_UNION_DATA;

typedef struct {
  UINT16               MyStringData[40];
  UINT16               SomethingHiddenForHtml;
  UINT8                HowOldAreYouInYearsManual;
  UINT16               HowTallAreYouManual;
  UINT8                HowOldAreYouInYears;
  UINT16               HowTallAreYou;
  UINT8                MyFavoriteNumber;
  UINT8                TestLateCheck;
  UINT8                TestLateCheck2;
  UINT8                QuestionAboutTreeHugging;
  UINT8                ChooseToActivateNuclearWeaponry;
  UINT8                SuppressGrayOutSomething;
  UINT8                OrderedList[8];
  UINT16               BootOrder[8];
  UINT8                BootOrderLarge;
  UINT8                DynamicRefresh;
  UINT8                DynamicOneof;
  UINT8                DynamicOrderedList[5];
  UINT8                Reserved;
  EFI_HII_REF          RefData;
  UINT8                NameValueVar0;
  UINT16               NameValueVar1;
  UINT16               NameValueVar2[20];
  UINT8                SerialPortNo;
  UINT8                SerialPortStatus;
  UINT16               SerialPortIo;
  UINT8                SerialPortIrq;
  UINT8                GetDefaultValueFromCallBack;
  UINT8                GetDefaultValueFromAccess;
  EFI_HII_TIME         Time;
  UINT8                RefreshGuidCount;
  UINT8                Match2;
  UINT8                GetDefaultValueFromCallBackForOrderedList[3];
  UINT8                BitCheckbox  : 1;
  UINT8                ReservedBits : 7; // Reserved bit fields for padding.
  UINT16               BitOneof     : 6;
  UINT16                            : 0; // Width 0 used to force alignment.
  UINT16               BitNumeric   : 12;
  MY_BITS_DATA         MyBitData;
  MY_EFI_UNION_DATA    MyUnionData;
  UINT8                QuestionXUefiKeywordRestStyle;
  UINT8                QuestionNonXUefiKeywordRestStyle;
} DRIVER_SAMPLE_CONFIGURATION;

//
// 2nd NV data structure definition
//
typedef struct {
  UINT8     Field8;
  UINT16    Field16;
  UINT8     OrderedList[3];
  UINT16    SubmittedCallback;
} MY_EFI_VARSTORE_DATA;

//
// 3rd NV data structure definition
//
typedef struct {
  MY_BITS_DATA    BitsData;
  UINT32          EfiBitGrayoutTest : 5;
  UINT32          EfiBitNumeric     : 4;
  UINT32          EfiBitOneof       : 10;
  UINT32          EfiBitCheckbox    : 1;
  UINT32                            : 0; // Width 0 used to force alignment.
} MY_EFI_BITS_VARSTORE_DATA;

//
// Labels definition
//
#define LABEL_UPDATE1  0x1234
#define LABEL_UPDATE2  0x2234
#define LABEL_UPDATE3  0x3234
#define LABEL_END      0x2223

#pragma pack()

#endif
