/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  LpcWpc83667Policy.h

Abstract:

  Protocol used for WPC83627 Policy definition.
-------------------------------------------------------------------------------
   Rev   Date<MM/DD/YYYY>    Name    Description
  -------------------------------------------------------------------------------
  R01   < 4/22/2011>         LB     Update driver for Sio83627UGH support.
  -------------------------------------------------------------------------------
**/

#ifndef _WPC83627_POLICY_PROTOCOL_H_
#define _WPC83627_POLICY_PROTOCOL_H_

EFI_FORWARD_DECLARATION (EFI_WPC83627_POLICY_PROTOCOL);

#define EFI_WPC83627_POLICY_PROTOCOL_GUID \
  { \
    0xd3ecc567, 0x9fd5, 0x44c1, 0x86, 0xcf, 0x5d, 0xa7, 0xa2, 0x4f, 0x4b, 0x5d \
  }

#define EFI_WPC83627_COM1_ENABLE          0x01
#define EFI_WPC83627_COM2_ENABLE          0x01

#define EFI_WPC83627_COM3_ENABLE          0x01
#define EFI_WPC83627_COM4_ENABLE          0x01

#define EFI_WPC83627_LPT1_ENABLE          0x01
#define EFI_WPC83627_LPT1_ENABLE          0x01
#define EFI_WPC83627_FDD_ENABLE           0x01
#define EFI_WPC83627_FDD_WRITE_ENABLE     0x01
#define EFI_WPC83627_PS2_KBC_ENABLE       0x01
#define EFI_WPC83627_ECIR_ENABLE	  0x01

#define EFI_WPC83627_COM1_DISABLE         0x00
#define EFI_WPC83627_COM2_DISABLE         0x00

#define EFI_WPC83627_COM3_DISABLE         0x00
#define EFI_WPC83627_COM4_DISABLE         0x00

#define EFI_WPC83627_LPT1_DISABLE         0x00
#define EFI_WPC83627_FDD_DISABLE          0x00
#define EFI_WPC83627_FDD_WRITE_PROTECT    0x00
#define EFI_WPC83627_PS2_KBC_DISABLE      0x00
#define EFI_WPC83627_ECIR_DISABLE         0x00
#define EFI_WPC83627_RESERVED_DEFAULT     0x00

typedef struct {
  UINT16  Com1               :1;             // 0 = Disable, 1 = Enable
  UINT16  Lpt1               :1;             // 0 = Disable, 1 = Enable
  UINT16  Floppy             :1;             // 0 = Disable, 1 = Enable
  UINT16  FloppyWriteProtect :1;             // 0 = Write Protect, 1 = Write Enable
  UINT16  Port80             :1;             // 0 = Disable, 1 = Enable
  UINT16  CIR                :1;             // CIR enable or disable
  UINT16  Ps2Keyboard        :1;             // 0 = Disable, 1 = Enable
  UINT16  Ps2Mouse           :1;             // 0 = Disable, 1 = Enable
  UINT16  Com2               :1;             // 0 = Disable, 1 = Enable

  UINT16  Com3               :1;             // 0 = Disable, 1 = Enable
  UINT16  Com4               :1;             // 0 = Disable, 1 = Enable

  UINT16  Dac                :1;             // 0 = Disable, 1 = Enable
  UINT16  Rsvd               :6;
} EFI_WPC83627_DEVICE_ENABLES;

typedef enum {
  LptModeOutput,
  LptModeBiDirectional,
  LptModeEpp,
  LptModeEcp
} EFI_LPT_MODE;

typedef struct _EFI_WPC83627_POLICY_PROTOCOL {
  EFI_WPC83627_DEVICE_ENABLES DeviceEnables;
  EFI_LPT_MODE              LptMode;
} EFI_WPC83627_POLICY_PROTOCOL;

extern EFI_GUID gEfiLpcWpc83627PolicyProtocolGuid;

#endif
