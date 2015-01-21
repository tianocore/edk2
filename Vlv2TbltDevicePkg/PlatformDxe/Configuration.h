/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

    Configuration.h

Abstract:

    Driver configuration include file


--*/

#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#define EFI_NON_DEVICE_CLASS               0x00
#define EFI_DISK_DEVICE_CLASS              0x01
#define EFI_VIDEO_DEVICE_CLASS             0x02
#define EFI_NETWORK_DEVICE_CLASS           0x04
#define EFI_INPUT_DEVICE_CLASS             0x08
#define EFI_ON_BOARD_DEVICE_CLASS          0x10
#define EFI_OTHER_DEVICE_CLASS             0x20

//
// Processor labels
//
#define PROCESSOR_HT_MODE           0x0100
#define PROCESSOR_FSB_MULTIPLIER    0x0101
#define PROCESSOR_MULTIPLIER_OVERRIDE_CONTROL  0x0211

//
// Memory labels
//
#define MEMORY_SLOT1_SPEED          0x0200
#define MEMORY_SLOT2_SPEED          0x0201
#define MEMORY_SLOT3_SPEED          0x0202
#define MEMORY_SLOT4_SPEED          0x0203
#define END_MEMORY_SLOT_SPEED       0x020F
#define PERFORMANCE_MEMORY_PROFILE_CONTROL  0x0210
#define UCLK_RATIO_CONTROL          0x0212

//
// Language label
//
#define FRONT_PAGE_ITEM_LANGUAGE    0x300

//
// Boot Labels
//
#define BOOT_DEVICE_PRIORITY_BEGIN  0x0400
#define BOOT_DEVICE_PRIORITY_END    0x0401
#define BOOT_OPTICAL_DEVICE_BEGIN   0x0410
#define BOOT_OPTICAL_DEVICE_END     0x0411
#define BOOT_REMOVABLE_DEVICE_BEGIN 0x0420
#define BOOT_REMOVABLE_DEVICE_END   0x0421
#define BOOT_PXE_DEVICE_BEGIN       0x0430
#define BOOT_PXE_DEVICE_END         0x0431
#define BOOT_MENU_TYPE_BEGIN        0x0440
#define BOOT_MENU_TYPE_END          0x0441
#define BOOT_USB_DEVICE_BEGIN       0x0450
#define BOOT_USB_DEVICE_END         0x0451
#define BOOT_USB_FIRST_BEGIN        0x0460
#define BOOT_USB_FIRST_END          0x0461
#define BOOT_UEFI_BEGIN             0x0470
#define BOOT_UEFI_END               0x0471
#define BOOT_USB_UNAVAILABLE_BEGIN  0x0480
#define BOOT_USB_UNAVAILABLE_END    0x0481
#define BOOT_CD_UNAVAILABLE_BEGIN   0x0490
#define BOOT_CD_UNAVAILABLE_END     0x0491
#define BOOT_FDD_UNAVAILABLE_BEGIN  0x04A0
#define BOOT_FDD_UNAVAILABLE_END    0x04A1
#define BOOT_DEVICE_PRIORITY_DEFAULT_BEGIN  0x04B0
#define BOOT_DEVICE_PRIORITY_DEFAULT_END    0x04B1
#define BOOT_USB_OPT_LABEL_BEGIN    0x04C0
#define BOOT_USB_OPT_LABEL_END      0x04C1

