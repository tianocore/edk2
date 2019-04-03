
/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent


Module Name:

  VlvPlatformPolicy.h

Abstract:

  Interface definition details between MCH and platform drivers during DXE phase.

--*/

#ifndef _VLV_PLATFORM_POLICY_H_
#define _VLV_PLATFORM_POLICY_H_

//
// VLV Policy provided by platform for DXE phase {5BAB88BA-E0E2-4674-B6AD-B812F6881CD6}
//
#define DXE_VLV_PLATFORM_POLICY_GUID \
  {0x5bab88ba, 0xe0e2, 0x4674, 0xb6, 0xad, 0xb8, 0x12, 0xf6, 0x88, 0x1c, 0xd6}

//
// Extern the GUID for protocol users.
//
extern EFI_GUID gDxeVlvPlatformPolicyGuid;

//
// Protocol revision number
// Any backwards compatible changes to this protocol will result in an update in the revision number
// Major changes will require publication of a new protocol
//
#define DXE_VLV_PLATFORM_POLICY_PROTOCOL_REVISION 0


typedef struct {
  UINT8  PFITStatus;
  UINT8  IgdTheramlSupport;
  UINT8  ALSEnabled;
  UINT8  LidStatus;
} IGD_PANEL_FEATURES;

typedef struct {
  UINT8   Reserved00;                     
  UINT8   Reserved01;                     
  UINT16  Reserved02;  
  UINT16  Reserved03; 
  UINT16  Reserved04; 
  UINT16  Reserved05;  
  UINT16  Reserved06;  
  UINT16  Reserved07; 
  UINT16  Reserved08; 
  UINT16  Reserved09;  
  UINT16  Reserved0A; 
  UINT16  Reserved0B;
  UINT16  Reserved0C;
  UINT16  Reserved0D;
  UINT8   Reserved0E;
  UINT8   Reserved0F;
  UINT32  Reserved10;
  UINT32  Reserved11;
  UINT32  Reserved12;
  UINT32  Reserved13;
  UINT32  Reserved14;
  UINT8   Reserved15;
  UINT8   Reserved16;
} DPTF_SETTINGS;

//
// MCH DXE Platform Policiy ==================================================
//

#define NO_AUDIO   0
#define HD_AUDIO   1
#define LPE_AUDIO  2

typedef struct _DXE_VLV_PLATFORM_POLICY_PROTOCOL {
  UINT8                   Revision;
  IGD_PANEL_FEATURES      IgdPanelFeatures;
  DPTF_SETTINGS           Reserved;
  UINT8                   GraphicReserve00;
  UINT8                   GraphicsPerfAnalyzers;
  UINT8                   PwmReserved00;
  UINT8                   PwmReserved01;  
  UINT8                   PmSupport;
  UINT8                   GraphicReserve01;
  UINT8                   GfxPause;
  UINT8                   GraphicsFreqReq;
  UINT8                   GraphicReserve03;
  UINT8                   GraphicReserve02;
  UINT8                   GraphicReserve04;
  UINT8                   PavpMode;
  UINT8                   GraphicReserve05;
  UINT8                   UlClockGating;
  UINT8                   IdleReserve;
  UINT8                   AudioTypeSupport;
  UINT8                   GraphicReserve06;
} DXE_VLV_PLATFORM_POLICY_PROTOCOL;

#endif
