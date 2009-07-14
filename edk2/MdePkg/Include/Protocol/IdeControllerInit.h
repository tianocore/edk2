/** @file
  This file declares EFI IDE Controller Init Protocol
  
  The EFI_IDE_CONTROLLER_INIT_PROTOCOL provides the chipset-specific information
  to the IDE bus driver. This protocol is mandatory for IDE controllers if the
  IDE devices behind the controller are to be enumerated by an IDE bus driver.
  
  There can only be one instance of EFI_IDE_CONTROLLER_INIT_PROTOCOL for each IDE
  controller in a system. It is installed on the handle that corresponds to the
  IDE controller. An IDE bus driver that wishes to manage an IDE bus and possibly
  IDE devices in a system will have to retrieve the EFI_IDE_CONTROLLER_INIT_PROTOCOL
  instance that is associated with the controller to be managed.
  
  A device handle for an IDE controller must contain an EFI_DEVICE_PATH_PROTOCOL.

  Copyright (c) 2007 - 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This Protocol is defined in UEFI Platform Initialization Specification 1.2 
  Volume 5: Standards
  
**/

#ifndef _EFI_IDE_CONTROLLER_INIT_PROTOCOL_H_
#define _EFI_IDE_CONTROLLER_INIT_PROTOCOL_H_

///
/// Global ID for the EFI_IDE_CONTROLLER_INIT_PROTOCOL
///
#define EFI_IDE_CONTROLLER_INIT_PROTOCOL_GUID \
  { \
    0xa1e37052, 0x80d9, 0x4e65, {0xa3, 0x17, 0x3e, 0x9a, 0x55, 0xc4, 0x3e, 0xc9 } \
  }

///
/// Forward declaration for EFI_IDE_CONTROLLER_INIT_PROTOCOL
///
typedef struct _EFI_IDE_CONTROLLER_INIT_PROTOCOL  EFI_IDE_CONTROLLER_INIT_PROTOCOL;

///
/// The phase of the IDE Controller enumeration
///
typedef enum {
  ///
  /// The IDE bus driver is about to begin enumerating the devices
  /// behind the specified channel. This notification can be used to
  /// perform any chipset-specific programming.
  ///
  EfiIdeBeforeChannelEnumeration,
  ///
  /// The IDE bus driver has completed enumerating the devices
  /// behind the specified channel. This notification can be used to
  /// perform any chipset-specific programming.
  ///
  EfiIdeAfterChannelEnumeration,
  ///
  /// The IDE bus driver is about to reset the devices behind the
  /// specified channel. This notification can be used to perform any
  /// chipset-specific programming.
  ///
  EfiIdeBeforeChannelReset,
  ///
  /// The IDE bus driver has completed resetting the devices behind
  /// the specified channel. This notification can be used to perform
  /// any chipset-specific programming.  
  ///
  EfiIdeAfterChannelReset,
  ///
  /// The IDE bus driver is about to detect the presence of devices
  /// behind the specified channel. This notification can be used to
  /// set up the bus signals to default levels or for implementing
  /// predelays.
  ///
  EfiIdeBusBeforeDevicePresenceDetection,
  ///
  /// The IDE bus driver is done with detecting the presence of
  /// devices behind the specified channel. This notification can be
  /// used to perform any chipset-specific programming.  
  ///
  EfiIdeBusAfterDevicePresenceDetection,
  ///
  /// The IDE bus is requesting the IDE controller driver to
  /// reprogram the IDE controller hardware and thereby reset all
  /// the mode and timing settings to default settings.  
  ///
  EfiIdeResetMode,
  EfiIdeBusPhaseMaximum
} EFI_IDE_CONTROLLER_ENUM_PHASE;

///
/// This extended mode describes the SATA physical protocol.
/// SATA physical layers can operate at different speeds.
/// These speeds are defined below. Various PATA protocols
/// and associated modes are not applicable to SATA devices.
///
typedef enum {
  EfiAtaSataTransferProtocol
} EFI_ATA_EXT_TRANSFER_PROTOCOL;

///
/// Automatically detects the optimum SATA speed.
///
#define  EFI_SATA_AUTO_SPEED  0

///
/// Indicates a first-generation (Gen1) SATA speed.
///
#define  EFI_SATA_GEN1_SPEED  1

///
/// Indicates a second-generation (Gen2) SATA speed.
///
#define  EFI_SATA_GEN2_SPEED  2