#define VAR_EQ_ADMIN_NAME               0x0041  // A
#define VAR_EQ_ADMIN_DECIMAL_NAME       L"65"
#define VAR_EQ_VIEW_ONLY_NAME           0x0042  // B
#define VAR_EQ_VIEW_ONLY_DECIMAL_NAME   L"66"
#define VAR_EQ_CONFIG_MODE_NAME         0x0043  // C
#define VAR_EQ_CONFIG_MODE_DECIMAL_NAME L"67"
#define VAR_EQ_CPU_EE_NAME              0x0045  // E
#define VAR_EQ_CPU_EE_DECIMAL_NAME  L"69"
#define VAR_EQ_FLOPPY_MODE_NAME         0x0046  // F
#define VAR_EQ_FLOPPY_MODE_DECIMAL_NAME L"70"
#define VAR_EQ_HT_MODE_NAME             0x0048  // H
#define VAR_EQ_HT_MODE_DECIMAL_NAME     L"72"
#define VAR_EQ_AHCI_MODE_NAME           0x0049  // I
#define VAR_EQ_AHCI_MODE_DECIMAL_NAME   L"73"
#define VAR_EQ_CPU_LOCK_NAME            0x004C  // L
#define VAR_EQ_CPU_LOCK_DECIMAL_NAME    L"76"
#define VAR_EQ_NX_MODE_NAME             0x004E  // N
#define VAR_EQ_NX_MODE_DECIMAL_NAME     L"78"
#define VAR_EQ_RAID_MODE_NAME           0x0052  // R
#define VAR_EQ_RAID_MODE_DECIMAL_NAME   L"82"
#define VAR_EQ_1394_MODE_NAME           0x0054  // T
#define VAR_EQ_1394_MODE_DECIMAL_NAME   L"84"
#define VAR_EQ_USER_NAME                0x0055  // U
#define VAR_EQ_USER_DECIMAL_NAME        L"85"
#define VAR_EQ_VIDEO_MODE_NAME          0x0056  // V
#define VAR_EQ_VIDEO_MODE_DECIMAL_NAME  L"86"
#define VAR_EQ_LEGACY_FP_AUDIO_NAME     0x0057  // W
#define VAR_EQ_LEGACY_FP_AUDIO_DECIMAL_NAME L"87"
#define VAR_EQ_EM64T_CAPABLE_NAME       0x0058  // X
#define VAR_EQ_EM64T_CAPABLE_DECIMAL_NAME L"88"
#define VAR_EQ_BOARD_FORMFACTOR_NAME    0x0059  // Y
#define VAR_EQ_BOARD_FORMFACTOR_DECIMAL_NAME L"89"
#define VAR_EQ_UNCON_CPU_NAME           0x005B  // ??
#define VAR_EQ_UNCON_CPU_DECIMAL_NAME   L"91"
#define VAR_EQ_VAR_HIDE_NAME            0x005C  // ??
#define VAR_EQ_VAR_HIDE_DECIMAL_NAME    L"92"
#define VAR_EQ_ENERGY_LAKE_NAME         0x005D  // ??
#define VAR_EQ_ENERGY_LAKE_DECIMAL_NAME L"93"
#define VAR_EQ_TPM_MODE_NAME            0x005E  // ^
#define VAR_EQ_TPM_MODE_DECIMAL_NAME    L"94"
#define VAR_EQ_DISCRETE_SATA_NAME       0x005F  // ??
#define VAR_EQ_DISCRETE_SATA_DECIMAL_NAME L"95"
#define VAR_EQ_ROEM_SKU_NAME            0x0060  // ??
#define VAR_EQ_ROEM_SKU_DECIMAL_NAME    L"96"
#define VAR_EQ_AMTSOL_MODE_NAME         0x0061  // ??
#define VAR_EQ_AMTSOL_MODE_DECIMAL_NAME L"97"
#define VAR_EQ_NO_PEG_MODE_NAME         0x0062  // ??
#define VAR_EQ_NO_PEG_MODE_DECIMAL_NAME L"98"
#define VAR_EQ_SINGLE_PROCESSOR_MODE_NAME 0x0063  // ??
#define VAR_EQ_SINGLE_PROCESSOR_MODE_DECIMAL_NAME L"99"
#define VAR_EQ_FLOPPY_HIDE_NAME         0x0064  // ??
#define VAR_EQ_FLOPPY_HIDE_DECIMAL_NAME L"100"
#define VAR_EQ_SERIAL_HIDE_NAME         0x0065  // ??
#define VAR_EQ_SERIAL_HIDE_DECIMAL_NAME L"101"
#define VAR_EQ_GV3_CAPABLE_NAME         0x0066 // f
#define VAR_EQ_GV3_CAPABLE_DECIMAL_NAME L"102"
#define VAR_EQ_2_MEMORY_NAME            0x0067 // ??
#define VAR_EQ_2_MEMORY_DECIMAL_NAME    L"103"
#define VAR_EQ_2_SATA_NAME              0x0068 // ??
#define VAR_EQ_2_SATA_DECIMAL_NAME      L"104"
#define VAR_EQ_NEC_SKU_NAME            0x0069  // ??
#define VAR_EQ_NEC_SKU_DECIMAL_NAME    L"105"
#define VAR_EQ_AMT_MODE_NAME            0x006A  // ??
#define VAR_EQ_AMT_MODE_DECIMAL_NAME    L"106"
#define VAR_EQ_LCLX_SKU_NAME            0x006B  // ??
#define VAR_EQ_LCLX_SKU_DECIMAL_NAME    L"107"
#define VAR_EQ_VT_NAME                  0x006C
#define VAR_EQ_VT_DECIMAL_NAME          L"108"
#define VAR_EQ_LT_NAME                  0x006D
#define VAR_EQ_LT_DECIMAL_NAME          L"109"
#define VAR_EQ_ITK_BIOS_MOD_NAME         0x006E  // ??
#define VAR_EQ_ITK_BIOS_MOD_DECIMAL_NAME L"110"
#define VAR_EQ_HPET_NAME                0x006F
#define VAR_EQ_HPET_DECIMAL_NAME        L"111"
#define VAR_EQ_ADMIN_INSTALLED_NAME          0x0070  // ??
#define VAR_EQ_ADMIN_INSTALLED_DECIMAL_NAME  L"112"
#define VAR_EQ_USER_INSTALLED_NAME          0x0071  // ??
#define VAR_EQ_USER_INSTALLED_DECIMAL_NAME  L"113"
#define VAR_EQ_CPU_CMP_NAME             0x0072
#define VAR_EQ_CPU_CMP_DECIMAL_NAME     L"114"
#define VAR_EQ_LAN_MAC_ADDR_NAME          0x0073  // ??
#define VAR_EQ_LAN_MAC_ADDR_DECIMAL_NAME  L"115"
#define VAR_EQ_PARALLEL_HIDE_NAME         0x0074  // ??
#define VAR_EQ_PARALLEL_HIDE_DECIMAL_NAME L"116"
#define VAR_EQ_AFSC_SETUP_NAME          0x0075
#define VAR_EQ_AFSC_SETUP_DECIMAL_NAME  L"117"
#define VAR_EQ_MINICARD_MODE_NAME       0x0076  //
#define VAR_EQ_MINICARD_MODE_DECIMAL_NAME   L"118"
#define VAR_EQ_VIDEO_IGD_NAME          0x0077  //
#define VAR_EQ_VIDEO_IGD_DECIMAL_NAME  L"119"
#define VAR_EQ_ALWAYS_ENABLE_LAN_NAME            0x0078  //
#define VAR_EQ_ALWAYS_ENABLE_LAN_DECIMAL_NAME    L"120"
#define VAR_EQ_LEGACY_FREE_NAME          0x0079  //
#define VAR_EQ_LEGACY_FREE_DECIMAL_NAME   L"121"
#define VAR_EQ_CLEAR_CHASSIS_INSTRUSION_STATUS_NAME           0x007A
#define VAR_EQ_CLEAR_CHASSIS_INSTRUSION_STATUS_DECIMAL_NAME  L"122"
#define VAR_EQ_CPU_FSB_NAME            0x007B  //
#define VAR_EQ_CPU_FSB_DECIMAL_NAME    L"123"
#define VAR_EQ_SATA0_DEVICE_NAME            0x007C  //
#define VAR_EQ_SATA0_DVICE_DECIMAL_NAME    L"124"
#define VAR_EQ_SATA1_DEVICE_NAME            0x007D  //
#define VAR_EQ_SATA1_DVICE_DECIMAL_NAME    L"125"
#define VAR_EQ_SATA2_DEVICE_NAME            0x007E  //
#define VAR_EQ_SATA2_DVICE_DECIMAL_NAME    L"126"
#define VAR_EQ_SATA3_DEVICE_NAME            0x007F  //
#define VAR_EQ_SATA3_DVICE_DECIMAL_NAME    L"127"
#define VAR_EQ_SATA4_DEVICE_NAME            0x0080  //
#define VAR_EQ_SATA4_DVICE_DECIMAL_NAME    L"128"
#define VAR_EQ_SATA5_DEVICE_NAME            0x0081  //
#define VAR_EQ_SATA5_DVICE_DECIMAL_NAME    L"129"
#define VAR_EQ_TPM_STATUS_NAME              0x0082      // To indicate if TPM is enabled
#define VAR_EQ_TPM_STATUS_DECIMAL_NAME     L"130"
#define VAR_EQ_HECETA6E_PECI_CPU_NAME   0x0083
#define VAR_EQ_HECETA6E_PECI_CPU_DECIMAL_NAME L"131"
#define VAR_EQ_USB_2_NAME                   0x0084  //
#define VAR_EQ_USB_2_DECIMAL_NAME          L"132"
#define VAR_EQ_RVP_NAME                   0x0085  //
#define VAR_EQ_RVP_DECIMAL_NAME          L"133"
#define VAR_EQ_ECIR_NAME                    0x0086
#define VAR_EQ_ECIR_DECIMAL_NAME           L"134"
#define VAR_EQ_WAKONS5KB_NAME               0x0087
#define VAR_EQ_WAKONS5KB_DECIMAL_NAME      L"135"
#define VAR_EQ_HDAUDIOLINKBP_NAME           0x0088
#define VAR_EQ_HDAUDIOLINKBP_DECIMAL_NAME  L"136"
#define VAR_EQ_FINGERPRINT_NAME             0x0089
#define VAR_EQ_FINGERPRINT_DECIMAL_NAME    L"137"
#define VAR_EQ_BLUETOOTH_NAME               0x008A
#define VAR_EQ_BLUETOOTH_DECIMAL_NAME      L"138"
#define VAR_EQ_WLAN_NAME                    0x008B
#define VAR_EQ_WLAN_DECIMAL_NAME           L"139"
#define VAR_EQ_1_PATA_NAME                  0x008C
#define VAR_EQ_1_PATA_DECIMAL_NAME         L"140"
#define VAR_EQ_ACTIVE_PROCESSOR_CORE_NAME          0x008D
#define VAR_EQ_ACTIVE_PROCESSOR_CORE_DECIMAL_NAME  L"141"
#define VAR_EQ_TURBO_MODE_CAP_NAME          0x008E
#define VAR_EQ_TURBO_MODE_CAP_DECIMAL_NAME  L"142"
#define VAR_EQ_XE_MODE_CAP_NAME             0x008F
#define VAR_EQ_XE_MODE_CAP_DECIMAL_NAME     L"143"
#define VAR_EQ_NPI_QPI_VOLTAGE_NAME         0x0090
#define VAR_EQ_NPI_QPI_VOLTAGE_DECIMAL_NAME L"144"
#define VAR_EQ_PRE_PROD_NON_XE_NAME         0x0091
#define VAR_EQ_PRE_PROD_NON_XE_DECIMAL_NAME L"145"
#define VAR_EQ_2_C0_MEMORY_NAME            0x0092 // ??
#define VAR_EQ_2_C0_MEMORY_DECIMAL_NAME    L"146"
#define VAR_EQ_LVDS_NAME                    0x0093
#define VAR_EQ_LVDS_DECIMAL_NAME            L"147"
#define VAR_EQ_USB_OPTION_SHOW_NAME                    0x0094
#define VAR_EQ_USB_OPTION_SHOW_DECIMAL_NAME            L"148"
#define VAR_EQ_HDD_MASTER_INSTALLED_NAME      0x0095
#define VAR_EQ_HDD_MASTER_INSTALLED_DECIMAL_NAME      L"149"
#define VAR_EQ_HDD_USER_INSTALLED_NAME        0x0096
#define VAR_EQ_HDD_USER_INSTALLED_DECIMAL_NAME       L"150"
#define VAR_EQ_PS2_HIDE_NAME         0x0097  // ??
#define VAR_EQ_PS2_HIDE_DECIMAL_NAME L"151"
#define VAR_EQ_VIDEO_SLOT_NAME       0x0098
#define VAR_EQ_VIDEO_SLOT_DECIMAL_NAME       L"152"
#define VAR_EQ_HDMI_SLOT_NAME       0x0099
#define VAR_EQ_HDMI_SLOT_DECIMAL_NAME       L"153"
#define VAR_EQ_SERIAL2_HIDE_NAME         0x009a
#define VAR_EQ_SERIAL2_HIDE_DECIMAL_NAME L"154"


