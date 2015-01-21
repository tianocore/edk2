/** 
  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
/*++

Module Name:

  PpmPlatformPolicy.h

Abstract:

  Interface definition details between PPM and platform drivers during DXE phase.

--*/
#ifndef _PPM_PLATFORM_POLICY_H_
#define _PPM_PLATFORM_POLICY_H_

//
//  PPM policy provided by platform for DXE phase {DDABFEAC-EF63-452c-8F39-ED7FAED8265E}
//
#define PPM_PLATFORM_POLICY_PROTOCOL_GUID \
  {0xddabfeac, 0xef63, 0x452c, 0x8f, 0x39, 0xed, 0x7f, 0xae, 0xd8, 0x26, 0x5e}

//
// Extern the GUID for protocol users.
//
extern EFI_GUID gPpmPlatformPolicyProtocolGuid;

//
// Forward reference for ANSI C compatibility
//
typedef struct _PPM_PLATFORM_POLICY_PROTOCOL PPM_PLATFORM_POLICY_PROTOCOL;

//
// Protocol revision number
// Any backwards compatible changes to this protocol will result in an update in the revision number
// Major changes will require publication of a new protocol
//
// Revision 1: Original version
// Revision 2: Added T-states field to the PPM_FUNCTION_ENABLES structure, Renamed unused fields - CxPopUpEnable, CxPopDownEnable, FastC4ExitEnable
// Revision 3: Extended VidCpuid to 32 bits for extended CPUID support (Penryn)
// Revision 4: Added support for extended C6 residency enabling
//
#define PPM_PLATFORM_POLICY_PROTOCOL_REVISION     1
#define PPM_PLATFORM_POLICY_PROTOCOL_REVISION_2   2
#define PPM_PLATFORM_POLICY_PROTOCOL_REVISION_3   3
#define PPM_PLATFORM_POLICY_PROTOCOL_REVISION_4   4

//
// Define maximum number of custom VID states supported
//
#ifndef MAX_CUSTOM_VID_TABLE_STATES
#define MAX_CUSTOM_VID_TABLE_STATES               6
#endif
//
// Custom VID table
//
typedef struct {
  UINT8   VidNumber;
  UINT32  VidCpuid;
  UINT16  VidMaxRatio;
  UINT16  VidMaxVid;
  UINT16  StateRatio[MAX_CUSTOM_VID_TABLE_STATES];
  UINT16  StateVid[MAX_CUSTOM_VID_TABLE_STATES];
} PPM_CUSTOM_VID_TABLE;

//
// PPM functional enables
//
typedef struct {
  UINT8   EnableGv                   :1; // 0: Disabled; 1: Enabled
  UINT8   EnableCx                   :1;
  UINT8   EnableCxe                  :1;
  UINT8   EnableC4                   :1;
  UINT8   EnableC6                   :1;
  UINT8   EnableC7                   :1;
  UINT8   EnableTm                   :1;
  UINT8   Reserve00                  :1;
  UINT8   Reserve01                  :1;
  UINT8   EnableTurboMode            :1;
  UINT8   PowerLimit2                :1;
  UINT8   EnableProcHot              :1;
  UINT8   Reserve02                  :1;
  UINT8   EnableCMP                  :1;
  UINT8   TStatesEnable              :1;
  UINT8   Reserve03                  :1;
  UINT8   Reserve04                  ;

} PPM_FUNCTION_ENABLES;


//
// PPM Turbo settings
//
typedef struct _PPM_TURBO_SETTINGS {
  UINT16  PowerLimit1;
  UINT32  PowerLimit1Time;
  UINT16  PowerLimit2;
  UINT8   TurboPowerLimitLock;
} PPM_TURBO_SETTINGS;

//
// Platform Policy
//
typedef struct _PPM_PLATFORM_POLICY_PROTOCOL {
  UINT8                                 Revision;
  PPM_FUNCTION_ENABLES                  FunctionEnables;
  PPM_CUSTOM_VID_TABLE                  CustomVidTable;
  PPM_TURBO_SETTINGS                    TurboSettings;

  UINT8                                 Reserve00;
  UINT8                                 Reserve01;
  UINT8                                 Reserve02;
  UINT8                                 Reserve03;
  UINT8                                 Reserve04;
  UINT8                                 Reserve05;
  UINT8                                 Reserve06;

  UINT8                                 S3RestoreMsrSwSmiNumber;
  UINT8                                 Reserve07;
  UINT32                                Reserve08;
  UINT8                                 Reserve09;
  //
  // Primary and Secondary Plane Current Limits
  //
  UINT16                                Reserve10;
  UINT8                                 Reserve11;
} PPM_PLATFORM_POLICY_PROTOCOL;

#endif
