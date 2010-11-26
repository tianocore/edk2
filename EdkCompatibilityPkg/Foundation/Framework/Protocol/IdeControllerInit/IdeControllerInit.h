/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    IdeControllerInit.h
    
Abstract:

    EFI Platform IDE Init Protocol

Revision History
        
        0.01 - Draft, 5-01-2002
        Add Atapi6 Identify structure definition, 8-14-2002

--*/

#ifndef _EFI_IDE_CONTROLLER_INIT_PROTOCOL_H
#define _EFI_IDE_CONTROLLER_INIT_PROTOCOL_H

//
// Global ID for the EFI Platform IDE Protocol GUID
//
#define EFI_IDE_CONTROLLER_INIT_PROTOCOL_GUID \
  { 0xa1e37052, 0x80d9, 0x4e65, {0xa3, 0x17, 0x3e, 0x9a, 0x55, 0xc4, 0x3e, 0xc9} }

////////////////////////////////////////////////////////////////////////////////////////
// Forward reference, ANSI compatability
//
EFI_FORWARD_DECLARATION (EFI_IDE_CONTROLLER_INIT_PROTOCOL);

//
//////////////////////////////////////////////////////////////////////////////////////////
// EFI_IDE_BUS_ENUMERATION_PHASE
// EFI_IDE_CONTROLLER_ENUM_PHASE
//
typedef enum{
  EfiIdeBeforeChannelEnumeration,
  EfiIdeAfterChannelEnumeration,
  EfiIdeBeforeChannelReset,
  EfiIdeAfterChannelReset,
  EfiIdeBusBeforeDevicePresenceDetection,
  EfiIdeBusAfterDevicePresenceDetection,
  EfiIdeResetMode,
  EfiIdeBusPhaseMaximum
} EFI_IDE_CONTROLLER_ENUM_PHASE;

//
//******************************************************
// EFI_ATA_EXT_TRANSFER_PROTOCOL
//******************************************************
//
// This extended mode describes the SATA physical protocol.
// SATA physical layers can operate at different speeds. 
// These speeds are defined below. Various PATA protocols 
// and associated modes are not applicable to SATA devices.
//

typedef enum {
  EfiAtaSataTransferProtocol  
} EFI_ATA_EXT_TRANSFER_PROTOCOL;

#define  EFI_SATA_AUTO_SPEED  0
#define  EFI_SATA_GEN1_SPEED  1
#define  EFI_SATA_GEN2_SPEED  2

//
//*******************************************************
// EFI_IDE_CABLE_TYPE
//*******************************************************
//
typedef enum {
  EfiIdeCableTypeUnknown,
  EfiIdeCableType40pin,
  EfiIdeCableType80Pin,
  EfiIdeCableTypeSerial,
  EfiIdeCableTypeMaximum
} EFI_IDE_CABLE_TYPE;

//
//******************************************************
// EFI_ATA_MODE
//******************************************************
//
typedef struct {
  BOOLEAN  Valid;
  UINT32       Mode; 
} EFI_ATA_MODE;

//
//******************************************************
// EFI_ATA_EXTENDED_MODE
//******************************************************
//
typedef struct {
  EFI_ATA_EXT_TRANSFER_PROTOCOL  TransferProtocol;
  UINT32                         Mode;
} EFI_ATA_EXTENDED_MODE;

//
//******************************************************
// EFI_ATA_COLLECTIVE_MODE
//******************************************************
//
typedef struct {
  EFI_ATA_MODE           PioMode; 
  EFI_ATA_MODE           SingleWordDmaMode;
  EFI_ATA_MODE           MultiWordDmaMode;
  EFI_ATA_MODE           UdmaMode;
  UINT32                         ExtModeCount;
  EFI_ATA_EXTENDED_MODE  ExtMode[1]; 
} EFI_ATA_COLLECTIVE_MODE;

//
//*******************************************************
// EFI_ATA_IDENTIFY_DATA
//*******************************************************
//

#pragma pack(1)

