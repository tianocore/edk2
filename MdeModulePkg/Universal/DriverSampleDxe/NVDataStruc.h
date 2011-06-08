/** @file

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

#define FORMSET_GUID \
  { \
    0xA04A27f4, 0xDF00, 0x4D42, {0xB5, 0x52, 0x39, 0x51, 0x13, 0x02, 0x11, 0x3D} \
  }

#define INVENTORY_GUID \
  { \
    0xb3f56470, 0x6141, 0x4621, {0x8f, 0x19, 0x70, 0x4e, 0x57, 0x7a, 0xa9, 0xe8} \
  }

#define EFI_USER_INFO_ACCESS_SETUP_ADMIN_GUID \
  { 0x85b75607, 0xf7ce, 0x471e, { 0xb7, 0xe4, 0x2a, 0xea, 0x5f, 0x72, 0x32, 0xee } }

#define EFI_IFR_REFRESH_ID_OP_GUID \
  { \
    0xF5E655D9, 0x02A6, 0x46f2, {0x9E, 0x76, 0xB8, 0xBE, 0x8E, 0x60, 0xAB, 0x22} \
  }

#define CONFIGURATION_VARSTORE_ID    0x1234

#pragma pack(1)
typedef struct {
  UINT16  WhatIsThePassword[20];
  UINT16  WhatIsThePassword2[20];
  UINT16  MyStringData[40];
  UINT16  PasswordClearText[20];
  UINT16  SomethingHiddenForHtml;
  UINT8   HowOldAreYouInYearsManual;
  UINT16  HowTallAreYouManual;
  UINT8   HowOldAreYouInYears;
  UINT16  HowTallAreYou;
  UINT8   MyFavoriteNumber;
  UINT8   TestLateCheck;
  UINT8   TestLateCheck2;
  UINT8   QuestionAboutTreeHugging;
  UINT8   ChooseToActivateNuclearWeaponry;
  UINT8   SuppressGrayOutSomething;
  UINT8   OrderedList[8];
  UINT16  BootOrder[8];
  UINT8   BootOrderLarge;
  UINT8   DynamicRefresh;
  UINT8   DynamicOneof;
  UINT8   DynamicOrderedList[5];
  UINT8   Reserved;
  UINT8   NameValueVar0;
  UINT16  NameValueVar1;
  UINT16  NameValueVar2[20];
  UINT8   SerialPortNo;
  UINT8   SerialPortStatus;
  UINT16  SerialPortIo;
  UINT8   SerialPortIrq;
  UINT8   GetDefaultValueFromCallBack;
  UINT8   GetDefaultValueFromAccess;
  EFI_HII_TIME  Time;
  UINT8   RefreshGuidCount;
} DRIVER_SAMPLE_CONFIGURATION;

//
// 2nd NV data structure definition
//
typedef struct {
  UINT8         Field8;
  UINT16        Field16;
  UINT8         OrderedList[3];
} MY_DATA2;

//
// Labels definition
//
#define LABEL_UPDATE1               0x1234
#define LABEL_UPDATE2               0x2234
#define LABEL_UPDATE3               0x3234
#define LABEL_END                   0x2223

#pragma pack()

#endif
