/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGlueDefinitionChangesPeim.h
  
Abstract: 

  Data structure definition changes from EDK to EDKII

--*/

#ifndef __EDKII_GLUE_DEFINITION_CHANGES_PEIM_H__
#define __EDKII_GLUE_DEFINITION_CHANGES_PEIM_H__

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#include "TianoHii.h"
#else
#include "EfiInternalFormRepresentation.h"
#endif

#include "EdkIIGlueDefinitionChangesBase.h"

#include "EfiPciCfg.h"

//
// typedef Edk types - EdkII types
//
typedef EFI_MEMORY_ARRAY_START_ADDRESS               EFI_MEMORY_ARRAY_START_ADDRESS_DATA;
typedef EFI_MEMORY_DEVICE_START_ADDRESS              EFI_MEMORY_DEVICE_START_ADDRESS_DATA;
typedef EFI_MISC_LAST_PCI_BUS                        EFI_MISC_LAST_PCI_BUS_DATA;
typedef EFI_MISC_BIOS_VENDOR                         EFI_MISC_BIOS_VENDOR_DATA;
typedef EFI_MISC_SYSTEM_MANUFACTURER                 EFI_MISC_SYSTEM_MANUFACTURER_DATA;
typedef EFI_MISC_BASE_BOARD_MANUFACTURER             EFI_MISC_BASE_BOARD_MANUFACTURER_DATA;
typedef EFI_MISC_CHASSIS_MANUFACTURER                EFI_MISC_CHASSIS_MANUFACTURER_DATA;
typedef EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA;
typedef EFI_MISC_SYSTEM_SLOT_DESIGNATION             EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA;
typedef EFI_MISC_ONBOARD_DEVICE                      EFI_MISC_ONBOARD_DEVICE_DATA;
typedef EFI_MISC_ONBOARD_DEVICE_TYPE_DATA            EFI_MISC_PORTING_DEVICE_TYPE_DATA;
typedef EFI_MISC_OEM_STRING                          EFI_MISC_OEM_STRING_DATA;
typedef EFI_MISC_SYSTEM_OPTION_STRING                EFI_MISC_SYSTEM_OPTION_STRING_DATA;
typedef EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES     EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA;
typedef EFI_MISC_SYSTEM_LANGUAGE_STRING              EFI_MISC_SYSTEM_LANGUAGE_STRING_DATA;
typedef EFI_MISC_BIS_ENTRY_POINT                     EFI_MISC_BIS_ENTRY_POINT_DATA;
typedef EFI_MISC_BOOT_INFORMATION_STATUS             EFI_MISC_BOOT_INFORMATION_STATUS_DATA;
typedef EFI_MISC_SYSTEM_POWER_SUPPLY                 EFI_MISC_SYSTEM_POWER_SUPPLY_DATA ;
typedef EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION         EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION_DATA;            