#define VAR_EQ_LVDS_WARNING_HIDE_NAME    0x009e
#define VAR_EQ_LVDS_WARNING_HIDE_DECIMAL_NAME L"158"


#define VAR_EQ_MSATA_HIDE_NAME    0x009f
#define VAR_EQ_MSATA_HIDE_DECIMAL_NAME L"159"


#define VAR_EQ_PCI_SLOT1_NAME    0x00a0
#define VAR_EQ_PCI_SLOT1_DECIMAL_NAME L"160"
#define VAR_EQ_PCI_SLOT2_NAME    0x00a1
#define VAR_EQ_PCI_SLOT2_DECIMAL_NAME L"161"

//
// Generic Form Ids
//
#define ROOT_FORM_ID                    1

//
// Advance Page. Do not have to be sequential but have to be unique
//
#define CONFIGURATION_ROOT_FORM_ID          2
#define BOOT_CONFIGURATION_ID               3
#define ONBOARDDEVICE_CONFIGURATION_ID      4
#define DRIVE_CONFIGURATION_ID              5
#define FLOPPY_CONFIGURATION_ID             6
#define EVENT_LOG_CONFIGURATION_ID          7
#define VIDEO_CONFIGURATION_ID              8
#define USB_CONFIGURATION_ID                9
#define HARDWARE_MONITOR_CONFIGURATION_ID   10
#define VIEW_EVENT_LOG_CONFIGURATION_ID     11
#define MEMORY_OVERRIDE_ID                  12
#define CHIPSET_CONFIGURATION_ID            13
#define BURN_IN_MODE_ID                     14
#define PCI_EXPRESS_ID                      15
#define MANAGEMENT_CONFIGURATION_ID         16
#define CPU_CONFIGURATION_ID                17
#define PCI_CONFIGURATION_ID                18
#define SECURITY_CONFIGURATION_ID           19
#define ZIP_CONFIGURATION_ID                20
#define AFSC_FAN_CONTROL_ID                 21
#define VFR_FORMID_CSI                      22
#define VFR_FORMID_MEMORY                   23
#define VFR_FORMID_IOH                      24
#define VFR_FORMID_CPU_CSI                  25
#define VFR_FORMID_IOH_CONFIG               26
#define VFR_FORMID_VTD                      27
#define VFR_FORMID_PCIE_P0                  28
#define VFR_FORMID_PCIE_P1                  29
#define VFR_FORMID_PCIE_P2                  30
#define VFR_FORMID_PCIE_P3                  31
#define VFR_FORMID_PCIE_P4                  32
#define VFR_FORMID_PCIE_P5                  33
#define VFR_FORMID_PCIE_P6                  34
#define VFR_FORMID_PCIE_P7                  35
#define VFR_FORMID_PCIE_P8                  36
#define VFR_FORMID_PCIE_P9                  37
#define VFR_FORMID_PCIE_P10                 38
#define VFR_FID_SKT0                        39
#define VFR_FID_IOH0                        40
#define VFR_FID_IOH_DEV_HIDE                41
#define PROCESSOR_OVERRIDES_FORM_ID         42
#define BUS_OVERRIDES_FORM_ID               43
#define REF_OVERRIDES_FORM_ID               44
#define MEMORY_INFORMATION_ID               45
#define LVDS_WARNING_ID                     46
#define LVDS_CONFIGURATION_ID               47
#define PCI_SLOT_CONFIGURATION_ID           48
#define HECETA_CONFIGURATION_ID             49
#define LVDS_EXPERT_CONFIGURATION_ID        50
#define PCI_SLOT_7_ID                       51
#define PCI_SLOT_6_ID                       52
#define PCI_SLOT_5_ID                       53
#define PCI_SLOT_4_ID                       54
#define PCI_SLOT_3_ID                       55
#define PCI_SLOT_2_ID                       56
#define PCI_SLOT_1_ID                       57
#define BOOT_DISPLAY_ID                     58
#define CPU_PWR_CONFIGURATION_ID            59

