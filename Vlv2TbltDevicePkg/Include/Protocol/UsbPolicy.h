/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:
  UsbPolicy.h

Abstract:

--*/

#ifndef _USB_POLICY_H_
#define _USB_POLICY_H_

EFI_FORWARD_DECLARATION (EFI_USB_POLICY_PROTOCOL);

#define USB_POLICY_GUID \
  {\
    0xf617b358, 0x12cf, 0x414a, 0xa0, 0x69, 0x60, 0x67, 0x7b, 0xda, 0x13, 0xb4\
  }

#define TIANO_CODE_BASE           0x00
#define ICBD_CODE_BASE            0x01

#define ATUO_TYPE                 0x00
#define USB_FDD_TYPE              0x01
#define HDD_TYPE                  0x02
#define ZIP_TYPE                  0x03
#define CDROM_TYPE                0x04
#define SIZE_TYPE                 0x05

#define ZIP_FDD                 0x80

#define FDD_EMULATION             0x00
#define HDD_EMULATION             0x01

#define HIGH_SPEED                0x00
#define FULL_SPEED                0x01
#define SUPER_SPEED               0x02

#define LEGACY_KB_EN              0x01
#define LEGACY_KB_DIS             0x00
#define LEGACY_MS_EN              0x01
#define LEGACY_MS_DIS             0x00
#define LEGACY_USB_EN             0x00
#define LEGACY_USB_DIS            0x01
#define LEGACY_FREE_SUPP          0x01
#define LEGACY_FREE_UN_SUPP       0x00
#define LEGACY_PERIOD_SUPP        0x01
#define LEGACY_PERIOD_UN_SUPP     0x00

#define LEGACY_USB_TIME_TUE_ENABLE       0x01
#define LEGACY_USB_TIME_TUE_DISABLE      0x00
#define USB_HAVE_HUB_INTERNEL            0x01
#define USB_NOT_HAVE_HUB_INTERNEL        0x00

#define USB_POLICY_PROTOCOL_REVISION_1 1
#define USB_POLICY_PROTOCOL_REVISION_2 2

#ifndef __GNUC__
#pragma warning ( disable : 4306 )
#pragma warning ( disable : 4054 )
#endif

#define GET_USB_CFG (UsbCfg);\
 do{\
  UINT16                *pSegOfEbda;\
  UINT32                mToEbda;\
  pSegOfEbda = (UINT16 *)(UINTN)0x40E;\
  mToEbda    = (UINT32)(((UINTN)(*pSegOfEbda) << 4) + 0x80);\
  UsbCfg     = (USB_CFG *)(UINTN)mToEbda;\
 }while(0);

#pragma    pack(1)
typedef struct {
    UINT8   HasUSBKeyboard:1;
    UINT8   HasUSBMouse:1;
    UINT8   LegacyFreeSupport:1;
    UINT8   UsbOperationMode:1;
    UINT8   LegacyKBEnable:1;
    UINT8   LegacyMSEnable:1;
    UINT8   USBPeriodSupport:1;
    UINT8   Reserved:1;
} USB_DEVICE_INFOR;

typedef struct {
    UINT8               Codebase;
    UINT8               USBHDDForceType;
    UINT8               Configurated;
    UINT8               LpcAcpiBase;
    UINT8               AcpiTimerReg;
    UINT8               Reserved1[0x01];
    UINT8               LegacyUsbEnable;
    USB_DEVICE_INFOR    UsbDeviceInfor;
    UINT16              UsbEmulationSize;
    UINT8               Reserved2[0x06];
} USB_CFG;
#pragma pack()

typedef struct _EFI_USB_POLICY_PROTOCOL{
  UINT8   Version;
  UINT8   UsbMassStorageEmulationType;  // 1: FDD_Type; 2: HDD_Type; other:Auto_Type*
  UINT8   UsbOperationMode;             // 0: High_Speed; 1: Full_Speed;
  UINT8   LegacyKBEnable;               // 0: Disabled;   1: Enabled*
  UINT8   LegacyMSEnable;               // 0: Disabled;   1: Enabled*
  UINT8   USBPeriodSupport;             // 0; Unsupport;  1: Support
  UINT8   LegacyUsbEnable;              // 1: Disabled;   0: Enabled*
  UINT8   LegacyFreeSupport;            // 0: Unsupport;  1: Support
  UINT8   CodeBase;
  UINT8   LpcAcpiBase;                  // 40h(default)
  UINT8   AcpiTimerReg;
  UINT8   UsbTimeTue;
  UINT8   InternelHubExist;             // 1: Host have internel hub on board; 0: No internel hub on board
  UINT8   EnumWaitPortStableStall;      // Value for wait port stable when enum a new dev.
  UINT16  UsbEmulationSize;             // Mbytes.
  UINT8   UsbZipEmulationType;
  UINT8   Reserved[3];                  // Reserved fields for future expansion w/o protocol change
} EFI_USB_POLICY_PROTOCOL;

extern EFI_GUID gUsbPolicyGuid;

#endif