///
/// EFI_ATA_MODE structure
///
typedef struct {
  BOOLEAN      Valid;   ///< TRUE if Mode is valid.
  UINT32       Mode;    ///< The actual ATA mode. This field is not a bit map.
} EFI_ATA_MODE;

///
/// EFI_ATA_EXTENDED_MODE structure
///
typedef struct {
  ///
  /// An enumeration defining various transfer protocols other than the protocols
  /// that exist at the time this specification was developed (i.e., PIO, single 
  /// word DMA, multiword DMA, and UDMA). Each transfer protocol is associated 
  /// with a mode. The various transfer protocols are defined by the ATA/ATAPI 
  /// specification. This enumeration makes the interface extensible because we 
  /// can support new transport protocols beyond UDMA. Type EFI_ATA_EXT_TRANSFER_PROTOCOL 
  /// is defined below.
  ///
  EFI_ATA_EXT_TRANSFER_PROTOCOL  TransferProtocol;
  ///
  /// The mode for operating the transfer protocol that is identified by TransferProtocol.
  ///
  UINT32                         Mode;
} EFI_ATA_EXTENDED_MODE;

///
/// EFI_ATA_COLLECTIVE_MODE structure
///
typedef struct {
  ///
  /// This field specifies the PIO mode. PIO modes are defined in the ATA/ATAPI
  /// specification. The ATA/ATAPI specification defines the enumeration.  In 
  /// other words, a value of 1 in this field means PIO mode 1. The actual meaning
  /// of PIO mode 1 is governed by the ATA/ATAPI specification. Type EFI_ATA_MODE
  /// is defined below.
  ///
  EFI_ATA_MODE           PioMode;
  ///
  /// This field specifies the single word DMA mode. Single word DMA modes are defined
  /// in the ATA/ATAPI specification, versions 1 and 2. Single word DMA support was
  /// obsoleted in the ATA/ATAPI specification, version 3; therefore, most devices and
  /// controllers will not support this transfer mode. The ATA/ATAPI specification defines
  /// the enumeration. In other words, a value of 1 in this field means single word DMA
  /// mode 1. The actual meaning of single word DMA mode 1 is governed by the ATA/
  /// ATAPI specification.  
  ///
  EFI_ATA_MODE           SingleWordDmaMode;
  ///
  /// This field specifies the multiword DMA mode. Various multiword DMA modes are
  /// defined in the ATA/ATAPI specification. A value of 1 in this field means multiword
  /// DMA mode 1. The actual meaning of multiword DMA mode 1 is governed by the
  /// ATA/ATAPI specification.  
  ///
  EFI_ATA_MODE           MultiWordDmaMode;
  ///
  /// This field specifies the ultra DMA (UDMA) mode. UDMA modes are defined in the
  /// ATA/ATAPI specification. A value of 1 in this field means UDMA mode 1. The
  /// actual meaning of UDMA mode 1 is governed by the ATA/ATAPI specification.  
  ///
  EFI_ATA_MODE           UdmaMode;
  ///
  /// The number of extended-mode bitmap entries. Extended modes describe transfer
  /// protocols beyond PIO, single word DMA, multiword DMA, and UDMA. This field
  /// can be zero and provides extensibility.  
  ///
  UINT32                 ExtModeCount;
  ///
  /// ExtModeCount number of entries. Each entry represents a transfer protocol other
  /// than the ones defined above (i.e., PIO, single word DMA, multiword DMA, and
  /// UDMA). This field is defined for extensibility. At this time, only one extended
  /// transfer protocol is defined to cover SATA transfers. Type
  /// EFI_ATA_EXTENDED_MODE is defined below.  
  ///
  EFI_ATA_EXTENDED_MODE  ExtMode[1];
} EFI_ATA_COLLECTIVE_MODE;