#define FSC_CONFIGURATION_ID                60
#define FSC_CPU_TEMPERATURE_FORM_ID         61
#define FSC_VTT_VOLTAGE_FORM_ID             62
#define FSC_FEATURES_CONTROL_ID             63
#define FSC_FAN_CONFIGURATION_ID            64
#define FSC_PROCESSOR_FAN_CONFIGURATION_ID  65
#define FSC_FRONT_FAN_CONFIGURATION_ID      66
#define FSC_REAR_FAN_CONFIGURATION_ID       67
#define FSC_AUX_FAN_CONFIGURATION_ID        68
#define FSC_12_VOLTAGE_FORM_ID              69
#define FSC_5_VOLTAGE_FORM_ID               70
#define FSC_3P3_VOLTAGE_FORM_ID             71
#define FSC_2P5_VOLTAGE_FORM_ID             72
#define FSC_VCC_VOLTAGE_FORM_ID             73
#define FSC_PCH_TEMPERATURE_FORM_ID         74
#define FSC_MEM_TEMPERATURE_FORM_ID         75
#define FSC_VR_TEMPERATURE_FORM_ID          76
#define FSC_3P3STANDBY_VOLTAGE_FORM_ID      77
#define FSC_5BACKUP_VOLTAGE_FORM_ID         78
#define ROOT_MAIN_FORM_ID                   79
#define ROOT_BOOT_FORM_ID                   80
#define ROOT_MAINTENANCE_ID                 81
#define ROOT_POWER_FORM_ID                  82
#define ROOT_SECURITY_FORM_ID               83
#define ROOT_PERFORMANCE_FORM_ID            84
#define ROOT_SYSTEM_SETUP_FORM_ID           85

