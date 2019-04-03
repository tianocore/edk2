/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:

  Asf.h

Abstract:

  Alert Standard Format address variable

--*/

#ifndef AlertStandardFormat_h_included
#define AlertStandardFormat_h_included


#pragma pack(1)

//
// ASF address
//
//
// {3D995FB4-4F05-4073-BE72-A19CFB5DE690}
//
#define  ALERT_STANDARD_FORMAT_VARIABLE_GUID \
  {0x3d995fb4, 0x4f05, 0x4073, 0xbe, 0x72, 0xa1, 0x9c, 0xfb, 0x5d, 0xe6, 0x90}

#define ALERT_STANDARD_FORMAT_VARIABLE_NAME (L"ASF")
#define ASCII_ALERT_STANDARD_FORMAT_VARIABLE_NAME ("ASF")

extern EFI_GUID gAlertStandardFormatGuid;
extern CHAR16   gAlertStandardFormatName[];

typedef struct {
  UINT8   SmbusAddr;
  struct {
    UINT32  VendorSpecificId;
    UINT16  SubsystemDeviceId;
    UINT16  SubsystemVendorId;
    UINT16  Interface;
    UINT16  DeviceId;
    UINT16  VendorId;
    UINT8   VendorRevision;
    UINT8   DeviceCapabilities;
  } Udid;
  struct {
    UINT8     SubCommand;
    UINT8     Version;
    UINT32    IanaId;
    UINT8     SpecialCommand;
    UINT16    SpecialCommandParam;
    UINT16    BootOptionsBits;
    UINT16    OemParam;
  } AsfBootOptions;
  struct {
    UINT8     Bus;
    UINT8     Device;
    UINT8     Function;
    UINT16    VendorId;
    UINT16    DeviceId;
    UINT16    IderCmdBar;
    UINT16    IderCtrlBar;
    UINT8     IderIrq;
    UINT16    SolBar;
    UINT8     SolIrq;
  } PciInfo;
  struct {
  UINT8   IamtProvisioningStatus;
  BOOLEAN IamtIsProvisioned;
  } IamtInfo;
  struct {
    BOOLEAN FlashUpdatingIsAllowed;
  } MeInfoForEbu;
  UINT32  EitBPFAddress;
} EFI_ASF_VARIABLE;

#pragma pack()

#endif