///
/// EFI_ATA_IDENTIFY_DATA structure
///
/// This structure definition is not part of the protocol
/// definition because the ATA/ATAPI Specification controls
/// the definition of all the fields. The ATA/ATAPI
/// Specification can obsolete old fields or redefine existing
/// fields. This definition is provided here for reference only.
///
#pragma pack(1)
typedef struct {
  UINT16  config;                 ///< General Configuration
  UINT16  cylinders;              ///< Number of Cylinders
  UINT16  reserved_2;
  UINT16  heads;                  ///< Number of logical heads
  UINT16  vendor_data1;
  UINT16  vendor_data2;
  UINT16  sectors_per_track;
  UINT16  vendor_specific_7_9[3];
  CHAR8   SerialNo[20];           ///< ASCII
  UINT16  vendor_specific_20_21[2];
  UINT16  ecc_bytes_available;
  CHAR8   FirmwareVer[8];         ///< ASCII
  CHAR8   ModelName[40];          ///< ASCII
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
  UINT16  command_set_supported_82;   ///< word 82
  UINT16  command_set_supported_83;   ///< word 83
  UINT16  command_set_feature_extn;   ///< word 84
  UINT16  command_set_feature_enb_85; ///< word 85
  UINT16  command_set_feature_enb_86; ///< word 86
  UINT16  command_set_feature_default; ///< word 87
  UINT16  ultra_dma_mode;             ///< word 88
  UINT16  reserved_89_105[17];
  UINT16  phy_logic_sector_support;   ///< word 106
  UINT16  reserved_107_116[10];
  UINT16  logic_sector_size_lo;       ///< word 117
  UINT16  logic_sector_size_hi;       ///< word 118
  UINT16  reserved_119_127[9];
  UINT16  security_status;
  UINT16  vendor_data_129_159[31];
  UINT16  reserved_160_208[49];
  UINT16  alignment_logic_in_phy_blocks; ///< word 209
  UINT16  reserved_210_255[46];
} EFI_ATA_IDENTIFY_DATA;
#pragma pack()

///
/// EFI_ATAPI_IDENTIFY_DATA structure
///
/// This structure definition is not part of the protocol
/// definition because the ATA/ATAPI Specification controls
/// the definition of all the fields. The ATA/ATAPI
/// Specification can obsolete old fields or redefine existing
/// fields. This definition is provided here for reference only.
///
#pragma pack(1)
typedef struct {
    UINT16  config;             ///< General Configuration
    UINT16  obsolete_1;
    UINT16  specific_config;
    UINT16  obsolete_3;
    UINT16  retired_4_5[2];
    UINT16  obsolete_6;
    UINT16  cfa_reserved_7_8[2];
    UINT16  retired_9;
    CHAR8   SerialNo[20];       ///< ASCII
    UINT16  retired_20_21[2];
    UINT16  obsolete_22;
    CHAR8   FirmwareVer[8];     ///< ASCII
    CHAR8   ModelName[40];      ///< ASCII
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

///
/// This flag indicates whether the IDENTIFY data is a response from an ATA device
/// (EFI_ATA_IDENTIFY_DATA) or response from an ATAPI device 
/// (EFI_ATAPI_IDENTIFY_DATA).  According to the ATA/ATAPI specification,
/// EFI_IDENTIFY_DATA is for an ATA device if bit 15 of the Config field is zero. 
/// The Config field is common to both EFI_ATA_IDENTIFY_DATA and 
/// EFI_ATAPI_IDENTIFY_DATA.
///
#define   EFI_ATAPI_DEVICE_IDENTIFY_DATA  0x8000

///
/// EFI_IDENTIFY_DATA structure
///
typedef union {
  ///
  /// The data that is returned by an ATA device upon successful completion 
  /// of the ATA IDENTIFY_DEVICE command. 
  ///
  EFI_ATA_IDENTIFY_DATA       AtaData;
  ///
  /// The data that is returned by an ATAPI device upon successful completion
  /// of the ATA IDENTIFY_PACKET_DEVICE command.
  ///
  EFI_ATAPI_IDENTIFY_DATA     AtapiData;
} EFI_IDENTIFY_DATA;

/**
  Returns the information about the specified IDE channel.
  
  This function can be used to obtain information about a particular IDE channel.
  The IDE bus driver uses this information during the enumeration process. 
  
  If Enabled is set to FALSE, the IDE bus driver will not scan the channel. Note 
  that it will not prevent an operating system driver from scanning the channel.
  
  For most of today's controllers, MaxDevices will either be 1 or 2. For SATA 
  controllers, this value will always be 1. SATA configurations can contain SATA 
  port multipliers. SATA port multipliers behave like SATA bridges and can support
  up to 16 devices on the other side. If an SATA port out of the IDE controller 
  is connected to a port multiplier, MaxDevices will be set to the number of SATA 
  devices that the port multiplier supports. Because today's port multipliers 
  support up to 15 SATA devices, this number can be as large as 15. The IDE bus 
  driver is required to scan for the presence of port multipliers behind an SATA 
  controller and enumerate up to MaxDevices number of devices behind the port 
  multiplier.    
  
  In this context, the devices behind a port multiplier constitute a channel.  
  
  @param[in]  This         Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param[in]  Channel      Zero-based channel number.
  @param[out] Enabled      TRUE if this channel is enabled.  Disabled channels 
                           are not scanned to see if any devices are present.
  @param[out] MaxDevices   The maximum number of IDE devices that the bus driver
                           can expect on this channel.  For the ATA/ATAPI 
                           specification, version 6, this number will either be 
                           1 or 2. For Serial ATA (SATA) configurations with a 
                           port multiplier, this number can be as large as 15.

  @retval EFI_SUCCESS             Information was returned without any errors.
  @retval EFI_INVALID_PARAMETER   Channel is invalid (Channel >= ChannelCount).

**/
typedef
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_GET_CHANNEL_INFO)(
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  UINT8                             Channel,
  OUT BOOLEAN                           *Enabled,
  OUT UINT8                             *MaxDevices
  );