#define ADDITIONAL_SYSTEM_INFO_FORM_ID      86

#define THERMAL_CONFIG_FORM_ID              87

#define PCI_SLOT_CONFIG_LABEL_ID_1            0x300A
#define PCI_SLOT_CONFIG_LABEL_ID_2            0x300B
#define PCI_SLOT_CONFIG_LABEL_ID_3            0x300C
#define PCI_SLOT_CONFIG_LABEL_ID_4            0x300D
#define PCI_SLOT_CONFIG_LABEL_ID_5            0x300E
#define PCI_SLOT_CONFIG_LABEL_ID_6            0x300F
#define PCI_SLOT_CONFIG_LABEL_ID_7            0x3010
#define PCI_SLOT_CONFIG_LABEL_ID_8            0x3011

//
// Advance Hardware Monitor Callback Keys. Do not have to be sequential but have to be unique
//
#define CONFIGURATION_HARDWARE_CALLBACK_KEY            0x2000
#define ADVANCE_VIDEO_CALLBACK_KEY                     0x2001
#define CONFIGURATION_FSC_CALLBACK_KEY                 0x2002
#define CONFIGURATION_RESTORE_FAN_CONTROL_CALLBACK_KEY 0x2003
#define CONFIGURATION_LVDS_CALLBACK_KEY                0x2004
#define CONFIGURATION_PREDEFINED_EDID_CALLBACK_KEY     0x2005
#define ADVANCE_LVDS_CALLBACK_KEY           0x2010

//
// Main Callback Keys. Do not have to be sequential but have to be unique
//
#define MAIN_LANGUAGE_CALLBACK_KEY          0x3000

//
// Power Hardware Monitor Callback Keys. Do not have to be sequential but have to be unique
//
#define POWER_HARDWARE_CALLBACK_KEY         0x4000

//
// Performance Callback Keys. Do not have to be sequential but have to be unique
//
#define PROCESSOR_OVERRIDES_CALLBACK_KEY 0x5000
#define PERFORMANCE_CALLBACK_KEY         0x5001
#define BUS_OVERRIDES_CALLBACK_KEY       0x5002
#define MEMORY_CFG_CALLBACK_KEY          0x5003
#define PERFORMANCE_STATUS_CALLBACK_KEY  0x5004
#define MEMORY_RATIO_CALLBACK_KEY        0x5005
#define MEMORY_MODE_CALLBACK_KEY         0x5006

