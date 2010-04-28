/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ExtendedSalGuid.c

Abstract:

  The Extended SAL Lock Services protocol as defined in SAL CIS.

  This protocol is installed by the entity that supplies low level
  lock primitives that work in dual mode. There are 3 functions
  as defined below.
                   
--*/



#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION(ExtendedSalGuid)

EFI_GUID gEfiExtendedSalBaseIoServicesProtocolGuid = EFI_EXTENDED_SAL_BASE_IO_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalStallServicesProtocolGuid = EFI_EXTENDED_SAL_STALL_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalLockServicesProtocolGuid = EFI_EXTENDED_SAL_LOCK_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalVirtualServicesProtocolGuid = EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalRtcServicesProtocolGuid = EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalVariableServicesProtocolGuid = EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalMtcServicesProtocolGuid = EFI_EXTENDED_SAL_MTC_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalResetServicesProtocolGuid = EFI_EXTENDED_SAL_RESET_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalStatusCodeServicesProtocolGuid = EFI_EXTENDED_SAL_STATUS_CODE_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalFvBlockServicesProtocolGuid = EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalMpServicesProtocolGuid = EFI_EXTENDED_SAL_MP_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalPalServicesProtocolGuid = EFI_EXTENDED_SAL_PAL_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalBaseServicesProtocolGuid = EFI_EXTENDED_SAL_BASE_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalMcaServicesProtocolGuid = EFI_EXTENDED_SAL_MCA_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalPciServicesProtocolGuid = EFI_EXTENDED_SAL_PCI_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalCacheServicesProtocolGuid = EFI_EXTENDED_SAL_CACHE_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalMcaLogServicesProtocolGuid = EFI_EXTENDED_SAL_MCA_LOG_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalElogServicesProtocolGuid = EFI_EXTENDED_SAL_ELOG_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalSensorServicesProtocolGuid = EFI_EXTENDED_SAL_SENSOR_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalSmComLayerServicesProtocolGuid = EFI_EXTENDED_SAL_SM_COM_LAYER_SERVICES_PROTOCOL_GUID;
EFI_GUID gEfiExtendedSalSstGuid = EFI_EXTENDED_SAL_SST_GUID;


EFI_GUID_STRING(&gEfiExtendedSalBaseIoServicesProtocolGuid, "SAL IO", "Extended SAL Base IO Protocol");
EFI_GUID_STRING(&gEfiExtendedSalStallServicesProtocolGuid, "SAL STALL", "Extended SAL Stall Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalLockServicesProtocolGuid, "SAL LOCK", "Extended SAL Lock Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalVirtualServicesProtocolGuid, "SAL VIRT", "Extended SAL Virtual Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalRtcServicesProtocolGuid, "SAL RTC", "Extended SAL Lock RTC Protocol");
EFI_GUID_STRING(&gEfiExtendedSalVariableServicesProtocolGuid, "SAL VARIABLE", "Extended SAL Variable Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalMtcServicesProtocolGuid, "SAL MTC", "Extended SAL MTC Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalResetServicesProtocolGuid, "SAL RESET", "Extended SAL Reset Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalStatusCodeServicesProtocolGuid, "SAL STATUS CODE", "Extended SAL Status Call Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalFvBlockServicesProtocolGuid, "SAL FVB", "Extended SAL FVB Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalMpServicesProtocolGuid, "SAL MP", "Extended SAL MP Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalPalServicesProtocolGuid, "SAL PAL", "Extended SAL PAL Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalBaseServicesProtocolGuid, "SAL BASE", "Extended SAL Base Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalBaseServicesProtocolGuid, "SAL MCA", "Extended SAL MCA Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalPciServicesProtocolGuid, "SAL PCI", "Extended SAL PCI Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalCacheServicesProtocolGuid, "SAL CACHE", "Extended SAL Cache Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalMcaLogServicesProtocolGuid, "MCA LOG", "Extended SAL MCA LOG Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalElogServicesProtocolGuid, "ELOG", "Extended SAL ELOG Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalSensorServicesProtocolGuid, "SENSOR", "Extended SAL SENSOR Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalSmComLayerServicesProtocolGuid, "SAL SM COM", "Extended SAL SM COM Services Protocol");
EFI_GUID_STRING(&gEfiExtendedSalSstGuid, "SST SET UP", "SAL System Table Header Set up");