// -------------------
// EdkII Names - Edk Names
// -------------------
#define gEfiPeiCpuIoPpiInServiceTableGuid             gPeiCpuIoPpiInServiceTableGuid   
#define gEfiEndOfPeiSignalPpiGuid                     gEndOfPeiSignalPpiGuid           
#define gEfiPeiFvFileLoaderPpiGuid                    gPeiFvFileLoaderPpiGuid          
#define gEfiPeiMasterBootModePpiGuid                  gPeiMasterBootModePpiGuid        
#define gEfiPeiMemoryDiscoveredPpiGuid                gPeiMemoryDiscoveredPpiGuid      
#define gEfiPciCfgPpiInServiceTableGuid               gPeiPciCfgPpiInServiceTableGuid     
#define gEfiPeiReadOnlyVariablePpiGuid                gPeiReadOnlyVariablePpiGuid      
#define gEfiPeiRecoveryModulePpiGuid                  gPeiRecoveryModulePpiGuid        
#define gEfiPeiResetPpiGuid                           gPeiResetPpiGuid                 
#define gEfiPeiS3ResumePpiGuid                        gPeiS3ResumePpiGuid              
#define gEfiPeiSectionExtractionPpiGuid               gPeiSectionExtractionPpiGuid     
#define gEfiPeiSecurityPpiGuid                        gPeiSecurityPpiGuid              
#define gEfiPeiStatusCodePpiGuid                      gPeiStatusCodePpiGuid            
#define gEfiPeiBootScriptExecuterPpiGuid              gPeiBootScriptExecuterPpiGuid    
#define gEfiPeiSmbusPpiGuid                           gPeiSmbusPpiGuid                 
#define gEfiPeiBlockIoPpiGuid                         gPeiBlockIoPpiGuid               
#define gEfiPeiDeviceRecoveryModulePpiGuid            gPeiDeviceRecoveryModulePpiGuid
#define gEfiPeiStallPpiGuid                           gPeiStallPpiGuid                 
#define gEfiPeiPciCfgPpiInServiceTableGuid            gPeiPciCfgPpiInServiceTableGuid
#define gEfiPeiAtaControllerPpiGuid                   gPeiAtaControllerPpiGuid
#define EFI_PEI_CPU_IO_PPI_INSTALLED_GUID             PEI_CPU_IO_PPI_GUID
#define EFI_PEI_RESET_PPI_GUID                        PEI_RESET_PPI_GUID           
#define EFI_PEI_PCI_CFG_PPI_INSTALLED_GUID            PEI_PCI_CFG_PPI_GUID
#define EFI_PEI_REPORT_PROGRESS_CODE_PPI_GUID         PEI_STATUS_CODE_PPI_GUID
#define EFI_PEI_BOOT_IN_RECOVERY_MODE_PEIM_PPI        PEI_BOOT_IN_RECOVERY_MODE_PEIM_PPI
#define EFI_PEI_END_OF_PEI_PHASE_PPI_GUID             PEI_END_OF_PEI_PHASE_PPI_GUID
#define EFI_PEI_MASTER_BOOT_MODE_PEIM_PPI             PEI_MASTER_BOOT_MODE_PEIM_PPI
#define EFI_PEI_PERMANENT_MEMORY_INSTALLED_PPI_GUID   PEI_PERMANENT_MEMORY_INSTALLED_PPI_GUID
#define EFI_PEI_READ_ONLY_VARIABLE_ACCESS_PPI_GUID    PEI_READ_ONLY_VARIABLE_ACCESS_PPI_GUID
#define EFI_PEI_RECOVERY_MODULE_PPI_GUID              PEI_RECOVERY_MODULE_INTERFACE_PPI
#define EFI_PEI_S3_RESUME_PPI_GUID                    PEI_S3_RESUME_PPI_GUID
#define EFI_PEI_SECURITY_PPI_GUID                     PEI_SECURITY_PPI_GUID
#define EFI_PEI_STALL_PPI_GUID                        PEI_STALL_PPI_GUID
#define EFI_PEI_SMBUS_PPI_GUID                        PEI_SMBUS_PPI_GUID
#define EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI_GUID         PEI_BOOT_SCRIPT_EXECUTER_PPI_GUID
#define EFI_PEI_FIND_FV_PPI_GUID                      EFI_FIND_FV_PPI_GUID
#define EFI_PEI_VIRTUAL_BLOCK_IO_PPI                  PEI_BLOCK_IO_PPI_GUID
#define EFI_PEI_DEVICE_RECOVERY_MODULE_PPI_GUID       PEI_DEVICE_RECOVERY_MODULE_INTERFACE_PPI