//
// Security Callback Keys. Do not have to be sequential but have to be unique
//
#define SECURITY_SUPERVISOR_CALLBACK_KEY    0x1000
#define SECURITY_USER_CALLBACK_KEY          0x1001
#define SECURITY_CLEAR_ALL_CALLBACK_KEY     0x1002
#define SECURITY_CLEAR_USER_CALLBACK_KEY    0x1004
#define SECURITY_RESET_AMT_CALLBACK_KEY     0x1008
#define SECURITY_CHANGE_VT_CALLBACK_KEY     0x1010
#define SECURITY_MASTER_HDD_CALLBACK_KEY    0x1020
#define SECURITY_USER_HDD_CALLBACK_KEY      0x1040

//
// Boot Callback Keys. Do not have to be sequential but have to be unique
//
#define BOOT_HYPERBOOT_CALLBACK_KEY                 0x6003
#define BOOT_HYPERBOOT_CALLBACK_KEY_DISABLE         0x6004
#define BOOT_HYPERBOOT_CALLBACK_KEY_USB             0x6005
#define BOOT_HYPERBOOT_CALLBACK_KEY_DISABLE_USB_OPT 0x6006

//
// IDCC/Setup FSB Frequency Override Range
//
#define EFI_IDCC_FSB_MIN   133
#define EFI_IDCC_FSB_MAX   240
#define EFI_IDCC_FSB_STEP  1

//
// Reference voltage
//
#define EFI_REF_DAC_MIN     0
#define EFI_REF_DAC_MAX     255
#define EFI_GTLREF_DEF      170
#define EFI_DDRREF_DEF      128
#define EFI_DIMMREF_DEF     128

//
// Setup FSB Frequency Override Range
//
#define EFI_FSB_MIN       133
#define EFI_FSB_MAX       240
#define EFI_FSB_STEP      1
#define EFI_FSB_AUTOMATIC 0
#define EFI_FSB_MANUAL    1
#define FSB_FREQ_ENTRY_COUNT ((EFI_FSB_MAX - EFI_FSB_MIN)/EFI_FSB_STEP) + 1
#define FSB_FREQ_ENTRY_TYPE  UINT16_TYPE

//
// Setup processor multiplier range
//
#define EFI_PROC_MULT_MIN   5
#define EFI_PROC_MULT_MAX   40
#define EFI_PROC_MULT_STEP  1
#define EFI_PROC_AUTOMATIC  0
#define EFI_PROC_MANUAL     1
#define PROC_MULT_ENTRY_COUNT ((EFI_PROC_MULT_MAX - EFI_PROC_MULT_MIN)/EFI_PROC_MULT_STEP) + 1
#define PROC_MULT_ENTRY_TYPE  UINT8_TYPE

//
// PCI Express Definitions
//
#define EFI_PCIE_FREQ_DEF       0x0

#define PCIE_FREQ_ENTRY_TYPE  UINT8_TYPE
#define PCIE_FREQ_ENTRY_7       0x7
#define PCIE_FREQ_ENTRY_6       0x6
#define PCIE_FREQ_ENTRY_5       0x5
#define PCIE_FREQ_ENTRY_4       0x4
#define PCIE_FREQ_ENTRY_3       0x3
#define PCIE_FREQ_ENTRY_2       0x2
#define PCIE_FREQ_ENTRY_1       0x1
#define PCIE_FREQ_ENTRY_0       0x0

#define PCIE_FREQ_TRANSLATION_TABLE_ENTRIES 8
#define PCIE_FREQ_TRANSLATION_TABLE     { PCIE_FREQ_ENTRY_0, \
                                          PCIE_FREQ_ENTRY_1, \
                                          PCIE_FREQ_ENTRY_2, \
                                          PCIE_FREQ_ENTRY_3, \
                                          PCIE_FREQ_ENTRY_4, \
                                          PCIE_FREQ_ENTRY_5, \
                                          PCIE_FREQ_ENTRY_6, \
                                          PCIE_FREQ_ENTRY_7 }


#define PCIE_FREQ_PRECISION     2
#define PCIE_FREQ_VALUE_7       10924
#define PCIE_FREQ_VALUE_6       10792
#define PCIE_FREQ_VALUE_5       10660
#define PCIE_FREQ_VALUE_4       10528
#define PCIE_FREQ_VALUE_3       10396
#define PCIE_FREQ_VALUE_2       10264
#define PCIE_FREQ_VALUE_1       10132
#define PCIE_FREQ_VALUE_0       10000

#define PCIE_FREQ_VALUES      { PCIE_FREQ_VALUE_0, \
                                PCIE_FREQ_VALUE_1, \
                                PCIE_FREQ_VALUE_2, \
                                PCIE_FREQ_VALUE_3, \
                                PCIE_FREQ_VALUE_4, \
                                PCIE_FREQ_VALUE_5, \
                                PCIE_FREQ_VALUE_6, \
                                PCIE_FREQ_VALUE_7 }