/**
  The notifications from the IDE bus driver that it is about to enter a certain
  phase of the IDE channel enumeration process.
  
  This function can be used to notify the IDE controller driver to perform 
  specific actions, including any chipset-specific initialization, so that the 
  chipset is ready to enter the next phase. Seven notification points are defined 
  at this time. 
  
  More synchronization points may be added as required in the future.  

  @param[in] This      Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param[in] Phase     The phase during enumeration.
  @param[in] Channel   Zero-based channel number.

  @retval EFI_SUCCESS             The notification was accepted without any errors.
  @retval EFI_NOT_SUPPORTED       Phase is not supported.
  @retval EFI_INVALID_PARAMETER   Channel is invalid (Channel >= ChannelCount).
  @retval EFI_NOT_READY           This phase cannot be entered at this time; for 
                                  example, an attempt was made to enter a Phase 
                                  without having entered one or more previous 
                                  Phase.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_NOTIFY_PHASE)(
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN EFI_IDE_CONTROLLER_ENUM_PHASE     Phase,
  IN UINT8                             Channel
  );

/**
  Submits the device information to the IDE controller driver.

  This function is used by the IDE bus driver to pass detailed information about 
  a particular device to the IDE controller driver. The IDE bus driver obtains 
  this information by issuing an ATA or ATAPI IDENTIFY_DEVICE command. IdentifyData
  is the pointer to the response data buffer. The IdentifyData buffer is owned 
  by the IDE bus driver, and the IDE controller driver must make a local copy 
  of the entire buffer or parts of the buffer as needed. The original IdentifyData 
  buffer pointer may not be valid when
  
    - EFI_IDE_CONTROLLER_INIT_PROTOCOL.CalculateMode() or
    - EFI_IDE_CONTROLLER_INIT_PROTOCOL.DisqualifyMode() is called at a later point.
    
  The IDE controller driver may consult various fields of EFI_IDENTIFY_DATA to 
  compute the optimum mode for the device. These fields are not limited to the 
  timing information. For example, an implementation of the IDE controller driver 
  may examine the vendor and type/mode field to match known bad drives.  
  
  The IDE bus driver may submit drive information in any order, as long as it 
  submits information for all the devices belonging to the enumeration group 
  before EFI_IDE_CONTROLLER_INIT_PROTOCOL.CalculateMode() is called for any device
  in that enumeration group. If a device is absent, EFI_IDE_CONTROLLER_INIT_PROTOCOL.SubmitData()
  should be called with IdentifyData set to NULL.  The IDE controller driver may 
  not have any other mechanism to know whether a device is present or not. Therefore, 
  setting IdentifyData to NULL does not constitute an error condition. 
  EFI_IDE_CONTROLLER_INIT_PROTOCOL.SubmitData() can be called only once for a 
  given (Channel, Device) pair.  
    
  @param[in] This           Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param[in] Channel        Zero-based channel number.
  @param[in] Device         Zero-based device number on the Channel.
  @param[in] IdentifyData   The device's response to the ATA IDENTIFY_DEVICE command.

  @retval EFI_SUCCESS             The information was accepted without any errors.
  @retval EFI_INVALID_PARAMETER   Channel is invalid (Channel >= ChannelCount).
  @retval EFI_INVALID_PARAMETER   Device is invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_SUBMIT_DATA)(
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN UINT8                             Channel,
  IN UINT8                             Device,
  IN EFI_IDENTIFY_DATA                 *IdentifyData
  );

/**
  Disqualifies specific modes for an IDE device.

  This function allows the IDE bus driver or other drivers (such as platform 
  drivers) to reject certain timing modes and request the IDE controller driver
  to recalculate modes. This function allows the IDE bus driver and the IDE 
  controller driver to negotiate the timings on a per-device basis. This function 
  is useful in the case of drives that lie about their capabilities. An example 
  is when the IDE device fails to accept the timing modes that are calculated 
  by the IDE controller driver based on the response to the Identify Drive command.

  If the IDE bus driver does not want to limit the ATA timing modes and leave that 
  decision to the IDE controller driver, it can either not call this function for 
  the given device or call this function and set the Valid flag to FALSE for all 
  modes that are listed in EFI_ATA_COLLECTIVE_MODE.
  
  The IDE bus driver may disqualify modes for a device in any order and any number 
  of times.
  
  This function can be called multiple times to invalidate multiple modes of the 
  same type (e.g., Programmed Input/Output [PIO] modes 3 and 4). See the ATA/ATAPI 
  specification for more information on PIO modes.  
  
  For Serial ATA (SATA) controllers, this member function can be used to disqualify
  a higher transfer rate mode on a given channel. For example, a platform driver
  may inform the IDE controller driver to not use second-generation (Gen2) speeds 
  for a certain SATA drive.
  
  @param[in] This       Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param[in] Channel    Zero-based channel number.
  @param[in] Device     Zero-based device number on the Channel.
  @param[in] BadModes   The modes that the device does not support and that
                        should be disqualified.

  @retval EFI_SUCCESS             The modes were accepted without any errors.
  @retval EFI_INVALID_PARAMETER   Channel is invalid (Channel >= ChannelCount).
  @retval EFI_INVALID_PARAMETER   Device is invalid.
  @retval EFI_INVALID_PARAMETER   IdentifyData is NULL.
                                
**/
typedef
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_DISQUALIFY_MODE)(
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN UINT8                             Channel,
  IN UINT8                             Device,
  IN EFI_ATA_COLLECTIVE_MODE           *BadModes
  );