//
// typedef Edk types - EdkII types
//
typedef PEI_RECOVERY_MODULE_INTERFACE                EFI_PEI_RECOVERY_MODULE_PPI;
typedef PEI_STALL_PPI                                EFI_PEI_STALL_PPI;
typedef PEI_SMBUS_PPI                                EFI_PEI_SMBUS_PPI;
typedef PEI_READ_ONLY_VARIABLE_PPI                   EFI_PEI_READ_ONLY_VARIABLE_PPI;
typedef PEI_PCI_CFG_PPI                              EFI_PEI_PCI_CFG_PPI;
typedef PEI_STATUS_CODE_PPI                          EFI_PEI_PROGRESS_CODE_PPI;
typedef PEI_CPU_IO_PPI_WIDTH                         EFI_PEI_CPU_IO_PPI_WIDTH;
typedef PEI_CPU_IO_PPI_IO_MEM                        EFI_PEI_CPU_IO_PPI_IO_MEM;
typedef PEI_CPU_IO_PPI_ACCESS                        EFI_PEI_CPU_IO_PPI_ACCESS;
typedef PEI_CPU_IO_PPI_IO_READ8                      EFI_PEI_CPU_IO_PPI_IO_READ8;
typedef PEI_CPU_IO_PPI_IO_READ16                     EFI_PEI_CPU_IO_PPI_IO_READ16;
typedef PEI_CPU_IO_PPI_IO_READ32                     EFI_PEI_CPU_IO_PPI_IO_READ32;
typedef PEI_CPU_IO_PPI_IO_READ64                     EFI_PEI_CPU_IO_PPI_IO_READ64;
typedef PEI_CPU_IO_PPI_IO_WRITE8                     EFI_PEI_CPU_IO_PPI_IO_WRITE8;
typedef PEI_CPU_IO_PPI_IO_WRITE16                    EFI_PEI_CPU_IO_PPI_IO_WRITE16;
typedef PEI_CPU_IO_PPI_IO_WRITE32                    EFI_PEI_CPU_IO_PPI_IO_WRITE32;
typedef PEI_CPU_IO_PPI_IO_WRITE64                    EFI_PEI_CPU_IO_PPI_IO_WRITE64;
typedef PEI_CPU_IO_PPI_MEM_READ8                     EFI_PEI_CPU_IO_PPI_MEM_READ8;
typedef PEI_CPU_IO_PPI_MEM_READ16                    EFI_PEI_CPU_IO_PPI_MEM_READ16;
typedef PEI_CPU_IO_PPI_MEM_READ32                    EFI_PEI_CPU_IO_PPI_MEM_READ32;
typedef PEI_CPU_IO_PPI_MEM_READ64                    EFI_PEI_CPU_IO_PPI_MEM_READ64;
typedef PEI_CPU_IO_PPI_MEM_WRITE8                    EFI_PEI_CPU_IO_PPI_MEM_WRITE8;
typedef PEI_CPU_IO_PPI_MEM_WRITE16                   EFI_PEI_CPU_IO_PPI_MEM_WRITE16;
typedef PEI_CPU_IO_PPI_MEM_WRITE32                   EFI_PEI_CPU_IO_PPI_MEM_WRITE32;
typedef PEI_CPU_IO_PPI_MEM_WRITE64                   EFI_PEI_CPU_IO_PPI_MEM_WRITE64;
typedef PEI_GET_VARIABLE                             EFI_PEI_GET_VARIABLE;
typedef PEI_GET_NEXT_VARIABLE_NAME                   EFI_PEI_GET_NEXT_VARIABLE_NAME;
typedef PEI_LOAD_RECOVERY_CAPSULE                    EFI_PEI_LOAD_RECOVERY_CAPSULE;
typedef PEI_RESET_PPI                                EFI_PEI_RESET_PPI;
typedef PEI_S3_RESUME_PPI                            EFI_PEI_S3_RESUME_PPI;
typedef PEI_S3_RESUME_PPI_RESTORE_CONFIG             EFI_PEI_S3_RESUME_PPI_RESTORE_CONFIG;
typedef SEC_PLATFORM_INFORMATION                     EFI_SEC_PLATFORM_INFORMATION;
typedef PEI_SECURITY_PPI                             EFI_PEI_SECURITY_PPI;
typedef PEI_SECURITY_AUTHENTICATION_STATE            EFI_PEI_SECURITY_AUTHENTICATION_STATE;
typedef PEI_STALL                                    EFI_PEI_STALL;
typedef PEI_SMBUS_PPI_EXECUTE_OPERATION              EFI_PEI_SMBUS_PPI_EXECUTE_OPERATION;     
typedef PEI_SMBUS_NOTIFY_FUNCTION                    EFI_PEI_SMBUS_NOTIFY_FUNCTION;           
typedef PEI_SMBUS_PPI_ARP_DEVICE                     EFI_PEI_SMBUS_PPI_ARP_DEVICE;            
typedef PEI_SMBUS_PPI_GET_ARP_MAP                    EFI_PEI_SMBUS_PPI_GET_ARP_MAP;           
typedef PEI_SMBUS_PPI_NOTIFY                         EFI_PEI_SMBUS_PPI_NOTIFY;                
typedef PEI_BOOT_SCRIPT_EXECUTE                      EFI_PEI_BOOT_SCRIPT_EXECUTE;       
typedef PEI_BOOT_SCRIPT_EXECUTER_PPI                 EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI;  
typedef EFI_FIND_FV_FINDFV                           EFI_PEI_FIND_FV_FINDFV;                    
typedef EFI_FIND_FV_PPI                              EFI_PEI_FIND_FV_PPI;                       
typedef PEI_RECOVERY_BLOCK_IO_INTERFACE              EFI_PEI_RECOVERY_BLOCK_IO_PPI;             
typedef PEI_LBA                                      EFI_PEI_LBA;                               
typedef PEI_BLOCK_IO_MEDIA                           EFI_PEI_BLOCK_IO_MEDIA;                    
typedef PEI_BLOCK_DEVICE_TYPE                        EFI_PEI_BLOCK_DEVICE_TYPE;                 
typedef PEI_GET_NUMBER_BLOCK_DEVICES                 EFI_PEI_GET_NUMBER_BLOCK_DEVICES;          
typedef PEI_GET_DEVICE_MEDIA_INFORMATION             EFI_PEI_GET_DEVICE_MEDIA_INFORMATION;      
typedef PEI_READ_BLOCKS                              EFI_PEI_READ_BLOCKS;                       
typedef PEI_DEVICE_RECOVERY_MODULE_INTERFACE         EFI_PEI_DEVICE_RECOVERY_MODULE_PPI;        
typedef PEI_DEVICE_GET_NUMBER_RECOVERY_CAPSULE       EFI_PEI_DEVICE_GET_NUMBER_RECOVERY_CAPSULE;
typedef PEI_DEVICE_GET_RECOVERY_CAPSULE_INFO         EFI_PEI_DEVICE_GET_RECOVERY_CAPSULE_INFO;  
typedef PEI_DEVICE_LOAD_RECOVERY_CAPSULE             EFI_PEI_DEVICE_LOAD_RECOVERY_CAPSULE;      


#endif