//
// Memory Frequency Definitions
//
#define MEMORY_REF_FREQ_ENTRY_DEF     0x08

#define MEMORY_REF_FREQ_ENTRY_TYPE    UINT8_TYPE
#define MEMORY_REF_FREQ_ENTRY_3       0x04
#define MEMORY_REF_FREQ_ENTRY_2       0x00
#define MEMORY_REF_FREQ_ENTRY_1       0x02
#define MEMORY_REF_FREQ_ENTRY_0       0x01

#define MEMORY_REF_FREQ_TRANSLATION_TABLE_ENTRIES 4
#define MEMORY_REF_FREQ_TRANSLATION_TABLE     { MEMORY_REF_FREQ_ENTRY_0, \
                                                MEMORY_REF_FREQ_ENTRY_1, \
                                                MEMORY_REF_FREQ_ENTRY_2, \
                                                MEMORY_REF_FREQ_ENTRY_3 }

#define MEMORY_REF_FREQ_PRECISION     0
#define MEMORY_REF_FREQ_VALUE_3       333
#define MEMORY_REF_FREQ_VALUE_2       267
#define MEMORY_REF_FREQ_VALUE_1       200
#define MEMORY_REF_FREQ_VALUE_0       133

#define MEMORY_REF_FREQ_VALUES      { MEMORY_REF_FREQ_VALUE_0, \
                                      MEMORY_REF_FREQ_VALUE_1, \
                                      MEMORY_REF_FREQ_VALUE_2, \
                                      MEMORY_REF_FREQ_VALUE_3 }


//
// Memory Reference Frequency Definitions
//

#define MEMORY_FREQ_ENTRY_TYPE    UINT8_TYPE
#define MEMORY_FREQ_ENTRY_3       0x4
#define MEMORY_FREQ_ENTRY_2       0x3
#define MEMORY_FREQ_ENTRY_1       0x2
#define MEMORY_FREQ_ENTRY_0       0x1

#define MEMORY_FREQ_TRANSLATION_TABLE_ENTRIES 4
#define MEMORY_FREQ_TRANSLATION_TABLE     { MEMORY_FREQ_ENTRY_0, \
                                            MEMORY_FREQ_ENTRY_1, \
                                            MEMORY_FREQ_ENTRY_2, \
                                            MEMORY_FREQ_ENTRY_3 }


#define MEMORY_FREQ_MULT_PRECISION             2
#define MEMORY_FREQ_MULT_333MHZ_VALUE_3        240
#define MEMORY_FREQ_MULT_333MHZ_VALUE_2        200
#define MEMORY_FREQ_MULT_333MHZ_VALUE_1        160
#define MEMORY_FREQ_MULT_333MHZ_VALUE_0        120

#define MEMORY_FREQ_MULT_266MHZ_VALUE_3        300
#define MEMORY_FREQ_MULT_266MHZ_VALUE_2        250
#define MEMORY_FREQ_MULT_266MHZ_VALUE_1        200
#define MEMORY_FREQ_MULT_266MHZ_VALUE_0        150

#define MEMORY_FREQ_MULT_200MHZ_VALUE_3        400
#define MEMORY_FREQ_MULT_200MHZ_VALUE_2        333
#define MEMORY_FREQ_MULT_200MHZ_VALUE_1        267
#define MEMORY_FREQ_MULT_200MHZ_VALUE_0        200

#define MEMORY_FREQ_MULT_133MHZ_VALUE_3        600
#define MEMORY_FREQ_MULT_133MHZ_VALUE_2        500
#define MEMORY_FREQ_MULT_133MHZ_VALUE_1        400
#define MEMORY_FREQ_MULT_133MHZ_VALUE_0        300

#define MEMORY_FREQ_MULT_333MHZ_VALUES      { MEMORY_FREQ_MULT_333MHZ_VALUE_0, \
                                              MEMORY_FREQ_MULT_333MHZ_VALUE_1, \
                                              MEMORY_FREQ_MULT_333MHZ_VALUE_2, \
                                              MEMORY_FREQ_MULT_333MHZ_VALUE_3 }

#define MEMORY_FREQ_MULT_266MHZ_VALUES      { MEMORY_FREQ_MULT_266MHZ_VALUE_0, \
                                              MEMORY_FREQ_MULT_266MHZ_VALUE_1, \
                                              MEMORY_FREQ_MULT_266MHZ_VALUE_2, \
                                              MEMORY_FREQ_MULT_266MHZ_VALUE_3 }

#define MEMORY_FREQ_MULT_200MHZ_VALUES      { MEMORY_FREQ_MULT_200MHZ_VALUE_0, \
                                              MEMORY_FREQ_MULT_200MHZ_VALUE_1, \
                                              MEMORY_FREQ_MULT_200MHZ_VALUE_2, \
                                              MEMORY_FREQ_MULT_200MHZ_VALUE_3 }