typedef struct {   
  UINT16  config;             // General Configuration
  UINT16  cylinders;          // Number of Cylinders
  UINT16  reserved_2;
  UINT16  heads;              //Number of logical heads
  UINT16  vendor_data1;
  UINT16  vendor_data2;
  UINT16  sectors_per_track;
  UINT16  vendor_specific_7_9[3];
  CHAR8   SerialNo[20];       // ASCII 
  UINT16  vendor_specific_20_21[2]; 
  UINT16  ecc_bytes_available;   
  CHAR8   FirmwareVer[8];     // ASCII 
  CHAR8   ModelName[40];      // ASCII   
  UINT16  multi_sector_cmd_max_sct_cnt;
  UINT16  reserved_48;
  UINT16  capabilities;
  UINT16  reserved_50;    
  UINT16  pio_cycle_timing;   
  UINT16  reserved_52;            
  UINT16  field_validity;    
  UINT16  current_cylinders;
  UINT16  current_heads;
  UINT16  current_sectors;   
  UINT16  CurrentCapacityLsb;
  UINT16  CurrentCapacityMsb;    
  UINT16  reserved_59;    
  UINT16  user_addressable_sectors_lo;
  UINT16  user_addressable_sectors_hi;
  UINT16  reserved_62;    
  UINT16  multi_word_dma_mode;   
  UINT16  advanced_pio_modes;
  UINT16  min_multi_word_dma_cycle_time;
  UINT16  rec_multi_word_dma_cycle_time;
  UINT16  min_pio_cycle_time_without_flow_control;
  UINT16  min_pio_cycle_time_with_flow_control;
  UINT16  reserved_69_79[11];    
  UINT16  major_version_no;
  UINT16  minor_version_no;
  UINT16  command_set_supported_82; // word 82
  UINT16  command_set_supported_83; // word 83
  UINT16  command_set_feature_extn; // word 84
  UINT16  command_set_feature_enb_85; // word 85
  UINT16  command_set_feature_enb_86; // word 86
  UINT16  command_set_feature_default; // word 87
  UINT16  ultra_dma_mode; // word 88
  UINT16  reserved_89_105[17];
  UINT16  phy_logic_sector_support; // word 106
  UINT16  reserved_107_116[10];
  UINT16  logic_sector_size_lo; // word 117
  UINT16  logic_sector_size_hi; // word 118
  UINT16  reserved_119_127[9];
  UINT16  security_status;
  UINT16  vendor_data_129_159[31];
  UINT16  reserved_160_208[49];
  UINT16  alignment_logic_in_phy_blocks; // word 209
  UINT16  reserved_210_255[46];
} EFI_ATA_IDENTIFY_DATA;

#pragma pack()
//
//*******************************************************
// EFI_ATAPI_IDENTIFY_DATA
//*******************************************************
//
#pragma pack(1)
typedef struct {
    UINT16  config;             // General Configuration
    UINT16  obsolete_1;
    UINT16  specific_config;
    UINT16  obsolete_3;   
    UINT16  retired_4_5[2];
    UINT16  obsolete_6;   
    UINT16  cfa_reserved_7_8[2];
    UINT16  retired_9;
    CHAR8   SerialNo[20];       // ASCII 
    UINT16  retired_20_21[2];
    UINT16  obsolete_22;
    CHAR8   FirmwareVer[8];     // ASCII 
    CHAR8   ModelName[40];      // ASCII 
    UINT16  multi_sector_cmd_max_sct_cnt;
    UINT16  reserved_48;
    UINT16  capabilities_49;
    UINT16  capabilities_50;
    UINT16  obsolete_51_52[2];   
    UINT16  field_validity;
    UINT16  obsolete_54_58[5];
    UINT16  mutil_sector_setting;
    UINT16  user_addressable_sectors_lo;
    UINT16  user_addressable_sectors_hi;
    UINT16  obsolete_62;
    UINT16  multi_word_dma_mode;
    UINT16  advanced_pio_modes;
    UINT16  min_multi_word_dma_cycle_time;
    UINT16  rec_multi_word_dma_cycle_time;
    UINT16  min_pio_cycle_time_without_flow_control;
    UINT16  min_pio_cycle_time_with_flow_control;
    UINT16  reserved_69_74[6];
    UINT16  queue_depth;
    UINT16  reserved_76_79[4];
    UINT16  major_version_no;
    UINT16  minor_version_no;
    UINT16  cmd_set_support_82;
    UINT16  cmd_set_support_83;
    UINT16  cmd_feature_support;
    UINT16  cmd_feature_enable_85;
    UINT16  cmd_feature_enable_86;
    UINT16  cmd_feature_default;
    UINT16  ultra_dma_select;
    UINT16  time_required_for_sec_erase;
    UINT16  time_required_for_enhanced_sec_erase;
    UINT16  current_advanced_power_mgmt_value;
    UINT16  master_pwd_revison_code;
    UINT16  hardware_reset_result;
    UINT16  current_auto_acoustic_mgmt_value;
    UINT16  reserved_95_99[5];
    UINT16  max_user_lba_for_48bit_addr[4];
    UINT16  reserved_104_126[23];
    UINT16  removable_media_status_notification_support;
    UINT16  security_status;
    UINT16  vendor_data_129_159[31];
    UINT16  cfa_power_mode;
    UINT16  cfa_reserved_161_175[15];
    UINT16  current_media_serial_no[30];
    UINT16  reserved_206_254[49];
    UINT16  integrity_word;
} EFI_ATAPI_IDENTIFY_DATA;

