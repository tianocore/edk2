/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FrontPage.h

Abstract:

  FrontPage routines to handle the callbacks and browser calls

Revision History

--*/

#ifndef _FRONT_PAGE_H
#define _FRONT_PAGE_H

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "Generic/DeviceMngr/DeviceManager.h"
#include "Generic/BootMaint/BootMaint.h"
#include "Generic/BootMngr/BootManager.h"

//
// This is the VFR compiler generated header file which defines the
// string identifiers.
//
#include "BdsStrDefs.h"
#define EFI_DISK_DEVICE_CLASS           0x01
#define EFI_VIDEO_DEVICE_CLASS          0x02
#define EFI_NETWORK_DEVICE_CLASS        0x04
#define EFI_INPUT_DEVICE_CLASS          0x08
#define EFI_ON_BOARD_DEVICE_CLASS       0x10
#define EFI_OTHER_DEVICE_CLASS          0x20
#define EFI_VBIOS_CLASS                 0x40

#define SET_VIDEO_BIOS_TYPE_QUESTION_ID 0x00

#pragma pack(1)
typedef struct {
  UINT8 VideoBIOS;
} MyDevMgrIfrNVData;
#pragma pack()

#define EFI_FP_CALLBACK_DATA_SIGNATURE  EFI_SIGNATURE_32 ('F', 'P', 'C', 'B')
#define EFI_FP_CALLBACK_DATA_FROM_THIS(a) \
  CR (a, \
      EFI_FRONTPAGE_CALLBACK_INFO, \
      DevMgrCallback, \
      EFI_FP_CALLBACK_DATA_SIGNATURE \
      )

typedef struct {
  UINTN                       Signature;
  MyDevMgrIfrNVData           Data;
  EFI_HII_HANDLE              DevMgrHiiHandle;
  EFI_HANDLE                  CallbackHandle;
  EFI_FORM_CALLBACK_PROTOCOL  DevMgrCallback;
} EFI_FRONTPAGE_CALLBACK_INFO;

//
// These are the VFR compiler generated data representing our VFR data.
//
// BugBug: we should put g in front of these tool generated globals.
//         maybe even gVrf would be a better prefix
//
extern UINT8  FrontPageVfrBin[];
extern UINT8  FrontPageStringsStr[];
extern UINT8  DeviceManagerVfrBin[];
extern UINT8  DeviceManagerStringsStr[];

#define FRONT_PAGE_QUESTION_ID  0x0000
#define FRONT_PAGE_DATA_WIDTH   0x01

EFI_STATUS
InitializeFrontPage (
  IN BOOLEAN    ReInitializeStrings
  );

BOOLEAN
TimeCompare (
  IN EFI_TIME               *FirstTime,
  IN EFI_TIME               *SecondTime
  );

VOID
PlatformBdsEnterFrontPage (
  IN UINT16                 TimeoutDefault,
  IN BOOLEAN                ConnectAllHappened
  );

#endif // _FRONT_PAGE_H_