#define MEMORY_FREQ_MULT_133MHZ_VALUES      { MEMORY_FREQ_MULT_133MHZ_VALUE_0, \
                                              MEMORY_FREQ_MULT_133MHZ_VALUE_1, \
                                              MEMORY_FREQ_MULT_133MHZ_VALUE_2, \
                                              MEMORY_FREQ_MULT_133MHZ_VALUE_3 }

//
// CAS Memory Timing Definitions
//

#define MEMORY_TCL_ENTRY_TYPE    UINT8_TYPE
#define MEMORY_TCL_ENTRY_3       0x2
#define MEMORY_TCL_ENTRY_2       0x1
#define MEMORY_TCL_ENTRY_1       0x0
#define MEMORY_TCL_ENTRY_0       0x3

#define MEMORY_TCL_TRANSLATION_TABLE_ENTRIES 4
#define MEMORY_TCL_TRANSLATION_TABLE     { MEMORY_TCL_ENTRY_0, \
                                           MEMORY_TCL_ENTRY_1, \
                                           MEMORY_TCL_ENTRY_2, \
                                           MEMORY_TCL_ENTRY_3 }


#define MEMORY_TCL_PRECISION     0
#define MEMORY_TCL_VALUE_3       3
#define MEMORY_TCL_VALUE_2       4
#define MEMORY_TCL_VALUE_1       5
#define MEMORY_TCL_VALUE_0       6

#define MEMORY_TCL_VALUES      { MEMORY_TCL_VALUE_0, \
                                 MEMORY_TCL_VALUE_1, \
                                 MEMORY_TCL_VALUE_2, \
                                 MEMORY_TCL_VALUE_3 }


//
// TRCD Memory Timing Definitions
//

#define MEMORY_TRCD_ENTRY_TYPE    UINT8_TYPE
#define MEMORY_TRCD_ENTRY_3       0x0
#define MEMORY_TRCD_ENTRY_2       0x1
#define MEMORY_TRCD_ENTRY_1       0x2
#define MEMORY_TRCD_ENTRY_0       0x3

#define MEMORY_TRCD_TRANSLATION_TABLE_ENTRIES 4
#define MEMORY_TRCD_TRANSLATION_TABLE     { MEMORY_TRCD_ENTRY_0, \
                                            MEMORY_TRCD_ENTRY_1, \
                                            MEMORY_TRCD_ENTRY_2, \
                                            MEMORY_TRCD_ENTRY_3 }


#define MEMORY_TRCD_PRECISION     0
#define MEMORY_TRCD_VALUE_3       2
#define MEMORY_TRCD_VALUE_2       3
#define MEMORY_TRCD_VALUE_1       4
#define MEMORY_TRCD_VALUE_0       5

#define MEMORY_TRCD_VALUES      { MEMORY_TRCD_VALUE_0, \
                                  MEMORY_TRCD_VALUE_1, \
                                  MEMORY_TRCD_VALUE_2, \
                                  MEMORY_TRCD_VALUE_3 }


//
// TRP Memory Timing Definitions
//

#define MEMORY_TRP_ENTRY_TYPE    UINT8_TYPE
#define MEMORY_TRP_ENTRY_3       0x0
#define MEMORY_TRP_ENTRY_2       0x1
#define MEMORY_TRP_ENTRY_1       0x2
#define MEMORY_TRP_ENTRY_0       0x3

#define MEMORY_TRP_TRANSLATION_TABLE_ENTRIES 4
#define MEMORY_TRP_TRANSLATION_TABLE     { MEMORY_TRP_ENTRY_0, \
                                           MEMORY_TRP_ENTRY_1, \
                                           MEMORY_TRP_ENTRY_2, \
                                           MEMORY_TRP_ENTRY_3 }


#define MEMORY_TRP_PRECISION     0
#define MEMORY_TRP_VALUE_3       2
#define MEMORY_TRP_VALUE_2       3
#define MEMORY_TRP_VALUE_1       4
#define MEMORY_TRP_VALUE_0       5

#define MEMORY_TRP_VALUES      { MEMORY_TRP_VALUE_0, \
                                 MEMORY_TRP_VALUE_1, \
                                 MEMORY_TRP_VALUE_2, \
                                 MEMORY_TRP_VALUE_3 }


//
// TRAS Memory Timing Definitions
//
#define MEMORY_TRAS_MIN     4
#define MEMORY_TRAS_MAX     18
#define MEMORY_TRAS_STEP    1
#define MEMORY_TRAS_DEFAULT 13
#define MEMORY_TRAS_COUNT ((MEMORY_TRAS_MAX - MEMORY_TRAS_MIN)/MEMORY_TRAS_STEP) + 1
#define MEMORY_TRAS_TYPE  UINT8_TYPE

//
// Uncore Multiplier Definitions
//
#define UCLK_RATIO_MIN     12
#define UCLK_RATIO_MAX     30
#define UCLK_RATIO_DEFAULT 20

#endif // #ifndef _CONFIGURATION_H