#pragma pack()
//
//*******************************************************
// EFI_IDENTIFY_DATA
//*******************************************************
//
typedef union {
  EFI_ATA_IDENTIFY_DATA       AtaData;
  EFI_ATAPI_IDENTIFY_DATA    AtapiData;
} EFI_IDENTIFY_DATA; 

#define   EFI_ATAPI_DEVICE_IDENTIFY_DATA  0x8000

//
/////////////////////////////////////////////////////////////////////////////////////////
// Function prototype declaration, for ANSI compatability
//
typedef 
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_GET_CHANNEL_INFO) (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL *This,
  IN  UINT8                           Channel, 
  OUT BOOLEAN                         *Enabled,
  OUT UINT8                           *MaxDevices

);

typedef 
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_NOTIFY_PHASE) (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN EFI_IDE_CONTROLLER_ENUM_PHASE     Phase,
  IN UINT8                             Channel
);

typedef 
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_SUBMIT_DATA) (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL *This,
  IN  UINT8                           Channel,
  IN  UINT8                           Device,
  IN  EFI_IDENTIFY_DATA               *IdentifyData
);


typedef
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_DISQUALIFY_MODE) (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL *This,
  IN  UINT8                           Channel,
  IN  UINT8                           Device,
  IN  EFI_ATA_COLLECTIVE_MODE         *BadModes
);

typedef
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_CALCULATE_MODE) (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL *This,
  IN  UINT8                           Channel,
  IN  UINT8                           Device,
  OUT EFI_ATA_COLLECTIVE_MODE         **SupportedModes
);

//
// ?? What happen to EFI_IDE_CONTROLLER_SET_TIMING ???
//
typedef
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_SET_TIMING) (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL *This,
  IN  UINT8                           Channel,
  IN  UINT8                           Device,
  IN  EFI_ATA_COLLECTIVE_MODE         *Modes
);

//
////////////////////////////////////////////////////////////////////////////////////////////////////
// Interface structure 
// EFI_IDE_CONTROLLER_INIT_PROTOCOL protocol provides the chipset specific information to the IDE bus driver.
// An IDE Bus driver wants to manage an IDE bus and possible IDE devices will have to retrieve the 
// EFI_IDE_CONTROLLER_INIT_PROTOCOL instances.
//
struct _EFI_IDE_CONTROLLER_INIT_PROTOCOL {
  EFI_IDE_CONTROLLER_GET_CHANNEL_INFO    GetChannelInfo;
  EFI_IDE_CONTROLLER_NOTIFY_PHASE        NotifyPhase;
  EFI_IDE_CONTROLLER_SUBMIT_DATA         SubmitData;
  EFI_IDE_CONTROLLER_DISQUALIFY_MODE     DisqualifyMode;
  EFI_IDE_CONTROLLER_CALCULATE_MODE      CalculateMode;
  EFI_IDE_CONTROLLER_SET_TIMING          SetTiming;
  BOOLEAN                                                        EnumAll;
  UINT8                                                               ChannelCount; 
};


extern EFI_GUID gEfiIdeControllerInitProtocolGuid;

#endif
