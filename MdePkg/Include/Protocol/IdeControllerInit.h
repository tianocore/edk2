/** @file
  This file declares EFI IDE Controller Init Protocol

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  IdeControllerInit.h

  @par Revision Reference:
  This Protocol is defined in IDE Controller Initialization Protocol Specification
  Version 0.9

**/

#ifndef _EFI_IDE_CONTROLLER_INIT_PROTOCOL_H
#define _EFI_IDE_CONTROLLER_INIT_PROTOCOL_H

//
// Global ID for the EFI Platform IDE Protocol GUID
//
#define EFI_IDE_CONTROLLER_INIT_PROTOCOL_GUID \
  { 0xa1e37052, 0x80d9, 0x4e65, {0xa3, 0x17, 0x3e, 0x9a, 0x55, 0xc4, 0x3e, 0xc9 } }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_IDE_CONTROLLER_INIT_PROTOCOL  EFI_IDE_CONTROLLER_INIT_PROTOCOL;

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
  BOOLEAN      Valid;
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
  UINT32                 ExtModeCount;
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
  UINT16  reserved_89_127[39];
  UINT16  security_status;
  UINT16  vendor_data_129_159[31];
  UINT16  reserved_160_255[96];
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
  EFI_ATAPI_IDENTIFY_DATA     AtapiData;
} EFI_IDENTIFY_DATA; 

#define   EFI_ATAPI_DEVICE_IDENTIFY_DATA  0x8000

//
/////////////////////////////////////////////////////////////////////////////////////////
// Function prototype declaration, for ANSI compatability
//
/**
  Returns the information about the specified IDE channel. 

  @param  This                  Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param  Channel               Zero-based channel number.
  @param  Enabled               TRUE if this channel is enabled. Disabled channels are not scanned
                                to see if any devices are present.
  @param  MaxDevices            The maximum number of IDE devices that the bus driver
                                can expect on this channel.

  @retval EFI_SUCCESS           Information was returned without any errors.
  @retval EFI_INVALID_PARAMETER Channel is invalid (Channel >= ChannelCount).

**/
typedef 
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_GET_CHANNEL_INFO) (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL *This,
  IN  UINT8                           Channel, 
  OUT BOOLEAN                         *Enabled,
  OUT UINT8                           *MaxDevices
);

/**
  The notifications from the IDE bus driver that it is about to enter a certain 
  phase of the IDE channel enumeration process. 

  @param  This                  Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param  Phase                 The phase during enumeration.
  @param  Channel               Zero-based channel number.

  @retval EFI_SUCCESS           The notification was accepted without any errors.
  @retval EFI_NOT_SUPPORTED     Phase is not supported.
  @retval EFI_INVALID_PARAMETER Channel is invalid (Channel >= ChannelCount).
  @retval EFI_NOT_READY         This phase cannot be entered at this time.

**/
typedef 
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_NOTIFY_PHASE) (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN EFI_IDE_CONTROLLER_ENUM_PHASE     Phase,
  IN UINT8                             Channel
);

/**
  Submits the device information to the IDE controller driver. 

  @param  This                  Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param  Channel               Zero-based channel number.
  @param  Device                Zero-based device number on the Channel.
  @param  IdentifyData          The device¡¯s response to the ATA IDENTIFY_DEVICE command.

  @retval EFI_SUCCESS           The information was accepted without any errors.
  @retval EFI_INVALID_PARAMETER Channel is invalid (Channel >= ChannelCount).
                                Or Device is invalid.

**/
typedef 
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_SUBMIT_DATA) (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL *This,
  IN  UINT8                           Channel,
  IN  UINT8                           Device,
  IN  EFI_IDENTIFY_DATA               *IdentifyData
);

/**
  Disqualifies specific modes for an IDE device. 

  @param  This                  Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param  Channel               Zero-based channel number.
  @param  Device                Zero-based device number on the Channel.
  @param  BadModes              The modes that the device does not support and that
                                should be disqualified.

  @retval EFI_SUCCESS           The modes were accepted without any errors.
  @retval EFI_INVALID_PARAMETER Channel is invalid (Channel >= ChannelCount).
                                Or Device is invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_DISQUALIFY_MODE) (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL *This,
  IN  UINT8                           Channel,
  IN  UINT8                           Device,
  IN  EFI_ATA_COLLECTIVE_MODE         *BadModes
);

/**
  Returns the information about the optimum modes for the specified IDE device.

  @param  This                  Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param  Channel               Zero-based channel number.
  @param  Device                Zero-based device number on the Channel.
  @param  SupportedModes        The optimum modes for the device.

  @retval EFI_SUCCESS           SupportedModes was returned.
  @retval EFI_INVALID_PARAMETER Channel is invalid (Channel >= ChannelCount).
                                Or Device is invalid. Or SupportedModes is NULL.
  @retval EFI_NOT_READY         Modes cannot be calculated due to a lack of data.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_CALCULATE_MODE) (
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL *This,
  IN  UINT8                           Channel,
  IN  UINT8                           Device,
  OUT EFI_ATA_COLLECTIVE_MODE         **SupportedModes
);

/**
  Commands the IDE controller driver to program the IDE controller hardware 
  so that the specified device can operate at the specified mode. 

  @param  This                  Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param  Channel               Zero-based channel number.
  @param  Device                Zero-based device number on the Channel.
  @param  Modes                 The modes to set.

  @retval EFI_SUCCESS           The command was accepted without any errors.
  @retval EFI_INVALID_PARAMETER Channel is invalid (Channel >= ChannelCount).
                                Or Device is invalid.
  @retval EFI_NOT_READY         Modes cannot be set at this time due to lack of data.
  @retval EFI_DEVICE_ERROR      Modes cannot be set due to hardware failure.
                                The IDE bus driver should not use this device.

**/
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
/**
  @par Protocol Description:
  Provides the basic interfaces to abstract an IDE controller.

  @param GetChannelInfo
  Returns the information about a specific channel. 

  @param NotifyPhase
  The notification that the IDE bus driver is about to enter the 
  specified phase during the enumeration process. 

  @param SubmitData 
  Submits the Drive Identify data that was returned by the device. 

  @param DisqualifyMode 
  Submits information about modes that should be disqualified. 

  @param CalculateMode 
  Calculates and returns the optimum mode for a particular IDE device.

  @param SetTiming 
  Programs the IDE controller hardware to the default timing or per the modes 
  that were returned by the last call to CalculateMode().

  @param EnumAll 
  Set to TRUE if the enumeration group includes all the channels that are 
  produced by this controller. FALSE if an enumeration group consists of 
  only one channel. 

  @param ChannelCount
  The number of channels that are produced by this controller.

**/
struct _EFI_IDE_CONTROLLER_INIT_PROTOCOL {
  EFI_IDE_CONTROLLER_GET_CHANNEL_INFO    GetChannelInfo;
  EFI_IDE_CONTROLLER_NOTIFY_PHASE        NotifyPhase;
  EFI_IDE_CONTROLLER_SUBMIT_DATA         SubmitData;
  EFI_IDE_CONTROLLER_DISQUALIFY_MODE     DisqualifyMode;
  EFI_IDE_CONTROLLER_CALCULATE_MODE      CalculateMode;
  EFI_IDE_CONTROLLER_SET_TIMING          SetTiming;
  BOOLEAN                                EnumAll;
  UINT8                                  ChannelCount; 
};

extern EFI_GUID gEfiIdeControllerInitProtocolGuid;

#endif