/**
  Returns the information about the optimum modes for the specified IDE device.

  This function is used by the IDE bus driver to obtain the optimum ATA modes for
  a specific device.  The IDE controller driver takes into account the following 
  while calculating the mode:
    - The IdentifyData inputs to EFI_IDE_CONTROLLER_INIT_PROTOCOL.SubmitData()
    - The BadModes inputs to EFI_IDE_CONTROLLER_INIT_PROTOCOL.DisqualifyMode()

  The IDE bus driver is required to call EFI_IDE_CONTROLLER_INIT_PROTOCOL.SubmitData() 
  for all the devices that belong to an enumeration group before calling 
  EFI_IDE_CONTROLLER_INIT_PROTOCOL.CalculateMode() for any device in the same group.  
  
  The IDE controller driver will use controller- and possibly platform-specific 
  algorithms to arrive at SupportedModes.  The IDE controller may base its 
  decision on user preferences and other considerations as well. This function 
  may be called multiple times because the IDE bus driver may renegotiate the mode 
  with the IDE controller driver using EFI_IDE_CONTROLLER_INIT_PROTOCOL.DisqualifyMode().
    
  The IDE bus driver may collect timing information for various devices in any 
  order. The IDE bus driver is responsible for making sure that all the dependencies
  are satisfied; for example, the SupportedModes information for device A that 
  was previously returned may become stale after a call to 
  EFI_IDE_CONTROLLER_INIT_PROTOCOL.DisqualifyMode() for device B.
  
  The buffer SupportedModes is allocated by the callee because the caller does 
  not necessarily know the size of the buffer. The type EFI_ATA_COLLECTIVE_MODE 
  is defined in a way that allows for future extensibility and can be of variable 
  length. This memory pool should be deallocated by the caller when it is no 
  longer necessary.  
  
  The IDE controller driver for a Serial ATA (SATA) controller can use this 
  member function to force a lower speed (first-generation [Gen1] speeds on a 
  second-generation [Gen2]-capable hardware).  The IDE controller driver can 
  also allow the IDE bus driver to stay with the speed that has been negotiated 
  by the physical layer.
  
  @param[in]  This             Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param[in]  Channel          Zero-based channel number.
  @param[in]  Device           Zero-based device number on the Channel.
  @param[out] SupportedModes   The optimum modes for the device.

  @retval EFI_SUCCESS             SupportedModes was returned.
  @retval EFI_INVALID_PARAMETER   Channel is invalid (Channel >= ChannelCount).
  @retval EFI_INVALID_PARAMETER   Device is invalid. 
  @retval EFI_INVALID_PARAMETER   SupportedModes is NULL.
  @retval EFI_NOT_READY           Modes cannot be calculated due to a lack of 
                                  data.  This error may happen if 
                                  EFI_IDE_CONTROLLER_INIT_PROTOCOL.SubmitData() 
                                  and EFI_IDE_CONTROLLER_INIT_PROTOCOL.DisqualifyData() 
                                  were not called for at least one drive in the 
                                  same enumeration group.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_CALCULATE_MODE)(
  IN  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN  UINT8                             Channel,
  IN  UINT8                             Device,
  OUT EFI_ATA_COLLECTIVE_MODE           **SupportedModes
  );

/**
  Commands the IDE controller driver to program the IDE controller hardware
  so that the specified device can operate at the specified mode.

  This function is used by the IDE bus driver to instruct the IDE controller 
  driver to program the IDE controller hardware to the specified modes. This 
  function can be called only once for a particular device. For a Serial ATA 
  (SATA) Advanced Host Controller Interface (AHCI) controller, no controller-
  specific programming may be required.

  @param[in] This      Pointer to the EFI_IDE_CONTROLLER_INIT_PROTOCOL instance.
  @param[in] Channel   Zero-based channel number.
  @param[in] Device    Zero-based device number on the Channel.
  @param[in] Modes     The modes to set.

  @retval EFI_SUCCESS             The command was accepted without any errors.
  @retval EFI_INVALID_PARAMETER   Channel is invalid (Channel >= ChannelCount).
  @retval EFI_INVALID_PARAMETER   Device is invalid.
  @retval EFI_NOT_READY           Modes cannot be set at this time due to lack of data.
  @retval EFI_DEVICE_ERROR        Modes cannot be set due to hardware failure.
                                  The IDE bus driver should not use this device.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_IDE_CONTROLLER_SET_TIMING)(
  IN EFI_IDE_CONTROLLER_INIT_PROTOCOL  *This,
  IN UINT8                             Channel,
  IN UINT8                             Device,
  IN EFI_ATA_COLLECTIVE_MODE           *Modes
  );

///
/// Provides the basic interfaces to abstract an IDE controller.
///
struct _EFI_IDE_CONTROLLER_INIT_PROTOCOL {
  ///
  /// Returns the information about a specific channel.
  ///
  EFI_IDE_CONTROLLER_GET_CHANNEL_INFO    GetChannelInfo;
  
  ///
  /// The notification that the IDE bus driver is about to enter the
  /// specified phase during the enumeration process.  
  ///
  EFI_IDE_CONTROLLER_NOTIFY_PHASE        NotifyPhase;
  
  ///
  /// Submits the Drive Identify data that was returned by the device.
  ///
  EFI_IDE_CONTROLLER_SUBMIT_DATA         SubmitData;
  
  ///
  /// Submits information about modes that should be disqualified.  The specified 
  /// IDE device does not support these modes and these modes should not be 
  /// returned by EFI_IDE_CONTROLLER_INIT_PROTOCOL.CalculateMode()
  ///
  EFI_IDE_CONTROLLER_DISQUALIFY_MODE     DisqualifyMode;
  
  ///
  /// Calculates and returns the optimum mode for a particular IDE device.
  ///
  EFI_IDE_CONTROLLER_CALCULATE_MODE      CalculateMode;
  
  ///
  /// Programs the IDE controller hardware to the default timing or per the modes
  /// that were returned by the last call to EFI_IDE_CONTROLLER_INIT_PROTOCOL.CalculateMode().  
  ///
  EFI_IDE_CONTROLLER_SET_TIMING          SetTiming;
  
  ///
  /// Set to TRUE if the enumeration group includes all the channels that are
  /// produced by this controller. FALSE if an enumeration group consists of
  /// only one channel.  
  ///
  BOOLEAN                                EnumAll;
  
  ///
  /// The number of channels that are produced by this controller. Parallel ATA
  /// (PATA) controllers can support up to two channels. Advanced Host Controller 
  /// Interface (AHCI) Serial ATA (SATA) controllers can support up to 32 channels,
  /// each of which can have up to one device. In the presence of a multiplier, 
  /// each channel can have 15 devices.
  ///
  UINT8                                  ChannelCount;
};

extern EFI_GUID gEfiIdeControllerInitProtocolGuid;

#endif
