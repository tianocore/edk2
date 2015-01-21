/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  IdccData.h

Abstract:

--*/

#ifndef _IDCCDATAHUB_GUID_H_
#define _IDCCDATAHUB_GUID_H_

//
// This GUID is for the IDCC related data found in the Data Hub.
//
#define IDCC_DATA_HUB_GUID \
  { 0x788e1d9f, 0x1eab, 0x47d2, 0xa2, 0xf3, 0x78, 0xca, 0xe8, 0x7d, 0x60, 0x12 }

extern EFI_GUID gIdccDataHubGuid;

#pragma pack(1)

typedef struct {
  UINT32    Type;
  UINT32    RecordLength;
} EFI_IDCC_DATA_HEADER;

typedef struct {
  EFI_IDCC_DATA_HEADER  IdccHeader;
  UINT32                Tcontrol;
} EFI_IDCC_TCONTROL;

typedef struct {
  UINT32    EntryCount;
} EFI_IDCC_CLOCK_COMMON;

typedef struct {
  UINT8     Polarity;
  UINT8     Percent;
  UINT32    FpValue;
} EFI_IDCC_TYPE_2_DATA;

typedef struct {
  UINT8     SetupVal;
  UINT32    FpValue;
} EFI_IDCC_TYPE_3_4_DATA;

typedef struct {
  EFI_IDCC_DATA_HEADER  IdccHeader;
  UINT32                ProcessorRatio;
} EFI_IDCC_PROCESSOR_RATIO;

typedef struct {
  EFI_IDCC_DATA_HEADER  IdccHeader;
  UINT32                BoardFormFactor;
} EFI_IDCC_BOARD_FORM_FACTOR;

typedef struct {
  EFI_IDCC_DATA_HEADER  IdccHeader;
  UINT32                ProcessorInfo;
} EFI_IDCC_PROCESSOR_INFO;

#define EFI_IDCC_PROCESSOR_UNCON    (1 << 0)  // Bit 0: UnCon CPU
#define EFI_IDCC_PROCESSOR_UNLOCK   (1 << 1)  // Bit 1: UnLock CPU
#define EFI_IDCC_PROCESSOR_CNR      (1 << 2)  // Bit 2: CNR CPU
#define EFI_IDCC_PROCESSOR_KNF      (1 << 3)  // Bit 3: KNF CPU

typedef struct {
  EFI_IDCC_DATA_HEADER  IdccHeader;
  UINT32    MinFSB;
  UINT32    MaxFSB;
  UINT8     StepFSB;
} EFI_IDCC_FSB_DATA;

#pragma pack()

#define EFI_IDCC_POSITIVE   0
#define EFI_IDCC_NEGATIVE   1

//
// Board Form Factor equates.
//
#define ATX_FORM_FACTOR		0x00
#define BTX_FORM_FACTOR		0x01


#define EFI_IDCC_TCONTROL_TYPE          1
#define EFI_IDCC_FSB_TYPE               2
#define EFI_IDCC_PCI_TYPE               3
#define EFI_IDCC_PCIE_TYPE              4
#define EFI_IDCC_PROC_RATIO_TYPE        5
#define EFI_IDCC_BOARD_FORM_FACTOR_TYPE 6
#define EFI_IDCC_PROC_INFO_TYPE         7
#define EFI_IDCC_FSB_DATA_TYPE          8

#endif
