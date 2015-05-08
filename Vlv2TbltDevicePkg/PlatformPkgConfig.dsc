#/** @file
# platform configuration file.
#
# Copyright (c) 2012  - 2015, Intel Corporation. All rights reserved.<BR>
#                                                                                  
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.  
# The full text of the license may be found at                                     
# http://opensource.org/licenses/bsd-license.php.                                  
#                                                                                  
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
#                                                                                  
#
#**/

#
# TRUE is ENABLE. FASLE is DISABLE.
#

#
# FSP selection
#
DEFINE MINNOW2_FSP_BUILD = FALSE


DEFINE SCSI_ENABLE = TRUE


#
# To enable extra configuration for clk gen
#
DEFINE CLKGEN_CONFIG_EXTRA_ENABLE=TRUE

#
# Feature selection
#

#
# Select system timer which is used to produce Timer Arch Protocol:
# TRUE  - HPET timer is used.
# FALSE - 8254 timer is used.
#
DEFINE USE_HPET_TIMER = FALSE


#
# Feature selection
#

DEFINE TPM_ENABLED = FALSE

DEFINE ACPI50_ENABLE = TRUE
DEFINE PERFORMANCE_ENABLE = FALSE


DEFINE LFMA_ENABLE = FALSE              # Load module at fixed address feature
DEFINE DXE_COMPRESS_ENABLE = TRUE
DEFINE DXE_CRC32_SECTION_ENABLE = TRUE
DEFINE SSE2_ENABLE = FALSE

DEFINE SECURE_BOOT_ENABLE = TRUE
DEFINE USER_IDENTIFICATION_ENABLE = FALSE
DEFINE VARIABLE_INFO_ENABLE = FALSE
DEFINE S3_ENABLE = TRUE
DEFINE CAPSULE_ENABLE = FALSE
DEFINE CAPSULE_RESET_ENABLE = TRUE

DEFINE GOP_DRIVER_ENABLE = TRUE
DEFINE DATAHUB_ENABLE = TRUE
DEFINE DATAHUB_STATUS_CODE_ENABLE = TRUE
DEFINE USB_ENABLE = TRUE

DEFINE ISA_SERIAL_STATUS_CODE_ENABLE = TRUE
DEFINE USB_SERIAL_STATUS_CODE_ENABLE = FALSE
DEFINE RAM_SERIAL_STATUS_CODE_ENABLE = FALSE

DEFINE ENBDT_S3_SUPPORT = TRUE

DEFINE LZMA_ENABLE = TRUE
DEFINE S4_ENABLE = TRUE
DEFINE NETWORK_ENABLE = TRUE
DEFINE NETWORK_IP6_ENABLE = TRUE
DEFINE NETWORK_ISCSI_ENABLE = FALSE
DEFINE NETWORK_VLAN_ENABLE = FALSE

DEFINE SATA_ENABLE       = TRUE
DEFINE PCIESC_ENABLE     = TRUE

#
# Enable source level debug default
#
DEFINE SOURCE_DEBUG_ENABLE     = FALSE




